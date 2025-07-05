// ===================================
// Type Definitions
// ===================================

#define SML_NEAR_PLANE 0.1f
#define SML_FAR_PLANE 100.0f

#define D_PI 3.141592653589793
#define F_PI 3.1415927f

// Column major
struct sml_matrix4
{
    f32 m0,  m4,  m8,  m12;
    f32 m1,  m5,  m9,  m13;
    f32 m2,  m6,  m10, m14;
    f32 m3,  m7,  m11, m15;
};

// ===================================
// Helpers
// ===================================

static sml_matrix4
SmlMat4_Identity()
{
    sml_matrix4 Result = {};
    Result.m0          = 1.0f;
    Result.m5          = 1.0f;
    Result.m10         = 1.0f;
    Result.m15         = 1.0f;

    return Result;
}

// ===================================
// Math Implementation (column-major)
// ===================================

static sml_matrix4
SmlMat4_Perspective(f32 FieldOfViewY, f32 AspectRatio)
{
    sml_matrix4 Matrix;

    f32         FovInRadians = FieldOfViewY * (F_PI / 180.0f);
    f32         TanHalfFov   = tanf(FovInRadians / 2);
    f32         Range        = SML_FAR_PLANE - SML_NEAR_PLANE;

    Matrix.m0 = 1 / (AspectRatio * TanHalfFov);
    Matrix.m1 = 0.0f;
    Matrix.m2 = 0.0f;
    Matrix.m3 = 0.0f;

    Matrix.m4 = 0.0f;
    Matrix.m5 = 1 / TanHalfFov;
    Matrix.m6 = 0.0f;
    Matrix.m7 = 0.0f;

    Matrix.m8  = 0.0f;
    Matrix.m9  = 0.0f;
    Matrix.m10 = SML_FAR_PLANE / Range;
    Matrix.m11 = (-SML_FAR_PLANE * SML_NEAR_PLANE) / Range;

    Matrix.m12 = 0.0f;
    Matrix.m13 = 0.0f;
    Matrix.m14 = 1.0f;
    Matrix.m15 = 0.0f;

    return Matrix;
}

static sml_matrix4
SmlMat4_LookAt(sml_vector3 Eye, sml_vector3 Target, sml_vector3 WorldUp)
{
    sml_vector3 F = SmlVec3_Normalize(Target - Eye);
    sml_vector3 R = SmlVec3_Normalize(SmlVec3_VectorProduct(WorldUp, F));
    sml_vector3 U = SmlVec3_VectorProduct(F, R);

    sml_matrix4 Result;

    Result.m0 = R.x;
    Result.m1 = R.y;
    Result.m2 = R.z;
    Result.m3 = -SmlVec3_Dot(R, Eye);

    Result.m4 = U.x;
    Result.m5 = U.y;
    Result.m6 = U.z;
    Result.m7 = -SmlVec3_Dot(U, Eye);

    Result.m8  = F.x;
    Result.m9  = F.y;
    Result.m10 = F.z;
    Result.m11 = -SmlVec3_Dot(F, Eye);

    Result.m12 = 0.0f;
    Result.m13 = 0.0f;
    Result.m14 = 0.0f;
    Result.m15 = 1.0f;

    return Result;
}

static sml_matrix4
SmlMat4_LookAt(sml_vector3 Eye,sml_vector3 R, sml_vector3 U, sml_vector3 F)
{
    sml_matrix4 Result;

    Result.m0 = R.x;
    Result.m1 = R.y;
    Result.m2 = R.z;
    Result.m3 = -SmlVec3_Dot(R, Eye);

    Result.m4 = U.x;
    Result.m5 = U.y;
    Result.m6 = U.z;
    Result.m7 = -SmlVec3_Dot(U, Eye);

    Result.m8  = F.x;
    Result.m9  = F.y;
    Result.m10 = F.z;
    Result.m11 = -SmlVec3_Dot(F, Eye);

    Result.m12 = 0.0f;
    Result.m13 = 0.0f;
    Result.m14 = 0.0f;
    Result.m15 = 1.0f;

    return Result;
}

static sml_matrix4
SmlMat4_Multiply(sml_matrix4 A, sml_matrix4 B)
{
    sml_matrix4 Result;

    // Row 0 * Columns
    Result.m0 = (A.m0 * B.m0) + (A.m1 * B.m4) + (A.m2 * B.m8)  + (A.m3 * B.m12);
    Result.m1 = (A.m0 * B.m1) + (A.m1 * B.m5) + (A.m2 * B.m9)  + (A.m3 * B.m13);
    Result.m2 = (A.m0 * B.m2) + (A.m1 * B.m6) + (A.m2 * B.m10) + (A.m3 * B.m14);
    Result.m3 = (A.m0 * B.m3) + (A.m1 * B.m7) + (A.m2 * B.m11) + (A.m3 * B.m15);

    // Row 1 * Columns
    Result.m4 = (A.m4 * B.m0) + (A.m5 * B.m4) + (A.m6 * B.m8)  + (A.m7 * B.m12);
    Result.m5 = (A.m4 * B.m1) + (A.m5 * B.m5) + (A.m6 * B.m9)  + (A.m7 * B.m13);
    Result.m6 = (A.m4 * B.m2) + (A.m5 * B.m6) + (A.m6 * B.m10) + (A.m7 * B.m14);
    Result.m7 = (A.m4 * B.m3) + (A.m5 * B.m7) + (A.m6 * B.m11) + (A.m7 * B.m15);

    // Row 2 * Columns
    Result.m8  = (A.m8 * B.m0) + (A.m9 * B.m4) + (A.m10 * B.m8)  + (A.m11 * B.m12);
    Result.m9  = (A.m8 * B.m1) + (A.m9 * B.m5) + (A.m10 * B.m9)  + (A.m11 * B.m13);
    Result.m10 = (A.m8 * B.m2) + (A.m9 * B.m6) + (A.m10 * B.m10) + (A.m11 * B.m14);
    Result.m11 = (A.m8 * B.m3) + (A.m9 * B.m7) + (A.m10 * B.m11) + (A.m11 * B.m15);

    // Row 3 * Columns

    Result.m12 = (A.m12 * B.m0) + (A.m13 * B.m4) + (A.m14 * B.m8)  + (A.m15 * B.m12);
    Result.m13 = (A.m12 * B.m1) + (A.m13 * B.m5) + (A.m14 * B.m9)  + (A.m15 * B.m13);
    Result.m14 = (A.m12 * B.m2) + (A.m13 * B.m6) + (A.m14 * B.m10) + (A.m15 * B.m14);
    Result.m15 = (A.m12 * B.m3) + (A.m13 * B.m7) + (A.m14 * B.m11) + (A.m15 * B.m15);

    return Result;
}

static inline sml_matrix4
SmlMat4_Translation(sml_vector3 Translation)
{
    sml_matrix4 Result;

    Result.m0 = 1.0f;
    Result.m1 = 0.0f;
    Result.m2 = 0.0f;
    Result.m3 = Translation.x;

    Result.m4 = 0.0f;
    Result.m5 = 1.0f;
    Result.m6 = 0.0f;
    Result.m7 = Translation.y;

    Result.m8  = 0.0f;
    Result.m9  = 0.0f;
    Result.m10 = 1.0f;
    Result.m11 = Translation.z;

    Result.m12 = 0.0f;
    Result.m13 = 0.0f;
    Result.m14 = 0.0f;
    Result.m15 = 1.0f;

    return Result;
}
