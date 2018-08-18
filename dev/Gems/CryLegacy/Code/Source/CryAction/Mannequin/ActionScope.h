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

#ifndef CRYINCLUDE_CRYACTION_MANNEQUIN_ACTIONSCOPE_H
#define CRYINCLUDE_CRYACTION_MANNEQUIN_ACTIONSCOPE_H
#pragma once

#include "ICryMannequin.h"
#include "ActionController.h"


struct SScopeContext
{
    SScopeContext()
        : id(0)
        , mask(0)
        , database(NULL)
        , charInst(NULL)
        , entityId(0)
        , cachedEntity(NULL)
        , enslavedController(NULL)
    {
    }

    void Reset(uint32 scopeContextId)
    {
        id = scopeContextId;
        mask = (1 << scopeContextId);
        database = NULL;
        charInst = NULL;
        entityId = AZ::EntityId(0);
        cachedEntity = NULL;
        enslavedController = NULL;
    }

    // HasInvalidCharInst returns true when
    //   character instance is not NULL
    // and
    //   the character instance is not part of the entity
    bool HasInvalidCharInst() const;

    // HasInvalidEntity returns true when
    //   entityId is not 0
    // and
    //   the cachedEntity is that entity
    bool HasInvalidEntity() const;

    uint32                                  id;
    uint32                                  mask;
    const CAnimationDatabase*               database;
    _smart_ptr<ICharacterInstance>          charInst;
    AZ::EntityId                            entityId;
    IEntity*                                cachedEntity;
    CActionController*                      enslavedController;
    TagState                                sharedTags;
    TagState                                setTags;
};


class CActionScope
    : public IScope
{
public:
    friend class CActionController;

    CActionScope(const char* _name, uint32 scopeID, CActionController& actionController, SAnimationContext& _context, SScopeContext& _scopeContext, int layer, int numLayers, const TagState& additionalTags);

    // -- IScope implementation -------------------------------------------------
    virtual ~CActionScope();

    virtual const char* GetName()
    {
        return m_name.c_str();
    }

    virtual uint32 GetID()
    {
        return m_id;
    }

    virtual ICharacterInstance* GetCharInst()
    {
        return m_scopeContext.charInst;
    }

    virtual IActionController& GetActionController() const
    {
        return (IActionController&)m_actionController;
    }

    virtual SAnimationContext& GetContext() const
    {
        return m_context;
    }

    virtual uint32 GetContextID() const
    {
        return m_scopeContext.id;
    }

    virtual const IAnimationDatabase& GetDatabase() const
    {
        CRY_ASSERT(m_scopeContext.database);
        return (const IAnimationDatabase&)*m_scopeContext.database;
    }

    virtual bool HasDatabase() const
    {
        return(m_scopeContext.database != NULL);
    }

    virtual IEntity& GetEntity() const
    {
        AZ_Assert(m_scopeContext.cachedEntity, "No cached entity is present");
        return *(m_scopeContext.cachedEntity);
    }

    virtual EntityId GetEntityId() const
    {
        return GetLegacyEntityId(m_scopeContext.entityId);
    }

    virtual const AZ::EntityId& GetAzEntityId() const
    {
        AZ_Warning("Action Scope", !IsLegacyEntityId(m_scopeContext.entityId), "AZ::EntityId requested is Legacy, this will yield unexpected results");
        return m_scopeContext.entityId;
    }

    virtual uint32 GetTotalLayers() const
    {
        return m_numLayers;
    }
    virtual uint32 GetBaseLayer() const
    {
        return m_layer;
    }
    virtual IActionPtr GetAction() const
    {
        return m_pAction.get();
    }

    virtual void IncrementTime(float timeDelta);

    virtual const CAnimation* GetTopAnim(int layer) const;

    virtual CAnimation* GetTopAnim(int layer);

    virtual void ApplyAnimWeight(uint32 layer, float weight);

    virtual bool IsDifferent(const FragmentID aaID, const TagState& fragmentTags, const TagID subContext = TAG_ID_INVALID) const;

    virtual ILINE FragmentID GetLastFragmentID() const
    {
        return m_lastFragmentID;
    }

    virtual ILINE const SFragTagState& GetLastTagState() const
    {
        return m_lastFragSelection.tagState;
    }

    virtual float CalculateFragmentTimeRemaining() const;

    virtual float CalculateFragmentDuration(const CFragment& fragment) const;

    virtual void _FlushFromEditor() { Flush(FM_Normal); }

    virtual float GetFragmentDuration() const { return m_fragmentDuration; }

    virtual float GetFragmentTime() const { return m_fragmentTime; }

    virtual TagState GetAdditionalTags() const { return m_additionalTags; }

    virtual void MuteLayers(uint32 mutedAnimLayerMask, uint32 mutedProcLayerMask);

    // -- ~IScope implementation ------------------------------------------------


    bool NeedsInstall(uint32 currentContextMask) const
    {
        return (m_additionalTags != TAG_STATE_EMPTY) || ((currentContextMask & m_scopeContext.mask) == 0);
    }

    bool InstallAnimation(int animID, const CryCharAnimationParams& animParams);
    bool InstallAnimation(const SAnimationEntry& animEntry, int layer, const SAnimBlend& animBlend);
    void StopAnimationOnLayer(uint32 layer, float blendTime);
    float GetFragmentStartTime() const;
    bool CanInstall(EPriorityComparison priorityComparison, FragmentID fragID, const SFragTagState& fragTagState, bool isRequeue, float& timeRemaining) const;
    void Install(IAction& action)
    {
        m_pAction = &action;
        m_speedBias = action.GetSpeedBias();
        m_animWeight = action.GetAnimWeight();
    }
    void UpdateSequencers(float timePassed);
    void AdvanceProcSequencers(float timePassed);
    void Update(float timePassed);
    void ClearSequencers();
    void Flush(EFlushMethod flushMethod);
    void QueueAnimFromSequence(uint32 layer, uint32 pos, bool isPersistent);
    void QueueProcFromSequence(uint32 layer, uint32 pos);
    int  GetNumAnimsInSequence(uint32 layer) const;
    bool PlayPendingAnim(uint32 layer, float timePassed = 0.0f);
    bool PlayPendingProc(uint32 layer);
    bool QueueFragment(FragmentID aaID, const SFragTagState& fragTagState, uint32 optionIdx = OPTION_IDX_RANDOM, float startTime = 0.0f, uint32 userToken = 0, bool isRootScope = true, bool isHigherPriority = false, bool principleContext = true);
    void BlendOutFragments();

    ILINE uint32 GetContextMask() const
    {
        return m_scopeContext.mask;
    }

    ILINE uint32 GetLastOptionIdx() const
    {
        return m_lastFragSelection.optionIdx;
    }

    ILINE bool HasFragment() const
    {
        return ((m_sequenceFlags & eSF_Fragment) != 0);
    }
    ILINE bool HasTransition() const
    {
        return ((m_sequenceFlags & eSF_Transition) != 0);
    }
    ILINE bool HasOutroTransition() const
    {
        return ((m_sequenceFlags & eSF_TransitionOutro) != 0);
    }
    ILINE float GetTransitionDuration() const
    {
        return m_transitionDuration;
    }

    ILINE float GetTransitionOutroDuration() const
    {
        return m_transitionOutroDuration;
    }
    ILINE const IActionPtr& GetPlayingAction()
    {
        return m_pExitingAction ? m_pExitingAction : m_pAction;
    }

    ILINE IActionController* GetEnslavedActionController() const
    {
        return m_scopeContext.enslavedController;
    }


    void InstallProceduralClip(const SProceduralEntry& proc, int layer, const SAnimBlend& blend, float duration);

    void Pause();
    void Resume(float forcedBlendOutTime, uint32 resumeFlags);

private:
    void InitAnimationParams(const SAnimationEntry& animEntry, const uint32 sequencerLayer, const SAnimBlend& animBlend, CryCharAnimationParams& paramsOut);
    void FillBlendQuery(SBlendQuery& query, FragmentID fragID, const SFragTagState& fragTagState, bool isHigherPriority, float* pLoopDuration) const;
    void ClipInstalled(uint8 clipType);

private:
    CActionScope();
    CActionScope(const CActionScope&);
    CActionScope& operator = (const CActionScope&);

private:

    enum ESequencerFlags
    {
        eSF_Queued      = BIT(0),
        eSF_BlendingOut = BIT(1)
    };

    struct SSequencer
    {
        SSequencer()
            : installTime(-1.0f)
            , referenceTime(-1.0f)
            , savedAnimNormalisedTime(-1)
            , pos(0)
            , flags(0)
        {
        }

        TAnimClipSequence sequence;
        SAnimBlend blend;
        float installTime; // time in seconds until installation
        float referenceTime;
        float savedAnimNormalisedTime;
        uint8 pos;
        uint8 flags;
    };

    struct SProcSequencer
    {
        SProcSequencer()
            : installTime(-1.0f)
            , pos(0)
            , flags(0)
        {
        }

        TProcClipSequence sequence;
        SAnimBlend blend;
        float installTime;
        AZStd::shared_ptr<IProceduralClip> proceduralClip;
        uint8 pos;
        uint8 flags;
    };

    string m_name;
    uint32 m_id;
    SAnimationContext& m_context;
    SScopeContext& m_scopeContext;
    CActionController& m_actionController;
    int m_layer;
    uint32 m_numLayers;
    SSequencer* m_layerSequencers;
    std::vector<SProcSequencer> m_procSequencers;
    float m_speedBias;
    float m_animWeight;
    float m_timeIncrement;
    TagState m_additionalTags;
    mutable TagState m_cachedFragmentTags;
    mutable TagState m_cachedContextStateMask;
    mutable FragmentID m_cachedaaID;
    mutable uint32 m_cachedTagSetIdx;
    FragmentID m_lastFragmentID;
    SFragmentSelection m_lastFragSelection;
    SFragTagState m_lastQueueTagState;
    uint32 m_sequenceFlags;
    float m_fragmentTime;
    float m_fragmentDuration;
    float m_transitionOutroDuration;
    float m_transitionDuration;
    float m_blendOutDuration;
    EClipType m_partTypes[SFragmentData::PART_TOTAL];

    float m_lastNormalisedTime;
    float m_normalisedTime;

    IActionPtr m_pAction;
    IActionPtr m_pExitingAction;
    uint32 m_userToken; // token that will be passed when installing animations

    uint32 m_mutedAnimLayerMask;
    uint32 m_mutedProcLayerMask;

    bool m_isOneShot;
    bool m_fragmentInstalled;
};

#endif // CRYINCLUDE_CRYACTION_MANNEQUIN_ACTIONSCOPE_H
