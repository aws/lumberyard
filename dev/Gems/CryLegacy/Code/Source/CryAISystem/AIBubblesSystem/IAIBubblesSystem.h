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

// Description : Bubbles Notifier interface


#ifndef CRYINCLUDE_CRYAISYSTEM_AIBUBBLESSYSTEM_IAIBUBBLESSYSTEM_H
#define CRYINCLUDE_CRYAISYSTEM_AIBUBBLESSYSTEM_IAIBUBBLESSYSTEM_H
#pragma once

#define MAX_BUBBLE_MESSAGE_LENGHT 512
#define MIN_CHARACTERS_INTERVAL 30

#include "TypeLibrary.h"
#include <IEntity.h>

typedef uint32 TBubbleRequestOptionFlags;

struct SAIBubbleRequest
{
    enum ERequestType
    {
        eRT_ErrorMessage = 0,
        eRT_PrototypeDialog,
    };

    SAIBubbleRequest(const EntityId _entityID, const char* _message, TBubbleRequestOptionFlags _flags, float _duration = 0.0f, const ERequestType _requestType = eRT_ErrorMessage)
        : entityID(_entityID)
        , message(_message)
        , flags(_flags)
        , duration(_duration)
        , requestType(_requestType)
        , totalLinesFormattedMessage(0)
    {
        FormatMessageWithMultiline(MIN_CHARACTERS_INTERVAL);
    }

    SAIBubbleRequest(const SAIBubbleRequest& request)
        : entityID(request.entityID)
        , message(request.message)
        , flags(request.flags)
        , duration(request.duration)
        , requestType(request.requestType)
        , totalLinesFormattedMessage(request.totalLinesFormattedMessage)
    {}

    size_t GetFormattedMessage(stack_string& outMessage) const { outMessage = message; return totalLinesFormattedMessage; }
    void GetMessage(stack_string& outMessage) const
    {
        outMessage = message;
        std::replace(outMessage.begin(), outMessage.end(), '\n', ' ');
    }

    const EntityId GetEntityID() const { return entityID; }
    TBubbleRequestOptionFlags GetFlags() const { return flags; }
    float GetDuration() const { return duration; }
    ERequestType GetRequestType() const { return requestType; }

private:
    void FormatMessageWithMultiline(size_t minimumCharactersInterval)
    {
        size_t pos = message.find(' ', minimumCharactersInterval);
        size_t i = 1;
        while (pos != message.npos)
        {
            message.replace(pos, 1, "\n");
            pos = message.find(' ', minimumCharactersInterval * (++i));
        }
        totalLinesFormattedMessage = i;
    }

    EntityId                                    entityID;
    CryFixedStringT<MAX_BUBBLE_MESSAGE_LENGHT>  message;
    TBubbleRequestOptionFlags                   flags;
    float                                       duration;
    const ERequestType                          requestType;
    size_t                                      totalLinesFormattedMessage;
};

bool AIQueueBubbleMessage(const char* messageName, const EntityId entityID,
    const char* message, uint32 flags, float duration = 0, SAIBubbleRequest::ERequestType requestType = SAIBubbleRequest::eRT_ErrorMessage);

enum EBubbleRequestOption
{
    eBNS_None           = 0,
    eBNS_Log            = BIT(0),
    eBNS_Balloon        = BIT(1),
    eBNS_BlockingPopup  = BIT(2),
    eBNS_LogWarning     = BIT(3),
};

/* Bubble System, manager for the Soft Code use */

struct IAIBubblesSystem
{
    virtual ~IAIBubblesSystem() {};

    virtual void QueueMessage(const char* messageName, const SAIBubbleRequest& request) = 0;
    virtual void Update() = 0;

    virtual void Reset() = 0;
    virtual void Init() = 0;
    virtual void Dispose() = 0;
};

/* This is the interface for the actual bubble notifier */

struct IAIBubblesNotifier
{
    DECLARE_TYPELIB(IAIBubblesNotifier);
    virtual ~IAIBubblesNotifier() {}

    virtual void Init() = 0;
    /*
        messageName is an identifier for the part of the code where the
        QueueMessage is called to avoid queueing the same message
        several time before the previous message is actually expired
        inside the BubbleSystem
    */
    virtual void QueueMessage(const char* messageName, const SAIBubbleRequest& request) = 0;
    virtual void Update() = 0;

    virtual void Reset() = 0;
};

#endif // CRYINCLUDE_CRYAISYSTEM_AIBUBBLESSYSTEM_IAIBUBBLESSYSTEM_H
