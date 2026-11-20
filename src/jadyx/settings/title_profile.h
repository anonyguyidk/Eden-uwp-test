// SPDX-FileCopyrightText: Copyright 2026 Eden-UWP experimental fork contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include <sstream>
#include "jadyx/settings/graphics_settings.h"
#include "jadyx/settings/optimization_settings.h"

namespace Jadyx {
inline void ApplyTitleProfile(std::string_view text, OptimizationSettings& optimization, GraphicsSettings& graphics) {
    std::istringstream input{std::string{text}};
    for (std::string line; std::getline(input, line);) {
        if (!line.empty() && line.back() == '\r') line.pop_back();
        if (!optimization.Apply(line)) graphics.Apply(line);
    }
}
// Replace one key, preserving comments, unknown settings and partial inheritance. Empty
// assignment resets only the settings this frontend owns, including invalid old values.
inline std::string EditTitleProfile(std::string_view text, std::string_view assignment) {
    std::istringstream input{std::string{text}};
    std::string output;
    const auto key = assignment.substr(0, assignment.find('='));
    const auto known = OptimizationSettings{}.Serialize() + GraphicsSettings{}.Serialize();
    for (std::string line; std::getline(input, line);) {
        if (!line.empty() && line.back() == '\r') line.pop_back();
        const auto equal = line.find('=');
        const auto current = std::string_view{line}.substr(0, equal);
        const bool alias = current == "astc_mip0";
        const bool remove = assignment.empty() ?
            equal != std::string::npos && (known.starts_with(std::string{current} + "=") ||
                known.find("\n" + std::string{current} + "=") != std::string::npos || alias) :
            current == key || (key == "astc_mip_drop" && alias);
        if (!remove) output += line + "\n";
    }
    if (!assignment.empty()) output += std::string{assignment} + "\n";
    return output;
}
} // namespace Jadyx
