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

#include <GameServices/PlayerData.h>
#include <StarterGame/StarterGameBus.h>

#include <LyShine/Bus/UiCanvasBus.h>
#include <LyShine/ISprite.h>
#include <AzCore/Component/Entity.h>
#include <AzCore/Component/EntityId.h>
#include <AzCore/std/containers/map.h>

#include <AzCore/std/smart_ptr/make_shared.h>
#include <IGem.h>

#define GAME_NAME "StarterGame"

namespace StarterGame
{
    class StarterGameGame
        : protected StarterGameRequestBus::Handler
        , protected PlayerDataRequestsBus::Handler
        , public CryHooksModule
    {
    public:
        StarterGameGame();
        virtual ~StarterGameGame();

        //////////////////////////////////////////////////////////////////////////
        //! CryHooksModule
        void OnSystemEvent(ESystemEvent event, UINT_PTR wparam, UINT_PTR lparam) override;
        
        //////////////////////////////////////////////////////////////////////////
        // StarterGameRequests::Bus interface implementation
        void SaveGameData() override;

        //////////////////////////////////////////////////////////////////////////
        // PlayerDataRequestsBus 
        void AddItemToInventory(AZ::u32 itemKind) override;
        void AddCoins(int nbCoins) override;
        int GetCoins() override;
        void ReduceHealth(int healthPoints) override;
        void AddHealth(int healthPoints) override;
        int GetHealth() override;
        void SellItems(AZ::u32 itemType, AZ::u32 quantity) override;
        void ResetPlayerData() override;
        Inventory::ItemData* GetInventoryItemData(AZ::u32 itemType) override;
        void SavePlayerDataToPersistentStorage(AzFramework::LocalUserId localUserId) override;
        void LoadPlayerDataFromPersistentStorage(AzFramework::LocalUserId localUserId) override;

    protected:

        AZStd::shared_ptr<PlayerData> m_playerData; 
    };
} // namespace StarterGame
