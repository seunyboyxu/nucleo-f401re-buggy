Import("env")
import os

# Get the project directory
project_dir = env.subst("$PROJECT_DIR")
mbed_lib_dir = os.path.join(project_dir, "lib", "mbed-dev")

# Add the precompiled libmbed.a using --whole-archive to pull in all symbols
env.Append(
    LINKFLAGS=[
        "-Wl,--whole-archive",
        os.path.join(mbed_lib_dir, "libmbed.a"),
        "-Wl,--no-whole-archive",
    ]
)
