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

#include <Cry_Math.h>
#include "Source/Core/StarterGameGame.h"

#include <ILevelSystem.h>

#include <MessagePopup/MessagePopupBus.h>
#include <LyShine/Bus/UiCursorBus.h>
#include <LocalUser/LocalUserRequestBus.h>
#include <SaveData/SaveDataRequestBus.h>
#include <AzCore/Component/ComponentApplicationBus.h>

//////////////////////////////////////////////////////////////////////////////////////////

namespace StarterGame
{
    StarterGameGame::StarterGameGame()
    {
        m_playerData = AZStd::make_shared<PlayerData>();
        StarterGameRequestBus::Handler::BusConnect();
        PlayerDataRequestsBus::Handler::BusConnect();
    }

    StarterGameGame::~StarterGameGame()
    {
        PlayerDataRequestsBus::Handler::BusDisconnect();
        StarterGameRequestBus::Handler::BusDisconnect();
        m_playerData.reset();
    }

    //-------------------------------------------------------------------------
    void StarterGameGame::OnSystemEvent(ESystemEvent event, UINT_PTR wparam, UINT_PTR lparam)
    {
        switch (event)
        {
        case ESYSTEM_EVENT_RANDOM_SEED:
            break;

        case ESYSTEM_EVENT_CHANGE_FOCUS:
            break;

        case ESYSTEM_EVENT_LEVEL_POST_UNLOAD:
            break;

        case ESYSTEM_EVENT_LEVEL_LOAD_END:
            break;
        case ESYSTEM_EVENT_FAST_SHUTDOWN:
        default:
            break;
        }
    }

    //-------------------------------------------------------------------------
    void StarterGameGame::AddItemToInventory(AZ::u32 itemKind)
    {
        m_playerData->GetInventory().AddItem((EInventoryItemType)itemKind);
        SavePlayerDataToPersistentStorage(LocalUser::LocalUserRequests::GetPrimaryLocalUserId());
    }

    //-------------------------------------------------------------------------
    Inventory::ItemData* StarterGameGame::GetInventoryItemData(AZ::u32 itemType)
    {
        return m_playerData->GetInventory().GetInventoryItemData((EInventoryItemType)itemType);
    }

    //-------------------------------------------------------------------------
    void StarterGameGame::AddCoins(int nbCoins)
    {
        m_playerData->AddCoins(nbCoins);
        SavePlayerDataToPersistentStorage(LocalUser::LocalUserRequests::GetPrimaryLocalUserId());
    }

    //-------------------------------------------------------------------------
    int StarterGameGame::GetCoins()
    {
        return m_playerData->GetCoins();
    }

    //-------------------------------------------------------------------------
    void StarterGameGame::ReduceHealth(int healthPoints)
    {
        m_playerData->ReduceHealth(healthPoints);
    }

    //-------------------------------------------------------------------------
    void StarterGameGame::AddHealth(int healthPoints)
    {
        m_playerData->AddHealth(healthPoints);
    }

    //-------------------------------------------------------------------------
    int StarterGameGame::GetHealth()
    {
        return m_playerData->GetHealth();
    }

    //-------------------------------------------------------------------------
    void StarterGameGame::SellItems(AZ::u32 itemType, AZ::u32 quantity)
    {
        m_playerData->SellItems(itemType, quantity);
    }

    //-------------------------------------------------------------------------
    void StarterGameGame::ResetPlayerData()
    {
        m_playerData->Reset();
    }

    //-------------------------------------------------------------------------
    void StarterGameGame::SavePlayerDataToPersistentStorage(AzFramework::LocalUserId localUserId)
    {
        SaveData::SaveDataRequests::SaveOrLoadObjectParams<PlayerData> saveObjectParams;
        saveObjectParams.serializableObject = m_playerData;
        saveObjectParams.dataBufferName = PlayerData::SaveDataBufferName;
        saveObjectParams.localUserId = localUserId;
        SaveData::SaveDataRequests::SaveObject(saveObjectParams);
    }

    //-------------------------------------------------------------------------
    void StarterGameGame::LoadPlayerDataFromPersistentStorage(AzFramework::LocalUserId localUserId)
    {
        SaveData::SaveDataRequests::SaveOrLoadObjectParams<PlayerData> loadObjectParams;
        loadObjectParams.serializableObject = m_playerData;
        loadObjectParams.dataBufferName = PlayerData::SaveDataBufferName;
        loadObjectParams.localUserId = localUserId;
        loadObjectParams.callback = [](const SaveData::SaveDataRequests::SaveOrLoadObjectParams<PlayerData>& params,
            SaveData::SaveDataNotifications::Result result)
        {
            // ToDo: If we need to take any action when the data has loaded
            // params.serializableObject->OnLoadedFromPersistentData();
        };
        SaveData::SaveDataRequests::LoadObject(loadObjectParams);
    }

    //-------------------------------------------------------------------------
    void StarterGameGame::SaveGameData()
    {
        SavePlayerDataToPersistentStorage(LocalUser::LocalUserRequests::GetPrimaryLocalUserId());
    }
}
