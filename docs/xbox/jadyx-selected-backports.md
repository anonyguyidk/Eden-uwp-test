<!-- coderabbit-ai-generated:v1 -->

# Selected JADYX 0.4.3.0 backports

This remains the Eden UWP optimization fork based on PrismFox67/Eden-UWP. It is not a full upgrade to JADYX 0.4.3.0. The comparison uses the official `v0.4.3.0` tag (`b3abce9bf7c6aad6f9ae1cd6bf787952c6e3e63d`), released November 17, 2026.

| Backport | Source and author | Applied scope |
| --- | --- | --- |
| Retire-driven live PSO index cleanup | [022098a567](https://github.com/JulianDr14/jadyx/commit/022098a5671c998d394a7a5780d23691d9ad15e8), Ho Tuan Kiet | The existing live index skips full scans until an indexed owner retires. Exact identity, weak ownership, capacity, sharing of in-flight builds and failure retries are preserved. |
| Derived texture alignment | [e50fb6f75d](https://github.com/JulianDr14/jadyx/commit/e50fb6f75dafde34bf98ec508bc2093db6f13c1c), JulianDr14 | Reset the copied resource descriptor's alignment in `Image::SliceArray()`. Its new layout must not inherit a source image's small-placement alignment. The upstream Volume/Cube changes do not apply because these helpers are absent here. |

These are automatic implementation fixes; existing shader cache and eviction controls remain available in Settings > Graphics and per-game profiles. The retirement signal adds one shared pointer per indexed entry (about 128 KiB at 8192 entries on x64), plus one shared atomic token; it does not retain PSOs or raw shader bytecode. Texture alignment is chosen by D3D12 and may change the allocation size.

The upstream query-cache optimization targets a different query implementation from the legacy cache used here. The newer texture remapping, Permafrost paging, CPU scheduling, buffer readback changes and full graphics UI have broader dependencies or interact with this fork's changes; they were not imported in this selection. FSR 1 remains the selected scaler. Internal resolution scaling and FSR 3 remain deferred.

## Verification

The upstream `live-cache-retirement.cpp` test covers capacity hits, 16 concurrent readers, immediate reclamation, weak lifetime, capacity bypass, hash collisions, build failure/retry, concurrent retirement and an entry outliving its cache. It is included in the preview workflow before Windows builds. Existing graphics/settings and import navigation gates remain enabled.

A Windows build confirms compilation and packaging. D3D12 resource allocation, Xbox visual correctness, memory use and CPU-time improvement still need console validation. No FPS or memory-saving figure is claimed for these backports.
