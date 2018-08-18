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

// Description : This is the header file for the module Realtime remote
//               update  The purpose of this module is to allow data update to happen
//               remotely so that you can  for example  edit the terrain and see the changes
//               in the console


#ifndef CRYINCLUDE_CRYACTION_LIVEPREVIEW_REALTIMEREMOTEUPDATE_H
#define CRYINCLUDE_CRYACTION_LIVEPREVIEW_REALTIMEREMOTEUPDATE_H
#pragma once

#include "INotificationNetwork.h"
#include "IRealtimeRemoteUpdate.h"
#include "I3DEngine.h"

typedef std::vector<IRealtimeUpdateGameHandler*> GameHandlerList;

class CRealtimeRemoteUpdateListener
    : public INotificationNetworkListener
    , public IRealtimeRemoteUpdate
{
    //////////////////////////////////////////////////////////////////////////
    // Methods
public:
    // From IRealtimeRemoteUpdate
    virtual bool    Enable(bool boEnable = true);
    virtual bool    IsEnabled();

    // Game code handlers for live preview updates
    virtual void AddGameHandler(IRealtimeUpdateGameHandler* handler);
    virtual void RemoveGameHandler(IRealtimeUpdateGameHandler* handler);

    // From INotificationNetworkListener and from IRealtimeRemoteUpdate
    virtual void OnNotificationNetworkReceive(const void* pBuffer, size_t length);

    // Singleton management
    static CRealtimeRemoteUpdateListener&   GetRealtimeRemoteUpdateListener();

    virtual void Update();
protected:
    CRealtimeRemoteUpdateListener();
    virtual ~CRealtimeRemoteUpdateListener();

    void LoadArchetypes(XmlNodeRef& root);
    void LoadTimeOfDay(XmlNodeRef& root);
    void LoadMaterials(XmlNodeRef& root);
    void LoadConsoleVariables(XmlNodeRef& root);
    void LoadParticles(XmlNodeRef& root);
    void LoadTerrainLayer(XmlNodeRef& root, unsigned char*  uchData);
    void LoadEntities(XmlNodeRef& root);

    bool IsSyncingWithEditor();
    //////////////////////////////////////////////////////////////////////////
    // Data
protected:

    // As currently you can't query if the currently listener is registered
    // the control has to be done here.
    bool m_boIsEnabled;

    GameHandlerList m_gameHandlers;
    CTimeValue m_lastKeepAliveMessageTime;

    typedef std::vector<unsigned char> TDBuffer;

    CryMT::CLocklessPointerQueue<TDBuffer>  m_ProcessingQueue;
};

#endif // CRYINCLUDE_CRYACTION_LIVEPREVIEW_REALTIMEREMOTEUPDATE_H
