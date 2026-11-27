# Porting workflow and pitfalls

Before picking work, read the [open GitHub issues](https://github.com/nizmow/AKM/issues), the relevant [build catalogue](APPLICATION_BUILDS.md) row, and the [verification limits](TESTING.md). Before changing an application, follow the [application porting workflow](PORTING_NOTES.md#application-porting-workflow), its boundary checklist, and the [encoding guidance](PORTING_NOTES.md#source-encoding-versus-runtime-bytes). Map shared ABI contracts and capture an archive baseline before broad edits. Prove a representative boundary before splitting remaining work. The integration owner checks a stable source snapshot; a representative compile does not prove the whole port.

## ABI and data boundaries

- `Makefile.apps` uses MiNTLib's default stack ABI and 32-bit `int`. Do not borrow kernel `-mshort`/`-mfastcall` flags or archived Pure C libraries. AES/VDI words, output pointers, message buffers and persisted fields can still be 16-bit. Check `scanf` formats, assert disk sizes and offsets, and match pointer target types: SDK long outputs use `long *`, while `int32_t` is `int`. Guard native `long` width where it matters.
- Use public `gem.h`/`gemx.h` callback definitions. MagiC dialog/listbox callbacks take the SDK's by-value argument structures; separate `short` arguments are not equivalent.
- Trace the callee's trap layout for MagiC extensions. `SC_WBACK` takes a word subcommand, unlike generic MiNTLib `Sconfig`'s long argument. A successful return can be a query rather than a state change.
- Read the library-specific SLB notes before linking. Registered editor AES handlers take `AESPB *` in A0; lifecycle entries use the stack. The SLB TEXT header must come before any startup wrapper. Preserve linked constructors and BSS relocations when supplying custom startup.
- Keep fixed-width binary keys distinct from strings. A four-byte key needs a separate five-byte buffer when read through a string scanner.
- Use `scripts/atari-text/atari_text.py` for known Atari ST text. Keep edited source UTF-8 and preserve Atari runtime bytes explicitly. CP850 is not an Atari ST substitute. Never transcode binary RSC files.

## Tooling and verification

- Run `mise tasks` before adding tooling. Reuse `scripts/port_scan.py` and `scripts/port_deps.py` for inventory and PRJ resolution, `scripts/elf_to_slb.py` for packaging, and `scripts/atari-container` for container commands.
- Reuse `shared/tos-args/TOS_ARGS.H` for PCMXSTRT-style raw-tail quoting while retaining MiNTLib's extended ARGV. Reuse only helpers with matching contracts; don't copy another application's compatibility header or build a universal AES shim just because names match.
- Use fresh numbered edit snapshots. Check the source affected by legacy parsers and automated preprocessor rewrites; a preview can hide altered line boundaries.
- Follow the [NeoST skill](../../.agents/skills/neost/SKILL.md). Reuse `scripts/neost_common.py` for discovery, ROM lookup and fresh artifacts, `scripts/neost_image.py` for disposable images, `scripts/neost_screen.py` for PPM comparisons, and `scripts/neost_server.py` as the canonical server client. `scripts/neost_host.py` is the optional [session lifecycle layer](NeoST_HOST.md). Keep the machine profile, fixtures, timing and assertions in the application-specific scenario. For components under `core/` or `base/`, keep the scenario and its guest probes in the component's `tests/` directory; existing runners and shared host-helper regressions remain in `scripts/`.
- Clone writable MagiC images and association files; saving an editor can also update the running desktop. Verify guest effects, not just emulator exit status. In GEM mouse automation, clamp below the menu bar rather than through `(0,0)`, which can open the system menu and consume later input.
- Compare meaningful cache fields, not process addresses. APPLICAT version-2 caches contain runtime `ICONBLK` pointers; document exactly which fields are excluded and verify geometry, tables and bitmap data.
