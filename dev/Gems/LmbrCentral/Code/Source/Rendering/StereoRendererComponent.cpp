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

#include "LmbrCentral_precompiled.h"
#include <StereoRendererBus.h>
#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/Serialization/SerializeContext.h>

#include "StereoRendererComponent.h"

using namespace AZ;

namespace LmbrCentral
{
    void StereoRendererComponent::Reflect(ReflectContext* context)
    {
        SerializeContext* serializeContext = azrtti_cast<SerializeContext*>(context);
        if (serializeContext)
        {
            serializeContext->Class<StereoRendererComponent, AZ::Component>()
                ->Version(1);
        }

        AZ::BehaviorContext* behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context);
        if (behaviorContext)
        {
            behaviorContext->EBus<StereoRendererRequestBus>("StereoRendererRequestBus")
                ->Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::Preview)
                ->Event("IsRenderingToHMD", &StereoRendererRequestBus::Events::IsRenderingToHMD);
        }
    }
}
