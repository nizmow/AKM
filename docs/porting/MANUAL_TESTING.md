# Manual hardware testing

Some Atari behaviour cannot be proved by NeoST: physical floppy formatting,
FDC error recovery, and interaction with a real drive and media. This file keeps
those tests reproducible and records their current status. Automated emulator
checks live in [TESTING.md](TESTING.md).

Use disposable media. Never run destructive tests against a disk containing
data you need, and never exercise hard-disk initialization paths on a working
installation.

## MGFORMAT

Status: **physical format and disk copy pending real hardware**. The archived
and rebuilt format dialogs match under MagiC, but NeoST reports
`Write fault auf A:` when MGFORMAT starts `Flopfmt`.

### Build the test kit

```sh
mise run apps -- mgformat-hardware-kit
```

This builds and packages:

```text
build/atari/apps/mgformat-hardware-kit.tar.gz
build/atari/apps/mgformat-hardware/ARCHIVE/MGFORMAT.PRG
build/atari/apps/mgformat-hardware/ARCHIVE/MGFORMAT.RSC
build/atari/apps/mgformat-hardware/REBUILT/MGFORMAT.PRG
build/atari/apps/mgformat-hardware/REBUILT/MGFORMAT.RSC
```

The rebuilt executable is stripped for hardware use. The archive executable is
the original release binary and is the behavioural baseline.

### Install on the Atari

1. Copy one variant (`ARCHIVE` or `REBUILT`) to `C:\GEMSYS\GEMDESK\`.
2. Keep `MGFORMAT.PRG` and `MGFORMAT.RSC` in the same directory.
3. Start MagiC and wait until MAGXDESK owns the desktop.
4. Use MAGXDESK's `Format`/`Disk copy` actions, or run from a shell:
   - `MGFORMAT.PRG -f A:` for physical format.
   - `MGFORMAT.PRG -c A: A:` for one-drive disk copy.
   - `MGFORMAT.PRG -c A: B:` when drive B is available.
5. Run the archive first, then replace only `MGFORMAT.PRG` with the rebuilt
   variant. Keep the same resource and media geometry.

Do not use `-fh` on a real hard disk. It selects the logical-initialize path and
can destroy an installation.

### Test matrix

Use clean DD media for 720K and HD media for 1.44M. The format dialog defaults
to 80 tracks, 9 sectors, 2 sides; the `HD` button selects 18 sectors.

| Case | Action | Pass condition |
|---|---|---|
| Dialog | Launch format mode | Format dialog opens; no AES alert |
| DD format | Label `TEST720`, default geometry, `Formatieren` | Completion alert; volume label and empty root visible |
| HD format | Click `HD`, label `TEST144`, `Formatieren` | Completion alert; 1.44M geometry and empty root |
| Logical init | Put a file on a disposable disk, select `Init` | Root/FAT cleared; no physical-format error |
| Disk copy, two drives | Copy a known disk from A: to B: | Files/labels match the source; source remains readable |
| Disk copy, one drive | `-c A: A:` and follow the source/destination swap prompts | Same result; prompts identify the required disk |
| Cancel | Start format or copy and press `Abbruch` | Returns cleanly; no hang or lost desktop |
| No disk/write-protected | Start format with no media or a protected disk | Clear error alert; app remains usable |

Do not compare serial numbers or timestamps. `Protobt` may generate different
values; compare geometry, label, directory contents and copied file bytes.

### Record the result

Report:

- Machine model, TOS version, MagiC version, drive type and media
- Archive result for each case
- Rebuilt result for each case
- Exact alert text and the button/action that produced it
- Whether the failure reproduces with the archive

If the rebuilt binary differs from the archive, preserve the media state and
the `MGFORMAT.INF` file from `C:\GEMSYS\HOME\`, if present.

## Adding future manual tests

Add one section per component with:

1. Build command and exact artifact paths
2. Hardware/OS prerequisites and safety boundaries
3. Archived-versus-rebuilt comparison matrix
4. Pass/fail evidence and current verification status
5. Any paths that remain untested

Keep emulator-only coverage in [TESTING.md](TESTING.md); do not duplicate it
here.
