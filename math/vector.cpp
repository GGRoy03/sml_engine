struct sml_vector4
{
    f32 x, y, z, w;

    sml_vector4(){};

    sml_vector4(f32 x_, f32 y_, f32 z_, f32 w_)
    {
        x = x_;
        y = y_;
        z = z_;
        w = w_;
    }
};

struct sml_vector3
{
    f32 x, y, z;

    sml_vector3(){};

    sml_vector3(f32 x_, f32 y_, f32 z_)
    {
        x = x_;
        y = y_;
        z = z_;
    }
};

struct sml_vector2
{
    f32 x, y;

    sml_vector2(){};

    sml_vector2(f32 x_, f32 y_)
    {
        x = x_;
        y = y_;
    }
};

inline sml_vector3 operator+(const sml_vector3 &A, const sml_vector3 &B)
{
    sml_vector3 Result;

    Result.x = A.x + B.x;
    Result.y = A.y + B.y;
    Result.z = A.z + B.z;

    return Result;
}

inline sml_vector3& operator+=(sml_vector3 &A, const sml_vector3 &B)
{
    A.x += B.x;
    A.y += B.y;
    A.z += B.z;
    return A;
}

inline sml_vector3 operator-(const sml_vector3 &A, const sml_vector3 &B)
{
    sml_vector3 Result;

    Result.x = A.x - B.x;
    Result.y = A.y - B.y;
    Result.z = A.z - B.z;

    return Result;
}

static f32
SmlVec3_Dot(sml_vector3 A, sml_vector3 B)
{
    f32 Result = 0;

    Result += A.x * B.x;
    Result += A.y * B.y;
    Result += A.z * B.z;

    return Result;
}

static sml_vector3
SmlVec3_Scale(const sml_vector3 &A, f32 Scalar)
{
    sml_vector3 Result;

    Result.x = A.x * Scalar;
    Result.y = A.y * Scalar;
    Result.z = A.z * Scalar;

    return Result;
}

static bool
SmlVec3_IsZero(const sml_vector3 &A)
{
    bool Result = (A.x == 0.0f) && (A.y == 0.0f) && (A.z == 0.0f);
    return Result;
}

static sml_vector3
SmlVec3_VectorProduct(sml_vector3 A, sml_vector3 B)
{
    sml_vector3 Result;

    Result.x = (A.y * B.z) - (A.z * B.y);
    Result.y = (A.z * B.x) - (A.x * B.z);
    Result.z = (A.x * B.y) - (A.y * B.x);

    return Result;
}

static sml_vector3
SmlVec3_Normalize(sml_vector3 A)
{
    sml_vector3 Result;

    f32 LengthSquared = SmlVec3_Dot(A, A);

    if(LengthSquared > 0.0f)
    {
        f32 Length    = sqrtf(LengthSquared);
        f32 InvLength = 1.0f/Length;

        Result.x = A.x * InvLength;
        Result.y = A.y * InvLength;
        Result.z = A.z * InvLength;
    }

    return Result;
}
