CROSS = aarch64-none-elf-

AS = $(CROSS)as.exe
CC = $(CROSS)g++.exe
#LD = $(CROSS)ld.exe
LD = $(CC)
OBJCOPY = $(CROSS)objcopy.exe

CFLAGS = \
	-std=c++20 \
	-ffreestanding \
	-fno-stack-protector \
	-fno-exceptions \
	-fno-rtti \
	-fno-threadsafe-statics \
	-fno-use-cxa-atexit \
	-fno-unwind-tables \
	-fno-asynchronous-unwind-tables \
	-fno-pie \
	-mstrict-align \
	-Wall \
	-Wextra \
	-g \
	-nostdlib \
	-Iinclude \
	-I include/mstd \
	-Os \
	-mcpu=cortex-a53

ASFLAGS = -g

LDFLAGS = \
	-T linker.ld \
	-nostdlib \
	-flto \
	-static

BUILD = build
SRC = .
BOOT_SRC = boot


ASM_SRCS = $(wildcard $(SRC)/arch/*.S)
C_SRCS = $(wildcard $(SRC)/kernel/*.cpp) \
		$(wildcard $(SRC)/mstd/*.cpp) \
		$(wildcard $(SRC)/kernel/handler/*.cpp)

ASM_OBJS := $(patsubst $(SRC)/%.S,$(BUILD)/%.o,$(ASM_SRCS))
C_OBJS   := $(patsubst $(SRC)/%.cpp,$(BUILD)/%.o,$(C_SRCS))

OBJS = $(ASM_OBJS) $(C_OBJS)

TARGET = $(BUILD)/kernel.elf


all: $(TARGET)


$(TARGET): $(OBJS)
	$(LD) $(LDFLAGS) $(OBJS) -o $@


$(BUILD)/%.o: $(SRC)/%.S
	@mkdir -p $(dir $@)
	$(AS) $(ASFLAGS) -c $< -o $@

$(BUILD)/%.o: $(SRC)/%.cpp
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@


run: $(TARGET)
	qemu-system-aarch64 \
		-M virt \
		-cpu cortex-a53 \
		-m 512M \
		-nographic \
		-kernel $(TARGET)


clean:
	rm -rf $(BUILD)


.PHONY: all run clean