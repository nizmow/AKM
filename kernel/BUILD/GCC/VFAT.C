/* Compile the original module with its kernel ABI, not the host C library. */
#define cdecl __attribute__((cdecl))
struct _mx_dmd;
struct _mx_fd;
struct _mx_dosfd;
#include "../../DOS/VFAT.C"

_Static_assert(sizeof(WORD) == 2 && sizeof(UWORD) == 2, "kernel words must be 16-bit");
_Static_assert(sizeof(LONG) == 4 && sizeof(void *) == 4, "kernel longs and pointers must be 32-bit");
_Static_assert(sizeof(DIR) == 32 && sizeof(LDIR) == 32, "FAT directory entries must be 32 bytes");
_Static_assert(__builtin_offsetof(DIR, xdata) == 0x0e, "DIR.xdata layout mismatch");
_Static_assert(__builtin_offsetof(DIR, flen) == 0x1c, "DIR.flen layout mismatch");
/* Offsets follow DOS/INC/STRUCTS.INC directives, not its stale offset comments. */
_Static_assert(__builtin_offsetof(MX_DOSFD, fd_dev) == 0x08, "FD device layout mismatch");
_Static_assert(__builtin_offsetof(MX_DOSFD, fd_fpos) == 0x34, "FD position layout mismatch");
_Static_assert(__builtin_offsetof(MX_DOSFD, fd_longname) == 0x50, "FD long-name layout mismatch");
