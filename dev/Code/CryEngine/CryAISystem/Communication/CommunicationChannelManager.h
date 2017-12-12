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
// Original file Copyright Crytek GMBH or its affiliates, used under license.

#ifndef CRYINCLUDE_CRYAISYSTEM_COMMUNICATION_COMMUNICATIONCHANNELMANAGER_H
#define CRYINCLUDE_CRYAISYSTEM_COMMUNICATION_COMMUNICATIONCHANNELMANAGER_H
#pragma once


#include "Communication.h"
#include "CommunicationChannel.h"


class CommunicationChannelManager
{
public:
    bool LoadChannel(const XmlNodeRef& channelNode, const CommChannelID& parentID);

    void Clear();
    void Reset();
    void Update(float updateTime);

    CommChannelID GetChannelID(const char* name) const;
    const SCommunicationChannelParams& GetChannelParams(const CommChannelID& channelID) const;
    CommunicationChannel::Ptr GetChannel(const CommChannelID& channelID, EntityId sourceId) const;
    CommunicationChannel::Ptr GetChannel(const CommChannelID& channelID, EntityId sourceId);

private:
    typedef std::map<CommChannelID, SCommunicationChannelParams> ChannelParams;
    ChannelParams m_params;


    typedef std::map<CommChannelID, CommunicationChannel::Ptr> Channels;
    Channels m_globalChannels;

    typedef std::map<int, Channels> GroupChannels;
    GroupChannels m_groupChannels;

    typedef std::map<EntityId, Channels> PersonalChannels;
    PersonalChannels m_personalChannels;
};

#endif // CRYINCLUDE_CRYAISYSTEM_COMMUNICATION_COMMUNICATIONCHANNELMANAGER_H
