/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates, or
* a third party where indicated.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/

#include <AzFramework/Physics/World.h>
#include <AzFramework/Physics/ShapeConfiguration.h>
#include <AzFramework/Physics/Casts.h>
#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/std/sort.h>

namespace Physics
{
    /// This structure is used only for reflecting type vector<RayCastHit> to 
    /// serialize and behavior context. It's not used in the API anywhere
    struct RaycastHitArray
    {
        AZ_TYPE_INFO(RaycastHitArray, "{BAFCC4E7-A06B-4909-B2AE-C89D9E84FE4E}");
        AZStd::vector<Physics::RayCastHit> m_hitArray;
    };

    void RayCastHit::Reflect(AZ::ReflectContext* context)
    {
        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<RayCastHit>()
                ->Field("Distance", &RayCastHit::m_distance)
                ->Field("Position", &RayCastHit::m_position)
                ->Field("Normal", &RayCastHit::m_normal)
                ;

            serializeContext->Class<RaycastHitArray>()
                ->Field("HitArray", &RaycastHitArray::m_hitArray)
                ;
        }

        if (auto behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
        {
            behaviorContext->Class<RayCastHit>("RaycastHit")
                ->Property("Distance", BehaviorValueProperty(&RayCastHit::m_distance))
                ->Property("Position", BehaviorValueProperty(&RayCastHit::m_position))
                ->Property("Normal", BehaviorValueProperty(&RayCastHit::m_normal))
                ->Property("EntityId", [](RayCastHit& result) {return result.m_body->GetEntityId(); }, nullptr)
                ;

            behaviorContext->Class<RaycastHitArray>()
                ->Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::All)
                ->Property("HitArray", BehaviorValueProperty(&RaycastHitArray::m_hitArray))
                ;
        }
    }

} // namespace Physics
