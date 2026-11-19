// SPDX-FileCopyrightText: Copyright 2026 Eden-UWP experimental fork contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#include <cassert>
#include "video_core/renderer_d3d12/d3d12_bytecode_store.h"
namespace Common::FS {
const std::filesystem::path& GetEdenPath(EdenPath) {
    static const auto root = std::filesystem::temp_directory_path() /
        ("eden-bytecode-test-" + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
    return root;
}
}
int main() {
    std::vector<u8> bytes(8192);
    for (size_t i=0;i<bytes.size();++i) bytes[i]=static_cast<u8>(i);
    auto store=D3D12::BytecodeStore::Write(bytes);
    assert(store && store->Read()==bytes);
    auto compressed=Common::Compression::CompressDataZSTDDefault(bytes.data(),bytes.size());
    assert(Common::Compression::DecompressDataZSTDBounded(compressed,8191).empty());
    assert(Common::Compression::DecompressDataZSTDBounded(compressed,8192)==bytes);
    assert(Common::Compression::DecompressDataZSTDBounded({},8192).empty());
    assert(!D3D12::BytecodeStore::Write({}));
    const auto directory = Common::FS::GetEdenPath(Common::FS::EdenPath::ShaderDir) / "live-dxil";
    const auto filename = std::filesystem::directory_iterator(directory)->path();
    { std::ofstream corrupt{filename, std::ios::binary | std::ios::trunc}; corrupt << "corrupted"; }
    assert(store->Read().empty());
    store.reset();
    assert(std::filesystem::is_empty(Common::FS::GetEdenPath(Common::FS::EdenPath::ShaderDir) / "live-dxil"));
    const auto root = Common::FS::GetEdenPath(Common::FS::EdenPath::ShaderDir);
    std::filesystem::remove_all(root);
    { std::ofstream obstruction{root}; obstruction << "not a directory"; }
    assert(!D3D12::BytecodeStore::Write(bytes)); // failed backing writes preserve caller's raw data
    std::filesystem::remove(root);
}
