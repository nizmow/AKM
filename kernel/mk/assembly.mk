# MAS source normalization preserves instructions and original source files.
VASM ?= vasmm68k_mot
VASM_FLAGS := -m68000 -spaces -Felf -quiet -I$(abspath $(BUILD_DIR)/asm)
ASM_ADAPTER := scripts/mas_to_vasm.py
ASM_DIR := $(BUILD_DIR)/asm
ASM_HEADERS := $(addprefix $(ASM_DIR)/,ERRNO.INC KERNEL.INC STRUCTS.INC DOS.INC DEBUG.INC)
ASM_SOURCES := $(addprefix $(ASM_DIR)/,VFAT_S.S STD.S XFS_VDOS.S)
ASM_OBJECTS := $(BUILD_DIR)/vfat-glue.o $(BUILD_DIR)/std.o $(BUILD_DIR)/xfs-vdos.o
# Split only complete utility routines, including boundaries after those used by
# VFAT. This lets a freestanding regression link discard unrelated trap wrappers.
STD_SECTIONS := fast_clrmem,memcpy,memmove,toupper,strcpy,strcmp,stricmp,strlen,strchr,strrchr,strcat,ext_8_3,int_8_3

.PHONY: asm-objects
asm-objects: $(ASM_OBJECTS)

$(ASM_DIR)/ERRNO.INC: MagiC/INC_AS/ERRNO.INC
$(ASM_DIR)/KERNEL.INC: MagiC/KERNEL/INC/KERNEL.INC
$(ASM_DIR)/STRUCTS.INC: kernel/DOS/INC/STRUCTS.INC
$(ASM_DIR)/DOS.INC: MagiC/KERNEL/INC/DOS.INC
$(ASM_DIR)/DEBUG.INC: kernel/DOS/INC/DEBUG.INC
$(ASM_DIR)/VFAT_S.S: kernel/DOS/VFAT_S.S
$(ASM_DIR)/STD.S: MagiC/KERNEL/ALLG/SRC/STD.S
$(ASM_DIR)/XFS_VDOS.S: kernel/DOS/XFS_VDOS.S

$(ASM_HEADERS) $(ASM_SOURCES): $(ASM_ADAPTER) kernel/mk/assembly.mk
	python3 $(ASM_ADAPTER) $(if $(filter STD.S,$(notdir $@)),--sections $(STD_SECTIONS)) \
	  $(filter-out $(ASM_ADAPTER) kernel/mk/assembly.mk,$^) $@

$(BUILD_DIR)/vfat-glue.o: kernel/DOS/compat/VFAT_ASM.S $(ASM_DIR)/VFAT_S.S $(ASM_HEADERS)
	$(VASM) $(VASM_FLAGS) -o $@ $<

$(BUILD_DIR)/std.o: $(ASM_DIR)/STD.S $(ASM_HEADERS)
	$(VASM) $(VASM_FLAGS) -o $@ $<

$(BUILD_DIR)/xfs-vdos.o: $(ASM_DIR)/XFS_VDOS.S $(ASM_HEADERS)
	$(VASM) $(VASM_FLAGS) -o $@ $<

$(ASM_OBJECTS): Makefile kernel/mk/assembly.mk
