// SPDX-FileCopyrightText: Copyright 2026 Eden-UWP experimental fork contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <algorithm>
#include <atomic>

#include "common/common_types.h"

namespace D3D12 {

inline std::atomic_bool redundant_bind_skipping{false};
inline std::atomic_bool disk_bytecode_enabled{false};
inline std::atomic<unsigned> astc_mip_drop{};

inline void SetRedundantBindSkipping(bool enabled) {
    redundant_bind_skipping.store(enabled, std::memory_order_relaxed);
}

inline void SetDiskBytecode(bool enabled) {
    disk_bytecode_enabled.store(enabled, std::memory_order_relaxed);
}

inline void SetAstcMipDrop(unsigned levels) {
    astc_mip_drop.store(std::min(levels, 2U), std::memory_order_relaxed);
}

void SetUploadRingSize(u32 mib);
void SetPipelineTuning(u32 shader_cache_mib, u32 eviction_aggressiveness);

} // namespace D3D12
