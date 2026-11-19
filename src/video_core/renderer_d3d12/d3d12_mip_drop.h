// SPDX-FileCopyrightText: Copyright 2026 Eden-UWP experimental fork contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include <algorithm>
#include <bit>
#include <cstdint>
#include "video_core/renderer_d3d12/d3d12_optimization_settings.h"
namespace D3D12 {
inline unsigned AstcMipDrop(unsigned requested, unsigned width, unsigned height, unsigned levels,
                            unsigned layers, bool eligible) {
    if (!eligible || layers != 1 || width < 1024 || height < 1024 ||
        !std::has_single_bit(width) || !std::has_single_bit(height)) return 0;
    return levels ? std::min({requested, levels - 1, 2U}) : 0;
}
}
