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
#ifndef CRYINCLUDE_CRYINPUT_IOSKEYBOARD_H
#define CRYINCLUDE_CRYINPUT_IOSKEYBOARD_H
#pragma once

#include "InputDevice.h"

@class UITextField;
@class IosKeyboardTextFieldDelegate;

////////////////////////////////////////////////////////////////////////////////////////////////////
class CIosKeyboard
    : public CInputDevice
{
public:
    CIosKeyboard(IInput& input);
    ~CIosKeyboard() override;

    int GetDeviceIndex() const override { return 0; }
    bool Init() override;

    void StartTextInput(const Vec2& topLeft = ZERO, const Vec2& bottomRight = ZERO);
    void StopTextInput();
    bool IsScreenKeyboardShowing() const;
    bool IsScreenKeyboardSupported() const;

private:
    UITextField* m_textField = nullptr;
    IosKeyboardTextFieldDelegate* m_textFieldDelegate = nullptr;
};

#endif // CRYINCLUDE_CRYINPUT_IOSKEYBOARD_H
