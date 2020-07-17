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

#include <AzCore/std/containers/array.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzFramework/Input/User/LocalUserId.h>

namespace StarterGame
{
    enum EInventoryItemType
    {
        EInventoryItemType_HealthPak=0,
        EInventoryItemType_Ammo,
        EInventoryItemType_MAX
    };

    class Inventory
    {
    public: 
        struct ItemData
        {
            AZ_CLASS_ALLOCATOR(ItemData, AZ::SystemAllocator, 0);
            AZ_RTTI(ItemData, "{C71FD6CD-0632-4713-8725-4C56C00AF293}");
            static void Reflect(AZ::SerializeContext& sc);
            virtual ~ItemData() = default;

            ItemData() { m_Quantity = 0; }
            AZStd::string m_Name;
            AZStd::string m_Description;
            // 2d image todo
            AZ::u32 m_UnitPrice;
            AZ::u32 m_Quantity;
        };

        AZ_CLASS_ALLOCATOR(Inventory, AZ::SystemAllocator, 0);
        AZ_RTTI(Inventory, "{C71FD6CD-0632-4713-8725-4C56C00AF292}");
        static void Reflect(AZ::SerializeContext& sc);

        Inventory();
        virtual ~Inventory() = default;
        void Reset();
        void AddItem(EInventoryItemType itemType);
        ItemData* GetInventoryItemData(EInventoryItemType itemType);

    private:
        // The index is the item type 
        AZStd::array<ItemData, EInventoryItemType_MAX > m_Items;
    };
}
