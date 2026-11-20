<p align="center">
  <img src="https://i.postimg.cc/tJH96TKb/toplogo.png" alt="World of Immortality">
</p>

<h1 align="center">ImmortalCore</h1>

<p align="center">
  <strong>The server core behind <em>World of Immortality</em> &mdash; a custom World of Warcraft: Wrath of the Lich King (3.3.5a) private server.</strong>
</p>

<p align="center">
  <a href="COPYING"><img src="https://img.shields.io/badge/license-GPL%20v2-blue.svg" alt="License: GPL v2"></a>
  <img src="https://img.shields.io/badge/client-3.3.5a-orange.svg" alt="Client 3.3.5a">
  <img src="https://img.shields.io/badge/based%20on-TrinityCore%203.3.5-green.svg" alt="Based on TrinityCore 3.3.5">
</p>

---

* [About](#about)
* [Origin and Base Code](#origin-and-base-code)
* [Custom Content](#custom-content)
* [Requirements](#requirements)
* [Building and Installing](#building-and-installing)
* [Reporting Issues](#reporting-issues)
* [Credits](#credits)
* [Copyright and License](#copyright-and-license)
* [Disclaimer](#disclaimer)
* [Links](#links)

---

## About

**ImmortalCore** is the emulation core that powers **World of Immortality**, a custom
private server that has been running in one form or another since 2009. It is an
open source *MMORPG* server framework written mostly in C++, targeting the
World of Warcraft 3.3.5a client.

This repository contains the server code, the database structure and the custom
content that make World of Immortality what it is.

## Origin and Base Code

ImmortalCore is built on the shoulders of two projects, and we are grateful to both:

* **[TrinityCore](https://www.trinitycore.org)** &mdash; this code is based on the
  TrinityCore **3.3.5 branch as of November 18, 2026**.
* **[MaNGOS](https://www.getmangos.eu)** (the *Massive Network Game Object Server*) &mdash;
  the project TrinityCore itself was derived from, whose foundations still run through
  this codebase.

Everything that is not custom to World of Immortality comes from the work of the
TrinityCore and MaNGOS communities. If you are looking for the stock, general-purpose
core, please visit the [TrinityCore repository](https://github.com/TrinityCore/TrinityCore).

## Custom Content

World of Immortality specific code lives alongside the TrinityCore sources. Custom
scripts are kept in [`src/server/scripts/Custom`](src/server/scripts/Custom) and are
registered through `custom_script_loader.cpp`.

Currently documented custom content:

| Feature | Description |
| :--- | :--- |
| **Vote Teleporter NPC** (`tele_npc_vote`) | Teleporter and utility NPC. Towns are open to everyone; dungeons, raids, player tools, PvP and the guild house menu are unlocked by voting. Includes talent and pet talent reset, resurrection sickness removal, cooldown reset, a heal option and instance lock reset (paid in vote points). |
| **Voting system** | Vote status and vote points are read from the `voting_points` table in the **auth** database. |
| **Guild house support** | Guild ownership of a guild house is read from the `guild_house_masks` table in the **characters** database. |

> More custom content will be documented here as it is ported and added.

## Requirements

ImmortalCore has the same software requirements as TrinityCore 3.3.5 (a C++ compiler
with C++20 support, CMake, Boost, OpenSSL and MySQL). The TrinityCore wiki lists the
exact versions for Windows, Linux and macOS:

* [TrinityCore requirements](https://trinitycore.info/en/install/requirements)

You will also need a **3.3.5a (build 12340)** World of Warcraft client.

## Building and Installing

The build and installation process is the same as for TrinityCore, so the TrinityCore
guides apply here as well:

* [TrinityCore installation guides](https://trinitycore.info/en/home)

In short:

1. Clone this repository.
2. Configure and build with CMake.
3. Create the `auth`, `characters` and `world` databases and apply the SQL from the [`sql`](sql) directory.
4. Copy `authserver.conf.dist` and `worldserver.conf.dist` to `.conf` files and adjust them.
5. Extract the client data (maps, vmaps, mmaps, DBC) and place it in your server's data directory.

The custom World of Immortality tables (voting and guild houses) need to exist in the
databases listed in [Custom Content](#custom-content) before the related features will work.

## Reporting Issues

* Problems with **World of Immortality specific** code or content: please open an issue
  on this repository's [issue tracker](https://github.com/Lhorath/ImmortalCore/issues).
* Problems that also exist in stock TrinityCore should be reported to
  [TrinityCore](https://github.com/TrinityCore/TrinityCore/issues), after reading their
  [issue tracker guide](https://community.trinitycore.org/topic/37-the-trinitycore-issuetracker-and-you/).

Please check existing issues first to avoid duplicates, and include as much detail as
you can (what you did, what you expected, what happened, relevant log output).

## Credits

### World of Immortality

World of Immortality would not exist without its original developers:

* **Sero**
* **Hellfrost**
* **Abub**
* **Brucey**

Thank you for building the server and its community from the very beginning.

### TrinityCore and MaNGOS

* The **[TrinityCore](https://github.com/TrinityCore/TrinityCore)** developers and
  contributors, whose 3.3.5 branch this project is based on. See the [AUTHORS](AUTHORS) file.
* The **[MaNGOS](https://www.getmangos.eu)** developers and contributors, whose work
  TrinityCore and, through it, this project are derived from.

### Everyone else

Thanks to the wider emulation community and to every player and staff member who has
been part of World of Immortality over the years.

## Copyright and License

Copyright &copy; 2009&ndash;2026 **World of Immortality**.

Portions of this software are copyright &copy; the **TrinityCore** and **MaNGOS**
projects and their respective contributors.

ImmortalCore is derived from TrinityCore and is therefore licensed under the
**GNU General Public License, version 2** (GPL v2). Read the [COPYING](COPYING) file for
the full license text and [AUTHORS](AUTHORS) for the list of upstream contributors.

## Disclaimer

World of Immortality and ImmortalCore are fan-made, non-commercial projects. They are
not affiliated with, endorsed by or sponsored by Blizzard Entertainment, Inc.
*World of Warcraft* and *Wrath of the Lich King* are trademarks or registered
trademarks of Blizzard Entertainment, Inc. You must own a legitimate copy of the game
client to use this software.

## Links

* [TrinityCore website](https://www.trinitycore.org)
* [TrinityCore wiki](https://www.trinitycore.info)
* [TrinityCore repository](https://github.com/TrinityCore/TrinityCore)
* [MaNGOS](https://www.getmangos.eu)

<!-- Add World of Immortality links here, e.g. website, Discord, forums -->
