/*
 * Index constants for the generated MAGXCONF resource (MAGXCONF.RSH)
 * and the small MagiC cookie records used by the CPX.
 */
#ifndef CPXDATA_H
#define CPXDATA_H

#include <stdint.h>

/* Object indices in the MAIN resource tree. */
#define ROOT             0
#define MAIN             0
#define VERSION          1
#define CF_FASTLOAD      2
#define CF_TOSCOMPAT     3
#define CF_SMARTREDRAW   4
#define CF_GROWBOX       5
#define CF_FLOPPY_DMA    6
#define CF_PULLDOWN      7
#define SAVE             10
#define OK               11
#define CANCEL           12

/* Free-string indices in rs_frstr[]. */
#define CPXTITLE_EN      0
#define CF_VERSION_EN    1
#define CF_FASTLOAD_EN   2
#define CF_TOSCOMPAT_EN  3
#define CF_SMARTREDRAW_EN 4
#define CF_GROWBOX_EN    5
#define CF_FLOPPY_DMA_EN 6
#define CF_PULLDOWN_EN   7
#define SAVE_EN          8
#define OK_EN            9
#define CANCEL_EN        10
#define AL_NO_MAGIC_EN   11
#define AL_NOT_ACTIVE_EN 12

#define CPXTITLE_DE      13
#define CF_VERSION_DE    14
#define CF_FASTLOAD_DE   15
#define CF_TOSCOMPAT_DE  16
#define CF_SMARTREDRAW_DE 17
#define CF_GROWBOX_DE    18
#define CF_FLOPPY_DMA_DE 19
#define CF_PULLDOWN_DE   20
#define SAVE_DE          21
#define OK_DE            22
#define CANCEL_DE        23
#define AL_NO_MAGIC_DE   24
#define AL_NOT_ACTIVE_DE 25

#define CPXTITLE_FR      26
#define CF_VERSION_FR    27
#define CF_FASTLOAD_FR   28
#define CF_TOSCOMPAT_FR  29
#define CF_SMARTREDRAW_FR 30
#define CF_GROWBOX_FR    31
#define CF_FLOPPY_DMA_FR 32
#define CF_PULLDOWN_FR   33
#define SAVE_FR          34
#define OK_FR            35
#define CANCEL_FR        36
#define AL_NO_MAGIC_FR   37
#define AL_NOT_ACTIVE_FR 38

/* MagiC Sconfig() configuration bits handled by this CPX. */
#define SCB_NFAST 0x010L
#define SCB_CMPTB 0x020L
#define SCB_NSMRT 0x040L
#define SCB_NGRSH 0x080L
#define SCB_NHALT 0x100L
#define SCB_PULLM 0x400L
#define SCB_FLPAR 0x800L

/* GEM country identifiers used by xcpb->Country_Code. */
#define COUNTRY_DE 1
#define COUNTRY_FR 2
#define COUNTRY_UK 3

/* Cookie 'MAGX': config_status, DOSVARS *, AESVARS *. */
typedef struct {
    int32_t magic;
    void *membot;
    void *aes_start;
    int32_t magic2;
    int32_t date;
    void *chgres;
    void *shel_vector;
    void *aes_bootdrv;
    void *vdi_device;
    void *reservd1;
    void *reservd2;
    void *reservd3;
    int16_t version;
    int16_t release;
} AESVARS;

typedef struct {
    int32_t config_status;
    void *dosvars;
    AESVARS *aesvars;
} MAGX_COOKIE;

#endif
