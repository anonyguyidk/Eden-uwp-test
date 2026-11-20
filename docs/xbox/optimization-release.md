<!-- coderabbit-ai-generated:v1 -->

# Eden UWP · Experimental memory build

> **VIBECODED WITH AI ASSISTANCE — EXPERIMENTAL**
> The optimization changes and documentation are AI generated. The inherited emulator belongs to PrismFox67, Jadyx, Eden, yuzu and their contributors. Expect bugs; back up saves before testing.

## Download & install

Download **EdenUWP-Xbox-Experimental.zip** from Assets and extract it. It contains the emulator APPX, one signing certificate, and the installable runtime dependencies. Scripts, symbols and duplicate certificates are excluded.

1. Enable Xbox Developer Mode and open the console's Device Portal from your PC.
2. Add the main **EdenUWP APPX**, select the included **VCLibs** package and any other included dependencies, and use **EdenUWP.cer** for signing trust where requested.
3. Set the installed app's type to **Game** in Dev Home before launching it.
4. Supply your own legally obtained keys, firmware and game dumps. None are included.

This build retains the inherited application identity. Back up before installing; do not uninstall an existing app without backing up its saves. Without a configured signing certificate, CI generates a test certificate for that run; it may not support updating a previous installation in place.

## Tune your game

<!-- coderabbit-ai-generated:v1 -->

<!-- coderabbit-ai-generated-section:v1:8e825417f217aefc7977746777325deff4b8cc058dd6664fd6672e05111bfeef -->

Open **Settings → Graphics** for FSR 1, its sharpness slider, handheld/docked mode, aspect ratio and memory controls. **Game Options → Graphics** opens the selected game's overrides. The Profile row switches between global defaults and that game; **Inherit global defaults** resets the game's overrides. Changes apply on the next game launch.

<!-- coderabbit-ai-generated:v1 -->

<!-- coderabbit-ai-generated-section:v1:737720bdf9f41d80346f6cbcc2a7c41e076492a69050b65961c1f2d6bdbd304c -->

**Internal resolution scaling and FSR 3 are deferred.** D3D12 renders at the game's native resolution. Handheld mode often requests a lower resolution, depending on the game. FSR 1 sharpens/upscales only when the source is smaller than the screen.

For console-mode changes, start the game from a cold boot: discard its kept Pulse session through Game Options first. Settings changes never automatically delete a kept session or normal saves.

<!-- /coderabbit-ai-generated:v1 -->

## Xbox folder browser

**Settings → File Manager → Folder picker = DevelopmentFiles** selects the in-app `D:\DevelopmentFiles\` browser. A/Enter opens folders or selects **Use this folder**; Left goes up; B/Escape cancels. The choice persists; the Windows picker remains available.

Use it for game folders, keys and firmware imports, update/DLC authorization and the game's Mods add-folder action. Supported mod layouts include `cheats/<build-id>.txt`, so cheat packs can be imported through Mods. Existing validation and install confirmation remain in effect. Create your folders through Xbox Device Portal; access is subject to the console's UWP permissions. If denied, use Network import. Directories show at most 2048 child folders.

## Memory controls

<!-- /coderabbit-ai-generated:v1 -->

| Control | Available choices | Default |
| --- | --- | --- |
| JIT cache | 32 or 48 MiB per JIT | 48 MiB |
| Upload ring | 32 / 64 / 128 MiB | 64 MiB |
| Linked shader cache | 0–16 MiB (memory policy) | 4 MiB |
| PSO eviction multiplier | 1–4 | 1 |
| Redundant bind suppression | Off / On | On |
| Shader bytecode storage | Memory / Zstd disk backing | Disk |
| ASTC mip omission | 0 / 1 / 2 top levels | Off |
| Inline software TLB | Off / 1 / 2 entries | Off |

Per-game overrides live in **LocalState/optimization/<16-digit-lowercase-title-id>.cfg**. For example:

```ini
jit_cache_mib=32
upload_ring_mib=32
shader_bytecode_policy=disk
astc_mip_drop=0
software_tlb_entries=0
```

## Known limitations

- ASTC mip omission is a constrained prototype for eligible large BC-backed sampled textures. Copies, aliases, blits, reinterpretation and readback of reduced images can stop emulation. Keep it off for compatibility and captures.
- The software TLB remains experimental and defaults off pending full runtime validation.
- Disk backing keeps raw DXIL if a write fails. Abnormal termination can leave session cache files behind.
- A successful Windows build does not establish Xbox stability. The 5,120 MiB ceiling, TotK stability and draw-time improvements have not been verified on hardware.

Please include the build source, console model, game version, settings and logs in bug reports. Never include keys, firmware or game files.
