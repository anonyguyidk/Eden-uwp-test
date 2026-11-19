#!/usr/bin/env python3
# SPDX-FileCopyrightText: Copyright 2026 Eden-UWP experimental fork contributors
# SPDX-License-Identifier: GPL-3.0-or-later
"""Execute the production TLB emitter helper with a small Xbyak register allocator adapter."""
import pathlib
import subprocess
import sys
import tempfile

root = pathlib.Path(__file__).resolve().parents[3]
source = (root / 'src/dynarmic/src/dynarmic/backend/x64/emit_x64_memory.h').read_text()
start = source.index('template<typename EmitContext>\nbool EmitSoftwareTlb(')
end = source.index('template<typename EmitContext>\nXbyak::RegExp EmitVAddrLookup', start)
helper = source[start:end]
program = r'''
#include <atomic>
#include <array>
#include <cassert>
#include <cstdint>
#include <optional>
#include <xbyak/xbyak.h>
using namespace Xbyak::util;
using u64 = std::uint64_t;
struct BlockOfCode : Xbyak::CodeGenerator {
    const Xbyak::Reg64 ABI_JIT_PTR{r15};
    struct Info { int offsetof_software_tlb = 0; };
    Info GetJitStateInfo() { return {}; }
};
struct Context {
    struct Config {
        const std::atomic<u64>* page_table_epoch;
        unsigned software_tlb_entries;
        bool absolute_offset_page_table = true;
        u64 page_table_pointer_mask = 0;
        std::optional<unsigned> page_table_marked_bit, page_table_sign_extension;
        unsigned page_table_log2_stride = 3;
    } conf;
    struct Registers {
        unsigned allocated = 0;
        Xbyak::Reg64 ScratchGpr(BlockOfCode&) {
            return Xbyak::Reg64(8 + allocated++);
        }
        void Release(Xbyak::Reg64) {}
    } reg_alloc;
};
'''
program += helper
program += r'''
int main() {
    for (unsigned count : {1U, 2U}) {
        std::atomic<u64> epoch{1};
        Context context{};
        context.conf.page_table_epoch = &epoch;
        context.conf.software_tlb_entries = count;
        BlockOfCode code;
        Xbyak::Label abort;
        code.push(r14); code.push(r15);
        code.mov(r14, rsi); code.mov(r15, rdx); code.mov(rax, rdi);
        assert(EmitSoftwareTlb(code, context, abort, rax, rax));
        code.pop(r15); code.pop(r14); code.ret();
        code.L(abort);
        code.xor_(rax, rax); code.pop(r15); code.pop(r14); code.ret();
        code.ready();
        using Fn = u64 (*)(u64, const u64*, u64*);
        const auto run = code.getCode<Fn>();
        std::array<u64, 8> slots{};
        std::array<u64, 4> table{111, 222, 333, 0};
        assert(run(0, table.data(), slots.data()) == 111); // miss
        table[0] = 999; // Deliberately omit invalidation to distinguish a cached hit.
        assert(run(0, table.data(), slots.data()) == 111);
        epoch.fetch_add(1, std::memory_order_release);
        assert(run(0, table.data(), slots.data()) == 999); // remap
        assert(run(1, table.data(), slots.data()) == 222);
        assert(run(2, table.data(), slots.data()) == 333); // direct-map collision
        assert(run(0, table.data(), slots.data()) == 999);
        table[0] = 0;
        epoch.fetch_add(1, std::memory_order_release);
        assert(run(0, table.data(), slots.data()) == 0); // callback path
        assert(run(3, table.data(), slots.data()) == 0); // null entries never cached
        table[0] = 111;
        epoch.fetch_add(1, std::memory_order_release);
        assert(run(0, table.data(), slots.data()) == 111);
    }
}
'''
with tempfile.TemporaryDirectory(prefix='eden-tlb-emitter-') as directory:
    source_path = pathlib.Path(directory) / 'test.cpp'
    source_path.write_text(program)
    executable = pathlib.Path(directory) / 'test'
    subprocess.run(['g++', '-std=c++20', '-O1', '-Wall', '-Wextra', '-Werror',
                    '-I' + sys.argv[1], str(source_path), '-o', str(executable)], check=True)
    subprocess.run([str(executable)], check=True)
print('production software TLB emitter: hit, miss, collision, remap and null fallback pass')
