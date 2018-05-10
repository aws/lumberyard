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

#include "Twitch_precompiled.h"

#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/std/time.h>
#include "TwitchSystemComponent.h"
#include "TwitchReflection.h"


#if defined(_WIN32)
#pragma warning (push) 
#pragma warning (disable:4355)
#endif

// there are warnings within the include chain in future, these will present
// as errors, so we need to disable them for "#include <future>" only.

#include <future>

#if defined(_WIN32)
#pragma warning (pop)
#endif

namespace Twitch
{
    TwitchSystemComponent::TwitchSystemComponent() 
        : m_receiptCounter(0)
    {
    }

    void TwitchSystemComponent::OnSystemTick()
    {
        if( m_twitchREST != nullptr)
            m_twitchREST->FlushEvents();
    }
    
    void TwitchSystemComponent::Reflect(AZ::ReflectContext* context)
    {
        if (AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serialize->Class<TwitchSystemComponent, AZ::Component>()
                ->Version(1);

            if (AZ::EditContext* ec = serialize->GetEditContext())
            {
                ec->Class<TwitchSystemComponent>("Twitch", "Provides access to Twitch \"Friends\", \"Rich Presence\" APIs")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                        // ->Attribute(AZ::Edit::Attributes::Category, "") Set a category
                        ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC("System"))
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ;
            }
        }

        if (AZ::BehaviorContext* behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context)) 
        {
            Internal::Reflect(*behaviorContext);
        }
    }

    void TwitchSystemComponent::SetApplicationID(const AZStd::string& twitchApplicationID)
    {
        if(m_fuelInterface != nullptr)
        {
            m_fuelInterface->SetApplicationID(twitchApplicationID);
        }
        else
        {
            AZ_Warning("TwitchSystemComponent::SetApplicationID", false, "Fuel is not initlized! Request ignored!");
        }
    }

    void TwitchSystemComponent::RequestUserID(ReceiptID& receipt)
    {
        // always return a receipt.
        receipt.SetID(GetReceipt());

        std::async(std::launch::async, [receipt, this]()
        {
            AZStd::string id;
            ResultCode result = ResultCode::Unknown;

            if (m_fuelInterface == nullptr)
            {
                // not ready yet....
                result = ResultCode::FuelSDKNotInitialized;
            }
            else
            {
                result = m_fuelInterface->GetClientID(id);
            }

            EBUS_QUEUE_EVENT(TwitchNotifyBus, UserIDNotify, StringValue(id, receipt, result));
        });
    }

    void TwitchSystemComponent::RequestOAuthToken(ReceiptID& receipt)
    {
        // always return a receipt.
        receipt.SetID(GetReceipt());

        std::async(std::launch::async, [receipt, this]()
        {
            AZStd::string token;
            ResultCode result = ResultCode::Unknown;

            if (m_fuelInterface == nullptr)
            {
                result = ResultCode::FuelSDKNotInitialized;
            }
            else
            {
                result = m_fuelInterface->GetOAuthToken(token);
            }

            EBUS_QUEUE_EVENT(TwitchNotifyBus, OAuthTokenNotify, StringValue(token, receipt, result));
        });
    }

    void TwitchSystemComponent::RequestEntitlement(ReceiptID& receipt)
    {
        // always return a receipt.
        receipt.SetID(GetReceipt());

        std::async(std::launch::async, [receipt, this]()
        {
            AZStd::string token;
            ResultCode result = ResultCode::Unknown;

            if (m_fuelInterface == nullptr)
            {
                result = ResultCode::FuelSDKNotInitialized;
            }
            else
            {
                result = m_fuelInterface->RequestEntitlement(token);
            }

            EBUS_QUEUE_EVENT(TwitchNotifyBus, EntitlementNotify, StringValue(token, receipt, result));
        });
    }

    void TwitchSystemComponent::RequestProductCatalog(ReceiptID& receipt)
    {
        // always return a receipt.
        receipt.SetID(GetReceipt());

        std::async(std::launch::async, [receipt, this]()
        {
            ProductData productData;
            ResultCode result = ResultCode::Unknown;

            if (m_fuelInterface == nullptr)
            {
                result = ResultCode::FuelSDKNotInitialized;
            }
            else
            {
                result = m_fuelInterface->RequestProductCatalog(productData);
            }

            EBUS_QUEUE_EVENT(TwitchNotifyBus, RequestProductCatalog, ProductDataReturnValue(productData, receipt, result));
        });
    }

    void TwitchSystemComponent::PurchaseProduct(ReceiptID& receipt, const Twitch::FuelSku& sku)
    {
        // always return a receipt.
        receipt.SetID(GetReceipt());

        std::async(std::launch::async, [receipt, sku, this]()
        {
            PurchaseReceipt purchaseReceipt;
            ResultCode result = ResultCode::Unknown;

            if (m_fuelInterface == nullptr)
            {
                result = ResultCode::FuelSDKNotInitialized;
            }
            else if( !IsValidSku(sku) )
            {
                result = ResultCode::FuelIllformedSku;
            }
            else
            {
                result = m_fuelInterface->PurchaseProduct(sku, purchaseReceipt);
            }

            EBUS_QUEUE_EVENT(TwitchNotifyBus, PurchaseProduct, PurchaseReceiptReturnValue(purchaseReceipt, receipt, result));
        });
    }

    void TwitchSystemComponent::GetPurchaseUpdates(ReceiptID& receipt, const AZStd::string& syncToken)
    {
        // always return a receipt.
        receipt.SetID(GetReceipt());

        std::async(std::launch::async, [receipt, syncToken, this]()
        {
            PurchaseUpdate purchaseUpdate;
            ResultCode result = ResultCode::Unknown;

            if (m_fuelInterface == nullptr)
            {
                result = ResultCode::FuelSDKNotInitialized;
            }
            else if (!IsValidSyncToken(syncToken))
            {
                result = ResultCode::InvalidParam;
            }
            else
            {
                result = m_fuelInterface->GetPurchaseUpdates(syncToken, purchaseUpdate);
            }

            EBUS_QUEUE_EVENT(TwitchNotifyBus, GetPurchaseUpdates, PurchaseUpdateReturnValue(purchaseUpdate, receipt, result));
        });
    }


    void TwitchSystemComponent::GetUser(ReceiptID& receipt)
    {
        receipt.SetID(GetReceipt());

        if ((m_fuelInterface == nullptr) || (m_twitchREST == nullptr))
        {
            EBUS_QUEUE_EVENT(TwitchNotifyBus, GetUser, UserInfoValue(UserInfo(), receipt, ResultCode::FuelSDKNotInitialized));
        }
        else
        {
            m_twitchREST->GetUser(receipt);
        }
    }

    void TwitchSystemComponent::ResetFriendsNotificationCount(ReceiptID& receipt, const AZStd::string& friendID)
    {
        receipt.SetID(GetReceipt());
        ResultCode rc(ResultCode::Success);

        if ((m_fuelInterface == nullptr) || (m_twitchREST == nullptr))
            rc = ResultCode::FuelSDKNotInitialized;
        else if ( !IsValidFriendID(friendID) )
            rc = ResultCode::InvalidParam;

        if (rc != ResultCode::Success)
        {
            EBUS_QUEUE_EVENT(TwitchNotifyBus, ResetFriendsNotificationCountNotify, Int64Value(0, receipt, rc));
        }
        else
        {
            m_twitchREST->ResetFriendsNotificationCount(receipt, friendID);
        }
    }

    void TwitchSystemComponent::GetFriendNotificationCount(ReceiptID& receipt, const AZStd::string& friendID)
    {
        receipt.SetID(GetReceipt());
        ResultCode rc(ResultCode::Success);

        if ((m_fuelInterface == nullptr) || (m_twitchREST == nullptr))
            rc = ResultCode::FuelSDKNotInitialized;
        else if ( !IsValidFriendID(friendID) )
            rc = ResultCode::InvalidParam;

        if (rc != ResultCode::Success)
        {
            EBUS_QUEUE_EVENT(TwitchNotifyBus, GetFriendNotificationCount, Int64Value(0, receipt, rc));
        }
        else
        {
            m_twitchREST->GetFriendNotificationCount(receipt, friendID);
        }
    }

    void TwitchSystemComponent::GetFriendRecommendations(ReceiptID& receipt, const AZStd::string& friendID)
    {
        receipt.SetID(GetReceipt());
        ResultCode rc(ResultCode::Success);

        if ((m_fuelInterface == nullptr) || (m_twitchREST == nullptr))
            rc = ResultCode::FuelSDKNotInitialized;
        else if ( !IsValidFriendID(friendID) )
            rc = ResultCode::InvalidParam;

        if (rc != ResultCode::Success)
        {
            EBUS_QUEUE_EVENT(TwitchNotifyBus, GetFriendRecommendations, FriendRecommendationValue(FriendRecommendationList(), receipt, rc));
        }
        else
        {
            m_twitchREST->GetFriendRecommendations(receipt, friendID);
        }
    }

    void TwitchSystemComponent::GetFriends(ReceiptID& receipt, const AZStd::string& friendID, const AZStd::string& cursor)
    {
        receipt.SetID(GetReceipt());
        ResultCode rc(ResultCode::Success);

        if ((m_fuelInterface == nullptr) || (m_twitchREST == nullptr))
            rc = ResultCode::FuelSDKNotInitialized;
        else if ( !IsValidFriendID(friendID) )
            rc = ResultCode::InvalidParam;

        if (rc != ResultCode::Success)
        {
            EBUS_QUEUE_EVENT(TwitchNotifyBus, GetFriends, GetFriendValue(GetFriendReturn(), receipt, rc));
        }
        else
        {
            m_twitchREST->GetFriends(receipt, friendID, cursor);
        }
    }

    void TwitchSystemComponent::GetFriendStatus(ReceiptID& receipt, const AZStd::string& sourceFriendID, const AZStd::string& targetFriendID)
    {
        receipt.SetID(GetReceipt());
        ResultCode rc(ResultCode::Success);

        if ((m_fuelInterface == nullptr) || (m_twitchREST == nullptr))
            rc = ResultCode::FuelSDKNotInitialized;
        else if ( (!sourceFriendID.empty() && !IsValidFriendID(sourceFriendID) ) || !IsValidFriendID(targetFriendID) )
            rc = ResultCode::InvalidParam;

        if(rc != ResultCode::Success)
        {
            EBUS_QUEUE_EVENT(TwitchNotifyBus, GetFriendStatus, FriendStatusValue(FriendStatus(), receipt, rc));
        }
        else
        {
            m_twitchREST->GetFriendStatus(receipt, sourceFriendID, targetFriendID);
        }
    }

    void TwitchSystemComponent::AcceptFriendRequest(ReceiptID& receipt, const AZStd::string& friendID)
    {
        receipt.SetID(GetReceipt());
        ResultCode rc(ResultCode::Success);

        if ((m_fuelInterface == nullptr) || (m_twitchREST == nullptr))
            rc = ResultCode::FuelSDKNotInitialized;
        else if ( !IsValidFriendID(friendID) )
            rc = ResultCode::InvalidParam;

        if (rc != ResultCode::Success)
        {
            EBUS_QUEUE_EVENT(TwitchNotifyBus, AcceptFriendRequest, Int64Value(0, receipt, rc));
        }
        else
        {
            m_twitchREST->AcceptFriendRequest(receipt, friendID);
        }
    }

    void TwitchSystemComponent::GetFriendRequests(ReceiptID& receipt, const AZStd::string& cursor)
    {
        receipt.SetID(GetReceipt());

        if ((m_fuelInterface == nullptr) || (m_twitchREST == nullptr))
        {
            EBUS_QUEUE_EVENT(TwitchNotifyBus, GetFriendRequests, FriendRequestValue(FriendRequestResult(), receipt, ResultCode::FuelSDKNotInitialized));
        }
        else
        {
            m_twitchREST->GetFriendRequests(receipt, cursor);
        }
     }

    void TwitchSystemComponent::CreateFriendRequest(ReceiptID& receipt, const AZStd::string& friendID)
    {
        receipt.SetID(GetReceipt());
        ResultCode rc(ResultCode::Success);

        if ((m_fuelInterface == nullptr) || (m_twitchREST == nullptr))
            rc = ResultCode::FuelSDKNotInitialized;
        else if ( !IsValidFriendID(friendID) )
            rc = ResultCode::InvalidParam;

        if (rc != ResultCode::Success)
        {
            EBUS_QUEUE_EVENT(TwitchNotifyBus, CreateFriendRequest, Int64Value(0, receipt, rc));
        }
        else
        {
            m_twitchREST->CreateFriendRequest(receipt, friendID);
        }
    }

    void TwitchSystemComponent::DeclineFriendRequest(ReceiptID& receipt, const AZStd::string& friendID)
    {
        receipt.SetID(GetReceipt());
        ResultCode rc(ResultCode::Success);

        if ((m_fuelInterface == nullptr) || (m_twitchREST == nullptr))
            rc = ResultCode::FuelSDKNotInitialized;
        else if ( !IsValidFriendID(friendID) )
            rc = ResultCode::InvalidParam;

        if (rc != ResultCode::Success)
        {
            EBUS_QUEUE_EVENT(TwitchNotifyBus, DeclineFriendRequest, Int64Value(0, receipt, rc));
        }
        else
        {
            m_twitchREST->DeclineFriendRequest(receipt, friendID);
        }
    }

    void TwitchSystemComponent::UpdatePresenceStatus(ReceiptID& receipt, PresenceAvailability availability, PresenceActivityType activityType, const AZStd::string& gameContext)
    {
        receipt.SetID(GetReceipt());

        ResultCode rc(ResultCode::Success);

        if ((m_fuelInterface == nullptr) || (m_twitchREST == nullptr))
        {
            rc = ResultCode::FuelSDKNotInitialized;
        }
        else if ((activityType == PresenceActivityType::Playing) && !m_twitchREST->IsValidGameContext(gameContext) )
        {
            rc = ResultCode::InvalidParam;
        }

        if (rc != ResultCode::Success)
        {
            EBUS_QUEUE_EVENT(TwitchNotifyBus, UpdatePresenceStatus, Int64Value(0, receipt, rc));
        }
        else
        {
            m_twitchREST->UpdatePresenceStatus(receipt, availability, activityType, gameContext);
        }
    }
    
    void TwitchSystemComponent::GetPresenceStatusofFriends(ReceiptID& receipt)
    {
        receipt.SetID(GetReceipt());

        if ((m_fuelInterface == nullptr) || (m_twitchREST == nullptr))
        {
            EBUS_QUEUE_EVENT(TwitchNotifyBus, GetPresenceStatusofFriends, PresenceStatusValue(PresenceStatusList(), receipt, ResultCode::FuelSDKNotInitialized));
        }
        else
        {
            m_twitchREST->GetPresenceStatusofFriends(receipt);
        }
    }
    
    void TwitchSystemComponent::GetPresenceSettings(ReceiptID& receipt)
    {
        receipt.SetID(GetReceipt());

        if ((m_fuelInterface == nullptr) || (m_twitchREST == nullptr))
        {
            EBUS_QUEUE_EVENT(TwitchNotifyBus, GetPresenceSettings, PresenceSettingsValue(PresenceSettings(), receipt, ResultCode::FuelSDKNotInitialized));
        }
        else
        {
            m_twitchREST->GetPresenceSettings(receipt);
        }
    }

    void TwitchSystemComponent::UpdatePresenceSettings(ReceiptID& receipt, bool isInvisible, bool shareActivity)
    {
        receipt.SetID(GetReceipt());

        if ((m_fuelInterface == nullptr) || (m_twitchREST == nullptr))
        {
            EBUS_QUEUE_EVENT(TwitchNotifyBus, UpdatePresenceSettings, PresenceSettingsValue(PresenceSettings(), receipt, ResultCode::FuelSDKNotInitialized));
        }
        else
        {
            m_twitchREST->UpdatePresenceSettings(receipt, isInvisible, shareActivity);
        }
    }

    void TwitchSystemComponent::GetChannel(ReceiptID& receipt)
    {
        receipt.SetID(GetReceipt());

        if ((m_fuelInterface == nullptr) || (m_twitchREST == nullptr))
        {
            EBUS_QUEUE_EVENT(TwitchNotifyBus, GetChannel, ChannelInfoValue(ChannelInfo(), receipt, ResultCode::FuelSDKNotInitialized));
        }
        else
        {
            m_twitchREST->GetChannel(receipt);
        }
    }
    
    void TwitchSystemComponent::GetChannelbyID(ReceiptID& receipt, const AZStd::string& channelID)
    {
        receipt.SetID(GetReceipt());
        ResultCode rc(ResultCode::Success);

        if ((m_fuelInterface == nullptr) || (m_twitchREST == nullptr))
            rc = ResultCode::FuelSDKNotInitialized;
        else if ( !IsValidChannelID(channelID) )
            rc = ResultCode::InvalidParam;

        if (rc != ResultCode::Success)
        {
            EBUS_QUEUE_EVENT(TwitchNotifyBus, GetChannelbyID, ChannelInfoValue(ChannelInfo(), receipt, rc));
        }
        else
        {
            m_twitchREST->GetChannelbyID(receipt, channelID);
        }
    }

    void TwitchSystemComponent::UpdateChannel(ReceiptID& receipt, const ChannelUpdateInfo & channelUpdateInfo)
    {
        receipt.SetID(GetReceipt());

        if ((m_fuelInterface == nullptr) || (m_twitchREST == nullptr))
        {
            EBUS_QUEUE_EVENT(TwitchNotifyBus, UpdateChannel, ChannelInfoValue(ChannelInfo(), receipt, ResultCode::FuelSDKNotInitialized));
        }
        else
        {
            m_twitchREST->UpdateChannel(receipt, channelUpdateInfo);
        }
    }

    void TwitchSystemComponent::GetChannelEditors(ReceiptID& receipt, const AZStd::string& channelID)
    {
        receipt.SetID(GetReceipt());
        ResultCode rc(ResultCode::Success);

        if ((m_fuelInterface == nullptr) || (m_twitchREST == nullptr))
            rc = ResultCode::FuelSDKNotInitialized;
        else if ( !IsValidChannelID(channelID) )
            rc = ResultCode::InvalidParam;

        if (rc != ResultCode::Success)
        {
            EBUS_QUEUE_EVENT(TwitchNotifyBus, GetChannelEditors, UserInfoListValue(UserInfoList(), receipt, rc));
        }
        else
        {
            m_twitchREST->GetChannelEditors(receipt, channelID);
        }
    }

    void TwitchSystemComponent::GetChannelFollowers(ReceiptID& receipt, const AZStd::string& channelID, const AZStd::string& cursor, AZ::u64 offset)
    {
        receipt.SetID(GetReceipt());
        ResultCode rc(ResultCode::Success);

        if ((m_fuelInterface == nullptr) || (m_twitchREST == nullptr))
            rc = ResultCode::FuelSDKNotInitialized;
        else if ( !IsValidChannelID(channelID) )
            rc = ResultCode::InvalidParam;

        if (rc != ResultCode::Success)
        {
            EBUS_QUEUE_EVENT(TwitchNotifyBus, GetChannelFollowers, FollowerResultValue(FollowerResult(), receipt, rc));
        }
        else
        {
            m_twitchREST->GetChannelFollowers(receipt, channelID, cursor, offset);
        }
    }

    void TwitchSystemComponent::GetChannelTeams(ReceiptID& receipt, const AZStd::string& channelID)
    {
        receipt.SetID(GetReceipt());
        ResultCode rc(ResultCode::Success);

        if ((m_fuelInterface == nullptr) || (m_twitchREST == nullptr))
            rc = ResultCode::FuelSDKNotInitialized;
        else if ( !IsValidChannelID(channelID) )
            rc = ResultCode::InvalidParam;

        if (rc != ResultCode::Success)
        {
            EBUS_QUEUE_EVENT(TwitchNotifyBus, GetChannelTeams, ChannelTeamValue(TeamInfoList(), receipt, rc));
        }
        else
        {
            m_twitchREST->GetChannelTeams(receipt, channelID);
        }
    }

    void TwitchSystemComponent::GetChannelSubscribers(ReceiptID& receipt, const AZStd::string& channelID, AZ::u64 offset)
    {
        receipt.SetID(GetReceipt());
        ResultCode rc(ResultCode::Success);

        if ((m_fuelInterface == nullptr) || (m_twitchREST == nullptr))
            rc = ResultCode::FuelSDKNotInitialized;
        else if ( !IsValidChannelID(channelID) )
            rc = ResultCode::InvalidParam;

        if (rc != ResultCode::Success)
        {
            EBUS_QUEUE_EVENT(TwitchNotifyBus, GetChannelSubscribers, SubscriberValue(Subscription(), receipt, rc));
        }
        else
        {
            m_twitchREST->GetChannelSubscribers(receipt, channelID, offset);
        }
    }

    void TwitchSystemComponent::CheckChannelSubscriptionbyUser(ReceiptID& receipt, const AZStd::string& channelID, const AZStd::string& userID)
    {
        receipt.SetID(GetReceipt());
        ResultCode rc(ResultCode::Success);

        if ((m_fuelInterface == nullptr) || (m_twitchREST == nullptr))
            rc = ResultCode::FuelSDKNotInitialized;
        else if ( !IsValidChannelID(channelID) || !IsValidFriendID(userID) )
            rc = ResultCode::InvalidParam;

        if (rc != ResultCode::Success)
        {
            EBUS_QUEUE_EVENT(TwitchNotifyBus, CheckChannelSubscriptionbyUser, SubscriberbyUserValue(SubscriberInfo(), receipt, rc));
        }
        else
        {
            m_twitchREST->CheckChannelSubscriptionbyUser(receipt, channelID, userID);
        }
    }

    void TwitchSystemComponent::GetChannelVideos(ReceiptID& receipt, const AZStd::string& channelID, BroadCastType boradcastType, const AZStd::string& language, AZ::u64 offset)
    {
        receipt.SetID(GetReceipt());
        ResultCode rc(ResultCode::Success);

        if ((m_fuelInterface == nullptr) || (m_twitchREST == nullptr))
            rc = ResultCode::FuelSDKNotInitialized;
        else if ( !IsValidChannelID(channelID) )
            rc = ResultCode::InvalidParam;

        if (rc != ResultCode::Success)
        {
            EBUS_QUEUE_EVENT(TwitchNotifyBus, GetChannelVideos, VideoReturnValue(VideoReturn(), receipt, rc));
        }
        else
        {
            m_twitchREST->GetChannelVideos(receipt, channelID, boradcastType, language, offset);
        }
    }

    void TwitchSystemComponent::StartChannelCommercial(ReceiptID& receipt, const AZStd::string& channelID, CommercialLength length)
    {
        receipt.SetID(GetReceipt());
        ResultCode rc(ResultCode::Success);

        if ((m_fuelInterface == nullptr) || (m_twitchREST == nullptr))
            rc = ResultCode::FuelSDKNotInitialized;
        else if ( !IsValidChannelID(channelID) )
            rc = ResultCode::InvalidParam;

        if (rc != ResultCode::Success)
        {
            EBUS_QUEUE_EVENT(TwitchNotifyBus, StartChannelCommercial, StartChannelCommercialValue(StartChannelCommercialResult(), receipt, rc));
        }
        else
        {
            m_twitchREST->StartChannelCommercial(receipt, channelID, length);
        }
    }

    void TwitchSystemComponent::ResetChannelStreamKey(ReceiptID& receipt, const AZStd::string& channelID)
    {
        receipt.SetID(GetReceipt());
        ResultCode rc(ResultCode::Success);

        if ((m_fuelInterface == nullptr) || (m_twitchREST == nullptr))
            rc = ResultCode::FuelSDKNotInitialized;
        else if ( !IsValidChannelID(channelID) )
            rc = ResultCode::InvalidParam;

        if (rc != ResultCode::Success)
        {
            EBUS_QUEUE_EVENT(TwitchNotifyBus, ResetChannelStreamKey, ChannelInfoValue(ChannelInfo(), receipt, rc));
        }
        else
        {
            m_twitchREST->ResetChannelStreamKey(receipt, channelID);
        }
    }

    void TwitchSystemComponent::GetChannelCommunity(ReceiptID& receipt, const AZStd::string& channelID)
    {
        receipt.SetID(GetReceipt());
        ResultCode rc(ResultCode::Success);

        if ((m_fuelInterface == nullptr) || (m_twitchREST == nullptr))
            rc = ResultCode::FuelSDKNotInitialized;
        else if ( !IsValidChannelID(channelID) )
            rc = ResultCode::InvalidParam;

        if (rc != ResultCode::Success)
        {
            EBUS_QUEUE_EVENT(TwitchNotifyBus, GetChannelCommunity, CommunityInfoValue(CommunityInfo(), receipt, rc));
        }
        else
        {
            m_twitchREST->GetChannelCommunity(receipt, channelID);
        }
    }

    void TwitchSystemComponent::SetChannelCommunity(ReceiptID& receipt, const AZStd::string& channelID, const AZStd::string& communityID)
    {
        receipt.SetID(GetReceipt());
        ResultCode rc(ResultCode::Success);

        if ((m_fuelInterface == nullptr) || (m_twitchREST == nullptr))
            rc = ResultCode::FuelSDKNotInitialized;
        else if ( !IsValidChannelID(channelID) || !IsValidCommunityID(communityID) )
            rc = ResultCode::InvalidParam;

        if (rc != ResultCode::Success)
        {
            EBUS_QUEUE_EVENT(TwitchNotifyBus, SetChannelCommunity, Int64Value(0, receipt, rc));
        }
        else
        {
            m_twitchREST->SetChannelCommunity(receipt, channelID, communityID);
        }
    }

    void TwitchSystemComponent::DeleteChannelfromCommunity(ReceiptID& receipt, const AZStd::string& channelID)
    {
        receipt.SetID(GetReceipt());
        ResultCode rc(ResultCode::Success);

        if ((m_fuelInterface == nullptr) || (m_twitchREST == nullptr))
            rc = ResultCode::FuelSDKNotInitialized;
        else if ( !IsValidChannelID(channelID) )
            rc = ResultCode::InvalidParam;

        if (rc != ResultCode::Success)
        {
            EBUS_QUEUE_EVENT(TwitchNotifyBus, DeleteChannelfromCommunity, Int64Value(0, receipt, rc));
        }
        else
        {
            m_twitchREST->DeleteChannelfromCommunity(receipt, channelID);
        }
    }

    void TwitchSystemComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
    {
        provided.push_back(AZ_CRC("TwitchService"));
    }

    void TwitchSystemComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
    {
        incompatible.push_back(AZ_CRC("TwitchService"));
    }

    void TwitchSystemComponent::GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required)
    {
        (void)required;
    }

    void TwitchSystemComponent::GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent)
    {
        (void)dependent;
    }

    void TwitchSystemComponent::Init()
    {
#if defined(AZ_PLATFORM_WINDOWS_X64)
        // You must define the Twitch Application Client ID
        m_fuelInterface = IFuelInterface::Alloc();
        m_twitchREST    = ITwitchREST::Alloc(m_fuelInterface);
#endif //defined(AZ_PLATFORM_WINDOWS_X64)
    }

    void TwitchSystemComponent::Activate()
    {
        TwitchRequestBus::Handler::BusConnect();
        AZ::SystemTickBus::Handler::BusConnect();
    }

    void TwitchSystemComponent::Deactivate()
    {
        AZ::SystemTickBus::Handler::BusDisconnect();
        TwitchRequestBus::Handler::BusDisconnect();
    }

    AZ::u64 TwitchSystemComponent::GetReceipt()
    {
        return ++m_receiptCounter;
    }

    bool TwitchSystemComponent::IsValidString(const AZStd::string& str, AZStd::size_t minLength, AZStd::size_t maxLength) const
    {
        bool isValid = false;

        /*
        ** From Twitch (Chris Adoretti <cadoretti@twitch.tv>) 3/14/17:
        **   I think its a safe bet though to make sure the string is alpha-numeric + dashes
        **   for now if that helps (0-9, a-f, A-F, -). We don't have a max length yet.
        **   The minimum length is 1.
        */

        if ((str.size() >= minLength) && (str.size() < maxLength))
        {
            AZStd::size_t found = str.find_first_not_of("0123456789abcdefABCDEF-");

            if (found == AZStd::string::npos)
            {
                isValid = true;
            }
        }

        return isValid;
    }
        
    bool TwitchSystemComponent::IsValidFriendID(const AZStd::string& friendID) const
    {
        // The min id length should be 1
        // The max id length will be huge, since there is no official max length, this will allow for a large id.

        return IsValidString(friendID, 1, 256);
    }

    bool TwitchSystemComponent::IsValidSyncToken(const AZStd::string& syncToken) const
    {
        // sync tokens are either empty of a opaque token of a min length of 8, but no longer than 64

        return syncToken.empty() ? true : IsValidString(syncToken, 8, 64);
    }
}
