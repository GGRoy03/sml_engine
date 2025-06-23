#include <stdint.h>
#include <stdbool.h>

typedef uint8_t  sml_u8;
typedef uint32_t sml_u32;

typedef int sml_i32;

typedef float sml_f32;

#define Sml_Unused(x) (void)(x)
#define Sml_Assert(cond) do { if (!(cond)) __debugbreak(); } while (0)

// WARN: Temp to test our math code.
#define SML_USE_ROW_MAJOR

#define STB_IMAGE_IMPLEMENTATION
#include "third_party/stb_image.h"

// Math
#include "math/vector.cpp"
#include "math/matrix.cpp"

// Rendering
#include "rendering/meshes.cpp"

#include "platform/sml_platform.cpp"
#include "rendering/render.cpp"
