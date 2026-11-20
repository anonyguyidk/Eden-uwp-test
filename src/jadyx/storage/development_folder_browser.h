// SPDX-FileCopyrightText: Copyright 2026 Eden-UWP experimental fork contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include <algorithm>
#include <string>
#include <vector>

namespace Jadyx {
struct DevelopmentFolderBrowser {
    static constexpr auto Root = L"D:\\DevelopmentFiles";
    static constexpr size_t UseFolder = 0, Parent = 1, Cancel = 2, FirstFolder = 3;
    bool open{}, busy{}, ready{};
    size_t selected{};
    std::wstring path, notice;
    std::vector<std::wstring> folders;
    size_t RowCount() const { return FirstFolder + folders.size(); }
    void Move(bool back) {
        selected = (selected + (back ? RowCount() - 1 : 1)) % RowCount();
    }
};
} // namespace Jadyx
