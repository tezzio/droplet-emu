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
- **[Eglantine](https://github.com/Djihads80/Eglantine)**, an experimental Switch 2 emulator built on
  eden-nx2 (GPL-3.0). Droplet merges its `nx2` branch up to commit `68a945d0a8` (2026-11-03), except
  Eglantine's CI, documentation, translation and branding changes, pull request #313 and a few commits
  left out on purpose. That is 376 of its commits, by Takoroka (eden-nx2's author) 188, kraibse 38,
  Duck912/duck912 27, Sssnowyyy 22, snowstormxd 21, rdenno 8, néo Luigi 7, givethesourceplox 7,
  a55uka 5, Djihads80 5, Jonas 4, Yuri Olive 4, Meow 4, Wael3rd 3, shiv121231 3, yumekiru/yumerkiru 3,
  Paulo Rossatto 2, theofficialhackclean 2, and one each by Adya, Alex, arisqz, Daniel Cabral, Dennys
  Kaluzny Pereira, Djibril, Franz, gabrielskaminski, Javier, Kaydax, lesmmwe, Mako, Omar Mahboubi,
  PavelBARABANOV, QuantumDriftDev, sdas-19, spaina93, vibecodekun, Whovian64, xtam, ymatrix1, yvonne
  and ZnOw01. [CREDITS-Eglantine.md](CREDITS-Eglantine.md) lists each commit with its author.
- **[AMD FidelityFX SDK](https://github.com/GPUOpen-LibrariesAndSDKs/FidelityFX-SDK)** v1.1.4 by
  Advanced Micro Devices (MIT), through Eglantine: the FSR 3.1 upscaler's shader code, unmodified, in
  `externals/FidelityFX-SDK`.
- **Third-party libraries** fetched or bundled by the build (see `cpmfile.json` and `externals`),
  each under its own license.

## Changes from eden-nx2

- **Name and policy:**
  - the name Droplet and a new logo;
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
    drag and drop and the command line, with list entries named by their folders;
  - the windowed app without a game and an online settings page;
  - controller automapping, mouse aim, and HD rumble and Switch 2 controller names for SDL
    controllers;
  - the area window filter, unlisted rooms, and frame generation on desktop Linux.
- **From Eglantine** (see [CREDITS-Eglantine.md](CREDITS-Eglantine.md)), among others: FSR 3.1
  upscaling where a game asks for DLSS, NUP game archives (Convert to NUP in a game's right-click
  menu, or `droplet-cli --pack-nup`), faster game discovery, and vibration in Switch 2 games.
- **Testing aids:** unattended test hooks, RenderDoc capture, shader IR dumps and stutter diagnostics.
  All are off unless their `EDEN_NX2_*` variables are set.
