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

#include <AzCore/EBus/EBus.h>
#include <AzCore/Component/ComponentBus.h>

namespace EMotionFX
{
    class MotionInstance;

    namespace Integration
    {
        class SimpleMotionComponentRequests
            : public AZ::ComponentBus
        {
        public:
            virtual void LoopMotion(bool enable) = 0;
            virtual void RetargetMotion(bool enable) = 0;
            virtual void ReverseMotion(bool enable) = 0;
            virtual void MirrorMotion(bool enable) = 0;
            virtual void SetPlaySpeed(float speed) = 0;
        };
        using SimpleMotionComponentRequestBus = AZ::EBus<SimpleMotionComponentRequests>;
    }
}