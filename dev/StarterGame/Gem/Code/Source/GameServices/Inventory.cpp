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

#include <Source/GameServices/PlayerData.h>
#include <ILevelSystem.h>

namespace StarterGame
{
    //-------------------------------------------------------------------------
    Inventory::Inventory()
    {
        m_Items[0].m_Name = "Dummy Item";
        m_Items[0].m_Description = "Description of a dummy item";
        m_Items[0].m_UnitPrice = 100;
    }

    //-------------------------------------------------------------------------
    void Inventory::ItemData::Reflect(AZ::SerializeContext& sc)
    {
        sc.Class<ItemData>()
            ->Version(1)
            ->Field("quantity", &ItemData::m_Quantity)
            ;
    }

    //-------------------------------------------------------------------------
    void Inventory::Reflect(AZ::SerializeContext& sc)
    {
        ItemData::Reflect(sc);
        sc.Class<Inventory>()
            ->Version(1)
            ->Field("items", &Inventory::m_Items)
            ;
    }
    
    //-------------------------------------------------------------------------
    void Inventory::Reset()
    {
        for (AZ::u32 i = 0; i < EInventoryItemType_MAX; ++i)
        {
            m_Items[i].m_Quantity = 0;
            if (GetISystem()->GetGlobalEnvironment()->IsEditor())
            {
                m_Items[i].m_Quantity = 100;
            }
        }
    }

    //-------------------------------------------------------------------------
    void Inventory::AddItem(EInventoryItemType itemType)
    {
        if (itemType < EInventoryItemType_MAX)
        {
            m_Items[itemType].m_Quantity++;
        }
    }

    //-------------------------------------------------------------------------
    Inventory::ItemData* Inventory::GetInventoryItemData(EInventoryItemType itemType)
    {
        return itemType < EInventoryItemType_MAX ? &m_Items[itemType]: nullptr;
    }
} // namespace StarterGame
