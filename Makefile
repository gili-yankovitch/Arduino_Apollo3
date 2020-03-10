# ##### UTILITIES ##### #
CROSS_COMPILE = arm-none-eabi
AS = ${CROSS_COMPILE}-gcc
AR = ${CROSS_COMPILE}-ar
CC = ${CROSS_COMPILE}-gcc
CXX = ${CROSS_COMPILE}-g++
LD = ${CROSS_COMPILE}-gcc
AXF2BIN = ${CROSS_COMPILE}-objcopy
SIZE = ${CROSS_COMPILE}-size
#UPLOAD = tools/artemis/linux/artemis_svl
UPLOAD = tools/artemis/artemis_svl.py

# ##### CONFIGURATION ##### #
TARGET_NAME = Apollo3Artemis
VARIANT = redboard_artemis_nano
CORE = arduino
MCU = apollo3
HAL = cores/${CORE}/am_sdk_ap3/mcu/${MCU}/hal/gcc
CPU = cortex-m4
FPU = fpv4-sp-d16
FABI = hard

# ##### SERIAL PORT ##### #
SERIAL_PORT = /dev/ttyUSB0
BAUD_RATE = 115200

# ##### BUILD FLAGS ##### #
INCLUDE =
INCLUDE += -Ivariants/${VARIANT}/bsp
INCLUDE += -Ivariants/${VARIANT}/config
INCLUDE += -Icores/${CORE}/ard_sup
INCLUDE += -Icores/${CORE}/ard_sup/ard_supers/
INCLUDE += -Icores/${CORE}/am_sdk_ap3/mcu/${MCU}/
INCLUDE += -Icores/${CORE}/am_sdk_ap3/CMSIS/AmbiqMicro/Include/
INCLUDE += -Icores/${CORE}/am_sdk_ap3/CMSIS/ARM/Include/
INCLUDE += -Icores/${CORE}/am_sdk_ap3/devices/
INCLUDE += -Icores/${CORE}/am_sdk_ap3/utils/
INCLUDE += -IFreeRTOSv10.1.1/Source/portable/GCC/AMapollo2/
INCLUDE += -IFreeRTOSv10.1.1/Source/include/

DEFINES =
DEFINES += -DF_CPU=48000000L
DEFINES += -DARDUINO=1812
DEFINES += -DARDUINO_AM_AP3_SFE_BB_ARTEMIS_NANO
DEFINES += -DARDUINO_ARCH_APOLLO3
DEFINES += -DPART_apollo3
DEFINES += -DAM_PACKAGE_BGA
DEFINES += -DAM_PART_APOLLO3
DEFINES += -DAM_FREERTOS
DEFINES += -DWSF_TRACE_ENABLED
DEFINES += -DWSF_TRACE_ENABLED

COMMON_CXX_C_S_FLAGS = -c -g -MMD -mcpu=${CPU} -mthumb
COMMON_CXX_C_FLAGS = -mcpu=${CPU} -mthumb -mfloat-abi=${FABI} -fdata-sections -Os
CXXFLAGS = ${COMMON_CXX_C_S_FLAGS} ${COMMON_CXX_C_FLAGS} -ffunction-sections -std=gnu++11 -fno-threadsafe-statics -nostdlib --param max-inline-insns-single=500 -fno-rtti -fno-exceptions
CFLAGS = ${COMMON_CXX_C_S_FLAGS} ${COMMON_CXX_C_FLAGS} --function-sections -mfpu=fpv4-sp-d16 -std=gnu11
ASMFLAGS = ${COMMON_CXX_C_S_FLAGS} -x assembler-with-cpp

LDFLAGS =
LDFLAGS += -Lcores/${CORE}/am_sdk_ap3/CMSIS/ARM/Lib/ARM -larm_cortexM4lf_math
LDFLAGS += -mthumb -mcpu=cortex-m4 -mfpu=fpv4-sp-d16 -mfloat-abi=hard
LDFLAGS += -static
LDFLAGS += -Wl,--gc-sections,--entry,Reset_Handler -Wl,--start-group -lm -lc -lgcc -Wl,--end-group
LDFLAGS += -Wl,-Map,${TARGET_NAME}.map
LDFLAGS += -fno-exceptions --specs=nano.specs -t -lstdc++ -lc -lnosys -lm
LDFLAGS += -L${HAL}/bin/ -lam_hal
LDFLAGS += -nostdlib

LD_SCRIPT = variants/${VARIANT}/linker_scripts/gcc/artemis_sbl_svl_app.ld

ARFLAGS =
ARGLAGS += rcs

AXFFLAGS =
AXFFLAGS += -O binary

# ##### SOURCES ##### #

BSP_SOURCES =
BSP_SOURCES += variants/${VARIANT}/bsp/am_bsp.c
BSP_SOURCES += variants/${VARIANT}/bsp/am_bsp_pins.c

UTILS_SOURCES =
UTILS_SOURCES += cores/${CORE}/am_sdk_ap3/utils/am_util_ble.c
UTILS_SOURCES += cores/${CORE}/am_sdk_ap3/utils/am_util_debug.c
UTILS_SOURCES += cores/${CORE}/am_sdk_ap3/utils/am_util_delay.c
UTILS_SOURCES += cores/${CORE}/am_sdk_ap3/utils/am_util_faultisr.c
UTILS_SOURCES += cores/${CORE}/am_sdk_ap3/utils/am_util_id.c
UTILS_SOURCES += cores/${CORE}/am_sdk_ap3/utils/am_util_stdio.c
UTILS_SOURCES += cores/${CORE}/am_sdk_ap3/utils/am_util_string.c
UTILS_SOURCES += cores/${CORE}/am_sdk_ap3/utils/am_util_time.c

UTILS_SOURCES += cores/${CORE}/ard_sup/uart/ap3_uart_structures.c
UTILS_SOURCES += cores/${CORE}/ard_sup/analog/ap3_analog_structures.c
UTILS_SOURCES += cores/${CORE}/ard_sup/gpio/ap3_gpio_structures.c
UTILS_SOURCES += cores/${CORE}/ard_sup/ard_supers/hooks.c
UTILS_SOURCES += cores/${CORE}/ard_sup/ard_supers/itoa.c
UTILS_SOURCES += cores/${CORE}/ard_sup/ard_supers/avr/dtostrf.c

CONFIG_SOURCES = variants/${VARIANT}/config/variant.cpp

CORE_SOURCES =
# CORE_SOURCES += cores/${CORE}/ard_sup/main.cpp
CORE_SOURCES += cores/${CORE}/ard_sup/uart/ap3_uart.cpp
CORE_SOURCES += cores/${CORE}/ard_sup/initialization/ap3_initialization.cpp
CORE_SOURCES += cores/${CORE}/ard_sup/timing/ap3_timing.cpp
CORE_SOURCES += cores/${CORE}/ard_sup/clock/ap3_clock_sources.cpp
CORE_SOURCES += cores/${CORE}/ard_sup/debugging/ap3_debugging.cpp
CORE_SOURCES += cores/${CORE}/ard_sup/analog/ap3_analog.cpp
CORE_SOURCES += cores/${CORE}/ard_sup/gpio/ap3_shift.cpp
CORE_SOURCES += cores/${CORE}/ard_sup/gpio/ap3_gpio.cpp
CORE_SOURCES += cores/${CORE}/ard_sup/ard_supers/IPAddress.cpp
CORE_SOURCES += cores/${CORE}/ard_sup/ard_supers/WMath.cpp
CORE_SOURCES += cores/${CORE}/ard_sup/ard_supers/Print.cpp
# CORE_SOURCES += cores/${CORE}/ard_sup/ard_supers/WString.cpp
CORE_SOURCES += cores/${CORE}/ard_sup/ard_supers/Stream.cpp
CORE_SOURCES += cores/${CORE}/ard_sup/iomaster/ap3_iomaster.cpp

STARTUP_SOURCES = variants/${VARIANT}/startup/startup_gcc.c

FREERTOS_SOURCES =
FREERTOS_SOURCES += FreeRTOSv10.1.1/Source/event_groups.c
FREERTOS_SOURCES += FreeRTOSv10.1.1/Source/list.c
FREERTOS_SOURCES += FreeRTOSv10.1.1/Source/queue.c
FREERTOS_SOURCES += FreeRTOSv10.1.1/Source/tasks.c
FREERTOS_SOURCES += FreeRTOSv10.1.1/Source/timers.c
FREERTOS_SOURCES += FreeRTOSv10.1.1/Source/portable/MemMang/heap_2.c
FREERTOS_SOURCES += FreeRTOSv10.1.1/Source/portable/GCC/AMapollo2/port.c

LIBRARIES_SOURCES =
LIBRARIES_SOURCES += libraries/Wire/src/Wire.cpp
LIBRARIES_SOURCES += libraries/ATECCX08a/src/SparkFun_ATECCX08a_Arduino_Library.cpp

LIBRARIES_INCLUDE =
LIBRARIES_INCLUDE += -Ilibraries/Wire/src/
LIBRARIES_INCLUDE += -Ilibraries/ATECCX08a/src/

INCLUDE += ${LIBRARIES_INCLUDE}

PAYLOAD_SOURCES =
#PAYLOAD_SOURCES += payload/blink.cpp
#PAYLOAD_SOURCES += payload/wire.cpp
#PAYLOAD_SOURCES += payload/crypto.cpp
PAYLOAD_SOURCES += payload/main.cpp
PAYLOAD_SOURCES += payload/rtos.cpp

# SOURCES_ASM = cores/${CORE}/am_sdk_ap3/CMSIS/AmbiqMicro/Source/startup_apollo3.s
SOURCES_C = ${BSP_SOURCES} ${UTILS_SOURCES} ${STARTUP_SOURCES} ${FREERTOS_SOURCES}
SOURCES_CPP = ${CONFIG_SOURCES} ${CORE_SOURCES} ${PAYLOAD_SOURCES} ${LIBRARIES_SOURCES}

OBJS_ASM = $(SOURCES_ASM:%.s=%.os)
OBJS_C = $(SOURCES_C:%.c=%.o)
OBJS_CXX = $(SOURCES_CPP:%.cpp=%.oo)

all: ${TARGET_NAME}.bin

${TARGET_NAME}.bin: hal ${OBJS_C} ${OBJS_CXX}
	${LD} ${OBJS_C} ${OBJS_CXX} -T ${LD_SCRIPT} ${LDFLAGS} -L${HAL}/bin/ -lam_hal -o ${TARGET_NAME}.axf
	${AXF2BIN} ${AXFFLAGS} ${TARGET_NAME}.axf ${TARGET_NAME}.bin
	${SIZE} -A ${TARGET_NAME}.axf

upload: ${TARGET_NAME}.bin
	if [ ! -r ${SERIAL_PORT} ]; then  sudo chmod a+r ${SERIAL_PORT} ; fi
	if [ ! -w ${SERIAL_PORT} ]; then  sudo chmod a+w ${SERIAL_PORT} ; fi
	${UPLOAD} ${SERIAL_PORT} -f ${TARGET_NAME}.bin -b ${BAUD_RATE};

hal:
	make -C ${HAL}

%.os: %.s
	echo "Building $<"
	${AS} ${ASMFLAGS} ${DEFINES} ${INCLUDE} $< -o $@

%.o: %.c
	echo "Building $<"
	${CC} ${CFLAGS} ${DEFINES} ${INCLUDE} $< -o $@

%.oo: %.cpp
	echo "Building $<"
	${CXX} ${CXXFLAGS} ${DEFINES} ${INCLUDE}  $< -o $@

clean:
	rm -rf ${OBJS_C} ${OBJS_CXX}
	rm -rf ${TARGET_NAME}.axf ${TARGET_NAME}.map
