cmake_minimum_required(VERSION 3.20)

# Имя проекта и языки (C и ассемблер)
project(GyroRTOS_native C ASM)

# Устанавливаем стандарты языка
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

# ==============================================================================
# 1. СБОР ИСХОДНЫХ ФАЙЛОВ (SOURCES)
# ==============================================================================

# --- Базовые папки проекта ---
file(GLOB_RECURSE SOURCES 
    "Core/Src/*.c"
    "Drivers/STM32F4xx_HAL_Driver/Src/*.c"
    "Middlewares/Third_Party/FreeRTOS/Source/*.c"
)

# Исключаем лишние файлы-шаблоны HAL
list(FILTER SOURCES EXCLUDE REGEX ".*_template\\.c$")

# Исключаем лишние менеджеры памяти FreeRTOS (оставляем только heap_4)
list(FILTER SOURCES EXCLUDE REGEX ".*heap_1\\.c$")
list(FILTER SOURCES EXCLUDE REGEX ".*heap_2\\.c$")
list(FILTER SOURCES EXCLUDE REGEX ".*heap_3\\.c$")
list(FILTER SOURCES EXCLUDE REGEX ".*heap_5\\.c$")

# Исключаем порты для других компиляторов
list(FILTER SOURCES EXCLUDE REGEX ".*portable/RVDS.*")
list(FILTER SOURCES EXCLUDE REGEX ".*portable/IAR.*")
list(FILTER SOURCES EXCLUDE REGEX ".*portable/MemMang.*")
list(FILTER SOURCES EXCLUDE REGEX ".*cmsis_os2\\.c$")

file(GLOB TRC_CORE_SOURCES "TraceRecorder/*.c")

# Собираем файлы порта RingBuffer (для Snapshot)
file(GLOB TRC_PORT_SOURCES "TraceRecorder/streamports/RingBuffer/*.c")
if(NOT TRC_PORT_SOURCES)
    # Если файлов в корне нет, ищем в подпапке src/
    file(GLOB TRC_PORT_SOURCES "TraceRecorder/streamports/RingBuffer/src/*.c")
endif()

# --- Финальный список файлов ---
list(APPEND SOURCES 
    "Middlewares/Third_Party/FreeRTOS/Source/portable/GCC/ARM_CM4F/port.c"
    "Middlewares/Third_Party/FreeRTOS/Source/portable/MemMang/heap_4.c"
    "startup_stm32f429xx.s"
    ${TRC_CORE_SOURCES}
    ${TRC_PORT_SOURCES}
)


# ==============================================================================
# 2. ПУТИ К ЗАГОЛОВОЧНЫМ ФАЙЛАМ (INCLUDE DIRS)
# ==============================================================================
include_directories(
    "Core/Inc"
    "Drivers/STM32F4xx_HAL_Driver/Inc"
    "Drivers/STM32F4xx_HAL_Driver/Inc/Legacy"
    "Drivers/CMSIS/Device/ST/STM32F4xx/Include"
    "Drivers/CMSIS/Include"
    "Middlewares/Third_Party/FreeRTOS/Source/include"
    "Middlewares/Third_Party/FreeRTOS/Source/portable/GCC/ARM_CM4F"
    
    # --- Пути к заголовкам Tracealyzer ---
    "TraceRecorder/config"
    "TraceRecorder/include"
    "TraceRecorder/streamports/RingBuffer/include"
    "TraceRecorder/streamports/RingBuffer/config"
)

# ==============================================================================
# 3. МАКРОСЫ И ФЛАГИ КОМПИЛЯЦИИ
# ==============================================================================
add_compile_definitions(
    USE_HAL_DRIVER
    STM32F429xx
)

add_compile_options(
    -Og -Wall -fdata-sections -ffunction-sections -g -gdwarf-2
)

# ==============================================================================
# 4. СОЗДАНИЕ ИСПОЛНЯЕМОГО ФАЙЛА И ЛИНКОВКА
# ==============================================================================
add_executable(${PROJECT_NAME}.elf ${SOURCES})

target_link_options(${PROJECT_NAME}.elf PRIVATE
    -T${CMAKE_SOURCE_DIR}/STM32F429xx_FLASH.ld
    -specs=nano.specs
    -lc -lm 
    -Wl,-Map=${PROJECT_NAME}.map,--cref
    -Wl,--gc-sections
)

# ==============================================================================
# 5. ГЕНЕРАЦИЯ BIN И HEX
# ==============================================================================
add_custom_command(TARGET ${PROJECT_NAME}.elf POST_BUILD
    COMMAND arm-none-eabi-objcopy -O binary -S $<TARGET_FILE:${PROJECT_NAME}.elf> ${PROJECT_NAME}.bin
    COMMAND arm-none-eabi-objcopy -O ihex $<TARGET_FILE:${PROJECT_NAME}.elf> ${PROJECT_NAME}.hex
    COMMENT "Generating BIN and HEX files"
)