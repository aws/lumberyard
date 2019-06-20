/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/
#include "CryLegacy_precompiled.h"
#include "DebugCamera.h"
#include "ISystem.h"
#include "IGame.h"
#include "Cry_Camera.h"
#include "IInput.h"

#include <AzCore/Casting/lossy_cast.h>

const float g_moveScaleIncrement = 0.1f;
const float g_moveScaleMin = 0.01f;
const float g_moveScaleMax = 10.0f;
const float g_mouseMoveScale = 0.1f;
const float g_gamepadRotationSpeed = 5.0f;
const float g_mouseMaxRotationSpeed = 270.0f;
const float g_moveSpeed = 10.0f;
const float g_maxPitch = 85.0f;
const float g_boostMultiplier = 10.0f;
const float g_minRotationSpeed = 15.0f;
const float g_maxRotationSpeed = 70.0f;

const float g_cameraAutoRotationSpeed = 40;
const float g_cameraAutoTranslationSpeed = 4;
const char* g_cameraAutoFilename = "Move_generated.cfg";

void ToggleDebugCamera(IConsoleCmdArgs* pArgs)
{
#if !defined(_RELEASE) && !defined(DEDICATED_SERVER)
    DebugCamera* debugCamera = CCryAction::GetCryAction()->GetDebugCamera();
    if (debugCamera)
    {
        if (!debugCamera->IsEnabled())
        {
            debugCamera->OnEnable();
        }
        else
        {
            debugCamera->OnNextMode();
        }
    }
#endif
}

void ToggleDebugCameraLoadPath(IConsoleCmdArgs* pArgs)
{
#if !defined(_RELEASE) && !defined(DEDICATED_SERVER)
    DebugCamera* debugCamera = CCryAction::GetCryAction()->GetDebugCamera();
    if (debugCamera)
    {
        debugCamera->OnTogglePathLoading();
    }
#endif
}

void ToggleDebugCameraSavePath(IConsoleCmdArgs* pArgs)
{
#if !defined(_RELEASE) && !defined(DEDICATED_SERVER)
    DebugCamera* debugCamera = CCryAction::GetCryAction()->GetDebugCamera();
    if (debugCamera)
    {
        debugCamera->OnTogglePathSaving();
    }
#endif
}
void ToggleDebugCameraRecordPath(IConsoleCmdArgs* pArgs)
{
#if !defined(_RELEASE) && !defined(DEDICATED_SERVER)
    DebugCamera* debugCamera = CCryAction::GetCryAction()->GetDebugCamera();
    if (debugCamera)
    {
        debugCamera->OnTogglePathRecording();
    }
#endif
}
void ToggleDebugCameraInvertY(IConsoleCmdArgs* pArgs)
{
#if !defined(_RELEASE) && !defined(DEDICATED_SERVER)
    DebugCamera* debugCamera = CCryAction::GetCryAction()->GetDebugCamera();
    if (debugCamera)
    {
        debugCamera->OnInvertY();
    }
#endif
}
void ToggleDebugCameraTakeScreenshot(IConsoleCmdArgs* pArgs)
{
#if !defined(_RELEASE) && !defined(DEDICATED_SERVER)
    DebugCamera* debugCamera = CCryAction::GetCryAction()->GetDebugCamera();
    if (debugCamera)
    {
        debugCamera->OnToggleTakeScreenshot();
    }
#endif
}

void DebugCameraMove(IConsoleCmdArgs* pArgs)
{
#if !defined(_RELEASE) && !defined(DEDICATED_SERVER)
    if (pArgs->GetArgCount() != 4)
    {
        CryLogAlways("debugCameraMove requires 3 args, not %d.", pArgs->GetArgCount() - 1);
        return;
    }

    DebugCamera* debugCamera = CCryAction::GetCryAction()->GetDebugCamera();
    if (debugCamera && debugCamera->IsFree())
    {
        Vec3::value_type x = azlossy_cast<float>(atof(pArgs->GetArg(1)));
        Vec3::value_type y = azlossy_cast<float>(atof(pArgs->GetArg(2)));
        Vec3::value_type z = azlossy_cast<float>(atof(pArgs->GetArg(3)));
        Vec3 newPos(x, y, z);
        debugCamera->MovePosition(newPos);
    }
#endif
}
void DebugCameraMoveTo(IConsoleCmdArgs* pArgs)
{
#if !defined(_RELEASE) && !defined(DEDICATED_SERVER)

    // Arguments are : name_of_command x y z pitch yaw  = 6 arguments.
    if (pArgs->GetArgCount() != 6)
    {
        CryLogAlways("debugCameraMoveTo requires 5 args, not %d.", pArgs->GetArgCount() - 1);
        return;
    }
    DebugCamera* debugCamera = CCryAction::GetCryAction()->GetDebugCamera();
    if (debugCamera && debugCamera->IsFree())
    {
        Vec3::value_type x = azlossy_cast<float>(atof(pArgs->GetArg(1)));
        Vec3::value_type y = azlossy_cast<float>(atof(pArgs->GetArg(2)));
        Vec3::value_type z = azlossy_cast<float>(atof(pArgs->GetArg(3)));
        Vec3 newPos(x, y, z);
        Vec3::value_type pitch = azlossy_cast<float>(atof(pArgs->GetArg(4)));
        Vec3::value_type yaw = azlossy_cast<float>(atof(pArgs->GetArg(5)));
        Vec3 newRotations(pitch, yaw, 0);
        debugCamera->MovePositionTo(newPos, newRotations);
    }
#endif
}
///////////////////////////////////////////////////////////////////////////////
DebugCamera::DebugCamera()
    : m_mouseMoveMode(0)
    , m_isYInverted(0)
    , m_cameraMode(DebugCamera::ModeOff)
    , m_cameraYawInput(0.0f)
    , m_cameraPitchInput(0.0f)
    , m_cameraYaw(0.0f)
    , m_cameraPitch(0.0f)
    , m_moveInput(ZERO)
    , m_moveScale(1.0f)
    , m_oldMoveScale(1.0f)
    , m_position(ZERO)
    , m_view(IDENTITY)
    , m_displayInfoCVar(gEnv->pConsole->GetCVar("r_DisplayInfo"))
#if !defined(_RELEASE) && !defined(DEDICATED_SERVER)
    , m_recordPath(false)
    , m_savePath(false)
    , m_runPath(false)
    , m_loadRunPath(false)
    , m_takeScreenshot(false)
    , m_recordingTimer(0.0f)
    , m_currentIndex(-1)
#endif // #if !defined(_RELEASE) && !defined(DEDICATED_SERVER)
{
    if (gEnv->pSystem->GetIInput())
    {
        gEnv->pSystem->GetIInput()->AddEventListener(this);
    }
    REGISTER_COMMAND("debugCameraSavePath", ToggleDebugCameraSavePath, VF_DEV_ONLY, "Write the recorded path.\n");
    REGISTER_COMMAND("debugCameraLoadPath", ToggleDebugCameraLoadPath, VF_DEV_ONLY, "Read the recorded path.\n");
    REGISTER_COMMAND("debugCameraRecordPathToggle", ToggleDebugCameraRecordPath, VF_DEV_ONLY, "Toggle the debug camera mode to record a path.\n");
    REGISTER_COMMAND("debugCameraToggle", ToggleDebugCamera, VF_DEV_ONLY, "Toggle the debug camera.\n");
    REGISTER_COMMAND("debugCameraInvertY", ToggleDebugCameraInvertY, VF_DEV_ONLY, "Toggle debug camera Y-axis inversion.\n");
    REGISTER_COMMAND("debugCameraMove", DebugCameraMove, VF_DEV_ONLY, "Move the debug camera the specified distance (x y z).\n");
    REGISTER_COMMAND("debugCameraMoveTo", DebugCameraMoveTo, VF_DEV_ONLY, "Move the debug camera to the specified position (x y z pitch yaw).\n");
    REGISTER_COMMAND("debugCameraTakeScreenshotToggle", ToggleDebugCameraTakeScreenshot, VF_DEV_ONLY, "TakeScreen shot when running on a path .\n");

    gEnv->pConsole->CreateKeyBind("ctrl_keyboard_key_punctuation_backslash", "debugCameraToggle");
    gEnv->pConsole->CreateKeyBind("alt_keyboard_key_punctuation_backslash", "debugCameraInvertY");

}

///////////////////////////////////////////////////////////////////////////////
DebugCamera::~DebugCamera()
{
    if (gEnv->pSystem->GetIInput())
    {
        gEnv->pSystem->GetIInput()->RemoveEventListener(this);
    }

    m_displayInfoCVar = nullptr;
}
///////////////////////////////////////////////////////////////////////////////
#if !defined(_RELEASE) && !defined(DEDICATED_SERVER)
void DebugCamera::OnTogglePathRecording()
{
    m_recordPath = !m_recordPath;
}
///////////////////////////////////////////////////////////////////////////////
void DebugCamera::OnTogglePathSaving()
{
    m_savePath = !m_savePath;
}
///////////////////////////////////////////////////////////////////////////////
void DebugCamera::OnTogglePathLoading()
{
    m_loadRunPath = !m_loadRunPath;
}
///////////////////////////////////////////////////////////////////////////////
void DebugCamera::OnToggleTakeScreenshot()
{
    m_takeScreenshot = !m_takeScreenshot;
}
#endif // #if !defined(_RELEASE) && !defined(DEDICATED_SERVER)
///////////////////////////////////////////////////////////////////////////////
void DebugCamera::OnEnable()
{
    m_position = gEnv->pSystem->GetViewCamera().GetPosition();
    m_moveInput = Vec3_Zero;

    Ang3 cameraAngles = Ang3(gEnv->pSystem->GetViewCamera().GetMatrix());
    m_cameraYaw = RAD2DEG(cameraAngles.z);
    m_cameraPitch = RAD2DEG(cameraAngles.x);
    m_view = Matrix33(Ang3(DEG2RAD(m_cameraPitch), 0.0f, DEG2RAD(m_cameraYaw)));

    m_cameraYawInput = 0.0f;
    m_cameraPitchInput = 0.0f;

    m_mouseMoveMode = 0;
    m_cameraMode = DebugCamera::ModeFree;
}

///////////////////////////////////////////////////////////////////////////////
void DebugCamera::OnDisable()
{
    m_mouseMoveMode = 0;
    m_cameraMode = DebugCamera::ModeOff;
}

///////////////////////////////////////////////////////////////////////////////
void DebugCamera::OnInvertY()
{
    m_isYInverted = !m_isYInverted;
}

///////////////////////////////////////////////////////////////////////////////
void DebugCamera::OnNextMode()
{
    if (m_cameraMode == DebugCamera::ModeFree)
    {
        m_cameraMode = DebugCamera::ModeFixed;
    }
    // ...
    else if (m_cameraMode == DebugCamera::ModeFixed)
    {
        // this is the last mode, go to disabled.
        OnDisable();
    }
}
///////////////////////////////////////////////////////////////////////////////
#if !defined(_RELEASE) && !defined(DEDICATED_SERVER)
void DebugCamera::RunPath()
{
    // If we reached the position, then target next position.
    uint numPositions = m_path.size();
    if (numPositions == 0)
    {
        return;
    }

    Vec3 nextPos = m_path[(m_currentIndex + 1) % numPositions];
    Vec3 direction = nextPos - m_currentPosition;
    if (direction.GetLengthSquared() < 0.01)
    {
        m_currentIndex++;
        m_currentPosition = nextPos;

        if (m_takeScreenshot && m_currentIndex % 10 == 0)
        {
            gEnv->pConsole->ExecuteString("screenshot path.tga");
        }
     }
    else
    {
        // if more than a certain distance (like 10m), then accelerate the transition;
        float largeDistanceFactor = direction.GetLengthSquared() > 100 ? direction.GetLengthSquared():1;
        m_currentPosition += direction.normalize() * gEnv->pTimer->GetFrameTime() * g_cameraAutoTranslationSpeed * largeDistanceFactor;
    }
 
    // Rotate constantly the camera in order to catch more shader variations.
    m_angle += gEnv->pTimer->GetFrameTime() * g_cameraAutoRotationSpeed;
    if (m_angle >= 360)
    {
        m_angle -= 360;
    }
 
    MovePositionTo(m_currentPosition, Vec3(0, m_angle,0));
}
///////////////////////////////////////////////////////////////////////////////
void DebugCamera::SavePath()
{
    FILE* file = nullptr;
    azfopen(&file, g_cameraAutoFilename, "w");
    if (!file)
    {
        CryLog("%s not found!", g_cameraAutoFilename);
    }
    for(auto &pos : m_path)
    {
        fprintf(file, "%f %f %f \n", pos.x, pos.y, pos.z);
    }
    fclose(file);
}
///////////////////////////////////////////////////////////////////////////////
void DebugCamera::RecordPath()
{
    m_recordingTimer += gEnv->pTimer->GetFrameTime();
    if (m_recordingTimer > 1)
    {
        m_recordingTimer = 0;
        const Vec3 currentCameraPosition = gEnv->p3DEngine->GetRenderingCamera().GetPosition();
        m_path.push_back(currentCameraPosition);
        CryLogAlways("Record position %f %f %f \n", currentCameraPosition.x, currentCameraPosition.y, currentCameraPosition.z);
    }
}
///////////////////////////////////////////////////////////////////////////////
void DebugCamera::LoadPath()
{

    // Open the file that contains positions data.
    string filename = g_cameraAutoFilename;
  
    if (filename[0] != '@') // console config files are actually by default in @root@ instead of @assets@
    {
        filename = PathUtil::Make("@root@", PathUtil::GetFile(filename));
    }

    string sfn = PathUtil::GetFile(filename);
    CCryFile file;
    if (!file.Open(filename, "rb", ICryPak::FOPEN_HINT_QUIET | ICryPak::FOPEN_ONDISK))
    {
        CryLog("%s not found!", filename.c_str());
        return;
    }
    CryLog("%s found in %s ...", PathUtil::GetFile(filename.c_str()), PathUtil::GetPath(filename).c_str());

    // Retrieve position data from text data.
    m_path.clear();
    int nLen = file.GetLength();
    if (nLen)
    {
        char* sAllText = new char[nLen + 16];
        file.ReadRaw(sAllText, nLen);
        sAllText[nLen] = '\0';

        char* strLast = sAllText + nLen;
        char* str = sAllText;
        while (str < strLast)
        {
            char* s = str;
            while (str < strLast && *str != '\n' && *str != '\r')
            {
                str++;
            }
            *str = '\0';
            str++;
            while (str < strLast && (*str == '\n' || *str == '\r'))
            {
                str++;
            }

            Vec3 pos;
            azsscanf(s, "%f %f %f", &pos.x, &pos.y, &pos.z);
            m_path.push_back(pos);
        }
        delete[] sAllText;
    }
}
#endif // #if !defined(_RELEASE) && !defined(DEDICATED_SERVER)
///////////////////////////////////////////////////////////////////////////////
void DebugCamera::Update()
{
#if !defined(_RELEASE) && !defined(DEDICATED_SERVER)
    if (m_recordPath)
    {
        RecordPath();
    }

    if (m_savePath)
    {
        SavePath();
        m_savePath = false;
    }

    if (m_loadRunPath)
    {
        OnEnable();
        LoadPath();
        m_runPath = true;
        m_loadRunPath = false;
        m_currentPosition = m_path[0];
        m_currentIndex = 0;
    }

    if (m_runPath)
    {
        RunPath();
    }
#endif //#if !defined(_RELEASE) && !defined(DEDICATED_SERVER)
    if (m_cameraMode == DebugCamera::ModeOff)
    {
        return;
    }

    float rotationSpeed = clamp_tpl(m_moveScale, g_minRotationSpeed, g_maxRotationSpeed);
    UpdateYaw(m_cameraYawInput * rotationSpeed * gEnv->pTimer->GetFrameTime());
    UpdatePitch(m_cameraPitchInput * rotationSpeed * gEnv->pTimer->GetFrameTime());

    m_view = Matrix33(Ang3(DEG2RAD(m_cameraPitch), 0.0f, DEG2RAD(m_cameraYaw)));
    UpdatePosition(m_moveInput);

    // update the listener of the active view
    if (IView* view = gEnv->pGame->GetIGameFramework()->GetIViewSystem()->GetActiveView())
    {
        view->UpdateAudioListener(Matrix34(m_view, m_position));
    }
}

///////////////////////////////////////////////////////////////////////////////
void DebugCamera::PostUpdate()
{
    if (m_cameraMode == DebugCamera::ModeOff)
    {
        return;
    }

    CCamera& camera = gEnv->pSystem->GetViewCamera();
    camera.SetMatrix(Matrix34(m_view, m_position));

    if (m_displayInfoCVar && m_displayInfoCVar->GetIVal())
    {
        const float FONT_COLOR[4] = { 1.0f, 0.0f, 0.0f, 1.0f };
        gEnv->pRenderer->Draw2dLabel(0.0f, 700.0f, 1.3f, FONT_COLOR, false,
        "Debug Camera: pos [ %.3f, %.3f, %.3f ] p/y [ %.1f, %.1f ] dir [ %.3f, %.3f, %.3f ] scl %.2f  inv %d",
            m_position.x, m_position.y, m_position.z,
        m_cameraPitch, m_cameraYaw,
            m_view.GetColumn1().x, m_view.GetColumn1().y, m_view.GetColumn1().z,
            m_moveScale,
            m_isYInverted);
    }
}

///////////////////////////////////////////////////////////////////////////////
bool DebugCamera::OnInputEvent(const SInputEvent& event)
{
    if (gEnv->pConsole->IsOpened())
    {
        return false;
    }

#if defined(AZ_RESTRICTED_PLATFORM)
    #if defined(AZ_PLATFORM_XENIA)
        #include "Xenia/DebugCamera_cpp_xenia.inl"
    #elif defined(AZ_PLATFORM_PROVO)
        #include "Provo/DebugCamera_cpp_provo.inl"
    #endif
#endif

    if (!IsEnabled() || m_cameraMode == DebugCamera::ModeFixed)
    {
        return false;
    }

    if (eIDT_Keyboard == event.deviceType)
    {
        if (eKI_W == event.keyId)
        {
            m_moveInput.y = event.value;
        }
        else if (eKI_S == event.keyId)
        {
            m_moveInput.y = -event.value;
        }
        else if (eKI_A == event.keyId)
        {
            m_moveInput.x = -event.value;
        }
        else if (eKI_D == event.keyId)
        {
            m_moveInput.x = event.value;
        }
        else if (eKI_LShift == event.keyId)
        {
            if (eIS_Released == event.state)
            {
                m_moveScale = m_oldMoveScale;
            }
            else if (eIS_Pressed == event.state)
            {
                m_oldMoveScale = m_moveScale;
                m_moveScale = clamp_tpl(m_moveScale * g_boostMultiplier, g_moveScaleMin, g_moveScaleMax);
            }
        }
    }
    else if (eIDT_Mouse == event.deviceType)
    {
        if (eKI_MouseWheelUp == event.keyId)
        {
            m_moveScale = clamp_tpl(m_moveScale + g_moveScaleIncrement, g_moveScaleMin, g_moveScaleMax);
        }
        else if (eKI_MouseWheelDown == event.keyId)
        {
            m_moveScale = clamp_tpl(m_moveScale - g_moveScaleIncrement, g_moveScaleMin, g_moveScaleMax);
        }
        else if (eKI_MouseX == event.keyId)
        {
            if (0 == m_mouseMoveMode)
            {
                UpdateYaw(fsgnf(-event.value) * clamp_tpl(fabs_tpl(event.value) * m_moveScale, 0.0f, g_mouseMaxRotationSpeed) * gEnv->pTimer->GetFrameTime());
            }
            //KC: If any mouse button is pressed then use the mouse movement
            //for horizontal movement.
            else
            {
                UpdatePosition(Vec3(event.value * g_mouseMoveScale, 0.0f, 0.0f));
            }
        }
        else if (eKI_MouseY == event.keyId)
        {
            if (0 == m_mouseMoveMode)
            {
                UpdatePitch(fsgnf(-event.value) * clamp_tpl(fabs_tpl(event.value) * m_moveScale, 0.0f, g_mouseMaxRotationSpeed) * gEnv->pTimer->GetFrameTime());
            }
            //KC: If both left and right mouse buttons are pressed then use
            //the mouse movement for vertical movement.
            else if (2 == m_mouseMoveMode)
            {
                UpdatePosition(Vec3(0.0f, 0.0f, -event.value * g_mouseMoveScale));
            }
            //KC: If one mouse button is pressed then use the mouse movement
            //for forward movement.
            else
            {
                UpdatePosition(Vec3(0.0f, -event.value * g_mouseMoveScale, 0.0f));
            }
        }
        else if (eKI_Mouse1 == event.keyId)
        {
            if (eIS_Released == event.state)
            {
                m_mouseMoveMode = clamp_tpl(m_mouseMoveMode - 1, 0, 2);
            }
            else if (eIS_Pressed == event.state)
            {
                m_mouseMoveMode = clamp_tpl(m_mouseMoveMode + 1, 0, 2);
            }
        }
        else if (eKI_Mouse2 == event.keyId)
        {
            if (eIS_Released == event.state)
            {
                m_mouseMoveMode = clamp_tpl(m_mouseMoveMode - 1, 0, 2);
            }
            else if (eIS_Pressed == event.state)
            {
                m_mouseMoveMode = clamp_tpl(m_mouseMoveMode + 1, 0, 2);
            }
        }
    }
    else if (eIDT_Gamepad == event.deviceType)
    {
        switch (event.keyId)
        {
        case eKI_XI_DPadUp:
        case eKI_Orbis_Up:
        {
            m_moveScale = clamp_tpl(m_moveScale + g_moveScaleIncrement, g_moveScaleMin, g_moveScaleMax);
            break;
        }
        case eKI_XI_DPadDown:
        case eKI_Orbis_Down:
        {
            m_moveScale = clamp_tpl(m_moveScale - g_moveScaleIncrement, g_moveScaleMin, g_moveScaleMax);
            break;
        }
        case eKI_XI_TriggerL:
        case eKI_Orbis_L2:
        {
            m_moveInput.z = -event.value;
            break;
        }
        case eKI_XI_TriggerR:
        case eKI_Orbis_R2:
        {
            m_moveInput.z = event.value;
            break;
        }
        case eKI_XI_ThumbLX:
        case eKI_Orbis_StickLX:
        {
            m_moveInput.x = event.value;
            break;
        }
        case eKI_XI_ThumbLY:
        case eKI_Orbis_StickLY:
        {
            m_moveInput.y = event.value;
            break;
        }
        case eKI_XI_ThumbRX:
        case eKI_Orbis_StickRX:
        {
            m_cameraYawInput = -event.value * g_gamepadRotationSpeed;
            break;
        }
        case eKI_XI_ThumbRY:
        case eKI_Orbis_StickRY:
        {
            m_cameraPitchInput = event.value * g_gamepadRotationSpeed;
            break;
        }
        //KC: Use the shoulder buttons to temporarily boost or reduce the scale.
        case eKI_XI_ShoulderL:
        case eKI_Orbis_L1:
        {
            if (eIS_Released == event.state)
            {
                m_moveScale = m_oldMoveScale;
            }
            else if (eIS_Pressed == event.state)
            {
                m_oldMoveScale = m_moveScale;
                m_moveScale = clamp_tpl(m_moveScale / g_boostMultiplier, g_moveScaleMin, g_moveScaleMax);
            }
            break;
        }
        case eKI_XI_ShoulderR:
        case eKI_Orbis_R1:
        {
            if (eIS_Released == event.state)
            {
                m_moveScale = m_oldMoveScale;
            }
            else if (eIS_Pressed == event.state)
            {
                m_oldMoveScale = m_moveScale;
                m_moveScale = clamp_tpl(m_moveScale * g_boostMultiplier, g_moveScaleMin, g_moveScaleMax);
            }
            break;
        }
        }
    }

    return false;
}

///////////////////////////////////////////////////////////////////////////////
void DebugCamera::UpdatePitch(float amount)
{
    if (m_isYInverted)
    {
        amount = -amount;
    }

    m_cameraPitch += amount;
    m_cameraPitch = clamp_tpl(m_cameraPitch, -g_maxPitch, g_maxPitch);
}

///////////////////////////////////////////////////////////////////////////////
void DebugCamera::UpdateYaw(float amount)
{
    m_cameraYaw += amount;
    if (m_cameraYaw < 0.0f)
    {
        m_cameraYaw += 360.0f;
    }
    else if (m_cameraYaw >= 360.0f)
    {
        m_cameraYaw -= 360.0f;
    }
}

///////////////////////////////////////////////////////////////////////////////
void DebugCamera::UpdatePosition(const Vec3& amount)
{
    Vec3 diff = amount * g_moveSpeed * m_moveScale * gEnv->pTimer->GetFrameTime();
    MovePosition(diff);
}

void DebugCamera::MovePosition(const Vec3& offset)
{
    m_position += m_view.GetColumn0() * offset.x;
    m_position += m_view.GetColumn1() * offset.y;
    m_position += m_view.GetColumn2() * offset.z;
}

void DebugCamera::MovePositionTo(const Vec3& offset, const Vec3& rotations)
{
    m_position.x = offset.x;
    m_position.y = offset.y;
    m_position.z = offset.z;
    m_cameraPitch = rotations.x;
    m_cameraYaw = rotations.y;
}
