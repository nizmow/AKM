.DEFAULT_GOAL := all

CROSS_COMPILE ?= m68k-atari-mintelf-
CC := $(CROSS_COMPILE)gcc
BUILD_DIR := build/atari
INCLUDE_DIR := $(BUILD_DIR)/include

CFLAGS ?= -O2 -g -Wall -Wextra
# Preserve the original kernel register ABI and its non-libc string routines.
# VFAT also views the same directory-entry memory through DIR and LDIR types.
KERNEL_FLAGS := -std=gnu11 -m68000 -mshort -mfastcall -ffreestanding -fno-builtin \
                -fno-strict-aliasing -Werror=attributes

HEADERS := $(addprefix $(INCLUDE_DIR)/,portab.h tos.h tosdefs.h magx.h \
           mgx_xfs.h mgx_dfs.h vfat_imp.h)

.PHONY: all vfat
all: magic
vfat: $(BUILD_DIR)/vfat.o

# Provide DOS-era lowercase include names without renaming the original files.
$(INCLUDE_DIR)/portab.h: MagiC/TOOLS/SHOWFREE/IMPORT/PORTAB.H
$(INCLUDE_DIR)/tos.h: base/applicat/SRC/IMPORT/TOS.H
$(INCLUDE_DIR)/tosdefs.h: MagiC/INC_C/TOSDEFS.H
$(INCLUDE_DIR)/magx.h: MagiC/INC_C/MAGX.H
$(INCLUDE_DIR)/mgx_xfs.h: MagiC/INC_C/MGX_XFS.H
$(INCLUDE_DIR)/mgx_dfs.h: MagiC/INC_C/MGX_DFS.H
$(INCLUDE_DIR)/vfat_imp.h: kernel/DOS/INC/VFAT_IMP.H
$(HEADERS):
	mkdir -p $(@D)
	ln -sf $(abspath $<) $@

$(BUILD_DIR)/vfat.o: kernel/DOS/compat/VFAT.C $(HEADERS)
	$(CC) $(CPPFLAGS) -I$(INCLUDE_DIR) $(CFLAGS) $(KERNEL_FLAGS) \
	  -MMD -MP -MF $@.dep -x c -c $< -o $@

$(BUILD_DIR)/vfat-check.o: kernel/DOS/compat/VFAT_CHECK.C $(HEADERS)
	$(CC) $(CPPFLAGS) -I$(INCLUDE_DIR) $(CFLAGS) $(KERNEL_FLAGS) \
	  -ffunction-sections -fdata-sections -MMD -MP -MF $@.dep -x c -c $< -o $@

-include $(BUILD_DIR)/vfat.o.dep $(BUILD_DIR)/vfat-check.o.dep
$(BUILD_DIR)/vfat.o $(BUILD_DIR)/vfat-check.o: Makefile

include kernel/mk/assembly.mk

.PHONY: filesystem
filesystem: $(BUILD_DIR)/filesystem.o

$(BUILD_DIR)/filesystem.o: $(BUILD_DIR)/vfat.o $(BUILD_DIR)/vfat-glue.o \
                          $(BUILD_DIR)/std.o $(BUILD_DIR)/xfs-vdos.o \
                          Makefile kernel/mk/assembly.mk
	$(CROSS_COMPILE)ld -r -o $@ $(filter %.o,$^)

include kernel/mk/bios-dos.mk
include kernel/mk/vdi.mk
include kernel/mk/aes-asm.mk
include kernel/mk/aes-c.mk

# Preserve the legacy M.INP module order, particularly BIOS first and ENDE last.
KERNEL_OBJECTS := \
  $(addprefix $(BIOSDOS_DIR)/,magibios.o xfs95ini.o ide.o read_inf.o) \
  $(VDI_BUILD)/math.o $(BUILD_DIR)/std.o $(BIOSDOS_DIR)/ser_my.o \
  $(addprefix $(BIOSDOS_DIR)/,magidos.o malloc.o) \
  $(BUILD_DIR)/xfs-vdos.o $(BUILD_DIR)/vfat-glue.o $(BUILD_DIR)/vfat.o \
  $(addprefix $(BIOSDOS_DIR)/,dfs_u.o dev_bios.o dev_pipe.o dev_mem.o dfs_fat.o) \
  $(filter-out $(VDI_BUILD)/math.o,$(VDI_OBJECTS)) \
  $(AES_ASM_OBJECTS) \
  $(addprefix $(AES_C_DIR)/,farbic.o wdialog.o listbox.o fnt_menu.o shelsort.o wdintrfc.o fsel.o) \
  $(AES_MTSMN_OBJECT) $(BIOSDOS_DIR)/ende.o

.PHONY: magic kernel-objects
magic: $(BUILD_DIR)/MAGIC.RAM
kernel-objects: $(KERNEL_OBJECTS)

$(BUILD_DIR)/kernel.elf: $(KERNEL_OBJECTS) kernel/mk/kernel.ld Makefile
	$(CROSS_COMPILE)ld -m m68kelf --emit-relocs -T kernel/mk/kernel.ld \
	  -Map=$(BUILD_DIR)/kernel.map -o $@ $(KERNEL_OBJECTS) \
	  $(shell $(CC) $(KERNEL_FLAGS) -print-libgcc-file-name)

$(BUILD_DIR)/MAGIC.RAM: $(BUILD_DIR)/kernel.elf kernel/tools/elf_to_magic.py
	python3 kernel/tools/elf_to_magic.py $< $@

$(KERNEL_OBJECTS): Makefile
$(AES_ASM_OBJECTS) $(AES_C_DIR)/wdintrfc.o: kernel/mk/assembly.mk
