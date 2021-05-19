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
#include <AzCore/Script/ScriptContext.h>
#include <AzCore/RTTI/BehaviorContext.h>
#include <Twitch/TwitchBus.h>
#include "TwitchReflection.h"

namespace AZ
{
    /*
    ** Helper macros to reflect the Twitch enums
    */

    AZ_TYPE_INFO_SPECIALIZE(Twitch::ResultCode, "{DA72B2F5-2983-4E30-B64C-BDF417FA73A6}");
    AZ_TYPE_INFO_SPECIALIZE(Twitch::PresenceAvailability, "{090CF417-870C-4E27-B9FC-6FE96787DE18}");
    AZ_TYPE_INFO_SPECIALIZE(Twitch::PresenceActivityType, "{B8D3EFFC-D71E-4441-9D09-BFD585A4B1B8}");
    AZ_TYPE_INFO_SPECIALIZE(Twitch::BroadCastType, "{751DA7A4-A080-4DE4-A15F-F63B2B066AA6}");
    AZ_TYPE_INFO_SPECIALIZE(Twitch::CommercialLength, "{76255136-2B04-4EE2-A499-BBB141A28716}");
}

namespace Twitch
{
    #define ENUMCASE(code) case code: txtCode=AZStd::string::format(#code"(%llu)",static_cast<AZ::u64>(code)); break;

    AZStd::string ResultCodeToString(ResultCode code)
    {
        AZStd::string txtCode;
        switch(code)
        {
            ENUMCASE(ResultCode::Success)
            ENUMCASE(ResultCode::InvalidParam)
            ENUMCASE(ResultCode::TwitchRESTError)
            ENUMCASE(ResultCode::TwitchChannelNoUpdatesToMake)
            ENUMCASE(ResultCode::Unknown)
            default:
                txtCode = AZStd::string::format("Undefined ResultCode (%llu)", static_cast<AZ::u64>(code));
        }

        return txtCode;
    }

    AZStd::string PresenceAvailabilityToString(PresenceAvailability availability)
    {
        AZStd::string txtCode;
        switch (availability)
        {
            ENUMCASE(PresenceAvailability::Unknown)
            ENUMCASE(PresenceAvailability::Online)
            ENUMCASE(PresenceAvailability::Idle)
        default:
            txtCode = AZStd::string::format("Undefined PresenceAvailability (%llu)", static_cast<AZ::u64>(availability));
        }

        return txtCode;
    }

    AZStd::string PresenceActivityTypeToString(PresenceActivityType activity)
    {
        AZStd::string txtCode;
        switch (activity)
        {
            ENUMCASE(PresenceActivityType::Unknown)
            ENUMCASE(PresenceActivityType::Watching)
            ENUMCASE(PresenceActivityType::Playing)
            ENUMCASE(PresenceActivityType::Broadcasting)
        default:
            txtCode = AZStd::string::format("Undefined PresenceActivityType (%llu)", static_cast<AZ::u64>(activity));
        }

        return txtCode;
    }

    AZStd::string BoolName(bool value, const AZStd::string & trueText, const AZStd::string & falseText)
    {
        return value ? trueText : falseText;
    }

    AZStd::string UserNotificationsToString(const UserNotifications & info)
    {
        return "Email: " + BoolName(info.EMail, "On", "Off") +
               "Push: " + BoolName(info.EMail, "On", "Off");
    }

    AZStd::string UserInfoIDToString(const UserInfo & info)
    {
        return  "UserID:" + info.ID;
    }

    AZStd::string UserInfoMiniString(const UserInfo & info)
    {
        return  UserInfoIDToString(info) +
                " DisplayName:" + info.DisplayName +
                " Name:" + info.Name +
                " Type:" + info.Type;
    }

    AZStd::string UserInfoToString(const UserInfo & info)
    {
        return  UserInfoMiniString(info) +
                " Bio:" + info.Bio +
                " EMail:" + info.EMail +
                " Logo:" + info.Logo +
                " Notifications:" + UserNotificationsToString(info.Notifications) +
                " CreatedDate:" + info.CreatedDate +
                " UpdatedDate:" + info.UpdatedDate +
                " EMailVerified:" + BoolName(info.EMailVerified, "Yes", "No");
                " Partnered:" + BoolName(info.Partnered, "Yes", "No");
                " TwitterConnected:" + BoolName(info.TwitterConnected, "Yes", "No");
    }

    AZStd::string UserInfoListToString(const UserInfoList & info)
    {
        AZStd::string strList;

        for (const auto & i : info)
        {
            if (!strList.empty())
            {
                strList += ",";
            }

            strList += "{";
            strList += UserInfoMiniString(i);
            strList += "}";
        }

        return strList;
    }

    AZStd::string FriendRecommendationToString(const FriendRecommendation & info)
    {
        return  UserInfoIDToString(info.User) +
                " Reason:" + info.Reason;
    }

    AZStd::string FriendRecommendationsToString(const FriendRecommendationList & info)
    {
        AZStd::string strList;

        for (const auto & i : info)
        {
            if (!strList.empty())
            {
                strList += ",";
            }

            strList += "{";
            strList += FriendRecommendationToString(i);
            strList += "}";
        }

        return strList;
    }

    AZStd::string FriendInfoToString(const FriendInfo & info)
    {
        return  UserInfoIDToString(info.User) +
                " CreatedDate:" + info.CreatedDate;
    }

    AZStd::string FriendListToString(const FriendList & info)
    {
        AZStd::string strList;

        for (const auto & i : info)
        {
            if (!strList.empty())
            {
                strList += ",";
            }

            strList += "{";
            strList += FriendInfoToString(i);
            strList += "}";
        }

        return strList;
    }

    AZStd::string FriendRequestToString(const FriendRequest & info)
    {
        return  UserInfoIDToString(info.User) +
                " IsRecommended:" + BoolName(info.IsRecommended, "Yes", "No") +
                " IsStranger:" + BoolName(info.IsStranger, "Yes", "No") +
                " NonStrangerReason:" + info.NonStrangerReason +
                " RequestedDate:" + info.RequestedDate;
    }

    AZStd::string FriendRequestListToString(const FriendRequestList & info)
    {
        AZStd::string strList;

        for (const auto & i : info)
        {
            if (!strList.empty())
            {
                strList += ",";
            }

            strList += "{";
            strList += FriendRequestToString(i);
            strList += "}";
        }

        return strList;
    }

    AZStd::string PresenceStatusToString(const PresenceStatus & info)
    {
        return  "UserID:" + info.UserID +
                " Index:" + AZStd::string::format("%lld", info.Index) +
                " UpdatedDate:" + AZStd::string::format("%lld", info.UpdatedDate) +
                " ActivityType:" + PresenceActivityTypeToString(info.ActivityType) +
                " Availability:" + PresenceAvailabilityToString(info.Availability);
    }

    AZStd::string PresenceStatusListToString(const PresenceStatusList & info)
    {
        AZStd::string strList;

        for (const auto & i : info)
        {
            if (!strList.empty())
            {
                strList += ",";
            }

            strList += "{";
            strList += PresenceStatusToString(i);
            strList += "}";
        }

        return strList;
    }

    AZStd::string PresenceSettingsToString(const PresenceSettings & info)
    {
        return  "IsInvisible:" + BoolName(info.IsInvisible, "Yes", "No") +
                " ShareActivity:" + BoolName(info.ShareActivity, "Shared", "None");
    }

    AZStd::string ChannelInfoToString(const ChannelInfo & info)
    {
        return  "Followers:" + AZStd::string::format("%llu", info.NumFollowers) +
                "Views:" + AZStd::string::format("%llu", info.NumViews) +
                "ItemsRecieved:" + AZStd::string::format("%llu", info.NumItemsRecieved) +
                "Partner:" + BoolName(info.Partner, "Yes", "No") +
                "Mature:" + BoolName(info.Mature, "Yes", "No") +
                "Id:" + info.Id +
                "BroadcasterLanguage:" + info.BroadcasterLanguage +
                "DisplayName:" + info.DisplayName +
                "eMail:" + info.eMail +
                "GameName:" + info.GameName +
                "Language:" + info.Lanugage +
                "Logo:" + info.Logo +
                "Name:" + info.Name +
                "ProfileBanner:" + info.ProfileBanner +
                "ProfileBannerBackgroundColor:" + info.ProfileBannerBackgroundColor +
                "Status:" + info.Status +
                "StreamKey:" + info.StreamKey +
                "UpdatedDate:" + info.UpdatedDate +
                "CreatedDate:" + info.CreatedDate +
                "URL:" + info.URL +
                "VideoBanner:" + info.VideoBanner;
    }

    AZStd::string FollowerToString(const Follower& info)
    {
        return  UserInfoIDToString(info.User) +
                " CreatedDate:" + info.CreatedDate +
                " Notifications:" + BoolName(info.Notifications, "On", "Off");
    }

    AZStd::string FollowerListToString(const FollowerList & info)
    {
        AZStd::string strList;

        for (const auto & i : info)
        {
            if (!strList.empty())
            {
                strList += ",";
            }

            strList += "{";
            strList += FollowerToString(i);
            strList += "}";
        }

        return strList;
    }

    AZStd::string TeamInfoToString(const TeamInfo& info)
    {
        return  "ID:" + info.ID +
                " Background:" + info.Background +
                " Banner:" + info.Banner +
                " CreatedDate:" + info.CreatedDate +
                " DisplayName:" + info.DisplayName +
                " Info:" + info.Info +
                " Logo:" + info.Logo +
                " Name:" + info.Name +
                " UpdatedDate:" + info.UpdatedDate;
    }

    AZStd::string TeamInfoListToString(const TeamInfoList& info)
    {
        AZStd::string strList;

        for (const auto & i : info)
        {
            if (!strList.empty())
            {
                strList += ",";
            }

            strList += "{";
            strList += TeamInfoToString(i);
            strList += "}";
        }

        return strList;
    }

    AZStd::string SubscriberInfoToString(const SubscriberInfo& info)
    {
        return  "ID:" + info.ID +
                " CreatedDate:" + info.CreatedDate +
                UserInfoIDToString(info.User);
    }

    AZStd::string SubscriberInfoListToString(const SubscriberInfoList& info)
    {
        AZStd::string strList;

        for (const auto & i : info)
        {
            if (!strList.empty())
            {
                strList += ",";
            }

            strList += "{";
            strList += SubscriberInfoToString(i);
            strList += "}";
        }

        return strList;
    }

    AZStd::string VideoInfoShortToString(const VideoInfo& info)
    {
        return "ID:" + info.ID;
    }

    AZStd::string VideoInfoListToString(const VideoInfoList& info)
    {
        AZStd::string strList;

        for (const auto & i : info)
        {
            if (!strList.empty())
            {
                strList += ",";
            }

            strList += "{";
            strList += VideoInfoShortToString(i);
            strList += "}";
        }

        return strList;
    }

    AZStd::string StartChannelCommercialResultToString(const StartChannelCommercialResult& info)
    {
        return  "Duration:" + AZStd::string::format("%llu", info.Duration) +
                " RetryAfter:" + AZStd::string::format("%llu", info.RetryAfter) +
                " Message:" + info.Message;
    }

    AZStd::string CommunityInfoToString(const CommunityInfo& info)
    {
        return  "ID:" + info.ID +
                " AvatarImageURL:" + info.AvatarImageURL +
                " CoverImageURL:" + info.CoverImageURL +
                " Description:" + info.Description +
                " DescriptionHTML:" + info.DescriptionHTML +
                " Language:" + info.Language +
                " Name:" + info.Name +
                " OwnerID:" + info.OwnerID +
                " Rules:" + info.Rules +
                " RulesHTML:" + info.RulesHTML +
                " Summary:" + info.Summary;
    }

    AZStd::string CommunityInfoListToString(const CommunityInfoList& info)
    {
        AZStd::string strList;

        for (const auto & i : info)
        {
            if (!strList.empty())
            {
                strList += ",";
            }

            strList += "{";
            strList += CommunityInfoToString(i);
            strList += "}";
        }

        return strList;
    }

    AZStd::string ReturnValueToString(const ReturnValue& info)
    {
        return  "ReceiptID:" + AZStd::string::format("%llu", info.GetID()) +
                " Result: " + ResultCodeToString(info.Result);
    }

    AZStd::string Int64Value::ToString() const
    {
        return  ReturnValueToString(*this) +
                AZStd::string::format(" Int64:%lld", Value);
    }

    AZStd::string Uint64Value::ToString() const
    {
        return  ReturnValueToString(*this) +
                AZStd::string::format(" Uint64:%llu", Value);
    }

    AZStd::string StringValue::ToString() const
    {
        return  ReturnValueToString(*this) +
                " String:" + "\"" + Value + "\"";
    }

    AZStd::string UserInfoValue::ToString() const
    {
        return  ReturnValueToString(*this) +
                UserInfoToString(Value);
    }

    AZStd::string FriendRecommendationValue::ToString() const
    {
        return  ReturnValueToString(*this) +
                " ListSize:" + AZStd::string::format("%llu-", static_cast<AZ::u64>(Value.size())) +
                " Recommendations:" + FriendRecommendationsToString(Value);
    }

    AZStd::string GetFriendValue::ToString() const
    {
        return  ReturnValueToString(*this) +
                " ListSize:" + AZStd::string::format("%llu-", static_cast<AZ::u64>(Value.Friends.size())) +
                " Cursor:" + Value.Cursor +
                " Friends:" + FriendListToString(Value.Friends);
    }

    AZStd::string FriendStatusValue::ToString() const
    {
        return  ReturnValueToString(*this) +
                " Status:" + Value.Status +
                UserInfoToString(Value.User);
    }

    AZStd::string FriendRequestValue::ToString() const
    {
        return  ReturnValueToString(*this) +
                " Total:" + AZStd::string::format("%llu", Value.Total) +
                " Cursor:" + Value.Cursor +
                " Requests:" + FriendRequestListToString(Value.Requests);
    }

    AZStd::string PresenceStatusValue::ToString() const
    {
        return  ReturnValueToString(*this) +
                " Total:" + AZStd::string::format("%llu", static_cast<AZ::u64>(Value.size())) +
                " StatusList:" + PresenceStatusListToString(Value);
    }

    AZStd::string PresenceSettingsValue::ToString() const
    {
        return  ReturnValueToString(*this) +
                " " + PresenceSettingsToString(Value);
    }

    AZStd::string ChannelInfoValue::ToString() const
    {
        return  ReturnValueToString(*this) +
                " " + ChannelInfoToString(Value);
    }

    AZStd::string UserInfoListValue::ToString() const
    {
        return  ReturnValueToString(*this) +
                " Total:" + AZStd::string::format("%llu", static_cast<AZ::u64>(Value.size())) +
                " Users:" + UserInfoListToString(Value);
    }

    AZStd::string FollowerResultValue::ToString() const
    {
        return  ReturnValueToString(*this) +
                " Total:" + AZStd::string::format("%llu", Value.Total) +
                " Cursor:" + Value.Cursor +
                " Followers:" + FollowerListToString(Value.Followers);
    }

    AZStd::string ChannelTeamValue::ToString() const
    {
        return  ReturnValueToString(*this) +
                " Total:" + AZStd::string::format("%llu", static_cast<AZ::u64>(Value.size())) +
                " Teams:" + TeamInfoListToString(Value);
    }

    AZStd::string SubscriberValue::ToString() const
    {
        return  ReturnValueToString(*this) +
                " Total:" + AZStd::string::format("%llu", Value.Total) +
                " Subscribers:" + SubscriberInfoListToString(Value.Subscribers);
    }

    AZStd::string SubscriberbyUserValue::ToString() const
    {
        return  ReturnValueToString(*this) +
                " SubscriberInfo:" + SubscriberInfoToString(Value);
    }

    AZStd::string VideoReturnValue::ToString() const
    {
        return  ReturnValueToString(*this) +
                " Total:" + AZStd::string::format("%llu", Value.Total) +
                " Videos:" + VideoInfoListToString(Value.Videos);
    }

    AZStd::string StartChannelCommercialValue::ToString() const
    {
        return  ReturnValueToString(*this) +
                " " + StartChannelCommercialResultToString(Value);
    }

    AZStd::string CommunityInfoValue::ToString() const
    {
        return  ReturnValueToString(*this) +
                " " + CommunityInfoToString(Value);
    }

    AZStd::string CommunityInfoReturnValue::ToString() const
    {
        return  ReturnValueToString(*this) +
            " Total:" + AZStd::string::format("%llu", Value.Total) +
            " Communities:" + CommunityInfoListToString(Value.Communities);
    }

    namespace Internal
    {
        //! Reflect a Value class     
        #define ReflectValueSerialization(context, _type)                                                       \
            context.Class<_type>()->                                                                            \
                Version(0)->                                                                                    \
                Field("Value", &_type::Value)->                                                                 \
                FieldFromBase<_type>("Result", &_type::Result);

        void ReflectSerialization(AZ::SerializeContext& context)
        {
            context.Class<ReceiptID>()->
                Version(0)
                ;
            ReflectValueSerialization(context, Int64Value);
            ReflectValueSerialization(context, Uint64Value);
            ReflectValueSerialization(context, StringValue);

            context.Class<UserNotifications>()->
                Version(0)->
                Field("EMail", &UserNotifications::EMail)->
                Field("Push", &UserNotifications::Push)
                ;

            context.Class<UserInfo>()->
                Version(0)->
                Field("ID", &UserInfo::ID)->
                Field("Bio", &UserInfo::Bio)->
                Field("CreatedDate", &UserInfo::CreatedDate)->
                Field("DisplayName", &UserInfo::DisplayName)->
                Field("EMail", &UserInfo::EMail)->
                Field("Logo", &UserInfo::Logo)->
                Field("Name", &UserInfo::Name)->
                Field("ProfileBanner", &UserInfo::ProfileBanner)->
                Field("ProfileBannerBackgroundColor", &UserInfo::ProfileBannerBackgroundColor)->
                Field("Type", &UserInfo::Type)->
                Field("UpdatedDate", &UserInfo::UpdatedDate)->
                Field("Notifications", &UserInfo::Notifications)->
                Field("EMailVerified", &UserInfo::EMailVerified)->
                Field("Partnered", &UserInfo::Partnered)->
                Field("TwitterConnected", &UserInfo::TwitterConnected)
                ;
            ReflectValueSerialization(context, UserInfoValue);

            context.Class<FriendRecommendation>()->
                Version(0)->
                Field("Reason", &FriendRecommendation::Reason)->
                Field("User", &FriendRecommendation::User)
                ;
            ReflectValueSerialization(context, FriendRecommendationValue);

            context.Class<GetFriendReturn>()->
                Version(0)->
                Field("Cursor", &GetFriendReturn::Cursor)->
                Field("Friends", &GetFriendReturn::Friends)
                ;
            ReflectValueSerialization(context, GetFriendValue);

            context.Class<FriendStatus>()->
                Version(0)->
                Field("Status", &FriendStatus::Status)->
                Field("User", &FriendStatus::User)
                ;
            ReflectValueSerialization(context, FriendStatusValue);

            context.Class<FriendRequest>()->
                Version(0)->
                Field("IsRecommended", &FriendRequest::IsRecommended)->
                Field("IsStranger", &FriendRequest::IsStranger)->
                Field("NonStrangerReason", &FriendRequest::NonStrangerReason)->
                Field("RequestedDate", &FriendRequest::RequestedDate)->
                Field("User", &FriendRequest::User)
                ;

            context.Class<FriendRequestResult>()->
                Version(0)->
                Field("Total", &FriendRequestResult::Total)->
                Field("Cursor", &FriendRequestResult::Cursor)->
                Field("Requests", &FriendRequestResult::Requests)
                ;
            ReflectValueSerialization(context, FriendRequestValue);

            context.Class<PresenceStatus>()->
                Version(0)->
                Field("ActivityType", &PresenceStatus::ActivityType)->
                Field("Availability", &PresenceStatus::Availability)->
                Field("Index", &PresenceStatus::Index)->
                Field("UpdatedDate", &PresenceStatus::UpdatedDate)->
                Field("UserID", &PresenceStatus::UserID)
                ;
            ReflectValueSerialization(context, PresenceStatusValue);

            context.Class<PresenceSettings>()->
                Version(0)->
                Field("IsInvisible", &PresenceSettings::IsInvisible)->
                Field("ShareActivity", &PresenceSettings::ShareActivity)
                ;
            ReflectValueSerialization(context, PresenceSettingsValue);

            context.Class<ChannelInfo>()->
                Version(0)->
                Field("NumFollowers", &ChannelInfo::NumFollowers)->
                Field("NumViews", &ChannelInfo::NumViews)->
                Field("NumItemsRecieved", &ChannelInfo::NumItemsRecieved)->
                Field("Partner", &ChannelInfo::Partner)->
                Field("Mature", &ChannelInfo::Mature)->
                Field("Id", &ChannelInfo::Id)->
                Field("BroadcasterLanguage", &ChannelInfo::BroadcasterLanguage)->
                Field("DisplayName", &ChannelInfo::DisplayName)->
                Field("eMail", &ChannelInfo::eMail)->
                Field("GameName", &ChannelInfo::GameName)->
                Field("Lanugage", &ChannelInfo::Lanugage)->
                Field("Logo", &ChannelInfo::Logo)->
                Field("Name", &ChannelInfo::Name)->
                Field("ProfileBanner", &ChannelInfo::ProfileBanner)->
                Field("ProfileBannerBackgroundColor", &ChannelInfo::ProfileBannerBackgroundColor)->
                Field("Status", &ChannelInfo::Status)->
                Field("StreamKey", &ChannelInfo::StreamKey)->
                Field("UpdatedDate", &ChannelInfo::UpdatedDate)->
                Field("CreatedDate", &ChannelInfo::CreatedDate)->
                Field("URL", &ChannelInfo::URL)->
                Field("VideoBanner", &ChannelInfo::VideoBanner)
                ;
            ReflectValueSerialization(context, ChannelInfoValue);

            context.Class<UpdateValuebool>()->
                Version(0)
                ;

            context.Class<UpdateValueuint>()->
                Version(0)
                ;

            context.Class<UpdateValuestring>()->
                Version(0)
                ;

            context.Class<ChannelUpdateInfo>()->
                Version(0)->
                Field("ChannelFeedEnabled", &ChannelUpdateInfo::ChannelFeedEnabled)->
                Field("Delay", &ChannelUpdateInfo::Delay)->
                Field("Status", &ChannelUpdateInfo::Status)->
                Field("GameName", &ChannelUpdateInfo::GameName)
                ;
            ReflectValueSerialization(context, UserInfoListValue);

            context.Class<Follower>()->
                Version(0)->
                Field("Notifications", &Follower::Notifications)->
                Field("CreatedDate", &Follower::CreatedDate)->
                Field("User", &Follower::User)
                ;

            context.Class<FollowerResult>()->
                Version(0)->
                Field("Total", &FollowerResult::Total)->
                Field("Cursor", &FollowerResult::Cursor)->
                Field("Followers", &FollowerResult::Followers)
                ;
            ReflectValueSerialization(context, FollowerResultValue);

            context.Class<TeamInfo>()->
                Version(0)->
                Field("ID", &TeamInfo::ID)->
                Field("Background", &TeamInfo::Background)->
                Field("Banner", &TeamInfo::Banner)->
                Field("CreatedDate", &TeamInfo::CreatedDate)->
                Field("DisplayName", &TeamInfo::DisplayName)->
                Field("Info", &TeamInfo::Info)->
                Field("Logo", &TeamInfo::Logo)->
                Field("Name", &TeamInfo::Name)->
                Field("UpdatedDate", &TeamInfo::UpdatedDate)
                ;
            ReflectValueSerialization(context, ChannelTeamValue);

            context.Class<SubscriberInfo>()->
                Version(0)->
                Field("ID", &SubscriberInfo::ID)->
                Field("CreatedDate", &SubscriberInfo::CreatedDate)->
                Field("User", &SubscriberInfo::User)
                ;

            context.Class<Subscription>()->
                Version(0)->
                Field("Total", &Subscription::Total)->
                Field("Subscribers", &Subscription::Subscribers)
                ;
            ReflectValueSerialization(context, SubscriberValue);
            ReflectValueSerialization(context, SubscriberbyUserValue);

            context.Class<VideoChannelInfo>()->
                Version(0)->
                Field("ID", &VideoChannelInfo::ID)->
                Field("DisplayName", &VideoChannelInfo::DisplayName)->
                Field("Name", &VideoChannelInfo::Name)
                ;

            context.Class<FPSInfo>()->
                Version(0)->
                Field("Chunked", &FPSInfo::Chunked)->
                Field("High", &FPSInfo::High)->
                Field("Low", &FPSInfo::Low)->
                Field("Medium", &FPSInfo::Medium)->
                Field("Mobile", &FPSInfo::Mobile)
                ;

            context.Class<PreviewInfo>()->
                Version(0)->
                Field("Large", &PreviewInfo::Large)->
                Field("Medium", &PreviewInfo::Medium)->
                Field("Small", &PreviewInfo::Small)->
                Field("Template", &PreviewInfo::Template)
                ;

            context.Class<ResolutionsInfo>()->
                Version(0)->
                Field("Chunked", &ResolutionsInfo::Chunked)->
                Field("High", &ResolutionsInfo::High)->
                Field("Low", &ResolutionsInfo::Low)->
                Field("Medium", &ResolutionsInfo::Medium)->
                Field("Mobile", &ResolutionsInfo::Mobile)
                ;

            context.Class<ThumbnailInfo>()->
                Version(0)->
                Field("Type", &ThumbnailInfo::Type)->
                Field("Url", &ThumbnailInfo::Url)
                ;

            context.Class<ThumbnailsInfo>()->
                Version(0)->
                Field("Large", &ThumbnailsInfo::Large)->
                Field("Medium", &ThumbnailsInfo::Medium)->
                Field("Small", &ThumbnailsInfo::Small)->
                Field("Template", &ThumbnailsInfo::Template)
                ;

            context.Class<VideoInfo>()->
                Version(0)->
                Field("Length", &VideoInfo::Length)->
                Field("Views", &VideoInfo::Views)->
                Field("BroadcastID", &VideoInfo::BroadcastID)->
                Field("Type", &VideoInfo::Type)->
                Field("CreatedDate", &VideoInfo::CreatedDate)->
                Field("Description", &VideoInfo::Description)->
                Field("DescriptionHTML", &VideoInfo::DescriptionHTML)->
                Field("ID", &VideoInfo::ID)->
                Field("Game", &VideoInfo::Game)->
                Field("Language", &VideoInfo::Language)->
                Field("PublishedDate", &VideoInfo::PublishedDate)->
                Field("Status", &VideoInfo::Status)->
                Field("TagList", &VideoInfo::TagList)->
                Field("Title", &VideoInfo::Title)->
                Field("URL", &VideoInfo::URL)->
                Field("Viewable", &VideoInfo::Viewable)->
                Field("ViewableAt", &VideoInfo::ViewableAt)->
                Field("Channel", &VideoInfo::Channel)->
                Field("FPS", &VideoInfo::FPS)->
                Field("Preview", &VideoInfo::Preview)->
                Field("Thumbnails", &VideoInfo::Thumbnails)->
                Field("Resolutions", &VideoInfo::Resolutions)
                ;

            context.Class<VideoReturn>()->
                Version(0)->
                Field("Total", &VideoReturn::Total)->
                Field("Videos", &VideoReturn::Videos)
                ;
            ReflectValueSerialization(context, VideoReturnValue);

            context.Class<StartChannelCommercialResult>()->
                Version(0)->
                Field("Duration", &StartChannelCommercialResult::Duration)->
                Field("RetryAfter", &StartChannelCommercialResult::RetryAfter)->
                Field("Message", &StartChannelCommercialResult::Message)
                ;
            ReflectValueSerialization(context, StartChannelCommercialValue);

            context.Class<CommunityInfo>()->
                Version(0)->
                Field("ID", &CommunityInfo::ID)->
                Field("AvatarImageURL", &CommunityInfo::AvatarImageURL)->
                Field("CoverImageURL", &CommunityInfo::CoverImageURL)->
                Field("Description", &CommunityInfo::Description)->
                Field("DescriptionHTML", &CommunityInfo::DescriptionHTML)->
                Field("Language", &CommunityInfo::Language)->
                Field("Name", &CommunityInfo::Name)->
                Field("OwnerID", &CommunityInfo::OwnerID)->
                Field("Rules", &CommunityInfo::Rules)->
                Field("RulesHTML", &CommunityInfo::RulesHTML)->
                Field("Summary", &CommunityInfo::Summary)
                ;
            ReflectValueSerialization(context, CommunityInfoValue);

            context.Class<CommunityInfoReturn>()->
                Version(0)->
                Field("Total", &CommunityInfoReturn::Total)->
                Field("Communities", &CommunityInfoReturn::Communities)
                ;
            ReflectValueSerialization(context, CommunityInfoReturnValue);
        }

        class BehaviorTwitchNotifyBus
            : public TwitchNotifyBus::Handler
            , public AZ::BehaviorEBusHandler
        {
        public:
            AZ_EBUS_BEHAVIOR_BINDER(BehaviorTwitchNotifyBus, "{63EEA49D-1205-4E43-9451-26ACF5771901}", AZ::SystemAllocator,
                UserIDNotify,
                OAuthTokenNotify,
                GetUser,
                ResetFriendsNotificationCountNotify,
                GetFriendNotificationCount,
                GetFriendRecommendations,
                GetFriends,
                GetFriendStatus,
                AcceptFriendRequest,
                GetFriendRequests,
                CreateFriendRequest,
                DeclineFriendRequest,
                UpdatePresenceStatus,
                GetPresenceStatusofFriends,
                GetPresenceSettings,
                UpdatePresenceSettings,
                GetChannel,
                GetChannelbyID,
                UpdateChannel,
                GetChannelEditors,
                GetChannelFollowers,
                GetChannelTeams,
                GetChannelSubscribers,
                CheckChannelSubscriptionbyUser,
                GetChannelVideos,
                StartChannelCommercial,
                ResetChannelStreamKey);

            void UserIDNotify(const StringValue& userID)
            {
                Call(FN_UserIDNotify, userID);
            }

            void OAuthTokenNotify(const StringValue& token)
            {
                Call(FN_OAuthTokenNotify, token);
            }

            void GetUser(const UserInfoValue& result)
            {
                Call(FN_GetUser, result);
            }

            void ResetFriendsNotificationCountNotify(const Int64Value& result)
            {
                Call(FN_ResetFriendsNotificationCountNotify, result);
            }

            void GetFriendNotificationCount(const Int64Value& result)
            {
                Call(FN_GetFriendNotificationCount, result);
            }

            void GetFriendRecommendations(const FriendRecommendationValue& result)
            {
                Call(FN_GetFriendRecommendations, result);
            }

            void GetFriends(const GetFriendValue& result)
            {
                Call(FN_GetFriends, result);
            }

            void GetFriendStatus(const FriendStatusValue& result)
            {
                Call(FN_GetFriendStatus, result);
            }

            void AcceptFriendRequest(const Int64Value& result)
            {
                Call(FN_AcceptFriendRequest, result);
            }

            void GetFriendRequests(const FriendRequestValue& result)
            {
                Call(FN_GetFriendRequests, result);
            }

            void CreateFriendRequest(const Int64Value& result)
            {
                Call(FN_CreateFriendRequest, result);
            }

            void DeclineFriendRequest(const Int64Value& result)
            {
                Call(FN_DeclineFriendRequest, result);
            }

            void UpdatePresenceStatus(const Int64Value& result)
            {
                Call(FN_UpdatePresenceStatus, result);
            }

            void GetPresenceStatusofFriends(const PresenceStatusValue& result)
            {
                Call(FN_GetPresenceStatusofFriends, result);
            }

            void GetPresenceSettings(const PresenceSettingsValue& result)
            {
                Call(FN_GetPresenceSettings, result);
            }

            void UpdatePresenceSettings(const PresenceSettingsValue& result)
            {
                Call(FN_UpdatePresenceSettings, result);
            }

            void GetChannelbyID(const ChannelInfoValue& result)
            {
                Call(FN_GetChannelbyID, result);
            }

            void GetChannel(const ChannelInfoValue& result)
            {
                Call(FN_GetChannel, result);
            }

            void UpdateChannel(const ChannelInfoValue& result)
            {
                Call(FN_UpdateChannel, result);
            }

            void GetChannelEditors(const UserInfoListValue& result)
            {
                Call(FN_GetChannelEditors, result);
            }

            void GetChannelFollowers(const FollowerResultValue& result)
            {
                Call(FN_GetChannelFollowers, result);
            }

            void GetChannelTeams(const ChannelTeamValue& result)
            {
                Call(FN_GetChannelTeams, result);
            }

            void GetChannelSubscribers(const SubscriberValue& result)
            {
                Call(FN_GetChannelSubscribers, result);
            }

            void CheckChannelSubscriptionbyUser(const SubscriberbyUserValue& result)
            {
                Call(FN_CheckChannelSubscriptionbyUser, result);
            }

            void GetChannelVideos(const VideoReturnValue& result)
            {
                Call(FN_GetChannelVideos, result);
            }

            void StartChannelCommercial(const StartChannelCommercialValue& result)
            {
                Call(FN_StartChannelCommercial, result);
            }

            void ResetChannelStreamKey(const ChannelInfoValue& result)
            {
                Call(FN_ResetChannelStreamKey, result);
            }
        };

        /*
        ** helper macro for reflecting the enums
        */

        #define ENUM_CLASS_HELPER(className, enumName) Enum<(int)className::enumName>(#enumName)

        //! Reflect a Result class     
        #define ReflectValueClass(context, _type)                                                               \
            context.Class<_type>()->                                                                            \
                Property("Value", [](const _type& value) { return value.Value; }, nullptr)->                    \
                Property("Result", [](const _type& value) { return value.Result; }, nullptr)->                  \
                Method("ToString", &_type::ToString)->                                                          \
                Attribute(AZ::Script::Attributes::Operator, AZ::Script::Attributes::OperatorType::ToString)     \
                ;

        void Reflect(AZ::BehaviorContext & context)
        {
            /*
            ** Reflect the enum's
            */

            context.Class<ResultCode>("ResultCode")->
                ENUM_CLASS_HELPER(ResultCode, Success)->
                ENUM_CLASS_HELPER(ResultCode, InvalidParam)->
                ENUM_CLASS_HELPER(ResultCode, TwitchRESTError)->
                ENUM_CLASS_HELPER(ResultCode, TwitchChannelNoUpdatesToMake)->
                ENUM_CLASS_HELPER(ResultCode, Unknown)
                ;

            context.Class<PresenceAvailability>("PresenceAvailability")->
                ENUM_CLASS_HELPER(PresenceAvailability, Unknown)->
                ENUM_CLASS_HELPER(PresenceAvailability, Online)->
                ENUM_CLASS_HELPER(PresenceAvailability, Idle)
                ;

            context.Class<PresenceActivityType>("PresenceActivityType")->
                ENUM_CLASS_HELPER(PresenceActivityType, Unknown)->
                ENUM_CLASS_HELPER(PresenceActivityType, Watching)->
                ENUM_CLASS_HELPER(PresenceActivityType, Playing)->
                ENUM_CLASS_HELPER(PresenceActivityType, Broadcasting)
                ;

            context.Class<BroadCastType>("BroadCastType")->
                ENUM_CLASS_HELPER(BroadCastType, Default)->
                ENUM_CLASS_HELPER(BroadCastType, Archive)->
                ENUM_CLASS_HELPER(BroadCastType, Highlight)->
                ENUM_CLASS_HELPER(BroadCastType, Upload)->
                ENUM_CLASS_HELPER(BroadCastType, ArchiveAndHighlight)->
                ENUM_CLASS_HELPER(BroadCastType, ArchiveAndUpload)->
                ENUM_CLASS_HELPER(BroadCastType, ArchiveAndHighlightAndUpload)->
                ENUM_CLASS_HELPER(BroadCastType, HighlightAndUpload)
                ;

            context.Class<CommercialLength>("CommercialLength")->
                ENUM_CLASS_HELPER(CommercialLength, T30Seconds)->
                ENUM_CLASS_HELPER(CommercialLength, T60Seconds)->
                ENUM_CLASS_HELPER(CommercialLength, T90Seconds)->
                ENUM_CLASS_HELPER(CommercialLength, T120Seconds)->
                ENUM_CLASS_HELPER(CommercialLength, T150Seconds)->
                ENUM_CLASS_HELPER(CommercialLength, T180Seconds)
                ;

            context.Class<ReceiptID>()->
                Method("Equal", &ReceiptID::operator==)->
                    Attribute(AZ::Script::Attributes::Operator, AZ::Script::Attributes::OperatorType::Equal)->
                Property("ID", &ReceiptID::GetID, &ReceiptID::SetID)
                ;
            ReflectValueClass(context, Int64Value);
            ReflectValueClass(context, Uint64Value);
            ReflectValueClass(context, StringValue);

            context.Class<UserNotifications>()->
                Property("EMail", [](const UserNotifications& value) { return value.EMail; }, nullptr)->
                Property("Push", [](const UserNotifications& value) { return value.Push; }, nullptr)
                ;

            context.Class<UserInfo>()->
                Property("ID", [](const UserInfo& value) { return value.ID; }, nullptr)->
                Property("Bio", [](const UserInfo& value) { return value.Bio; }, nullptr)->
                Property("CreatedDate", [](const UserInfo& value) { return value.CreatedDate; }, nullptr)->
                Property("DisplayName", [](const UserInfo& value) { return value.DisplayName; }, nullptr)->
                Property("EMail", [](const UserInfo& value) { return value.EMail; }, nullptr)->
                Property("Logo", [](const UserInfo& value) { return value.Logo; }, nullptr)->
                Property("Name", [](const UserInfo& value) { return value.Name; }, nullptr)->
                Property("ProfileBanner", [](const UserInfo& value) { return value.ProfileBanner; }, nullptr)->
                Property("ProfileBannerBackgroundColor", [](const UserInfo& value) { return value.ProfileBannerBackgroundColor; }, nullptr)->
                Property("Type", [](const UserInfo& value) { return value.Type; }, nullptr)->
                Property("UpdatedDate", [](const UserInfo& value) { return value.UpdatedDate; }, nullptr)->
                Property("Notifications", [](const UserInfo& value) { return value.Notifications; }, nullptr)->
                Property("EMailVerified", [](const UserInfo& value) { return value.EMailVerified; }, nullptr)->
                Property("Partnered", [](const UserInfo& value) { return value.Partnered; }, nullptr)->
                Property("TwitterConnected", [](const UserInfo& value) { return value.TwitterConnected; }, nullptr)
                ;
            ReflectValueClass(context, UserInfoValue);

            context.Class<FriendRecommendation>()->
                Property("Reason", [](const FriendRecommendation& value) { return value.Reason; }, nullptr)->
                Property("User", [](const FriendRecommendation& value) { return value.User; }, nullptr)
                ;
            ReflectValueClass(context, FriendRecommendationValue);

            context.Class<GetFriendReturn>()->
                Property("Cursor", [](const GetFriendReturn& value) { return value.Cursor; }, nullptr)->
                Property("Friends", [](const GetFriendReturn& value) { return value.Friends; }, nullptr)
                ;
            ReflectValueClass(context, GetFriendValue);

            context.Class<FriendStatus>()->
                Property("Status", [](const FriendStatus& value) { return value.Status; }, nullptr)->
                Property("User", [](const FriendStatus& value) { return value.User; }, nullptr)
                ;
            ReflectValueClass(context, FriendStatusValue);

            context.Class<FriendRequest>()->
                Property("IsRecommended", [](const FriendRequest& value) { return value.IsRecommended; }, nullptr)->
                Property("IsStranger", [](const FriendRequest& value) { return value.IsStranger; }, nullptr)->
                Property("NonStrangerReason", [](const FriendRequest& value) { return value.NonStrangerReason; }, nullptr)->
                Property("RequestedDate", [](const FriendRequest& value) { return value.RequestedDate; }, nullptr)->
                Property("User", [](const FriendRequest& value) { return value.User; }, nullptr)
                ;

            context.Class<FriendRequestResult>()->
                Property("Total", [](const FriendRequestResult& value) { return value.Total; }, nullptr)->
                Property("Cursor", [](const FriendRequestResult& value) { return value.Cursor; }, nullptr)->
                Property("Requests", [](const FriendRequestResult& value) { return value.Requests; }, nullptr)
                ;
            ReflectValueClass(context, FriendRequestValue);

            context.Class<PresenceStatus>()->
                Property("ActivityType", [](const PresenceStatus& value) { return value.ActivityType; }, nullptr)->
                Property("Availability", [](const PresenceStatus& value) { return value.Availability; }, nullptr)->
                Property("Index", [](const PresenceStatus& value) { return value.Index; }, nullptr)->
                Property("UpdatedDate", [](const PresenceStatus& value) { return value.UpdatedDate; }, nullptr)->
                Property("UserID", [](const PresenceStatus& value) { return value.UserID; }, nullptr)
                ;
            ReflectValueClass(context, PresenceStatusValue);

            context.Class<PresenceSettings>()->
                Property("IsInvisible", [](const PresenceSettings& value) { return value.IsInvisible; }, nullptr)->
                Property("ShareActivity", [](const PresenceSettings& value) { return value.ShareActivity; }, nullptr)
                ;
            ReflectValueClass(context, PresenceSettingsValue);

            context.Class<ChannelInfo>()->
                Property("NumFollowers", [](const ChannelInfo& value) { return value.NumFollowers; }, nullptr)->
                Property("NumViews", [](const ChannelInfo& value) { return value.NumViews; }, nullptr)->
                Property("NumItemsRecieved", [](const ChannelInfo& value) { return value.NumItemsRecieved; }, nullptr)->
                Property("Partner", [](const ChannelInfo& value) { return value.Partner; }, nullptr)->
                Property("Mature", [](const ChannelInfo& value) { return value.Mature; }, nullptr)->
                Property("Id", [](const ChannelInfo& value) { return value.Id; }, nullptr)->
                Property("BroadcasterLanguage", [](const ChannelInfo& value) { return value.BroadcasterLanguage; }, nullptr)->
                Property("DisplayName", [](const ChannelInfo& value) { return value.DisplayName; }, nullptr)->
                Property("eMail", [](const ChannelInfo& value) { return value.eMail; }, nullptr)->
                Property("GameName", [](const ChannelInfo& value) { return value.GameName; }, nullptr)->
                Property("Lanugage", [](const ChannelInfo& value) { return value.Lanugage; }, nullptr)->
                Property("Logo", [](const ChannelInfo& value) { return value.Logo; }, nullptr)->
                Property("Name", [](const ChannelInfo& value) { return value.Name; }, nullptr)->
                Property("ProfileBanner", [](const ChannelInfo& value) { return value.ProfileBanner; }, nullptr)->
                Property("ProfileBannerBackgroundColor", [](const ChannelInfo& value) { return value.ProfileBannerBackgroundColor; }, nullptr)->
                Property("Status", [](const ChannelInfo& value) { return value.Status; }, nullptr)->
                Property("StreamKey", [](const ChannelInfo& value) { return value.StreamKey; }, nullptr)->
                Property("UpdatedDate", [](const ChannelInfo& value) { return value.UpdatedDate; }, nullptr)->
                Property("CreatedDate", [](const ChannelInfo& value) { return value.CreatedDate; }, nullptr)->
                Property("URL", [](const ChannelInfo& value) { return value.URL; }, nullptr)->
                Property("VideoBanner", [](const ChannelInfo& value) { return value.VideoBanner; }, nullptr)
                ;
            ReflectValueClass(context, ChannelInfoValue);

            context.Class<UpdateValuebool>()->
                Property("Value", &UpdateValuebool::GetValue, &UpdateValuebool::SetValue)->
                Method("ToBeUpdated", &UpdateValuebool::ToBeUpdated)
                ;

            context.Class<UpdateValueuint>()->
                Property("Value", &UpdateValueuint::GetValue, &UpdateValueuint::SetValue)->
                Method("ToBeUpdated", &UpdateValueuint::ToBeUpdated)
                ;

            context.Class<UpdateValuestring>()->
                Property("Value", &UpdateValuestring::GetValue, &UpdateValuestring::SetValue)->
                Method("ToBeUpdated", &UpdateValuestring::ToBeUpdated)
                ;

            context.Class<ChannelUpdateInfo>()->
                Property("ChannelFeedEnabled", BehaviorValueProperty(&ChannelUpdateInfo::ChannelFeedEnabled))->
                Property("Delay", BehaviorValueProperty(&ChannelUpdateInfo::Delay))->
                Property("Status", BehaviorValueProperty(&ChannelUpdateInfo::Status))->
                Property("GameName", BehaviorValueProperty(&ChannelUpdateInfo::GameName))
                ;
            ReflectValueClass(context, UserInfoListValue);

            context.Class<Follower>()->
                Property("Notifications", [](const Follower& value) { return value.Notifications; }, nullptr)->
                Property("CreatedDate", [](const Follower& value) { return value.CreatedDate; }, nullptr)->
                Property("User", [](const Follower& value) { return value.User; }, nullptr)
                ;

            context.Class<FollowerResult>()->
                Property("Total", [](const FollowerResult& value) { return value.Total; }, nullptr)->
                Property("Cursor", [](const FollowerResult& value) { return value.Cursor; }, nullptr)->
                Property("Followers", [](const FollowerResult& value) { return value.Followers; }, nullptr)
                ;
            ReflectValueClass(context, FollowerResultValue);

            context.Class<TeamInfo>()->
                Property("ID", [](const TeamInfo& value) { return value.ID; }, nullptr)->
                Property("Background", [](const TeamInfo& value) { return value.Background; }, nullptr)->
                Property("Banner", [](const TeamInfo& value) { return value.Banner; }, nullptr)->
                Property("CreatedDate", [](const TeamInfo& value) { return value.CreatedDate; }, nullptr)->
                Property("DisplayName", [](const TeamInfo& value) { return value.DisplayName; }, nullptr)->
                Property("Info", [](const TeamInfo& value) { return value.Info; }, nullptr)->
                Property("Logo", [](const TeamInfo& value) { return value.Logo; }, nullptr)->
                Property("Name", [](const TeamInfo& value) { return value.Name; }, nullptr)->
                Property("UpdatedDate", [](const TeamInfo& value) { return value.UpdatedDate; }, nullptr)
                ;
            ReflectValueClass(context, ChannelTeamValue);

            context.Class<SubscriberInfo>()->
                Property("ID", [](const SubscriberInfo& value) { return value.ID; }, nullptr)->
                Property("CreatedDate", [](const SubscriberInfo& value) { return value.CreatedDate; }, nullptr)->
                Property("User", [](const SubscriberInfo& value) { return value.User; }, nullptr)
                ;

            context.Class<Subscription>()->
                Property("Total", [](const Subscription& value) { return value.Total; }, nullptr)->
                Property("Subscribers", [](const Subscription& value) { return value.Subscribers; }, nullptr)
                ;
            ReflectValueClass(context, SubscriberValue);
            ReflectValueClass(context, SubscriberbyUserValue);

            context.Class<VideoChannelInfo>()->
                Property("ID", [](const VideoChannelInfo& value) { return value.ID; }, nullptr)->
                Property("DisplayName", [](const VideoChannelInfo& value) { return value.DisplayName; }, nullptr)->
                Property("Name", [](const VideoChannelInfo& value) { return value.Name; }, nullptr)
                ;

            context.Class<FPSInfo>()->
                Property("Chunked", [](const FPSInfo& value) { return value.Chunked; }, nullptr)->
                Property("High", [](const FPSInfo& value) { return value.High; }, nullptr)->
                Property("Low", [](const FPSInfo& value) { return value.Low; }, nullptr)->
                Property("Medium", [](const FPSInfo& value) { return value.Medium; }, nullptr)->
                Property("Mobile", [](const FPSInfo& value) { return value.Mobile; }, nullptr)
                ;

            context.Class<PreviewInfo>()->
                Property("Large", [](const PreviewInfo& value) { return value.Large; }, nullptr)->
                Property("Medium", [](const PreviewInfo& value) { return value.Medium; }, nullptr)->
                Property("Small", [](const PreviewInfo& value) { return value.Small; }, nullptr)->
                Property("Template", [](const PreviewInfo& value) { return value.Template; }, nullptr)
                ;

            context.Class<ResolutionsInfo>()->
                Property("Chunked", [](const ResolutionsInfo& value) { return value.Chunked; }, nullptr)->
                Property("High", [](const ResolutionsInfo& value) { return value.High; }, nullptr)->
                Property("Low", [](const ResolutionsInfo& value) { return value.Low; }, nullptr)->
                Property("Medium", [](const ResolutionsInfo& value) { return value.Medium; }, nullptr)->
                Property("Mobile", [](const ResolutionsInfo& value) { return value.Mobile; }, nullptr)
                ;

            context.Class<ThumbnailInfo>()->
                Property("Type", [](const ThumbnailInfo& value) { return value.Type; }, nullptr)->
                Property("Url", [](const ThumbnailInfo& value) { return value.Url; }, nullptr)
                ;

            context.Class<ThumbnailsInfo>()->
                Property("Large", [](const ThumbnailsInfo& value) { return value.Large; }, nullptr)->
                Property("Medium", [](const ThumbnailsInfo& value) { return value.Medium; }, nullptr)->
                Property("Small", [](const ThumbnailsInfo& value) { return value.Small; }, nullptr)->
                Property("Template", [](const ThumbnailsInfo& value) { return value.Template; }, nullptr)
                ;

            context.Class<VideoInfo>()->
                Property("Length", [](const VideoInfo& value) { return value.Length; }, nullptr)->
                Property("Views", [](const VideoInfo& value) { return value.Views; }, nullptr)->
                Property("BroadcastID", [](const VideoInfo& value) { return value.BroadcastID; }, nullptr)->
                Property("Type", [](const VideoInfo& value) { return value.Type; }, nullptr)->
                Property("CreatedDate", [](const VideoInfo& value) { return value.CreatedDate; }, nullptr)->
                Property("Description", [](const VideoInfo& value) { return value.Description; }, nullptr)->
                Property("DescriptionHTML", [](const VideoInfo& value) { return value.DescriptionHTML; }, nullptr)->
                Property("ID", [](const VideoInfo& value) { return value.ID; }, nullptr)->
                Property("Game", [](const VideoInfo& value) { return value.Game; }, nullptr)->
                Property("Language", [](const VideoInfo& value) { return value.Language; }, nullptr)->
                Property("PublishedDate", [](const VideoInfo& value) { return value.PublishedDate; }, nullptr)->
                Property("Status", [](const VideoInfo& value) { return value.Status; }, nullptr)->
                Property("TagList", [](const VideoInfo& value) { return value.TagList; }, nullptr)->
                Property("Title", [](const VideoInfo& value) { return value.Title; }, nullptr)->
                Property("URL", [](const VideoInfo& value) { return value.URL; }, nullptr)->
                Property("Viewable", [](const VideoInfo& value) { return value.Viewable; }, nullptr)->
                Property("ViewableAt", [](const VideoInfo& value) { return value.ViewableAt; }, nullptr)->
                Property("Channel", [](const VideoInfo& value) { return value.Channel; }, nullptr)->
                Property("FPS", [](const VideoInfo& value) { return value.FPS; }, nullptr)->
                Property("Preview", [](const VideoInfo& value) { return value.Preview; }, nullptr)->
                Property("Thumbnails", [](const VideoInfo& value) { return value.Thumbnails; }, nullptr)->
                Property("Resolutions", [](const VideoInfo& value) { return value.Resolutions; }, nullptr)
                ;

            context.Class<VideoReturn>()->
                Property("Total", [](const VideoReturn& value) { return value.Total; }, nullptr)->
                Property("Videos", [](const VideoReturn& value) { return value.Videos; }, nullptr)
                ;
            ReflectValueClass(context, VideoReturnValue);

            context.Class<StartChannelCommercialResult>()->
                Property("Duration", [](const StartChannelCommercialResult& value) { return value.Duration; }, nullptr)->
                Property("RetryAfter", [](const StartChannelCommercialResult& value) { return value.RetryAfter; }, nullptr)->
                Property("Message", [](const StartChannelCommercialResult& value) { return value.Message; }, nullptr)
                ;
            ReflectValueClass(context, StartChannelCommercialValue);

            context.Class<CommunityInfo>()->
                Property("ID", [](const CommunityInfo& value) { return value.ID; }, nullptr)->
                Property("AvatarImageURL", [](const CommunityInfo& value) { return value.AvatarImageURL; }, nullptr)->
                Property("CoverImageURL", [](const CommunityInfo& value) { return value.CoverImageURL; }, nullptr)->
                Property("Description", [](const CommunityInfo& value) { return value.Description; }, nullptr)->
                Property("DescriptionHTML", [](const CommunityInfo& value) { return value.DescriptionHTML; }, nullptr)->
                Property("Language", [](const CommunityInfo& value) { return value.Language; }, nullptr)->
                Property("Name", [](const CommunityInfo& value) { return value.Name; }, nullptr)->
                Property("OwnerID", [](const CommunityInfo& value) { return value.OwnerID; }, nullptr)->
                Property("Rules", [](const CommunityInfo& value) { return value.Rules; }, nullptr)->
                Property("RulesHTML", [](const CommunityInfo& value) { return value.RulesHTML; }, nullptr)->
                Property("Summary", [](const CommunityInfo& value) { return value.Summary; }, nullptr)
                ;
            ReflectValueClass(context, CommunityInfoValue);

            context.Class<CommunityInfoReturn>()->
                Property("Total", [](const CommunityInfoReturn& value) { return value.Total; }, nullptr)->
                Property("Communities", [](const CommunityInfoReturn& value) { return value.Communities; }, nullptr)
                ;
            ReflectValueClass(context, CommunityInfoReturnValue);

            context.EBus<TwitchRequestBus>("TwitchRequestBus")
                ->Event("SetApplicationID", &TwitchRequestBus::Events::SetApplicationID)
                ->Event("GetApplicationID", &TwitchRequestBus::Events::GetApplicationID)
                ->Event("GetUserID", &TwitchRequestBus::Events::GetUserID)
                ->Event("GetOAuthToken", &TwitchRequestBus::Events::GetOAuthToken)
                ->Event("GetSessionID", &TwitchRequestBus::Events::GetSessionID)
                ->Event("SetUserID", &TwitchRequestBus::Events::SetUserID)
                ->Event("SetOAuthToken", &TwitchRequestBus::Events::SetOAuthToken)
                ->Event("RequestUserID", &TwitchRequestBus::Events::RequestUserID)
                ->Event("RequestOAuthToken", &TwitchRequestBus::Events::RequestOAuthToken)
                ->Event("GetUser", &TwitchRequestBus::Events::GetUser)
                ->Event("ResetFriendsNotificationCount", &TwitchRequestBus::Events::ResetFriendsNotificationCount)
                ->Event("GetFriendNotificationCount", &TwitchRequestBus::Events::GetFriendNotificationCount)
                ->Event("GetFriendRecommendations", &TwitchRequestBus::Events::GetFriendRecommendations)
                ->Event("GetFriends", &TwitchRequestBus::Events::GetFriends)
                ->Event("GetFriendStatus", &TwitchRequestBus::Events::GetFriendStatus)
                ->Event("AcceptFriendRequest", &TwitchRequestBus::Events::AcceptFriendRequest)
                ->Event("GetFriendRequests", &TwitchRequestBus::Events::GetFriendRequests)
                ->Event("CreateFriendRequest", &TwitchRequestBus::Events::CreateFriendRequest)
                ->Event("DeclineFriendRequest", &TwitchRequestBus::Events::DeclineFriendRequest)
                ->Event("UpdatePresenceStatus", &TwitchRequestBus::Events::UpdatePresenceStatus)
                ->Event("GetPresenceStatusofFriends", &TwitchRequestBus::Events::GetPresenceStatusofFriends)
                ->Event("GetPresenceSettings", &TwitchRequestBus::Events::GetPresenceSettings)
                ->Event("UpdatePresenceSettings", &TwitchRequestBus::Events::UpdatePresenceSettings)
                ->Event("GetChannel", &TwitchRequestBus::Events::GetChannel)
                ->Event("GetChannelbyID", &TwitchRequestBus::Events::GetChannelbyID)
                ->Event("UpdateChannel", &TwitchRequestBus::Events::UpdateChannel)
                ->Event("GetChannelEditors", &TwitchRequestBus::Events::GetChannelEditors)
                ->Event("GetChannelFollowers", &TwitchRequestBus::Events::GetChannelFollowers)
                ->Event("GetChannelTeams", &TwitchRequestBus::Events::GetChannelTeams)
                ->Event("GetChannelSubscribers", &TwitchRequestBus::Events::GetChannelSubscribers)
                ->Event("CheckChannelSubscriptionbyUser", &TwitchRequestBus::Events::CheckChannelSubscriptionbyUser)
                ->Event("GetChannelVideos", &TwitchRequestBus::Events::GetChannelVideos)
                ->Event("StartChannelCommercial", &TwitchRequestBus::Events::StartChannelCommercial)
                ->Event("ResetChannelStreamKey", &TwitchRequestBus::Events::ResetChannelStreamKey)
                ;

            context.EBus<TwitchNotifyBus>("TwitchNotifyBus")
                ->Handler<BehaviorTwitchNotifyBus>();
        }
    }
}
