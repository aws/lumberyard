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
#ifndef CRYINCLUDE_CRYINPUT_IOSINPUT_H
#define CRYINCLUDE_CRYINPUT_IOSINPUT_H
#pragma once

#ifdef USE_IOSINPUT

#include "MobileInput.h"

class CIosKeyboard;

////////////////////////////////////////////////////////////////////////////////////////////////////
class CIosInput
    : public CMobileInput
{
public:
    CIosInput(ISystem* pSystem);
    ~CIosInput() override;

    bool Init() override;
    void ShutDown() override;
    void Update(bool bFocus) override;

    void StartTextInput(const Vec2& topLeft = ZERO, const Vec2& bottomRight = ZERO) override;
    void StopTextInput() override;
    bool IsScreenKeyboardShowing() const override;
    bool IsScreenKeyboardSupported() const override;

private:
    CIosKeyboard* m_keyboardDevice = nullptr;
};

#endif // USE_IOSINPUT

#endif // CRYINCLUDE_CRYINPUT_IOSINPUT_H
