#!/usr/bin/env python3
# SPDX-FileCopyrightText: Copyright 2026 Eden-UWP experimental fork contributors
# SPDX-License-Identifier: GPL-3.0-or-later

"""Compile the boot.cfg optimization branch in its global-namespace context."""
import os
from pathlib import Path
import subprocess
import tempfile

root = Path(__file__).resolve().parents[3]
source = (root / 'src/jadyx/app/uwp_boot.cpp').read_text(encoding='utf-8')
start = source.index('} else if (line.starts_with("jit_cache_mib=")')
end = source.index('} else if (line == "readback_uncached=1"', start)
branch = source[start:end].strip()
program = r'''
#include <cassert>
#include <string>
#include <vector>
#include "jadyx/settings/optimization_settings.h"
#include "jadyx/settings/graphics_settings.h"
struct Config { std::vector<std::string> optimization_overrides; };
void WriteDiag(const std::string&) {}
bool Parse(const std::string& line, Config& config) {
    if (false) { }
    BRANCH
    } else { return false; }
    return true;
}
int main() {
    Config config;
    assert(Parse("jit_cache_mib=32", config));
    assert(Parse("shader_bytecode_policy=disk", config));
    assert(Parse("astc_mip0=drop", config));
    assert(Parse("software_tlb_entries=2", config));
    assert(config.optimization_overrides.size() == 4);
    assert(Parse("jit_cache_mib=31", config));
    assert(config.optimization_overrides.size() == 4);
    assert(Parse("graphics_fsr1=1", config));
    assert(Parse("graphics_rcas=200", config));
    assert(Parse("graphics_docked=0", config));
    assert(Parse("graphics_aspect=4", config));
    assert(config.optimization_overrides.size() == 8);
    assert(Parse("graphics_rcas=201", config));
    assert(Parse("graphics_fsr3=1", config));
    assert(config.optimization_overrides.size() == 8);
    assert(!Parse("unrelated=1", config));
}
'''.replace('BRANCH', branch.removeprefix('} '))
with tempfile.TemporaryDirectory() as tmp:
    src = Path(tmp) / 'test.cpp'
    binary = Path(tmp) / 'test'
    src.write_text(program, encoding='utf-8')
    subprocess.run([os.environ.get('CXX', 'c++'), '-std=c++20', '-Wall', '-Wextra',
                    '-Werror', '-I', str(root / 'src'), str(src), '-o', str(binary)], check=True)
    subprocess.run([str(binary)], check=True)
print('Boot optimization parsing in its global namespace: passed')
