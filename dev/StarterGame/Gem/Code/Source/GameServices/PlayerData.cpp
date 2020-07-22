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
#include <Source/GameServices/Inventory.h>

namespace StarterGame
{
    //-------------------------------------------------------------------------
    PlayerData::PlayerData()
        : m_NbCoins(0)
        , m_Health(100)
    {
    }

    //-------------------------------------------------------------------------
    void PlayerData::Reflect(AZ::SerializeContext& sc)
    {
        Inventory::Reflect(sc);

        sc.Class<PlayerData>()
            ->Version(1)
            ->Field("coins", &PlayerData::m_NbCoins)
            ->Field("health", &PlayerData::m_Health)
            ->Field("inventory", &PlayerData::m_Inventory)
            ->Field("ship_position", &PlayerData::m_ShipPosition)
            ;
    }

    //-------------------------------------------------------------------------
    void PlayerData::AddCoins(int nbCoins)
    {
        m_NbCoins += nbCoins;
    }

    //-------------------------------------------------------------------------
    int PlayerData::GetCoins()
    {
        return m_NbCoins;
    }

    //-------------------------------------------------------------------------
    void PlayerData::ReduceHealth(int healthPoints)
    {
        m_Health -= healthPoints;
        if (m_Health < 0)
        {
            m_Health = 0;
        }
    }

    //-------------------------------------------------------------------------
    void PlayerData::AddHealth(int healthPoints)
    {
        m_Health += healthPoints;
        if (m_Health > 100)
        {
            m_Health = 100;
        }
    }

    //-------------------------------------------------------------------------
    int PlayerData::GetHealth()
    {
        return m_Health;
    }

    //-------------------------------------------------------------------------
    void PlayerData::SellItems(AZ::u32 itemType, AZ::u32 quantity)
    {
        Inventory::ItemData *itemData = m_Inventory.GetInventoryItemData((EInventoryItemType)itemType);
        if (itemData->m_Quantity >= quantity)
        {
            itemData->m_Quantity -= quantity;
            AddCoins(itemData->m_UnitPrice * quantity);
        }
    }

    //-------------------------------------------------------------------------
    void PlayerData::Reset()
    {
        m_NbCoins = 0;
        m_Health = 100;
        m_Inventory.Reset();
    }

} // namespace StarterGame
