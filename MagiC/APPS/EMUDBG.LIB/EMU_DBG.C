/*********************************************************************
*
* Print string to host debug output, if possible, otherwise to stdout.
* For use with MagicOnLinux emulator
*
*********************************************************************/

#include <MGX_DOS.H>
#include <STDIO.H>
#include "emu_dbg.h"

typedef unsigned long UINT32_BE;
typedef void * PTR32_BE;
typedef unsigned short UINT16_BE;
typedef long PTR32_HOST;
typedef long PTR32x4_HOST[4];

/* EMU_DBG.S */
extern int _emu_dbgout(const PTR32_HOST *fn, void *param);

#define NOT_INITIALISED ((const PTR32_HOST *) 0xffffffffL)
static const PTR32_HOST *emu_dbgout_fn = NOT_INITIALISED;
static char buf[256];

/*
 The old MagiCMac system header which is now included in the
 new MacXSysHdr, for compatibility purposes with Behnes' MXVDI.
*/
typedef struct
{
	UINT32_BE magic;            /* is 'MagC' */
	UINT32_BE syshdr;           /* address of the Atari Syshdr structure */
	UINT32_BE keytabs;          /* 5*128 Bytes for keyboard tables */
	UINT32_BE ver;              /* version */
	UINT16_BE cpu;              /* CPU (30=68030, 40=68040) */
	UINT16_BE fpu;              /* FPU (0=none, 4=68881, 6=68882, 8=68040) */
	UINT32_BE boot_sp;          /* sp for booting */
	UINT32_BE biosinit;         /* to be called after initialisation */
	UINT32_BE pixmap;           /* data for the VDI */
	UINT32_BE offs_32k;         /* address offset for first 32k in the Mac */
	UINT32_BE a5;               /* global 68k register a5 for Mac programs */
	UINT32_BE tasksw;           /* != NULL, if task switch is necessary */
	UINT32_BE gettime;          /* get date and time */
	UINT32_BE bombs;            /* Atari function, called by the Mac */
	UINT32_BE syshalt;          /* "System halted", text string in 68k register a0 */
	UINT32_BE coldboot;
	UINT32_BE debugout;         /* for debugging */
	UINT32_BE prtis;            /*    for printer (PRT) */
	UINT32_BE prtos;            /* */
	UINT32_BE prtin;            /* */
	UINT32_BE prtout;           /* */
	UINT32_BE serconf;          /*    Rsconf for ser1 */
	UINT32_BE seris;            /*    for ser1 (AUX) */
	UINT32_BE seros;            /* */
	UINT32_BE serin;            /* */
	UINT32_BE serout;           /* */
	UINT32_BE xfs;              /* functions for the XFS (Mac file system driver) */
	UINT32_BE xfs_dev;          /*  corresponding file driver */
	UINT32_BE set_physbase;     /* change video memory address in Setscreen() (a0 points to stack of Setscreen()) */
	UINT32_BE VsetRGB;          /* set colour (a0 points to the stack of VsetRGB()) */
	UINT32_BE VgetRGB;          /* ask colour (a0 points to the stack of VgetRGB()) */
	UINT32_BE error;            /* pass error message in 68k register d0.l to the MacOS */
                                /*    error messages for MacSys_error: */
                                /*    -1: unsupported graphics format => no VDI driver */
	UINT32_BE init;             /* called on Atari warmboot */
	UINT32_BE drv2devcode;      /* convert drive number to device number */
	UINT32_BE rawdrvr;          /* Raw driver (eject) for Mac */
	UINT32_BE floprd;
	UINT32_BE flopwr;
	UINT32_BE flopfmt;
	UINT32_BE flopver;
	UINT32_BE superstlen;        /* size of supervisor stack per application */
	UINT32_BE dos_macfn;         /* DOS functions 0x60..0xfe */
	UINT32_BE settime;           /* xbios Settime */
	UINT32_BE prn_wrts;          /* text string to printer */
	UINT32_BE version;           /* version number of this structure */
	UINT32_BE in_interrupt;      /* interrupt counter for the MacOS side */
	UINT32_BE drv_fsspec;        /* List of FSSpec data for Mac drives */
	UINT32_BE cnverr;            /* LONG cnverr( WORD mac_errcode ) */
	UINT32_BE res1;              /* reserved */
	UINT32_BE res2;              /* reserved */
	UINT32_BE res3;              /* reserved */
} OldMmSysHdr;


typedef struct
{
	/* Atari -> host */
	UINT32_BE    MacSysX_magic;             /* is 'MagC' */
	UINT32_BE    MacSysX_len;               /* length of this structure */
	PTR32_BE     MacSysX_syshdr;            /* address of Atari Syshdr */
	PTR32_BE     MacSysX_keytabs;           /* 5*128 bytes for keyboard tables */
	PTR32_BE     MacSysX_mem_root;          /* memory lists */
	PTR32_BE     MacSysX_act_pd;            /* pointer to current process */
	PTR32_BE     MacSysX_act_appl;          /* pointer to current task (appl) */
	UINT32_BE    MacSysX_verAtari;          /* version number of MagicMacX.OS resp. MAGICLIN.OS */
	/* host -> Atari */
	UINT32_BE    MacSysX_verMac;            /* 0x20: version number of this structure */
	UINT16_BE    MacSysX_cpu;               /* 0x24: CPU (20 = 68020, 30=68030, 40=68040) */
	UINT16_BE    MacSysX_fpu;               /* 0x26: FPU (0=nothing,4=68881,6=68882,8=68040) */
	PTR32_HOST   MacSysX_init;              /* 0x28: first initialisation done */
	UINT32_BE    MacSysX_dev_in;            /* 0x2c: Bconin() */
	UINT32_BE    MacSysX_dev_out;           /* 0x30: Bconout() */
	UINT32_BE    MacSysX_Ikbdws;            /* 0x34: Ikbdws() */
	PTR32_HOST   MacSysX_biosinit;          /* 0x38: called after initialisation */
	UINT32_BE    MacSysX_dev_istat;         /* 0x3c: Bconstat() */
	UINT32_BE    MacSysX_dev_ostat;         /* 0x40: Bcostat() */
	PTR32_HOST   MacSysX_Dosound;           /* 0x44: Dosound() */
	PTR32x4_HOST MacSysX_VdiInit;           /* called after initialisation of VDI */
	PTR32_BE     MacSysX_pixmap;            /* 68k pointer, data for the VDI */
	PTR32_BE     MacSysX_pMMXCookie;        /* 68k pointer to MgMx Cookie */
	PTR32x4_HOST MacSysX_Xcmd;              /* XCMD commands */
	PTR32_HOST   MacSysX_PPCAddr;           /* actual host address of 68k address 0 (on 64-bit host: 0x00000000) */
	PTR32_HOST   MacSysX_VideoAddr;         /* actual host address of video memory (on 64-bit host: 0x80000000) */
	PTR32x4_HOST MacSysX_Exec68k;           /* here the host callback can run 68k code */
	PTR32_HOST   MacSysX_gettime;           /* LONG GetTime(void): get date and time */
	PTR32_HOST   MacSysX_settime;           /* void SetTime(LONG *time): set date and time */
	PTR32_HOST   MacSysX_Setpalette;        /* void Setpalette( int ptr[16] ) */
	PTR32_HOST   MacSysX_Setcolor;          /* int Setcolor( int nr, int val ) */
	PTR32_HOST   MacSysX_VsetRGB;           /* void VsetRGB( WORD index, WORD count, LONG *array ) */
	PTR32_HOST   MacSysX_VgetRGB;           /* void VgetRGB( WORD index, WORD count, LONG *array ) */
	PTR32_HOST   MacSysX_syshalt;           /* SysHalt( char *str ) "System halted" */
	PTR32_HOST   MacSysX_syserr;            /* SysErr( long val ) "a1 = 0 => bombs" */
	PTR32_HOST   MacSysX_coldboot;          /* ColdBoot(void) perform cold boot */
	PTR32_HOST   MacSysX_exit;              /* Called by Puntaes(), which is called by SHUTDOWN.PRG */
	PTR32_HOST   MacSysX_debugout;          /* MacPuts( char *str ) for debugging */
	PTR32_HOST   MacSysX_error;             /* d0 = -1: no graphics driver */
	PTR32_HOST   MacSysX_VDI;               /* VDI call, parameters in 68k registers */
	PTR32_HOST   MacSysX_resbc;
	PTR32_HOST   MacSysX_resc0;
	PTR32_HOST   MacSysX_prtouts;           /* LONG PrnOuts({char *buf, LONG count}) character string to printer */
	PTR32_HOST   MacSysX_serconf;           /* Rsconf( void *params ) for ser1 */
	PTR32_HOST   MacSysX_rescc;
	PTR32_HOST   MacSysX_resd0;
	PTR32_HOST   MacSysX_resd4;
	PTR32_HOST   MacSysX_resd8;
	PTR32_HOST   MacSysX_SerOpen;           /* DEV_SER: open serial port */
	PTR32_HOST   MacSysX_SerClose;          /* DEV_SER: close serial port */
	PTR32_HOST   MacSysX_SerRead;           /* DEV_SER: read multiple characters from serial port */
	PTR32_HOST   MacSysX_SerWrite;          /* DEV_SER: write multiple characters to serial port */
	PTR32_HOST   MacSysX_SerStat;           /* DEV_SER: read/write state for serial port */
	PTR32_HOST   MacSysX_SerIoctl;          /* DEV_SER: Ioctl calls for serial port */
	PTR32x4_HOST MacSysX_GetKeybOrMouse;    /* get keys and mouse events */
	PTR32_HOST   MacSysX_dos_macfn;         /* DosFn({int,void*} *) DOS functions 0x60..0xfe */
	UINT32_BE    MacSysX_xfs_version;       /* version of host XFS */
	UINT32_BE    MacSysX_xfs_flags;         /* flags for host XFS */
	PTR32x4_HOST MacSysX_xfs;               /* central entry to host XFS */
	PTR32x4_HOST MacSysX_xfs_dev;           /* corresponding file driver */
	PTR32x4_HOST MacSysX_drv2devcode;       /* convert drive number to device number */
	PTR32x4_HOST MacSysX_rawdrvr;           /* LONG RawDrvr({int, long} *) Raw driver (eject) for Mac */
	PTR32_HOST   MacSysX_Daemon;            /* call for the mmx daemon */
	PTR32_HOST   MacSysX_BlockDevice;       /* new for MagicOnLinux */
	PTR32_HOST   MacSysX_Network;           /* network functions */
	PTR32_HOST   MacSysX_Setscreen;         /* XBIOS #5, void Setscreen(void *log, void *phys, int res) */
	PTR32_HOST   MacSysX_Yield;             /* call to yield CPU time (idle) */
	OldMmSysHdr  MacSys_OldHdr;             /* for compatibility with Behne's code */
} MacXSysHdr;


/*
 The Cookie structure is provided by the emulator. Its address
 is passed to the kernel via the information transfer structure.
 Except the first three ones, the other structure members are
 written by the kernel.
*/
typedef struct
{
	UINT32_BE mgmx_magic;        /* 0: is "MgMx" */
	UINT32_BE mgmx_version;      /* 4: version number */
	UINT32_BE mgmx_len;          /* 8: structure length */
	UINT32_BE mgmx_xcmd;         /* 12: load and manage PPC (Mac PowerPC CPU) libraries */
	UINT32_BE mgmx_xcmd_exec;    /* 16: call PPC function from a PPC library */
	UINT32_BE mgmx_internal;     /* 20: 68k address of the information transfer structure */
	UINT32_BE mgmx_daemon;       /* 24: function call for the "mmx.prg" background (idle) process */
} MgMxCookieData;


/*********************************************************************
*
* get a Cookie
*
*********************************************************************/

static COOKIE *getcookie(long key)
{
	long ret = xbios(39, 'AnKr', 4, key);
	if ((ret == 39) || (ret < 0))
	{
		/* not implemented */
		ret = 0;
	}

	return (COOKIE *) ret;
}


/*********************************************************************
*
* Initialise emu_dbgout_fn to host function or NULL
*
*********************************************************************/

static void emu_dbgout_init(void)
{
	const COOKIE *pcookie = getcookie('MgMx');
	if (pcookie != NULL)
	{
		const MgMxCookieData *pMgMxCookie = (const MgMxCookieData *) pcookie->value;
		const MacXSysHdr *pEmuSysHdr = (const MacXSysHdr *) pMgMxCookie->mgmx_internal;
		if (pEmuSysHdr->MacSysX_magic == 'MagC')
		{
			/* Cconws("MagiC Emulator found"); */
			emu_dbgout_fn = &pEmuSysHdr->MacSysX_debugout;
			return;
		}
	}

	emu_dbgout_fn = 0;
}


/*********************************************************************
*
* Send character string to emulator or to stdout (fallback)
*
*********************************************************************/

void emu_prints(const char *text)
{
	if (emu_dbgout_fn == NOT_INITIALISED)
	{
		/* first call: initialise */
		emu_dbgout_init();
	}

	if (emu_dbgout_fn)
	{
		/* call assembler part (illegal instruction) */
  		_emu_dbgout(emu_dbgout_fn, text);
	}
	else
	{
		/* fallback solution */
		printf(text);
	}
}


/*********************************************************************
*
* Send printf() to emulator or to stdout (fallback)
*
*********************************************************************/

int emu_printf(const char *format, ...)
{
	int ret;

	va_list args;
	va_start(args, format);
	
	ret = vsprintf(buf, format, args);
	emu_prints(buf);
	return ret;
}
