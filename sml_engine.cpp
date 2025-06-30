#include <stdint.h>
#include <stdbool.h>

typedef uint8_t  sml_u8;
typedef uint32_t sml_u32;

typedef int sml_i32;

typedef float sml_f32;

#define Sml_Unused(x) (void)(x)
#define Sml_Assert(cond) do { if (!(cond)) __debugbreak(); } while (0)

#define Sml_Kilobytes(Amount) ((Amount) * 1024)
#define Sml_Megabytes(Amount) (Kilobytes(Amount) * 1024)
#define Sml_Gigabytes(Amount) (Megabytes(Amount) * 1024)

enum class sml_instance  : sml_u32 {};
enum class sml_instanced : sml_u32 {};

#define STB_IMAGE_IMPLEMENTATION
#include "third_party/stb_image.h"

#define IMGUI_DISABLE_CHECKVERSION
#include "../code/third_party/imgui/imgui.h"

#pragma warning(push)
#pragma warning(disable: 4505) // Unreferenced functions

// Math
#include "math/vector.cpp"
#include "math/matrix.cpp"

// Spatial
#include "spatial/entity_test.cpp"

// Rendering
#include "platform/sml_platform.cpp"
#include "rendering/sml_rendering.cpp"

#pragma warning(pop)
