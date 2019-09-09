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

#include "AzToLyInput.h"

#include "AzToLyInputDeviceGamepad.h"
#include "AzToLyInputDeviceKeyboard.h"
#include "AzToLyInputDeviceMotion.h"
#include "AzToLyInputDeviceMouse.h"
#include "AzToLyInputDeviceTouch.h"
#include "AzToLyInputDeviceVirtualKeyboard.h"
#include "InputCVars.h"
#include "SynergyContext.h"
#include "SynergyKeyboard.h"
#include "SynergyMouse.h"

#include <ILog.h>
#include <IRenderer.h>
#include <MathConversion.h>
#include <UnicodeIterator.h>

#include <AzFramework/Input/Channels/InputChannelAxis3D.h>
#include <AzFramework/Input/Devices/Motion/InputDeviceMotion.h>
#include <AzFramework/Input/Devices/Keyboard/InputDeviceKeyboard.h>
#include <AzFramework/Input/Devices/VirtualKeyboard/InputDeviceVirtualKeyboard.h>

#if !defined(_RELEASE) && !defined(WIN32) && !defined(DEDICATED_SERVER)
#   define SYNERGY_INPUT_ENABLED
#endif // !defined(_RELEASE) && !defined(WIN32) && !defined(DEDICATED_SERVER)

using namespace AzFramework;

#if defined(AZ_RESTRICTED_PLATFORM) || defined(AZ_TOOLS_EXPAND_FOR_RESTRICTED_PLATFORMS)
#undef AZ_RESTRICTED_SECTION
#define AZTOLYINPUT_CPP_SECTION_ADDINPUT 1
#define AZTOLYINPUT_CPP_SECTION_ADDTOOLSINPUT 2
#endif

////////////////////////////////////////////////////////////////////////////////////////////////////
AzToLyInput::AzToLyInput()
    : CBaseInput()
    , m_motionSensorListeners()
    , m_mostRecentMotionSensorData()
    , m_motionSensorFilter(nullptr)
    , m_currentlyActivatedMotionSensorFlags(eMSF_None)
    , m_manuallyActivatedMotionSensorFlags(eMSF_None)
    , m_motionSensorPausedCount(0)
    , m_activeKeyboardModifiers(eMM_None)
{
}

////////////////////////////////////////////////////////////////////////////////////////////////////
AzToLyInput::~AzToLyInput()
{
}

////////////////////////////////////////////////////////////////////////////////////////////////////
bool AzToLyInput::Init()
{
    gEnv->pLog->Log("Initializing AzToLyInput");

    if (!CBaseInput::Init())
    {
        gEnv->pLog->Log("Error: CBaseInput::Init failed");
        return false;
    }

    if (!AddInputDevice(new AzToLyInputDeviceMouse(*this)))
    {
        // CBaseInput::AddInputDevice calls delete on it's argument if it fails.
        gEnv->pLog->Log("Error: Initializing AzToLy Mouse");
        return false;
    }

    if (!AddInputDevice(new AzToLyInputDeviceKeyboard(*this)))
    {
        // CBaseInput::AddInputDevice calls delete on it's argument if it fails.
        gEnv->pLog->Log("Error: Initializing AzToLy Keyboard");
        return false;
    }

    if (!AddInputDevice(new AzToLyInputDeviceTouch(*this)))
    {
        // CBaseInput::AddInputDevice calls delete on it's argument if it fails.
        gEnv->pLog->Log("Error: Initializing AzToLy Touch");
        return false;
    }

    if (!AddInputDevice(new AzToLyInputDeviceMotion(*this)))
    {
        // CBaseInput::AddInputDevice calls delete on it's argument if it fails.
        gEnv->pLog->Log("Error: Initializing AzToLy Motion");
        return false;
    }

    if (!AddInputDevice(new AzToLyInputDeviceVirtualKeyboard(*this)))
    {
        // CBaseInput::AddInputDevice calls delete on it's argument if it fails.
        gEnv->pLog->Log("Error: Initializing AzToLy Virtual Keyboard");
        return false;
    }

    for (AZ::u32 i = 0; i < 4; ++i)
    {
#if defined(AZ_RESTRICTED_PLATFORM)
#define AZ_RESTRICTED_SECTION AZTOLYINPUT_CPP_SECTION_ADDINPUT
    #if defined(AZ_PLATFORM_XENIA)
        #include "Xenia/AzToLyInput_cpp_xenia.inl"
    #elif defined(AZ_PLATFORM_PROVO)
        #include "Provo/AzToLyInput_cpp_provo.inl"
    #endif
#endif
#if defined(AZ_RESTRICTED_SECTION_IMPLEMENTED)
#undef AZ_RESTRICTED_SECTION_IMPLEMENTED
#else
        if (!AddInputDevice(new AzToLyInputDeviceGamepad(*this, i)))
#endif
        {
            // CBaseInput::AddInputDevice calls delete on it's argument if it fails.
            gEnv->pLog->Log("Error: Initializing AzToLy Gamepad");
            return false;
        }
    }

#if defined(AZ_TOOLS_EXPAND_FOR_RESTRICTED_PLATFORMS)
#if defined(TOOLS_SUPPORT_XENIA)
#define AZ_RESTRICTED_SECTION AZTOLYINPUT_CPP_SECTION_ADDTOOLSINPUT
    #include "Xenia/AzToLyInput_cpp_xenia.inl"
#endif
#if defined(TOOLS_SUPPORT_PROVO)
#define AZ_RESTRICTED_SECTION AZTOLYINPUT_CPP_SECTION_ADDTOOLSINPUT
    #include "Provo/AzToLyInput_cpp_provo.inl"
#endif
#endif

#if defined(SYNERGY_INPUT_ENABLED)
    const char* pServer = g_pInputCVars->i_synergyServer->GetString();
    if (pServer && pServer[0] != '\0')
    {
        // Create an instance of SynergyClient that will broadcast RawInputNotificationBusSynergy events.
        m_synergyContext = AZStd::make_unique<SynergyInput::SynergyClient>(g_pInputCVars->i_synergyScreenName->GetString(), pServer);

        // Create the custom synergy keyboard implementation.
        AzFramework::InputDeviceImplementationRequest<AzFramework::InputDeviceKeyboard>::Bus::Broadcast(
            &AzFramework::InputDeviceImplementationRequest<AzFramework::InputDeviceKeyboard>::CreateCustomImplementation,
            SynergyInput::InputDeviceKeyboardSynergy::Create);

        // Create the custom synergy mouse implementation.
        AzFramework::InputDeviceImplementationRequest<AzFramework::InputDeviceMouse>::Bus::Broadcast(
            &AzFramework::InputDeviceImplementationRequest<AzFramework::InputDeviceMouse>::CreateCustomImplementation,
            SynergyInput::InputDeviceMouseSynergy::Create);
    }
#endif // defined(SYNERGY_INPUT_ENABLED)

    InputTextNotificationBus::Handler::BusConnect();
    AZ::TickBus::Handler::BusConnect();

    return true;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void AzToLyInput::ShutDown()
{
    gEnv->pLog->Log("AzToLyInput Shutdown");

    m_synergyContext.reset();
    AZ::TickBus::Handler::BusDisconnect();
    InputTextNotificationBus::Handler::BusDisconnect();

    CBaseInput::ShutDown();
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void AzToLyInput::ClearKeyState()
{
    InputChannelRequestBus::Broadcast(&InputChannelRequests::ResetState);
    CBaseInput::ClearKeyState();
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void AzToLyInput::PostHoldEvents()
{
    // Do nothing, AzToLyInputDevice dispatches hold events directly
}

////////////////////////////////////////////////////////////////////////////////////////////////////
int AzToLyInput::GetModifiers() const
{
    return m_activeKeyboardModifiers;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
bool AzToLyInput::AddAzToLyInputDevice(EInputDeviceType cryInputDeviceType,
                                       const char* cryInputDeviceDisplayName,
                                       const std::vector<SInputSymbol>& azToCryInputSymbols,
                                       const AzFramework::InputDeviceId& azFrameworkInputDeviceId)
{
    if (!AddInputDevice(new AzToLyInputDevice(*this,
                                              cryInputDeviceType,
                                              cryInputDeviceDisplayName,
                                              azToCryInputSymbols,
                                              azFrameworkInputDeviceId)))
    {
        // CBaseInput::AddInputDevice calls delete on it's argument if it fails.
        gEnv->pLog->Log("Error: AzToLyInput::AddAzToLyInputDevice");
        return false;
    }
    return true;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
int AzToLyInput::GetTickOrder()
{
    return AZ::ComponentTickBus::TICK_INPUT + 1; // Right after AzFramework input
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void AzToLyInput::OnTick(float deltaTime, AZ::ScriptTimePoint scriptTimePoint)
{
    if (!gEnv->IsEditor() || gEnv->IsEditorGameMode())
    {
        Update(true);
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void AzToLyInput::Update(bool bFocus)
{
    UpdateKeyboardModifiers();
    CBaseInput::Update(bFocus);
    UpdateMotionSensorInput();
}

////////////////////////////////////////////////////////////////////////////////////////////////////
bool IsInputChannelActive(const InputChannelId& inputChannelId)
{
    const InputChannel* inputChannel = InputChannelRequests::FindInputChannel(inputChannelId);
    return inputChannel ? inputChannel->IsActive() : false;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void AzToLyInput::UpdateKeyboardModifiers()
{
    // Cache the active keyboard modifiers to avoid doing all this every time an input event is
    // posted. As we transition systems to use AzFramework/Input directly they will simply need
    // to call InputChannelRequests::FindInputChannel and IsActive directly to check modifiers.
    m_activeKeyboardModifiers = eMM_None;

    if (IsInputChannelActive(InputDeviceKeyboard::Key::ModifierAltL))
    {
        m_activeKeyboardModifiers |= eMM_LAlt;
    }

    if (IsInputChannelActive(InputDeviceKeyboard::Key::ModifierAltR))
    {
        m_activeKeyboardModifiers |= eMM_RAlt;
    }

    if (IsInputChannelActive(InputDeviceKeyboard::Key::ModifierCtrlL))
    {
        m_activeKeyboardModifiers |= eMM_LCtrl;
    }

    if (IsInputChannelActive(InputDeviceKeyboard::Key::ModifierCtrlR))
    {
        m_activeKeyboardModifiers |= eMM_RCtrl;
    }

    if (IsInputChannelActive(InputDeviceKeyboard::Key::ModifierShiftL))
    {
        m_activeKeyboardModifiers |= eMM_LShift;
    }

    if (IsInputChannelActive(InputDeviceKeyboard::Key::ModifierShiftR))
    {
        m_activeKeyboardModifiers |= eMM_RShift;
    }

    if (IsInputChannelActive(InputDeviceKeyboard::Key::ModifierSuperL))
    {
        m_activeKeyboardModifiers |= eMM_LWin;
    }

    if (IsInputChannelActive(InputDeviceKeyboard::Key::ModifierSuperR))
    {
        m_activeKeyboardModifiers |= eMM_RWin;
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
bool CheckForUpdatedMotionSenserInput(const InputChannelId& channelId, Vec3& data)
{
    const InputChannel* inputChannel = InputChannelRequests::FindInputChannel(channelId);
    if (inputChannel == nullptr || inputChannel->IsStateIdle())
    {
        return false;
    }

    const InputChannelAxis3D::AxisData3D* axisData3D = inputChannel->GetCustomData<InputChannelAxis3D::AxisData3D>();
    if (axisData3D == nullptr)
    {
        return false;
    }

    data = AZVec3ToLYVec3(axisData3D->m_values);
    return true;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
bool CheckForUpdatedOrientationInput(const InputChannelId& channelId, Quat& data, Quat& delta)
{
    const InputChannel* inputChannel = InputChannelRequests::FindInputChannel(channelId);
    if (inputChannel == nullptr || inputChannel->IsStateIdle())
    {
        return false;
    }

    const InputChannelQuaternion::QuaternionData* quaternionData = inputChannel->GetCustomData<InputChannelQuaternion::QuaternionData>();
    if (quaternionData == nullptr)
    {
        return false;
    }

    data = AZQuaternionToLYQuaternion(quaternionData->m_value);

    // Wait until the orientation has been updated at least once before processing any deltas
    delta = inputChannel->IsStateUpdated() ? AZQuaternionToLYQuaternion(quaternionData->m_delta) : Quat::CreateIdentity();

    return true;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void AzToLyInput::UpdateMotionSensorInput()
{
    if (m_currentlyActivatedMotionSensorFlags == eMSF_None)
    {
        return;
    }

    int updatedSensorFlags = eMSF_None;

    // Check for updated acceleration data
    if (CheckForUpdatedMotionSenserInput(InputDeviceMotion::Acceleration::Raw, m_mostRecentMotionSensorData.accelerationRaw))
    {
        updatedSensorFlags |= eMSF_AccelerationRaw;
    }
    if (CheckForUpdatedMotionSenserInput(InputDeviceMotion::Acceleration::User, m_mostRecentMotionSensorData.accelerationUser))
    {
        updatedSensorFlags |= eMSF_AccelerationUser;
    }
    if (CheckForUpdatedMotionSenserInput(InputDeviceMotion::Acceleration::Gravity, m_mostRecentMotionSensorData.accelerationGravity))
    {
        updatedSensorFlags |= eMSF_AccelerationGravity;
    }

    // Check for updated rotation rate data
    if (CheckForUpdatedMotionSenserInput(InputDeviceMotion::RotationRate::Raw, m_mostRecentMotionSensorData.rotationRateRaw))
    {
        updatedSensorFlags |= eMSF_RotationRateRaw;
    }
    if (CheckForUpdatedMotionSenserInput(InputDeviceMotion::RotationRate::Unbiased, m_mostRecentMotionSensorData.rotationRateUnbiased))
    {
        updatedSensorFlags |= eMSF_RotationRateUnbiased;
    }

    // Check for updated magnetic field data
    if (CheckForUpdatedMotionSenserInput(InputDeviceMotion::MagneticField::Raw, m_mostRecentMotionSensorData.magneticFieldRaw))
    {
        updatedSensorFlags |= eMSF_MagneticFieldRaw;
    }
    if (CheckForUpdatedMotionSenserInput(InputDeviceMotion::MagneticField::Unbiased, m_mostRecentMotionSensorData.magneticFieldUnbiased))
    {
        updatedSensorFlags |= eMSF_MagneticFieldUnbiased;
    }
    if (CheckForUpdatedMotionSenserInput(InputDeviceMotion::MagneticField::North, m_mostRecentMotionSensorData.magneticNorth))
    {
        updatedSensorFlags |= eMSF_MagneticNorth;
    }

    // Check for updated orientation data
    if (CheckForUpdatedOrientationInput(InputDeviceMotion::Orientation::Current,
                                        m_mostRecentMotionSensorData.orientation,
                                        m_mostRecentMotionSensorData.orientationDelta))
    {
        updatedSensorFlags |= eMSF_Orientation;
        updatedSensorFlags |= eMSF_OrientationDelta;
    }

    if (updatedSensorFlags == eMSF_None)
    {
        return;
    }

    if (m_motionSensorFilter)
    {
        m_motionSensorFilter->Filter(m_mostRecentMotionSensorData, static_cast<EMotionSensorFlags>(updatedSensorFlags));
    }

    SMotionSensorEvent motionSensorEvent(m_mostRecentMotionSensorData, static_cast<EMotionSensorFlags>(updatedSensorFlags));
    PostMotionSensorEvent(motionSensorEvent);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
bool IsMotionSensorChannelEnabled(const InputChannelId& channelId)
{
    bool isEnabled = false;
    InputMotionSensorRequestBus::EventResult(isEnabled,
                                             InputDeviceMotion::Id,
                                             &InputMotionSensorRequests::GetInputChannelEnabled,
                                             channelId);
    return isEnabled;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
bool AzToLyInput::IsMotionSensorDataAvailable(EMotionSensorFlags sensorFlags) const
{
    // Check acceleration channels
    if (sensorFlags & eMSF_AccelerationRaw && !IsMotionSensorChannelEnabled(InputDeviceMotion::Acceleration::Raw)) { return false; }
    if (sensorFlags & eMSF_AccelerationUser && !IsMotionSensorChannelEnabled(InputDeviceMotion::Acceleration::User)) { return false; }
    if (sensorFlags & eMSF_AccelerationGravity && !IsMotionSensorChannelEnabled(InputDeviceMotion::Acceleration::Gravity)) { return false; }

    // Check rotation rate channels
    if (sensorFlags & eMSF_RotationRateRaw && !IsMotionSensorChannelEnabled(InputDeviceMotion::RotationRate::Raw)) { return false; }
    if (sensorFlags & eMSF_RotationRateUnbiased && !IsMotionSensorChannelEnabled(InputDeviceMotion::RotationRate::Unbiased)) { return false; }

    // Check magnetic field channels
    if (sensorFlags & eMSF_MagneticFieldRaw && !IsMotionSensorChannelEnabled(InputDeviceMotion::MagneticField::Raw)) { return false; }
    if (sensorFlags & eMSF_MagneticFieldUnbiased && !IsMotionSensorChannelEnabled(InputDeviceMotion::MagneticField::Unbiased)) { return false; }
    if (sensorFlags & eMSF_MagneticNorth && !IsMotionSensorChannelEnabled(InputDeviceMotion::MagneticField::North)) { return false; }

    // Check orientation channels
    if (sensorFlags & eMSF_Orientation & eMSF_OrientationDelta && !IsMotionSensorChannelEnabled(InputDeviceMotion::Orientation::Current)) { return false; }

    return true;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
bool AzToLyInput::AddMotionSensorEventListener(IMotionSensorEventListener* pListener, EMotionSensorFlags sensorFlags)
{
    m_motionSensorListeners[pListener] |= sensorFlags;
    RefreshCurrentlyActivatedMotionSensors();
    return true;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void AzToLyInput::RemoveMotionSensorEventListener(IMotionSensorEventListener* pListener, EMotionSensorFlags sensorFlags)
{
    m_motionSensorListeners[pListener] &= ~sensorFlags;
    if (m_motionSensorListeners[pListener] == eMSF_None)
    {
        m_motionSensorListeners.erase(pListener);
    }
    RefreshCurrentlyActivatedMotionSensors();
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void AzToLyInput::PostMotionSensorEvent(const SMotionSensorEvent& event, bool bForce /*= false*/)
{
    if (!bForce && !IsEventPostingEnabled())
    {
        return;
    }

    for (const auto& keyValue : m_motionSensorListeners)
    {
        if (keyValue.second & event.updatedFlags)
        {
            keyValue.first->OnMotionSensorEvent(event);
        }
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void AzToLyInput::ActivateMotionSensors(EMotionSensorFlags sensorFlags)
{
    m_manuallyActivatedMotionSensorFlags |= sensorFlags;
    RefreshCurrentlyActivatedMotionSensors();
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void AzToLyInput::DeactivateMotionSensors(EMotionSensorFlags sensorFlags)
{
    m_manuallyActivatedMotionSensorFlags &= ~sensorFlags;
    RefreshCurrentlyActivatedMotionSensors();
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void AzToLyInput::PauseMotionSensorInput()
{
    if (++m_motionSensorPausedCount == 1)
    {
        RefreshCurrentlyActivatedMotionSensors();
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void AzToLyInput::UnpauseMotionSensorInput()
{
    if (--m_motionSensorPausedCount == 0)
    {
        RefreshCurrentlyActivatedMotionSensors();
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void RefreshMotionSensorInputChannel(const InputChannelId& channelId, bool enabled)
{
    InputMotionSensorRequestBus::Event(InputDeviceMotion::Id,
                                       &InputMotionSensorRequests::SetInputChannelEnabled,
                                       channelId,
                                       enabled);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void AzToLyInput::RefreshCurrentlyActivatedMotionSensors()
{
    m_currentlyActivatedMotionSensorFlags = eMSF_None;
    if (m_motionSensorPausedCount <= 0)
    {
        m_currentlyActivatedMotionSensorFlags = m_manuallyActivatedMotionSensorFlags;
        for (const auto& keyValue : m_motionSensorListeners)
        {
            m_currentlyActivatedMotionSensorFlags |= keyValue.second;
        }
    }

    // Enable or disable acceleration channels
    RefreshMotionSensorInputChannel(InputDeviceMotion::Acceleration::Raw, (m_currentlyActivatedMotionSensorFlags & eMSF_AccelerationRaw) != 0);
    RefreshMotionSensorInputChannel(InputDeviceMotion::Acceleration::User, (m_currentlyActivatedMotionSensorFlags & eMSF_AccelerationUser) != 0);
    RefreshMotionSensorInputChannel(InputDeviceMotion::Acceleration::Gravity, (m_currentlyActivatedMotionSensorFlags & eMSF_AccelerationGravity) != 0);

    // Enable or disable rotation rate channels
    RefreshMotionSensorInputChannel(InputDeviceMotion::RotationRate::Raw, (m_currentlyActivatedMotionSensorFlags & eMSF_RotationRateRaw) != 0);
    RefreshMotionSensorInputChannel(InputDeviceMotion::RotationRate::Unbiased, (m_currentlyActivatedMotionSensorFlags & eMSF_RotationRateUnbiased) != 0);

    // Enable or disable magnetic field channels
    RefreshMotionSensorInputChannel(InputDeviceMotion::MagneticField::Raw, (m_currentlyActivatedMotionSensorFlags & eMSF_MagneticFieldRaw) != 0);
    RefreshMotionSensorInputChannel(InputDeviceMotion::MagneticField::Unbiased, (m_currentlyActivatedMotionSensorFlags & eMSF_MagneticFieldUnbiased) != 0);
    RefreshMotionSensorInputChannel(InputDeviceMotion::MagneticField::North, (m_currentlyActivatedMotionSensorFlags & eMSF_MagneticNorth) != 0);

    // Enable or disable orientation channels
    RefreshMotionSensorInputChannel(InputDeviceMotion::Orientation::Current, (m_currentlyActivatedMotionSensorFlags & (eMSF_Orientation | eMSF_OrientationDelta)) != 0);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void AzToLyInput::SetMotionSensorFilter(IMotionSensorFilter* filter)
{
    m_motionSensorFilter = filter;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
const SMotionSensorData* AzToLyInput::GetMostRecentMotionSensorData() const
{
    return &m_mostRecentMotionSensorData;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void AzToLyInput::StartTextInput(const Vec2& inputRectTopLeft, const Vec2& inputRectBottomRight)
{
    InputTextEntryRequests::VirtualKeyboardOptions options;
    options.m_normalizedMinY = inputRectBottomRight.y / static_cast<float>(gEnv->pRenderer->GetHeight());
    InputTextEntryRequestBus::Broadcast(&InputTextEntryRequests::TextEntryStart, options);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void AzToLyInput::StopTextInput()
{
    InputTextEntryRequestBus::Broadcast(&InputTextEntryRequests::TextEntryStop);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
bool AzToLyInput::IsScreenKeyboardShowing() const
{
    bool hasTextEntryStarted = false;
    InputTextEntryRequestBus::EventResult(hasTextEntryStarted,
                                          InputDeviceVirtualKeyboard::Id,
                                          &InputTextEntryRequests::HasTextEntryStarted);
    return hasTextEntryStarted;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
bool AzToLyInput::IsScreenKeyboardSupported() const
{
    const InputDevice* virtualKeyboardDevice = InputDeviceRequests::FindInputDevice(InputDeviceVirtualKeyboard::Id);
    return virtualKeyboardDevice ? virtualKeyboardDevice->IsSupported() : false;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void AzToLyInput::OnInputTextEvent(const AZStd::string& textUTF8, bool& o_hasBeenConsumed)
{
    if (o_hasBeenConsumed)
    {
        return;
    }

    // Iterate over and send an event for each unicode codepoint of the new text.
    for (Unicode::CIterator<const char*, false> it(textUTF8.c_str()); *it != 0; ++it)
    {
        const uint32_t codepoint = *it;
        SUnicodeEvent event(codepoint);
        PostUnicodeEvent(event);
    }
}
