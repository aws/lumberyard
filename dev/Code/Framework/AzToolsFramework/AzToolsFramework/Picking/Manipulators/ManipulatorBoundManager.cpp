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

#include <AzCore/Math/Transform.h>
#include <AzCore/RTTI/RTTI.h>
#include <AzCore/std/smart_ptr/make_shared.h>

#include <AzToolsFramework/Picking/Manipulators/ManipulatorBoundManager.h>
#include <AzToolsFramework/Picking/Manipulators/ManipulatorBounds.h>

namespace AzToolsFramework
{
    namespace Picking
    {
        ManipulatorBoundManager::ManipulatorBoundManager(AZ::u32 manipulatorManagerId)
            : DefaultContextBoundManager(manipulatorManagerId)
        { }

        AZStd::shared_ptr<BoundShapeInterface> ManipulatorBoundManager::CreateShape(const BoundRequestShapeBase& shapeData, RegisteredBoundId id, AZ::u64 /*userContext*/)
        {
            AZ_Assert(id != InvalidBoundId, "Invalid Bound Id!");

            if (azrtti_istypeof<BoundShapeBox>(&shapeData))
            {
                AZStd::shared_ptr<BoundShapeInterface> box = AZStd::make_shared<ManipulatorBoundBox>(id);
                box->SetShapeData(shapeData);
                m_bounds.push_back(box);
                return box;
            }
            else if (azrtti_istypeof<BoundShapeCylinder>(&shapeData))
            {
                AZStd::shared_ptr<BoundShapeInterface> cylinder = AZStd::make_shared<ManipulatorBoundCylinder>(id);
                cylinder->SetShapeData(shapeData);
                m_bounds.push_back(cylinder);
                return cylinder;
            }
            else if (azrtti_istypeof<BoundShapeCone>(&shapeData))
            {
                AZStd::shared_ptr<BoundShapeInterface> cone = AZStd::make_shared<ManipulatorBoundCone>(id);
                cone->SetShapeData(shapeData);
                m_bounds.push_back(cone);
                return cone;
            }
            else if (azrtti_istypeof<BoundShapeQuad>(&shapeData))
            {
                AZStd::shared_ptr<BoundShapeInterface> quad = AZStd::make_shared<ManipulatorBoundQuad>(id);
                quad->SetShapeData(shapeData);
                m_bounds.push_back(quad);
                return quad;
            }
            else if (azrtti_istypeof<BoundShapeTorus>(&shapeData))
            {
                AZStd::shared_ptr<BoundShapeInterface> torus = AZStd::make_shared<ManipulatorBoundTorus>(id);
                torus->SetShapeData(shapeData);
                m_bounds.push_back(torus);
                return torus;
            }
            else
            {
                return nullptr;
            }
        }

        void ManipulatorBoundManager::DeleteShape(AZStd::shared_ptr<BoundShapeInterface> bound)
        {
            auto found = AZStd::find(m_bounds.begin(), m_bounds.end(), bound);
            if (found != m_bounds.end())
            {
                m_bounds.erase(found);
            }
        }

        void ManipulatorBoundManager::RaySelect(RaySelectInfo &rayInfo)
        {
            AZStd::vector<AZStd::pair<RegisteredBoundId, float>> rayHits;
            rayHits.reserve(m_bounds.size());
            for (AZStd::shared_ptr<BoundShapeInterface> bound : m_bounds)
            {
                if (bound->IsValid())
                {
                    float t = 0.0f;
                    bool isHit = bound->IntersectRay(rayInfo.m_origin, rayInfo.m_direction, t);
                    if (isHit)
                    {
                        auto hitItr = rayHits.begin();
                        for (; hitItr != rayHits.end(); ++hitItr)
                        {
                            if (t < hitItr->second)
                            {
                                break;
                            }
                        }
                        rayHits.insert(hitItr, AZStd::make_pair(bound->GetBoundID(), t));
                    }
                }
            }

            rayInfo.m_boundIDsHit.reserve(rayHits.size());
            for (auto& hit : rayHits)
            {
                rayInfo.m_boundIDsHit.push_back(hit);
            }
        }
    } // namespace Picking
} // namespace AzToolsFramework