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
// Original file Copyright Crytek GMBH or its affiliates, used under license.

// Description : BaseInput implementation. This is primarily a "get things to
//               compile" thing for new platforms which haven't gotten a
//               real input implementation done yet. It implements all
//               the listener functionality and offers a uniform device
//               interface using IInputDevice.

#pragma once

#include <platform.h>
#include <IInput.h>

#if defined(AZ_RESTRICTED_PLATFORM)
#undef AZ_RESTRICTED_SECTION
#define BASEINPUT_H_SECTION_1 1
#define BASEINPUT_H_SECTION_2 2
#define BASEINPUT_H_SECTION_3 3
#endif

#if defined(AZ_RESTRICTED_PLATFORM)
#define AZ_RESTRICTED_SECTION BASEINPUT_H_SECTION_1
#include AZ_RESTRICTED_FILE(BaseInput_h, AZ_RESTRICTED_PLATFORM)
#endif

struct IInputDevice;
class CInputCVars;

class CBaseInput
    : public IInput
    , public ISystemEventListener
{
public:
    CBaseInput();
    virtual ~CBaseInput();

    // IInput
    // stub implementation
    virtual bool    Init();
    virtual void  PostInit();
    virtual void    Update(bool bFocus);
    virtual void    ShutDown();
    virtual void SetExclusiveMode(EInputDeviceType deviceType, bool exclusive, void* pUser);
    virtual bool    InputState(const TKeyName& keyName, EInputState state);
    virtual const char* GetKeyName(const SInputEvent& event) const;
    virtual const char* GetKeyName(EKeyId keyId) const;
    virtual char GetInputCharAscii(const SInputEvent& event);
    virtual SInputSymbol* LookupSymbol(EInputDeviceType deviceType, int deviceIndex, EKeyId keyId);
    virtual const SInputSymbol* GetSymbolByName(const char* name) const;
    virtual const char* GetOSKeyName(const SInputEvent& event);
    virtual void ClearKeyState();
    virtual void ClearAnalogKeyState();
    virtual void RetriggerKeyState();
    virtual bool Retriggering() {   return m_retriggering;  }
    virtual bool HasInputDeviceOfType(EInputDeviceType type);
    virtual void SetDeadZone(float fThreshold);
    virtual void RestoreDefaultDeadZone();
    virtual IInputDevice* GetDevice(uint16 id, EInputDeviceType deviceType);

    // listener functions (implemented)
    virtual void    AddEventListener(IInputEventListener* pListener);
    virtual void    RemoveEventListener(IInputEventListener* pListener);
    virtual bool    IsMotionSensorDataAvailable(EMotionSensorFlags sensorFlags) const { return false; }
    virtual bool    AddMotionSensorEventListener(IMotionSensorEventListener* pListener, EMotionSensorFlags sensorFlags = eMSF_AllAvailable) { return false; }
    virtual void    RemoveMotionSensorEventListener(IMotionSensorEventListener* pListener, EMotionSensorFlags sensorFlags = eMSF_AllAvailable) {}
    virtual void    AddConsoleEventListener(IInputEventListener* pListener);
    virtual void    RemoveConsoleEventListener(IInputEventListener* pLstener);
    virtual void    SetExclusiveListener(IInputEventListener* pListener);
    virtual IInputEventListener* GetExclusiveListener();
    virtual void    ActivateMotionSensors(EMotionSensorFlags sensorFlags = eMSF_AllAvailable) {}
    virtual void    DeactivateMotionSensors(EMotionSensorFlags sensorFlags = eMSF_AllAvailable) {}
    virtual void    PauseMotionSensorInput() {}
    virtual void    UnpauseMotionSensorInput() {}
    virtual void    SetMotionSensorUpdateInterval(float seconds, bool force = false) {}
    virtual void    SetMotionSensorFilter(IMotionSensorFilter* filter) {}
    virtual const   SMotionSensorData* GetMostRecentMotionSensorData() const { return nullptr; }
    virtual bool    AddInputDevice(IInputDevice* pDevice);
    virtual bool    AddAzToLyInputDevice(EInputDeviceType, const char*, const std::vector<SInputSymbol>&, const AzFramework::InputDeviceId&) { return false; }
    virtual void    EnableEventPosting(bool bEnable);
    virtual bool    IsEventPostingEnabled () const;
    virtual void    PostInputEvent(const SInputEvent& event, bool bForce = false);
    virtual void    PostMotionSensorEvent(const SMotionSensorEvent& event, bool bForce = false) {}
    virtual void    PostUnicodeEvent(const SUnicodeEvent& event, bool bForce = false);
    virtual void    ForceFeedbackEvent(const SFFOutputEvent& event);
    virtual void    ForceFeedbackSetDeviceIndex(int index);
    virtual void    EnableDevice(EInputDeviceType deviceType, bool enable);
    virtual void    ProcessKey(uint32 key, bool pressed, wchar_t unicode, bool repeat) {};
    // ~IInput

    // ISystemEventListener
    virtual void OnSystemEvent(ESystemEvent event, UINT_PTR wparam, UINT_PTR lparam);
    // ~ISystemEventListener

    bool    HasFocus() const        { return m_hasFocus;    }
    int     GetModifiers() const    { return m_modifiers;   }
    void    SetModifiers(int modifiers) {       m_modifiers = modifiers;    }

    virtual uint32 GetPlatformFlags() const { return m_platformFlags; }

    // Input blocking functionality
    virtual bool SetBlockingInput(const SInputBlockData& inputBlockData);
    virtual bool RemoveBlockingInput(const SInputBlockData& inputBlockData);
    virtual bool HasBlockingInput(const SInputBlockData& inputBlockData) const;
    virtual int GetNumBlockingInputs() const;
    virtual void ClearBlockingInputs();
    virtual bool ShouldBlockInputEventPosting(const EKeyId keyId, const EInputDeviceType deviceType, const uint8 deviceIndex) const;

    virtual IKinectInput* GetKinectInput()
    {
#if defined(AZ_RESTRICTED_PLATFORM)
#define AZ_RESTRICTED_SECTION BASEINPUT_H_SECTION_2
#include AZ_RESTRICTED_FILE(BaseInput_h, AZ_RESTRICTED_PLATFORM)
#endif
#if defined(AZ_RESTRICTED_SECTION_IMPLEMENTED)
#undef AZ_RESTRICTED_SECTION_IMPLEMENTED
#else
        return nullptr;
#endif
    }

    virtual INaturalPointInput* GetNaturalPointInput(){return nullptr; }

    virtual bool GrabInput(bool bGrab);
    const TInputDevices& GetInputDevices() const override { return m_inputDevices; }

    virtual void StartTextInput(const Vec2& inputRectTopLeft = ZERO, const Vec2& inputRectBottomRight = ZERO) {}
    virtual void StopTextInput() {}
    virtual bool IsScreenKeyboardShowing() const { return false; }
    virtual bool IsScreenKeyboardSupported() const { return false; }

protected:
    // Input blocking functionality
    void UpdateBlockingInputs();

    virtual void PostHoldEvents();
    void    ClearHoldEvent(SInputSymbol* pSymbol);

private:
    bool SendEventToListeners(const SInputEvent& event);
    bool SendEventToListeners(const SUnicodeEvent& event);
    void AddEventToHoldSymbols(const SInputEvent& event);
    void RemoveDeviceHoldSymbols(EInputDeviceType deviceType, uint8 deviceIndex);

    // listener functionality
    typedef std::list<IInputEventListener*> TInputEventListeners;
    TInputSymbols                   m_holdSymbols;
    TInputEventListeners    m_listeners;
    TInputEventListeners    m_consoleListeners;
    IInputEventListener*    m_pExclusiveListener;

    bool                    m_enableEventPosting;
    bool                    m_retriggering;
    CryCriticalSection      m_postInputEventMutex;

    bool                    m_hasFocus;

    // input device management
    TInputDevices m_inputDevices;
    AZStd::vector<AZStd::string> m_inputDeviceNames;
    AZStd::unordered_map<AZ::Crc32, AZStd::vector<AZStd::string> > m_deviceToInputsMap;

    //Filter for exclusive force-feedback output on an individual device
    int                                     m_forceFeedbackDeviceIndex;

    // Input blocking functionality
    typedef std::list<SInputBlockData> TInputBlockData;
    TInputBlockData             m_inputBlockData;

    // marcok: a bit nasty ... but I want to restrict access to CKeyboard ... this makes sure that
    // even mouse events could have a modifier
    int                                     m_modifiers;

    //CVars
    CInputCVars*                    m_pCVars;

#if defined(AZ_RESTRICTED_PLATFORM)
#define AZ_RESTRICTED_SECTION BASEINPUT_H_SECTION_3
#include AZ_RESTRICTED_FILE(BaseInput_h, AZ_RESTRICTED_PLATFORM)
#endif

protected:
    uint32                              m_platformFlags;
};
