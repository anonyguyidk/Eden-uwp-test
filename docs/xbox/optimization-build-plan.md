<!-- coderabbit-ai-generated:v1 -->

<!-- coderabbit-ai-generated:v1 -->

<!-- coderabbit-ai-generated-section:v1:17e96516b7d371b1b6174e795aca92560243d519d19d77aea461bb4ea9971287 -->

# Experimental UWP optimization patch series

This is the original staged design. See [the follow-up implementation status](optimization-implementation.md) for code now added, current controls, remaining compatibility restrictions and validation.

<!-- /coderabbit-ai-generated:v1 -->

## Baseline and delivery

Source: PrismFox67/Eden-UWP main, full revision
`d06a11e886133a5970766f7b5f8c854de11548e5`. The connected target repository was empty;
its assigned optimization branch now starts at this imported history. No changes are sent upstream.
The current working diff is the implementation of patch 1 below. Patches 2–4 are architecture
and acceptance criteria, not completed implementations. The inherited instruction to leave
publication to the owner is preserved; this delivery contains a local branch and an applicable diff.

The user's AI-assisted experimental fork request intentionally departs from the inherited
`docs/policies/AI.md` prohibition. Copyright and license notices remain intact. README attribution
separates AI-generated fork changes from the original authors' code.

## Patch 1: configurable memory and command recording foundation (implemented)

| Exact file | Change |
| --- | --- |
| `CMakePresets.json` | `uwp-memory` configure/build presets; independent binary directory |
| `src/jadyx/settings/optimization_settings.h` | Defaults, strict validation, serialization and partial overrides |
| `src/jadyx/settings/system_settings.h` | Saved global optimization profile |
| `src/jadyx/settings/uwp_system_settings.cpp` | Versioned LocalSettings persistence |
| `src/jadyx/settings/system_page.h` | Controller/keyboard choices and displayed values |
| `src/jadyx/library/uwp_library_canvas.cpp` | Choice labels/hints in the existing scrolling settings list |
| `src/jadyx/app/uwp_boot.cpp` | Compose UI + boot.cfg + per-title overrides before System::Load creates renderer/JITs |
| `src/video_core/renderer_d3d12/d3d12_binding_state.h` | Portable root/VB/RT state cache and invalidation |
| `src/video_core/renderer_d3d12/d3d12_command_stream.h` | Skip redundant calls; invalidate for Record and ExecuteIndirect |
| `src/video_core/renderer_d3d12/d3d12_descriptor_heap.h` and `.cpp` | Attachment allocation generation guards handle reuse |
| `src/video_core/renderer_d3d12/d3d12_staging_buffer_pool.h` and `.cpp` | Set per-launch ring ceiling; retain existing fence retirement and fallback |
| `src/video_core/renderer_d3d12/d3d12_pipeline_cache.h` and `.cpp` | Linked-shader memoization budget and idle eviction multiplier |
| `tools/xbox/tests/optimization-settings.cpp` | Invalid/overflow config, inheritance, serialization, partial VB overlap, descriptor reuse and reset cases |
| `tools/xbox/tests/system-settings.cpp` | UI choice/navigation coverage; portable UTF-16 fixture and assertion diagnostics |
| `README.md` | Experimental disclaimer, badges, setup, controls and truthful status |

Existing paths reused unchanged:

- `src/core/arm/dynarmic/arm_dynarmic.h`: SetJitCodeCacheSize/JitCodeCacheSize.
- `src/core/arm/dynarmic/arm_dynarmic_64.cpp` and `arm_dynarmic_32.cpp`: per-JIT configuration.
- `src/dynarmic/src/dynarmic/backend/x64/a64_interface.cpp` and `a32_interface.cpp`: cache exhaustion clears code, block links and metadata. This is not ring eviction.
- `src/video_core/renderer_d3d12/d3d12_uniform_stream.h` and `d3d12_buffer_cache.cpp`: CBVs already suballocate 256-byte-aligned slices from 4 KiB staging reservations, then the persistent upload ring. Tick changes invalidate reservations. Replacing this with a second ring would duplicate memory and fence policy.
- `src/video_core/renderer_d3d12/d3d12_scheduler.cpp`: submission/reset passes through CommandStream::Record, which now invalidates state on the producer timeline before recording the next list. The worker never accesses BindingState.

Controls apply on a fresh game launch, not to an already running or suspended Pulse session.
Saved Pulse state is not migrated by this patch. The current UI exposes global choices; per-game
changes are text-file overrides, not an additional per-game editing screen. Labels need translation.
Existing `jit_cache_mib` values above 48 are intentionally rejected in this optimization branch;
this narrows the baseline's 32–512 range to the requested memory cap. Other frontends retain their
JIT defaults. The staging pool retains 128 MiB when no UWP override is applied.

<!-- coderabbit-ai-generated:v1 -->

<!-- coderabbit-ai-generated-section:v1:2c533f1a3fe97c8def0a7dff9cfce8485e81110110a5290c9f13d8e9d1da26af -->

The baseline rasterizer already suppresses many root-signature and RT binds in `d3d12_rasterizer_state.cpp`; patch 1 adds recorder-wide tracking and vertex-view comparisons. Its incremental CPU benefit may be small and requires A/B measurement. Bindings compare fields instead of padded structs.

<!-- /coderabbit-ai-generated:v1 --> OM handles alone are insufficient because CPU
descriptor slots may be recycled before list reset: each attachment allocation changes a generation.
Opaque Record/ExecuteIndirect calls clear all cached bindings. Future raw-list mutations must keep
using Record or explicitly invalidate the cache. This is CPU call suppression, not a change to GPU draws.

## Patch 2: disk-backed shader bytecode ownership (designed, pending)

The requested immediate vector clear is incompatible with the baseline's ownership contracts:
`GraphicsPipeline::Wake` reconstructs PSOs from retained DXIL;
`GraphicsPsoKey` stores bytecode spans with an owning shared_ptr and compares complete bytes to
resolve hash collisions. <!-- coderabbit-ai-generated:v1 -->

<!-- coderabbit-ai-generated-section:v1:72970555cb1fb8f2df1aa1f2ed9c29d341f001165c73d899a26d2863e2024515 -->

`DiagnosticDxil` supplies diagnostic captures.

<!-- /coderabbit-ai-generated:v1 --> SPIR-V in the compile closure is
already freed when the initial job completes; the small linked-shader cache separately retains
SPIR-V-containing keys. Existing d3d12.bin stores reconstruction inputs, not a ready-to-load signed
DXIL store. A Zstd cache cannot simply be assumed to exist for that purpose.

Exact files for this stage:

- `src/video_core/renderer_d3d12/d3d12_pipeline_cache.cpp` / `.h`: publish full linked identity and disk record references; exclude transient raw input from long-lived callbacks.
- `src/video_core/renderer_d3d12/d3d12_graphics_pipeline.cpp` / `.h`: retain a disk record reference and binding metadata; acquire transient DXIL for Build/Wake, and release it after successful CreateGraphicsPipelineState and diagnostic work.
- `src/video_core/renderer_d3d12/d3d12_pso_key.h`, `d3d12_pso_cache.h`, `d3d12_linked_shader_cache.h`: replace raw-span ownership with collision-safe immutable identities; preserve complete-identity equality, shared PSO accounting and fence retirement.
- `src/video_core/renderer_d3d12/diagnostics/d3d12_full_capture.cpp` and `d3d12_draw_trace.cpp`: load transient stage bytes for diagnostics without retaining every pipeline's DXIL.
- New `src/video_core/renderer_d3d12/d3d12_bytecode_store.h` / `.cpp` plus `src/video_core/CMakeLists.txt`: versioned signed-DXIL store under the resolved ShaderDir in LocalState; use `src/common/zstd_compression.h`.

Implementation sequence:

1. Key every linked input with stage ordering/lengths, specialization, translation options, ABI/cache version and compiler/signer version. Keep a canonical collision resolution record; never change equality to a hash-only comparison.
2. Persist compressed signed DXIL with bounded stage lengths, checksums and atomic temporary-file replacement. Fully validate before allocation/decompression. Reject stale, truncated and oversized records.
3. Publish a durable record only after the write succeeds. On disk-full/access errors keep a bounded memory fallback or keep the PSO non-evictable; retain enough source to rebuild. Never discard the only rebuild data.
4. On Build/Wake acquire a transient shared bytecode lease, build, then release all raw owners including the PSO index and linked memoization. Keep diagnostic leases scoped. A failed PSO build must retain retry and vertex-flattening behavior.
5. Add a `shader_bytecode_policy=memory|disk` UI choice and per-title key only once this lifecycle is implemented. Existing `shader_cache_mib=0` disables memoization; it does not free live pipeline DXIL.

Gates: cold/warm/restart cache, signed DXIL validity, corruption, compiler version mismatch,
concurrent writes, disk-full, failed create + flattened retry, eviction/wake, hash collisions,
debug capture after eviction, and memory-ledger delta. Benchmark decompression and SSD stalls.
Do not advertise 300–400 MiB of shader savings without measured byte counts.

## Patch 3: physical ASTC mip omission (designed, pending)

`astc_mip0=drop` is absent from the pinned baseline. An SRV MostDetailedMip or sampler LOD
change leaves the top level physically allocated and cannot satisfy the RAM reduction request.
Retain guest image metadata and introduce an explicit guest-to-host mip mapping for eligible images.

Exact files:

- `src/video_core/renderer_d3d12/d3d12_texture_cache.cpp` / `.h`: choose drop count before host resource allocation; store host base mip and dimensions separately from guest identity.
- `src/video_core/renderer_d3d12/d3d12_astc_gpu_decoder.cpp`: omit decode/encode dispatches for dropped levels and compute retained physical dimensions.
- `src/video_core/renderer_d3d12/d3d12_image_transfer.cpp`: translate subresource indices, footprints and buffer offsets; skip dropped source payloads, preserve retained payload offsets.
- `src/video_core/renderer_d3d12/d3d12_image_view.cpp`: translate/clamp view ranges, preserve explicit LOD/texture-size semantics or exclude incompatible views.
<!-- coderabbit-ai-generated:v1 -->

<!-- coderabbit-ai-generated-section:v1:10484c4d40a5612b1e9f7dce8a42af7c829e10ab3cfd31a07722b67e8c2d55c2 -->

- `src/video_core/renderer_d3d12/d3d12_image_copy_blit.cpp` and `d3d12_framebuffer.cpp`: audit every copy/blit entry point, reject drop-ineligible mutable attachments and handle aliases/readbacks consistently.

<!-- /coderabbit-ai-generated:v1 -->
- The settings/UI/boot files in patch 1: canonical `astc_mip_drop=0|1|2`; alias `astc_mip0=drop` to 1. Enable 1 by default only in this experimental build after correctness gates.

Start with large (at least 1024x1024), sampled-only 2D ASTC images with a real mip chain. Exclude
single-level images, storage/render-target use, unusual views, arrays/cubemaps and cross-format
aliases until each has a test. Never downscale ASTC headers or guest addresses in place. A shader
using texelFetch/image size can observe the omission; preserve those semantics or exclude it.
Dropping mip 0 removes roughly 75% of a full 2D chain's texel payload; physical heap savings also
depend on alignment, allocation pooling and fragmentation. Verify BC1/BC3 and alpha quality.

Gates: golden retained mip data for every ASTC block size, odd dimensions, sRGB/alpha, arrays,
view ranges starting above/below the drop point, explicit/implicit LOD, copy/alias and Pulse
capture/restore. Assert matching shader-visible dimensions where promised, and measure actual
committed texture heaps. Provide a per-game off switch before enabling the default.

## Patch 4: optional inline software TLB (designed, pending profiling)

The hypothesized repeated C++ lookup is not the ordinary-RAM path in this revision:
`src/dynarmic/src/dynarmic/backend/x64/emit_x64_memory.h` already emits the page-index arithmetic,
a direct load from r14's software table and a null check. `arm_dynarmic_64.cpp` points it at
`Common::PageTable::jit_entries`, containing pointer-only direct-access entries. Special pages
are deliberately null so callbacks enforce debug, mapping and rasterizer coherence behavior.
A TLB adds tag comparisons and state loads; any speedup must be demonstrated.

Exact files for a measured prototype:

- `src/dynarmic/src/dynarmic/backend/x64/emit_x64_memory.h` and `emit_x64_memory.cpp.inc`: guard address range/alignment first; probe one entry or use (vpage & 1) for two; fall through to existing inline table load on miss; cache only valid ordinary-RAM entries.
- `src/dynarmic/src/dynarmic/backend/x64/a64_jitstate.h`, `a32_jitstate.h`, `jitstate_info.h`: per-JIT tag, host-offset, address-space identity and coherence epoch, with invalid sentinels.
- `src/dynarmic/src/dynarmic/interface/A64/config.h`, `A32/config.h`: optional TLB configuration and mapping epoch pointer.
- `src/core/arm/dynarmic/arm_dynarmic_64.cpp`, `arm_dynarmic_32.cpp`: pass the epoch and selected 0/1/2-entry mode.
- `src/common/page_table.h` / `.cpp`, `src/core/memory.cpp`: invalidate on all map/unmap/remap, rasterizer-cache state, debug/protection and process-switch changes. Audit every jit_entries Store/ZeroRegion and resize path.
- Patch 1's settings files: `software_tlb_entries=0|1|2`, initially 0; expose in UI only with the implemented emitter path.

Use an epoch shared by each address space and a defined stop-the-world/remapping synchronization
contract. An epoch increment alone does not make a cached host pointer safe against concurrent
unmap/free. Invalidate before a stale access can proceed; preserve all existing scheduling barriers.
Handle epoch rollover, process switching, exclusive accesses, MMIO, cross-page/misaligned loads,
32/64/128-bit operations and cached-texture read/write callbacks. Do not cache MMIO or null entries.
A block-local cache killed at every possible call/control-flow boundary is a smaller prototype,
but still needs coherence with other emulated cores.

Gates: emitted-code tests for hit/miss/collision, remap same virtual page to another host allocation,
permission/debug changes, rasterizer transitions, process switches, boundary accesses, atomics,
and fault handling. Compare generated instruction count and end-to-end frame CPU time with
software TLB off/1/2; abandon it if slower than the existing inline load.

## Additional architecture and measurement

- Log app commit + OS-reported budget, GPU heap commitment, JIT code/metadata, retained DXIL/SPIR-V,
  live/retired PSOs, ring/dedicated staging and peak compile queue bytes. Never sum DXGI and app
  totals as independent budgets on unified memory. Keep at least the memory guard's current
  reserve rather than increasing the 5,120 MiB limit.
- Bound parallel shader compilation by transient byte demand as well as worker count. Otherwise
  lowering steady-state caches can still OOM during compilation bursts.
- Prefer reclaiming retired heap blocks and limiting dedicated staging growth before introducing
  SSD guest-memory paging. Blocked guest/GPU readbacks can erase any draw-call CPU improvement.
- Measure descriptor construction/copies, root writes, locks, barriers, uploads and command
  recording separately. Existing indirect and async-recording paths need A/B evidence before
  changing their defaults. State suppression cannot eliminate the full ~8 microseconds/draw.
- Collect cache clears/recompiles per minute and 99th-percentile frame times for 32 vs 48 MiB.
  Report committed bytes and high-water marks; reserved cache capacity is a separate metric.

Use the same console, app mode, scene, resolution and warm cache for every comparison. Start with
`tools/xbox/draw_stress` (4000 draws) and legally obtained homebrew, then user-owned game dumps.
Run cold start, warm traversal, worst-case area transitions and repeated return-to-library launches.
Capture median/p95/p99 CPU recording time, GPU time, process commit high-water mark, stutters and
cache misses. Require pixel/capture equivalence for lossless controls; document visual loss for
mip omission. The ~32 ms baseline and 5,120 MiB budget come from upstream documentation and the
user's target, not a measurement made in this environment.

## Validation for patch 1

Portable Linux checks passed: sanitizer-enabled settings/binding test, system UI model, linked
shader concurrency/budget/collision tests and pipeline residency tests. The original system test
failed on Linux's 32-bit wchar_t fixture; the test now spells the UTF-16 surrogate pair explicitly.
Its stale navigation expectations also omitted existing ShaderShare/Updates rows; coverage now
checks those rows and the new choices. No production locale/name behavior was changed.

Windows UWP configure/build/link, D3D12 debug-layer execution, packaging, actual UI rendering,
Xbox memory and draw-time A/B measurements remain outstanding. No runnable APPX is supplied.
