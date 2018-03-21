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
// Original file Copyright Crytek GMBH or its affiliates, used under license.

// Description : Interface to create game framework extension

#ifndef __IGameFrameworkExtension_h__
#define __IGameFrameworkExtension_h__
#pragma once

#include <CryExtension/ICryUnknown.h>

struct IGameFramework;

struct IGameFrameworkExtensionCreator
    : public ICryUnknown
{
    CRYINTERFACE_DECLARE(IGameFrameworkExtensionCreator, 0x86197E35AD024DA8, 0xA8D483A98F424FFD);

    // Description
    //  Creates an extension and returns the interface to it (extension interface must derivate for ICryUnknown)
    // Parameters
    //  pIGameFramework - Pointer to game framework interface, so the new extension can be registered against it
    // Return
    //  ICryUnknown pointer to just created extension (it can be safely casted with cryinterface_cast< > to the corresponding interface)
    virtual ICryUnknown* Create(IGameFramework* pIGameFramework) = 0;
};
DECLARE_SMART_POINTERS(IGameFrameworkExtensionCreator);

#endif //__IGameFrameworkExtension_h__