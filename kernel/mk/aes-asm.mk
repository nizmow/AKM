# German and English MagiC AES assembly modules and the XMEN_MGR C module.
#
# The MAS sources are normalized by the shared adapter into a per-module
# include tree below $(AES_ASM_DIR) inside the per-locale $(KERNEL_OBJ_ROOT);
# the original files are never modified.
# Each module gets its own directory because AESINC.S, DEBUG.INC and the
# include set differ per module (AESMAIN also pulls VTSYS.INC and DEBUG.INC,
# AESOBJ pulls FARBICON.S).
#
# AESMAIN references fsel_exinput. It is provided by FSEL.C in this build
# (the C file-selector module); AESFSL.S is the legacy assembly alternative
# and must not be linked at the same time (duplicate fsel_exinput).
#
# Build configuration comes from the original Atari build:
#   COUNTRY    the AES language, from KERNEL_LANGUAGE: 1 German (FRG),
#              0 English (USA).  It is deliberately not KERNEL_COUNTRY: the
#              AES COUNTRY is the interface language reported by appl_getinfo(3),
#              while KERNEL_COUNTRY is the BIOS machine region.
#   DEMO=0     full AES (not the 6-application demo build)
#   MMX_YIELD=0 MacX multitasking helper not linked in this build
AES_ASM_MODULES := AESMAIN AESEVT AESFRM AESOBJ AESWIN AESMEN AESGRAF AESRSC

AES_ASM_DIR := $(KERNEL_OBJ_ROOT)/aes-asm
AES_ASM_DEFS := -DDEMO=0 -DMMX_YIELD=0 -DCOUNTRY=$(KERNEL_LANGUAGE)
AES_ASM_SHARED := AESINC.S KERNEL.INC DOS.INC
# The adapter reads the canonical Atari character table to encode UTF-8 string
# literals (GERMAN uses ue/oe/ae/ss and the French AES strings use accents).
AES_ATARI_UTF8 := scripts/atari-text/atari-utf8.c
AES_NORMALIZE := python3 $(ASM_ADAPTER) --atari-strings

# Module-specific generated includes.
AES_ASM_DEPS_AESMAIN := DEBUG.INC VTSYS.INC
AES_ASM_DEPS_AESOBJ := FARBICON.S

AES_ASM_OBJECTS := $(addprefix $(AES_ASM_DIR)/,$(addsuffix .o,$(AES_ASM_MODULES)))
AES_MTSMN_OBJECT := $(AES_ASM_DIR)/mtsmn.o
AES_OBJECTS := $(AES_ASM_OBJECTS) $(AES_MTSMN_OBJECT)

.PHONY: aes-asm aes-asm-objects
aes-asm: aes-asm-objects
aes-asm-objects: $(AES_OBJECTS)

# Normalized module sources: one directory per module.
define AES_ASM_SOURCE_RULE
$(AES_ASM_DIR)/$(1)/$(1).S: kernel/AES/SRC/$(1).S $(ASM_ADAPTER) \
                             $(AES_ATARI_UTF8) kernel/mk/aes-asm.mk
	$(AES_NORMALIZE) $$< $$@
endef
$(foreach m,$(AES_ASM_MODULES),$(eval $(call AES_ASM_SOURCE_RULE,$(m))))

# Normalized includes. The same rules populate every module directory; the
# adapter rewrites INCLUDE to a bare uppercase basename, so the directory is
# selected with -I when assembling.
$(AES_ASM_DIR)/%/AESINC.S: kernel/AES/SRC/AESINC.S $(ASM_ADAPTER) $(AES_ATARI_UTF8)
	$(AES_NORMALIZE) $< $@

$(AES_ASM_DIR)/%/KERNEL.INC: kernel/INC/KERNEL.INC $(ASM_ADAPTER) $(AES_ATARI_UTF8)
	$(AES_NORMALIZE) $< $@

$(AES_ASM_DIR)/%/DOS.INC: kernel/INC/DOS.INC $(ASM_ADAPTER) $(AES_ATARI_UTF8)
	$(AES_NORMALIZE) $< $@

$(AES_ASM_DIR)/%/DEBUG.INC: kernel/AES/SRC/DEBUG.INC $(ASM_ADAPTER) $(AES_ATARI_UTF8)
	$(AES_NORMALIZE) $< $@

$(AES_ASM_DIR)/%/VTSYS.INC: kernel/INC/VTSYS.INC $(ASM_ADAPTER) $(AES_ATARI_UTF8)
	$(AES_NORMALIZE) $< $@

$(AES_ASM_DIR)/%/FARBICON.S: kernel/AES/SRC/FARBICON.S $(ASM_ADAPTER) $(AES_ATARI_UTF8)
	$(AES_NORMALIZE) $< $@

define AES_ASM_OBJECT_RULE
$(AES_ASM_DIR)/$(1).o: $(AES_ASM_DIR)/$(1)/$(1).S \
                       $(addprefix $(AES_ASM_DIR)/$(1)/,$(AES_ASM_SHARED) $(AES_ASM_DEPS_$(1))) \
                       kernel/mk/aes-asm.mk
	$(VASM) $(VASM_FLAGS) $(AES_ASM_DEFS) -I$(AES_ASM_DIR)/$(1) -o $$@ $$<
endef
$(foreach m,$(AES_ASM_MODULES),$(eval $(call AES_ASM_OBJECT_RULE,$(m))))

# XMEN_MGR is C source compiled with the kernel register ABI. The source uses
# the original `cdecl` keyword on its AES callbacks; Borland's PORTAB.H does
# not define it, so map it to the GCC attribute the rest of the kernel uses.
AES_MTSMN_CPPFLAGS := -D'cdecl=__attribute__((cdecl))'

$(AES_ASM_DIR)/mtsmn.o: kernel/AES/SRC/XMEN_MGR/MTSMN.C \
                        $(INCLUDE_DIR)/portab.h kernel/mk/aes-asm.mk
	$(CC) $(CPPFLAGS) $(AES_MTSMN_CPPFLAGS) -I$(INCLUDE_DIR) $(CFLAGS) \
	  $(KERNEL_FLAGS) -MMD -MP -MF $@.dep -x c -c $< -o $@

-include $(addsuffix .dep,$(AES_MTSMN_OBJECT))
