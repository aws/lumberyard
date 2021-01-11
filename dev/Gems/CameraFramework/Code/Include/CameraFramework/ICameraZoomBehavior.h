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
#include "CameraFramework/ICameraSubComponent.h"
#include <AzCore/Math/Transform.h>

namespace Camera
{
    //////////////////////////////////////////////////////////////////////////
    // This class is responsible for modifying the fov of a camera
    //////////////////////////////////////////////////////////////////////////
    class ICameraZoomBehavior
        : public ICameraSubComponent
    {
    public:
        AZ_RTTI(ICameraZoomBehavior, "{D7CED09E-B6B0-42CE-995F-B4FD235C2EC3}", ICameraSubComponent);
        virtual ~ICameraZoomBehavior() = default;

        // Modify the zoom (passes in the current zoom, this value should be modified by the behaviour)
        virtual void ModifyZoom(const AZ::Transform& targetTransform, float& inOutZoom) = 0;
    };
} //namespace Camera  
