Import("env")
import os

# Get the project directory
project_dir = env.subst("$PROJECT_DIR")
mbed_lib_dir = os.path.join(project_dir, "lib", "mbed-dev")

# Add the precompiled libmbed.a using --whole-archive to pull in all symbols.
# Also add critical linker flags to match the Keil/Mbed 2 build environment.
#
# Key fixes vs previous build:
# 1. --specs=nano.specs: Use newlib-nano (smaller, matches Keil Mbed 2 runtime)
# 2. --specs=nosys.specs: Provide system call stubs for baremetal
# 3. -u _printf_float: Force float printf (needed for LCD.printf("%.2f",...))
# 4. FPU flags in link: Ensure correct libc/libgcc/libm FPU variants are linked
# 5. --wrap,main: Mbed 2 wraps main() to run mbed_sdk_init + SetSysClock before
#    user code, ensuring clocks and peripherals are configured properly
# 6. --wrap memory allocators: Mbed 2 provides wrapped malloc/free/calloc/realloc
# 7. --wrap,exit / --wrap,atexit: Mbed 2 overrides these for proper shutdown

env.Append(
    LINKFLAGS=[
        # Link with the precompiled Mbed 2 library (all object files)
        "-Wl,--whole-archive",
        os.path.join(mbed_lib_dir, "libmbed.a"),
        "-Wl,--no-whole-archive",
        # Use newlib-nano for smaller/correct printf (matches Keil Mbed 2)
        "--specs=nano.specs",
        # Provide system call stubs for baremetal
        "--specs=nosys.specs",
        # Force float printf support (needed for LCD.printf("%.2f", ...))
        "-u", "_printf_float",
        # FPU flags must be in link flags for correct library variant selection
        "-mfpu=fpv4-sp-d16",
        "-mfloat-abi=softfp",
        # Mbed 2 wraps main() to inject initialization (clock setup, etc.)
        "-Wl,--wrap,main",
        # Mbed 2 wraps memory allocation for thread safety
        "-Wl,--wrap,_calloc_r",
        "-Wl,--wrap,_free_r",
        "-Wl,--wrap,_malloc_r",
        "-Wl,--wrap,_memalign_r",
        "-Wl,--wrap,_realloc_r",
        # Mbed 2 wraps exit/atexit
        "-Wl,--wrap,exit",
        "-Wl,--wrap,atexit",
    ]
)

# Ensure compile flags include section separation for --gc-sections
env.Append(
    CCFLAGS=[
        "-ffunction-sections",
        "-fdata-sections",
    ]
)
