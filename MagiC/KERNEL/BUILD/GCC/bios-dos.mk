# BIOS/DOS kernel modules for the Atari (Falcon-capable, German) MAGIC.RAM.
#
# Included from Makefile.atari.  All intermediate files live under
# build/atari/kernel/bios-dos; original sources are never modified in place.
#
# Every MAS source uses the shared syntax adapter and canonical Atari strings.

BIOSDOS_DIR       := $(BUILD_DIR)/kernel/bios-dos
BIOSDOS_ASM_DIR   := $(BIOSDOS_DIR)/asm
BIOSDOS_INC_DIR   := $(BIOSDOS_DIR)/include
BIOSDOS_COMMON    := $(BIOSDOS_INC_DIR)/common
BIOSDOS_BIOS_INC  := $(BIOSDOS_INC_DIR)/bios
BIOSDOS_DOS_INC   := $(BIOSDOS_INC_DIR)/dos
BIOSDOS_READ_INC  := $(BIOSDOS_INC_DIR)/read_inf

BIOSDOS_ADAPTER    := MagiC/KERNEL/BUILD/GCC/mas_to_vasm.py
BIOSDOS_MK         := MagiC/KERNEL/BUILD/GCC/bios-dos.mk
BIOSDOS_ATARI_TABLE := NON-TOS/atari-utf8-conversion/atari-utf8.c
BIOSDOS_TOOLS      := $(BIOSDOS_ADAPTER) $(BIOSDOS_ATARI_TABLE) $(BIOSDOS_MK)

BIOSDOS_VASM       ?= $(if $(VASM),$(VASM),vasmm68k_mot)
BIOSDOS_ASFLAGS    := -m68000 -spaces -Felf -quiet
BIOSDOS_CPPFLAGS   := -I$(abspath $(BIOSDOS_COMMON))

# Generated files are separate from the archived sources.
define biosdos_stage
$(1): $(2) $(BIOSDOS_TOOLS)
	@mkdir -p $$(@D)
	python3 $(BIOSDOS_ADAPTER) --atari-strings $$< $$@
endef

# --- shared include files (identical for every slice) ---------------------
$(eval $(call biosdos_stage,$(BIOSDOS_COMMON)/ERRNO.INC,MagiC/INC_AS/ERRNO.INC))
$(eval $(call biosdos_stage,$(BIOSDOS_COMMON)/KERNEL.INC,MagiC/KERNEL/INC/KERNEL.INC))
$(eval $(call biosdos_stage,$(BIOSDOS_COMMON)/DOS.INC,MagiC/KERNEL/INC/DOS.INC))
$(eval $(call biosdos_stage,$(BIOSDOS_COMMON)/STRUCTS.INC,MagiC/KERNEL/DOS/INC/STRUCTS.INC))
$(eval $(call biosdos_stage,$(BIOSDOS_COMMON)/VTSYS.INC,MagiC/KERNEL/INC/VTSYS.INC))
$(eval $(call biosdos_stage,$(BIOSDOS_COMMON)/LOWMEM.INC,MagiC/KERNEL/BIOS/COMMON/LOWMEM.INC))
$(eval $(call biosdos_stage,$(BIOSDOS_COMMON)/BIOS.INC,MagiC/KERNEL/BIOS/COMMON/BIOS.INC))
$(eval $(call biosdos_stage,$(BIOSDOS_COMMON)/HARDWARE.INC,MagiC/KERNEL/BIOS/COMMON/HARDWARE.INC))

BIOSDOS_COMMON_FILES := $(addprefix $(BIOSDOS_COMMON)/,ERRNO.INC KERNEL.INC DOS.INC \
                         STRUCTS.INC VTSYS.INC LOWMEM.INC BIOS.INC HARDWARE.INC)

# --- magibios slice -------------------------------------------------------
# BIOS.INC provides FRG/USA/FRA/UK.  DEADKEYS and MACOSX are only referenced
# by the Atari build and are zero there, matching MAS' undefined-symbol rules.
$(eval $(call biosdos_stage,$(BIOSDOS_BIOS_INC)/DEBUG.INC,MagiC/KERNEL/BIOS/MILAN/INC/DEBUG.INC))
$(eval $(call biosdos_stage,$(BIOSDOS_BIOS_INC)/AUTO.S,MagiC/KERNEL/BIOS/COMMON/AUTO.S))
$(eval $(call biosdos_stage,$(BIOSDOS_BIOS_INC)/PUNTAES.S,MagiC/KERNEL/BIOS/COMMON/PUNTAES.S))
$(eval $(call biosdos_stage,$(BIOSDOS_BIOS_INC)/READ_KTB.S,MagiC/KERNEL/BIOS/COMMON/READ_KTB.S))
$(eval $(call biosdos_stage,$(BIOSDOS_BIOS_INC)/HANDLKEY.S,MagiC/KERNEL/BIOS/COMMON/HANDLKEY.S))
$(eval $(call biosdos_stage,$(BIOSDOS_BIOS_INC)/SCSI.S,MagiC/KERNEL/BIOS/ATARI/MODULES/SCSI.S))
$(eval $(call biosdos_stage,$(BIOSDOS_BIOS_INC)/FDC.S,MagiC/KERNEL/BIOS/ATARI/MODULES/FDC.S))
$(eval $(call biosdos_stage,$(BIOSDOS_BIOS_INC)/DRIVE.S,MagiC/KERNEL/BIOS/ATARI/MODULES/DRIVE.S))
$(eval $(call biosdos_stage,$(BIOSDOS_BIOS_INC)/DSP.S,MagiC/KERNEL/BIOS/ATARI/MODULES/DSP.S))
$(eval $(call biosdos_stage,$(BIOSDOS_BIOS_INC)/SERIAL.S,MagiC/KERNEL/BIOS/ATARI/MODULES/SERIAL.S))
$(eval $(call biosdos_stage,$(BIOSDOS_BIOS_INC)/CLOCK.S,MagiC/KERNEL/BIOS/ATARI/MODULES/CLOCK.S))
$(eval $(call biosdos_stage,$(BIOSDOS_BIOS_INC)/VIDEO.S,MagiC/KERNEL/BIOS/ATARI/MODULES/VIDEO.S))
$(eval $(call biosdos_stage,$(BIOSDOS_BIOS_INC)/KEYB.S,MagiC/KERNEL/BIOS/ATARI/MODULES/KEYB.S))
$(eval $(call biosdos_stage,$(BIOSDOS_BIOS_INC)/COOK.S,MagiC/KERNEL/BIOS/ATARI/MODULES/COOK.S))
$(eval $(call biosdos_stage,$(BIOSDOS_BIOS_INC)/IVID.S,MagiC/KERNEL/BIOS/ATARI/MODULES/IVID.S))

BIOSDOS_BIOS_FILES := $(addprefix $(BIOSDOS_BIOS_INC)/,DEBUG.INC AUTO.S PUNTAES.S \
                        READ_KTB.S HANDLKEY.S SCSI.S FDC.S DRIVE.S DSP.S SERIAL.S \
                        CLOCK.S VIDEO.S KEYB.S COOK.S IVID.S)

# --- DOS slice (DEBUG.INC differs from the BIOS one: DEBLEVEL 0) ---------
$(eval $(call biosdos_stage,$(BIOSDOS_DOS_INC)/DEBUG.INC,MagiC/KERNEL/DOS/INC/DEBUG.INC))
BIOSDOS_DOS_FILES := $(BIOSDOS_DOS_INC)/DEBUG.INC

# --- read_inf slice -------------------------------------------------------
$(eval $(call biosdos_stage,$(BIOSDOS_READ_INC)/DEBUG.INC,MagiC/KERNEL/ALLG/SRC/DEBUG.INC))
BIOSDOS_READ_FILES := $(BIOSDOS_READ_INC)/DEBUG.INC

# --- top-level assembler sources -----------------------------------------
$(eval $(call biosdos_stage,$(BIOSDOS_ASM_DIR)/magibios.S,MagiC/KERNEL/BIOS/ATARI/MAGIBIOS.S))
$(eval $(call biosdos_stage,$(BIOSDOS_ASM_DIR)/xfs95ini.S,MagiC/KERNEL/BIOS/ATARI/XFS95/XFS95INI.S))
$(eval $(call biosdos_stage,$(BIOSDOS_ASM_DIR)/read_inf.S,MagiC/KERNEL/ALLG/SRC/READ_INF.S))
$(eval $(call biosdos_stage,$(BIOSDOS_ASM_DIR)/ser_my.S,MagiC/KERNEL/SER_MY.S))
$(eval $(call biosdos_stage,$(BIOSDOS_ASM_DIR)/magidos.S,MagiC/KERNEL/DOS/MAGIDOS.S))
$(eval $(call biosdos_stage,$(BIOSDOS_ASM_DIR)/malloc.S,MagiC/KERNEL/DOS/MALLOC.S))
$(eval $(call biosdos_stage,$(BIOSDOS_ASM_DIR)/dev_bios.S,MagiC/KERNEL/DOS/DEV_BIOS.S))
$(eval $(call biosdos_stage,$(BIOSDOS_ASM_DIR)/dev_pipe.S,MagiC/KERNEL/DOS/DEV_PIPE.S))
$(eval $(call biosdos_stage,$(BIOSDOS_ASM_DIR)/dev_mem.S,MagiC/KERNEL/DOS/DEV_MEM.S))
$(eval $(call biosdos_stage,$(BIOSDOS_ASM_DIR)/dfs_u.S,MagiC/KERNEL/DOS/DFS_U.S))
$(eval $(call biosdos_stage,$(BIOSDOS_ASM_DIR)/dfs_fat.S,MagiC/KERNEL/DOS/DFS_FAT.S))
$(eval $(call biosdos_stage,$(BIOSDOS_ASM_DIR)/ende.S,MagiC/KERNEL/ENDE.S))

BIOSDOS_STAGED := $(BIOSDOS_COMMON_FILES) $(BIOSDOS_BIOS_FILES) $(BIOSDOS_DOS_FILES) \
                  $(BIOSDOS_READ_FILES) \
                  $(addprefix $(BIOSDOS_ASM_DIR)/,magibios.S xfs95ini.S read_inf.S \
                    ser_my.S magidos.S malloc.S dev_bios.S dev_pipe.S dev_mem.S \
                    dfs_u.S dfs_fat.S ende.S)

# --- objects --------------------------------------------------------------
# Same order as BUILD/ATARI/M.INP; filesystem.o (VFAT, STD, XFS_VDOS) is
# provided by the parent Makefile and is deliberately not duplicated here.
BIOS_DOS_OBJECTS := \
	$(BIOSDOS_DIR)/magibios.o \
	$(BIOSDOS_DIR)/xfs95ini.o \
	$(BIOSDOS_DIR)/ide.o \
	$(BIOSDOS_DIR)/read_inf.o \
	$(BIOSDOS_DIR)/ser_my.o \
	$(BIOSDOS_DIR)/magidos.o \
	$(BIOSDOS_DIR)/malloc.o \
	$(BIOSDOS_DIR)/dfs_u.o \
	$(BIOSDOS_DIR)/dev_bios.o \
	$(BIOSDOS_DIR)/dev_pipe.o \
	$(BIOSDOS_DIR)/dev_mem.o \
	$(BIOSDOS_DIR)/dfs_fat.o \
	$(BIOSDOS_DIR)/ende.o

BIOSDOS_BIOS_INCLUDES := -I$(abspath $(BIOSDOS_COMMON)) -I$(abspath $(BIOSDOS_BIOS_INC))
BIOSDOS_DOS_INCLUDES  := -I$(abspath $(BIOSDOS_COMMON)) -I$(abspath $(BIOSDOS_DOS_INC))
BIOSDOS_READ_INCLUDES := -I$(abspath $(BIOSDOS_COMMON)) -I$(abspath $(BIOSDOS_READ_INC))

# COUNTRY=1 is the German localisation.  The BIOS gets FRG/USA/FRA/UK from
# BIOS.INC; MAGIDOS has no such include, MALLOC defines them itself.
$(BIOSDOS_DIR)/magibios.o: BIOSDOS_ASINCLUDES := $(BIOSDOS_BIOS_INCLUDES)
$(BIOSDOS_DIR)/magibios.o: BIOSDOS_ASDEFS     := -DCOUNTRY=1 -DDEADKEYS=0 -DMACOSX=0
$(BIOSDOS_DIR)/magidos.o:  BIOSDOS_ASINCLUDES := $(BIOSDOS_DOS_INCLUDES)
$(BIOSDOS_DIR)/magidos.o:  BIOSDOS_ASDEFS     := -DCOUNTRY=1
$(BIOSDOS_DIR)/malloc.o:   BIOSDOS_ASINCLUDES := $(BIOSDOS_DOS_INCLUDES)
$(BIOSDOS_DIR)/malloc.o:   BIOSDOS_ASDEFS     := -DCOUNTRY=1
$(BIOSDOS_DIR)/dfs_u.o:    BIOSDOS_ASINCLUDES := $(BIOSDOS_DOS_INCLUDES)
$(BIOSDOS_DIR)/dfs_fat.o:  BIOSDOS_ASINCLUDES := $(BIOSDOS_DOS_INCLUDES)
$(BIOSDOS_DIR)/dev_bios.o: BIOSDOS_ASINCLUDES := $(BIOSDOS_DOS_INCLUDES)
$(BIOSDOS_DIR)/dev_pipe.o: BIOSDOS_ASINCLUDES := $(BIOSDOS_DOS_INCLUDES)
$(BIOSDOS_DIR)/dev_mem.o:  BIOSDOS_ASINCLUDES := $(BIOSDOS_DOS_INCLUDES)
$(BIOSDOS_DIR)/read_inf.o: BIOSDOS_ASINCLUDES := $(BIOSDOS_READ_INCLUDES)

$(BIOSDOS_DIR)/%.o: $(BIOSDOS_ASM_DIR)/%.S $(BIOSDOS_STAGED) Makefile.atari
	@mkdir -p $(@D)
	$(BIOSDOS_VASM) $(BIOSDOS_ASFLAGS) $(BIOSDOS_ASINCLUDES) $(BIOSDOS_ASDEFS) -o $@ $<

# IDE.C comes from the original source tree; see the source-level fixes for
# the header case and the two compile-time size checks. -mfastcall matches the
# d0/d1/d2/a0/a1 register ABI used by DRIVE.S.
$(BIOSDOS_DIR)/ide.o: MagiC/KERNEL/BIOS/ATARI/MODULES/IDE.C \
                      MagiC/KERNEL/BIOS/ATARI/MODULES/IDE.H \
                      $(INCLUDE_DIR)/portab.h $(INCLUDE_DIR)/tos.h Makefile.atari
	@mkdir -p $(@D)
	$(CC) $(CPPFLAGS) -I$(abspath $(INCLUDE_DIR)) \
	  -I$(abspath MagiC/KERNEL/BIOS/ATARI/MODULES) \
	  $(CFLAGS) $(KERNEL_FLAGS) -MMD -MP -x c -c $< -o $@

.PHONY: bios-dos
bios-dos: $(BIOS_DOS_OBJECTS)

-include $(BIOSDOS_DIR)/ide.d
