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
#pragma once
#include <AzCore/RTTI/RTTI.h>
#include <AzCore/std/typetraits/is_unsigned.h>

// CryCommon includes
#include <InputEventBus.h>

namespace Input
{
    //////////////////////////////////////////////////////////////////////////
    /// Classes that inherit from this one will share the life-cycle API's
    /// with components.  Components that contain the subclasses are expected
    /// to call these methods in their Init/Activate/Deactivate methods
    //////////////////////////////////////////////////////////////////////////
    class InputSubComponent
    {
    public:
        AZ_RTTI(InputSubComponent, "{3D0F14F8-AE29-4ECC-BC88-26B8F8168398}");
        virtual ~InputSubComponent() = default;

        //////////////////////////////////////////////////////////////////////////
        /// InputSubComponents will share the life-cycle API's of components.
        /// Any Component that contains an InputSubComponent is expected to call
        /// these methods in their Activate/Deactivate methods
        virtual void Activate(const AZ::InputEventNotificationId& channel) = 0;
        virtual void Deactivate(const AZ::InputEventNotificationId& channel) = 0;
    };
} // namespace Input
