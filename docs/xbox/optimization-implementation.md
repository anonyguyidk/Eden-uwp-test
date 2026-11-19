<!-- coderabbit-ai-generated:v1 -->

# Remaining optimization implementation

The branch now contains experimental implementations of the three remaining mechanisms. They are
not certified Xbox builds: Windows UWP compilation and GPU integration remain outstanding.

## Controls

All controls are global choices in Settings > System and compose with boot.cfg and
LocalState/optimization/<16-digit-lowercase-title-id>.cfg overrides at the next game launch.

| Key | Values | Default | Behavior |
| --- | --- | --- | --- |
| astc_mip_drop | 0, 1, 2 | 0 | Physically omit top host mip levels on eligible ASTC images |
| astc_mip0 | keep, drop | alias | Alias for astc_mip_drop=0/1 |
| shader_bytecode_policy | memory, disk | disk | Spill live signed graphics DXIL after successful PSO creation |
| software_tlb_entries | 0, 1, 2 | 0 | Disable, one-entry, or two-entry direct-mapped emitted x86-64 translation cache |

ASTC omission intentionally changes texture resolution, shader-visible dimensions and explicit
LOD interpretation. It is not lossless. The implementation currently admits only power-of-two
2D images at least 1024x1024, one layer, no MSAA/sparse layout, and BC1/BC3-backed ASTC decoding.
Allocation dimensions and mip count shrink, retained uploads keep their original guest-buffer
positions, GPU decoding skips omitted levels, host subresource numbers and SRV ranges translate.
CPU-converted uploads skip omitted copies as well. Guest image metadata remains unchanged.

**Limitation:** alias/image copies, blits and readback of reduced images currently fail explicitly,
requesting astc_mip_drop=0, instead of submitting mismatched resource accesses. Pulse capture may
therefore require omission off. This is a constrained prototype, not general ASTC-copy support.
Mip omission stays off by default until those paths and target visual results are validated. This
departs from the earlier proposed default-on behavior to avoid enabling a known compatibility
restriction for all titles. Arrays, NPOT textures and small images retain every level.

## Disk-backed DXIL

New d3d12_bytecode_store.h writes bounded Zstd data to ShaderDir/live-dxil within the existing
LocalState data tree. Each immutable lease is session-scoped and deleted at normal destruction.
A verified read-back must match before raw DXIL can be released. A failed write keeps raw memory.
Reads bound compressed input and decompressed output, check the per-lease content checksum and
validate each stage length. Files are not imported as trusted cross-session shader records; the
existing d3d12.bin reconstruction cache remains responsible for later launches. Abnormal process
termination can leave orphan backing files, which may be removed while the emulator is stopped.

<!-- coderabbit-ai-generated:v1 -->

<!-- coderabbit-ai-generated-section:v1:4bbc3d0b238b69c5e8b3efce0d7098a097dc52a5d6bbeb5dad8b1cf8b993b443 -->

Dormant pipelines also spill their compiled bytecode before the first PSO build when disk mode is enabled. GraphicsPipeline reloads a transient lease for Wake/Build; diagnostic capture obtains its own

<!-- /coderabbit-ai-generated:v1 -->
bytecode lease. Probe PSOs hold leases through CreateGraphicsPipelineState. Diagnostic bytes can
remain resident after capture until pipeline destruction. Shader memory charges track restored
raw bytes and compressed PSO identities. The PSO key retains deterministic compressed full shader
bytes for collision-safe equality, not raw spans or a hash-only identity. Linked-shader raw
memoization is disabled in disk mode; the global shader_cache_mib choice applies in memory mode.
Raw SPIR-V in the initial compilation closure is released by the existing completed-job lifecycle.

## Emitted software TLB

A32/A64 JIT state holds two 32-byte slots appended to preserve existing member offsets. The emitter
checks the selected tag and mapping epoch, returns the cached absolute offset on a hit, and loads
the authoritative software table on a miss. Null entries follow the existing callback path and
are not cached. Existing bounds/alignment checks execute before probing; configurations with
pointer masks/marked bits/sign extension use the existing path. MMIO/debug/rasterizer-cached pages
remain null and therefore cannot enter the cache.

Common::PageTable::JitPageEntry::Store, ResizeAndClear and the bulk ZeroRegion path increment a
shared atomic mapping epoch after publishing changes. This invalidates across cores/address spaces
on mapping, permission/debug and rasterizer transitions. The memory manager's existing mapping and
reclamation synchronization remains required; the epoch is not a new concurrent reclamation scheme.
Epoch wrap is not specially handled (a 64-bit generation). A32/A64 mapping identity belongs to each
configured JIT; another address space cannot reuse its slots. The option defaults off pending full
Dynarmic integration and performance tests. No speedup is claimed over the baseline inline table load.

## CI installation artifact

Renamed .github/workflows/build.yml to build-uwp.yml and updated ci.yml/release.yml callers.
The package job stages xbox-install, with only appx/msix files, one EdenUWP.cer and installable
packages from Dependencies. VCLibs remains included. Scripts, appxsym, resources, symbols ZIP,
build-info.json and checksums are absent from the installation artifact. The CI release job now
gets the version from the main appx filename rather than the excluded build-info.json. Intermediate
build artifacts retain the executables/libraries required between jobs. Packaging and PowerShell
execution still require Windows; YAML parsing and local workflow-reference checks pass here.

## Validation

Portable tests cover configuration round-trips/invalid values, mip eligibility boundaries, global
UI navigation, bounded Zstd store round-trip and file lifecycle. A test extracts and executes the
production TLB emitter helper with pinned Xbyak v7.40.1, testing cached hits, misses, collisions,
epoch-driven remaps and null fallback. <!-- coderabbit-ai-generated:v1 -->

<!-- coderabbit-ai-generated-section:v1:b9838292d6adad13979ec64bc3cd5968f1d5b01c537536dfbd73ea4b88f64755 -->

The actual A32/A64 memory-emitter and interface translation units also pass native syntax checks with ARCHITECTURE_x86_64. This does not link the full Dynarmic runtime or validate its

<!-- /coderabbit-ai-generated:v1 -->
register allocator, callbacks, exclusive accesses, cross-core remapping or UWP W^X integration.

Remaining gates: Windows compilation/link/package, real D3D12 PSO eviction/recreation and captures,
ASTC GPU/CPU retained mip verification, unsupported copy fallback support, target TLB integration,
concurrent mapping stress, Xbox memory/per-frame draw timings and TotK stability. No APPX or target
performance numbers are produced in this Linux sandbox.
