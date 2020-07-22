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
#include <AzCore/Component/Component.h>
#include <AzCore/Component/ComponentBus.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/std/containers/array.h>
#include <Source/GameServices/Inventory.h>
#include <LocalUser/LocalUserRequestBus.h>
#include <AzCore/Math/Vector3.h>

#include <AzFramework/Input/User/LocalUserId.h>

namespace StarterGame
{
    class PlayerData
    {
    public:
        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Name of the player data save data file.
        static constexpr const char* SaveDataBufferName = "PlayerData";

        AZ_CLASS_ALLOCATOR(PlayerData, AZ::SystemAllocator, 0);
        AZ_RTTI(PlayerData, "{0976397C-B406-4128-8D5F-9CE74930FFC4}");
        static void Reflect(AZ::SerializeContext& sc);
        virtual ~PlayerData() = default;
        
        PlayerData();

        void AddCoins(int nbCoins);
        int GetCoins();
        void ReduceHealth(int healthPoints);
        void AddHealth(int healthPoints);
        int GetHealth();

        void SellItems(AZ::u32 itemType, AZ::u32 quantity);
        void Reset();
        Inventory& GetInventory() { return m_Inventory; }
        
        virtual void SavePlayerDataToPersistentStorage(AzFramework::LocalUserId localUserId) {}
        virtual void LoadPlayerDataFromPersistentStorage(AzFramework::LocalUserId localUserId) {}

    private:
        int m_NbCoins;
        Inventory m_Inventory;
        int m_Health;
        AZ::Vector3 m_ShipPosition;
    };


    class PlayerDataRequests
        : public AZ::EBusTraits
    {
    public:
        //////////////////////////////////////////////////////////////////////////
        // EBusTraits overrides
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::Single;
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;

        virtual void AddCoins(int nbCoins) { }
        virtual int GetCoins() { return 0; }
        virtual void ReduceHealth(int healthPoints) {}
        virtual void AddHealth(int healthPoints) {}
        virtual int GetHealth() { return 0; }
        virtual void ResetPlayerData() {}
        virtual void AddItemToInventory(AZ::u32 itemType) {}
        virtual void SellItems(AZ::u32 itemType, AZ::u32 quantity) {}
        virtual Inventory::ItemData* GetInventoryItemData(AZ::u32 itemType) { return nullptr; }
        virtual void SavePlayerDataToPersistentStorage(AzFramework::LocalUserId localUserId) {}
        virtual void LoadPlayerDataFromPersistentStorage(AzFramework::LocalUserId localUserId) {}
    };
    using PlayerDataRequestsBus = AZ::EBus<PlayerDataRequests>;
}
