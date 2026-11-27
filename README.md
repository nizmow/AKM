# AKM

An independent fork of MagiC for Atari 68000 computers: a multitasking operating
system, MAGXDESK desktop, applications and drivers, rebuilt with GCC and VASM.
This is not the official MagiC release.

The current distribution targets German and English Atari/ST installations. The
[build catalogue](docs/porting/APPLICATION_BUILDS.md) distinguishes rebuilt
components, retained binaries and unsupported variants.

## Build

Use a Linux host with [Podman](https://podman.io/) and
[mise](https://mise.jdx.dev/) installed. From the repository root:

```sh
mise trust
mise install
mise run toolchain:build
mise run release:core
```

This builds the kernel, selected applications and drivers, then writes
`build/atari/release/core-de.zip` and `core-de.manifest`. The manifest records
SHA-256 hashes and which files were rebuilt or retained.

Other useful targets:

```sh
mise run release:english       # English core, UK keyboard, mono Mega STE
mise run release:minimal       # Boot-only German mono MegaSTE installation
mise run kernel -- -j4         # Kernel only: build/atari/MAGIC.RAM
mise run apps -- mgedit        # One application bundle: build/atari/apps/mgedit/
mise tasks                    # Available build and test commands
```

The minimal package is `build/atari/release/minimal-de.zip`, with its own
manifest. It includes MAGXDESK and the MCMD fallback, but not COPS or optional
extensions.

The English core is `build/atari/release/core-en.zip` with `core-en.manifest`.
Its kernel is built separately under `build/atari/en/`; existing English
application resources are selected without changing the German build.
Original helper diagnostics and legacy help are not all translated.

Make owns the build; mise is a convenience layer. Without mise:

```sh
scripts/atari-container build
scripts/atari-container run make -j4 magic
scripts/atari-container run make -f Makefile.apps mgedit
python3 scripts/make-release.py --output build/atari/release/core-de.zip
```

Toolchain versions and dependencies are pinned in
[`Containerfile.atari`](Containerfile.atari) and [`mise.toml`](mise.toml).

## Install

Use a spare Atari installation or a disposable emulator disk image. Back up
the boot volume first: copying the package replaces system files.

1. Prepare a writable, bootable Atari hard-disk volume accessible as `C:`.
   The ZIP is not a disk image and does not include partitioning tools, a
   hard-disk boot driver or a TOS ROM.
2. Extract the selected ZIP and copy its **contents** to the root of `C:`,
   preserving the directory layout and existing disk driver. Do this from TOS
   or another host, not over a running MagiC installation. Packages already
   select their resources; do not run the old upstream localization scripts.
3. Boot TOS and run `C:\AUTO\MAGXBOOT.TOS` to start MagiC.
   Once startup works, rename it to `MAGXBOOT.PRG` for AUTO-folder startup.

The selected profiles have NeoST install/boot checks. That is not proof of
all hardware, video modes or long-term stability. The English profile selects
UK keyboard and ST high monochrome; other boot-drive letters and English
video/keyboard profiles are outside these instructions. Read the package's
`INSTALL.TXT` before copying files to real hardware.

To verify installation on a cloned MagiC image:

```sh
export MAGIC_IMAGE=/path/to/bootable-magic.img
export TOS206_ROM=/path/to/tos206.img
mise run test:release-neost
mise run test:release-minimal-neost
mise run test:release-english-neost
```

Install host `mtools` first. The runners require an existing bootable image,
clone it, install the ZIP and check guest effects. See the
[disk-fixture requirements](docs/porting/TESTING.md#neost-magic-disk-fixture)
for partition offsets and the [testing guide](docs/porting/TESTING.md) for
component checks and coverage limits.

## CI and releases

[GitHub Actions](.github/workflows/ci.yml) builds the toolchain and a pinned,
headless NeoST container, runs the public EmuTOS regressions, and uploads the
German core/minimal and English core ZIPs with manifests and build provenance.
To use the same emulator environment locally, after building the toolchain:

```sh
mise run neost:build
mise run neost:run -- python3 scripts/neost-console-tools-smoke.py
```

Version tags matching `v*` run those checks before creating a **draft**
[GitHub release](.github/workflows/release.yml), including checksums and a
matching source archive. Full MagiC/TOS cold boots remain a separate private
fixture check before publication; ROMs and working disk images are not
included. See the [automation contract](docs/porting/TESTING.md#github-actions-and-release-publication).


For development, start with the [porting guide](docs/porting/README.md).
Project direction is in the [roadmap](docs/ROADMAP.md); remaining work is in
[GitHub issues](https://github.com/nizmow/AKM/issues).

## Appendix: history

AKM derives from [Andreas Kromke's sources](https://gitlab.com/AndreasK/Atari-Mac-MagiC-Sources).
The upstream collection includes TOS, KAOS, MagiC, MagiCMac, MagicMac X and
AtariX material. Its completed source upload, including MVDI, dates to
2018-11-20; not every historical component is an active AKM build.

### Historical source tree

- [`original-import`](https://github.com/nizmow/AKM/tree/original-import)
  preserves the upstream snapshot before AKM development.
- [`pre-magic-retirement`](https://github.com/nizmow/AKM/tree/pre-magic-retirement)
  (`88eb5ee`) preserves the former `MagiC/` tree, old release ZIPs and retired
  compiler artifacts. Active sources now live in `kernel/`, `core/` and `base/`;
  retained distribution inputs live in `release/atari/`.

Browse archived files without changing the checkout:

```sh
git fetch origin tag pre-magic-retirement
git ls-tree -r pre-magic-retirement -- MagiC/APPS
git show pre-magic-retirement:MagiC/KERNEL/BUILD/ATARI/M.INP
```

## License

The files are licensed under GPLv3; see [LICENSE](LICENSE).
MVDI and its sources were provided courtesy of Sven and Wilfried Behne and
distributed under the main license. Third-party distribution inputs retain
their applicable notices; TOS ROMs must be obtained separately.
