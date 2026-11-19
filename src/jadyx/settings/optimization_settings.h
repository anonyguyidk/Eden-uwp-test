// SPDX-FileCopyrightText: Copyright 2026 Eden-UWP experimental fork contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <charconv>
#include <cstdint>
#include <string>
#include <string_view>

namespace Jadyx {

struct OptimizationSettings {
    std::uint32_t jit_cache_mib{48};
    std::uint32_t upload_ring_mib{64};
    std::uint32_t shader_cache_mib{4};
    std::uint32_t eviction_aggressiveness{1};
    bool redundant_binds{true};
    bool disk_bytecode{true};
    std::uint32_t software_tlb_entries{};
    std::uint32_t astc_mip_drop{};

    bool operator==(const OptimizationSettings&) const = default;

    // Apply one complete, validated assignment. Unknown keys are left to boot.cfg's parser.
    bool Apply(std::string_view line) {
        if (line == "astc_mip0=drop") { astc_mip_drop = 1; return true; }
        if (line == "astc_mip0=keep") { astc_mip_drop = 0; return true; }
        const auto equal = line.find('=');
        if (equal == std::string_view::npos) return false;
        const auto key = line.substr(0, equal);
        const auto value = line.substr(equal + 1);
        if (key == "shader_bytecode_policy" && (value == "disk" || value == "memory")) {
            disk_bytecode = value == "disk";
            return true;
        }
        std::uint32_t number{};
        const auto parsed = std::from_chars(value.data(), value.data() + value.size(), number);
        if (parsed.ec != std::errc{} || parsed.ptr != value.data() + value.size()) return false;
        if (key == "astc_mip_drop" && number <= 2) {
            astc_mip_drop = number;
        } else if (key == "software_tlb_entries" && number <= 2) {
            software_tlb_entries = number;
        } else if (key == "jit_cache_mib" && number >= 32 && number <= 48) {
            jit_cache_mib = number;
        } else if (key == "upload_ring_mib" && (number == 32 || number == 64 || number == 128)) {
            upload_ring_mib = number;
        } else if (key == "shader_cache_mib" && number <= 16) {
            shader_cache_mib = number;
        } else if (key == "shader_eviction" && number >= 1 && number <= 4) {
            eviction_aggressiveness = number;
        } else if (key == "redundant_binds" && number <= 1) {
            redundant_binds = number != 0;
        } else {
            return false;
        }
        return true;
    }

    std::string Serialize() const {
        return "jit_cache_mib=" + std::to_string(jit_cache_mib) +
            "\nupload_ring_mib=" + std::to_string(upload_ring_mib) +
            "\nshader_cache_mib=" + std::to_string(shader_cache_mib) +
            "\nshader_eviction=" + std::to_string(eviction_aggressiveness) +
            "\nastc_mip_drop=" + std::to_string(astc_mip_drop) +
            "\nsoftware_tlb_entries=" + std::to_string(software_tlb_entries) +
            "\nshader_bytecode_policy=" + std::string(disk_bytecode ? "disk" : "memory") +
            "\nredundant_binds=" + std::to_string(redundant_binds) + "\n";
    }
};

} // namespace Jadyx
