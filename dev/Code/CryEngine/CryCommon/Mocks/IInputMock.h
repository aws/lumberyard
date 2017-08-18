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
#pragma once
#include <IInput.h>

#pragma warning( push )
#pragma warning( disable: 4373 )      // virtual function overrides differ only by const/volatile qualifiers, mock issue

namespace AzFramework
{
    class InputDeviceId {};
}

class InputMock
    : public IInput
{
public:
    MOCK_METHOD1(AddEventListener,
        void(IInputEventListener * pListener));
    MOCK_METHOD1(RemoveEventListener,
        void(IInputEventListener * pListener));
    MOCK_METHOD1(AddConsoleEventListener,
        void(IInputEventListener * pListener));
    MOCK_METHOD1(RemoveConsoleEventListener,
        void(IInputEventListener * pListener));
    MOCK_CONST_METHOD1(IsMotionSensorDataAvailable,
        bool(EMotionSensorFlags sensorFlags));
    MOCK_METHOD2(AddMotionSensorEventListener,
        bool(IMotionSensorEventListener * pListener, EMotionSensorFlags sensorFlags));
    MOCK_METHOD2(RemoveMotionSensorEventListener,
        void(IMotionSensorEventListener * pListener, EMotionSensorFlags sensorFlags));
    MOCK_METHOD1(ActivateMotionSensors,
        void(EMotionSensorFlags sensorFlags));
    MOCK_METHOD1(DeactivateMotionSensors,
        void(EMotionSensorFlags sensorFlags));
    MOCK_METHOD0(PauseMotionSensorInput,
        void());
    MOCK_METHOD0(UnpauseMotionSensorInput,
        void());
    MOCK_METHOD2(SetMotionSensorUpdateInterval,
        void(float seconds, bool force));
    MOCK_METHOD1(SetMotionSensorFilter,
        void(IMotionSensorFilter * filter));
    MOCK_CONST_METHOD0(GetMostRecentMotionSensorData,
        const SMotionSensorData * ());
    MOCK_METHOD1(SetExclusiveListener,
        void(IInputEventListener * pListener));
    MOCK_METHOD0(GetExclusiveListener,
        IInputEventListener * ());
    MOCK_METHOD1(AddInputDevice,
        bool(IInputDevice * pDevice));
    MOCK_METHOD4(AddAzToLyInputDevice,
        bool(EInputDeviceType, const char*, const std::vector<SInputSymbol>&, const AzFramework::InputDeviceId&));
    MOCK_METHOD1(EnableEventPosting,
        void(bool bEnable));
    MOCK_CONST_METHOD0(IsEventPostingEnabled,
        bool());
    MOCK_METHOD2(PostInputEvent,
        void(const SInputEvent&event, bool bForce));
    MOCK_METHOD2(PostMotionSensorEvent,
        void(const SMotionSensorEvent&event, bool bForce));
    MOCK_METHOD2(PostUnicodeEvent,
        void(const SUnicodeEvent&event, bool bForce));
    MOCK_METHOD4(ProcessKey,
        void(uint32 key, bool pressed, wchar_t unicode, bool repeat));
    MOCK_METHOD1(ForceFeedbackEvent,
        void(const SFFOutputEvent&event));
    MOCK_METHOD1(ForceFeedbackSetDeviceIndex,
        void(int index));
    MOCK_METHOD0(Init,
        bool());
    MOCK_METHOD0(PostInit,
        void());
    MOCK_METHOD1(Update,
        void(bool bFocus));
    MOCK_METHOD0(ShutDown,
        void());
    MOCK_METHOD3(SetExclusiveMode,
        void(EInputDeviceType deviceType, bool exclusive, void* hwnd));
    MOCK_METHOD2(InputState,
        bool(const TKeyName&key, EInputState state));
    MOCK_CONST_METHOD1(GetKeyName,
        const char*(const SInputEvent&event));
    MOCK_CONST_METHOD1(GetKeyName,
        const char*(EKeyId keyId));
    MOCK_METHOD1(GetInputCharAscii,
        char(const SInputEvent&event));
    MOCK_METHOD3(LookupSymbol,
        SInputSymbol * (EInputDeviceType deviceType, int deviceIndex, EKeyId keyId));
    MOCK_CONST_METHOD1(GetSymbolByName,
        const SInputSymbol * (const char* name));
    MOCK_METHOD1(GetOSKeyName,
        const char*(const SInputEvent&event));
    MOCK_METHOD0(ClearKeyState,
        void());
    MOCK_METHOD0(ClearAnalogKeyState,
        void());
    MOCK_METHOD0(RetriggerKeyState,
        void());
    MOCK_METHOD0(Retriggering,
        bool());
    MOCK_METHOD1(HasInputDeviceOfType,
        bool(EInputDeviceType type));
    MOCK_CONST_METHOD0(GetModifiers,
        int());
    MOCK_METHOD2(EnableDevice,
        void(EInputDeviceType deviceType, bool enable));
    MOCK_METHOD1(SetDeadZone,
        void(float fThreshold));
    MOCK_METHOD0(RestoreDefaultDeadZone,
        void());
    MOCK_METHOD2(GetDevice,
        IInputDevice * (uint16 id, EInputDeviceType deviceType));
    MOCK_CONST_METHOD0(GetPlatformFlags,
        uint32());
    MOCK_METHOD1(SetBlockingInput,
        bool(const SInputBlockData&inputBlockData));
    MOCK_METHOD1(RemoveBlockingInput,
        bool(const SInputBlockData&inputBlockData));
    MOCK_CONST_METHOD1(HasBlockingInput,
        bool(const SInputBlockData&inputBlockData));
    MOCK_CONST_METHOD0(GetNumBlockingInputs,
        int());
    MOCK_METHOD0(ClearBlockingInputs,
        void());
    MOCK_CONST_METHOD3(ShouldBlockInputEventPosting,
        bool(EKeyId keyId, EInputDeviceType deviceType, uint8 deviceIndex));
    MOCK_METHOD2(StartTextInput,
        void(const Vec2&inputRectTopLeft, const Vec2&inputRectBottomRight));
    MOCK_METHOD0(StopTextInput,
        void());
    MOCK_CONST_METHOD0(IsScreenKeyboardShowing,
        bool());
    MOCK_CONST_METHOD0(IsScreenKeyboardSupported,
        bool());
    MOCK_METHOD0(GetKinectInput,
        IKinectInput * ());
    MOCK_METHOD0(GetNaturalPointInput,
        INaturalPointInput * ());
    MOCK_METHOD1(GrabInput,
        bool(bool bGrab));
    MOCK_CONST_METHOD0(GetInputDevices,
        const TInputDevices&());
};

#pragma warning( pop )
