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

#ifndef CRYINCLUDE_CRYAISYSTEM_COMMUNICATION_COMMUNICATIONTESTMANAGER_H
#define CRYINCLUDE_CRYAISYSTEM_COMMUNICATION_COMMUNICATIONTESTMANAGER_H
#pragma once


#include "CommunicationPlayer.h"


class CommunicationTestManager
    : public CommunicationPlayer::ICommunicationFinishedListener
{
public:
    CommunicationTestManager();

    void Reset();

    void StartFor(EntityId actorID, const char* commName, int variationNumber = 0);
    void Stop(EntityId actorID);
    void Update(float updateTime);

private:
    // ICommunicationFinishedListener
    /// playID is the id of the communication to stop.
    /// StateFlags contains  whether the components of the communication are finished or not (animation, sound, etc).
    virtual void OnCommunicationFinished(const CommPlayID& playID, uint8 stateFlags);
    //~ICommunicationFinishedListener

    struct PlayingActor
    {
        PlayingActor()
            : configID(0)
            , commID(0)
            , playID(0)
            , variation(0)
            , failedCount(0)
            , totalCount(0)
            , onlyOne(false)
        {
        }

        CommConfigID configID;

        CommID commID;
        CommPlayID playID;
        uint32 variation;

        uint32 failedCount;
        uint32 totalCount;
        bool onlyOne;
    };

    void PlayNext(EntityId actorID);
    void Report(EntityId actorID, const PlayingActor& playingActor, const char* configName);

    CommunicationPlayer m_player;

    typedef std::map<tAIObjectID, PlayingActor> PlayingActors;
    PlayingActors m_playingActors;

    typedef std::map<CommPlayID, tAIObjectID> PlayingCommunications;
    PlayingCommunications m_playing;

    CommPlayID m_playGenID;
};


#endif // CRYINCLUDE_CRYAISYSTEM_COMMUNICATION_COMMUNICATIONTESTMANAGER_H
