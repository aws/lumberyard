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
#ifndef CRYINCLUDE_CRYINPUT_SDLINPUT_H
#define CRYINCLUDE_CRYINPUT_SDLINPUT_H
#pragma once

#if defined(USE_SDLINPUT)

#include "BaseInput.h"
#include "InputDevice.h"

////////////////////////////////////////////////////////////////////////////////////////////////////
class CSDLInput
    : public CBaseInput
{
public:
    CSDLInput(ISystem* pSystem);
    ~CSDLInput() override;

    bool Init() override;
    void ShutDown() override;
    void Update(bool focus) override;
};

////////////////////////////////////////////////////////////////////////////////////////////////////
class CSDLInputDevice
    : public CInputDevice
{
public:
    CSDLInputDevice(IInput& input, const char* deviceName);
    ~CSDLInputDevice() override;

protected:
    void PostEvent(SInputSymbol* pSymbol, unsigned keyMod = ~0);
};

#endif // defined(USE_SDLINPUT)

#endif // CRYINCLUDE_CRYINPUT_SDLINPUT_H
