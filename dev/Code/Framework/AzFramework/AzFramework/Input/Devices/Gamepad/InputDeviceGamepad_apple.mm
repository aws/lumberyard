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

#include <AzFramework/Input/Devices/Gamepad/InputDeviceGamepad.h>

#include <GameController/GameController.h>

#include <AzCore/Debug/Trace.h>

////////////////////////////////////////////////////////////////////////////////////////////////////
namespace
{
    using namespace AzFramework;

    ////////////////////////////////////////////////////////////////////////////////////////////////
    //! Digital button bitmasks
    const AZ::u32 DIGITAL_BUTTON_MASK_DU    = 0x0001;
    const AZ::u32 DIGITAL_BUTTON_MASK_DD    = 0x0002;
    const AZ::u32 DIGITAL_BUTTON_MASK_DL    = 0x0004;
    const AZ::u32 DIGITAL_BUTTON_MASK_DR    = 0x0008;

    const AZ::u32 DIGITAL_BUTTON_MASK_L1    = 0x0010;
    const AZ::u32 DIGITAL_BUTTON_MASK_R1    = 0x0020;
    const AZ::u32 DIGITAL_BUTTON_MASK_L3    = 0x0040;
    const AZ::u32 DIGITAL_BUTTON_MASK_R3    = 0x0080;

    const AZ::u32 DIGITAL_BUTTON_MASK_A     = 0x0100;
    const AZ::u32 DIGITAL_BUTTON_MASK_B     = 0x0200;
    const AZ::u32 DIGITAL_BUTTON_MASK_X     = 0x0400;
    const AZ::u32 DIGITAL_BUTTON_MASK_Y     = 0x0800;

    const AZ::u32 DIGITAL_BUTTON_MASK_PAUSE = 0x1000;

    ////////////////////////////////////////////////////////////////////////////////////////////////
    //! Map of digital button ids keyed by their button bitmask
    const AZStd::unordered_map<AZ::u32, const InputChannelId*> GetDigitalButtonIdByBitMaskMap()
    {
        const AZStd::unordered_map<AZ::u32, const InputChannelId*> map =
        {
            { DIGITAL_BUTTON_MASK_DU,       &InputDeviceGamepad::Button::DU },
            { DIGITAL_BUTTON_MASK_DD,       &InputDeviceGamepad::Button::DD },
            { DIGITAL_BUTTON_MASK_DL,       &InputDeviceGamepad::Button::DL },
            { DIGITAL_BUTTON_MASK_DR,       &InputDeviceGamepad::Button::DR },

            { DIGITAL_BUTTON_MASK_L1,       &InputDeviceGamepad::Button::L1 },
            { DIGITAL_BUTTON_MASK_R1,       &InputDeviceGamepad::Button::R1 },
            { DIGITAL_BUTTON_MASK_L3,       &InputDeviceGamepad::Button::L3 },
            { DIGITAL_BUTTON_MASK_R3,       &InputDeviceGamepad::Button::R3 },

            { DIGITAL_BUTTON_MASK_A,        &InputDeviceGamepad::Button::A },
            { DIGITAL_BUTTON_MASK_B,        &InputDeviceGamepad::Button::B },
            { DIGITAL_BUTTON_MASK_X,        &InputDeviceGamepad::Button::X },
            { DIGITAL_BUTTON_MASK_Y,        &InputDeviceGamepad::Button::Y },

            { DIGITAL_BUTTON_MASK_PAUSE,    &InputDeviceGamepad::Button::Start }
        };
        return map;
    }
} // namespace

////////////////////////////////////////////////////////////////////////////////////////////////////
namespace AzFramework
{
    ////////////////////////////////////////////////////////////////////////////////////////////////
    //! Platform specific implementation for apple game-pad input devices
    class InputDeviceGamepadApple : public InputDeviceGamepad::Implementation
    {
    public:
        ////////////////////////////////////////////////////////////////////////////////////////////
        // Allocator
        AZ_CLASS_ALLOCATOR(InputDeviceGamepadApple, AZ::SystemAllocator, 0);

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Constructor
        //! \param[in] inputDevice Reference to the input device being implemented
        InputDeviceGamepadApple(InputDeviceGamepad& inputDevice);

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Destructor
        ~InputDeviceGamepadApple() override;

    private:
        ////////////////////////////////////////////////////////////////////////////////////////////
        //! \ref AzFramework::InputDeviceGamepad::Implementation::IsConnected
        bool IsConnected() const override;

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! \ref AzFramework::InputDeviceGamepad::Implementation::SetVibration
        void SetVibration(float leftMotorSpeedNormalized, float rightMotorSpeedNormalized) override;

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! \ref AzFramework::InputDeviceGamepad::Implementation::GetPhysicalKeyOrButtonText
        bool GetPhysicalKeyOrButtonText(const InputChannelId& inputChannelId,
                                        AZStd::string& o_keyOrButtonText) const override;

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! \ref AzFramework::InputDeviceGamepad::Implementation::TickInputDevice
        void TickInputDevice() override;

        ////////////////////////////////////////////////////////////////////////////////////////////
        // Variables
        RawGamepadState m_rawGamepadState;        //!< The last known raw game-pad state
        GCController*   m_controller;             //!< The currently assigned controller
        bool            m_wasPausedHandlerCalled; //!< Was the controller paused handler called?
    };

    ////////////////////////////////////////////////////////////////////////////////////////////////
    AZ::u32 InputDeviceGamepad::GetMaxSupportedGamepads()
    {
        return GCControllerPlayerIndex4 + 1;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    InputDeviceGamepad::Implementation* InputDeviceGamepad::Implementation::Create(
        InputDeviceGamepad& inputDevice)
    {
        return aznew InputDeviceGamepadApple(inputDevice);
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    InputDeviceGamepadApple::InputDeviceGamepadApple(InputDeviceGamepad& inputDevice)
        : InputDeviceGamepad::Implementation(inputDevice)
        , m_rawGamepadState(GetDigitalButtonIdByBitMaskMap())
        , m_controller(nullptr)
        , m_wasPausedHandlerCalled(false)
    {
        AZ_Assert(inputDevice.GetInputDeviceId().GetIndex() < InputDeviceGamepad::GetMaxSupportedGamepads(),
                  "Creating InputDeviceGamepadApple with index %d that is greater than the max supported by the game controller framework: %d",
                  inputDevice.GetInputDeviceId().GetIndex(), InputDeviceGamepad::GetMaxSupportedGamepads());
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    InputDeviceGamepadApple::~InputDeviceGamepadApple()
    {
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    bool InputDeviceGamepadApple::IsConnected() const
    {
        return m_controller != nullptr;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    void InputDeviceGamepadApple::SetVibration(float leftMotorSpeedNormalized,
                                               float rightMotorSpeedNormalized)
    {
        // The apple game controller framework does not (yet?) support force-feedback
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    bool InputDeviceGamepadApple::GetPhysicalKeyOrButtonText(const InputChannelId& inputChannelId,
                                                             AZStd::string& o_keyOrButtonText) const
    {
        if (inputChannelId == InputDeviceGamepad::Button::Start)
        {
            o_keyOrButtonText = "Pause";
            return true;
        }
        return false;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    GCController* GetControllerWithPreferredLayout(GCController* a, GCController* b)
    {
        if (a == nil)
        {
            return b;
        }
        else if (b == nil)
        {
            return a;
        }

        if (a.extendedGamepad != nil)
        {
            return a;
        }
        else if (b.extendedGamepad != nil)
        {
            return b;
        }

        if (a.gamepad != nil)
        {
            return a;
        }
        else if (b.gamepad != nil)
        {
            return b;
        }

        // It doesn't matter
        return a;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    GCController* FindUnassignedController(bool isPrimaryPlayer)
    {
        GCController* preferredUnassignedController = nil;
        for (GCController* connectedController in GCController.controllers)
        {
            if (connectedController.playerIndex != GCControllerPlayerIndexUnset)
            {
                // This controller has already been assigned to a local player
                continue;
            }

            if (connectedController.attachedToDevice)
            {
                // A controller attached to the device...
                if (isPrimaryPlayer)
                {
                    // ...should always be first preference for the primary player...
                    return connectedController;
                }
                else
                {
                    // ...but should always be ignored for additional players.
                    continue;
                }
            }

            preferredUnassignedController = GetControllerWithPreferredLayout(
                                                preferredUnassignedController,
                                                connectedController);
        }

        return preferredUnassignedController;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    bool IsControllerStillConnected(GCController* controller)
    {
        for (GCController* connectedController in GCController.controllers)
        {
            if (connectedController == controller)
            {
                return true;
            }
        }
        return false;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    void InputDeviceGamepadApple::TickInputDevice()
    {
        if (m_controller == nil)
        {
            // Check if there are any connected controllers that have yet to be assigned to a player
            const AZ::u32 deviceIndex = GetInputDeviceIndex();
            m_controller = FindUnassignedController(deviceIndex == 0);
            if (m_controller == nil)
            {
                // Could not find any connected controllers for this player
                return;
            }

            // The controller connected since the last call to this function
            m_controller.controllerPausedHandler = ^(GCController* controller) { m_wasPausedHandlerCalled = true; };
            m_controller.playerIndex = static_cast<GCControllerPlayerIndex>(deviceIndex);

        #if defined(AZ_PLATFORM_APPLE_TV)
            // The micro gamepad profile is actually supported on macOS version 10.12 and above,
            // but this is above our min-spec and it's very unlikely to actually be used anyway.
            if (m_controller.microGamepad)
            {
                m_controller.microGamepad.reportsAbsoluteDpadValues = true;
                m_controller.microGamepad.allowsRotation = true;
            }
        #endif // defined(AZ_PLATFORM_APPLE_TV)

            BroadcastInputDeviceConnectedEvent();
        }
        else if (!IsControllerStillConnected(m_controller))
        {
            // The controller disconnected since the last call to this function
            m_controller = nil;
            m_rawGamepadState.Reset();
            ResetInputChannelStates();
            BroadcastInputDeviceDisconnectedEvent();
            return;
        }

        AZ_Assert(m_controller != nil, "Logic error in InputDeviceGamepadApple::TickInputDevice");

        // Always update the input channels while the game-pad is connected
        m_rawGamepadState.m_digitalButtonStates = 0;
        if (GCExtendedGamepad* extendedGamepad = m_controller.extendedGamepad)
        {
            if (extendedGamepad.dpad.up.pressed) { m_rawGamepadState.m_digitalButtonStates |= DIGITAL_BUTTON_MASK_DU; }
            if (extendedGamepad.dpad.down.pressed) { m_rawGamepadState.m_digitalButtonStates |= DIGITAL_BUTTON_MASK_DD; }
            if (extendedGamepad.dpad.left.pressed) { m_rawGamepadState.m_digitalButtonStates |= DIGITAL_BUTTON_MASK_DL; }
            if (extendedGamepad.dpad.right.pressed) { m_rawGamepadState.m_digitalButtonStates |= DIGITAL_BUTTON_MASK_DR; }

            if (extendedGamepad.leftShoulder.pressed) { m_rawGamepadState.m_digitalButtonStates |= DIGITAL_BUTTON_MASK_L1; }
            if (extendedGamepad.rightShoulder.pressed) { m_rawGamepadState.m_digitalButtonStates |= DIGITAL_BUTTON_MASK_R1; }
            // L3 and R3 (thumbstick buttons) are not currently supported by the apple Game Controller Framework,
            // but if support is added in future we should just need to do something like the following:
            //if (extendedGamepad.leftThumbstick.pressed) { m_rawGamepadState.m_digitalButtonStates |= DIGITAL_BUTTON_MASK_L3; }
            //if (extendedGamepad.rightThumbstick.pressed) { m_rawGamepadState.m_digitalButtonStates |= DIGITAL_BUTTON_MASK_R3; }

            if (extendedGamepad.buttonA.pressed) { m_rawGamepadState.m_digitalButtonStates |= DIGITAL_BUTTON_MASK_A; }
            if (extendedGamepad.buttonB.pressed) { m_rawGamepadState.m_digitalButtonStates |= DIGITAL_BUTTON_MASK_B; }
            if (extendedGamepad.buttonX.pressed) { m_rawGamepadState.m_digitalButtonStates |= DIGITAL_BUTTON_MASK_X; }
            if (extendedGamepad.buttonY.pressed) { m_rawGamepadState.m_digitalButtonStates |= DIGITAL_BUTTON_MASK_Y; }

            m_rawGamepadState.m_triggerButtonLState = extendedGamepad.leftTrigger.value;
            m_rawGamepadState.m_triggerButtonRState = extendedGamepad.rightTrigger.value;
            m_rawGamepadState.m_thumbStickLeftXState = extendedGamepad.leftThumbstick.xAxis.value;
            m_rawGamepadState.m_thumbStickLeftYState = extendedGamepad.leftThumbstick.yAxis.value;
            m_rawGamepadState.m_thumbStickRightXState = extendedGamepad.rightThumbstick.xAxis.value;
            m_rawGamepadState.m_thumbStickRightYState = extendedGamepad.rightThumbstick.yAxis.value;
        }
        else if (GCGamepad* gamepad = m_controller.gamepad)
        {
            if (gamepad.dpad.up.pressed) { m_rawGamepadState.m_digitalButtonStates |= DIGITAL_BUTTON_MASK_DU; }
            if (gamepad.dpad.down.pressed) { m_rawGamepadState.m_digitalButtonStates |= DIGITAL_BUTTON_MASK_DD; }
            if (gamepad.dpad.left.pressed) { m_rawGamepadState.m_digitalButtonStates |= DIGITAL_BUTTON_MASK_DL; }
            if (gamepad.dpad.right.pressed) { m_rawGamepadState.m_digitalButtonStates |= DIGITAL_BUTTON_MASK_DR; }

            if (gamepad.leftShoulder.pressed) { m_rawGamepadState.m_digitalButtonStates |= DIGITAL_BUTTON_MASK_L1; }
            if (gamepad.rightShoulder.pressed) { m_rawGamepadState.m_digitalButtonStates |= DIGITAL_BUTTON_MASK_R1; }

            if (gamepad.buttonA.pressed) { m_rawGamepadState.m_digitalButtonStates |= DIGITAL_BUTTON_MASK_A; }
            if (gamepad.buttonB.pressed) { m_rawGamepadState.m_digitalButtonStates |= DIGITAL_BUTTON_MASK_B; }
            if (gamepad.buttonX.pressed) { m_rawGamepadState.m_digitalButtonStates |= DIGITAL_BUTTON_MASK_X; }
            if (gamepad.buttonY.pressed) { m_rawGamepadState.m_digitalButtonStates |= DIGITAL_BUTTON_MASK_Y; }
        }
    #if defined(AZ_PLATFORM_APPLE_TV)
        // The micro gamepad profile is actually supported on macOS version 10.12 and above,
        // but this is above our min-spec and it's very unlikely to actually be used anyway.
        else if (GCMicroGamepad* microGamepad = m_controller.microGamepad)
        {
            if (microGamepad.dpad.up.pressed) { m_rawGamepadState.m_digitalButtonStates |= DIGITAL_BUTTON_MASK_DU; }
            if (microGamepad.dpad.down.pressed) { m_rawGamepadState.m_digitalButtonStates |= DIGITAL_BUTTON_MASK_DD; }
            if (microGamepad.dpad.left.pressed) { m_rawGamepadState.m_digitalButtonStates |= DIGITAL_BUTTON_MASK_DL; }
            if (microGamepad.dpad.right.pressed) { m_rawGamepadState.m_digitalButtonStates |= DIGITAL_BUTTON_MASK_DR; }

            if (microGamepad.buttonA.pressed) { m_rawGamepadState.m_digitalButtonStates |= DIGITAL_BUTTON_MASK_A; }
            if (microGamepad.buttonX.pressed) { m_rawGamepadState.m_digitalButtonStates |= DIGITAL_BUTTON_MASK_X; }
        }
    #endif // defined(AZ_PLATFORM_APPLE_TV)
        else
        {
            AZ_WarningOnce("InputDeviceGamepadApple", false, "Unknown game-pad profile");
        }

        if (m_wasPausedHandlerCalled)
        {
            m_rawGamepadState.m_digitalButtonStates |= DIGITAL_BUTTON_MASK_PAUSE;
            m_wasPausedHandlerCalled = false;
        }

        ProcessRawGamepadState(m_rawGamepadState);
    }
} // namespace AzFramework
