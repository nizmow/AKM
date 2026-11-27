.DEFAULT_GOAL := all

# Kernel localisation.  de keeps the original German kernel (COUNTRY=1/FRG).
# en selects the original English kernel strings, AES alerts and file/font
# selector resources with the original English country code (COUNTRY=0/USA).
# The kernel has no COUNTRY-selected keyboard table: KEYB.S ships a German
# (GER) default and boot replaces all nine tables from
# GEMSYS/MAGIC/XTENSION/KEYTABLS.SYS when present, so a UK layout comes from
# that file rather than from the kernel build.  COUNTRY=3 (UK) is not a
# complete kernel build; only the BIOS and COOK.S have UK text branches.
KERNEL_LANG ?= de

ifeq ($(KERNEL_LANG),de)
KERNEL_COUNTRY   := 1
KERNEL_LANG_SLUG :=
KERNEL_DEFAULT_BUILD_DIR := build/atari
else ifeq ($(KERNEL_LANG),en)
KERNEL_COUNTRY   := 0
KERNEL_LANG_SLUG := -en
KERNEL_DEFAULT_BUILD_DIR := build/atari/en
else
$(error KERNEL_LANG must be de or en, got '$(KERNEL_LANG)')
endif

CROSS_COMPILE ?= m68k-atari-mintelf-
CC := $(CROSS_COMPILE)gcc
BUILD_DIR ?= $(KERNEL_DEFAULT_BUILD_DIR)
INCLUDE_DIR := $(BUILD_DIR)/include
# Country-dependent kernel objects live in a per-language tree.  The German
# tree keeps its historic paths; another language gets a sibling tree, so
# switching KERNEL_LANG inside one BUILD_DIR cannot reuse the other language's
# objects.  Country-independent objects (VFAT, STD, XFS_VDOS, VDI) stay shared.
KERNEL_OBJ_ROOT := $(BUILD_DIR)/kernel$(KERNEL_LANG_SLUG)

# The final MAGIC.RAM has one name per BUILD_DIR.  If both languages are built
# in the same BUILD_DIR, a switch must relink the final image even though the
# per-language object trees are still valid.  This stamp's timestamp changes
# only when the requested language differs from the last build in that tree.
KERNEL_LANG_STAMP := $(BUILD_DIR)/.kernel-lang

.PHONY: FORCE
FORCE:

$(KERNEL_LANG_STAMP): FORCE
	@mkdir -p $(@D)
	@printf '%s\n' '$(KERNEL_LANG)' > $@.tmp
	@cmp -s $@.tmp $@ || mv -f $@.tmp $@
	@rm -f $@.tmp

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
$(INCLUDE_DIR)/portab.h: kernel/INC/PORTAB.H
$(INCLUDE_DIR)/tos.h: base/applicat/SRC/IMPORT/TOS.H
$(INCLUDE_DIR)/tosdefs.h: kernel/INC/TOSDEFS.H
$(INCLUDE_DIR)/magx.h: kernel/INC/MAGX.H
$(INCLUDE_DIR)/mgx_xfs.h: kernel/INC/MGX_XFS.H
$(INCLUDE_DIR)/mgx_dfs.h: kernel/INC/MGX_DFS.H
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

$(BUILD_DIR)/kernel.elf: $(KERNEL_OBJECTS) kernel/mk/kernel.ld Makefile \
                         $(KERNEL_LANG_STAMP)
	$(CROSS_COMPILE)ld -m m68kelf --emit-relocs -T kernel/mk/kernel.ld \
	  -Map=$(BUILD_DIR)/kernel.map -o $@ $(KERNEL_OBJECTS) \
	  $(shell $(CC) $(KERNEL_FLAGS) -print-libgcc-file-name)

$(BUILD_DIR)/MAGIC.RAM: $(BUILD_DIR)/kernel.elf kernel/tools/elf_to_magic.py
	python3 kernel/tools/elf_to_magic.py $< $@

$(KERNEL_OBJECTS): Makefile
$(AES_ASM_OBJECTS) $(AES_C_DIR)/wdintrfc.o: kernel/mk/assembly.mk
