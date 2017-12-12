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
#ifndef CRYINCLUDE_CRYINPUT_ANDROIDKEYBOARD_H
#define CRYINCLUDE_CRYINPUT_ANDROIDKEYBOARD_H
#pragma once

#include "InputDevice.h"

#include <AzCore/std/containers/vector.h>
#include <AzCore/std/parallel/mutex.h>
#include <AzCore/std/smart_ptr/unique_ptr.h>

#include <AzInput/KeyboardEvents.h>


namespace AZ
{
    namespace Android
    {
        namespace JNI
        {
           class Object;
        }
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
class CAndroidKeyboard
    : public CInputDevice
    , public AzInput::RawVirtualKeyboardNotificationsBus::Handler
{
public:
    explicit CAndroidKeyboard(IInput& input);
    ~CAndroidKeyboard() override;

    int GetDeviceIndex() const override { return 0; }
    bool Init() override;

    void Update(bool) override;

    void OnUnicodeEvent(const AzInput::UnicodeEvent& unicodeEvent) override;
    void OnKeyEvent(const SInputEvent& keyEvent) override;

    void StartTextInput(const Vec2& topLeft = ZERO, const Vec2& bottomRight = ZERO);
    void StopTextInput();
    bool IsScreenKeyboardShowing() const;
    bool IsScreenKeyboardSupported() const;


private:
    AZStd::unique_ptr<AZ::Android::JNI::Object> m_keyboardHandler;

    AZStd::mutex m_unicodeEventsMutex;
    AZStd::vector<AzInput::UnicodeEvent> m_unicodeEvents;

    AZStd::mutex m_keyEventsMutex;
    AZStd::vector<SInputEvent> m_keyEvents;
};

#endif // CRYINCLUDE_CRYINPUT_ANDROIDKEYBOARD_H
