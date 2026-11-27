.DEFAULT_GOAL := all

# Kernel localisation.  Three independent selectors:
#   KERNEL_LANG      de|en    interface language (BIOS/GEMDOS/AES messages)
#   KERNEL_REGION    de|uk|us machine country: BIOS header COUNTRY, COOK _IDT
#   KERNEL_KEYBOARD  de|uk    built-in BIOS keyboard tables; an external
#                             full 9x128 GEMSYS/MAGIC/XTENSION/KEYTABLS.SYS
#                             replaces them, otherwise they remain the default
# Region and keyboard default to the language's home country: de/de for German,
# uk/uk for English.  KERNEL_LANGUAGE, KERNEL_COUNTRY and
# KERNEL_KEYBOARD_COUNTRY carry the numbers understood by the original sources
# (COUNTRY and KEYBOARD: 0 USA, 1 FRG, 3 UK).
KERNEL_LANG ?= de

ifeq ($(KERNEL_LANG),de)
KERNEL_LANGUAGE      := 1
KERNEL_DEFAULT_BUILD_DIR := build/atari
KERNEL_LANG_DEFAULT_REGION   := de
KERNEL_LANG_DEFAULT_KEYBOARD := de
else ifeq ($(KERNEL_LANG),en)
KERNEL_LANGUAGE      := 0
KERNEL_DEFAULT_BUILD_DIR := build/atari/en
KERNEL_LANG_DEFAULT_REGION   := uk
KERNEL_LANG_DEFAULT_KEYBOARD := uk
else
$(error KERNEL_LANG must be de or en, got '$(KERNEL_LANG)')
endif

KERNEL_REGION   ?= $(KERNEL_LANG_DEFAULT_REGION)
KERNEL_KEYBOARD ?= $(KERNEL_LANG_DEFAULT_KEYBOARD)

ifeq ($(KERNEL_REGION),de)
KERNEL_COUNTRY := 1
else ifeq ($(KERNEL_REGION),uk)
KERNEL_COUNTRY := 3
else ifeq ($(KERNEL_REGION),us)
KERNEL_COUNTRY := 0
else
$(error KERNEL_REGION must be de, uk or us, got '$(KERNEL_REGION)')
endif

ifeq ($(KERNEL_KEYBOARD),de)
KERNEL_KEYBOARD_COUNTRY := 1
else ifeq ($(KERNEL_KEYBOARD),uk)
KERNEL_KEYBOARD_COUNTRY := 3
else
$(error KERNEL_KEYBOARD must be de or uk, got '$(KERNEL_KEYBOARD)')
endif

CROSS_COMPILE ?= m68k-atari-mintelf-
CC := $(CROSS_COMPILE)gcc
BUILD_DIR ?= $(KERNEL_DEFAULT_BUILD_DIR)
INCLUDE_DIR := $(BUILD_DIR)/include
# Every locale selection that changes object bytes is visible in the object
# tree path, so switching selectors inside one explicit BUILD_DIR cannot reuse
# objects built for another selection.  The default German tree keeps the
# historic $(BUILD_DIR)/kernel paths; every other combination gets a
# -lang-region-keyboard sibling.  Country-independent objects (VFAT, STD,
# XFS_VDOS, VDI) stay shared.
KERNEL_LOCALE := $(KERNEL_LANG)-$(KERNEL_REGION)-$(KERNEL_KEYBOARD)
KERNEL_LOCALE_SLUG := $(if $(filter de-de-de,$(KERNEL_LOCALE)),,-$(KERNEL_LOCALE))
KERNEL_OBJ_ROOT := $(BUILD_DIR)/kernel$(KERNEL_LOCALE_SLUG)

# The final MAGIC.RAM has one name per BUILD_DIR.  If several locale selections
# are built in the same BUILD_DIR, a switch must relink the final image even
# though the per-locale object trees are still valid.  This stamp changes only
# when the selection differs from the last build in that tree, so an unchanged
# selection does not relink.
KERNEL_LOCALE_STAMP := $(BUILD_DIR)/.kernel-locale

.PHONY: FORCE
FORCE:

$(KERNEL_LOCALE_STAMP): FORCE
	@mkdir -p $(@D)
	@printf '%s\n' 'lang=$(KERNEL_LANG) region=$(KERNEL_REGION) keyboard=$(KERNEL_KEYBOARD)' > $@.tmp
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
                         $(KERNEL_LOCALE_STAMP)
	$(CROSS_COMPILE)ld -m m68kelf --emit-relocs -T kernel/mk/kernel.ld \
	  -Map=$(BUILD_DIR)/kernel.map -o $@ $(KERNEL_OBJECTS) \
	  $(shell $(CC) $(KERNEL_FLAGS) -print-libgcc-file-name)

$(BUILD_DIR)/MAGIC.RAM: $(BUILD_DIR)/kernel.elf kernel/tools/elf_to_magic.py
	python3 kernel/tools/elf_to_magic.py $< $@

$(KERNEL_OBJECTS): Makefile
$(AES_ASM_OBJECTS) $(AES_C_DIR)/wdintrfc.o: kernel/mk/assembly.mk
