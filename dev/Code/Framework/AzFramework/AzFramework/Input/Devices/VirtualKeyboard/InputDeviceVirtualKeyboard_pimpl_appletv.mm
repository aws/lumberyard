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

#include <AzFramework/Input/Devices/VirtualKeyboard/InputDeviceVirtualKeyboard.h>

////////////////////////////////////////////////////////////////////////////////////////////////////
namespace AzFramework
{
    ////////////////////////////////////////////////////////////////////////////////////////////////
    //! Platform specific implementation for appletv virtual keyboard input devices
    class InputDeviceVirtualKeyboardAppleTV : public InputDeviceVirtualKeyboard::Implementation
    {
    public:
        ////////////////////////////////////////////////////////////////////////////////////////////
        // Allocator
        AZ_CLASS_ALLOCATOR(InputDeviceVirtualKeyboardAppleTV, AZ::SystemAllocator, 0);

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Constructor
        //! \param[in] inputDevice Reference to the input device being implemented
        InputDeviceVirtualKeyboardAppleTV(InputDeviceVirtualKeyboard& inputDevice);

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Destructor
        ~InputDeviceVirtualKeyboardAppleTV() override;

    private:
        ////////////////////////////////////////////////////////////////////////////////////////////
        //! \ref AzFramework::InputDeviceVirtualKeyboard::Implementation::IsConnected
        bool IsConnected() const override;

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! \ref AzFramework::InputDeviceVirtualKeyboard::Implementation::TextEntryStarted
        void TextEntryStarted(float activeTextFieldNormalizedBottomY = 0.0f) override;

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! \ref AzFramework::InputDeviceVirtualKeyboard::Implementation::TextEntryStopped
        void TextEntryStopped() override;

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! \ref AzFramework::InputDeviceVirtualKeyboard::Implementation::TickInputDevice
        void TickInputDevice() override;
    };

    ////////////////////////////////////////////////////////////////////////////////////////////////
    InputDeviceVirtualKeyboard::Implementation* InputDeviceVirtualKeyboard::Implementation::Create(
        InputDeviceVirtualKeyboard& inputDevice)
    {
        return aznew InputDeviceVirtualKeyboardAppleTV(inputDevice);
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    InputDeviceVirtualKeyboardAppleTV::InputDeviceVirtualKeyboardAppleTV(
        InputDeviceVirtualKeyboard& inputDevice)
        : InputDeviceVirtualKeyboard::Implementation(inputDevice)
    {
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    InputDeviceVirtualKeyboardAppleTV::~InputDeviceVirtualKeyboardAppleTV()
    {
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    bool InputDeviceVirtualKeyboardAppleTV::IsConnected() const
    {
        // ToDo: return true if the virtual keyboard is active
        return false;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    void InputDeviceVirtualKeyboardAppleTV::TextEntryStarted(float /*activeTextFieldNormalizedBottomY*/)
    {
        // ToDo: Activate the virtual keyboard
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    void InputDeviceVirtualKeyboardAppleTV::TextEntryStopped()
    {
        // ToDo: Deactivate the virtual keyboard
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    void InputDeviceVirtualKeyboardAppleTV::TickInputDevice()
    {
        // ToDo: Process raw input and update input channels
    }
} // namespace AzFramework
