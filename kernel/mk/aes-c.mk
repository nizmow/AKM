# AES/SRC C modules compiled with the MagiC kernel ABI.
# Original sources stay in place; wrappers add the attributes and local
# standard-library bindings that Pure C supplied from its GEMLIB.
AES_C_DIR := $(BUILD_DIR)/kernel/aes-c
AES_C_WRAP_DIR := kernel/AES/compat
AES_C_INCLUDE := $(abspath $(AES_C_WRAP_DIR)/include)
AES_C_ASM_DIR := $(AES_C_DIR)/asm
AES_C_ADAPTER := $(ASM_ADAPTER)
AES_C_VASM := $(if $(VASM),$(VASM),vasmm68k_mot)

AES_C_OBJECTS := $(addprefix $(AES_C_DIR)/, \
    farbic.o fsel.o wdialog.o listbox.o fnt_menu.o shelsort.o wdintrfc.o)

AES_C_HEADERS := $(addprefix $(AES_C_INCLUDE)/, \
    portab.h country.h tos.h tosdefs.h mgx_dos.h vdi.h aes.h string.h)

AES_C_CFLAGS := $(CPPFLAGS) -I$(AES_C_INCLUDE) -DCOUNTRY=1 $(CFLAGS) $(KERNEL_FLAGS)
# WDIALOG sources are AtariST/CP437 text.  They contain no non-ASCII string
# literals, so reading bytes as ISO-8859-1 is lossless for their code.
AES_C_LEGACY_CFLAGS := $(AES_C_CFLAGS) \
    -finput-charset=ISO-8859-1 -fexec-charset=ISO-8859-1

.PHONY: aes-c-objects
aes-c-objects: $(AES_C_OBJECTS)

$(AES_C_DIR)/farbic.o: $(AES_C_WRAP_DIR)/AES_FARBIC.C \
                       kernel/AES/SRC/FARBIC.C $(AES_C_HEADERS)
	@mkdir -p $(@D)
	$(CC) $(AES_C_CFLAGS) -MMD -MP -MF $@.dep -x c -c $< -o $@

$(AES_C_DIR)/fsel.o: $(AES_C_WRAP_DIR)/AES_FSEL.C \
                     kernel/AES/SRC/FSEL/FSEL.C \
                     kernel/AES/SRC/FSEL/FSEL.H \
                     kernel/AES/SRC/FSEL/KER_BIND.H \
                     kernel/AES/SRC/FSEL/LISTBX_G.H \
                     kernel/AES/SRC/FSEL/GER/FSELX.H \
                     kernel/AES/SRC/FSEL/GER/FSELRSRC.H \
                     kernel/AES/SRC/WDIALOG/SHELSORT.H \
                     $(AES_C_HEADERS)
	@mkdir -p $(@D)
	$(CC) $(AES_C_CFLAGS) -MMD -MP -MF $@.dep -x c -c $< -o $@

$(AES_C_DIR)/wdialog.o: $(AES_C_WRAP_DIR)/AES_WDIALOG.C \
                        kernel/AES/SRC/WDIALOG/WDIALOG.C \
                        kernel/AES/SRC/WDIALOG/WDIALOG.H \
                        $(AES_C_HEADERS)
	@mkdir -p $(@D)
	$(CC) $(AES_C_LEGACY_CFLAGS) -MMD -MP -MF $@.dep -x c -c $< -o $@

$(AES_C_DIR)/listbox.o: $(AES_C_WRAP_DIR)/AES_LISTBOX.C \
                        kernel/AES/SRC/WDIALOG/LISTBOX.C \
                        kernel/AES/SRC/WDIALOG/WDIALOG.H \
                        kernel/AES/SRC/WDIALOG/LISTBOX.H \
                        $(AES_C_HEADERS)
	@mkdir -p $(@D)
	$(CC) $(AES_C_LEGACY_CFLAGS) -MMD -MP -MF $@.dep -x c -c $< -o $@

$(AES_C_DIR)/fnt_menu.o: $(AES_C_WRAP_DIR)/AES_FNT_MENU.C \
                         kernel/AES/SRC/WDIALOG/FNT_MENU.C \
                         kernel/AES/SRC/WDIALOG/VDI_BIND.C \
                         kernel/AES/SRC/WDIALOG/VDI_BIND.H \
                         kernel/AES/SRC/WDIALOG/OBJ_TOOL.C \
                         kernel/AES/SRC/WDIALOG/OBJ_TOOL.H \
                         kernel/AES/SRC/WDIALOG/FNTS.H \
                         kernel/AES/SRC/WDIALOG/LISTBOX.H \
                         kernel/AES/SRC/WDIALOG/WDIALOG.H \
                         kernel/AES/SRC/WDIALOG/GER/FONTSLCT.H \
                         kernel/AES/SRC/WDIALOG/GER/FNTS_RSC.H \
                         $(AES_C_HEADERS)
	@mkdir -p $(@D)
	$(CC) $(AES_C_LEGACY_CFLAGS) -MMD -MP -MF $@.dep -x c -c $< -o $@

$(AES_C_DIR)/shelsort.o: $(AES_C_WRAP_DIR)/AES_SHELSORT.C \
                         kernel/AES/SRC/WDIALOG/SHELSORT.C \
                         $(AES_C_HEADERS)
	@mkdir -p $(@D)
	$(CC) $(AES_C_LEGACY_CFLAGS) -MMD -MP -MF $@.dep -x c -c $< -o $@

# WDINTRFC.S keeps its Motorola instructions; the shared normalizer only
# rewrites MAS directives, comments and include/string syntax.
$(AES_C_ASM_DIR)/WDINTRFC.S: kernel/AES/SRC/WDIALOG/WDINTRFC.S \
                             $(AES_C_ADAPTER)
	@mkdir -p $(@D)
	python3 $(AES_C_ADAPTER) --atari-strings $< $@

$(AES_C_DIR)/wdintrfc.o: $(AES_C_ASM_DIR)/WDINTRFC.S \
                         kernel/AES/SRC/WDIALOG/WDIALOG.I \
                         kernel/AES/SRC/WDIALOG/LISTBOX.I \
                         kernel/AES/SRC/WDIALOG/FNTS.I
	@mkdir -p $(@D)
	$(AES_C_VASM) $(VASM_FLAGS) -I$(abspath kernel/AES/SRC/WDIALOG) -o $@ $<

-include $(addsuffix .dep,$(AES_C_OBJECTS))
$(AES_C_OBJECTS): Makefile kernel/mk/aes-c.mk
