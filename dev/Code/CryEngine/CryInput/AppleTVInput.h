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
#ifndef CRYINCLUDE_CRYINPUT_APPLETVINPUT_H
#define CRYINCLUDE_CRYINPUT_APPLETVINPUT_H
#pragma once

#ifdef USE_APPLETVINPUT

#include "BaseInput.h"

////////////////////////////////////////////////////////////////////////////////////////////////////
class CAppleTVInput
    : public CBaseInput
{
public:
    CAppleTVInput(ISystem* pSystem);
    ~CAppleTVInput() override;

    bool Init() override;
    void ShutDown() override;
};

#endif // USE_APPLETVINPUT

#endif // CRYINCLUDE_CRYINPUT_APPLETVINPUT_H
