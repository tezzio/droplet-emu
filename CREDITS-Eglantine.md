# Eglantine commits in Droplet

Droplet includes the work of [Eglantine](https://github.com/Djihads80/Eglantine), an experimental Switch 2 emulator built on
eden-nx2 (GPL-3.0). Its `nx2` branch is merged up to commit `68a945d0a8` (2026-11-03). These are
the 376 Eglantine commits Droplet carries, newest first, with their authors. "In part"
means Droplet kept its own version of some of what the commit changed.

Not included: Eglantine's CI, documentation, translation and branding changes; pull request #313
(a LAN room handshake; Droplet has its own LAN play code); and a few commits Droplet leaves out on
purpose: the removal of the amiibo hotkeys, a prompt for decryption keys (Droplet loads no keys),
and a handler for bsd command 36 (Droplet has its own). Eglantine also carries eden-nx2's own
commits; Droplet has those from eden-nx2 itself (see [CREDITS.md](CREDITS.md)).

| Commit | Date | Author | Title |
|---|---|---|---|
| [`465aa33077f4`](https://github.com/Djihads80/Eglantine/commit/465aa33077f4) | 2026-11-03 | néo Luigi | Vibration support for ns2 games |
| [`537c1f4e173a`](https://github.com/Djihads80/Eglantine/commit/537c1f4e173a) | 2026-11-03 | Jonas | video_core: fix the -Wshadow error in the FSR 3.1 upscaler |
| [`bd12e7c4ef7d`](https://github.com/Djihads80/Eglantine/commit/bd12e7c4ef7d) | 2026-11-02 | kraibse | fix: update BUILD_ID definition and improve path handling in GameListWorker |
| [`890dc1e6c7d8`](https://github.com/Djihads80/Eglantine/commit/890dc1e6c7d8) | 2026-11-02 | kraibse | nx2: improve save data handling for titles with missing NACP sizes to redirect the user to the correct save location |
| [`ebec180bf112`](https://github.com/Djihads80/Eglantine/commit/ebec180bf112) | 2026-11-03 | a55uka | nup_writer: keep the dump's own layout for non-exefs/romfs entries |
| [`873cae5569e9`](https://github.com/Djihads80/Eglantine/commit/873cae5569e9) | 2026-11-03 | Jonas | nx2: video decoder: copy NV12 frames from a GPU decoder without swscale |
| [`031e58e93046`](https://github.com/Djihads80/Eglantine/commit/031e58e93046) | 2026-10-31 | Javier | nx2: video decoder: keep post-stream release fences pending |
| [`3ffa97133714`](https://github.com/Djihads80/Eglantine/commit/3ffa97133714) | 2026-10-31 | rdenno | nx2: vdecsvc refreshes the GPU copy of each decoded frame (Yoshi demo) |
| [`701c91b402f4`](https://github.com/Djihads80/Eglantine/commit/701c91b402f4) | 2026-10-31 | rdenno | nx2: vdecsvc picks H.264 when the configure carries no codec name and the stream is H.264 (Yoshi demo) |
| [`bfd6430918ff`](https://github.com/Djihads80/Eglantine/commit/bfd6430918ff) | 2026-10-31 | Yuri Olive | service: nx2:vdecsvc: handle end-of-stream without aborting |
| [`8d6cb5c0d4fb`](https://github.com/Djihads80/Eglantine/commit/8d6cb5c0d4fb) | 2026-10-30 | PavelBARABANOV | [video_core/host1x] VP9 Video Playback Fixes (#4590) |
| [`e996661b99f4`](https://github.com/Djihads80/Eglantine/commit/e996661b99f4) | 2026-11-03 | Jonas | qt: ask about the recommended MKW settings only when booting MKW |
| [`11cebf56014a`](https://github.com/Djihads80/Eglantine/commit/11cebf56014a) | 2026-11-02 | Duck912 | video_core: replace Fortune's Weave's DLSS with AMD FSR 3.1 |
| [`08dbcc7e3b3d`](https://github.com/Djihads80/Eglantine/commit/08dbcc7e3b3d) | 2026-11-02 | Duck912 | externals: add the FSR 3.1 upscaler shaders from AMD's FidelityFX SDK |
| [`960ebcc73d99`](https://github.com/Djihads80/Eglantine/commit/960ebcc73d99) | 2026-11-02 | Duck912 | video_core: sharpen the temporal DLSS stand-in |
| [`bb8fdfac2143`](https://github.com/Djihads80/Eglantine/commit/bb8fdfac2143) | 2026-11-02 | Duck912 | qt_common: name the FSR filter and frame generation entries by what they do |
| [`405adb698e5e`](https://github.com/Djihads80/Eglantine/commit/405adb698e5e) | 2026-11-02 | snowstormxd | settings: default to ScaleForce and FXAA (in part) |
| [`85436a5acd36`](https://github.com/Djihads80/Eglantine/commit/85436a5acd36) | 2026-11-02 | snowstormxd | kernel: keep migrating fiber TLS lookups outside link-time optimization |
| [`06e211e0d0a1`](https://github.com/Djihads80/Eglantine/commit/06e211e0d0a1) | 2026-11-01 | snowstormxd | nx2: rearm clean read areas when the GPU writes them again |
| [`b75ea7bcba60`](https://github.com/Djihads80/Eglantine/commit/b75ea7bcba60) | 2026-11-02 | snowstormxd | settings: restore the recommended MKW settings prompt |
| [`80bfef8e0c5f`](https://github.com/Djihads80/Eglantine/commit/80bfef8e0c5f) | 2026-11-02 | Dennys Kaluzny Pereira | input_common/sdl: only log motion capabilities when they change |
| [`9f4f01f76e85`](https://github.com/Djihads80/Eglantine/commit/9f4f01f76e85) | 2026-11-02 | Sssnowyyy | nx2: map GPU memory before later command lists are read |
| [`cd9b19c69d7e`](https://github.com/Djihads80/Eglantine/commit/cd9b19c69d7e) | 2026-11-02 | Sssnowyyy | vulkan: repack 69-byte vertices without dynamic vertex input on MoltenVK |
| [`b45f4c6d2c79`](https://github.com/Djihads80/Eglantine/commit/b45f4c6d2c79) | 2026-11-02 | Sssnowyyy | vk_pipeline_cache: keep the pipelines on MoltenVK and preload eight sessions |
| [`46d1554186cd`](https://github.com/Djihads80/Eglantine/commit/46d1554186cd) | 2026-11-02 | Sssnowyyy | vulkan: back a sparse tiled image whole when its tiles can't be set up |
| [`427b9abfdc48`](https://github.com/Djihads80/Eglantine/commit/427b9abfdc48) | 2026-11-02 | Sssnowyyy | video_core: continue a split instance loop at its own instance index |
| [`8556e27015de`](https://github.com/Djihads80/Eglantine/commit/8556e27015de) | 2026-11-02 | Sssnowyyy | buffer_cache: fix the already-disabled check for uniform buffers |
| [`13fc5fda26e6`](https://github.com/Djihads80/Eglantine/commit/13fc5fda26e6) | 2026-11-02 | Sssnowyyy | vulkan: repack 69-byte vertices only when needed |
| [`256b1dc346e1`](https://github.com/Djihads80/Eglantine/commit/256b1dc346e1) | 2026-11-02 | Sssnowyyy | video_core: don't execute command lists copied before their channel waited |
| [`690f80e15f41`](https://github.com/Djihads80/Eglantine/commit/690f80e15f41) | 2026-11-02 | Sssnowyyy | nx2: don't stall presents on a current-frame wait the GPU can't finish |
| [`a1f781fa24cc`](https://github.com/Djihads80/Eglantine/commit/a1f781fa24cc) | 2026-11-02 | Sssnowyyy | video_core: flush GPU-written code when capturing native shaders |
| [`e46c6cbfb0f6`](https://github.com/Djihads80/Eglantine/commit/e46c6cbfb0f6) | 2026-11-02 | Sssnowyyy | video_core: read texture handles as header offsets only on Switch 2 |
| [`0ed4a3a9e795`](https://github.com/Djihads80/Eglantine/commit/0ed4a3a9e795) | 2026-11-02 | Sssnowyyy | shader: drop the slots of a failed handle subtree with its code |
| [`8b0666b74868`](https://github.com/Djihads80/Eglantine/commit/8b0666b74868) | 2026-11-02 | Sssnowyyy | shader: restore the handle build depth when a build throws |
| [`bda49e810b59`](https://github.com/Djihads80/Eglantine/commit/bda49e810b59) | 2026-11-02 | Sssnowyyy | shader: evaluate handle programs for image and texture buffer descriptors |
| [`e277b4ba6169`](https://github.com/Djihads80/Eglantine/commit/e277b4ba6169) | 2026-11-02 | Sssnowyyy | nx2: make the 216 sparse request range checks overflow-safe |
| [`089105ab44c8`](https://github.com/Djihads80/Eglantine/commit/089105ab44c8) | 2026-11-02 | Sssnowyyy | file_sys: reject NUP tables with wrapping bounds or cyclic directories |
| [`8748fbd542a0`](https://github.com/Djihads80/Eglantine/commit/8748fbd542a0) | 2026-11-02 | Sssnowyyy | file_sys: decompress large NUP zstd members once |
| [`a3d67c14c86f`](https://github.com/Djihads80/Eglantine/commit/a3d67c14c86f) | 2026-11-02 | Sssnowyyy | frontend: bound the wait for the emulation thread at shutdown |
| [`c83eb1d1f5c1`](https://github.com/Djihads80/Eglantine/commit/c83eb1d1f5c1) | 2026-11-02 | Sssnowyyy | shader: tell handle-program textures apart when adding descriptors |
| [`6cdf4c28a77a`](https://github.com/Djihads80/Eglantine/commit/6cdf4c28a77a) | 2026-11-02 | snowstormxd | loader: find mixed-case loose RomFS directories consistently |
| [`5e7a14c05757`](https://github.com/Djihads80/Eglantine/commit/5e7a14c05757) | 2026-11-01 | snowstormxd | input: join calibration workers before destroying their owner |
| [`6a0d50bcca77`](https://github.com/Djihads80/Eglantine/commit/6a0d50bcca77) | 2026-11-01 | snowstormxd | tests: restore batched page accounting and isolate timer fixtures |
| [`43490c0417f6`](https://github.com/Djihads80/Eglantine/commit/43490c0417f6) | 2026-11-02 | snowstormxd | kernel: define thread-local state out of line for LTO |
| [`9e5f9bbdb9fe`](https://github.com/Djihads80/Eglantine/commit/9e5f9bbdb9fe) | 2026-11-02 | snowstormxd | settings: default to handheld mode on all platforms |
| [`00285925faeb`](https://github.com/Djihads80/Eglantine/commit/00285925faeb) | 2026-11-02 | snowstormxd | core: skip empty bitmap ranges during cache and scheduler traversal |
| [`226162edf525`](https://github.com/Djihads80/Eglantine/commit/226162edf525) | 2026-11-02 | snowstormxd | vulkan: submit native work sooner and use the active draw count |
| [`76f6aec86011`](https://github.com/Djihads80/Eglantine/commit/76f6aec86011) | 2026-11-02 | Daniel Cabral | tests when building on Windows: compare FindBestUpdatePackage with a size_t |
| [`24da16703dd2`](https://github.com/Djihads80/Eglantine/commit/24da16703dd2) | 2026-11-01 | snowstormxd | texture_cache: clear only exposed descriptor bitmap words |
| [`1e37847a805e`](https://github.com/Djihads80/Eglantine/commit/1e37847a805e) | 2026-11-01 | snowstormxd | tests: verify deferred descriptors against eager invalidation |
| [`ddbd076cfe95`](https://github.com/Djihads80/Eglantine/commit/ddbd076cfe95) | 2026-11-01 | snowstormxd | shader_cache: use sorted membership for large removal batches |
| [`b2430b4d9001`](https://github.com/Djihads80/Eglantine/commit/b2430b4d9001) | 2026-11-01 | snowstormxd | shader_cache: batch memory-write invalidations without skipping ranges |
| [`202c6b0356e1`](https://github.com/Djihads80/Eglantine/commit/202c6b0356e1) | 2026-11-01 | gabrielskaminski | video_core/buffer_cache: reuse unchanged small uniform buffers within command buffer |
| [`1a346fb3bb69`](https://github.com/Djihads80/Eglantine/commit/1a346fb3bb69) | 2026-11-02 | snowstormxd | gpu: make register-write history opt-in in release builds |
| [`665938d0b157`](https://github.com/Djihads80/Eglantine/commit/665938d0b157) | 2026-11-02 | snowstormxd | macro: execute native parameters without a second FIFO copy |
| [`e64268269f4f`](https://github.com/Djihads80/Eglantine/commit/e64268269f4f) | 2026-11-02 | Franz | Prevents crashes when creating game shortcuts by validating the loader before accessing game titles or icons. Adds safe fallbacks for missing titles and icons, allowing shortcuts to be created even when the game loader or icon is unavailable. |
| [`8f1435f8db10`](https://github.com/Djihads80/Eglantine/commit/8f1435f8db10) | 2026-11-02 | Takoroka | nx2: read GPU-written command lists from copies instead of draining |
| [`15255641e2ca`](https://github.com/Djihads80/Eglantine/commit/15255641e2ca) | 2026-11-02 | kraibse | qt: add a metadata dialog and a progress window to Convert to NUP |
| [`8185d45d5d9d`](https://github.com/Djihads80/Eglantine/commit/8185d45d5d9d) | 2026-11-02 | kraibse | loader: read the display version from GameDataList.Product.NNN too |
| [`4267169006fc`](https://github.com/Djihads80/Eglantine/commit/4267169006fc) | 2026-11-02 | kraibse | file_sys: let the NUP packer override and probe the name, icon and version |
| [`1bcedf8009bb`](https://github.com/Djihads80/Eglantine/commit/1bcedf8009bb) | 2026-11-02 | snowstormxd | qt_common: remove unused path capture to fix MSVC build |
| [`39e1babb1c1a`](https://github.com/Djihads80/Eglantine/commit/39e1babb1c1a) | 2026-11-02 | kraibse | refactor: remove redundant EnsureRomFS call in AppLoader_DeconstructedRomDirectory |
| [`15dd9807110a`](https://github.com/Djihads80/Eglantine/commit/15dd9807110a) | 2026-10-31 | Yuri Olive | buffer_cache: allow 32 storage buffers per stage |
| [`7f42af81ad65`](https://github.com/Djihads80/Eglantine/commit/7f42af81ad65) | 2026-11-02 | Takoroka | video_core: bump the shader cache versions for the FSEL lowering fix |
| [`7ac108e2571d`](https://github.com/Djihads80/Eglantine/commit/7ac108e2571d) | 2026-11-02 | rdenno | shader: lower TXD with any channel mask |
| [`8b8d0fc85f12`](https://github.com/Djihads80/Eglantine/commit/8b8d0fc85f12) | 2026-11-02 | rdenno | shader: lower TLD and TLD4 with half-precision results |
| [`9e368c29181c`](https://github.com/Djihads80/Eglantine/commit/9e368c29181c) | 2026-11-02 | rdenno | shader: lower F2I from a half with a sign or high-half selector |
| [`8af82558bb25`](https://github.com/Djihads80/Eglantine/commit/8af82558bb25) | 2026-11-02 | rdenno | texture_cache: look up X8Z24 with UNORM, UINT, UNORM, UNORM components |
| [`a407703f6d29`](https://github.com/Djihads80/Eglantine/commit/a407703f6d29) | 2026-11-02 | Whovian64 | Linux and AMD: BC3 flashing on RADV, ASTC decode stalls, lower RAM usage, a buffer cache crash (#260) |
| [`46b2577bf788`](https://github.com/Djihads80/Eglantine/commit/46b2577bf788) | 2026-11-02 | ymatrix1 | video_core/buffer_cache: fast-guard stage buffer updates and uniform buffer disable (#264) |
| [`926e7ac6af10`](https://github.com/Djihads80/Eglantine/commit/926e7ac6af10) | 2026-11-02 | Jonas | file_sys: add a deterministic NUP packer, Convert to NUP in Qt and eden-cli --pack-nup (#271) |
| [`e38b738b84eb`](https://github.com/Djihads80/Eglantine/commit/e38b738b84eb) | 2026-11-02 | Mako | vulkan: repack 69-byte vertices to 72 bytes (#281) |
| [`b7d654b9dcaf`](https://github.com/Djihads80/Eglantine/commit/b7d654b9dcaf) | 2026-11-02 | spaina93 | qt: save frame generation settings (#283) |
| [`82e34d410ff6`](https://github.com/Djihads80/Eglantine/commit/82e34d410ff6) | 2026-11-02 | Takoroka | nx2: decode sRGB in the upscale fallback when the program does |
| [`06e0541e18e5`](https://github.com/Djihads80/Eglantine/commit/06e0541e18e5) | 2026-11-01 | Duck912 | am: decode the native swkbd config for library version 0x160012 |
| [`92d49d8a0c3e`](https://github.com/Djihads80/Eglantine/commit/92d49d8a0c3e) | 2026-11-01 | Duck912 | video_core: temporal upscale stand-in for Fortune's Weave's DLSS |
| [`d125cfb41c0b`](https://github.com/Djihads80/Eglantine/commit/d125cfb41c0b) | 2026-11-01 | Duck912 | kepler_compute: stand in for Fortune's Weave's DLSS with an upscale |
| [`c08b5efbaa4a`](https://github.com/Djihads80/Eglantine/commit/c08b5efbaa4a) | 2026-11-01 | Duck912 | shader: keep root table reads the blit constants would zero |
| [`808feaf3bf34`](https://github.com/Djihads80/Eglantine/commit/808feaf3bf34) | 2026-11-01 | Duck912 | video_core: keep root tables and bind table 0 as constant bank 17 |
| [`9acc324228b7`](https://github.com/Djihads80/Eglantine/commit/9acc324228b7) | 2026-11-01 | Duck912 | nx2:vi2: report the display's maximum buffer resolution |
| [`a169391e6019`](https://github.com/Djihads80/Eglantine/commit/a169391e6019) | 2026-11-01 | Duck912 | shader: count only register writes in the callee link check |
| [`ef3ae4ae23b2`](https://github.com/Djihads80/Eglantine/commit/ef3ae4ae23b2) | 2026-11-01 | Duck912 | shader: apply FSEL's floating operand modifiers |
| [`43ab1ee42c70`](https://github.com/Djihads80/Eglantine/commit/43ab1ee42c70) | 2026-11-01 | Duck912 | shader: lower more conversion, comparison and memory forms |
| [`44c0fa86e2d4`](https://github.com/Djihads80/Eglantine/commit/44c0fa86e2d4) | 2026-11-01 | Duck912 | shader: lower more 64-bit integer carry forms |
| [`93f8c89595d2`](https://github.com/Djihads80/Eglantine/commit/93f8c89595d2) | 2026-11-01 | Duck912 | shader: lower warp reductions, shared atomics and descriptor loads |
| [`2b8002474d25`](https://github.com/Djihads80/Eglantine/commit/2b8002474d25) | 2026-11-01 | Duck912 | shader: lower more integer forms |
| [`18b0671e1b64`](https://github.com/Djihads80/Eglantine/commit/18b0671e1b64) | 2026-11-01 | Duck912 | shader: read extra texture descriptor buffers bindless |
| [`e7973dae7e2e`](https://github.com/Djihads80/Eglantine/commit/e7973dae7e2e) | 2026-11-01 | Duck912 | nx2:vi2: sleep in the guest kernel while pacing presents |
| [`b8d2c08d6efd`](https://github.com/Djihads80/Eglantine/commit/b8d2c08d6efd) | 2026-11-02 | a55uka | nup_container: update record layouts to aligned NUP v1 fields |
| [`10268f8c4ba3`](https://github.com/Djihads80/Eglantine/commit/10268f8c4ba3) | 2026-11-02 | a55uka | nup_container: update record layouts to aligned NUP v1 fields |
| [`6612817cb4b6`](https://github.com/Djihads80/Eglantine/commit/6612817cb4b6) | 2026-11-02 | kraibse | dma_pusher: restores the trees as the lorax wouldn't be happy without them (add IsGpuWrittenCommands method to check GPU-written entries) |
| [`6be5c163338d`](https://github.com/Djihads80/Eglantine/commit/6be5c163338d) | 2026-11-01 | Takoroka | nx2: lower the HSETP2 register form |
| [`51d798ff0373`](https://github.com/Djihads80/Eglantine/commit/51d798ff0373) | 2026-11-01 | kraibse | texture_cache: keep the allocator's block slack out of the usable VRAM |
| [`518712cc5d2c`](https://github.com/Djihads80/Eglantine/commit/518712cc5d2c) | 2026-11-01 | kraibse | nx2: lower the TXD and TXQ variants Splatoon Raiders uses late |
| [`3fe4f1b189f9`](https://github.com/Djihads80/Eglantine/commit/3fe4f1b189f9) | 2026-11-01 | kraibse | nx2: answer nx2:camera commands without a log line per call |
| [`16967eb30e97`](https://github.com/Djihads80/Eglantine/commit/16967eb30e97) | 2026-11-01 | kraibse | vulkan: don't make small images unbacked when device memory runs out |
| [`10e70639058b`](https://github.com/Djihads80/Eglantine/commit/10e70639058b) | 2026-11-01 | kraibse | nx2: capture and lower native shader programs once |
| [`cce6d44af23f`](https://github.com/Djihads80/Eglantine/commit/cce6d44af23f) | 2026-11-01 | kraibse | nx2: execute command lists as submitted |
| [`803ece5f90bb`](https://github.com/Djihads80/Eglantine/commit/803ece5f90bb) | 2026-11-01 | kraibse | vulkan: don't divide by a zero sparse block size (AMD) |
| [`021c9724371e`](https://github.com/Djihads80/Eglantine/commit/021c9724371e) | 2026-11-01 | snowstormxd | game_list: snapshot directory type before handing rows to the model |
| [`426f120ee808`](https://github.com/Djihads80/Eglantine/commit/426f120ee808) | 2026-11-01 | snowstormxd | frontend: keep loose-game preparation and shutdown responsive |
| [`8e777214b02b`](https://github.com/Djihads80/Eglantine/commit/8e777214b02b) | 2026-11-01 | snowstormxd | game_list: discover loose dumps without walking their RomFS payloads |
| [`ac22c4087d9a`](https://github.com/Djihads80/Eglantine/commit/ac22c4087d9a) | 2026-11-01 | kraibse | Revert "video_core: draw deferred instances before the state after their loop" |
| [`70d291d3aaab`](https://github.com/Djihads80/Eglantine/commit/70d291d3aaab) | 2026-11-01 | kraibse | Revert "nx2/ampere: implement ISBERD (0x923) and accept ISBEWR (0x927)" |
| [`12d9386e7ef1`](https://github.com/Djihads80/Eglantine/commit/12d9386e7ef1) | 2026-11-01 | kraibse | Revert "Code review, cleanup and fix" |
| [`5979e989d11f`](https://github.com/Djihads80/Eglantine/commit/5979e989d11f) | 2026-11-01 | kraibse | Revert "video_core: EDEN_NX2_DRAW_LOG diagnostics for draws and macro parameters" |
| [`511d19f5de08`](https://github.com/Djihads80/Eglantine/commit/511d19f5de08) | 2026-11-01 | kraibse | Revert "nx2: read command lists the GPU wrote from the GPU's copy" |
| [`703d2bbb3ba3`](https://github.com/Djihads80/Eglantine/commit/703d2bbb3ba3) | 2026-11-01 | kraibse | Revert "video_core: log slots, mesh registers and launch of skipped draws (EDEN_NX2_DRAW_LOG)" |
| [`9b547adddfa8`](https://github.com/Djihads80/Eglantine/commit/9b547adddfa8) | 2026-11-01 | kraibse | Revert "video_core/dma_pusher: fast-path batching of consecutive non-execution methods" |
| [`2cfdcaae88e0`](https://github.com/Djihads80/Eglantine/commit/2cfdcaae88e0) | 2026-11-01 | kraibse | Revert "video_core: split the no-pipeline count into rejected stages and async builds (EDEN_NX2_DRAW_LOG)" |
| [`2131b5b5d416`](https://github.com/Djihads80/Eglantine/commit/2131b5b5d416) | 2026-11-01 | kraibse | Revert "video_core: list the programs behind skipped draws in the DRAWLOG summary" |
| [`2c99c41310b6`](https://github.com/Djihads80/Eglantine/commit/2c99c41310b6) | 2026-11-01 | kraibse | Revert "video_core: drop the macro-parameter check from the DRAWLOG diagnostics" |
| [`9f22054c6360`](https://github.com/Djihads80/Eglantine/commit/9f22054c6360) | 2026-11-01 | Duck912 | nx2:gsv: map decoder images the client reads on the GPU |
| [`f8d38607b6cf`](https://github.com/Djihads80/Eglantine/commit/f8d38607b6cf) | 2026-11-01 | Duck912 | nx2:vi2: pace presents by their swap interval |
| [`e23b0f2fab90`](https://github.com/Djihads80/Eglantine/commit/e23b0f2fab90) | 2026-11-01 | Takoroka | shader: keep the handle slot map thread-local instead of a pointer to it |
| [`e37b4b9fa60f`](https://github.com/Djihads80/Eglantine/commit/e37b4b9fa60f) | 2026-11-01 | Takoroka | texture_cache: collect images in the emptiest device memory blocks first |
| [`ca1fac93fb43`](https://github.com/Djihads80/Eglantine/commit/ca1fac93fb43) | 2026-11-01 | Takoroka | texture_cache: cold-hint the guest memory of sparse images too |
| [`40902b09c7b8`](https://github.com/Djihads80/Eglantine/commit/40902b09c7b8) | 2026-11-01 | Takoroka | nx2: leave unwritten sparse blocks without memory when they get zeros |
| [`5793d12801a5`](https://github.com/Djihads80/Eglantine/commit/5793d12801a5) | 2026-11-01 | kraibse | video_core: drop the macro-parameter check from the DRAWLOG diagnostics |
| [`96d30a1c4553`](https://github.com/Djihads80/Eglantine/commit/96d30a1c4553) | 2026-11-01 | kraibse | video_core: list the programs behind skipped draws in the DRAWLOG summary |
| [`8c9096253d35`](https://github.com/Djihads80/Eglantine/commit/8c9096253d35) | 2026-11-01 | kraibse | video_core: split the no-pipeline count into rejected stages and async builds (EDEN_NX2_DRAW_LOG) |
| [`91691cf551e8`](https://github.com/Djihads80/Eglantine/commit/91691cf551e8) | 2026-11-01 | Yuri Olive | video_core/dma_pusher: fast-path batching of consecutive non-execution methods |
| [`00b298a7b61f`](https://github.com/Djihads80/Eglantine/commit/00b298a7b61f) | 2026-11-01 | kraibse | video_core: log slots, mesh registers and launch of skipped draws (EDEN_NX2_DRAW_LOG) |
| [`d151e53e6148`](https://github.com/Djihads80/Eglantine/commit/d151e53e6148) | 2026-10-30 | Takoroka | nx2: read command lists the GPU wrote from the GPU's copy |
| [`0c0a2441d61e`](https://github.com/Djihads80/Eglantine/commit/0c0a2441d61e) | 2026-11-01 | kraibse | video_core: EDEN_NX2_DRAW_LOG diagnostics for draws and macro parameters |
| [`67d53ce50581`](https://github.com/Djihads80/Eglantine/commit/67d53ce50581) | 2026-11-01 | néo Luigi | Code review, cleanup and fix |
| [`d9e4ac8360b3`](https://github.com/Djihads80/Eglantine/commit/d9e4ac8360b3) | 2026-11-01 | Paulo Rossatto | nx2/ampere: implement ISBERD (0x923) and accept ISBEWR (0x927) |
| [`eb99da60729b`](https://github.com/Djihads80/Eglantine/commit/eb99da60729b) | 2026-10-30 | Takoroka | video_core: draw deferred instances before the state after their loop |
| [`68f8d9636850`](https://github.com/Djihads80/Eglantine/commit/68f8d9636850) | 2026-10-31 | duck912 | nvhost_as_gpu: share Switch 2 device ranges between a process's address spaces |
| [`9cf6a499c84a`](https://github.com/Djihads80/Eglantine/commit/9cf6a499c84a) | 2026-10-31 | duck912 | nx2:vi2: reply to 104 with the display's output format |
| [`cce19772ce43`](https://github.com/Djihads80/Eglantine/commit/cce19772ce43) | 2026-10-31 | duck912 | nx2:vi2: crop presents to their frame size and honour RGB10A2 images |
| [`e6c0a42d11f1`](https://github.com/Djihads80/Eglantine/commit/e6c0a42d11f1) | 2026-10-31 | duck912 | audio_core: accept the Switch 2 audio renderer revision |
| [`681d50cd9cad`](https://github.com/Djihads80/Eglantine/commit/681d50cd9cad) | 2026-10-31 | duck912 | nx2: report the class family flags in the GPU characteristics |
| [`210fb0cb6903`](https://github.com/Djihads80/Eglantine/commit/210fb0cb6903) | 2026-10-31 | duck912 | nvhost_ctrl_gpu: implement VSMS_MAPPING |
| [`5f0325e2b4e7`](https://github.com/Djihads80/Eglantine/commit/5f0325e2b4e7) | 2026-10-31 | duck912 | nx2:gpu: create graphics devices and report an empty gsv partition |
| [`31b901b93b99`](https://github.com/Djihads80/Eglantine/commit/31b901b93b99) | 2026-11-01 | a55uka | loader: add NUP (Unencrypted (Archival) Package) container support |
| [`1d945527856d`](https://github.com/Djihads80/Eglantine/commit/1d945527856d) | 2026-11-01 | a55uka | fix: nix shell script |
| [`7c27039041dc`](https://github.com/Djihads80/Eglantine/commit/7c27039041dc) | 2026-11-01 | Takoroka | nx2: show the area of a present the game says it drew |
| [`3005cc4149a0`](https://github.com/Djihads80/Eglantine/commit/3005cc4149a0) | 2026-11-01 | Paulo Rossatto | core: do not index registered_processes with an unregistered ASID |
| [`c5a270afc8d3`](https://github.com/Djihads80/Eglantine/commit/c5a270afc8d3) | 2026-10-31 | Takoroka | nx2: report SDR output from vi2 104 |
| [`2f803fa2271c`](https://github.com/Djihads80/Eglantine/commit/2f803fa2271c) | 2026-10-31 | Takoroka | nx2: route the gsv feature groups' image commands to the image handlers |
| [`37446d60a63d`](https://github.com/Djihads80/Eglantine/commit/37446d60a63d) | 2026-10-31 | Takoroka | nx2: give the gsv commands games call their real behavior |
| [`a07b8a31908e`](https://github.com/Djihads80/Eglantine/commit/a07b8a31908e) | 2026-10-31 | Takoroka | service: placeholder handlers for known but unimplemented commands |
| [`c26cfaeac4cd`](https://github.com/Djihads80/Eglantine/commit/c26cfaeac4cd) | 2026-10-31 | Takoroka | service: zero the reply of auto-stubbed commands |
| [`7f31fc4f538e`](https://github.com/Djihads80/Eglantine/commit/7f31fc4f538e) | 2026-10-31 | Takoroka | nx2: echo the TIPC 0 handshake on the gpu and vi2 sessions |
| [`5b4695e5f277`](https://github.com/Djihads80/Eglantine/commit/5b4695e5f277) | 2026-10-31 | Takoroka | nx2: answer vi2 212 with the layer's ready buffer slots |
| [`b878201f334b`](https://github.com/Djihads80/Eglantine/commit/b878201f334b) | 2026-10-31 | Takoroka | nx2: answer vi2 display queries with full replies |
| [`d5c8b1cbedff`](https://github.com/Djihads80/Eglantine/commit/d5c8b1cbedff) | 2026-10-31 | Takoroka | nx2: give each vi2 event slot its own source |
| [`efe9283de57c`](https://github.com/Djihads80/Eglantine/commit/efe9283de57c) | 2026-10-31 | Takoroka | hid: handle Switch 2 commands 213 and 220 |
| [`20df0fac2160`](https://github.com/Djihads80/Eglantine/commit/20df0fac2160) | 2026-10-31 | Takoroka | audio: hand out IAudioDevice device notification events |
| [`dd8742e5634d`](https://github.com/Djihads80/Eglantine/commit/dd8742e5634d) | 2026-10-31 | Takoroka | nvdrv: report 2D/3D/compute/I2M/ZBC support in Switch 2 GPU flags |
| [`a08eb2ead837`](https://github.com/Djihads80/Eglantine/commit/a08eb2ead837) | 2026-11-01 | Takoroka | vulkan: 16 MiB memory blocks |
| [`1f0e3b88c4f8`](https://github.com/Djihads80/Eglantine/commit/1f0e3b88c4f8) | 2026-11-01 | Takoroka | vulkan: keep sparse pages and device-local buffers out of the image blocks |
| [`50fb1556cb00`](https://github.com/Djihads80/Eglantine/commit/50fb1556cb00) | 2026-11-01 | Takoroka | vulkan: 32 MiB memory blocks and best-fit placement for images |
| [`09d1315f16b8`](https://github.com/Djihads80/Eglantine/commit/09d1315f16b8) | 2026-11-01 | Takoroka | common: forget a sparse vector's region when it is freed |
| [`f398ff2e17f4`](https://github.com/Djihads80/Eglantine/commit/f398ff2e17f4) | 2026-11-01 | Takoroka | texture_cache: free the slots of deleted allocations and framebuffers |
| [`eca63ccb1006`](https://github.com/Djihads80/Eglantine/commit/eca63ccb1006) | 2026-11-01 | Takoroka | vfs_real: forget a path when its file is gone |
| [`5261efa901dc`](https://github.com/Djihads80/Eglantine/commit/5261efa901dc) | 2026-11-01 | Takoroka | file_sys: don't let a RomFS build's nodes own each other |
| [`22c50e7cab66`](https://github.com/Djihads80/Eglantine/commit/22c50e7cab66) | 2026-11-01 | Takoroka | vfs_real: keep only the path in a real file |
| [`a999d34f393a`](https://github.com/Djihads80/Eglantine/commit/a999d34f393a) | 2026-11-01 | Takoroka | vk_pipeline_cache: cap boot builds when no used list exists yet |
| [`6bde8c20d1f7`](https://github.com/Djihads80/Eglantine/commit/6bde8c20d1f7) | 2026-11-01 | kraibse | vulkan: treat RADV like the other AMD drivers for unbound sparse images |
| [`3d6156434a96`](https://github.com/Djihads80/Eglantine/commit/3d6156434a96) | 2026-11-01 | kraibse | nx2: guard empty and overflowing 216 map requests, log failed whole-backing |
| [`130c255ed91e`](https://github.com/Djihads80/Eglantine/commit/130c255ed91e) | 2026-11-01 | Takoroka | vulkan: share one zeroed block between empty sparse blocks on AMD |
| [`1c794af56407`](https://github.com/Djihads80/Eglantine/commit/1c794af56407) | 2026-11-01 | Takoroka | texture_cache: back large sparse 2D images by sparse block |
| [`a855e8373f55`](https://github.com/Djihads80/Eglantine/commit/a855e8373f55) | 2026-10-31 | kraibse | vulkan: back large sparse images normally on AMD |
| [`139b7e5efb7c`](https://github.com/Djihads80/Eglantine/commit/139b7e5efb7c) | 2026-10-31 | Takoroka | nx2: count a mapped sparse tile as written |
| [`4d6a13e3d9ea`](https://github.com/Djihads80/Eglantine/commit/4d6a13e3d9ea) | 2026-10-31 | Takoroka | nx2: map and unmap sparse texture tiles (nx2:gpu 216) |
| [`004ca9653181`](https://github.com/Djihads80/Eglantine/commit/004ca9653181) | 2026-10-30 | Takoroka | NX2_NOTES: task/mesh programs and their support |
| [`e6538fbc977d`](https://github.com/Djihads80/Eglantine/commit/e6538fbc977d) | 2026-10-30 | Takoroka | video_core: EDEN_NX2_NO_MESH_FILE toggles task/mesh draws at run time |
| [`9243382264db`](https://github.com/Djihads80/Eglantine/commit/9243382264db) | 2026-10-30 | Takoroka | video_core: keep task/mesh pipelines out of the disk pipeline cache |
| [`6d9821d50bec`](https://github.com/Djihads80/Eglantine/commit/6d9821d50bec) | 2026-10-30 | Takoroka | video_core: EDEN_NX2_NO_MESH skips task/mesh draws |
| [`02504aeb37cb`](https://github.com/Djihads80/Eglantine/commit/02504aeb37cb) | 2026-10-30 | Takoroka | video_core: draw Switch 2 task/mesh pipelines with VK_EXT_mesh_shader |
| [`310dd596d0f6`](https://github.com/Djihads80/Eglantine/commit/310dd596d0f6) | 2026-10-30 | Takoroka | shader_recompiler: translate Switch 2 task and mesh programs |
| [`eb02d1dd53a0`](https://github.com/Djihads80/Eglantine/commit/eb02d1dd53a0) | 2026-10-30 | Takoroka | nx2: keep ISBERD/ISBEWR and lower UPOPC for task and mesh programs |
| [`6a95f4fb29d6`](https://github.com/Djihads80/Eglantine/commit/6a95f4fb29d6) | 2026-10-31 | Takoroka | nx2: lower TLD.SCR with a texel offset |
| [`27cf4285e054`](https://github.com/Djihads80/Eglantine/commit/27cf4285e054) | 2026-10-31 | Takoroka | nx2: lower TLD4 with a texel offset |
| [`b0043b6065ab`](https://github.com/Djihads80/Eglantine/commit/b0043b6065ab) | 2026-10-31 | Takoroka | nx2: lower ISETP.EX, the high half of 64-bit compares |
| [`cefcf3a9b1c7`](https://github.com/Djihads80/Eglantine/commit/cefcf3a9b1c7) | 2026-10-30 | Takoroka | nx2: keep the uniform clamp before a jump table when instructions sit between |
| [`01389ee5266f`](https://github.com/Djihads80/Eglantine/commit/01389ee5266f) | 2026-10-30 | Takoroka | nx2: mirror uniform registers above UR27 below R216 |
| [`e39825807109`](https://github.com/Djihads80/Eglantine/commit/e39825807109) | 2026-10-30 | Takoroka | nx2: lower TMML.B |
| [`a08454340e52`](https://github.com/Djihads80/Eglantine/commit/a08454340e52) | 2026-10-30 | Takoroka | nx2: lower TEX.B with an explicit LOD |
| [`563ed8e60b0d`](https://github.com/Djihads80/Eglantine/commit/563ed8e60b0d) | 2026-10-30 | Takoroka | nx2: lower TLD4 with a uniform handle |
| [`6e289d893535`](https://github.com/Djihads80/Eglantine/commit/6e289d893535) | 2026-10-30 | Takoroka | nx2: lower TMML for 1D, 3D and cube textures and with a uniform handle |
| [`b14a268ebce6`](https://github.com/Djihads80/Eglantine/commit/b14a268ebce6) | 2026-10-30 | Takoroka | nx2: lower TLD.B at level zero |
| [`7aa9857432f8`](https://github.com/Djihads80/Eglantine/commit/7aa9857432f8) | 2026-10-30 | Takoroka | nx2: lower TXQ.B and TXQ with three or four outputs |
| [`23b194b84a3e`](https://github.com/Djihads80/Eglantine/commit/23b194b84a3e) | 2026-10-30 | Takoroka | nx2: place TEX.B's handle after the coordinates; take .DC and .LZ |
| [`328d8df61274`](https://github.com/Djihads80/Eglantine/commit/328d8df61274) | 2026-10-30 | Takoroka | nx2: lower IADD3.X with a register and a constant buffer operand |
| [`9e2e50aed768`](https://github.com/Djihads80/Eglantine/commit/9e2e50aed768) | 2026-10-30 | Takoroka | nx2: lower REDUX.MIN |
| [`f4d79570e995`](https://github.com/Djihads80/Eglantine/commit/f4d79570e995) | 2026-10-30 | Takoroka | nx2: lower MATCH.ALL into P6 |
| [`f74256da81ab`](https://github.com/Djihads80/Eglantine/commit/f74256da81ab) | 2026-10-30 | Takoroka | nx2: lower USHF on uniform registers above UR15 |
| [`3fc3c016d541`](https://github.com/Djihads80/Eglantine/commit/3fc3c016d541) | 2026-10-30 | Takoroka | nx2: lower FSETP with a uniform register operand |
| [`f963645e8913`](https://github.com/Djihads80/Eglantine/commit/f963645e8913) | 2026-10-30 | Takoroka | nx2: lower FRND with a constant buffer source |
| [`7998b0bc5532`](https://github.com/Djihads80/Eglantine/commit/7998b0bc5532) | 2026-10-30 | Takoroka | nx2: lower UIMAD.WIDE.U32 with an immediate multiplier |
| [`151ad5c59813`](https://github.com/Djihads80/Eglantine/commit/151ad5c59813) | 2026-10-30 | Takoroka | nx2: lower I2F.U8 from any byte of its source |
| [`0ee1e4f64768`](https://github.com/Djihads80/Eglantine/commit/0ee1e4f64768) | 2026-10-30 | Takoroka | nx2: read the local load/store size from bits 73-75 |
| [`e71bb09e2da0`](https://github.com/Djihads80/Eglantine/commit/e71bb09e2da0) | 2026-10-30 | Takoroka | nx2: lower UPRMT |
| [`6a96ec2cdcd8`](https://github.com/Djihads80/Eglantine/commit/6a96ec2cdcd8) | 2026-10-30 | Takoroka | nx2: read SR_VARIABLE_RATE as 1x1 shading |
| [`fdca67d7312c`](https://github.com/Djihads80/Eglantine/commit/fdca67d7312c) | 2026-10-30 | Takoroka | nx2: lower LDC.IS, constant loads with segmented addressing |
| [`716539186bec`](https://github.com/Djihads80/Eglantine/commit/716539186bec) | 2026-10-30 | Takoroka | nx2: lower IMAD.WIDE.U32 with an immediate multiplier |
| [`de1929c9346d`](https://github.com/Djihads80/Eglantine/commit/de1929c9346d) | 2026-10-30 | Takoroka | nx2: lower bindless TEX (handle in a register or a uniform register) |
| [`732f91ded38d`](https://github.com/Djihads80/Eglantine/commit/732f91ded38d) | 2026-10-30 | Takoroka | nx2: lower LEA with a carry out |
| [`b52f29993f4a`](https://github.com/Djihads80/Eglantine/commit/b52f29993f4a) | 2026-10-30 | Takoroka | nx2: lower CSMTEST as a false test |
| [`572c89d71346`](https://github.com/Djihads80/Eglantine/commit/572c89d71346) | 2026-10-30 | Takoroka | nx2: lower LEA with a uniform register addend |
| [`b2dd8f113441`](https://github.com/Djihads80/Eglantine/commit/b2dd8f113441) | 2026-10-30 | Takoroka | nx2: lower 128-bit and register-addressed local spills |
| [`6e38b4c8fbb6`](https://github.com/Djihads80/Eglantine/commit/6e38b4c8fbb6) | 2026-10-30 | Takoroka | nx2: lower REDUX.OR and REDUX.XOR |
| [`d88554cedb39`](https://github.com/Djihads80/Eglantine/commit/d88554cedb39) | 2026-10-30 | Takoroka | nx2: lower every ATOMS operation |
| [`2f2b0a746832`](https://github.com/Djihads80/Eglantine/commit/2f2b0a746832) | 2026-10-30 | Takoroka | nx2: lower ATOMS.POPC.INC with an immediate offset |
| [`f9c6ae332a0f`](https://github.com/Djihads80/Eglantine/commit/f9c6ae332a0f) | 2026-10-30 | Takoroka | nx2: lower SULD.P.3D |
| [`9df12f005178`](https://github.com/Djihads80/Eglantine/commit/9df12f005178) | 2026-10-30 | Takoroka | nx2: lower TLD.SCR.LL, texel fetches at a given level |
| [`f7253f24f7f2`](https://github.com/Djihads80/Eglantine/commit/f7253f24f7f2) | 2026-10-30 | Takoroka | nx2: lower TXQ on any texture descriptor buffer |
| [`567aaa3e8824`](https://github.com/Djihads80/Eglantine/commit/567aaa3e8824) | 2026-10-30 | Takoroka | nx2: lower 32-bit MATCH.ALL |
| [`fa1f6b1d7ba4`](https://github.com/Djihads80/Eglantine/commit/fa1f6b1d7ba4) | 2026-10-30 | Takoroka | nx2: pass more special registers through S2R lowering |
| [`6a10b2750747`](https://github.com/Djihads80/Eglantine/commit/6a10b2750747) | 2026-10-30 | Takoroka | nx2: lower MUFU with a uniform register source |
| [`0bd5130d2dc3`](https://github.com/Djihads80/Eglantine/commit/0bd5130d2dc3) | 2026-10-30 | Takoroka | nx2: lower SURED.ADD on a 2D surface |
| [`fce9c5fd25c6`](https://github.com/Djihads80/Eglantine/commit/fce9c5fd25c6) | 2026-11-01 | Takoroka | vulkan_device: tell AMD GCN4 apart before float16 is turned off |
| [`bbe3464f410e`](https://github.com/Djihads80/Eglantine/commit/bbe3464f410e) | 2026-10-31 | Takoroka | ssl: give three ssl replies their data |
| [`9039dfe9630a`](https://github.com/Djihads80/Eglantine/commit/9039dfe9630a) | 2026-10-31 | Takoroka | ssl: return a context from ssl:s CreateContext |
| [`959973a76aba`](https://github.com/Djihads80/Eglantine/commit/959973a76aba) | 2026-11-01 | Takoroka | externals: FFmpeg with AV1 on Windows and Linux x86_64 |
| [`39b9892f000b`](https://github.com/Djihads80/Eglantine/commit/39b9892f000b) | 2026-11-01 | Takoroka | vulkan_device: keep an integrated GPU's budget within its memory share |
| [`86968cb5ce31`](https://github.com/Djihads80/Eglantine/commit/86968cb5ce31) | 2026-11-01 | Takoroka | buffer_cache: mark guest memory large buffer uploads came from cold |
| [`e2c512916c6f`](https://github.com/Djihads80/Eglantine/commit/e2c512916c6f) | 2026-11-01 | Takoroka | texture_cache: mark guest memory textures were uploaded from cold |
| [`6d4c595634f8`](https://github.com/Djihads80/Eglantine/commit/6d4c595634f8) | 2026-11-01 | Takoroka | vulkan: reuse ASTC transcode decode images |
| [`7b6b10b3bc86`](https://github.com/Djihads80/Eglantine/commit/7b6b10b3bc86) | 2026-11-01 | Takoroka | vk_staging_buffer_pool: 128 MiB stream buffer everywhere |
| [`70b39fe1d247`](https://github.com/Djihads80/Eglantine/commit/70b39fe1d247) | 2026-11-01 | Takoroka | vk_pipeline_cache: leave the pipeline cache to NVIDIA's and AMD's drivers |
| [`ab943ab4f32a`](https://github.com/Djihads80/Eglantine/commit/ab943ab4f32a) | 2026-11-01 | Takoroka | sw_blitter: keep scratch buffers of up to 2 MiB between blits |
| [`594e07ee5302`](https://github.com/Djihads80/Eglantine/commit/594e07ee5302) | 2026-11-01 | Takoroka | buffer_cache: collect Switch 2 buffers from 512 MiB |
| [`3871de9da994`](https://github.com/Djihads80/Eglantine/commit/3871de9da994) | 2026-10-31 | Takoroka | texture_cache: find a deleted image's view entries without a scan |
| [`446c34aa8b30`](https://github.com/Djihads80/Eglantine/commit/446c34aa8b30) | 2026-10-31 | Takoroka | texture_cache: free deleted images once the GPU is past them |
| [`2f94fb9680cb`](https://github.com/Djihads80/Eglantine/commit/2f94fb9680cb) | 2026-10-31 | Takoroka | vk_pipeline_cache: destroy graphics pipelines unused for 9000 frames |
| [`8551ee6462b6`](https://github.com/Djihads80/Eglantine/commit/8551ee6462b6) | 2026-11-01 | Takoroka | vulkan: drop shader modules once their pipeline is built |
| [`f8813998e7dd`](https://github.com/Djihads80/Eglantine/commit/f8813998e7dd) | 2026-11-01 | Takoroka | loader: build a loose RomFS once and only when it is read |
| [`9c83502ed8bb`](https://github.com/Djihads80/Eglantine/commit/9c83502ed8bb) | 2026-11-01 | Takoroka | fiber: leave guest thread stacks uninitialized, drop the unused rewind stack |
| [`adae7635708e`](https://github.com/Djihads80/Eglantine/commit/adae7635708e) | 2026-11-01 | Djihads80 | Fixed Pokopia stuck on ditto in linux by bundeling Opus with build |
| [`9c8af5b21288`](https://github.com/Djihads80/Eglantine/commit/9c8af5b21288) | 2026-11-01 | Takoroka | vulkan: keep the stream buffer within a small iGPU heap |
| [`edabc3e61486`](https://github.com/Djihads80/Eglantine/commit/edabc3e61486) | 2026-11-01 | Takoroka | memory_manager: copy big pages without host memory page by page |
| [`5de94bdd1d24`](https://github.com/Djihads80/Eglantine/commit/5de94bdd1d24) | 2026-10-31 | Takoroka | vk_pipeline_cache: build only the last two sessions' pipelines at boot |
| [`2b413a46a282`](https://github.com/Djihads80/Eglantine/commit/2b413a46a282) | 2026-11-01 | néo Luigi | Vibration fix for ns2 games \| Assisted by claude (code) |
| [`37908985af9d`](https://github.com/Djihads80/Eglantine/commit/37908985af9d) | 2026-10-31 | Yuri Olive | cmake: skip the pre-commit hook copy when .git is not a directory |
| [`dbd9572a5a03`](https://github.com/Djihads80/Eglantine/commit/dbd9572a5a03) | 2026-10-31 | Takoroka | texture_cache: delete images without walking every channel's tables |
| [`8266a6a91b7a`](https://github.com/Djihads80/Eglantine/commit/8266a6a91b7a) | 2026-10-31 | Takoroka | textures: ASTC opaque scan from a table of block modes |
| [`f61ea2a62d92`](https://github.com/Djihads80/Eglantine/commit/f61ea2a62d92) | 2026-10-30 | Sssnowyyy | vk_pipeline_cache: skip unused colour targets when picking MoltenVK output types |
| [`b8813c6797fc`](https://github.com/Djihads80/Eglantine/commit/b8813c6797fc) | 2026-10-30 | Sssnowyyy | eden-cli: set the window title from the main thread |
| [`514cc97980e6`](https://github.com/Djihads80/Eglantine/commit/514cc97980e6) | 2026-10-30 | Sssnowyyy | macOS: fix the build with AppleClang and use the Eglantine app icon (in part) |
| [`b48afba6d34d`](https://github.com/Djihads80/Eglantine/commit/b48afba6d34d) | 2026-10-31 | Adya | Fix Compilation and crash on MACOS (in part) |
| [`e36d46693b0c`](https://github.com/Djihads80/Eglantine/commit/e36d46693b0c) | 2026-10-31 | Omar Mahboubi | Initial linux aarch64 commit (in part) |
| [`919368bec2bf`](https://github.com/Djihads80/Eglantine/commit/919368bec2bf) | 2026-10-31 | Djihads80 | Added FSR 4 and AMD Boost (in part) |
| [`bb881e290dc0`](https://github.com/Djihads80/Eglantine/commit/bb881e290dc0) | 2026-10-31 | rdenno | am: software keyboard config revision 0x140010 (Switch 2) uses UTF-8 text |
| [`dee08ca5afb8`](https://github.com/Djihads80/Eglantine/commit/dee08ca5afb8) | 2026-10-31 | rdenno | am: map Switch 2 library applet ids to the applet in their low byte (Pokopia name entry) |
| [`75385e994dc8`](https://github.com/Djihads80/Eglantine/commit/75385e994dc8) | 2026-10-31 | Takoroka | texture_cache: don't count deleted images still waiting to be destroyed |
| [`561f87abec2f`](https://github.com/Djihads80/Eglantine/commit/561f87abec2f) | 2026-10-31 | Takoroka | shader: compute texture handles read from storage buffers for each draw |
| [`77ea16e2f26e`](https://github.com/Djihads80/Eglantine/commit/77ea16e2f26e) | 2026-10-31 | Takoroka | nx2: drop R2UR's unreachable keep-old-value select when unpredicated |
| [`9dca3ab2e436`](https://github.com/Djihads80/Eglantine/commit/9dca3ab2e436) | 2026-10-31 | néo Luigi | Analysis for the change |
| [`41a81ab5e3d8`](https://github.com/Djihads80/Eglantine/commit/41a81ab5e3d8) | 2026-10-31 | néo Luigi | Gpu buffer readback to cpu. Fixing visible glitches such as missing collision on some object, but can extend to more. |
| [`0a47ba956765`](https://github.com/Djihads80/Eglantine/commit/0a47ba956765) | 2026-10-31 | Kaydax | nx2: crop handheld present images to the 1920x1080 panel |
| [`c08864b3fed3`](https://github.com/Djihads80/Eglantine/commit/c08864b3fed3) | 2026-10-31 | kraibse | refactor: remove unused gamepad button and axis event handling in SDLDriver |
| [`1051d394d206`](https://github.com/Djihads80/Eglantine/commit/1051d394d206) | 2026-10-31 | kraibse | refactor: enhance GPU memory management with MappedBacking structure to prevent a deadlock between GPU and guest thread |
| [`82963e6750ce`](https://github.com/Djihads80/Eglantine/commit/82963e6750ce) | 2026-10-31 | néo Luigi | 1st pass of review: code cleanup & windows test |
| [`2065b1431c0e`](https://github.com/Djihads80/Eglantine/commit/2065b1431c0e) | 2026-10-31 | Wael3rd | texture_cache: keep Switch 2 VRAM use under the driver's budget |
| [`b23915de2049`](https://github.com/Djihads80/Eglantine/commit/b23915de2049) | 2026-10-31 | sdas-19 | fix: resolve frame_gen double binding in graphics configuration |
| [`ee2d55ee497c`](https://github.com/Djihads80/Eglantine/commit/ee2d55ee497c) | 2026-10-31 | Alex | boot Pokopia past audio init (Switch 2 heap limits, aud:u 1, stpl:u) (#77) |
| [`b610259c74ab`](https://github.com/Djihads80/Eglantine/commit/b610259c74ab) | 2026-11-01 | shiv121231 | Suppress unused warnings with WERROR enabled |
| [`049c9633edd9`](https://github.com/Djihads80/Eglantine/commit/049c9633edd9) | 2026-10-31 | Takoroka | nx2: read texture handles that hold a header byte offset |
| [`bcb98dc1f7a8`](https://github.com/Djihads80/Eglantine/commit/bcb98dc1f7a8) | 2026-11-01 | shiv121231 | Implement platform-specific handling for -Werror |
| [`574517814d1e`](https://github.com/Djihads80/Eglantine/commit/574517814d1e) | 2026-10-31 | Takoroka | nx2: free decoder output images on gsv 912 |
| [`140489e51fc6`](https://github.com/Djihads80/Eglantine/commit/140489e51fc6) | 2026-10-31 | Takoroka | nx2: give the Switch 2 audio streams (auddmg:u, audsmx:u) real objects |
| [`af7289e62fe0`](https://github.com/Djihads80/Eglantine/commit/af7289e62fe0) | 2026-11-01 | shiv121231 | Add files via upload |
| [`e82731c2c9fb`](https://github.com/Djihads80/Eglantine/commit/e82731c2c9fb) | 2026-10-31 | Djibril | build: require Boost 1.84 or newer (#94) |
| [`d4e2dc886112`](https://github.com/Djihads80/Eglantine/commit/d4e2dc886112) | 2026-10-31 | QuantumDriftDev | Fix game list crash, unresponsive macOS menu, and missing SDL gamepad input (#100) |
| [`df8d77511bcc`](https://github.com/Djihads80/Eglantine/commit/df8d77511bcc) | 2026-10-31 | Takoroka | audio: don't free an upsampler when cleaning up a circular buffer sink |
| [`914423e87de9`](https://github.com/Djihads80/Eglantine/commit/914423e87de9) | 2026-10-31 | Takoroka | dynarmic: drop the cached code page when a range is invalidated |
| [`4d2cf4183743`](https://github.com/Djihads80/Eglantine/commit/4d2cf4183743) | 2026-10-31 | Meow | game_list, loader: find loose ExeFS dumps and show their name and icon |
| [`da17ced530d4`](https://github.com/Djihads80/Eglantine/commit/da17ced530d4) | 2026-10-31 | Meow | refactor: move MaskValues to a header file and cleaning |
| [`b7575b0305dc`](https://github.com/Djihads80/Eglantine/commit/b7575b0305dc) | 2026-10-31 | Wael3rd | nvdrv: clear the sparse region state with the other Switch 2 globals |
| [`bb353609e6c2`](https://github.com/Djihads80/Eglantine/commit/bb353609e6c2) | 2026-10-31 | arisqz | AMD only, green/blue patches near glowing objects. |
| [`59047f0e6fd1`](https://github.com/Djihads80/Eglantine/commit/59047f0e6fd1) | 2026-10-31 | Wael3rd | shader_recompiler: run the destructors of pooled objects |
| [`837133792d56`](https://github.com/Djihads80/Eglantine/commit/837133792d56) | 2026-10-31 | Takoroka | nx2: map GPU memory on the GPU thread |
| [`7d82146663c0`](https://github.com/Djihads80/Eglantine/commit/7d82146663c0) | 2026-10-31 | Takoroka | nx2: shrink the graphics server pool to 2 GiB |
| [`6bc1ef08c29d`](https://github.com/Djihads80/Eglantine/commit/6bc1ef08c29d) | 2026-10-30 | Takoroka | nx2: back GPU-only regions as they are written |
| [`076b390110b6`](https://github.com/Djihads80/Eglantine/commit/076b390110b6) | 2026-10-31 | Takoroka | nx2: find the upscale images outside title 4's slots |
| [`520826452f76`](https://github.com/Djihads80/Eglantine/commit/520826452f76) | 2026-10-31 | Takoroka | nvdrv: let Switch 2 callers open nvhost-prof-gpu |
| [`08f82cc00ca9`](https://github.com/Djihads80/Eglantine/commit/08f82cc00ca9) | 2026-10-31 | Takoroka | nx2: hand out graphics device ids for gpu 41 and 42 |
| [`28f66d0c8b12`](https://github.com/Djihads80/Eglantine/commit/28f66d0c8b12) | 2026-10-31 | Takoroka | nsd: fix the reply sizes of 15 and 21 |
| [`28b23be30fb4`](https://github.com/Djihads80/Eglantine/commit/28b23be30fb4) | 2026-10-31 | Takoroka | am: reply with 12 bytes for GetCurrentGraphicsScale |
| [`c3e530625e06`](https://github.com/Djihads80/Eglantine/commit/c3e530625e06) | 2026-10-31 | Takoroka | am: reply with data for IApplicationFunctions 320 and 321 |
| [`6f33896d8cad`](https://github.com/Djihads80/Eglantine/commit/6f33896d8cad) | 2026-10-31 | Takoroka | nx2: decode AV1 movies |
| [`1b726d93c586`](https://github.com/Djihads80/Eglantine/commit/1b726d93c586) | 2026-10-31 | Takoroka | nx2: video decoder commands of later SDKs |
| [`a03e816886f4`](https://github.com/Djihads80/Eglantine/commit/a03e816886f4) | 2026-10-31 | Takoroka | nx2: let the host page out texture pool backing first |
| [`4d8a9629a6f4`](https://github.com/Djihads80/Eglantine/commit/4d8a9629a6f4) | 2026-10-31 | Takoroka | texture_cache: collect from 5 GiB of device memory on every GPU |
| [`b096fbea6646`](https://github.com/Djihads80/Eglantine/commit/b096fbea6646) | 2026-10-31 | Takoroka | video_core: free software blit buffers grown for huge surfaces |
| [`e9f31461b6ff`](https://github.com/Djihads80/Eglantine/commit/e9f31461b6ff) | 2026-10-31 | Takoroka | dynarmic: commit x64 code cache memory as code is emitted |
| [`38230873a4a5`](https://github.com/Djihads80/Eglantine/commit/38230873a4a5) | 2026-10-31 | vibecodekun | shader_recompiler: fold max(x, -x) of merged predicate masks |
| [`be76402ab51d`](https://github.com/Djihads80/Eglantine/commit/be76402ab51d) | 2026-10-30 | Takoroka | vulkan: build at boot only the cached pipelines recent sessions used |
| [`1b7c637c615d`](https://github.com/Djihads80/Eglantine/commit/1b7c637c615d) | 2026-10-30 | Takoroka | video_core: software 2D blits touch only the rows they cover |
| [`b69f7394f857`](https://github.com/Djihads80/Eglantine/commit/b69f7394f857) | 2026-10-30 | Takoroka | settings: recompress ASTC to BC3 by default |
| [`e24a1190172b`](https://github.com/Djihads80/Eglantine/commit/e24a1190172b) | 2026-10-30 | Meow | game_list: find loose ExeFS dumps in shallow scans |
| [`0047b69daeb5`](https://github.com/Djihads80/Eglantine/commit/0047b69daeb5) | 2026-10-30 | Meow | loader: read a loose dump's name and icon from game-folder sidecars |
| [`0f146960444b`](https://github.com/Djihads80/Eglantine/commit/0f146960444b) | 2026-10-30 | Takoroka | nx2: wait for the presented frame too when the GPU is behind |
| [`affa8969acb3`](https://github.com/Djihads80/Eglantine/commit/affa8969acb3) | 2026-10-30 | Takoroka | nx2: ignore syncpoint signals past the syncpoint table |
| [`a4a746b47b1d`](https://github.com/Djihads80/Eglantine/commit/a4a746b47b1d) | 2026-10-30 | xtam | fix: restore Mario Kart World text by accepting R0 shader returns |
| [`3e950dd677b6`](https://github.com/Djihads80/Eglantine/commit/3e950dd677b6) | 2026-10-30 | Takoroka | loader: read a loose dump's display version from its own files |
| [`ca524c9aa883`](https://github.com/Djihads80/Eglantine/commit/ca524c9aa883) | 2026-10-30 | givethesourceplox | vulkan: keep the image's view type when a shader's dimension disagrees |
| [`bef001c1515a`](https://github.com/Djihads80/Eglantine/commit/bef001c1515a) | 2026-10-30 | Takoroka | nx2: read command lists the GPU wrote from the GPU's copy |
| [`2ad0830d2f04`](https://github.com/Djihads80/Eglantine/commit/2ad0830d2f04) | 2026-10-30 | Takoroka | video_core: draw deferred instances before the state after their loop |
| [`d2ac2275247a`](https://github.com/Djihads80/Eglantine/commit/d2ac2275247a) | 2026-10-30 | néo Luigi | Fixed game support about docked/handheld mode, applying only to ns2 games. Tested on windows. |
| [`8a72dc011e0a`](https://github.com/Djihads80/Eglantine/commit/8a72dc011e0a) | 2026-10-30 | Djihads80 | Added FSR 3 Upscaling and Frame Generation |
| [`ddd367a8152f`](https://github.com/Djihads80/Eglantine/commit/ddd367a8152f) | 2026-10-30 | Takoroka | core: end device page runs where pages are unmapped or mapped again |
| [`ea33ad186597`](https://github.com/Djihads80/Eglantine/commit/ea33ad186597) | 2026-10-30 | givethesourceplox | vulkan: write the driver pipeline cache atomically; reject truncated ones |
| [`36d9276b1ae5`](https://github.com/Djihads80/Eglantine/commit/36d9276b1ae5) | 2026-10-30 | Takoroka | svc: name the handles a traced WaitSynchronization waits on |
| [`6f5e137ec4eb`](https://github.com/Djihads80/Eglantine/commit/6f5e137ec4eb) | 2026-10-30 | kraibse | texture_cache: log unbacked layers over the image's memory cap |
| [`d4ee7fcd5129`](https://github.com/Djihads80/Eglantine/commit/d4ee7fcd5129) | 2026-10-30 | Takoroka | vulkan: bind memory to sparse terrain arrays layer by layer |
| [`940d44ca2562`](https://github.com/Djihads80/Eglantine/commit/940d44ca2562) | 2026-10-30 | Takoroka | texture_cache: give images over unwritten sparse blocks no device memory |
| [`97ce27ab3515`](https://github.com/Djihads80/Eglantine/commit/97ce27ab3515) | 2026-10-30 | Takoroka | texture_cache: re-decode only the written layers of large ASTC arrays |
| [`6380b6687ded`](https://github.com/Djihads80/Eglantine/commit/6380b6687ded) | 2026-10-30 | Takoroka | nx2: give sparse regions memory only where the GPU writes |
| [`f9fc1e10fd7e`](https://github.com/Djihads80/Eglantine/commit/f9fc1e10fd7e) | 2026-10-30 | Takoroka | memory_manager: notify the caches once per changed range |
| [`b8794af274a0`](https://github.com/Djihads80/Eglantine/commit/b8794af274a0) | 2026-10-30 | Takoroka | vulkan: keep a failing shader's dump from ending the process |
| [`f669bbbfff70`](https://github.com/Djihads80/Eglantine/commit/f669bbbfff70) | 2026-10-30 | Takoroka | file_sys: serve an empty archive for missing Switch 2 system data |
| [`cdce22cd06ce`](https://github.com/Djihads80/Eglantine/commit/cdce22cd06ce) | 2026-10-30 | Takoroka | audio_core: accept audio renderer revision 16 |
| [`37893c683143`](https://github.com/Djihads80/Eglantine/commit/37893c683143) | 2026-10-30 | Takoroka | ro: read Switch 2 NRR hash lists where they are |
| [`c96d1e01931f`](https://github.com/Djihads80/Eglantine/commit/c96d1e01931f) | 2026-10-30 | Takoroka | file_sys: don't format a null name for an unknown system archive |
| [`75b707a90ac8`](https://github.com/Djihads80/Eglantine/commit/75b707a90ac8) | 2026-10-30 | ZnOw01 | yuzu_cmd: skip kernel teardown when nothing was booted |
| [`e6d2ef69e70f`](https://github.com/Djihads80/Eglantine/commit/e6d2ef69e70f) | 2026-10-30 | Takoroka | ns: give Switch 2 built-in fonts their own shared memory |
| [`24e3cb462e47`](https://github.com/Djihads80/Eglantine/commit/24e3cb462e47) | 2026-10-30 | givethesourceplox | core: return empty perf stats before the system is running (#30) |
| [`dfe734dce6ae`](https://github.com/Djihads80/Eglantine/commit/dfe734dce6ae) | 2026-10-30 | Takoroka | Revert "textures: (un)swizzle large images in bands on the transcode workers" |
| [`dfb3241838c1`](https://github.com/Djihads80/Eglantine/commit/dfb3241838c1) | 2026-10-30 | kraibse | nx2: apply GPU accuracy changes mid-game by stopping and restarting the GPU split |
| [`d47078801042`](https://github.com/Djihads80/Eglantine/commit/d47078801042) | 2026-10-30 | Takoroka | textures: (un)swizzle large images in bands on the transcode workers |
| [`e6d53e728a93`](https://github.com/Djihads80/Eglantine/commit/e6d53e728a93) | 2026-10-30 | Takoroka | dynarmic: skip untouched locations when an IR instruction ends |
| [`9a5a1fbdf688`](https://github.com/Djihads80/Eglantine/commit/9a5a1fbdf688) | 2026-10-30 | Takoroka | dynarmic: skip untouched locations when an IR instruction ends |
| [`c7dffce68ad2`](https://github.com/Djihads80/Eglantine/commit/c7dffce68ad2) | 2026-10-30 | Takoroka | dynarmic: keep the x64 JIT's per-block tables compact |
| [`34f81e052c37`](https://github.com/Djihads80/Eglantine/commit/34f81e052c37) | 2026-10-30 | Takoroka | dynarmic: keep the x64 JIT's per-block tables compact |
| [`9ad01b60c8ce`](https://github.com/Djihads80/Eglantine/commit/9ad01b60c8ce) | 2026-10-30 | givethesourceplox | video_core: arm64 spin hint in the GPU split |
| [`487d04f8edf6`](https://github.com/Djihads80/Eglantine/commit/487d04f8edf6) | 2026-10-30 | givethesourceplox | yuzu_cmd: give MoltenVK the view's CAMetalLayer |
| [`adf02d0f1e63`](https://github.com/Djihads80/Eglantine/commit/adf02d0f1e63) | 2026-10-30 | givethesourceplox | vulkan: clamp compute shared memory on MoltenVK |
| [`9e4e58c8e54a`](https://github.com/Djihads80/Eglantine/commit/9e4e58c8e54a) | 2026-10-30 | givethesourceplox | yuzu_cmd: raise the open file limit on macOS and Linux |
| [`6e7edd746232`](https://github.com/Djihads80/Eglantine/commit/6e7edd746232) | 2026-10-30 | kraibse | fix: Improve QMD cache management by removing completed entries to prevent stalling and memory leakage |
| [`7f328339ddd2`](https://github.com/Djihads80/Eglantine/commit/7f328339ddd2) | 2026-10-30 | Takoroka | vulkan: bind memory to sparse terrain arrays layer by layer |
| [`862ab88831e5`](https://github.com/Djihads80/Eglantine/commit/862ab88831e5) | 2026-10-30 | Takoroka | texture_cache: give images over unwritten sparse blocks no device memory |
| [`f260bcb5a26a`](https://github.com/Djihads80/Eglantine/commit/f260bcb5a26a) | 2026-10-30 | Takoroka | memory: skip clearing guest memory that is still zero |
| [`f0d1bbff79e9`](https://github.com/Djihads80/Eglantine/commit/f0d1bbff79e9) | 2026-10-30 | Takoroka | vk_texture_cache: trim huge ASTC arrays harder on small GPUs |
| [`8739989d3494`](https://github.com/Djihads80/Eglantine/commit/8739989d3494) | 2026-10-30 | Takoroka | service: report each auto-stubbed command once |
| [`bed068523eb0`](https://github.com/Djihads80/Eglantine/commit/bed068523eb0) | 2026-10-30 | Takoroka | texture_cache: re-decode only the written layers of large ASTC arrays |
| [`920f3f6ebdba`](https://github.com/Djihads80/Eglantine/commit/920f3f6ebdba) | 2026-10-30 | Takoroka | nx2: give sparse regions memory only where the GPU writes |
| [`0a13c9b2e09f`](https://github.com/Djihads80/Eglantine/commit/0a13c9b2e09f) | 2026-10-30 | Takoroka | memory_manager: notify the caches once per changed range |
| [`cb4fbb73f8f1`](https://github.com/Djihads80/Eglantine/commit/cb4fbb73f8f1) | 2026-10-30 | Takoroka | memory: skip clearing guest memory that is still zero |
| [`4ec2e0c5de97`](https://github.com/Djihads80/Eglantine/commit/4ec2e0c5de97) | 2026-10-30 | Takoroka | vk_texture_cache: trim huge ASTC arrays harder on small GPUs |
| [`ad7b3b6c63dc`](https://github.com/Djihads80/Eglantine/commit/ad7b3b6c63dc) | 2026-10-30 | Takoroka | service: report each auto-stubbed command once |
| [`5c7d304cf525`](https://github.com/Djihads80/Eglantine/commit/5c7d304cf525) | 2026-10-30 | Takoroka | loader: read a loose dump's display version from its own files |
| [`0218eacfd560`](https://github.com/Djihads80/Eglantine/commit/0218eacfd560) | 2026-10-30 | Takoroka | nx2: run the guest counter at 31.25 MHz for Switch 2 titles |
| [`092c16d6f39b`](https://github.com/Djihads80/Eglantine/commit/092c16d6f39b) | 2026-10-30 | Takoroka | gpu_split: reset the ring when the split stops |
| [`59fce4d557a2`](https://github.com/Djihads80/Eglantine/commit/59fce4d557a2) | 2026-10-30 | Takoroka | nvdrv: reset the Switch 2 GPU globals when the module is created |
| [`d22a03efcd0a`](https://github.com/Djihads80/Eglantine/commit/d22a03efcd0a) | 2026-10-30 | Takoroka | kernel: clear the fastmem arena when the application process is closed |
| [`b937805eb573`](https://github.com/Djihads80/Eglantine/commit/b937805eb573) | 2026-10-30 | Takoroka | core_timing: request the timer thread's stop before waking it |
| [`c90ce4034dd1`](https://github.com/Djihads80/Eglantine/commit/c90ce4034dd1) | 2026-10-30 | Takoroka | nvdrv: reset the Switch 2 GPU globals when the module is created |
| [`74cbdf00565e`](https://github.com/Djihads80/Eglantine/commit/74cbdf00565e) | 2026-10-30 | Takoroka | kernel: clear the fastmem arena when the application process is closed |
| [`636d4526108a`](https://github.com/Djihads80/Eglantine/commit/636d4526108a) | 2026-10-30 | Takoroka | core_timing: request the timer thread's stop before waking it |
| [`a1d47717ae7f`](https://github.com/Djihads80/Eglantine/commit/a1d47717ae7f) | 2026-10-30 | Takoroka | nx2: log per-call and periodic diagnostic lines at debug level |
| [`68d166f43810`](https://github.com/Djihads80/Eglantine/commit/68d166f43810) | 2026-10-30 | Takoroka | nx2: crop Mario Kart World's frame to 1920x1080 in handheld mode |
| [`1eb97beca0e7`](https://github.com/Djihads80/Eglantine/commit/1eb97beca0e7) | 2026-10-29 | Takoroka | nx2: log per-call GPU memory, syncpoint and shader lowering lines at debug level |
| [`da7752585c46`](https://github.com/Djihads80/Eglantine/commit/da7752585c46) | 2026-10-29 | yumerkiru | Fix MK World crash on initial loading screen for some users (AI-assisted) |
| [`9f774d502d07`](https://github.com/Djihads80/Eglantine/commit/9f774d502d07) | 2026-10-29 | Takoroka | texture_cache: don't shadow ImageBase::info in FindView |
| [`b09f5339c1e6`](https://github.com/Djihads80/Eglantine/commit/b09f5339c1e6) | 2026-10-29 | Takoroka | shader: compare the native FMUL/FADD mode bits as integers |
| [`2203754e5d05`](https://github.com/Djihads80/Eglantine/commit/2203754e5d05) | 2026-10-29 | Takoroka | cmake: find windeployqt for Windows builds with an installed Qt |
| [`a08fc6a6a990`](https://github.com/Djihads80/Eglantine/commit/a08fc6a6a990) | 2026-10-29 | Takoroka | nx2: lower 2D TXD and split destination pairs |
| [`42e9e37e081f`](https://github.com/Djihads80/Eglantine/commit/42e9e37e081f) | 2026-10-29 | Takoroka | eden-cli: use the entry point SDL provides in MSVC Release builds |
| [`04a64dee831e`](https://github.com/Djihads80/Eglantine/commit/04a64dee831e) | 2026-10-29 | Takoroka | sm: give Switch 2 programs a stub for services Eden does not have |
| [`7fa88f0bb18a`](https://github.com/Djihads80/Eglantine/commit/7fa88f0bb18a) | 2026-10-29 | Takoroka | game_list: name loose ExeFS dumps after their game folder |
| [`d56039ace993`](https://github.com/Djihads80/Eglantine/commit/d56039ace993) | 2026-10-30 | kraibse | Fix hang on emulation stop when audio renderer outlives its manager |
| [`dfa55d167975`](https://github.com/Djihads80/Eglantine/commit/dfa55d167975) | 2026-10-29 | lesmmwe | Update deconstructed_rom_directory.cpp |
| [`64d9632703d6`](https://github.com/Djihads80/Eglantine/commit/64d9632703d6) | 2026-10-30 | Djihads80 | Increased open file limit to prevent loading crashes |
| [`81700d032a82`](https://github.com/Djihads80/Eglantine/commit/81700d032a82) | 2026-10-29 | yumerkiru | Fix Raiders viewport crop (AI-assisted) |
| [`467ec722581e`](https://github.com/Djihads80/Eglantine/commit/467ec722581e) | 2026-10-29 | yvonne | vfs: keep the open-file cache below RLIMIT_NOFILE on Unix |
| [`50c9fd3471d5`](https://github.com/Djihads80/Eglantine/commit/50c9fd3471d5) | 2026-10-29 | theofficialhackclean | fixed window size for mkw |
| [`a8657997e1eb`](https://github.com/Djihads80/Eglantine/commit/a8657997e1eb) | 2026-10-29 | theofficialhackclean | push for fix for mkw |
| [`73cad97d7146`](https://github.com/Djihads80/Eglantine/commit/73cad97d7146) | 2026-10-29 | Takoroka | eden-cli: use the wmain entry point in Release builds |
| [`134310b81ece`](https://github.com/Djihads80/Eglantine/commit/134310b81ece) | 2026-10-29 | Djihads80 | Build fixes for linux |
| [`ea6c1193fbfa`](https://github.com/Djihads80/Eglantine/commit/ea6c1193fbfa) | 2026-10-29 | Takoroka | Revert "Import experimental NS2 support and accumulated GPU fixes" |
| [`535b6769709e`](https://github.com/Djihads80/Eglantine/commit/535b6769709e) | 2026-10-28 | yumekiru | Import experimental NS2 support and accumulated GPU fixes |

Ported from Eglantine pull requests before the merge (Droplet has carried them since v0.1.0):

- [`71f3d8d30420`](https://github.com/Djihads80/Eglantine/commit/71f3d8d30420) by Takoroka, pull request #41: GPU memory map notify ranges
- [`942c51c0b457`](https://github.com/Djihads80/Eglantine/commit/942c51c0b457) by Takoroka, pull request #41: GPU memory map notify ranges
- [`ef65ec709d34`](https://github.com/Djihads80/Eglantine/commit/ef65ec709d34) by givethesourceplox, pull request #35: yuzu_cmd: update the window title on the main thread
