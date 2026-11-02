# Droplet

Droplet is an experimental Nintendo Switch 2 emulator for Linux and Windows. It is a modified version
of [eden-nx2](https://git.eden-emu.dev/Takoroka/eden-nx2), which brings Switch 2 support to the
[Eden](https://git.eden-emu.dev/eden-emu/eden) emulator. Work so far has focused on Splatoon Raiders
and Mario Kart World, including their online and LAN modes. See [CREDITS.md](CREDITS.md) for everyone
whose work Droplet builds on.

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

Droplet runs decrypted games from their folders. Choose **File > Open Game Folder** (Ctrl+O), or
the **Open Game Folder** button under the game list, and pick the game's folder: the one with its
`exefs` and `romfs` folders inside. Its `exefs` folder works too, and so does dropping the folder
onto Droplet's window or giving it to `eden-cli` as the game's path. To keep your games in the
list, add the folder that holds them (double-click the empty list, or use **Add New Game
Directory** in the list); Droplet finds every game folder inside it.

## Playing together

- **Online play, emulators only.** Games that use Nintendo's NPLN online platform can play on a
  private [Reservoir](https://gitlab.com/seighttt/droplet/-/tree/server) server: sign-in, friend
  codes (`SW-xxxx-xxxx-xxxx`, which you can change), friends, matchmaking, rooms and room IDs. In
  Configure > System > Network, turn online play on, enter the server's address and pick its
  certificate (`ca.der`, from the server's status page). Nothing ever reaches Nintendo's servers.
- **LAN play.** Mario Kart World has a hidden LAN mode: on the main menu, highlight Wireless Play, hold
  L and R and click the left stick, and it becomes LAN Play. Players on the same network find each
  other's rooms. Emulators far apart can share one through an emulator room (Multiplayer > Create Room).
  Playing against a console in LAN mode should work the same way, but has not been tested yet.

## Known issues

- **Splatoon Raiders:** now and then the top of the sky turns black when the game loads into the
  beach, and stays black for the rest of the session. It doesn't happen every time, and the cause
  isn't known yet.

## Building

See [docs/Build.md](docs/Build.md). The windowed app is built with `-DENABLE_QT=ON` (`bin/eden`); the
command-line app is `bin/eden-cli`. This tree has no Android frontend yet.

## License

Droplet is free software under the GNU General Public License, version 3 or later
([LICENSE.txt](LICENSE.txt)). Parts of it come from projects under other free licenses; their texts
are in [LICENSES](LICENSES) and their notices stay in the files they cover.
