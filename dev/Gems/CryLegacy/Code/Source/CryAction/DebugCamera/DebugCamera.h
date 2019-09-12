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
#ifndef __DEBUG_CAMERA_H__
#define __DEBUG_CAMERA_H__

#include <AzFramework/Debug/DebugCameraBus.h>
#include <AzFramework/Input/Events/InputChannelEventListener.h>

///////////////////////////////////////////////////////////////////////////////
class DebugCamera
    : public AzFramework::InputChannelEventListener
    , public AzFramework::DebugCameraBus::Handler
{
public:
    enum Mode
    {
        ModeOff,    // no debug cam
        ModeFree,   // free-fly
        ModeFixed,  // fixed cam, control goes back to game
    };

    DebugCamera();
    ~DebugCamera() override;

    void Update();
    void PostUpdate();
    bool IsEnabled();
    bool IsFixed();
    bool IsFree();

    void OnEnable();
    void OnDisable();
    void OnInvertY();
    void OnNextMode();

    // AzFramework::InputChannelEventListener
    bool OnInputChannelEventFiltered(const AzFramework::InputChannel& inputChannel) override;

    // AzFramework::DebugCameraBus
    void SetMode(AzFramework::DebugCameraInterface::Mode mode) override;
    AzFramework::DebugCameraInterface::Mode GetMode() const override;
    void GetPosition(AZ::Vector3& result) const override;
    void GetView(AZ::Matrix3x3& result) const override;
    void GetTransform(AZ::Transform& result) const override;

    void OnTogglePathRecording();
    void OnTogglePathSaving();
    void OnTogglePathLoading();
    void OnToggleTakeScreenshot();
protected:
    int m_mouseMoveMode;
    int m_isYInverted;
    int m_cameraMode;
    float m_cameraYawInput;
    float m_cameraPitchInput;
    float m_cameraYaw;
    float m_cameraPitch;
    Vec3 m_moveInput;

    float m_moveScale;
    float m_oldMoveScale;
    Vec3 m_position;
    Matrix33 m_view;

    //! True if the camera orientation or position has been updated.
    bool m_hasMoved = false;

    static void ToggleDebugCamera(IConsoleCmdArgs* pArgs);
    static void ToggleDebugCameraInvertY(IConsoleCmdArgs* pArgs);
    static void DebugCameraMove(IConsoleCmdArgs* pArgs);
    static void DebugCameraMoveTo(IConsoleCmdArgs* pArgs);

    void UpdatePitch(float amount);
    void UpdateYaw(float amount);
    void UpdatePosition(const Vec3& amount);
    void MovePosition(const Vec3& offset);
    void MovePositionTo(const Vec3& offset, const Vec3& rotations);
    
    // automation
#if !defined(_RELEASE) && !defined(DEDICATED_SERVER)
    void LoadPath();
    void SavePath();
    void RecordPath();
    void RunPath();

    bool m_recordPath;
    bool m_savePath;
    bool m_loadRunPath;
    bool m_takeScreenshot;

    std::vector< Vec3> m_path;
    Vec3 m_currentPosition;
    int m_currentIndex;

    bool m_runPath;
    float m_recordingTimer;
    float m_angle;
#endif // #if !defined(_RELEASE) && !defined(DEDICATED_SERVER)
private:
    ICVar* m_displayInfoCVar;
};


inline bool DebugCamera::IsEnabled()
{
    return m_cameraMode != DebugCamera::ModeOff;
}

inline bool DebugCamera::IsFixed()
{
    return m_cameraMode == DebugCamera::ModeFixed;
}

inline bool DebugCamera::IsFree()
{
    return m_cameraMode == DebugCamera::ModeFree;
}

#endif  //__DEBUG_CAMERA_H__
