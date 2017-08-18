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
#include "StdAfx.h"

#include "IosKeyboard.h"

#include "UnicodeIterator.h"

#include <IRenderer.h>

#include <AzCore/Math/MathUtils.h>

#include <UIKit/UIKit.h>

////////////////////////////////////////////////////////////////////////////////////////////////////
@interface IosKeyboardTextFieldDelegate : NSObject <UITextFieldDelegate>
{
    UITextField* m_textField;
    IInput* m_inputInterface;
@public
    float m_activeInputFieldBottomNormalized;
}
- (id)initWithTextField: (UITextField*)textField withInputInterface: (IInput*)inputInterface;
- (void)keyboardWillChangeFrame: (NSNotification*)notification;
- (BOOL)textFieldShouldClear: (UITextField*)textField;
- (BOOL)textFieldShouldReturn: (UITextField*)textField;
- (BOOL)textField: (UITextField*)textField shouldChangeCharactersInRange: (NSRange)range replacementString: (NSString*)string;
@end

////////////////////////////////////////////////////////////////////////////////
@implementation IosKeyboardTextFieldDelegate

////////////////////////////////////////////////////////////////////////////////
- (id)initWithTextField: (UITextField*)textField withInputInterface: (IInput*)inputInterface
{
    if ((self = [super init]))
    {
        self->m_textField = textField;
        self->m_inputInterface = inputInterface;

        // Resgister to be notified when the keyboard frame size changes so we
        // can adjust the position of the view accordingly to ensure we don't
        // obscure the text field being edited. We no don't need to explicitly
        // remove the observer:
        // https://developer.apple.com/library/mac/releasenotes/Foundation/RN-Foundation/index.html#10_11NotificationCenter
        [[NSNotificationCenter defaultCenter] addObserver: self
                                                 selector: @selector(keyboardWillChangeFrame:)
                                                     name: UIKeyboardWillChangeFrameNotification
                                                   object: nil];
    }

    return self;
}

////////////////////////////////////////////////////////////////////////////////
- (void)keyboardWillChangeFrame: (NSNotification*)notification
{
    if (!m_textField || !m_textField.superview)
    {
        return;
    }

    // Get the keyboard rect in terms of the view to account for orientation.
    CGRect keyboardRect = [notification.userInfo[UIKeyboardFrameEndUserInfoKey] CGRectValue];
    keyboardRect = [m_textField.superview convertRect: keyboardRect fromView: nil];

    // Calculate how far we need to push the view up to ensure
    // active input field is not being covered by the keyboard.
    const double activeInputFieldBottom = m_activeInputFieldBottomNormalized * m_textField.superview.bounds.size.height;
    const double offsetY = AZ::GetMin(0.0, keyboardRect.origin.y - activeInputFieldBottom);

    // Create the offset view rect and transform it
    // into the coordinate space of the main window.
    CGRect offsetViewRect = CGRectMake(0, offsetY, m_textField.superview.bounds.size.width, m_textField.superview.bounds.size.height);
    offsetViewRect = [m_textField.superview convertRect: offsetViewRect toView: nil];

    // Remove any existing offset applied in previous calls to this function.
    offsetViewRect.origin.x -= m_textField.superview.frame.origin.x;
    offsetViewRect.origin.y -= m_textField.superview.frame.origin.y;

    m_textField.superview.frame = offsetViewRect;
}

////////////////////////////////////////////////////////////////////////////////
- (BOOL)textFieldShouldClear: (UITextField*)textField
{
    // ToDo: We should send an event here to indicate that we wish to clear all
    // existing text, but this would be adding new functionality so we'll leave
    // it as unsupported for the time being.
    return FALSE;
}

////////////////////////////////////////////////////////////////////////////////
- (BOOL)textFieldShouldReturn: (UITextField*)textField
{
    // Post a regular (not unicode) 'enter' keyboard event.
    SInputEvent event;
    event.deviceType = eIDT_Keyboard;
    event.keyId = eKI_Enter;
    event.keyName = "enter";

    event.state = eIS_Pressed;
    event.value = 1.0f;
    m_inputInterface->PostInputEvent(event);

    event.state = eIS_Released;
    event.value = 0.0f;
    m_inputInterface->PostInputEvent(event);

    return FALSE;
}

////////////////////////////////////////////////////////////////////////////////
- (BOOL)textField: (UITextField*)textField shouldChangeCharactersInRange: (NSRange)range replacementString: (NSString*)string
{
    NSUInteger stringLength = string.length;
    if (stringLength == 0)
    {
        // The user has pressed the backspace key on the virtual keyboard.
        SUnicodeEvent event('\b');
        m_inputInterface->PostUnicodeEvent(event);

        // Return false so that the text field itself does not update.
        return FALSE;
    }

    // Iterate over and send an event for each unicode codepoint of the new text.
    //const char* utf8String = string.UTF8String;
    for (Unicode::CIterator<const char*, false> it(string.UTF8String); *it != 0; ++it)
    {
        const uint32_t codepoint = *it;
        SUnicodeEvent event(codepoint);
        m_inputInterface->PostUnicodeEvent(event);
    }

    // Return false so that the text field itself does not update.
    return FALSE;
}
@end // IosKeyboardTextFieldDelegate Implementation

////////////////////////////////////////////////////////////////////////////////////////////////////
CIosKeyboard::CIosKeyboard(IInput& input)
    : CInputDevice(input, "iOS Keyboard")
{
}

////////////////////////////////////////////////////////////////////////////////////////////////////
CIosKeyboard::~CIosKeyboard()
{
    if (m_textField && m_textFieldDelegate)
    {
        m_textField.delegate = nullptr;
        [m_textFieldDelegate release];
        m_textFieldDelegate = nullptr;

        [m_textField removeFromSuperview];
        [m_textField release];
        m_textField = nullptr;
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
bool CIosKeyboard::Init()
{
    // Get the applications root view.
    UIWindow* window = [[UIApplication sharedApplication] keyWindow];
    UIView* rootView = window ? window.rootViewController.view : nullptr;
    if (!rootView)
    {
        return false;
    }

    // Create UITextField that we can call becomeFirstResponder on to show the keyboard.
    m_textField = [[UITextField alloc] initWithFrame: CGRectZero];

    // Create and set the text fields delegate so we can respond to keyboard input.
    m_textFieldDelegate = [[IosKeyboardTextFieldDelegate alloc] initWithTextField: m_textField withInputInterface: &GetIInput()];
    m_textField.delegate = m_textFieldDelegate;

    // Disable autocapitalization and autocorrection, which both behave strangely.
    m_textField.autocapitalizationType = UITextAutocapitalizationTypeNone;
    m_textField.autocorrectionType = UITextAutocorrectionTypeNo;

    // Hide the text field so it will never actually be shown.
    m_textField.hidden = YES;

    // Add something to the text field so delete works.
    m_textField.text = @" ";

    // Add the text field to the root view. This assumes the root view
    // (which is created and added in MetalDevice) will never change.
    [rootView addSubview: m_textField];

    return true;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void CIosKeyboard::StartTextInput(const Vec2& inputRectTopLeft, const Vec2& inputRectBottomRight)
{
    if (m_textField)
    {
        // We must set m_activeInputFieldBottomNormalized before calling becomeFirstResponder,
        // which causes a UIKeyboardWillChangeFrameNotification to be sent.
        m_textFieldDelegate->m_activeInputFieldBottomNormalized = inputRectBottomRight.y / static_cast<float>(gEnv->pRenderer->GetHeight());
        [m_textField becomeFirstResponder];
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void CIosKeyboard::StopTextInput()
{
    if (m_textField)
    {
        // We must set m_activeInputFieldBottomNormalized before calling becomeFirstResponder,
        // which causes a UIKeyboardWillChangeFrameNotification to be sent.
        m_textFieldDelegate->m_activeInputFieldBottomNormalized = 0.0f;
        [m_textField resignFirstResponder];
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
bool CIosKeyboard::IsScreenKeyboardShowing() const
{
    return m_textField ? m_textField.isFirstResponder : false;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
bool CIosKeyboard::IsScreenKeyboardSupported() const
{
    return true;
}
