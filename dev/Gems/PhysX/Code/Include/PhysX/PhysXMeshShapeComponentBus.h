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

#include <AzCore/Component/ComponentBus.h>
#include <Pipeline/PhysXMeshAsset.h>

namespace PhysX
{
    /**
     * Services provided by the PhysX Mesh Shape Component.
     */
    class PhysXMeshShapeComponentRequests
        : public AZ::ComponentBus
    {
    public:
        /**
        * Gets Mesh information from this entity.
        * @return Asset pointer to mesh asset.
        */
        virtual AZ::Data::Asset<Pipeline::PhysXMeshAsset> GetMeshAsset() = 0;
    };

    /**
     * Bus to service the PhysX Mesh Shape Component event group.
     */
    using PhysXMeshShapeComponentRequestBus = AZ::EBus<PhysXMeshShapeComponentRequests>;
} // namespace PhysX