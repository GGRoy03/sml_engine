#include <stdint.h>
#include <stdbool.h>
#include <intrin.h>

typedef uint8_t  sml_u8;
typedef uint32_t sml_u32;
typedef uint64_t sml_u64;

typedef int sml_i32;

typedef float  sml_f32;
typedef double sml_f64;

#define Sml_Unused(x) (void)(x)
#define Sml_Assert(cond) do { if (!(cond)) __debugbreak(); } while (0)

#define Sml_Kilobytes(Amount) ((Amount) * 1024)
#define Sml_Megabytes(Amount) (Kilobytes(Amount) * 1024)
#define Sml_Gigabytes(Amount) (Megabytes(Amount) * 1024)

#define STB_IMAGE_IMPLEMENTATION
#define STBI_NO_PSD
#define STBI_NO_TGA
#define STBI_NO_GIF
#define STBI_NO_HDR
#define STBI_NO_PIC
#define STBI_NO_PNM
#include "third_party/stb_image.h"

#define IMGUI_DISABLE_CHECKVERSION
#include "third_party/imgui/imgui.h"

#define XXH_STATIC_LINKING_ONLY
#define XXH_IMPLEMENTATION
#include "third_party/xxhash.h"

#pragma warning(push)
#pragma warning(disable: 4505 4996) // Unreferenced functions | Unsafe functions

// Memory
#include "memory/sml_stack_memory.cpp"

// Data structures
#include "data_structures/sml_dynamic_array.cpp"
#include "data_structures/sml_stack.cpp"
#include "data_structures/sml_hashmap.cpp"
#include "data_structures/sml_slot_map.cpp"

// Math
#include "math/vector.cpp"
#include "math/matrix.cpp"
#include "math/geometry.cpp"

// Rendering
#include "platform/sml_platform.cpp"
#include "rendering/sml_rendering.cpp"

// Spatial
#include "spatial/sml_nav_mesh.cpp"
#include "spatial/entity_test.cpp"

// Editor
#include "editor/editor.cpp"

#pragma warning(pop)
