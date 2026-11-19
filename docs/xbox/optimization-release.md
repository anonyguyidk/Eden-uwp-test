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

Open **Settings → System**. Changes apply on the next game launch.

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
