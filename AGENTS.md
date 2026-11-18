# Repository working guide

## Start here

- Before choosing work, read `TODO.md` and the relevant row in `APPLICATION_BUILDS.md`.
  Distinguish compiled, launched and functionally tested; an unchecked runtime path
  is not evidence of a missing implementation.
- Before porting an application, read `PORTING_NOTES.md`, especially “Source
  encoding versus runtime bytes” and “Checklist for the next application”.
  Inspect its original PRJ and resource/data dependencies before editing.
- Keep documentation useful to someone returning in 12 months. Record durable
  contracts and verification limits, not a transcript of the latest session.

## Application porting traps

- `Makefile.apps` uses MiNTLib's default stack ABI and 32-bit `int`. Do not borrow
  kernel `-mshort`/`-mfastcall` flags or archived Pure C libraries. AES/VDI words,
  output pointers, message buffers and persisted fields can still be 16-bit.
  Check `scanf` formats as well as declarations; guard disk layouts with size
  and offset assertions.
- Use public `gem.h`/`gemx.h` callback definitions. MagiC dialog/listbox callbacks
  need the SDK's by-value argument structures; changing separate parameters to
  `short` does not establish the correct callback stack layout.
- Trace the callee's actual trap layout for MagiC extensions. For example,
  `SC_WBACK` takes a word subcommand, unlike generic MiNTLib `Sconfig`'s long
  argument. A successful return can be a query, not the requested state change.
- Distinguish fixed-width binary keys from NUL-terminated strings. A four-byte
  key needs a separate five-byte buffer when passed through a string scanner.
- Use `NON-TOS/atari-utf8-conversion/atari_text.py` for known Atari ST text.
  Keep canonical edited source UTF-8 and preserve Atari runtime bytes explicitly.
  CP850 is not an Atari ST substitute; never transcode binary RSC files.

## Reuse and verification

- Reuse `MagiC/APPS/TOS_ARGS.H` for the established PCMXSTRT-style raw-tail
  quoting contract, retaining MiNTLib's extended ARGV vector.
- Reuse `scripts/neost_common.py` for emulator discovery, ROM lookup, bounded
  execution/logging and fresh artifact directories. Keep fixtures, launch
  profiles, input timing and success assertions application-specific.
- Establish shared ABI contracts before splitting a port among file owners.
  Check representative boundary code before spreading an assumed adaptation
  across every module. Use fresh numbered snapshots for edits; never guess
  offsets. Legacy parsers and automated preprocessor rewrites can fail or alter
  line boundaries—check the actual affected text rather than trusting a preview.
- Follow `.agents/skills/neost/SKILL.md` for emulator work. Clone writable MagiC
  images and association files; saving an editor can also update the running
  desktop. Verify guest effects, not just emulator exit status.
- In GEM mouse automation, do not clamp through `(0,0)`: that can open the system
  menu and consume later input. Clamp below the menu bar, then move to the target.
- Compare meaningful cache fields, not process addresses. APPLICAT version-2
  caches contain runtime ICONBLK pointers; document precisely which fields are
  excluded from a comparison and verify geometry, tables and bitmap data.