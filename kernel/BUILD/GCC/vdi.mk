# MagiC VDI kernel objects (Atari/German build).
# Included from Makefile.atari after the shared kernel and assembly rules.
#
# The generated assembly tree is kept per slice so the VDI INCLUDE\*.I
# namespace cannot collide with the filesystem build.

VDI_SRC := kernel/VDI/SRC/MAGICVDI.2
VDI_BUILD := $(BUILD_DIR)/kernel/vdi
VDI_ASM := $(VDI_BUILD)/asm
VDI_INCLUDE := $(VDI_BUILD)/include
VDI_COMPAT := kernel/BUILD/GCC/vdi-include
VDI_ASM_ADAPTER ?= kernel/BUILD/GCC/mas_to_vasm.py
VDI_ATARI_UTF8 := NON-TOS/atari-utf8-conversion/atari-utf8.c

VDI_CPPFLAGS := -I$(VDI_INCLUDE) -I$(VDI_COMPAT) -I$(VDI_SRC)
VDI_CFLAGS := $(CFLAGS) $(KERNEL_FLAGS) $(VDI_CPPFLAGS)

VDI_VASM ?= vasmm68k_mot
VDI_VASM_FLAGS := -m68000 -spaces -Felf -quiet -I$(abspath $(VDI_ASM))

# MXVDIKNL.S textually includes the other functional parts.  They are
# generated, never assembled as separate objects.
VDI_ASM_TOP := MXVDIKNL.S MATH.S STRING.S TOS.S
VDI_ASM_PARTS := FONTS.S INQUIRE.S INPUT.S RASTER.S ATTRIBUT.S OUTPUT.S \
                 CONTROL.S TEXT_BMP.S LINEA.S ESCAPE.S DOS.S DISP.S
VDI_ASM_HEADERS := NVDI_DIV.I MEMORY.I TOS.I VDI.I LINEA.I HARDWARE.I \
                   DRIVER.I SEEDFILL.I NVDI_WK.I PIXMAP.I MXVDI.I AESVARS.I

VDI_GEN_ASM := $(addprefix $(VDI_ASM)/,$(VDI_ASM_TOP) $(VDI_ASM_PARTS))
VDI_GEN_HEADERS := $(addprefix $(VDI_ASM)/,$(VDI_ASM_HEADERS))

VDI_C_HEADERS := $(VDI_SRC)/LANGUAGE.H $(VDI_SRC)/MEMORY.H \
                 $(VDI_SRC)/STRING.H $(VDI_SRC)/TOS.H $(VDI_SRC)/DRIVERS.H \
                 $(VDI_SRC)/NVDI_WK.H $(VDI_SRC)/VDI.H $(VDI_SRC)/FILEDIV.H \
                 $(VDI_SRC)/INIT_RES.H $(VDI_SRC)/MXVDI.H $(VDI_SRC)/TARGET.H \
                 $(VDI_SRC)/PIXMAP.H $(VDI_COMPAT)/MAC/QDRAW.H \
                 $(VDI_COMPAT)/Types2B.h $(VDI_COMPAT)/Colors2B.h

VDI_OBJECTS := $(VDI_BUILD)/math.o $(VDI_BUILD)/mxvdiknl.o \
               $(VDI_BUILD)/setup.o $(VDI_BUILD)/drivers.o \
               $(VDI_BUILD)/filediv.o $(VDI_BUILD)/init_res.o \
               $(VDI_BUILD)/string.o $(VDI_BUILD)/tos.o

.PHONY: vdi-objects
vdi-objects: $(VDI_OBJECTS)

# Uppercase TOS-era include names are resolved without touching the sources.
PORTAB_H := MagiC/TOOLS/SHOWFREE/IMPORT/PORTAB.H
$(VDI_INCLUDE)/portab.h: $(PORTAB_H)
	mkdir -p $(@D)
	ln -sf $(abspath $<) $@

$(VDI_INCLUDE)/PORTAB.h: $(PORTAB_H)
	mkdir -p $(@D)
	ln -sf $(abspath $<) $@

# DRIVERS.C/INIT_RES.C use <PORTAB.h>, FILEDIV.C uses <PORTAB.H>.
$(VDI_INCLUDE)/PORTAB.H: $(PORTAB_H)
	mkdir -p $(@D)
	ln -sf $(abspath $<) $@

# INIT_RES.H includes "MXVDI.h"; the archived name is MXVDI.H.
$(VDI_INCLUDE)/MXVDI.h: $(VDI_SRC)/MXVDI.H
	mkdir -p $(@D)
	ln -sf $(abspath $<) $@

$(VDI_INCLUDE)/target.h: $(VDI_SRC)/TARGET.H
	mkdir -p $(@D)
	ln -sf $(abspath $<) $@

$(VDI_ASM)/%.S: $(VDI_SRC)/%.S $(VDI_ASM_ADAPTER) $(VDI_ATARI_UTF8)
	mkdir -p $(@D)
	python3 $(VDI_ASM_ADAPTER) --atari-strings $< $@

$(VDI_ASM)/%.I: $(VDI_SRC)/INCLUDE/%.I $(VDI_ASM_ADAPTER) $(VDI_ATARI_UTF8)
	mkdir -p $(@D)
	python3 $(VDI_ASM_ADAPTER) --atari-strings $< $@

$(VDI_BUILD)/drivers.o: $(VDI_SRC)/DRIVERS.C
$(VDI_BUILD)/filediv.o: $(VDI_SRC)/FILEDIV.C
$(VDI_BUILD)/init_res.o: $(VDI_SRC)/INIT_RES.C
$(VDI_BUILD)/setup.o: $(VDI_SRC)/SETUP.C
$(VDI_BUILD)/drivers.o $(VDI_BUILD)/filediv.o $(VDI_BUILD)/init_res.o \
    $(VDI_BUILD)/setup.o: $(VDI_C_HEADERS) \
                  $(VDI_INCLUDE)/portab.h $(VDI_INCLUDE)/PORTAB.h \
                  $(VDI_INCLUDE)/PORTAB.H $(VDI_INCLUDE)/target.h \
                  $(VDI_INCLUDE)/MXVDI.h
	@mkdir -p $(@D)
	$(CC) $(VDI_CFLAGS) -MMD -MP -MF $@.d -x c -c $(filter %.C,$^) -o $@

$(VDI_BUILD)/mxvdiknl.o: $(VDI_ASM)/MXVDIKNL.S $(VDI_GEN_ASM) $(VDI_GEN_HEADERS)
	@mkdir -p $(@D)
	$(VDI_VASM) $(VDI_VASM_FLAGS) -o $@ $<

$(VDI_BUILD)/math.o: $(VDI_ASM)/MATH.S $(VDI_GEN_HEADERS)
	@mkdir -p $(@D)
	$(VDI_VASM) $(VDI_VASM_FLAGS) -o $@ $<

$(VDI_BUILD)/string.o: $(VDI_ASM)/STRING.S $(VDI_GEN_HEADERS)
	@mkdir -p $(@D)
	$(VDI_VASM) $(VDI_VASM_FLAGS) -o $@ $<

$(VDI_BUILD)/tos.o: $(VDI_ASM)/TOS.S $(VDI_GEN_HEADERS)
	@mkdir -p $(@D)
	$(VDI_VASM) $(VDI_VASM_FLAGS) -o $@ $<

$(VDI_OBJECTS): Makefile.atari kernel/BUILD/GCC/assembly.mk \
                kernel/BUILD/GCC/vdi.mk

-include $(VDI_OBJECTS:.o=.o.d)
