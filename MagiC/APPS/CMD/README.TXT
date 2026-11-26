Andreas Kromke
2018-03-05

This directory retains binaries, help, and historical documentation. Active
MCMD sources are in core/mcmd/SRC. The old CMD source is in original-import.


This is the highly compact command line interpreter for TOS, KAOS and MagiC.
By changing the first lines one can choose with variant to build.

The executable should be named as follows:

- MCMD.TOS for MagiC
- KCMD.TOS for KAOS
- CMD.TOS for TOS
- *.ACC as accessory

The TOS version tries to avoid operating system bugs that do not exist without TOS.

This program seems to be not localised, i.e. is written for Germany.

CMD.DOC is the original documentation (only German) in WORDPLUS format.
CMD.TXT has been converted to UTF-8.


2026-02-11
==========

Overhauled version that is capable of displaying large numbers for free volume space.
Also now localised for German, French and English.
Only the MCMD version has been built and basically tested!

Regarding EN version:

Due to conflicts, the DIR sort modes remain 'A' for type and 'G' for size.
In other cases you can either press "J"/"O"/"Y" for Ja/Oui/Yes resp. "W"/"R" for RETRY etc..

2026-09-20
==========

v2.69:
Crash in GOTO avoided when batch file has lines with more than 89 characters.

2026-10-10
==========

v2.70:
REM internal command added, for compatibility

2026-10-31
==========

v2.71:
- "IF EXIST bla" finds files and folders, "IF EXIST bla\" only finds folders, like in MS-DOS.
- "IF" also works in interactive mode, like in MS-DOS.
- Some help texts improved.

