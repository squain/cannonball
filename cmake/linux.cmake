# -----------------------------------------------------------------------------
# CannonBall Linux Setup
# -----------------------------------------------------------------------------

# Use OpenGL for rendering.
set(OPENGL 1)
find_package(OpenGL REQUIRED)

# Platform Specific Libraries
set(platform_link_libs
    ${OPENGL_LIBRARIES}
)