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
#include "CryLegacy_precompiled.h"

#include "AzToLyInputDeviceTouch.h"

#include <AzFramework/Input/Devices/Touch/InputDeviceTouch.h>

using namespace AzFramework;

////////////////////////////////////////////////////////////////////////////////////////////////////
AzToLyInputDeviceTouch::AzToLyInputDeviceTouch(IInput& input)
    : AzToLyInputDevice(input, eIDT_TouchScreen, "touch", InputDeviceTouch::Id)
{
    MapSymbol(InputDeviceTouch::Touch::Index0.GetNameCrc32(), eKI_Touch0, "touch0");
    MapSymbol(InputDeviceTouch::Touch::Index1.GetNameCrc32(), eKI_Touch1, "touch1");
    MapSymbol(InputDeviceTouch::Touch::Index2.GetNameCrc32(), eKI_Touch2, "touch2");
    MapSymbol(InputDeviceTouch::Touch::Index3.GetNameCrc32(), eKI_Touch3, "touch3");
    MapSymbol(InputDeviceTouch::Touch::Index4.GetNameCrc32(), eKI_Touch4, "touch4");
    MapSymbol(InputDeviceTouch::Touch::Index5.GetNameCrc32(), eKI_Touch5, "touch5");
    MapSymbol(InputDeviceTouch::Touch::Index6.GetNameCrc32(), eKI_Touch6, "touch6");
    MapSymbol(InputDeviceTouch::Touch::Index7.GetNameCrc32(), eKI_Touch7, "touch7");
    MapSymbol(InputDeviceTouch::Touch::Index8.GetNameCrc32(), eKI_Touch8, "touch8");
    MapSymbol(InputDeviceTouch::Touch::Index9.GetNameCrc32(), eKI_Touch9, "touch9");
}

////////////////////////////////////////////////////////////////////////////////////////////////////
AzToLyInputDeviceTouch::~AzToLyInputDeviceTouch()
{
}
