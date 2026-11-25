Strict Python workflow (recommended for new work)
================================================

Requires Python 3, with no third-party packages. Run from the repository root:

  python3 NON-TOS/atari-utf8-conversion/atari_text.py decode OLD.C -o OLD.C.utf8
  python3 NON-TOS/atari-utf8-conversion/atari_text.py encode TEXT.utf8 -o TEXT.ST
  printf '%s' 'Straße' | python3 NON-TOS/atari-utf8-conversion/atari_text.py c-literal

The last command prints "Stra\236e", an ASCII C string literal containing the
correct Atari byte for sharp S. c-literal takes plain text, NOT C source; paste
its output in the relevant literal. Existing backslashes are literal text,
not interpreted escapes. Include actual newlines if they belong in the string;
printf '%s' avoids accidentally including a terminal newline.

decode: Atari ST bytes to UTF-8.
encode: UTF-8 to Atari ST bytes.
c-literal: UTF-8 to a quoted ASCII C literal with fixed three-digit octal
escapes for non-ASCII bytes, controls, quotes, backslashes and question marks.

Input/output default to stdin/stdout; "-" explicitly selects either stream.
File modes preserve CR/LF bytes exactly, do not normalize Unicode, reject
invalid UTF-8 and unrepresentable characters, and return nonzero on errors.
Conversion finishes before an output file is opened. The tool refuses input
and output that name the same file, including symlinks/hardlinks. Shell
redirection can truncate files before the tool runs: never use > INPUT.

The tool reads the shared Atari2Unicode table in atari-utf8.c. It does not
guess encodings or substitute CP850. Byte 0xed is phi (U+03C6), not sigma.
The repository deliberately maps 0xf9 to bullet (U+2022), whereas Unicode's
ATARIST.TXT uses bullet operator (U+2219); all 256 bytes still round-trip.
Reference: https://www.unicode.org/Public/MAPPINGS/VENDORS/MISC/ATARIST.TXT

Use only for known text files, never .RSC, .PRG, images or other binary data.
For C ports, keep reviewed UTF-8 source as canonical and use explicit byte
escapes for Atari runtime strings. Do not convert an entire C source back to
raw Atari bytes and feed it to a compiler expecting UTF-8. See
docs/porting/PORTING_NOTES.md from the repository root.

Regression checks:
  python3 -m unittest discover -s NON-TOS/atari-utf8-conversion -p 'test_*.py'
These check all-byte round-trips, Atari-specific mappings, literal escaping,
and failure paths that must leave input and existing output untouched.

Historical utilities
====================
The utilities below are retained for existing workflows. In particular, the
old C encoder reports unsupported characters but can copy their UTF-8 bytes
through and still return success; prefer the strict Python CLI above.

Andreas Kromke
2018-04-29


Text coding conversion helpers.

atari-utf8.c
				Converts all input files between AtariST coding and UTF-8.
				It does not work "inplace", i.e. does not overwrite the source file,
				but creates new ones with name suffix ".utf8" or ".ataritext".
				Command line switch "-v" tells about each replacement.
				Normally converts from UTF-8 to AtariST.
				Command line switch "-r" converts back from AtariST to UTF-8.
				Do not write "-vr", but "-v -r".

atari-to-utf8.sh
utf8-to-atari.sh

				Shell scripts doing more or less the same, using the "recode" command.
