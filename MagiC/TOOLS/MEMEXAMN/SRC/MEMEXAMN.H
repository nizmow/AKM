/* Fixed-width MagiC records used by MEMEXAMN. */
#ifndef MEMEXAMN_H
#define MEMEXAMN_H

#include <stdint.h>

typedef struct syshdr {
    uint16_t os_entry;
    uint16_t os_version;
    uint32_t os_start;
    struct syshdr *os_base;
    uint32_t os_membot;
    uint32_t os_rsv1;
    uint32_t *os_magic;
    uint32_t os_gendat;
    uint16_t os_palmode;
    uint16_t os_gendatg;
    uint32_t root;
    uint32_t kbshift;
    uint32_t run;
    uint32_t p_rsv2;
} SYSHDR;

typedef struct {
    uint32_t magic;
    uint32_t membot;
    uint32_t aes_start;
    uint32_t magic2;
    uint32_t date;
    uint32_t chgres;
    uint32_t shel_vector;
    uint32_t aes_bootdrv;
    uint32_t vdi_device;
    uint32_t reservd1;
    uint32_t reservd2;
    uint32_t reservd3;
    uint16_t version;
    uint16_t release;
    uint32_t _basepage;
} AESVARS;

typedef struct {
    uint32_t in_dos;
    uint32_t dos_time;
    uint32_t dos_date;
    uint32_t dos_stack;
    uint32_t pgm_superst;
    uint32_t *memlist;
    uint32_t act_pd;
    uint32_t fcbx;
    uint16_t fcbn;
    uint32_t dmdx;
    uint32_t imbx;
    uint32_t resv_intmem;
    uint32_t etv_critic;
    uint32_t err_to_str;
    uint32_t xaes_appls;
    void *mem_root;
    void *ur_pd;
} DOSVARS;

typedef struct {
    uint32_t mcb_magic;
    uint32_t mcb_len;
    uint32_t mcb_owner;
    uint32_t mcb_prev;
} MCB;

typedef struct {
    void *p_lowtpa;
    void *p_hitpa;
    void *p_tbase;
    long p_tlen;
    void *p_dbase;
    long p_dlen;
    void *p_bbase;
    long p_blen;
    void *p_dta;
    void *p_parent;
    void *p_reserved;
    char *p_env;
    uint8_t p_junk[8];
    uint32_t p_undef[18];
    uint8_t p_cmdlin[128];
} BASPAG;

#endif
