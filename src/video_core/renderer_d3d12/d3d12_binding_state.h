// SPDX-FileCopyrightText: Copyright 2026 Eden-UWP experimental fork contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <algorithm>
#include <array>
#include <cstdint>
#include <optional>
#include <span>

namespace D3D12 {

class BindingState {
public:
    struct Vertex {
        std::uint64_t address{};
        std::uint32_t size{}, stride{};
        bool operator==(const Vertex&) const = default;
    };

    void Invalidate() { *this = {}; }
    bool Root(std::uintptr_t signature) {
        const bool same = root == signature;
        root = signature;
        return same;
    }
    bool Vertices(std::uint32_t first, std::span<const Vertex> views) {
        if (first > vertices.size() || views.size() > vertices.size() - first) {
            vertices = {};
            return false;
        }
        bool same = true;
        for (std::size_t i = 0; i < views.size(); ++i) {
            same &= vertices[first + i] == views[i];
            vertices[first + i] = views[i];
        }
        return same;
    }
    bool Targets(std::uint32_t count, bool range, std::span<const std::uintptr_t> handles,
                 std::uintptr_t depth, std::uint64_t generation) {
        if (handles.size() > targets.size()) { targets_valid = false; return false; }
        const bool same = targets_valid && target_count == count && single_range == range &&
            target_handles == handles.size() && dsv == depth && allocation_generation == generation &&
            std::equal(handles.begin(), handles.end(), targets.begin());
        std::copy(handles.begin(), handles.end(), targets.begin());
        target_count = count;
        target_handles = handles.size();
        single_range = range;
        dsv = depth;
        allocation_generation = generation;
        targets_valid = true;
        return same;
    }

private:
    std::optional<std::uintptr_t> root;
    std::array<std::optional<Vertex>, 32> vertices{};
    std::array<std::uintptr_t, 8> targets{};
    std::size_t target_handles{};
    std::uint32_t target_count{};
    std::uintptr_t dsv{};
    std::uint64_t allocation_generation{};
    bool single_range{}, targets_valid{};
};

} // namespace D3D12
