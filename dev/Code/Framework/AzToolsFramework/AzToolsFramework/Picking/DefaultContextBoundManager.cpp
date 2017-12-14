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

#include "DefaultContextBoundManager.h"

namespace AzToolsFramework
{
    namespace Picking
    {
        RegisteredBoundId DefaultContextBoundManager::m_boundsGlobalId = 1;

        DefaultContextBoundManager::DefaultContextBoundManager(ManipulatorManagerId manipulatorManagerId)
        {
            ContextBoundManagerRequestBus::Handler::BusConnect(manipulatorManagerId);
        }

        DefaultContextBoundManager::~DefaultContextBoundManager()
        {
            ContextBoundManagerRequestBus::Handler::BusDisconnect();
        }

        RegisteredBoundId DefaultContextBoundManager::UpdateOrRegisterBound(const BoundRequestShapeBase& shapeData, RegisteredBoundId boundId, AZ::u64 userContext)
        {
            if (boundId == InvalidBoundId)
            {
                // make a new bound
                boundId = m_boundsGlobalId++;
            }

            auto result = m_boundIdToShapeMap.find(boundId);
            if (result == m_boundIdToShapeMap.end())
            {
                AZStd::shared_ptr<BoundShapeInterface> createdShape = CreateShape(shapeData, boundId, userContext);
                if (createdShape)
                {
                    m_boundIdToShapeMap[boundId] = createdShape;
                    createdShape->SetValidity(true);
                }
                else
                {
                    boundId = InvalidBoundId;
                }
            }
            else
            {
                result->second->SetShapeData(shapeData);
                result->second->SetValidity(true);
            }

            return boundId;
        }

        void DefaultContextBoundManager::UnregisterBound(RegisteredBoundId boundId)
        {
            const auto findIter = m_boundIdToShapeMap.find(boundId);
            if (findIter != m_boundIdToShapeMap.end())
            {
                DeleteShape(findIter->second);
                m_boundIdToShapeMap.erase(findIter);
            }
        }

        AZStd::shared_ptr<BoundShapeInterface> DefaultContextBoundManager::CreateShape(const BoundRequestShapeBase& /*shapeData*/, RegisteredBoundId /*id*/, AZ::u64 /*userContext*/)
        {
            return nullptr;
        }

        void DefaultContextBoundManager::DeleteShape(AZStd::shared_ptr<BoundShapeInterface> /*shape*/)
        {
        }

        void DefaultContextBoundManager::RaySelect(RaySelectInfo& /*rayInfo*/)
        {
        }

        void DefaultContextBoundManager::SetBoundValidity(RegisteredBoundId boundId, bool valid)
        {
            auto found = m_boundIdToShapeMap.find(boundId);
            if (found != m_boundIdToShapeMap.end())
            {
                found->second->SetValidity(valid);
            }
        }
    } // namespace Picking
} // namespace AzToolsFramework