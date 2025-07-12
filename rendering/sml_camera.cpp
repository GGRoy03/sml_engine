// ===================================
// Type Definitions
// ===================================

constexpr sml_f32 DefaultNearPlane = 0.1f;
constexpr sml_f32 DefaultFarPlane  = 100.0f;
constexpr sml_f32 DefaultFovY      = 90.0f;
constexpr sml_f32 DefaultSpeed     = 0.05f;

struct camera_perspective
{
    sml_matrix4 View;
    sml_matrix4 Projection;

    sml_vector3 Position;
    sml_vector3 Target;

    sml_vector3 Up;

    sml_f32 FovY;
    sml_f32 Aspect;
    sml_f32 NearClip;
    sml_f32 FarClip;

    sml_f32 Speed;

    bool IsInitialized;
};

// ===================================
// Internal Helpers
// ===================================


// ===================================
// User API
// ===================================

// WARN: Projection code does not accept different planes.
// WARN: Does not use the delta time

static void
UpdateCamera(camera_perspective *Camera, sml_window Window,
             sml_game_controller_input *Inputs)
{
    // NOTE: Uhm, Idk about this.

    if(!Camera->IsInitialized)
    {
        Camera->Position = sml_vector3(0.0f, 0.0f, -2.0f);
        Camera->Up       = sml_vector3(0.0f, 1.0f,  0.0f);
        Camera->Target   = sml_vector3(0.0f, 0.0f,  0.0f);

        Camera->FovY     = DefaultFovY;
        Camera->NearClip = DefaultNearPlane; 
        Camera->FarClip  = DefaultFarPlane; 
        Camera->Aspect   = Window.Width / (f32)Window.Height;

        Camera->Speed = DefaultSpeed;

        Camera->IsInitialized = true;
    }

    sml_vector3 Forward = SmlVec3_Normalize(Camera->Target - Camera->Position);
    sml_vector3 Right   = SmlVec3_Normalize(SmlVec3_VectorProduct(Camera->Up,
                                                                   Forward));
    if(Inputs)
    {
        sml_vector3 Movement = sml_vector3(0.0f, 0.0f, 0.0f);

        if(Sml_IsKeyDown('W', Inputs))
        {
            Movement += Forward;
        }

        if(Sml_IsKeyDown('S', Inputs))
        {
            Movement += SmlVec3_Scale(Forward, -1.0f);
        }

        if(Sml_IsKeyDown('A', Inputs))
        {
            Movement += SmlVec3_Scale(Right, -1.0f);
        }

        if(Sml_IsKeyDown('D', Inputs))
        {
            Movement += Right;
        }

        if(!SmlVec3_IsZero(Movement))
        {
            sml_vector3 Normalized = SmlVec3_Normalize(Movement);
            Movement               = SmlVec3_Scale(Normalized, Camera->Speed);

            Camera->Position += Movement;
            Camera->Target    = Camera->Position + Forward;
        }
    }

    Camera->View        = SmlMat4_LookAt(Camera->Position, Right, Camera->Up,Forward);
    Camera->Projection  = SmlMat4_Perspective(Camera->FovY, Camera->Aspect);
    auto ViewProjection = SmlMat4_Multiply(Camera->Projection, Camera->View);

    UpdateCamera(ViewProjection);
}
