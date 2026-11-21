# Contributing to ImmortalCore

Thank you for wanting to help with **ImmortalCore**, the server core behind
**World of Immortality**. Contributing is not only about writing code: good bug
reports, testing, documentation and ideas are just as valuable.

ImmortalCore is based on the **TrinityCore 3.3.5 branch as of November 18, 2026**
(see the [README](README.md)). That shapes how we work, so this guide points to the
TrinityCore resources where they still apply and spells out what is specific to us.

* [Before You Start](#before-you-start)
* [Reporting Issues](#reporting-issues)
* [Creating Pull Requests](#creating-pull-requests)
* [Code Standards](#code-standards)
* [Custom Scripts](#custom-scripts)
* [Database Changes](#database-changes)
* [Commit Messages](#commit-messages)
* [Upstream (TrinityCore) Issues](#upstream-trinitycore-issues)
* [License of Contributions](#license-of-contributions)
* [Requirements and Help](#requirements-and-help)

---

## Before You Start

* Make sure you are using the **latest core and database revision** from this
  repository. Many problems are already fixed.
* Work against the **`3.3.5`** branch. This is the only branch we target.
* Search the [existing issues](https://github.com/Lhorath/ImmortalCore/issues) and pull
  requests first so you do not duplicate someone else's work.
* Be respectful. This is a community project run by volunteers, and everyone is here
  because they enjoy the game and the server.

## Reporting Issues

Open an issue on the [issue tracker](https://github.com/Lhorath/ImmortalCore/issues).

### Please include

* **Branch**: `3.3.5`.
* **Commit hash**: the first line printed when `worldserver` or `authserver` starts.
  If it says something like `rev. unknown 1970-01-01`, you downloaded the source as a
  zip file. Please **clone the repository with git** instead so the revision is correct.
* **Operating system** and build type (for example Windows, `RelWithDebInfo`).
* **A clear title and description** of the problem, in English. If your English is
  limited, a machine translation plus the same text in your own language is fine.
* **Current behaviour**: what actually happens.
* **Expected behaviour**: what should happen instead.
* **Steps to reproduce** the problem.
* **Affected entries**: creature, item, quest, spell or NPC IDs, with a
  [Wowhead](https://www.wowhead.com) link where one exists.
* **Logs**: relevant lines from `Server.log`, `DBErrors.log` or the console.

### Crashes

When reporting a crash, attach the crash log (upload it to a service such as
[GitHub Gist](https://gist.github.com/) and link it) and say which build type you used.
Crash dumps from **debug builds** contain far more information than release builds, so
a debug build makes it much more likely the crash can be found and fixed.

### Suggested title and body

Use `Area/Component: Short description` for the title:

```
Script/Teleporter: Guild Hall option does nothing

3.3.5 branch
hash 63f96a282307
Windows 11, RelWithDebInfo

Clicking "Guild Hall" in the vote teleporter closes the menu but does not teleport.

Steps to reproduce:
1. Be in a guild that owns a guild house
2. Talk to the teleporter NPC and open "Guild House Menu"
3. Click "Guild Hall"

Expected: teleported to the guild hall.
Actual: nothing happens, no error in the console.
```

For **security problems** (for example anything that lets players cheat, duplicate
items or access other accounts), please do not post details publicly. Contact the
maintainers privately instead.

## Creating Pull Requests

1. **Fork** the repository.
2. **Create a branch** for your change: `git checkout -b my-fix`
   (the name is up to you).
3. **Commit** your changes: `git commit -am "Scripts/Custom: Fix guild hall teleport"`
4. **Push** the branch: `git push origin my-fix`
5. **Open a Pull Request** against the `3.3.5` branch.

Tips for pull requests:

* Create **one branch per fix or feature**. This lets you keep working on other things
  while a pull request is being reviewed.
* Keep pull requests **small and focused**. A PR that does one thing is much easier to
  review than one that does five.
* **Squash** noisy commits ("fix typo", "oops") into meaningful ones before you open the
  PR. See this guide on [squashing commits](https://ariejan.net/2011/07/05/git-squash-your-latests-commits-into-one/).
* In the description, explain **what changed and why**, link the issue it fixes
  (`Fixes #123`), and say **how you tested it** (does it build, did you test in-game).
* If the change is not finished, say so and list what is left to do.
* Be prepared for feedback. Review comments are about making the code better, not
  about you.

## Code Standards

* Follow the [TrinityCore C++ Development Standards](https://trinitycore.atlassian.net/wiki/spaces/tc/pages/2130103/C+Development+Standards).
  Our code base is TrinityCore's, so its conventions are ours.
* Respect the repository's [`.editorconfig`](.editorconfig): 4 spaces for indentation
  (**no tabs**), UTF-8 text files, a final newline, and no trailing whitespace.
  C++ source files are configured as **Latin-1**, so avoid non-ASCII characters
  (curly quotes, accented letters, symbols) in `.cpp` and `.h` files. Use plain ASCII in
  comments and strings.
* Make sure your change **builds without new warnings**, and test it in-game before
  asking for a review.
* Do not reformat code you are not otherwise changing. It makes diffs hard to read.
* Do not commit generated files, build output, local configuration (`.conf`), logs
  or game client data.

## Custom Scripts

World of Immortality specific scripts live in
[`src/server/scripts/Custom`](src/server/scripts/Custom).

To add a new script:

1. Add your `.cpp` file to that folder.
2. Declare and call its registration function in `custom_script_loader.cpp`.
3. Define that function in your script with **exactly the same name**.

The name in `custom_script_loader.cpp` and the `AddSC_...` function in your script must
match exactly, **including upper and lower case**. The script compiles either way, but a
mismatch fails later with a linker error such as
`unresolved external symbol AddSC_...`. The file name itself does not have to match
anything.

If your script relies on a `ScriptName` set in the database, make sure the name you
register in the script is the one used in `creature_template.ScriptName`.

## Database Changes

* SQL changes that ship with a C++ change are named
  **`YYYY_MM_DD_NN_<database>.sql`**, where `YYYY_MM_DD` is the date, `NN` is the
  number of the SQL file you created that day for that database (`00`, `01`, ...),
  and `<database>` is `auth`, `characters` or `world`. For example:
  `2026_11_20_00_world.sql`.
* Place the file in `sql/updates/<database>/3.3.5/`.
* Because it is unlikely your pull request is merged on the day you open it, you can
  use an impossible date (for example `2026_13_32_00_world.sql`) to avoid conflicts.
  A maintainer will give it its real name when merging.
* When you change the **`auth`** or **`characters`** database structure, also update the
  base files in `sql/base/`.
* Keep update scripts **re-runnable where possible** (`DELETE` before `INSERT`,
  `IF NOT EXISTS`, and so on) and test them on a copy of your database first.
* `sql/custom/` is **ignored by git** (`*.sql`). Put SQL there only for private,
  local-only changes that should never be committed. Anything other contributors
  need must go in `sql/updates` instead.
* Never commit real player data, account data or passwords.

## Commit Messages

Short, descriptive commit messages make the history useful. We suggest the
`Area/Component: Short description` style used by TrinityCore:

```
Scripts/Custom: Fix guild hall teleport
Core/Player: Fix crash when resetting talents with no pet
DB/Quest: Add missing reward for "The Collapse"
Docs: Update contributing guide
```

Write the summary line in the imperative mood ("Fix", "Add", "Remove"), keep it
reasonably short, and use the body of the commit to explain *why* if it is not obvious.

## Upstream (TrinityCore) Issues

Most of this code is TrinityCore. If a bug is not specific to World of Immortality, for
example a spell, a core crash or a quest that is broken on a stock TrinityCore 3.3.5
server, it is usually best fixed **upstream**, where more people can benefit and review
it.

* Check whether the problem exists in the
  [TrinityCore 3.3.5 branch](https://github.com/TrinityCore/TrinityCore/tree/3.3.5) and
  read their
  [issue tracker guide](https://community.trinitycore.org/topic/37-the-trinitycore-issuetracker-and-you/)
  before reporting it there.
* You are still welcome to open an issue or pull request here. Just mention that it
  also affects upstream, if you know it does.

## License of Contributions

ImmortalCore is licensed under the **GNU General Public License, version 2** (see
[COPYING](COPYING)). By submitting a contribution you confirm that you have the right to
submit it and that it may be distributed under the same license.

Do not submit code you do not have the right to share, and keep existing copyright and
license notices in the files you edit.

## Requirements and Help

* Software requirements and build instructions are the same as for TrinityCore 3.3.5.
  See the [README](README.md#requirements) and the
  [TrinityCore wiki](https://trinitycore.info/en/home).
* If you are stuck, ask in the issue or pull request. Questions are welcome.

<!-- Add World of Immortality links here, e.g. Discord or forum, for contributor questions -->
