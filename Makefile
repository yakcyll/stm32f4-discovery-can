TCPREFIX = /usr/bin/arm-none-eabi-
AS		 = $(TCPREFIX)as
CC       = $(TCPREFIX)gcc
LD       = $(TCPREFIX)ld -v
CP       = $(TCPREFIX)objcopy
OD       = $(TCPREFIX)objdump
GDB		 = $(TCPREFIX)gdb
GDBTUI   = $(TCPREFIX)gdb -tui

OPENOCD  = openocd
OPENOCDPID = "$(OPENOCD).pid"

CUBE_PATH = /opt/stm32cubef4
DRV_PATH = $(CUBE_PATH)/Drivers

BSP_PATH = $(DRV_PATH)/BSP/STM32F4-Discovery
CMSIS_PATH = $(DRV_PATH)/CMSIS
HAL_PATH = $(DRV_PATH)/STM32F4xx_HAL_Driver

BSP_INC	 = -I$(BSP_PATH)
CMSIS_INC = -I$(CMSIS_PATH)/Device/ST/STM32F4xx/Include -I$(CMSIS_PATH)/Include
HAL_INC	 = -I$(HAL_PATH)/Inc

INC		 = $(CMSIS_INC) $(BSP_INC) $(HAL_INC) -I.
LIB		 = -L/usr/arm-none-eabi/lib

HAL_FILES = $(wildcard $(HAL_PATH)/Src/*.c)
HAL_OBJ_FILES = $(addprefix haldrv/,$(notdir $(HAL_FILES:.c=.o)))
HAL_DRV_PATH = ./haldrv

CFLAGS   = $(INC) -c -fno-common -O0 -g -mcpu=cortex-m4 -mthumb -DSTM32F407xx -mtune=cortex-m4 -ansi -std=gnu99 -Wall
ASFLAGS	 = -mcpu=cortex-m4
LFLAGS   = -Tstm32.ld -nostartfiles --cref $(LIB)
CPFLAGS  = -Obinary
ODFLAGS  = -S

.PHONY: haldrvdir

all: compile

clean:
	rm -f main.lst *.o *.a main.elf main.lst main.bin

distclean:
	rm -f main.lst *.o *.a main.elf main.lst main.bin
	rm -rf $(HAL_DRV_PATH)

compile: main.bin

flash: main.bin
	$(OPENOCD) -f board/stm32f4discovery.cfg \
			   -c "program main.bin verify reset exit 0x08000000"

run: main.bin
	$(OPENOCD) -f board/stm32f4discovery.cfg \
		       -c "program main.bin verify reset exit 0x08000000"
	$(OPENOCD) -f board/stm32f4discovery.cfg &> openocd.log &
	echo $! > $(OPENOCDPID)
	$(GDB)     -ex "target remote localhost:3333" \ 
			   -ex "set remote hardware-breakpoint-limit 6" \ 
			   -ex "set remote hardware-watchpoint-limit 4" main.elf
	kill $(<"$(OPENOCDPID)")
	rm $(OPENOCDPID)

main.bin: main.elf
	@echo "...copying"
	$(CP) $(CPFLAGS) main.elf main.bin
	$(OD) $(ODFLAGS) main.elf> main.lst

main.elf: main.o system_stm32f4xx.o startup_stm32f407xx.o stm32f4_discovery.o libstm32f4xx_hal.a stm32.ld
	@echo ""
	@echo "..linking"
	$(LD) $(LFLAGS) -o main.elf main.o system_stm32f4xx.o startup_stm32f407xx.o stm32f4_discovery.o libstm32f4xx_hal.a

main.o: can_example.c
	@echo ""
	@echo ".compiling can_example.c"
	$(CC) $(CFLAGS) can_example.c -o main.o

system_stm32f4xx.o: system_stm32f4xx.c
	@echo ""
	@echo ".compiling system_stm32f4xx.c"
	$(CC) $(CFLAGS) -o $@ $< 

startup_stm32f407xx.o: startup_stm32f407xx.s
	@echo ""
	@echo ".assembling startup_stm32f407xx.s"
	$(AS) $(ASFLAGS) -o $@ $<

stm32f4_discovery.o: $(BSP_PATH)/stm32f4_discovery.c
	@echo ""
	@echo ".compiling the BSP driver for stm32f4disco"
	$(CC) $(CFLAGS) -o $@ $< 

libstm32f4xx_hal.a: haldrvdir $(HAL_OBJ_FILES)
	@echo ""
	@echo ".compiling the HAL driver for stm32f4xx"
	$(AR) rcs libstm32f4xx_hal.a $(HAL_OBJ_FILES)

haldrvdir:
	mkdir -p $(HAL_DRV_PATH)

$(HAL_DRV_PATH)/%.o: $(HAL_PATH)/Src/%.c
	$(CC) $(INC) -c -fno-common -O0 -g -mcpu=cortex-m4 -mthumb -DSTM32F407xx -nostartfiles -nostdlib -std=gnu99 -o $@ $^

debug:
	$(GDBTUI) -ex "target remote localhost:3333" \ 
	-ex "set remote hardware-breakpoint-limit 6" \ 
	-ex "set remote hardware-watchpoint-limit 4" main.elf
