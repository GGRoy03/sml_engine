// ===================================
// Type Definitions
// ===================================

#define SML_DEFAULT_NEAR_PLANE 0.1f
#define SML_DEFAULT_FAR_PLANE 100.0f
#define SML_DEFAULT_FOV_Y 90.0f
#define SML_DEFAULT_CAMERA_SPEED 0.05f

// NOTE: I wonder if we need to hold the matrices?
struct sml_camera_perspective
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

static sml_matrix4 
Sml_UpdateCamera(sml_camera_perspective *Camera, sml_window Window,
                 bool InternalUpdate)
{
    if(!Camera->IsInitialized)
    {
        Camera->Position = sml_vector3(0.0f, 0.0f, -2.0f);
        Camera->Up       = sml_vector3(0.0f, 1.0f,  0.0f);
        Camera->Target   = sml_vector3(0.0f, 0.0f,  0.0f);

        Camera->FovY     = SML_DEFAULT_FOV_Y;
        Camera->NearClip = SML_DEFAULT_NEAR_PLANE; 
        Camera->FarClip  = SML_DEFAULT_FAR_PLANE;
        Camera->Aspect   = Window.Width / (f32)Window.Height;

        Camera->Speed = SML_DEFAULT_CAMERA_SPEED;

        Camera->IsInitialized = true;
    }

    if(InternalUpdate)
    {
        // Read the inputs (W, A, S, D, Mouse) and rebuild the camera
    }

    Camera->View       = SmlMat4_LookAt(Camera->Position, Camera->Target, Camera->Up);
    Camera->Projection = SmlMat4_Perspective(Camera->FovY, Camera->Aspect);

    sml_matrix4 Result = SmlMat4_Multiply(Camera->Projection, Camera->View);

    return Result;
}
