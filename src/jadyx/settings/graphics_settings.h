// SPDX-FileCopyrightText: Copyright 2026 Eden-UWP experimental fork contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include <charconv>
#include <cstdint>
#include <string>
#include <string_view>

namespace Jadyx {
struct GraphicsSettings {
    bool docked{true};
    bool fsr{};
    std::uint32_t sharpness{25}; // RCAS stops x 100: lower is sharper.
    std::uint32_t aspect_ratio{};
    bool operator==(const GraphicsSettings&) const = default;
    bool Apply(std::string_view line) {
        const auto equal = line.find('=');
        if (equal == std::string_view::npos) return false;
        const auto key = line.substr(0, equal), value = line.substr(equal + 1);
        std::uint32_t number{};
        const auto parsed = std::from_chars(value.data(), value.data() + value.size(), number);
        if (parsed.ec != std::errc{} || parsed.ptr != value.data() + value.size()) return false;
        if (key == "graphics_docked" && number <= 1) docked = number != 0;
        else if (key == "graphics_fsr1" && number <= 1) fsr = number != 0;
        else if (key == "graphics_rcas" && number <= 200) sharpness = number;
        else if (key == "graphics_aspect" && number <= 4) aspect_ratio = number;
        else return false;
        return true;
    }
    std::string Serialize() const {
        return "graphics_docked=" + std::to_string(docked) +
            "\ngraphics_fsr1=" + std::to_string(fsr) +
            "\ngraphics_rcas=" + std::to_string(sharpness) +
            "\ngraphics_aspect=" + std::to_string(aspect_ratio) + "\n";
    }
};
} // namespace Jadyx
