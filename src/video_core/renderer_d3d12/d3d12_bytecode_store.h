// SPDX-FileCopyrightText: Copyright 2026 Eden-UWP experimental fork contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <atomic>
#include <chrono>
#include <exception>
#include <filesystem>
#include <fstream>
#include <memory>
#include <vector>
#include "common/fs/path_util.h"
#include "common/zstd_compression.h"

namespace D3D12 {
inline std::atomic_bool disk_bytecode_enabled{false};
inline void SetDiskBytecode(bool enabled) {
    disk_bytecode_enabled.store(enabled, std::memory_order_relaxed);
}

// A session-scoped, immutable backing file. The signed shader cache remains the rebuild source
// across launches; these leases only replace live raw DXIL ownership during the current launch.
class BytecodeStore {
public:
    ~BytecodeStore() {
        std::error_code error;
        std::filesystem::remove(path, error);
    }
    static std::shared_ptr<BytecodeStore> Write(std::span<const u8> bytes) try {
        if (bytes.empty() || bytes.size() > MAX_BYTES) return {};
        auto compressed = Common::Compression::CompressDataZSTDDefault(bytes.data(), bytes.size());
        if (compressed.empty()) return {};
        auto result = std::make_shared<BytecodeStore>();
        static std::atomic<u64> sequence{};
        static const auto session = std::chrono::steady_clock::now().time_since_epoch().count();
        const auto directory = Common::FS::GetEdenPath(Common::FS::EdenPath::ShaderDir) / "live-dxil";
        std::error_code error;
        std::filesystem::create_directories(directory, error);
        if (error) return {};
        result->checksum = Hash(bytes);
        result->path = directory / (std::to_string(session) + "-" +
                                  std::to_string(sequence.fetch_add(1)) + ".zst");
        std::ofstream file{result->path, std::ios::binary | std::ios::trunc};
        file.write(reinterpret_cast<const char*>(compressed.data()), compressed.size());
        file.close();
        if (!file) return {};
        // Verify a fresh read before dropping the only raw owner.
        if (result->Read() != std::vector<u8>(bytes.begin(), bytes.end())) return {};
        return result;
    } catch (const std::exception&) {
        return {};
    }
    std::vector<u8> Read() const {
        std::ifstream file{path, std::ios::binary | std::ios::ate};
        if (!file) return {};
        const auto size = file.tellg();
        if (size <= 0 || size > static_cast<std::streamoff>(MAX_BYTES * 2)) return {};
        std::vector<u8> bytes(static_cast<size_t>(size));
        file.seekg(0);
        file.read(reinterpret_cast<char*>(bytes.data()), bytes.size());
        if (!file) return {};
        auto raw = Common::Compression::DecompressDataZSTDBounded(bytes, MAX_BYTES);
        return Hash(raw) == checksum ? raw : std::vector<u8>{};
    }
private:
    static u64 Hash(std::span<const u8> bytes) {
        u64 hash = 14695981039346656037ULL;
        for (const u8 byte : bytes) hash = (hash ^ byte) * 1099511628211ULL;
        return hash;
    }
    u64 checksum{};
    static constexpr size_t MAX_BYTES = 64 * 1024 * 1024;
    std::filesystem::path path;
};
} // namespace D3D12
