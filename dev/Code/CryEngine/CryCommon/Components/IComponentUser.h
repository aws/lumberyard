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
#ifndef CRYINCLUDE_CRYCOMMON_COMPONENTS_ICOMPONENTUSER_H
#define CRYINCLUDE_CRYCOMMON_COMPONENTS_ICOMPONENTUSER_H
#pragma once

#include <IComponent.h>

//! Deprecated
//! Provides a mechanism for games to add a custom component into game entities. It was done this way to
//! create an abstraction between engine side components and game specific components.
//! Currently it is used by CryAction to create the GameObject component.
struct IComponentUser
    : public IComponent
{
    // <interfuscator:shuffle>
    DECLARE_COMPONENT_TYPE("ComponentUser", 0x536C627644404E1E, 0x8176BFB53907F064)

    IComponentUser() {}

    ComponentEventPriority GetEventPriority(const int eventID) const override { return EntityEventPriority::User; }

    // </interfuscator:shuffle>
};

DECLARE_SMART_POINTERS(IComponentUser);

#endif // CRYINCLUDE_CRYCOMMON_COMPONENTS_ICOMPONENTUSER_H