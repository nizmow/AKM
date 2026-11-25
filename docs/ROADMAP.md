# Roadmap

## Direction

Make the obtainable source reproducibly buildable, recreate the distribution,
modernize localization, then develop fixes and improvements on a dependable
baseline. Keep compiler/build changes separate from unrelated behavior changes
so regressions remain attributable.

The aim is **every obtainable component accounted for**, not a promise to
reconstruct missing source or silently replace unavailable dependencies.

## Starting point

- The German Atari/68000 kernel builds with GCC 15.2 and VASM and packages as
  `MAGIC.RAM`.
- Both the original release and the rebuilt kernel reached MAGXDESK in Hatari
  using isolated copies of the same installation.
- User smoke testing reports successful file operations, nested copies and
  four-process multitasking/switching. This is useful evidence, not exhaustive
  correctness, stability or performance coverage.
- Most applications used in those tests are still the existing release binaries;
  rebuilding the kernel did not rebuild the distribution.

- MOD_APP is now an independently built GCC/MiNTLib application, with 17 Hatari
  comparison cases matching the original executable. The [porting work list](porting/TODO.md)
  tracks the concrete phase-1 work informed by that port.

Supporting documents:

- [Build catalogue](porting/APPLICATION_BUILDS.md): component inventory, outputs,
  variants, dependencies and proposed porting order.
- [Porting notes](porting/PORTING_NOTES.md): practical workflow, ABI/encoding issues,
  verification and emulator baseline.
- [Root README](../README.md): current toolchain and build commands.

## Phase 1 — Build the source-backed components

Give each application, utility, driver, shared library and relevant host tool
an independently invokable build target. Include the reusable prerequisites
needed to make those targets real.

- Use GCC for C and retain VASM for Motorola assembly; this is not a GNU
  assembler conversion project.
- Select the appropriate compiler target and ABI per component. Do not apply
  kernel register-ABI flags indiscriminately to applications or host tools.
- Record actual sources, startup code, compatible libraries, resources, language
  selection and platform variants. Remove hard-coded developer paths from the
  modern build recipes.
- Keep generated files and outputs separate from original source and archived
  releases. Isolate variants which produce the same filename.
- Resolve shared dependencies explicitly, with one integration owner. Prefer
  small application-sized tasks for humans and cheaper agents.
- Keep platform-specific and historical targets identifiable rather than forcing
  them into the default Atari build or silently dropping them.
- Track **source/dependencies accounted for → compiled → linked → packaged →
  launched → functionally tested**. A compiler exit status is not runtime proof.
- Document missing-source and external-dependency blockers. Continue independent
  work rather than stalling the whole effort or manufacturing stubs.
- Fix port-induced bugs immediately; defer unrelated cleanup and behavior changes.

### Build tooling

Keep **Make** as the dependency/build engine and **Podman** as the reproducible
cross-toolchain environment. There is no current need to change build systems
while also changing compilers.

Optionally add **mise** as a convenient front door for build, test and packaging
commands. Its tasks should invoke the authoritative recipes, not maintain a
second dependency graph or duplicate compiler/linker flags. Mise should not be
required to understand or invoke the underlying build.

### Completion criteria

Every inventoried component is either independently buildable with documented
inputs and verification status, or explicitly classified as blocked,
binary-only, historical or platform-specific with the reason recorded. The
source-backed core installation has repeatable builds and runtime smoke checks.

## Phase 2 — Reproduce a distribution ZIP

Build an installation archive corresponding to the existing release layout,
not merely a ZIP of whatever happens to be in a build directory.

- Define an explicit installation manifest: selected executables, drivers, shared
  libraries, resources, help/data files, configuration and directory layout.
- Select versions, platforms and languages deliberately. Do not accidentally
  install historical variants or overwrite distinct outputs with the same name.
- Identify rebuilt files versus retained binary-only components and record their
  provenance. Check redistribution rights before carrying imported artifacts
  forward.
- Make packaging repeatable, including stable ordering and controlled timestamps
  where appropriate. Record source revision, toolchain and build options.
- Preserve the original release as a comparison baseline; do not overwrite it.
- Install and boot **from the produced ZIP** on a fresh disposable image, rather
  than testing only the development installation. Exercise the desktop helpers,
  file operations, multitasking and shutdown.
- Supply the corresponding source and build material required for distributed
  GPL-covered binaries, alongside the applicable notices.

### Completion criteria

A documented command produces the selected distribution, with a file manifest
and clear rebuilt/retained status. A clean installation from that archive boots
and passes the recorded smoke tests.

## Phase 3 — Modernize localization and repair translations

Treat localization as a reproducible source-to-resource workflow, not a series
of edits to unrelated binary resources.

1. Inventory languages, missing translations, duplicated strings, existing
   translation sources and resource-generation tools.
2. Establish extraction, editing, validation and generation tooling before mass
   translation work. Choose formats based on the actual resource constraints;
   do not force a new format without a reliable round trip.
3. Keep translation sources maintainable and generate the required Atari target
   encoding and resources deterministically.
4. Validate placeholders, keyboard shortcuts, string lengths, dialog geometry,
   resource identifiers and fallback behavior. Correct text alone is insufficient.
5. Repair missing/inconsistent translations and verify the actual UI for each
   supported language, including relevant display modes.

### Completion criteria

Missing translations are visible and actionable; supported language variants
can be regenerated and checked without undocumented manual binary editing.
The distributed resources have been exercised in the corresponding applications.

## Phase 4 — Fix bugs and evolve the system

With repeatable builds, a reproducible installation and usable localization
infrastructure, pursue unrelated bug fixes and deliberate modifications.

- Start from reproducible reports and add focused regression coverage where it
  catches a plausible user-visible failure.
- Keep fixes, refactors and feature changes separable. Preserve a known-working
  installation for comparisons.
- Expand hardware, video-mode and memory-configuration coverage as relevant.
- Investigate size/performance only through controlled comparisons. A separate
  `-Os` experiment remains useful, but is not a prerequisite for broader porting.
- Review ABI, loader, filesystem and shared-library changes more strongly than
  routine application build work.

### Completion criteria

Each change has an explicit behavioral goal, appropriate verification and a
known distribution/build baseline. Improvements do not depend on undocumented
local tooling or unreviewable batches of unrelated changes.

## Longer term — Possible independent fork

An independent fork is a possible direction, not a decision to rename this
repository or its artifacts now.

- **AKM** is the proposed repository name.
- **A Kinda MagiC** is the tentative unofficial project name.
- Present it clearly as an independent, unofficial project; preserve upstream
  attribution and provenance rather than implying an official continuation or
  endorsement.
- Naming is provisional: trademark/name availability has not been assessed.
  An unofficial label alone is not a determination about naming rights.
- Retain useful upstream history and make the relationship between inherited
  sources, porting changes and later modifications clear.

### Licensing and distribution

The repository README explicitly states **GPLv3**, and `LICENSE` contains the
GNU General Public License version 3. The README also states that the contributed
MVDI sources are distributed under that main license. Do not infer an
“or later” grant solely from the presence of the GPLv3 license text; preserve
and inspect the applicable notices.

For a fork, retain the applicable GPL licensing, copyright/license notices,
attributions and modification notices, and meet the corresponding-source
obligations when distributing covered binaries.

Before publishing a complete distribution, check the provenance and terms of
imported libraries, binary-only programs, resources and other third-party files.
The repository-wide statement is not by itself proof of the rights to every
bundled artifact. Treat TOS ROMs and external compiler/toolchain packages
separately; do not assume that the project's GPL notice licenses those for
redistribution. Record exclusions or separately obtained dependencies explicitly.
