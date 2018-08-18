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

#include "CryLegacy_precompiled.h"
#include "AIBubblesNotifier.h"
#include "IRenderer.h"
#include "DebugDrawContext.h"
#include "IRenderAuxGeom.h"
#include "AILog.h"
#include "IGame.h"
#include "IGameFramework.h"

#ifdef CRYAISYSTEM_DEBUG

IMPLEMENT_TYPE(CAIBubblesNotifier);

CAIBubblesNotifier::CAIBubblesNotifier()
{
}

////////////////////////////////////////////////////////////////////
CAIBubblesNotifier::~CAIBubblesNotifier()
{
    Reset();
}

////////////////////////////////////////////////////////////////////
void CAIBubblesNotifier::Init()
{
}

////////////////////////////////////////////////////////////////////
void CAIBubblesNotifier::QueueMessage(const char* messageName, const SAIBubbleRequest& request)
{
    const   TBubbleRequestOptionFlags requestFlags = request.GetFlags();
    const EntityId entityID = request.GetEntityID();
    const IEntity* pEntity = gEnv->pEntitySystem->GetEntity(entityID);
    if (!pEntity)
    {
        LogMessage("The AIBubblesSystem received a message for a non existing entity."
            " The message will be discarded.", requestFlags);
        return;
    }

    RequestsList& requestsList = m_entityRequestsMap[request.GetEntityID()];
    uint32 uniqueId = CCrc32::ComputeLowercase(messageName);
    RequestsList::iterator requestContainerIt = std::find_if(requestsList.begin(), requestsList.end(), SRequestFinder(uniqueId));

    if (request.GetRequestType() == SAIBubbleRequest::eRT_PrototypeDialog && requestContainerIt != requestsList.end())
    {
        requestsList.erase(requestContainerIt);
        requestContainerIt = requestsList.end();
    }

    if (requestContainerIt == requestsList.end())
    {
        if (request.GetRequestType() == SAIBubbleRequest::eRT_PrototypeDialog)
        {
            requestsList.push_back(SAIBubbleRequestContainer(uniqueId, request));
        }
        else
        {
            requestsList.push_front(SAIBubbleRequestContainer(uniqueId, request));
        }

        // Log the message and if it's needed show the blocking popup
        stack_string sMessage;
        request.GetMessage(sMessage);
        sMessage.Format("%s - Message: %s", pEntity->GetName(), sMessage.c_str());

        LogMessage(sMessage.c_str(), requestFlags);

        PopupBlockingAlert(sMessage.c_str(), requestFlags);
    }
}

////////////////////////////////////////////////////////////////////
void CAIBubblesNotifier::Update()
{
    if (!m_entityRequestsMap.empty())
    {
        const CTimeValue currentTimestamp = gEnv->pTimer->GetFrameStartTime();
        EntityRequestsMap::iterator it = m_entityRequestsMap.begin();
        EntityRequestsMap::iterator end = m_entityRequestsMap.end();
        for (; it != end; ++it)
        {
            RequestsList& requestsList = it->second;
            if (!requestsList.empty())
            {
                SAIBubbleRequestContainer& requestContainer = requestsList.back();
                bool result = DisplaySpeechBubble(requestContainer.GetRequest());
                if (requestContainer.IsOld(currentTimestamp) || !result)
                {
                    requestsList.pop_back();
                }
            }
        }
    }
}

////////////////////////////////////////////////////////////////////
bool CAIBubblesNotifier::DisplaySpeechBubble(const SAIBubbleRequest& request) const
{
    /* This function returns false if the entity connected to the message stop
        to exist. In this case the corresponding message will be removed from the queue */

    if (ShouldSuppressMessageVisibility(request.GetRequestType()))
    {
        // We only want to suppress the visibility of the message
        // but we don't want to remove it from the queue
        return true;
    }

    const TBubbleRequestOptionFlags requestFlags = request.GetFlags();

    if (requestFlags & eBNS_Balloon && gAIEnv.CVars.BubblesSystemAlertnessFilter & eBNS_Balloon)
    {
        const EntityId entityID = request.GetEntityID();
        const IEntity* pEntity = gEnv->pEntitySystem->GetEntity(entityID);
        if (!pEntity)
        {
            return false;
        }

        Vec3 pos = pEntity->GetPos();
        AABB aabb;
        pEntity->GetLocalBounds(aabb);
        pos.z = pos.z + aabb.max.z + 0.5f;

        stack_string sMessage;
        request.GetMessage(sMessage);
        sMessage.Format("%s - Pos:(%f %f %f) - Message: %s", pEntity->GetName(), pos.x, pos.y, pos.z, sMessage.c_str());

        stack_string sFormattedMessage;
        const size_t numberOfLines = request.GetFormattedMessage(sFormattedMessage);
        DrawBubble(sFormattedMessage.c_str(), pos, numberOfLines);
    }

    return true;
}


////////////////////////////////////////////////////////////////////
void CAIBubblesNotifier::LogMessage(const char* const message, const TBubbleRequestOptionFlags flags) const
{
    if (gAIEnv.CVars.BubblesSystemAlertnessFilter & eBNS_Log)
    {
        if (flags & eBNS_LogWarning)
        {
            AIWarning(message);
        }
        else if (flags & eBNS_Log)
        {
            gEnv->pLog->Log(message);
        }
    }
}

////////////////////////////////////////////////////////////////////
void CAIBubblesNotifier::PopupBlockingAlert(const char* const message, const TBubbleRequestOptionFlags flags) const
{
    if (ShouldSuppressMessageVisibility())
    {
        return;
    }

    if (flags & eBNS_BlockingPopup && gAIEnv.CVars.BubblesSystemAlertnessFilter & eBNS_BlockingPopup)
    {
        CryMessageBox(message, "AIBubbleSystemMessageBox", 0);
    }
}

////////////////////////////////////////////////////////////////////
void CAIBubblesNotifier::DrawBubble(const char* const message, const Vec3& pos, const size_t numberOfLines) const
{
    const float distance  = gEnv->pSystem->GetViewCamera().GetPosition().GetDistance(pos);
    const float currentTextSize = gAIEnv.CVars.BubblesSystemFontSize / distance;
    const Vec3 drawingPosition = pos + Vec3(.0f, .0f, currentTextSize * (numberOfLines - 1));
    CDebugDrawContext dc;
    dc->Draw3dLabelEx(drawingPosition, currentTextSize, ColorB(0, 0, 0), true, true,
        (gAIEnv.CVars.BubblesSystemUseDepthTest) ? true : false, true, message);
}

////////////////////////////////////////////////////////////////////
void CAIBubblesNotifier::Reset()
{
    stl::free_container(m_entityRequestsMap);
}

////////////////////////////////////////////////////////////////////
bool CAIBubblesNotifier::ShouldSuppressMessageVisibility(const SAIBubbleRequest::ERequestType requestType) const
{
    if (gAIEnv.CVars.EnableBubblesSystem != 1)
    {
        return true;
    }

    switch (requestType)
    {
    case SAIBubbleRequest::eRT_ErrorMessage:
        return gAIEnv.CVars.DebugDraw < 1;
    case SAIBubbleRequest::eRT_PrototypeDialog:
        return gAIEnv.CVars.BubbleSystemAllowPrototypeDialogBubbles != 1;
    default:
        assert(0);
        return gAIEnv.CVars.DebugDraw > 0;
    }
}

#endif // CRYAISYSTEM_DEBUG
