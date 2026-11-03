# Droplet

Droplet is an experimental Nintendo Switch 2 emulator for Linux and Windows. It is a modified version
of [eden-nx2](https://git.eden-emu.dev/Takoroka/eden-nx2), which brings Switch 2 support to the
[Eden](https://git.eden-emu.dev/eden-emu/eden) emulator, and it includes the work of
[Eglantine](https://github.com/Djihads80/Eglantine), another emulator built on eden-nx2. Work so far has
focused on Splatoon Raiders and Mario Kart World, including their online and LAN modes. See
[CREDITS.md](CREDITS.md) for everyone whose work Droplet builds on.

## Legal notice

Droplet is an independent project. It is not affiliated with, sponsored by or endorsed by Nintendo.
Nintendo Switch, Nintendo Switch 2, Splatoon and Mario Kart are trademarks of Nintendo.

### No circumvention of copy protection

Nintendo encrypts its games and system software to control access to them. Section 1201 of the
United States Digital Millennium Copyright Act (17 U.S.C. § 1201), and similar laws in other
countries, prohibit circumventing such technological protection measures and distributing tools made
to do so. Droplet is deliberately designed and maintained so that it contains no such tool:

- **No keys.** Droplet cannot load or obtain cryptographic keys of any kind. It does not read key
  files (such as `prod.keys` or `title.keys`), derive keys from console data or accept keys from its
  user, and its key store is always empty. Without keys, it cannot decrypt anything.
- **No encrypted formats.** The parts of eden-nx2, inherited from Eden and yuzu, that open encrypted
  content were removed: the loaders for NCA, NAX, NSP and XCI files, the derivation of keys from
  console data, and the installing of games, updates, keys and firmware. Droplet cannot run a game
  from an encrypted file or from a game card.
- **Unencrypted files only.** Droplet runs only program files that are not encrypted: homebrew
  (NRO), NSO and KIP files, and a game's program and data in decrypted form (an extracted ExeFS and
  RomFS folder). On a console, a game never decrypts its own files: the system's file system service
  does that and hands the game plain data. Droplet stands in for that service, but it starts from
  files that are already plain, so it never has anything to decrypt.

Droplet's source code is public, so anyone can check these points.

### No Nintendo material

Droplet contains no Nintendo code, keys, firmware, games or other copyrighted Nintendo content. The
project does not distribute any of these, link to them or explain how to obtain them, and it does
not condone piracy. Droplet's online play connects only to private servers, never to Nintendo's
services.

### Your responsibility

Use Droplet only with software you own and have the right to use, and obtain your copies lawfully.
Laws on emulation, copying and copy protection differ between countries; you are responsible for
following those that apply to you.

This notice describes how Droplet is designed. It is not legal advice. Rights holders with questions
or concerns about this project can open a confidential issue on its
[GitLab project](https://gitlab.com/seighttt/droplet/-/issues), which only the project's members can
see.

## About this project

Droplet and its online server, [Reservoir](https://gitlab.com/seighttt/droplet/-/tree/server),
began as a proof of concept. The idea came from how easy it is to set up a Minecraft server on an
Oracle Cloud instance in a few minutes: hosting private online play for a group of friends should
be that simple too.

- **No official servers.** The project's developer does not host any online servers for players.
  Every Reservoir server is run independently by whoever set it up; the project does not run,
  endorse or take responsibility for any of them.
- **No official Discord.** Droplet has no official Discord server or other community chat. Any
  server or group that claims to be official is not run by the project. The only official place is
  this GitLab project.

## Running games

Droplet runs decrypted games from their folders. Choose **File > Open Game Folder** (Ctrl+O) and
pick the game's folder: the one with its `exefs` and `romfs` folders inside. Its `exefs` folder works
too, and so does dropping the folder onto Droplet's window or giving it to `droplet-cli` as the
game's path. To keep your games in the list, add the folder that holds them (double-click the empty
list, or use **Add New Game Directory** in the list); Droplet finds every game folder inside it.

## Playing together

- **Online play, emulators only.** Games that use Nintendo's NPLN online platform can play on a
  private [Reservoir](https://gitlab.com/seighttt/droplet/-/tree/server) server: sign-in, friend
  codes (`SW-xxxx-xxxx-xxxx`, which you can change), friends, matchmaking, rooms and room IDs. In
  Multiplayer > Online Play Settings, turn online play on, enter the server's address and pick its
  certificate (`ca.der`, from the server's status page). Nothing ever reaches Nintendo's servers.
- **LAN play.** Mario Kart World has a hidden LAN mode: on the main menu, highlight Wireless Play, hold
  L and R and click the left stick, and it becomes LAN Play. Players on the same network find each
  other's rooms. Emulators far apart can share one through an emulator room: one player hosts it with
  Multiplayer > Create Room, and the others join with Multiplayer > Direct Connect to Room.
  Playing against a console in LAN mode should work the same way, but has not been tested yet.

## Known issues

- **Splatoon Raiders:** now and then the top of the sky turns black when the game loads into the
  beach, and stays black for the rest of the session. It doesn't happen every time, and the cause
  isn't known yet.
- **NUP archives:** games packed with Convert to NUP run slower than from their folders (Splatoon
  Raiders: 25-30 fps instead of 60), so keep playing from folders for now.

## Building

Droplet builds like Eden, and [docs/Build.md](docs/Build.md) is the full guide. Below are the short
versions for Linux and Windows. [docs/Deps.md](docs/Deps.md) lists the packages for other systems,
including Gentoo, Alpine, Void, NixOS, macOS and the BSDs.

Whatever the system, the build needs three options:
- `-DENABLE_WERROR=OFF` keeps compiler warnings from stopping the build. Droplet's own changes still
  trigger some warnings that Eden treats as errors.
- `-DENABLE_WEB_SERVICE=OFF` leaves out Eden's web services (telemetry and the public room list),
  which Droplet doesn't use.
- `-DYUZU_TESTS=OFF` skips the unit tests.

The first configure downloads a few libraries that Droplet builds in. They stay in `.cache/cpm`.

### Linux

1. Install the tools and libraries for your distribution:

   <details>
   <summary>Arch Linux and distributions based on it</summary>

   ```sh
   sudo pacman -Syu --needed base-devel boost catch2 cmake enet ffmpeg fmt git glslang libzip lz4 ninja nlohmann-json openssl opus qt6-base qt6-multimedia qt6-charts sdl3 zlib zstd zip unzip vulkan-headers vulkan-utility-libraries libusb spirv-tools spirv-headers
   ```

   </details>

   <details>
   <summary>Ubuntu, Debian and Mint (Ubuntu 26.04, Mint 22.3, Debian 13 or later)</summary>

   ```sh
   sudo apt-get install autoconf cmake g++ gcc git glslang-tools libglu1-mesa-dev libhidapi-dev libpulse-dev libtool libudev-dev libxcb-icccm4 libxcb-image0 libxcb-keysyms1 libxcb-render-util0 libxcb-xinerama0 libxcb-xkb1 libxext-dev libxkbcommon-x11-0 mesa-common-dev nasm ninja-build qt6-base-private-dev catch2 libfmt-dev liblz4-dev nlohmann-json3-dev libzstd-dev libssl-dev libavfilter-dev libavcodec-dev libswscale-dev pkg-config zlib1g-dev libva-dev libvdpau-dev qt6-tools-dev qt6-charts-dev libvulkan-dev spirv-tools spirv-headers libusb-1.0-0-dev libxbyak-dev libboost-dev libboost-fiber-dev libboost-context-dev libsdl3-dev libopus-dev libasound2t64 vulkan-utility-libraries-dev
   ```

   </details>

   <details>
   <summary>Fedora (needs <a href="https://rpmfusion.org/Configuration">RPM Fusion</a> for ffmpeg-devel)</summary>

   ```sh
   sudo dnf install autoconf cmake fmt-devel gcc{,-c++} glslang hidapi-devel json-devel libtool libusb1-devel libzstd-devel lz4-devel nasm ninja-build openssl-devel pulseaudio-libs-devel qt6-linguist qt6-qtbase{-private,}-devel qt6-qtmultimedia-devel qt6-qtcharts-devel speexdsp-devel wayland-devel zlib-devel ffmpeg-devel libXext-devel boost jq git vulkan-utility-libraries-devel SDL3-devel
   ```

   </details>

2. Get the source and build it:

   ```sh
   git clone -b main https://gitlab.com/seighttt/droplet.git
   cd droplet
   cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release -DENABLE_WERROR=OFF -DENABLE_WEB_SERVICE=OFF -DYUZU_TESTS=OFF
   cmake --build build
   ```

3. Start `build/bin/droplet` (the windowed app) or `build/bin/droplet-cli` (the command-line app), and
   open a game as described in [Running games](#running-games).

### Windows

1. Install [MSYS2](https://www.msys2.org/) and open its **MSYS2 MinGW 64-bit** shell (`mingw64.exe`).
2. Install the tools and libraries:

   ```sh
   BASE="git make autoconf libtool automake-wrapper jq patch"
   MINGW="qt6-base qt6-charts qt6-tools qt6-translations qt6-svg cmake toolchain clang python-pip openssl vulkan-memory-allocator vulkan-devel glslang boost fmt lz4 nlohmann-json zlib zstd enet opus libusb openssl SDL3"
   packages="$BASE"
   for pkg in $MINGW; do packages="$packages mingw-w64-x86_64-$pkg"; done
   pacman -Syuu --needed --noconfirm $packages
   ```

3. Get the source and build it, in the same shell:

   ```sh
   git clone -b main https://gitlab.com/seighttt/droplet.git
   cd droplet
   cmake -S . -B build -G "MSYS Makefiles" -DCMAKE_BUILD_TYPE=Release -DENABLE_WERROR=OFF -DENABLE_WEB_SERVICE=OFF -DYUZU_TESTS=OFF
   cmake --build build --parallel
   ```

4. Start `build/bin/droplet.exe` or `build/bin/droplet-cli.exe` from that shell, which has the libraries
   they need on its path.

This tree has no Android frontend yet.

## License

Droplet is free software under the GNU General Public License, version 3 or later
([LICENSE.txt](LICENSE.txt)). Parts of it come from projects under other free licenses; their texts
are in [LICENSES](LICENSES) and their notices stay in the files they cover.
