/* Freestanding Linux/m68k regression of the actual VFAT C/assembly paths. */
#include "VFAT.C"

/* A bounded directory image: real read/write/seek behavior, no kernel stubs. */
static DIR records[8];
static MX_DOSFD directory;
static DIR short_entry = { .name = { 'E','X','A','M','P','L','~','1','T','X','T' } };

static LONG image_read(MX_DOSFD *fd, LONG count, void *buffer)
{
     unsigned char *src;
     unsigned char *dst = buffer;
     LONG i;
     if (count < 0 || fd->fd_fpos > sizeof(records) ||
         (ULONG) count > sizeof(records) - fd->fd_fpos)
          return 0;
     src = (unsigned char *) records + fd->fd_fpos;
     fd->fd_fpos += count;
     if (!buffer)
          return (LONG) src;
     for (i = 0; i < count; i++)
          dst[i] = src[i];
     return count;
}

static LONG image_write(MX_DOSFD *fd, LONG count, void *buffer)
{
     unsigned char *src = buffer;
     unsigned char *dst;
     LONG i;
     if (count < 0 || fd->fd_fpos > sizeof(records) ||
         (ULONG) count > sizeof(records) - fd->fd_fpos)
          return ERANGE;
     dst = (unsigned char *) records + fd->fd_fpos;
     for (i = 0; i < count; i++)
          dst[i] = src[i];
     fd->fd_fpos += count;
     return count;
}

static LONG image_seek(MX_DOSFD *fd, LONG offset, WORD mode)
{
     if (mode != 0 || offset < 0 || (ULONG) offset > sizeof(records))
          return ERANGE;
     fd->fd_fpos = offset;
     return offset;
}

static MX_DEV image_device = {
     .dev_read = image_read, .dev_write = image_write, .dev_seek = image_seek
};

static LONG directory_roundtrip(void)
{
     char expected[] = "Example long \x82name.txt";
     char name[64], alias[14];
     LONG start, ret;
     directory.fd_dev = &image_device;
     if (vf_crnam(&directory, &short_entry, expected) != sizeof(DIR))
          return 16;
     if (directory.fd_fpos != 3 * sizeof(DIR) ||
         ((LDIR *) &records[0])->head != 0x42 ||
         ((LDIR *) &records[1])->head != 1)
          return 17;
     if (p_fseek(&directory, 0) != 0)
          return 18;
     ret = _vf_readdir(&directory, name, sizeof(name), alias, &start, FINDALL);
     if (ret != (LONG) &records[2])
          return 19;
     if (start != 0)
          return 22;
     if (strcmp(name, expected))
          return 23;
     if (strcmp(alias, "EXAMPL~1.TXT"))
          return 24;
     p_fseek(&directory, 0);
     if (_vf_readdir(&directory, name, 8, alias, &start, FINDALL) != ERANGE)
          return 20;
     /* A corrupt LFN checksum must fall back to the intact short alias. */
     ((LDIR *) &records[0])->chksum ^= 1;
     p_fseek(&directory, 0);
     ret = _vf_readdir(&directory, name, sizeof(name), alias, &start, FINDALL);
     return (ret != (LONG) &records[2] || start != -1 ||
             name[0] != 0 || strcmp(alias, "EXAMPL~1.TXT")) ? 21 : 0;
}

void _start(void)
{
     unsigned char name[] = { 0x41, 0x00, 0xe9, 0x00, 0x00, 0x00, 0x42, 0x00 };
     unsigned char padding[] = { 0xff, 0xff };
     unsigned char unknown[] = { 0x03, 0x26 };
     char text[5] = { 0x55, 0x55, 0x55, 0x55, 0x55 };
     char *end = text;
     long failed;

     unic2ibm(&end, name, 4);
     failed = end != text + 2 || text[0] != 'A' ||
              (unsigned char) text[1] != 0x82 || text[2] != 0 || text[3] != 0x55;
     end = text;
     unic2ibm(&end, padding, 1);
     failed |= end != text || text[0] != 0;
     end = text;
     unic2ibm(&end, unknown, 1);
     failed |= end != text + 1 || (unsigned char) text[0] != 0xb6 || text[1] != 0;

     /* FAT LFN checksum of the eleven-byte on-disk 8.3 name, not the dot form. */
     failed |= (unsigned char) vf_chksum("FOO     TXT") != 0x65;
     failed |= stricmp("\x84pfel", "\x8epfel") != 0;
     failed |= (unsigned char) toupper((char) 0xc0) != 0xc1;
     failed |= directory_roundtrip();

     /* Linux exit(status), not an Atari GEMDOS trap. */
     register long syscall __asm__("d0") = 1;
     register long status __asm__("d1") = failed;
     __asm__ volatile ("trap #0" : : "d" (syscall), "d" (status) : "memory");
     __builtin_unreachable();
}
