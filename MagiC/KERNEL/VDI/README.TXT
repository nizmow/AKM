Andreas Kromke
2025-12-14

All VDI related files are written by Sven and Wilfried Behne, except some corrections, see below.
Note that VDI for MagiC is often called MVDI, contrary to NVDI.


BIN/
	VDI object files to be linked into kernel

DRIVERS/
	Drivers for Atari and Mac, to be loaded at runtime from the appropriate paths
	MFM16M.SYS had a bug, and its binary is patched. The original is in ORIG.
	MFM4IP.SYS has been repaired via source code. The original is in ORIG.

DRIVERS/OSD.TXT
	Original mail from Wilfried Behne describing the naming scheme of .OSD and .SYS files.
	Note that .OSD files can be replaced by .NOD files from NVDI to get significantly faster graphics.
	Rough translation:

	MFA*.SYS : Screen drivers for ST/TT/Falcon/MagicPC
	MFM*.SYS : Screen drivers for MagicMac [and MagicMacX/AtariX]

	4IP : 4 colours, interleaved plane (ST Mid)
	MFA4IP.SYS : is obviously the 4-colour-driver for Atari
	MFM4IP.SYS : pendant for MagicMac, without hardware register access

	*.OSD : offscreen divers. Necessary drivers are in MIN_ATAR/, others are desirable.

	MagicMac needs all drivers from OSD/ and all from MIN/ATAR

SRC/
	Sources for both MVDI and the loadable drivers.
	Note that these are untouched, i.e. not converted to UTF-8, but still in Atari text coding.
	MFM4IP.S has been repaired, as is caused colour palette trouble with NVDI and was not compilable.

Important notes:

	The driver "MFM16M.SYS" had an annoying bug. I patched the binary.
	I remember a wrong mask 0x1fff or similar, but but not the details.

	The driver "MFM4IP.SYS" had an annoying bug. I resolved it via source code.
	The symptom was a yellow colour instead of black, but only with NVDI.

	MXVDIKNP.O seems to be a patched version from MXVDIKNL.O. Its timestamp (2001-09-03)
	 is newer than those of the other files (1996-05-17 and 1998-02-06). Is is referred to
	 by the latest linker input files (.INP) and is most probably needed for MagicMacX/AtariX.
	 A binary diff reveals that I replaced two 68k instructions by NOPs.
