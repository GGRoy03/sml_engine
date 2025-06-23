#define SML_NEAR_PLANE 0.1f
#define SML_FAR_PLANE 100.0f

#define D_PI 3.141592653589793
#define F_PI 3.1415927f

struct sml_matrix4
{
    f32 m0 , m1 , m2 , m3,
        m4 , m5 , m6 , m7,
        m8 , m9 , m10, m11,
        m12, m13, m14, m15;
};

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

#if defined(SML_USE_ROW_MAJOR)

// WARN: Matrix.m14 depends on the coordinate system we are using.
// WARN: This maps to [0..1] which we might wanna do by convention? Unsure.
static sml_matrix4
SmlMat4_Perspective(f32 FieldOfView, f32 AspectRatio)
{
    sml_matrix4 Matrix;
    f32         FovInRadians = FieldOfView * (F_PI / 180.0f);
    f32         TanHalfFov   = tanf(FovInRadians / 2);
    f32         Range        = SML_FAR_PLANE - SML_NEAR_PLANE;

    Matrix.m0  = 1 / (AspectRatio * TanHalfFov);
    Matrix.m4  = 0.0f;
    Matrix.m8  = 0.0f;
    Matrix.m12 = 0.0f;

    Matrix.m1  = 0.0f;
    Matrix.m5  = 1 / TanHalfFov;
    Matrix.m9  = 0.0f;
    Matrix.m13 = 0.0f;

    Matrix.m2  = 0.0f;
    Matrix.m6  = 0.0f;
    Matrix.m10 = SML_FAR_PLANE / Range;
    Matrix.m14 = (-SML_FAR_PLANE * SML_NEAR_PLANE) / Range;

    Matrix.m3  = 0.0f;
    Matrix.m7  = 0.0f;
    Matrix.m11 = 1.0f;
    Matrix.m15 = 0.0f;

    return Matrix;
}

// WARN: The third row might need to get negated.
static sml_matrix4
SmlMat4_LookAt(sml_vector3 Eye, sml_vector3 Target, sml_vector3 WorldUp)
{
    sml_vector3 Forward = SmlVec3_Normalize(Target - Eye);
    sml_vector3 Right   = SmlVec3_Normalize(SmlVec3_VectorProduct(WorldUp, Forward));
    sml_vector3 Up      = SmlVec3_Normalize(SmlVec3_VectorProduct(Forward, Right));

    sml_matrix4 Matrix;

    // First row
    Matrix.m0  = Right.x;
    Matrix.m4  = Right.y;
    Matrix.m8  = Right.z;
    Matrix.m12 = -SmlVec3_Dot(Right, Eye);

    // Second row
    Matrix.m1  = Up.x;
    Matrix.m5  = Up.y;
    Matrix.m9  = Up.z;
    Matrix.m13 = -SmlVec3_Dot(Up, Eye);

    // Third row
    Matrix.m2  = Forward.x;
    Matrix.m6  = Forward.y;
    Matrix.m10 = Forward.z;
    Matrix.m14 = -SmlVec3_Dot(Forward, Eye);

    // Fourth row
    Matrix.m3  = 0.0f;
    Matrix.m7  = 0.0f;
    Matrix.m11 = 0.0f;
    Matrix.m15 = 1.0f;

    return Matrix;
}

static sml_matrix4
SmlMat4_Multiply()
{
    sml_matrix4 Result;

    return Result;
}

#else

#endif
