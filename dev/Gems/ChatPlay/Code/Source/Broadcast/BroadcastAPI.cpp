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
#include "ChatPlay_precompiled.h"
#include "BroadcastAPI.h"
#include "BroadcastCVars.h"

#include <aws/core/utils/json/JsonSerializer.h>
#include <HttpRequestor/HttpRequestorBus.h>

namespace ChatPlay
{

    /******************************************************************************/
    // Twitch-Specific Implementation of IBroadcast
    //

    class TwitchAPI
        : public IBroadcast
    {
    public:
        explicit TwitchAPI();

        void GetBoolValue(const ChannelId& channelID, ApiKey type, const BoolCallback& userCallback) override;
        void GetIntValue(const ChannelId& channelID, ApiKey type, const IntCallback& userCallback) override;
        void GetFloatValue(const ChannelId& channelID, ApiKey type, const FloatCallback& userCallback) override;
        void GetStringValue(const ChannelId& channelID, ApiKey type, const StringCallback& userCallback) override;

        size_t DispatchEvents() override;

        const char* GetFlowNodeString() override;

    private:
        typedef std::tuple<std::string, std::string> UrlKeyPair;
        typedef AZStd::function<void()> BroadcastEvent;

        template <typename T>
        void GetValue(const ChannelId& channelID, ApiKey type,
            const AZStd::function<void(ApiCallResult, Aws::Http::HttpResponseCode, T)>& userCallback
            );

        template <typename T>
        void JSONParse(Aws::Http::HttpResponseCode httpResponse, const Aws::Utils::Json::JsonValue& jsonValue, const std::string& pathToKey,
            const AZStd::function<void(ApiCallResult, Aws::Http::HttpResponseCode, T)>& userCallback
            );

        std::string MakeTwitchURL(const ChannelId& channelID, ApiKey type);

        Aws::Utils::Json::JsonValue getJSONValue(const std::string& pathToKey, const Aws::Utils::Json::JsonValue& jsonValue);

        void RegisterEvent(const BroadcastEvent& event);

        void PrepareUrlMaps();
        void PrepareFlowNodeString();

        // Members:
        std::shared_ptr<BroadcastCVars> m_vars;
        std::string m_flowString;

        std::map<ApiKey, UrlKeyPair> m_urlMap;

        AZStd::vector<BroadcastEvent> m_events;
        CryCriticalSectionNonRecursive m_eventLock;
    };

    namespace
    {
        // Templates and specializations
        template <typename T>
        bool JSONTypeCheck(const Aws::Utils::Json::JsonValue&); // unimplemented method

        template <>
        bool JSONTypeCheck<bool>(const Aws::Utils::Json::JsonValue& jsonValue) { return jsonValue.IsBool(); }
        template <>
        bool JSONTypeCheck<int>(const Aws::Utils::Json::JsonValue& jsonValue) { return jsonValue.IsIntegerType(); }
        template <>
        bool JSONTypeCheck<float>(const Aws::Utils::Json::JsonValue& jsonValue) { return jsonValue.IsFloatingPointType(); }
        template <>
        bool JSONTypeCheck<std::string>(const Aws::Utils::Json::JsonValue& jsonValue) { return !jsonValue.IsListType() && !jsonValue.IsObject(); }

        template <typename T>
        T JSONGetValue(const Aws::Utils::Json::JsonValue&); // unimplemented method

        template <>
        bool JSONGetValue<bool>(const Aws::Utils::Json::JsonValue& jsonValue) { return jsonValue.AsBool(); }
        template <>
        int JSONGetValue<int>(const Aws::Utils::Json::JsonValue& jsonValue) { return jsonValue.AsInteger(); }
        template <>
        float JSONGetValue<float>(const Aws::Utils::Json::JsonValue& jsonValue) { return static_cast<float>(jsonValue.AsDouble()); }
        template <>
        std::string JSONGetValue<std::string>(const Aws::Utils::Json::JsonValue& jsonValue) { return std::string(jsonValue.AsString().c_str()); }
    }

    IBroadcastPtr CreateBroadcastAPI()
    {
        return IBroadcastPtr(new TwitchAPI());
    }

    TwitchAPI::TwitchAPI()
    {
        m_vars = BroadcastCVars::GetInstance();
        CRY_ASSERT(m_vars); // This is unexpected, GetInstance should always work; we assert since exceptions are disabled ...

        PrepareUrlMaps();
        PrepareFlowNodeString();
    }

    void TwitchAPI::GetBoolValue(const ChannelId& channelID, ApiKey type, const BoolCallback& userCallback)
    {
        GetValue<bool>(channelID, type, userCallback);
    }

    void TwitchAPI::GetIntValue(const ChannelId& channelID, ApiKey type, const IntCallback& userCallback)
    {
        GetValue<int>(channelID, type, userCallback);
    }

    void TwitchAPI::GetFloatValue(const ChannelId& channelID, ApiKey type, const FloatCallback& userCallback)
    {
        GetValue<float>(channelID, type, userCallback);
    }

    void TwitchAPI::GetStringValue(const ChannelId& channelID, ApiKey type, const StringCallback& userCallback)
    {
        GetValue<std::string>(channelID, type, userCallback);
    }

    // Returns the number of events fired
    size_t TwitchAPI::DispatchEvents()
    {
        /* DISPATCH EVENT THREAD */

        AZStd::vector<BroadcastEvent> events;

        {
            CryAutoLock<CryCriticalSectionNonRecursive> lock(m_eventLock);
            AZStd::swap(m_events, events);
        }

        for (auto event : events)
        {
            event();
        }

        return events.size();
    }

    template <typename T>
    void TwitchAPI::GetValue(const ChannelId& channelID, ApiKey type, const AZStd::function<void(ApiCallResult, Aws::Http::HttpResponseCode, T)>& userCallback)
    {
        std::string requestUrl = MakeTwitchURL(channelID, type);
        std::string pathToKey = std::get<1>(m_urlMap[type]);

        AZStd::string uri = requestUrl.c_str();
        HttpRequestor::Headers headers;
        headers["Client-ID"] = m_vars->GetClientID();

        HttpRequestor::Callback cb = AZStd::bind(&TwitchAPI::JSONParse<T>, this, AZStd::placeholders::_2, AZStd::placeholders::_1, pathToKey, userCallback);

        EBUS_EVENT(HttpRequestor::HttpRequestorRequestBus, AddRequestWithHeaders, uri, Aws::Http::HttpMethod::HTTP_GET, headers, cb);
    }

    template <typename T>
    void TwitchAPI::JSONParse(Aws::Http::HttpResponseCode httpResponse, const Aws::Utils::Json::JsonValue& jsonValue, const std::string& pathToKey, const AZStd::function<void(ApiCallResult, Aws::Http::HttpResponseCode, T)>& userCallback)
    {
        /* HTTP MANAGER THREAD */

        if (static_cast<Aws::Http::HttpResponseCode>(httpResponse) == Aws::Http::HttpResponseCode::OK)
        {
            Aws::Utils::Json::JsonValue leafJsonValue = getJSONValue(pathToKey, std::move(jsonValue));

            // if returned JsonValue is non-empty
            if (leafJsonValue != Aws::Utils::Json::JsonValue())
            {
                if (JSONTypeCheck<T>(leafJsonValue))
                {
                    RegisterEvent(AZStd::bind(userCallback, ApiCallResult::SUCCESS, httpResponse, JSONGetValue<T>(leafJsonValue)));
                    return;
                }
                else
                {
                    RegisterEvent(AZStd::bind(userCallback, ApiCallResult::ERROR_UNEXPECTED_TYPE, httpResponse, T()));
                    return;
                }
            }
            else
            {
                RegisterEvent(AZStd::bind(userCallback, ApiCallResult::ERROR_NULL_OBJECT, httpResponse, T()));
                return;
            }
        }

        // GET request failed, queue up the callback with the error code
        RegisterEvent(AZStd::bind(userCallback, ApiCallResult::ERROR_HTTP_REQUEST_FAILED, httpResponse, T()));
    }

    // Helper function to check if a key in the form "node.node.leaf" exists:
    // * Returns the JsonValue object of the leaf if the keypath exists
    // * Returns an empty JsonValue object otherwise
    Aws::Utils::Json::JsonValue TwitchAPI::getJSONValue(const std::string& pathToKey, const Aws::Utils::Json::JsonValue& jsonValue)
    {
        string subKeyLeft;
        string subKeyRight(pathToKey.c_str());
        int delimiterPos;
        Aws::Utils::Json::JsonValue jsonNodeValue = jsonValue;

        do
        {
            delimiterPos = subKeyRight.find(".");
            subKeyLeft = subKeyRight.substr(0, delimiterPos);
            subKeyRight = subKeyRight.substr(delimiterPos + 1, std::string::npos);

            if (jsonNodeValue.ValueExists(subKeyLeft))
            {
                jsonNodeValue = jsonNodeValue.GetObject(subKeyLeft);
            }
            else
            {
                return Aws::Utils::Json::JsonValue();
            }
        } while (delimiterPos != std::string::npos);

        return jsonNodeValue;
    }

    void TwitchAPI::RegisterEvent(const BroadcastEvent& event)
    {
        /* ANY THREAD? */
        CryAutoLock<CryCriticalSectionNonRecursive> lock(m_eventLock);
        m_events.push_back(event);
    }

    std::string TwitchAPI::MakeTwitchURL(const ChannelId& channelID, ApiKey type)
    {
        // Construct the URL
        std::string twitchUrl = m_vars->GetBroadcastEndpoint() + std::get<0>(m_urlMap[type]) + channelID;
        return twitchUrl;
    }

    void TwitchAPI::PrepareUrlMaps()
    {
        // Bool
        m_urlMap[ApiKey::CHANNEL_MATURE] = UrlKeyPair("channels/", "mature");
        m_urlMap[ApiKey::CHANNEL_PARTNER] = UrlKeyPair("channels/", "partner");

        // Int
        m_urlMap[ApiKey::CHANNEL_DELAY] = UrlKeyPair("channels/", "delay");
        m_urlMap[ApiKey::CHANNEL_VIEWS] = UrlKeyPair("channels/", "views");
        m_urlMap[ApiKey::CHANNEL_FOLLOWERS] = UrlKeyPair("channels/", "followers");
        m_urlMap[ApiKey::STREAM_VIEWERS] = UrlKeyPair("streams/", "stream.viewers");
        m_urlMap[ApiKey::STREAM_VIDEO_HEIGHT] = UrlKeyPair("streams/", "stream.video_height");

        // Float
        m_urlMap[ApiKey::STREAM_AVERAGE_FPS] = UrlKeyPair("streams/", "stream.average_fps");

        // String
        m_urlMap[ApiKey::CHANNEL_STATUS] = UrlKeyPair("channels/", "status");
        m_urlMap[ApiKey::CHANNEL_BROADCASTER_LANGUAGE] = UrlKeyPair("channels/", "broadcaster_language");
        m_urlMap[ApiKey::CHANNEL_DISPLAY_NAME] = UrlKeyPair("channels/", "display_name");
        m_urlMap[ApiKey::CHANNEL_GAME] = UrlKeyPair("channels/", "game");
        m_urlMap[ApiKey::CHANNEL_LANGUAGE] = UrlKeyPair("channels/", "language");
        m_urlMap[ApiKey::CHANNEL_NAME] = UrlKeyPair("channels/", "name");
        m_urlMap[ApiKey::CHANNEL_CREATED_AT] = UrlKeyPair("channels/", "created_at");
        m_urlMap[ApiKey::CHANNEL_UPDATED_AT] = UrlKeyPair("channels/", "updated_at");
        m_urlMap[ApiKey::CHANNEL_URL] = UrlKeyPair("channels/", "url");
        m_urlMap[ApiKey::STREAM_GAME] = UrlKeyPair("streams/", "stream.game");
        m_urlMap[ApiKey::STREAM_CREATED_AT] = UrlKeyPair("streams/", "stream.created_at");
        m_urlMap[ApiKey::USER_TYPE] = UrlKeyPair("users/", "type");
        m_urlMap[ApiKey::USER_NAME] = UrlKeyPair("users/", "name");
        m_urlMap[ApiKey::USER_CREATED_AT] = UrlKeyPair("users/", "created_at");
        m_urlMap[ApiKey::USER_UPDATED_AT] = UrlKeyPair("users/", "updated_at");
        m_urlMap[ApiKey::USER_LOGO] = UrlKeyPair("users/", "logo");
        m_urlMap[ApiKey::USER_DISPLAY_NAME] = UrlKeyPair("users/", "display_name");
        m_urlMap[ApiKey::USER_BIO] = UrlKeyPair("users/", "bio");
        m_urlMap[ApiKey::CHANNEL_ID] = UrlKeyPair("channels/", "_id");
        m_urlMap[ApiKey::STREAM_ID] = UrlKeyPair("streams/", "stream._id");
        m_urlMap[ApiKey::USER_ID] = UrlKeyPair("users/", "_id");
    }

    void TwitchAPI::PrepareFlowNodeString()
    {
        std::stringstream stringBuilder;
        stringBuilder << "enum_int:";
        stringBuilder << "ChannelMature=" << static_cast<int>(ApiKey::CHANNEL_MATURE);
        stringBuilder << ",ChannelPartner=" << static_cast<int>(ApiKey::CHANNEL_PARTNER);
        stringBuilder << ",ChannelDelay=" << static_cast<int>(ApiKey::CHANNEL_DELAY);
        stringBuilder << ",ChannelId=" << static_cast<int>(ApiKey::CHANNEL_ID);
        stringBuilder << ",ChannelViews=" << static_cast<int>(ApiKey::CHANNEL_VIEWS);
        stringBuilder << ",ChannelFollowers=" << static_cast<int>(ApiKey::CHANNEL_FOLLOWERS);
        stringBuilder << ",StreamViewers=" << static_cast<int>(ApiKey::STREAM_VIEWERS);
        stringBuilder << ",StreamVideoHeight=" << static_cast<int>(ApiKey::STREAM_VIDEO_HEIGHT);
        stringBuilder << ",StreamId=" << static_cast<int>(ApiKey::STREAM_ID);
        stringBuilder << ",UserId=" << static_cast<int>(ApiKey::USER_ID);
        stringBuilder << ",StreamAverageFPS=" << static_cast<int>(ApiKey::STREAM_AVERAGE_FPS);
        stringBuilder << ",ChannelStatus=" << static_cast<int>(ApiKey::CHANNEL_STATUS);
        stringBuilder << ",ChannelBroadcasterLanguage=" << static_cast<int>(ApiKey::CHANNEL_BROADCASTER_LANGUAGE);
        stringBuilder << ",ChannelDisplayName=" << static_cast<int>(ApiKey::CHANNEL_DISPLAY_NAME);
        stringBuilder << ",ChannelGame=" << static_cast<int>(ApiKey::CHANNEL_GAME);
        stringBuilder << ",ChannelLanguage=" << static_cast<int>(ApiKey::CHANNEL_LANGUAGE);
        stringBuilder << ",ChannelName=" << static_cast<int>(ApiKey::CHANNEL_NAME);
        stringBuilder << ",ChannelCreatedAt=" << static_cast<int>(ApiKey::CHANNEL_CREATED_AT);
        stringBuilder << ",ChannelUpdatedAt=" << static_cast<int>(ApiKey::CHANNEL_UPDATED_AT);
        stringBuilder << ",ChannelURL=" << static_cast<int>(ApiKey::CHANNEL_URL);
        stringBuilder << ",StreamGame=" << static_cast<int>(ApiKey::STREAM_GAME);
        stringBuilder << ",StreamCreatedAt=" << static_cast<int>(ApiKey::STREAM_CREATED_AT);
        stringBuilder << ",UserType=" << static_cast<int>(ApiKey::USER_TYPE);
        stringBuilder << ",UserName=" << static_cast<int>(ApiKey::USER_NAME);
        stringBuilder << ",UserCreatedAt=" << static_cast<int>(ApiKey::USER_CREATED_AT);
        stringBuilder << ",UserUpdatedAt=" << static_cast<int>(ApiKey::USER_UPDATED_AT);
        stringBuilder << ",UserLogo=" << static_cast<int>(ApiKey::USER_LOGO);
        stringBuilder << ",UserDisplayName=" << static_cast<int>(ApiKey::USER_DISPLAY_NAME);
        stringBuilder << ",UserBio=" << static_cast<int>(ApiKey::USER_BIO);

        m_flowString = stringBuilder.str();
    }

    const char* TwitchAPI::GetFlowNodeString()
    {
        return m_flowString.c_str();
    }

}
