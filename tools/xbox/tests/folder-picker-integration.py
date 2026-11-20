#!/usr/bin/env python3
# SPDX-FileCopyrightText: Copyright 2026 Eden-UWP experimental fork contributors
# SPDX-License-Identifier: GPL-3.0-or-later
"""Compile the actual folder truncation branch with WinRT's constructor constraint."""
import os
from pathlib import Path
import subprocess
import sys
import tempfile

root = Path(__file__).resolve().parents[3]
source_path = Path(sys.argv[1]) if len(sys.argv) > 1 else root / 'src/jadyx/library/uwp_library.cpp'
source = source_path.read_text(encoding='utf-8')
start = source.index('if (browsed.children.size() > 2048) {')
end = source.index('for (const auto& child : browsed.children)', start)
branch = source[start:end]
program = r'''
#include <cassert>
#include <vector>
#include "jadyx/storage/development_folder_browser.h"
// StorageFolder has a nullptr constructor, but no default constructor.
struct Folder {
    Folder() = delete;
    explicit Folder(unsigned id_) : id{id_} {}
    unsigned id;
};
struct Listing { std::vector<Folder> children; };
int main() {
    for (const unsigned count : {0, 1, 2048, 2049}) {
        Listing browsed;
        for (unsigned i = 0; i < count; ++i) browsed.children.emplace_back(i);
        Jadyx::DevelopmentFolderBrowser browser;
        BRANCH
        assert(browsed.children.size() == (count > 2048 ? 2048 : count));
        assert(browser.notice.empty() == (count <= 2048));
        for (unsigned i = 0; i < browsed.children.size(); ++i) assert(browsed.children[i].id == i);
    }
}
'''.replace('BRANCH', branch)
with tempfile.TemporaryDirectory() as temporary:
    src = Path(temporary) / 'folder-test.cpp'
    binary = Path(temporary) / 'folder-test'
    src.write_text(program, encoding='utf-8')
    subprocess.run([os.environ.get('CXX', 'c++'), '-std=c++20', '-Wall', '-Wextra', '-Werror',
                    '-I' + str(root / 'src'), str(src), '-o', str(binary)], check=True)
    subprocess.run([str(binary)], check=True)
print('Actual folder truncation branch with a non-default-constructible folder: passed')
