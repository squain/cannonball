# -----------------------------------------------------------------------------
# CannonBall macOS Setup
#
# Works on both Intel and Apple Silicon Macs.
# Uses SDL2's accelerated renderer (which uses Metal on macOS).
#
# Dependencies (install via Homebrew):
#   brew install sdl2 boost cmake
#
# Build:
#   cd cmake
#   mkdir build && cd build
#   cmake -DTARGET=macos.cmake ..
#   make
# -----------------------------------------------------------------------------

# Use SDL2 software renderer (which uses Metal acceleration on macOS)
# Do NOT set OPENGL or OPENGLES - this selects rendersurface.cpp

# Let CMake find SDL2 and Boost via Homebrew paths
# Homebrew installs to /opt/homebrew on Apple Silicon, /usr/local on Intel
# find_package() handles both automatically

# Platform Specific Libraries
# SDL2 is linked via find_package() in main CMakeLists.txt
set(platform_link_libs
)

# Platform Specific Link Directories
set(platform_link_dirs
)
