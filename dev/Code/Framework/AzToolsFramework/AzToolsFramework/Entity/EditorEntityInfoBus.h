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
#include <AzCore/Component/Entity.h>
#include <AzCore/std/containers/vector.h>
#include "EditorEntitySortBus.h"

namespace AzToolsFramework
{
    /**
     * 
     *
     */
    class EditorEntityInfoRequests : public AZ::EBusTraits
    {
    public:
        virtual ~EditorEntityInfoRequests() = default;

        //////////////////////////////////////////////////////////////////////////
        // EBusTraits overrides
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::ById;
        using BusIdType = AZ::EntityId;
        //////////////////////////////////////////////////////////////////////////

        // EntityID -> Parent
        virtual AZ::EntityId GetParent() = 0;
        
        // EntityID -> Children
        virtual AZStd::vector<AZ::EntityId> GetChildren() = 0;

        // EntityID -> IsSliceEntity
        virtual bool IsSliceEntity() = 0;

        // EntityID -> IsSliceRoot
        virtual bool IsSliceRoot() = 0;

        // EntityID -> DepthInHierarchy
        virtual AZ::u64 DepthInHierarchy() = 0;

        // EntityID -> SiblingSortIndex
        virtual AZ::u64 SiblingSortIndex() = 0;
    };
    using EditorEntityInfoRequestBus = AZ::EBus<EditorEntityInfoRequests>;
}
