# USB texture pack import — package 2.0.3.1

## In-game steps

1. Extract a Dusklight-compatible texture pack on your computer and copy its folder to USB. This importer accepts **folders**, not ZIP/7z archives or mod packages.
2. Open **Settings → Video → Rendering → Texture Pack → Import Texture Pack from USB...**. The same Video settings are available before launching the game.
3. Pick the USB drive, enter the extracted pack folder, and choose **Use this folder**. Confirm **Import**. If a drive is not listed or access is denied, do not change console security settings; record the exact result for device testing.
4. Keep USB connected while scanning/copying. The dialog reports file/byte progress and supports cancellation. Selection, scanning and copying do not change the USB source or current pack.
5. On successful completion the copied pack is enabled and selected in the saved configuration. The USB is no longer needed. Texture registrations are reloaded on the render/UI thread; restart if an already displayed texture has not refreshed.
6. **Use Default Texture Folder** switches back to your existing manual `texture_replacements` folder. The **Enable Texture Replacements** switch still controls both sources.

## Files and compatibility

- PNG and DDS texture files are copied recursively, preserving relative subfolders and filenames, including mip sidecars. Other extensions are ignored.
- At least one filename must be recognized by Aurora's actual replacement filename parser, normally `tex1_*.png` / `tex1_*.dds`. Images without compatible names, unrelated archives and native code mods are not automatically converted into texture packs.
- This importer does not change pack format support or priority: mod texture overrides still take precedence over user texture replacements.
- Links/junctions that resolve outside the source, symbolic links, unsupported file types, unreadable files, deep directory trees and overlapping source/destination selections are rejected. The file-count limit is 200,000 and directory-depth limit is 64.

## Storage and failure behavior

- New imports are copied into private `.import-*` staging folders under `<data folder>/texture_packs/` and renamed to an independent `pack-<id>` directory only after copying completes.
- The active pack is not replaced during copying. There is no modification or deletion of existing `texture_replacements` files, prior completed imports, saves, or the USB source.
- Only the managed pack ID is saved as `backend.importedTexturePack`; arbitrary external paths cannot be activated through that setting. Empty/invalid/missing IDs fall back to the original manual folder.
- Selection activation and settings persistence run on the UI thread after the worker finishes. If saving the selection fails, the previous selection is restored.
- Cancellation, detected USB/source changes and ordinary copy failures clean up this attempt's temporary files. Force-closing the app or losing power may leave an **inactive** `.import-*` folder; it is never selected as a pack.
- Imports are deliberately kept as backups. Re-importing uses additional app storage; there is no automatic deletion of your old packs. A preflight check requires the whole copied pack to fit plus 8 MiB headroom; later disk-write failures are still handled without switching the active pack.
- If a very late Cancel races with completed copying, the finished copy can remain in app storage but is not activated.

## Checks and limits of verification

Portable tests cover nested/Unicode paths, preserved sources/manual packs, repeated imports, invalid packs and selection IDs, source/destination overlap, links, USB source changes, cancellation before/during copying, staging cleanup and free-space preflight. Syntax checks cover the UWP menu integration. These do **not** establish Xbox runtime behavior.

Required Xbox checks:
- USB drive enumeration/access and controller-only navigation; cancel from the browser and confirmation dialog.
- Successful small and large PNG/DDS imports, subfolders, free-space errors, USB removal during scanning/copying and cancellation.
- Live texture refresh, restart persistence, enabling/disabling replacements and reverting to the default manual folder.
- Existing saves and manual texture folders remain unchanged; test both prelaunch and in-game menus.

This remains a test release. Code mods are disabled. The minimum package OS requirement remains unchanged from the previous build. A new ephemeral self-signed certificate is needed because the prior test signing key was intentionally not retained; in-place upgrade/trust behavior must not be assumed. Back up saves and do not uninstall an older installation just to try the update.
