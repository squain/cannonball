# -----------------------------------------------------------------------------
# CannonBall Windows CI Setup
#
# This cmake file is designed for GitHub Actions CI builds using vcpkg.
# It uses vcpkg for SDL2 and Boost dependencies.
# DirectX headers/libs are provided by Windows SDK.
# -----------------------------------------------------------------------------

# Use OpenGL for rendering.
set(OPENGL 1)

# Platform Specific Libraries
set(platform_link_libs
    opengl32 # For OpenGL
    glu32    # For OpenGL
    dxguid   # Direct X Haptic Support
    dinput8  # Direct X Haptic Support
)

# Platform Specific Link Directories (none needed - Windows SDK provides DirectX)
set(platform_link_dirs
)
