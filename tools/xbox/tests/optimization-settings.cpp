// SPDX-FileCopyrightText: Copyright 2026 Eden-UWP experimental fork contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#include <cassert>
#include <sstream>
#include "video_core/renderer_d3d12/d3d12_mip_drop.h"
#include "jadyx/settings/optimization_settings.h"
#include "video_core/renderer_d3d12/d3d12_binding_state.h"

int main() {
    Jadyx::OptimizationSettings settings;
    assert(settings.jit_cache_mib == 48 && settings.upload_ring_mib == 64);
    for (const auto* invalid : {"jit_cache_mib=0", "jit_cache_mib=512", "jit_cache_mib=49",
                               "jit_cache_mib=31", "jit_cache_mib=-32", "jit_cache_mib=32junk",
                               "jit_cache_mib=4294967296", "jit_cache_mib=", "jit_cache_mib",
                               "upload_ring_mib=33", "shader_eviction=0", "shader_eviction=5",
                               "redundant_binds=2", "shader_cache_mib=17", "unknown=1"}) {
        const auto original = settings;
        assert(!settings.Apply(invalid));
        assert(settings == original);
    }
    assert(settings.Apply("jit_cache_mib=32"));
    assert(settings.Apply("upload_ring_mib=128"));
    assert(settings.Apply("shader_cache_mib=0"));
    assert(settings.Apply("shader_eviction=4"));
    assert(settings.Apply("redundant_binds=0"));
    assert(settings.Apply("shader_bytecode_policy=memory"));
    assert(!settings.Apply("shader_bytecode_policy=bogus"));
    assert(settings.Apply("astc_mip0=drop") && settings.astc_mip_drop == 1);
    assert(settings.Apply("astc_mip_drop=2"));
    assert(settings.Apply("software_tlb_entries=2"));
    assert(!settings.Apply("software_tlb_entries=3"));
    assert(!settings.Apply("astc_mip_drop=3"));
    assert(D3D12::AstcMipDrop(2, 2048, 1024, 12, 1, true) == 2);
    assert(D3D12::AstcMipDrop(2, 2048, 1024, 2, 1, true) == 1);
    assert(D3D12::AstcMipDrop(2, 2048, 1024, 1, 1, true) == 0);
    assert(D3D12::AstcMipDrop(2, 2047, 1024, 12, 1, true) == 0);
    assert(D3D12::AstcMipDrop(2, 2048, 1024, 12, 2, true) == 0);
    assert(D3D12::AstcMipDrop(2, 512, 1024, 12, 1, true) == 0);
    assert(D3D12::AstcMipDrop(2, 2048, 1024, 12, 1, false) == 0);
    Jadyx::OptimizationSettings restored;
    std::istringstream saved{settings.Serialize()};
    for (std::string line; std::getline(saved, line);) assert(restored.Apply(line));
    assert(restored == settings);
    // A partial per-game file overrides only its own keys; a bad line preserves prior values.
    assert(restored.Apply("jit_cache_mib=48"));
    assert(!restored.Apply("upload_ring_mib=999"));
    assert(restored.jit_cache_mib == 48 && restored.upload_ring_mib == 128);

    D3D12::BindingState state;
    assert(!state.Root(0));
    assert(state.Root(0));
    assert(!state.Root(100));
    assert(state.Root(100));
    const std::array<D3D12::BindingState::Vertex, 2> vertices{{{1000, 256, 16}, {2000, 128, 8}}};
    assert(!state.Vertices(1, vertices));
    assert(state.Vertices(1, vertices));
    assert(state.Vertices(2, std::span{vertices}.last(1)));
    auto changed = vertices;
    changed[1].stride = 32;
    assert(!state.Vertices(1, changed));
    assert(!state.Vertices(31, vertices)); // out of range invalidates vertex state
    assert(!state.Vertices(1, vertices));
    const std::array<std::uintptr_t, 2> targets{10, 20};
    assert(!state.Targets(2, false, targets, 30, 1));
    assert(state.Targets(2, false, targets, 30, 1));
    assert(!state.Targets(2, false, targets, 30, 2)); // recycled descriptor, same address
    assert(!state.Targets(2, false, targets, 31, 2)); // read-only DSV switch
    assert(!state.Targets(1, true, std::span{targets}.first(1), 31, 2));
    assert(!state.Targets(0, false, {}, 0, 2));
    assert(state.Targets(0, false, {}, 0, 2));
    state.Invalidate(); // list reset, custom Record or ExecuteIndirect
    assert(!state.Root(100));
    assert(!state.Vertices(1, vertices));
    assert(!state.Targets(0, false, {}, 0, 2));
}
