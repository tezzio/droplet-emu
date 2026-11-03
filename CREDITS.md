# Credits

Droplet is built on the work of the projects below. Their copyright notices stay in the files they
cover, and their license texts are in [LICENSES](LICENSES).

## Where Droplet comes from

- **[eden-nx2](https://git.eden-emu.dev/Takoroka/eden-nx2)** by Takoroka: Nintendo Switch 2 support
  for Eden. Droplet is a modified version of eden-nx2 commit `50fa86b` ("nx2: fix the Linux (GCC)
  build of the vi2 present stats"), changed from 2026-10-30 on (see below). GPL-3.0-or-later.
- **[Eden](https://git.eden-emu.dev/eden-emu/eden)**, the Eden Emulator Project, which eden-nx2 is
  built on. GPL-3.0-or-later.
- **yuzu** (the yuzu Emulator Project) and **Sudachi**, which Eden continues, and **Citra**, where
  yuzu's code began. yuzu's code is GPL-2.0-or-later.
- **[Eglantine](https://github.com/Djihads80/Eglantine)**, an experimental Switch 2 emulator
  (GPL-3.0). Droplet carries these of its commits, by their authors:
  - Takoroka (eden-nx2's author): `636d4526108a` core_timing: request the timer thread's stop before
    waking it; `74cbdf00565e` kernel: clear the fastmem arena when the application process is closed;
    `c90ce4034dd1` nvdrv: reset the Switch 2 GPU globals when the module is created; `71f3d8d30420`
    and `942c51c0b457` (pull request #41) GPU memory map notify ranges; `0218eacfd560` nx2: run the
    guest counter at 31.25 MHz for Switch 2 titles; `1eb97beca0e7` and `a1d47717ae7f` nx2: log
    levels; `04a64dee831e` sm: give Switch 2 programs a stub for services Eden does not have;
    `68d166f43810` nx2: crop Mario Kart World's frame to 1920x1080 in handheld mode; `6380b6687ded`
    nx2: give sparse regions memory only where the GPU writes; `97ce27ab3515` texture_cache:
    re-decode only the written layers of large ASTC arrays; `940d44ca2562` texture_cache: give images
    over unwritten sparse blocks no device memory; `d4ee7fcd5129` vulkan: bind memory to sparse
    terrain arrays layer by layer; `eb99da60729b` video_core: draw deferred instances before the state
    after their loop; `d151e53e6148` nx2: read command lists the GPU wrote from the GPU's copy;
    `f260bcb5a26a` memory: skip clearing guest memory that is still zero.
  - xtam: `a4a746b47b1d` restore Mario Kart World text by accepting R0 shader returns.
  - ZnOw01: `75b707a90ac8` yuzu_cmd: skip kernel teardown when nothing was booted.
  - givethesourceplox: `24e3cb462e47` core: return empty perf stats before the system is running;
    `ea33ad186597` vulkan: write the driver pipeline cache atomically; `ca524c9aa883` vulkan: keep
    the image's view type when a shader's dimension disagrees; `ef65ec709d34` (pull request #35)
    yuzu_cmd: update the window title on the main thread.
  - kraibse: `6e7edd746232` QMD cache: remove completed entries.
  - Wael3rd: `59047f0e6fd1` shader_recompiler: run the destructors of pooled objects.
  - theofficialhackclean: `a8657997e1eb` and `50c9fd3471d5` Mario Kart World fixes (window size).
  - ymatrix1: `91691cf551e8` video_core/dma_pusher: fast-path batching of consecutive
    non-execution methods.
  - pull request #48, `ddd367a815`: core: end device page runs where pages are unmapped or mapped
    again.
- **Third-party libraries** fetched or bundled by the build (see `cpmfile.json` and `externals`),
  each under its own license.

## Changes from eden-nx2

- **Name and policy:**
  - the name Droplet, a new logo and an on-screen watermark;
  - no encrypted content. Droplet loads no keys and has no NCA, NAX, NSP or XCI loaders, no game
    card, and no installing of games, keys or firmware.
- **Switch 2 games:**
  - graphics: mesh and task shaders (Mario Kart World), Ampere shader lowering fixes, small
    GPU-written buffer downloads, MME DMA flushes, sparse images and terrain, ASTC array layers,
    unmapped device memory pages, present pacing, the game's swap interval and a custom refresh rate;
  - system: the software keyboard (UTF-8 or UTF-16 text, as each game reads it), missing service stubs,
    the audio renderer lifetime, the JIT's near calls, and the Linux open file limit.
- **Playing together:**
  - online play on private servers (Reservoir): a server redirect, with Nintendo's hosts blocked;
    socket, SSL and network fixes; per-server friend codes, friends and profiles; room IDs;
  - LAN play: local network requests, emulator rooms in droplet-cli, polling of room sockets.
- **Frontends:**
  - decrypted games open from their folders: File > Open Game Folder (the main way to start a game),
    a button under the game list, drag and drop and the command line, with list entries named by
    their folders;
  - the windowed app without a game and an online settings page;
  - controller automapping, mouse aim, and HD rumble and Switch 2 controller names for SDL
    controllers;
  - the area window filter, unlisted rooms, and frame generation on desktop Linux.
- **Testing aids:** unattended test hooks, RenderDoc capture, shader IR dumps and stutter diagnostics.
  All are off unless their `EDEN_NX2_*` variables are set.
