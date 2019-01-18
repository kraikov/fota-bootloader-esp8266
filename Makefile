Q := @

# Project specific folders
OBJECT_FOLDER           := obj
BIN_FOLDER              := bin
DRIVER_FOLDER           := driver
LD_SCRIPT_FOLDER        := ld

# Include directories
SDK_INCLUDE_DIR         := ../espressif/ESP8266_SDK/include
INCLUDE_DIR             := $(addprefix -I,/$(SDK_INCLUDE_DIR))
INCLUDE_DIR             += $(SDK_INCLUDE_DIR)

# Tools
XTENSA_TOOLS_ROOT       ?= ../espressif/xtensa-lx106-elf/bin
COMPILE                 := $(addprefix $(XTENSA_TOOLS_ROOT)/,xtensa-lx106-elf-gcc)
LINK                    := $(addprefix $(XTENSA_TOOLS_ROOT)/,xtensa-lx106-elf-gcc)
GEN_TOOL                ?= ../esptool/esptool2.exe
FLASH_TOOL              ?= ../esptool/esptool.exe

# Compiler/Linker options
CFLAGS    				= -Os -O3 -Wpointer-arith -Wundef -Werror -Wl,-EL -fno-inline-functions -nostdlib -mlongcalls -mtext-section-literals  -D__ets__ -DICACHE_FLASH
LDFLAGS   				= -nostdlib -Wl,--no-check-sections -u call_user_start -Wl,-static
LD_SCRIPT 				= $(LD_SCRIPT_FOLDER)/eagle.app.v6.ld
E2_OPTS 				= -quiet -bin -boot0
CFLAGS 					+= $(addprefix -I,$(INCLUDE_DIR))
CFLAGS 					+= $(addprefix -I,.)

# Output files
BIN_NAME				:= BootloaderDriver
APPLICATION_DIR			:= ../firmware_over_the_air/boot/

# Functions
.SECONDARY:

build: $(OBJECT_FOLDER) $(BIN_FOLDER) $(BIN_FOLDER)/$(BIN_NAME).bin

$(OBJECT_FOLDER):
	$(Q) mkdir -p $@

$(BIN_FOLDER):
	$(Q) mkdir -p $@

$(OBJECT_FOLDER)/Loader.o: $(DRIVER_FOLDER)/Loader.c $(DRIVER_FOLDER)/LoaderConfiguration.h $(DRIVER_FOLDER)/$(BIN_NAME).h
	@echo "COMPILE $(notdir $<)"
	$(Q) $(COMPILE) $(CFLAGS) -c $< -o $@

$(OBJECT_FOLDER)/Loader.elf: $(OBJECT_FOLDER)/Loader.o
	@echo "LINK $(notdir $@)"
	$(Q) $(LINK) -T$(LD_SCRIPT_FOLDER)/Loader.ld $(LDFLAGS) -Wl,--start-group $^ -Wl,--end-group -o $@

$(OBJECT_FOLDER)/Loader.h: $(OBJECT_FOLDER)/Loader.elf
	$(Q) $(GEN_TOOL) -quiet -header $< $@ .text

$(OBJECT_FOLDER)/$(BIN_NAME).o: $(DRIVER_FOLDER)/$(BIN_NAME).c $(DRIVER_FOLDER)/LoaderConfiguration.h $(DRIVER_FOLDER)/$(BIN_NAME).h $(OBJECT_FOLDER)/Loader.h
	@echo "COMPILE $(notdir $<)"
	$(Q) $(COMPILE) $(CFLAGS) -I$(OBJECT_FOLDER) -c $< -o $@

$(OBJECT_FOLDER)/%.o: %.c %.h
	@echo "COMPILE $(notdir $<)"
	$(Q) $(COMPILE) $(CFLAGS) -c $< -o $@

$(OBJECT_FOLDER)/%.elf: $(OBJECT_FOLDER)/%.o
	@echo "LINK $(notdir $@)"
	$(Q) $(LINK) -T$(LD_SCRIPT) $(LDFLAGS) -Wl,--start-group $^ -Wl,--end-group -o $@

$(BIN_FOLDER)/%.bin: $(OBJECT_FOLDER)/%.elf
	@echo "GEN $(notdir $@)"
	$(Q) $(GEN_TOOL) $(E2_OPTS) $< $@ .text .rodata
	$(Q) cp -f $(BIN_FOLDER)/$(BIN_NAME).bin $(APPLICATION_DIR)

clean:
	@echo "Cleaning..."
	$(Q) rm -rf $(OBJECT_FOLDER)
	$(Q) rm -rf $(BIN_FOLDER)
	@echo "Done"


flash:
	$(FLASH_TOOL) --port COM7 write_flash -fs 4MB 0x000000 $(BIN_FOLDER)/$(BIN_NAME).bin


erase:
	$(FLASH_TOOL) --port COM7 erase_flash