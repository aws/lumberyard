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

// Description : Implementation of IAnimSequence interface.

#ifndef CRYINCLUDE_CRYMOVIE_ANIMSEQUENCE_H
#define CRYINCLUDE_CRYMOVIE_ANIMSEQUENCE_H

#pragma once

#include "IMovieSystem.h"

#include "TrackEventTrack.h"

class CAnimSequence
    : public IAnimSequence
{
public:
    CAnimSequence(IMovieSystem* pMovieSystem, uint32 id, ESequenceType = eSequenceType_Legacy);
    ~CAnimSequence();

    //////////////////////////////////////////////////////////////////////////
    virtual void Release()
    {
        if (--m_nRefCounter <= 0)
        {
            delete this;
        }
    }
    //////////////////////////////////////////////////////////////////////////

    // Movie system.
    IMovieSystem* GetMovieSystem() const { return m_pMovieSystem; };

    void SetName(const char* name);
    const char* GetName() const;
    uint32 GetId() const { return m_id; }

    float GetTime() const { return m_time; }

    float GetFixedTimeStep() const { return m_fixedTimeStep; }
    void SetFixedTimeStep(float dt) { m_fixedTimeStep = dt; }

    void SetOwner(IAnimSequenceOwner* pOwner) override { m_pOwner = pOwner; }
    virtual IAnimSequenceOwner* GetOwner() const override { return m_pOwner; }
    void SetOwner(const AZ::EntityId& entityOwnerId) override;
    const AZ::EntityId& GetOwnerId() const override { return m_ownerId; }

    virtual void SetActiveDirector(IAnimNode* pDirectorNode);
    virtual IAnimNode* GetActiveDirector() const;

    virtual void SetFlags(int flags);
    virtual int GetFlags() const;
    virtual int GetCutSceneFlags(const bool localFlags = false) const;

    virtual void SetParentSequence(IAnimSequence* pParentSequence);
    virtual const IAnimSequence* GetParentSequence() const;
    virtual bool IsAncestorOf(const IAnimSequence* pSequence) const;

    void SetTimeRange(Range timeRange);
    Range GetTimeRange() { return m_timeRange; };

    void AdjustKeysToTimeRange(const Range& timeRange);

    //! Return number of animation nodes in sequence.
    int GetNodeCount() const;
    //! Get specified animation node.
    IAnimNode* GetNode(int index) const;

    IAnimNode* FindNodeByName(const char* sNodeName, const IAnimNode* pParentDirector);
    IAnimNode* FindNodeById(int nNodeId);
    virtual void ReorderNode(IAnimNode* node, IAnimNode* pPivotNode, bool next);

    void Reset(bool bSeekToStart);
    void ResetHard();
    void Pause();
    void Resume();
    bool IsPaused() const;

    virtual void OnStart();
    virtual void OnStop();
    void OnLoop() override;

    void TimeChanged(float newTime) override;

    //! Add animation node to sequence.
    bool AddNode(IAnimNode* node);
    IAnimNode* CreateNode(EAnimNodeType nodeType);
    IAnimNode* CreateNode(XmlNodeRef node);
    void RemoveNode(IAnimNode* node, bool removeChildRelationships=true) override;
    //! Add scene node to sequence.
    void RemoveAll();

    virtual void Activate();
    virtual bool IsActivated() const { return m_bActive; }
    virtual void Deactivate();

    virtual void PrecacheData(float startTime);
    void PrecacheStatic(const float startTime);
    void PrecacheDynamic(float time);
    void PrecacheEntity(IEntity* pEntity);

    void StillUpdate();
    void Animate(const SAnimContext& ec);
    void Render();

    void Serialize(XmlNodeRef& xmlNode, bool bLoading, bool bLoadEmptyTracks = true, uint32 overrideId = 0, bool bResetLightAnimSet = false);
    void CopyNodes(XmlNodeRef& xmlNode, IAnimNode** pSelectedNodes, uint32 count);
    void PasteNodes(const XmlNodeRef& xmlNode, IAnimNode* pParent);

    //! Add/remove track events in sequence
    virtual bool AddTrackEvent(const char* szEvent);
    virtual bool RemoveTrackEvent(const char* szEvent);
    virtual bool RenameTrackEvent(const char* szEvent, const char* szNewEvent);
    virtual bool MoveUpTrackEvent(const char* szEvent);
    virtual bool MoveDownTrackEvent(const char* szEvent);
    virtual void ClearTrackEvents();

    //! Get the track events in the sequence
    virtual int GetTrackEventsCount() const;
    virtual char const* GetTrackEvent(int iIndex) const;
    virtual IAnimStringTable* GetTrackEventStringTable() { return m_pEventStrings; }

    //! Call to trigger a track event
    virtual void TriggerTrackEvent(const char* event, const char* param = NULL);

    //! Track event listener
    virtual void AddTrackEventListener(ITrackEventListener* pListener);
    virtual void RemoveTrackEventListener(ITrackEventListener* pListener);

    virtual void GetMemoryUsage(ICrySizer* pSizer) const
    {
        pSizer->AddObject(this, sizeof(*this));
        pSizer->AddObject(m_name);
        pSizer->AddObject(m_nodes);
        pSizer->AddObject(m_listeners);
    }

    ESequenceType GetSequenceType() const override { return m_sequenceType; }

private:
    void ComputeTimeRange();
    void CopyNodeChildren(XmlNodeRef& xmlNode, IAnimNode* pAnimNode);
    void NotifyTrackEvent(ITrackEventListener::ETrackEventReason reason,
        const char* event, const char* param = NULL);

    // Create a new animation node.
    IAnimNode* CreateNodeInternal(EAnimNodeType nodeType, uint32 nNodeId = -1);

    bool AddNodeNeedToRender(IAnimNode* pNode);
    void RemoveNodeNeedToRender(IAnimNode* pNode);

    void SetId(uint32 newId);

    typedef std::vector< _smart_ptr<IAnimNode> > AnimNodes;
    AnimNodes m_nodes;
    AnimNodes m_nodesNeedToRender;

    uint32 m_id;
    string m_name;
    mutable string m_fullNameHolder;
    Range m_timeRange;
    TrackEvents m_events;

    _smart_ptr<IAnimStringTable> m_pEventStrings;

    // Listeners
    typedef std::list<ITrackEventListener*> TTrackEventListeners;
    TTrackEventListeners m_listeners;

    int m_flags;

    bool m_precached;
    bool m_bResetting;

    IAnimSequence* m_pParentSequence;

    //
    IMovieSystem* m_pMovieSystem;
    bool m_bPaused;
    bool m_bActive;

    uint32 m_lastGenId;

    IAnimSequenceOwner* m_pOwner;   // legacy owners (sequence entities) are connected by pointer
    AZ::EntityId        m_ownerId;  // SequenceComponent owners are connected by Id

    IAnimNode* m_pActiveDirector;

    float m_time;
    float m_fixedTimeStep;

    VectorSet<IEntity*> m_precachedEntitiesSet;
    ESequenceType m_sequenceType;       // indicates if this sequence is connected to a legacy sequence entity or Sequence Component

};

#endif // CRYINCLUDE_CRYMOVIE_ANIMSEQUENCE_H
