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

#include "Precompiled.h"

#include "Shadows.h"
#include <MathConversion.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/RTTI/BehaviorContext.h>
#include <IRenderer.h>
#include <ISystem.h>
#include <I3DEngine.h>

namespace GraphicsReflectContext
{
    void Shadows::RecomputeStaticShadows(const AZ::Aabb& bounds, float nextCascadeScale)
    {
        gEnv->p3DEngine->SetCachedShadowBounds(AABB(AZVec3ToLYVec3(bounds.GetMin()), AZVec3ToLYVec3(bounds.GetMax())), nextCascadeScale);
    }

    void Shadows::Reflect(AZ::ReflectContext* context)
    {
        if (AZ::BehaviorContext* behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
        {
            behaviorContext->Class<Shadows>()
                ->Attribute(AZ::Script::Attributes::Category, "Rendering")
                ->Method("RecomputeStaticShadows", &RecomputeStaticShadows,
                    { { 
                        { "Bounds", "An AABB that specifies the area where shadow maps should be recomputed", aznew AZ::BehaviorDefaultValue(AZ::Aabb::CreateNull()) },
                        { "NextCascadeScale", "Bounding boxes for subsequent cached cascades are scaled versions of the preceding cascades, based on this multiplier", aznew AZ::BehaviorDefaultValue(2.0f) }
                    } } )
                    ->Attribute(AZ::Script::Attributes::ToolTip, "Triggers recalculation of cached shadow maps")
                ;

        }
    }
}

