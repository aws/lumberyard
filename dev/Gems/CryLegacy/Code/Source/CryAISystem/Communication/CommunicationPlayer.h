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

#ifndef CRYINCLUDE_CRYAISYSTEM_COMMUNICATION_COMMUNICATIONPLAYER_H
#define CRYINCLUDE_CRYAISYSTEM_COMMUNICATION_COMMUNICATIONPLAYER_H
#pragma once


#include "Communication.h"


class CommunicationPlayer
    : public IAICommunicationHandler::IEventListener
{
public:
    struct ICommunicationFinishedListener
    {
        /// playID is the id of the communication to stop.
        /// StateFlags contains  whether the components of the communication are finished or not (animation, sound, etc).
        virtual void OnCommunicationFinished(const CommPlayID& playID, uint8 stateFlags) = 0;
        virtual ~ICommunicationFinishedListener(){}
    };

public:
    void Clear();
    void Reset();

    bool Play(const CommPlayID& playID, const SCommunicationRequest& request,
        const SCommunication& comm, uint32 variationIdx, ICommunicationFinishedListener* listener, void* param);
    void Stop(const CommPlayID& playID);
    void Update(float updateTime);

    bool IsPlaying(const CommPlayID& playID, float* remainingTime = 0) const;

    virtual void OnCommunicationHandlerEvent(
        IAICommunicationHandler::ECommunicationHandlerEvent type, CommPlayID playID, EntityId actorID);

    CommID GetCommunicationID(const CommPlayID& playID) const;

public:
    struct PlayState
    {
        enum EFinishedFlags
        {
            FinishedAnimation   = 1 << 0,
            FinishedSound           = 1 << 1,
            FinishedVoice           = 1 << 2,
            FinishedTimeout     = 1 << 3,
            FinishedAll             = FinishedAnimation | FinishedSound | FinishedVoice | FinishedTimeout,
        };

        PlayState(const SCommunicationRequest& request, const SCommunicationVariation& variation,
            ICommunicationFinishedListener* _listener = 0, void* _param = 0)
            : actorID(request.actorID)
            , commID(request.commID)
            , timeout(variation.timeout)
            , target(request.target)
            , targetID(request.targetID)
            , flags(variation.flags)
            , finishedFlags(0)
            , listener(_listener)
            , param(_param)
        {
        }
        ~PlayState(){}

        void OnCommunicationEvent(
            IAICommunicationHandler::ECommunicationHandlerEvent type, EntityId actorID);

        Vec3 target;

        string animationName;

        ICommunicationFinishedListener* listener;
        void* param;

        EntityId targetID;
        EntityId actorID;

        SCommunicationSound soundInfo;
        SCommunicationSound voiceInfo;

        CommID commID;

        float timeout;
        uint32 flags;

        //Tracks whether this playstate has finished.
        uint8 finishedFlags;
    };

private:

    void CleanUpPlayState(const CommPlayID& playID, PlayState& playState);

    bool UpdatePlaying(const CommPlayID& playId, PlayState& playing, float updateTime);
    void BlockingSettings(IAIObject* aiObject, PlayState& playing, bool set);

    typedef std::map<CommPlayID, PlayState> PlayingCommunications;
    PlayingCommunications m_playing;
};

#endif // CRYINCLUDE_CRYAISYSTEM_COMMUNICATION_COMMUNICATIONPLAYER_H
