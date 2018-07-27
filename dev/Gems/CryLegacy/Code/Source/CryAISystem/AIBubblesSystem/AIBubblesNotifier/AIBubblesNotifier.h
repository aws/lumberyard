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

// Description : Central class to create and display speech bubbles over the
//               AI agents to notify messages to the designers


#ifndef CRYINCLUDE_CRYAISYSTEM_AIBUBBLESSYSTEM_AIBUBBLESNOTIFIER_AIBUBBLESNOTIFIER_H
#define CRYINCLUDE_CRYAISYSTEM_AIBUBBLESSYSTEM_AIBUBBLESNOTIFIER_AIBUBBLESNOTIFIER_H
#pragma once

#ifdef CRYAISYSTEM_DEBUG

#include "../IAIBubblesSystem.h"

class CAIBubblesNotifier
    : public IAIBubblesNotifier
{
    DECLARE_TYPE(CAIBubblesNotifier, IAIBubblesNotifier);
public:
    CAIBubblesNotifier();
    virtual ~CAIBubblesNotifier();

    // IAIBubblesNotifier methods
    virtual void Init();

    virtual void QueueMessage(const char* messageName, const SAIBubbleRequest& request);
    virtual void Update();

    virtual void Reset();
    // ~IAIBubblesNotifier methods

private:

    // SAIBubbleRequestContainer needs only to be known internally in the
    // BubblesNotifier
    struct SAIBubbleRequestContainer
    {
        SAIBubbleRequestContainer(const uint32 _requestId, const SAIBubbleRequest& _request)
            : requestId(_requestId)
            , expiringTime(.0f)
            , request(_request)
            , startExpiringTime(true)
        {
        }

        const SAIBubbleRequest& GetRequest()
        {
            if (startExpiringTime)
            {
                UpdateDuration(request.GetDuration());
                startExpiringTime = false;
            }
            return request;
        }
        uint32 GetRequestId() const { return requestId; }

        bool IsOld (const CTimeValue currentTimestamp) const
        {
            return currentTimestamp > expiringTime;
        }

    private:
        void UpdateDuration(const float duration)
        {
            expiringTime = gEnv->pTimer->GetFrameStartTime() + (duration ? CTimeValue(duration) : CTimeValue(gAIEnv.CVars.BubblesSystemDecayTime));
        }

        uint32           requestId;
        CTimeValue       expiringTime;
        bool             startExpiringTime;
        SAIBubbleRequest request;
    };

    struct SRequestFinder
    {
        SRequestFinder(uint32 _requestIndex)
            : requestIndex(_requestIndex) {}
        bool operator()(const SAIBubbleRequestContainer& container) const {return container.GetRequestId() == requestIndex; }
        unsigned requestIndex;
    };


    bool DisplaySpeechBubble(const SAIBubbleRequest& request) const;
    // Drawing functions
    void DrawBubble(const char* const message, const Vec3& pos, const size_t numberOfLines) const;

    // Information logging/popping up
    void LogMessage(const char* const message, const TBubbleRequestOptionFlags flags) const;
    void PopupBlockingAlert(const char* const message, const TBubbleRequestOptionFlags flags) const;

    bool ShouldSuppressMessageVisibility(const SAIBubbleRequest::ERequestType requestType = SAIBubbleRequest::eRT_ErrorMessage) const;

    typedef std::list<SAIBubbleRequestContainer> RequestsList;
    typedef std__hash_map<EntityId, RequestsList> EntityRequestsMap;
    EntityRequestsMap   SOFT(m_entityRequestsMap);
};

#endif // CRYAISYSTEM_DEBUG

#endif // CRYINCLUDE_CRYAISYSTEM_AIBUBBLESSYSTEM_AIBUBBLESNOTIFIER_AIBUBBLESNOTIFIER_H
