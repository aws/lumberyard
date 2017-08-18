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
#include <AzFramework/Input/Buses/Requests/RawInputRequestBus_ios.h>

#include <UIKit/UIKit.h>

////////////////////////////////////////////////////////////////////////////////////////////////////
@interface VirtualKeyboardTextFieldDelegate : NSObject <UITextFieldDelegate>
{
    UITextField* m_textField;
    AzFramework::InputDeviceVirtualKeyboard::Implementation* m_inputDevice;
@public
    float m_activeTextFieldNormalizedBottomY;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
- (id)initWithTextField: (UITextField*)textField
        withInputDevice: (AzFramework::InputDeviceVirtualKeyboard::Implementation*)inputDevice;

////////////////////////////////////////////////////////////////////////////////////////////////////
- (void)keyboardWillChangeFrame: (NSNotification*)notification;

////////////////////////////////////////////////////////////////////////////////////////////////////
- (BOOL)textFieldShouldClear: (UITextField*)textField;

////////////////////////////////////////////////////////////////////////////////////////////////////
- (BOOL)textFieldShouldReturn: (UITextField*)textField;

////////////////////////////////////////////////////////////////////////////////////////////////////
- (BOOL)textField: (UITextField*)textField
        shouldChangeCharactersInRange: (NSRange)range
        replacementString: (NSString*)string;
@end // VirtualKeyboardTextFieldDelegate interface

////////////////////////////////////////////////////////////////////////////////////////////////////
@implementation VirtualKeyboardTextFieldDelegate

////////////////////////////////////////////////////////////////////////////////////////////////////
- (id)initWithTextField: (UITextField*)textField
        withInputDevice: (AzFramework::InputDeviceVirtualKeyboard::Implementation*)inputDevice
{
    if ((self = [super init]))
    {
        self->m_textField = textField;
        self->m_inputDevice = inputDevice;

        // Resgister to be notified when the keyboard frame size changes so we can then adjust the
        // position of the view accordingly to ensure we don't obscure the text field being edited.
        // We don't need to explicitly remove the observer:
        // https://developer.apple.com/library/mac/releasenotes/Foundation/RN-Foundation/index.html#10_11NotificationCenter
        [[NSNotificationCenter defaultCenter] addObserver: self
                                                 selector: @selector(keyboardWillChangeFrame:)
                                                     name: UIKeyboardWillChangeFrameNotification
                                                   object: nil];
    }

    return self;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
- (void)keyboardWillChangeFrame: (NSNotification*)notification
{
    if (!m_textField || !m_textField.superview)
    {
        return;
    }

    // Get the keyboard rect in terms of the view to account for orientation.
    CGRect keyboardRect = [notification.userInfo[UIKeyboardFrameEndUserInfoKey] CGRectValue];
    keyboardRect = [m_textField.superview convertRect: keyboardRect fromView: nil];

    // Calculate the offset needed so the active text field is not being covered by the keyboard.
    const double activeTextFieldBottom = m_activeTextFieldNormalizedBottomY * m_textField.superview.bounds.size.height;
    const double offsetY = AZ::GetMin(0.0, keyboardRect.origin.y - activeTextFieldBottom);

    // Create the offset view rect and transform it into the coordinate space of the main window.
    CGRect offsetViewRect = CGRectMake(0, offsetY, m_textField.superview.bounds.size.width,
                                                   m_textField.superview.bounds.size.height);
    offsetViewRect = [m_textField.superview convertRect: offsetViewRect toView: nil];

    // Remove any existing offset applied in previous calls to this function.
    offsetViewRect.origin.x -= m_textField.superview.frame.origin.x;
    offsetViewRect.origin.y -= m_textField.superview.frame.origin.y;

    m_textField.superview.frame = offsetViewRect;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
- (BOOL)textFieldShouldClear: (UITextField*)textField
{
    // Queue an 'clear' command event.
    m_inputDevice->QueueRawCommandEvent(AzFramework::InputDeviceVirtualKeyboard::Command::EditClear);

    // Return false so that the text field itself does not update.
    return FALSE;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
- (BOOL)textFieldShouldReturn: (UITextField*)textField
{
    // Queue an 'enter' command event.
    m_inputDevice->QueueRawCommandEvent(AzFramework::InputDeviceVirtualKeyboard::Command::EditEnter);

    // Return false so that the text field itself does not update.
    return FALSE;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
- (BOOL)textField: (UITextField*)textField
        shouldChangeCharactersInRange: (NSRange)range
        replacementString: (NSString*)string
{
    // If the string length is 0, the user has pressed the backspace key on the virtual keyboard.
    const AZStd::string textUTF8 = string.length ? string.UTF8String : "\b";
    m_inputDevice->QueueRawTextEvent(textUTF8);

    // Return false so that the text field itself does not update.
    return FALSE;
}
@end // VirtualKeyboardTextFieldDelegate implementation

////////////////////////////////////////////////////////////////////////////////////////////////////
namespace AzFramework
{
    ////////////////////////////////////////////////////////////////////////////////////////////////
    //! Platform specific implementation for ios virtual keyboard input devices
    class InputDeviceVirtualKeyboardIos : public InputDeviceVirtualKeyboard::Implementation
    {
    public:
        ////////////////////////////////////////////////////////////////////////////////////////////
        // Allocator
        AZ_CLASS_ALLOCATOR(InputDeviceVirtualKeyboardIos, AZ::SystemAllocator, 0);

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Constructor
        //! \param[in] inputDevice Reference to the input device being implemented
        InputDeviceVirtualKeyboardIos(InputDeviceVirtualKeyboard& inputDevice);

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Destructor
        ~InputDeviceVirtualKeyboardIos() override;

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

        ////////////////////////////////////////////////////////////////////////////////////////////
        // Variables
        UITextField* m_textField = nullptr;
        VirtualKeyboardTextFieldDelegate* m_textFieldDelegate = nullptr;
    };

    ////////////////////////////////////////////////////////////////////////////////////////////////
    InputDeviceVirtualKeyboard::Implementation* InputDeviceVirtualKeyboard::Implementation::Create(
        InputDeviceVirtualKeyboard& inputDevice)
    {
        return aznew InputDeviceVirtualKeyboardIos(inputDevice);
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    InputDeviceVirtualKeyboardIos::InputDeviceVirtualKeyboardIos(
        InputDeviceVirtualKeyboard& inputDevice)
        : InputDeviceVirtualKeyboard::Implementation(inputDevice)
        , m_textField(nullptr)
        , m_textFieldDelegate(nullptr)
    {
        // Create a UITextField that we can call becomeFirstResponder on to show the keyboard.
        m_textField = [[UITextField alloc] initWithFrame: CGRectZero];

        // Create and set the text field's delegate so we can respond to keyboard input.
        m_textFieldDelegate = [[VirtualKeyboardTextFieldDelegate alloc] initWithTextField: m_textField withInputDevice: this];
        m_textField.delegate = m_textFieldDelegate;

        // Disable autocapitalization and autocorrection, which both behave strangely.
        m_textField.autocapitalizationType = UITextAutocapitalizationTypeNone;
        m_textField.autocorrectionType = UITextAutocorrectionTypeNo;

        // Hide the text field so it will never actually be shown.
        m_textField.hidden = YES;

        // Add something to the text field so delete works.
        m_textField.text = @" ";
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    InputDeviceVirtualKeyboardIos::~InputDeviceVirtualKeyboardIos()
    {
        if (m_textField)
        {
            m_textField.delegate = nullptr;
            [m_textFieldDelegate release];
            m_textFieldDelegate = nullptr;

            [m_textField removeFromSuperview];
            [m_textField release];
            m_textField = nullptr;
        }
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    bool InputDeviceVirtualKeyboardIos::IsConnected() const
    {
        return m_textField ? m_textField.isFirstResponder : false;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    void InputDeviceVirtualKeyboardIos::TextEntryStarted(float activeTextFieldNormalizedBottomY)
    {
        // Get the application's root view.
        UIWindow* window = [[UIApplication sharedApplication] keyWindow];
        UIView* rootView = window ? window.rootViewController.view : nullptr;
        if (!rootView)
        {
            return;
        }

        // Add the text field to the root view.
        [rootView addSubview: m_textField];

        // We must set m_activeTextFieldNormalizedBottomY before calling becomeFirstResponder,
        // which causes a UIKeyboardWillChangeFrameNotification to be sent.
        m_textFieldDelegate->m_activeTextFieldNormalizedBottomY = activeTextFieldNormalizedBottomY;
        [m_textField becomeFirstResponder];
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    void InputDeviceVirtualKeyboardIos::TextEntryStopped()
    {
        // We must set m_activeTextFieldNormalizedBottomY before calling resignFirstResponder,
        // which causes a UIKeyboardWillChangeFrameNotification to be sent.
        m_textFieldDelegate->m_activeTextFieldNormalizedBottomY = 0.0f;
        [m_textField resignFirstResponder];
        [m_textField removeFromSuperview];
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    void InputDeviceVirtualKeyboardIos::TickInputDevice()
    {
        // This may not actually be strictly necessary for the virtual keyboard, but it can't hurt.
        //
        // Pump the ios event loop to ensure that it has dispatched all input events. Other systems
        // (or other input devices) may also do this so some (or all) input events may have already
        // been dispatched, but they are queued until ProcessRawEventQueues is called below so that
        // all raw input events are processed at the same time every frame.
        RawInputRequestBusIos::Broadcast(&RawInputRequestsIos::PumpRawEventLoop);

        // Process the raw event queues once each frame
        ProcessRawEventQueues();
    }
} // namespace AzFramework
