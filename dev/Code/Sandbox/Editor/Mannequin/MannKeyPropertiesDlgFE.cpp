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

// Description : CMannequinDialog Implementation file.


#include "StdAfx.h"
#include "FragmentEditor.h"

#include "ViewPane.h"
#include "StringDlg.h"

#include <IGameFramework.h>

#include "ICryMannequin.h"

#include "MannKeyPropertiesDlgFE.h"
#include "Objects/SequenceObject.h"
#include "FragmentTrack.h"
#include "SequencerNode.h"
#include "MannequinDialog.h"

#include "Serialization/VariableOArchive.h"
#include "Serialization/VariableIArchive.h"

// removed temporarily while we get the functionality to support these into the engine
#define ALLOW_CURVE_EDIT 0

//////////////////////////////////////////////////////////////////////////
class CAnimClipUIControls
    : public CSequencerKeyUIControls
{
public:
    bool m_settingUp;

    bool SupportTrackType(ESequencerParamType type) const
    {
        if (type == SEQUENCER_PARAM_ANIMLAYER)
        {
            return true;
        }
        return false;
    }
    virtual bool OnKeySelectionChange(SelectedKeys& selectedKeys);
    virtual void OnUIChange(IVariable* pVar, SelectedKeys& selectedKeys);

protected:
    virtual void OnCreateVars()
    {
        mv_table->DeleteAllVariables();

        m_settingUp = false;
        AddVariable(mv_table, "Anim Clip Properties");
        AddVariable(mv_table, mv_animation, "Animation", IVariable::DT_ANIMATION);
        AddVariable(mv_table, mv_playbackWeight, "Playback Weight");
        AddVariable(mv_table, mv_playbackSpeed, "Playback Speed");
        AddVariable(mv_table, mv_time, "Time");
        AddVariable(mv_table, mv_startTime, "Start Time");
        AddVariable(mv_table, mv_looping, "Looping");
        AddVariable(mv_table, mv_timeWarping, "Time Warping");
        AddVariable(mv_table, mv_idle2move, "Idle2Move");
        AddVariable(mv_table, mv_fullRootControl, "Full control over root movement");
        AddVariable(mv_table, mv_blendDuration, "Blend");
#if ALLOW_CURVE_EDIT
        AddVariable(mv_table, mv_blendCurveType, "Blend Type");
#endif
        AddVariable(mv_table, mv_jointMask, "UseJointMasking");
        AddVariable(mv_table, mv_alignToPrevious, "AlignToPrevious");

        for (uint32 i = 0; i < MANN_NUMBER_BLEND_CHANNELS; i++)
        {
            AddVariable(mv_table, mv_blendChannel[i], "Blend Channel");
        }

        mv_alignToPrevious->SetFlags(mv_alignToPrevious->GetFlags() | IVariable::UI_DISABLED);
        mv_playbackWeight->SetLimits(0.0f, 20.0f);
        mv_playbackSpeed->SetLimits(0.0f, 20.0f);
        mv_blendDuration->SetLimits(0.0f, 20.0f);

#if ALLOW_CURVE_EDIT
        mv_blendCurveType.SetEnumList(nullptr);
        mv_blendCurveType->AddEnumItem("Ease In/Out", static_cast<int>(ECurveType::EaseInOut));
        mv_blendCurveType->AddEnumItem("Ease In", static_cast<int>(ECurveType::EaseIn));
        mv_blendCurveType->AddEnumItem("Ease Out", static_cast<int>(ECurveType::EaseOut));
        mv_blendCurveType->AddEnumItem("Linear", static_cast<int>(ECurveType::Linear));
#endif
    }

private:
    CSmartVariableArray mv_table;

    CSmartVariableEnum<QString> mv_animation;
    CSmartVariable<float> mv_startTime;
    CSmartVariable<float> mv_playbackWeight;
    CSmartVariable<float> mv_playbackSpeed;
    CSmartVariable<float> mv_time;
    CSmartVariable<float> mv_blendDuration;
    CSmartVariableEnum<int> mv_blendCurveType;
    CSmartVariable<bool> mv_alignToPrevious;
    CSmartVariable<bool> mv_looping;
    CSmartVariable<bool> mv_timeWarping;
    CSmartVariable<bool> mv_idle2move;
    CSmartVariable<bool> mv_fullRootControl;
    CSmartVariable<int> mv_jointMask;
    CSmartVariable<float> mv_blendChannel[MANN_NUMBER_BLEND_CHANNELS];
};

bool SynchFlag(int& dest, int flag, IVariable* pVar, CSmartVariable<bool>& flagVar)
{
    if (pVar == flagVar.GetVar())
    {
        if (flagVar)
        {
            dest |= flag;
        }
        else
        {
            dest &= ~flag;
        }
        return true;
    }

    return false;
}

//////////////////////////////////////////////////////////////////////////
bool CAnimClipUIControls::OnKeySelectionChange(SelectedKeys& selectedKeys)
{
    bool bAssigned = false;
    m_settingUp = true;
    if (selectedKeys.keys.size() == 1)
    {
        const SelectedKey& selectedKey = selectedKeys.keys[0];
        const ESequencerParamType paramType = selectedKey.pTrack->GetParameterType();
        if (paramType == SEQUENCER_PARAM_ANIMLAYER)
        {
            mv_animation.SetEnumList(NULL);

            // Add available animations.
            // Find editor object who owns this node.
            IEntity* entity = selectedKey.pNode->GetEntity();
            if (entity)
            {
                // Set the entity as a user data so that the UI
                // can display the proper list of animations.
                mv_animation->SetUserData(static_cast<AZ::u64>(entity->GetId()));
            }

            CClipKey key;
            selectedKey.pTrack->GetKey(selectedKey.nKey, &key);

            mv_animation                = key.animRef.c_str();
            mv_playbackWeight       = key.playbackWeight;
            mv_playbackSpeed        = key.playbackSpeed;
            mv_time                 = key.time;
            mv_startTime                = key.startTime;
            mv_blendDuration        = key.blendDuration;
            mv_blendCurveType       = static_cast<int>(key.blendCurveType);
            mv_alignToPrevious  = key.alignToPrevious;
            mv_looping                  = key.animFlags & CA_LOOP_ANIMATION;
            mv_timeWarping          = key.animFlags & CA_TRANSITION_TIMEWARPING;
            mv_idle2move                = key.animFlags & CA_IDLE2MOVE;
            mv_fullRootControl  = key.animFlags & CA_FULL_ROOT_PRIORITY;
            mv_jointMask              = key.jointMask;
            for (uint32 i = 0; i < MANN_NUMBER_BLEND_CHANNELS; i++)
            {
                *mv_blendChannel[i] = key.blendChannels[i];
            }

            mv_startTime->SetLimits(0.0f, key.GetAssetDuration());

            bAssigned = true;
        }
    }
    m_settingUp = false;
    return bAssigned;
}

// Called when UI variable changes.
void CAnimClipUIControls::OnUIChange(IVariable* pVar, SelectedKeys& selectedKeys)
{
    if (!m_settingUp)
    {
        for (int keyIndex = 0, num = (int)selectedKeys.keys.size(); keyIndex < num; keyIndex++)
        {
            SelectedKey& selectedKey = selectedKeys.keys[keyIndex];

            CClipKey key;
            selectedKey.pTrack->GetKey(selectedKey.nKey, &key);

            QString animation;
            animation = mv_animation;

            IEntity* pEntity = selectedKey.pNode->GetEntity();
            if ((pVar == mv_animation.GetVar()) && pEntity)
            {
                ICharacterInstance* pCharInst = pEntity->GetCharacter(0);
                IAnimationSet* pAnimations = NULL;
                if (pCharInst)
                {
                    pAnimations = pCharInst->GetIAnimationSet();
                    assert(pAnimations);
                }
                key.SetAnimation(animation, pAnimations);
            }
            SyncValue(mv_blendDuration, key.blendDuration, false, pVar);
            if (pVar == mv_blendCurveType.GetVar())
            {
                key.blendCurveType = static_cast<ECurveType>(static_cast<int>(mv_blendCurveType));
            }
            SyncValue(mv_alignToPrevious, key.alignToPrevious, false, pVar);
            SyncValue(mv_playbackSpeed, key.playbackSpeed, false, pVar);
            SyncValue(mv_time, key.time, false, pVar);
            SyncValue(mv_startTime, key.startTime, false, pVar);
            SyncValue(mv_playbackWeight, key.playbackWeight, false, pVar);
            if (pVar == mv_jointMask.GetVar())
            {
                key.jointMask = mv_jointMask;
            }
            SynchFlag(key.animFlags, CA_LOOP_ANIMATION, pVar, mv_looping);
            SynchFlag(key.animFlags, CA_TRANSITION_TIMEWARPING, pVar, mv_timeWarping);
            SynchFlag(key.animFlags, CA_IDLE2MOVE, pVar, mv_idle2move);
            SynchFlag(key.animFlags, CA_FULL_ROOT_PRIORITY, pVar, mv_fullRootControl);

            for (uint32 i = 0; i < MANN_NUMBER_BLEND_CHANNELS; i++)
            {
                if (pVar == mv_blendChannel[i].GetVar())
                {
                    key.blendChannels[i] = mv_blendChannel[i];
                }
            }

            selectedKey.pTrack->SetKey(selectedKey.nKey, &key);
            selectedKey.pTrack->OnChange();
        }

        RefreshSequencerKeys();
    }
}

//////////////////////////////////////////////////////////////////////////
class CTransitionPropertyUIControls
    : public CSequencerKeyUIControls
{
public:
    CMannKeyPropertiesDlgFE* m_parent;

    bool SupportTrackType(ESequencerParamType type) const
    {
        if (type == SEQUENCER_PARAM_TRANSITIONPROPS)
        {
            return true;
        }
        return false;
    }
    virtual bool OnKeySelectionChange(SelectedKeys& selectedKeys);
    virtual void OnUIChange(IVariable* pVar, SelectedKeys& selectedKeys);

protected:
    virtual void OnCreateVars()
    {
        mv_table->DeleteAllVariables();

        AddVariable(mv_table, "Transition Properties");
        AddVariable(mv_table, mv_cyclicTransition, "Cyclic Transition", IVariable::DT_SIMPLE);
        AddVariable(mv_table, mv_cycleLocked, "Cycle Locked", IVariable::DT_SIMPLE);
        AddVariable(mv_table, mv_outroTransition, "Outro Transition", IVariable::DT_SIMPLE);
        AddVariable(mv_table, mv_time, "Time");
        AddVariable(mv_table, mv_selectTime, "Select Time", IVariable::DT_SIMPLE);
        AddVariable(mv_table, mv_startTime, "Earliest Start Time", IVariable::DT_SIMPLE);
    }

private:

    CSmartVariableArray mv_table;
    CSmartVariable<bool> mv_cyclicTransition;
    CSmartVariable<bool> mv_cycleLocked;
    CSmartVariable<bool> mv_outroTransition;
    CSmartVariable<float> mv_time;
    CSmartVariable<float> mv_selectTime;
    CSmartVariable<float> mv_startTime;
};

//////////////////////////////////////////////////////////////////////////
bool CTransitionPropertyUIControls::OnKeySelectionChange(SelectedKeys& selectedKeys)
{
    bool bAssigned = false;
    const uint32 numScopes = m_parent->m_contexts->m_scopeData.size();
    const int numSelKeys = selectedKeys.keys.size();

    if (numSelKeys == 1)
    {
        const ESequencerParamType paramType = selectedKeys.keys[0].pTrack->GetParameterType();
        if (paramType == SEQUENCER_PARAM_TRANSITIONPROPS)
        {
            CTransitionPropertyTrack* propTrack = (CTransitionPropertyTrack*)selectedKeys.keys[0].pTrack;

            CTransitionPropertyKey key;
            propTrack->GetKey(selectedKeys.keys[0].nKey, &key);

            mv_time = key.time;

            if (key.blend.pFragmentBlend)
            {
                mv_cyclicTransition = (key.tranFlags & SFragmentBlend::Cyclic);
                mv_cycleLocked          = (key.tranFlags & SFragmentBlend::CycleLocked);
                mv_outroTransition  = (key.tranFlags & SFragmentBlend::ExitTransition);

                mv_selectTime = key.blend.pFragmentBlend->selectTime;
                mv_startTime = key.blend.pFragmentBlend->startTime;
            }

            bAssigned = true;
        }
    }

    return bAssigned;
}

// Called when UI variable changes.
void CTransitionPropertyUIControls::OnUIChange(IVariable* pVar, SelectedKeys& selectedKeys)
{
    std::vector<SelectedKey> selKeys;

    //--- Strip out any repeat references to shared keys
    std::set<uint32> sharedKeys;
    const int numSelKeys = (int)selectedKeys.keys.size();
    for (int keyIndex = 0; keyIndex < numSelKeys; keyIndex++)
    {
        SelectedKey& selectedKey = selectedKeys.keys[keyIndex];

        const ESequencerParamType paramType = selectedKey.pTrack->GetParameterType();
        if (paramType == SEQUENCER_PARAM_TRANSITIONPROPS)
        {
            CTransitionPropertyKey key;
            selectedKey.pTrack->GetKey(selectedKey.nKey, &key);
            selKeys.push_back(selectedKey);
        }
    }

    const int newNumSelKeys = (int)selKeys.size();
    for (int keyIndex = 0; keyIndex < newNumSelKeys; keyIndex++)
    {
        SelectedKey& selectedKey = selKeys[keyIndex];
        const ESequencerParamType paramType = selectedKey.pTrack->GetParameterType();
        if (paramType == SEQUENCER_PARAM_TRANSITIONPROPS)
        {
            CTransitionPropertyTrack* propTrack = (CTransitionPropertyTrack*)selectedKey.pTrack;

            CTransitionPropertyKey key;
            selectedKey.pTrack->GetKey(selectedKey.nKey, &key);

            key.time = mv_time;

            bool overrideProps = false;
            uint8 oldFlags = key.tranFlags;
            if (pVar == mv_cyclicTransition.GetVar())
            {
                if (mv_cyclicTransition)
                {
                    key.tranFlags |= SFragmentBlend::Cyclic;
                }
                else
                {
                    key.tranFlags &= ~SFragmentBlend::Cyclic;
                }
            }
            if (pVar == mv_cycleLocked.GetVar())
            {
                if (mv_cycleLocked)
                {
                    key.tranFlags |= SFragmentBlend::CycleLocked;
                }
                else
                {
                    key.tranFlags &= ~SFragmentBlend::CycleLocked;
                }
            }
            if (pVar == mv_outroTransition.GetVar())
            {
                if (mv_outroTransition)
                {
                    key.tranFlags |= SFragmentBlend::ExitTransition;
                }
                else
                {
                    key.tranFlags &= ~SFragmentBlend::ExitTransition;
                }
            }
            overrideProps |= (oldFlags != key.tranFlags);
            SFragmentBlend blendCopy = *propTrack->GetBlendForKey(keyIndex);
            SyncValue(mv_selectTime, blendCopy.selectTime, false, pVar);
            SyncValue(mv_startTime, blendCopy.startTime, false, pVar);
            overrideProps |= (blendCopy.selectTime != key.blend.pFragmentBlend->selectTime);
            overrideProps |=  (blendCopy.startTime != key.blend.pFragmentBlend->startTime);
            key.overrideSelectTime = blendCopy.selectTime;
            key.overrideStartTime = blendCopy.startTime;
            propTrack->UpdateBlendForKey(keyIndex, blendCopy);
            key.overrideProps = overrideProps;

            selectedKey.pTrack->SetKey(selectedKey.nKey, &key);

            selectedKey.pTrack->OnChange();
        }
    }

    RefreshSequencerKeys();
}

//////////////////////////////////////////////////////////////////////////
class CFragmentIDUIControls
    : public CSequencerKeyUIControls
{
public:
    CTagControl m_tagControls;

    CMannKeyPropertiesDlgFE* m_parent;

    void RecreateVars(const CTagDefinition* tagDef, bool isTransition)
    {
        if ((tagDef != m_lastTagDef) || (isTransition != m_lastTransition))
        {
            m_lastTagDef = tagDef;
            m_pVarBlock->DeleteAllVariables();
            CVariableBase& var = mv_table;
            m_pVarBlock->AddVariable(&var);

            if (tagDef)
            {
                m_tagControls.Init(m_pVarBlock, *tagDef);
                m_tagControls.SetCallback(functor((CSequencerKeyUIControls&)*this, &CSequencerKeyUIControls::OnInternalVariableChange));
            }

            if (isTransition != m_lastTransition)
            {
                if (isTransition)
                {
                    AddVariable(mv_table, mv_cyclicTransition, "Cyclic Transition", IVariable::DT_SIMPLE);
                    AddVariable(mv_table, mv_outroTransition, "Outro Transition", IVariable::DT_SIMPLE);

                    AddVariable(mv_table, mv_selectTime, "Select Time", IVariable::DT_SIMPLE);
                    AddVariable(mv_table, mv_startTime, "Start Time", IVariable::DT_SIMPLE);
                }
                else
                {
                    CVariableBase& varTran = mv_cyclicTransition;
                    mv_table->DeleteVariable(&varTran, false);
                    CVariableBase& varTranOutro = mv_outroTransition;
                    mv_table->DeleteVariable(&varTranOutro, false);
                    CVariableBase& varSelect = mv_selectTime;
                    mv_table->DeleteVariable(&varSelect, false);
                    CVariableBase& varStart = mv_startTime;
                    mv_table->DeleteVariable(&varStart, false);
                }
                m_lastTransition = isTransition;
            }

            m_parent->ForceUpdate();
        }
    }

    bool SupportTrackType(ESequencerParamType type) const
    {
        if (type == SEQUENCER_PARAM_FRAGMENTID)
        {
            return true;
        }
        return false;
    }
    virtual bool OnKeySelectionChange(SelectedKeys& selectedKeys);
    virtual void OnUIChange(IVariable* pVar, SelectedKeys& selectedKeys);

protected:
    virtual void OnCreateVars()
    {
        m_lastTagDef = NULL;
        mv_table->DeleteAllVariables();

        AddVariable(mv_table, "Fragment Properties");
        AddVariable(mv_table, mv_fragmentID, "FragmentID", IVariable::DT_SIMPLE);
        AddVariable(mv_table, mv_fragmentOptionIdx, "OptionIdx", IVariable::DT_SIMPLE);
        AddVariable(mv_table, mv_trumpPrevious, "Trump Previous Fragment", IVariable::DT_SIMPLE);
        AddVariable(mv_table, mv_time, "Time");
    }

private:

    const CTagDefinition* m_lastTagDef;
    bool m_lastTransition;

    CSmartVariableArray mv_table;
    CSmartVariableEnum<QString> mv_fragmentID;
    CSmartVariable<int> mv_fragmentOptionIdx;
    CSmartVariable<bool> mv_trumpPrevious;
    CSmartVariable<float> mv_time;
    CSmartVariable<bool> mv_cyclicTransition;
    CSmartVariable<bool> mv_cycleLocked;
    CSmartVariable<bool> mv_outroTransition;
    CSmartVariable<float> mv_selectTime;
    CSmartVariable<float> mv_startTime;
};

//////////////////////////////////////////////////////////////////////////
bool CFragmentIDUIControls::OnKeySelectionChange(SelectedKeys& selectedKeys)
{
    bool bAssigned = false;
    const uint32 numScopes = m_parent->m_contexts->m_scopeData.size();
    const int numSelKeys = selectedKeys.keys.size();

    bool isSame = true;
    FragmentID fragID = FRAGMENT_ID_INVALID;
    for (int i = 0; i < numSelKeys; i++)
    {
        SelectedKey& selectedKey = selectedKeys.keys[i];
        const ESequencerParamType paramType = selectedKey.pTrack->GetParameterType();
        if (paramType == SEQUENCER_PARAM_FRAGMENTID)
        {
            CFragmentTrack* fragTrack = (CFragmentTrack*)selectedKey.pTrack;
            CFragmentKey key;
            fragTrack->GetKey(selectedKey.nKey, &key);

            if (i == 0)
            {
                fragID = key.fragmentID;
            }
            else if (key.fragmentID != fragID)
            {
                isSame = false;
            }
        }
    }

    if ((numSelKeys > 0) && isSame)
    {
        const ESequencerParamType paramType = selectedKeys.keys[0].pTrack->GetParameterType();
        if (paramType == SEQUENCER_PARAM_FRAGMENTID)
        {
            CFragmentTrack* fragTrack = (CFragmentTrack*)selectedKeys.keys[0].pTrack;
            const CTagDefinition& fragDefs = fragTrack->GetScopeData().mannContexts->m_controllerDef->m_fragmentIDs;
            const uint32 numFrags = fragDefs.GetNum();

            const SControllerDef* m_controllerDef = fragTrack->GetScopeData().mannContexts->m_controllerDef;
            mv_fragmentID.SetEnumList(NULL);
            mv_fragmentID->AddEnumItem("", "None");
            const ActionScopes scopeMask = (1 << fragTrack->GetScopeData().scopeID);
            for (uint32 i = 0; i < numFrags; i++)
            {
                if (m_controllerDef->m_fragmentDef[i].scopeMaskList.GetDefault() & scopeMask)
                {
                    const char* szFragment = fragDefs.GetTagName(i);
                    mv_fragmentID->AddEnumItem(szFragment, szFragment);
                }
            }

            CFragmentKey key;
            fragTrack->GetKey(selectedKeys.keys[0].nKey, &key);

            //for (uint32 i=0; i<numScopes; i++)
            //{
            //  *m_scopeMask[i] = ((key.scopeMask&(1<<i)) != 0);
            //  (*m_scopeMask[i])->SetFlags(IVariable::UI_DISABLED);
            //}

            const CTagDefinition* fragTagDef = NULL;
            const char* fragName = "None";
            if (key.fragmentID != FRAGMENT_ID_INVALID)
            {
                fragName     = fragDefs.GetTagName(key.fragmentID);
                fragTagDef = fragTrack->GetScopeData().mannContexts->m_controllerDef->GetFragmentTagDef(key.fragmentID);
            }
            mv_fragmentID = fragName;
            if (key.fragOptionIdx == OPTION_IDX_RANDOM)
            {
                mv_fragmentOptionIdx = 0;
            }
            else
            {
                mv_fragmentOptionIdx = key.fragOptionIdx + 1;
            }
            mv_trumpPrevious = key.trumpPrevious;
            mv_time = key.time;
            mv_cyclicTransition = ((key.tranFlags & SFragmentBlend::Cyclic) != 0);
            mv_cycleLocked = ((key.tranFlags & SFragmentBlend::Cyclic) != 0);
            mv_outroTransition = ((key.tranFlags & SFragmentBlend::ExitTransition) != 0);

            mv_selectTime = key.tranSelectTime;
            mv_startTime = key.tranStartTimeValue;

            RecreateVars(fragTagDef, key.transition);
            if (m_lastTagDef)
            {
                m_tagControls.Set(key.tagStateFull.fragmentTags);
            }

            bAssigned = true;
        }
    }



    return bAssigned;
}

// Called when UI variable changes.
void CFragmentIDUIControls::OnUIChange(IVariable* pVar, SelectedKeys& selectedKeys)
{
    std::vector<SelectedKey> selKeys;

    //--- Strip out any repeat references to shared keys
    std::set<uint32> sharedKeys;
    const int numSelKeys = (int)selectedKeys.keys.size();
    for (int keyIndex = 0; keyIndex < numSelKeys; keyIndex++)
    {
        SelectedKey& selectedKey = selectedKeys.keys[keyIndex];

        const ESequencerParamType paramType = selectedKey.pTrack->GetParameterType();
        if (paramType == SEQUENCER_PARAM_FRAGMENTID)
        {
            CFragmentKey key;
            selectedKey.pTrack->GetKey(selectedKey.nKey, &key);
            if (key.sharedID)
            {
                if (sharedKeys.find(key.sharedID) != sharedKeys.end())
                {
                    continue;
                }

                sharedKeys.insert(key.sharedID);
            }
            selKeys.push_back(selectedKey);
        }
    }

    const int newNumSelKeys = (int)selKeys.size();
    for (int keyIndex = 0; keyIndex < newNumSelKeys; keyIndex++)
    {
        SelectedKey& selectedKey = selKeys[keyIndex];
        const ESequencerParamType paramType = selectedKey.pTrack->GetParameterType();
        if (paramType == SEQUENCER_PARAM_FRAGMENTID)
        {
            CFragmentTrack* fragTrack = (CFragmentTrack*)selectedKey.pTrack;
            const SControllerDef& contDef  = *fragTrack->GetScopeData().mannContexts->m_controllerDef;
            const CTagDefinition& fragDefs = contDef.m_fragmentIDs;

            CFragmentKey key;
            selectedKey.pTrack->GetKey(selectedKey.nKey, &key);

            QString fragment = mv_fragmentID;

            if (pVar == mv_fragmentID.GetVar())
            {
                key.fragmentID = fragDefs.Find(fragment.toUtf8().data());

                const CTagDefinition* fragTagDef = (key.fragmentID == FRAGMENT_ID_INVALID) ? NULL : contDef.GetFragmentTagDef(key.fragmentID);
                RecreateVars(fragTagDef, key.transition);
                if (fragTagDef)
                {
                    m_tagControls.Set(key.tagStateFull.fragmentTags);
                }
            }
            else if (pVar == mv_fragmentOptionIdx.GetVar())
            {
                key.fragOptionIdx = mv_fragmentOptionIdx;
                if (key.fragOptionIdx == 0)
                {
                    key.fragOptionIdx = OPTION_IDX_RANDOM;
                }
                else
                {
                    key.fragOptionIdx--;
                }
            }
            else if (m_lastTagDef)
            {
                key.tagStateFull.fragmentTags = m_tagControls.Get();
            }

            SyncValue(mv_trumpPrevious, key.trumpPrevious, false, pVar);
            SyncValue(mv_time, key.time, false, pVar);
            if (pVar == mv_cyclicTransition.GetVar())
            {
                if (mv_cyclicTransition)
                {
                    key.tranFlags |= SFragmentBlend::Cyclic;
                }
                else
                {
                    key.tranFlags &= ~SFragmentBlend::Cyclic;
                }
            }
            if (pVar == mv_cycleLocked.GetVar())
            {
                if (mv_cycleLocked)
                {
                    key.tranFlags |= SFragmentBlend::CycleLocked;
                }
                else
                {
                    key.tranFlags &= ~SFragmentBlend::CycleLocked;
                }
            }
            if (pVar == mv_outroTransition.GetVar())
            {
                if (mv_outroTransition)
                {
                    key.tranFlags |= SFragmentBlend::ExitTransition;
                }
                else
                {
                    key.tranFlags &= ~SFragmentBlend::ExitTransition;
                }
            }
            SyncValue(mv_selectTime, key.tranSelectTime, false, pVar);

            const float prevStartTimeValue = key.tranStartTimeValue;
            SyncValue(mv_startTime, key.tranStartTimeValue, false, pVar);
            key.tranStartTimeRelative = prevStartTimeValue - key.tranStartTimeValue;

            selectedKey.pTrack->SetKey(selectedKey.nKey, &key);

            selectedKey.pTrack->OnChange();
        }
    }

    RefreshSequencerKeys();
}


//////////////////////////////////////////////////////////////////////////
class CProcClipUIControls
    : public CSequencerKeyUIControls
{
public:
    CMannKeyPropertiesDlgFE* m_parent;

    bool SupportTrackType(ESequencerParamType type) const
    {
        if (type == SEQUENCER_PARAM_PROCLAYER)
        {
            return true;
        }
        return false;
    }


    virtual bool OnKeySelectionChange(SelectedKeys& selectedKeys);
    virtual void OnUIChange(IVariable* pVar, SelectedKeys& selectedKeys);
    void OnProceduralParamsVariableChanged(IVariable* pVar);

protected:
    virtual void OnCreateVars()
    {
        mv_table->DeleteAllVariables();

        AddVariable(mv_table, "Procedural Clip Properties");
        AddVariable(mv_table, mv_procType, "Type", IVariable::DT_SIMPLE);
        AddVariable(mv_table, mv_blendDuration, "Blend");
#if ALLOW_CURVE_EDIT
        AddVariable(mv_table, mv_blendCurveType, "Blend Type");
#endif
        AddVariable(mv_table, mv_time, "Time");

#if ALLOW_CURVE_EDIT
        mv_blendCurveType.SetEnumList(nullptr);
        mv_blendCurveType->AddEnumItem("Ease In/Out", static_cast<int>(ECurveType::EaseInOut));
        mv_blendCurveType->AddEnumItem("Ease In", static_cast<int>(ECurveType::EaseIn));
        mv_blendCurveType->AddEnumItem("Ease Out", static_cast<int>(ECurveType::EaseOut));
        mv_blendCurveType->AddEnumItem("Linear", static_cast<int>(ECurveType::Linear));
#endif
    }

private:
    void UpdateProcType(CProcClipKey& key, EntityId entityId);

    CSmartVariableArray mv_table;
    CSmartVariableEnum<QString> mv_procType;
    CSmartVariable<float> mv_blendDuration;
    CSmartVariableEnum<int> mv_blendCurveType;
    CSmartVariable<float> mv_time;

    _smart_ptr<IVariable> mv_params;
};

void AddOnSetCallback(IVariable* const pVariable, const IVariable::OnSetCallback& callback)
{
    CRY_ASSERT(pVariable);

    const int childCount = pVariable->GetNumVariables();
    for (int i = 0; i < childCount; ++i)
    {
        IVariable* const pChildVariable = pVariable->GetVariable(i);
        AddOnSetCallback(pChildVariable, callback);
    }

    pVariable->AddOnSetCallback(callback);
}

//////////////////////////////////////////////////////////////////////////
bool CProcClipUIControls::OnKeySelectionChange(SelectedKeys& selectedKeys)
{
    ///
    struct ScopedDisableUpdateCallbacks
    {
        void EnableCallbacks(IVariable* const pVariable, const bool enable)
        {
            if (pVariable)
            {
                pVariable->EnableUpdateCallbacks(enable);
                for (int i = 0; i < pVariable->GetNumVariables(); ++i)
                {
                    EnableCallbacks(pVariable->GetVariable(i), enable);
                }
            }
        }

        ScopedDisableUpdateCallbacks(IVariablePtr p)
            : m_p(p) { EnableCallbacks(m_p.get(), false); }
        ~ScopedDisableUpdateCallbacks() { EnableCallbacks(m_p.get(), true); }
        IVariablePtr m_p;
    };

    ScopedDisableUpdateCallbacks scopedDisableCallbacks(mv_table.GetVar());

    bool bAssigned = false;
    if (selectedKeys.keys.size() == 1)
    {
        SelectedKey& selectedKey = selectedKeys.keys[0];
        const ESequencerParamType paramType = selectedKey.pTrack->GetParameterType();
        if (paramType == SEQUENCER_PARAM_PROCLAYER)
        {
            mv_procType.SetEnumList(NULL);

            IProceduralClipFactory& proceduralClipFactory = gEnv->pGame->GetIGameFramework()->GetMannequinInterface().GetProceduralClipFactory();
            QStringList procClipTypes;
            procClipTypes.reserve(proceduralClipFactory.GetRegisteredCount());
            for (size_t i = 0; i < proceduralClipFactory.GetRegisteredCount(); ++i)
            {
                const char* typeName = proceduralClipFactory.GetTypeNameByIndex(i);
                if (typeName != nullptr)
                {
                    procClipTypes.push_back(typeName);
                }
            }
            std::sort(procClipTypes.begin(), procClipTypes.end());
            mv_procType->AddEnumItem("<None>", "");
            for (size_t i = 0; i < procClipTypes.size(); ++i)
            {
                const QString& typeName = procClipTypes[i];
                mv_procType->AddEnumItem(typeName, typeName);
            }

            CProcClipKey key;
            selectedKey.pTrack->GetKey(selectedKey.nKey, &key);

            {
                const char* const keyTypeName = proceduralClipFactory.FindTypeName(key.typeNameHash);
                mv_procType->EnableUpdateCallbacks(false);
                mv_procType = keyTypeName;
                mv_procType->EnableUpdateCallbacks(true);
            }
            mv_blendDuration = key.blendDuration;
            mv_blendCurveType = static_cast<int>(key.blendCurveType);

            mv_time = key.time;

            {
                if (mv_params)
                {
                    m_pVarBlock->DeleteVariable(mv_params);
                    mv_params = IVariablePtr();
                }

                if (key.pParams)
                {
                    Serialization::CVariableOArchive ar;

                    IEntity* entity = selectedKey.pNode->GetEntity();
                    if (entity)
                    {
                        ar.SetAnimationEntityId(entity->GetId());
                    }

                    key.pParams->Serialize(ar);

                    mv_params = ar.GetIVariable();
                    if (mv_params && !mv_params->IsEmpty())
                    {
                        AddOnSetCallback(mv_params.get(), functor(*this, &CProcClipUIControls::OnProceduralParamsVariableChanged));

                        m_pVarBlock->AddVariable(mv_params, "Params");
                    }
                }
            }



            bAssigned = true;
        }
    }
    if (bAssigned)
    {
        m_parent->RedrawProps();
        m_parent->ForceUpdate();
    }
    return bAssigned;
}

void CProcClipUIControls::UpdateProcType(CProcClipKey& key, EntityId entityId)
{
    if (mv_params)
    {
        m_pVarBlock->DeleteVariable(mv_params);
        mv_params = IVariablePtr();
    }

    IProceduralClipFactory& proceduralClipFactory = gEnv->pGame->GetIGameFramework()->GetMannequinInterface().GetProceduralClipFactory();
    const char* const keyTypeName = proceduralClipFactory.FindTypeName(key.typeNameHash);
    mv_procType = keyTypeName;

    {
        if (!key.typeNameHash.IsEmpty())
        {
            key.pParams = proceduralClipFactory.CreateProceduralClipParams(key.typeNameHash);
            if (key.pParams)
            {
                Serialization::CVariableOArchive ar;
                ar.SetAnimationEntityId(entityId);

                key.pParams->Serialize(ar);

                mv_params = ar.GetIVariable();

                if (mv_params && !mv_params->IsEmpty())
                {
                    AddOnSetCallback(mv_params.get(), functor(*this, &CProcClipUIControls::OnProceduralParamsVariableChanged));
                    m_pVarBlock->AddVariable(mv_params, "Params");
                }

                key.blendDuration = max(0.f, key.pParams->GetEditorDefaultBlendDuration());
            }
        }
        else
        {
            key.pParams.reset();
        }
    }

    m_parent->RedrawProps();
}

// Called when UI variable changes.
void CProcClipUIControls::OnUIChange(IVariable* pVar, SelectedKeys& selectedKeys)
{
    bool typeChanged = false;
    for (int keyIndex = 0, num = (int)selectedKeys.keys.size(); keyIndex < num; keyIndex++)
    {
        SelectedKey& selectedKey = selectedKeys.keys[keyIndex];
        const ESequencerParamType paramType = selectedKey.pTrack->GetParameterType();
        if (paramType == SEQUENCER_PARAM_PROCLAYER)
        {
            CProcClipKey key;
            selectedKey.pTrack->GetKey(selectedKey.nKey, &key);

            QString procType = mv_procType;

            if (pVar == mv_procType.GetVar())
            {
                key.typeNameHash = IProceduralClipFactory::THash(procType.toUtf8().data());

                const EntityId entityId = selectedKey.pNode->GetEntity() ? selectedKey.pNode->GetEntity()->GetId() : 0;
                UpdateProcType(key, entityId);
                typeChanged = true;
            }
            else if (pVar == mv_blendDuration.GetVar())
            {
                SyncValue(mv_blendDuration, key.blendDuration, false, pVar);
            }
            else if (pVar == mv_blendCurveType.GetVar())
            {
                key.blendCurveType = static_cast<ECurveType>(static_cast<int>(mv_blendCurveType));
            }

            key.time = mv_time;

            key.UpdateDurationBasedOnParams();

            selectedKey.pTrack->SetKey(selectedKey.nKey, &key);

            selectedKey.pTrack->OnChange();
        }
    }

    if (typeChanged)
    {
        m_parent->ForceUpdate();
    }

    RefreshSequencerKeys();
}

//////////////////////////////////////////////////////////////////////////
void CProcClipUIControls::OnProceduralParamsVariableChanged(IVariable* pVar)
{
    SelectedKeys selectedKeys;
    CSequencerUtils::GetSelectedKeys(m_pKeyPropertiesDlg->GetSequence(), selectedKeys);

    for (int keyIndex = 0, num = (int)selectedKeys.keys.size(); keyIndex < num; keyIndex++)
    {
        SelectedKey& selectedKey = selectedKeys.keys[keyIndex];
        const ESequencerParamType paramType = selectedKey.pTrack->GetParameterType();
        if (paramType == SEQUENCER_PARAM_PROCLAYER)
        {
            CProcClipKey& key = *static_cast<CProcClipKey*>(selectedKey.pTrack->GetKey(selectedKey.nKey));

            if (key.pParams && mv_params)
            {
                CRY_ASSERT(mv_params == m_pVarBlock->FindVariable("Params"));
                Serialization::CVariableIArchive ar(mv_params);
                key.pParams->Serialize(ar);
            }
            // -- force update if params changed, incase we want to render different variables
            if (m_parent)
            {
                m_parent->ForceUpdate();
            }
            key.UpdateDurationBasedOnParams();
            selectedKey.pTrack->OnChange();
        }
    }

    RefreshSequencerKeys();
}

//////////////////////////////////////////////////////////////////////////
class CTagStateUIControls
    : public CSequencerKeyUIControls
{
public:

    CTagControl m_tagControls;

    CMannKeyPropertiesDlgFE* m_parent;

    bool SupportTrackType(ESequencerParamType type) const
    {
        if (type == SEQUENCER_PARAM_TAGS)
        {
            return true;
        }
        return false;
    }


    virtual bool OnKeySelectionChange(SelectedKeys& selectedKeys);
    virtual void OnUIChange(IVariable* pVar, SelectedKeys& selectedKeys);

    void RenameTag(TagID tagID, const QString& name) override;
    void RemoveTag(TagID tagID) override;
    void AddTag(const QString& name) override;

protected:
    virtual void OnCreateVars()
    {
        AddVariable(mv_time, "Time");

        if (m_parent->m_contexts->m_controllerDef)
        {
            m_tagControls.Init(m_pVarBlock, m_parent->m_contexts->m_controllerDef->m_tags);
            m_pVarBlock->AddOnSetCallback(functor((CSequencerKeyUIControls&)*this, &CSequencerKeyUIControls::OnInternalVariableChange));
        }
    }

private:
    CSmartVariable<float> mv_time;
};

//////////////////////////////////////////////////////////////////////////
void CTagStateUIControls::RenameTag(TagID tagID, const QString& name)
{
    m_tagControls.RenameTag(tagID, name);
}

void CTagStateUIControls::RemoveTag(TagID tagID)
{
    m_tagControls.RemoveTag(m_pVarBlock, tagID);

    SelectedKeys selectedKeys;
    CSequencerUtils::GetAllKeys(m_pKeyPropertiesDlg->GetSequence(), selectedKeys);

    for (SelectedKey& selectedKey : selectedKeys.keys)
    {
        const ESequencerParamType paramType = selectedKey.pTrack->GetParameterType();
        if (paramType == SEQUENCER_PARAM_TAGS)
        {
            CTagKey key;
            selectedKey.pTrack->GetKey(selectedKey.nKey, &key);

            const CTagDefinition& tagDef = static_cast<CTagTrack*>(selectedKey.pTrack)->GetTagDef();

            TagState tags;
            tags.Clear();

            for (uint32 i = 0; i < tagDef.GetNum(); ++i)
            {
                if (i != tagID)
                {
                    if (tagDef.IsSet(key.tagState, i))
                    {
                        tagDef.Set(tags, i < tagID ? i : i - 1, true);
                    }
                }
            }

            key.tagState = tags;
        }
    }
}

void CTagStateUIControls::AddTag(const QString& name)
{
    m_tagControls.AddTag(m_pVarBlock, name);
    m_pVarBlock->AddOnSetCallback(functor(static_cast<CSequencerKeyUIControls&>(*this), &CSequencerKeyUIControls::OnInternalVariableChange));
}

bool CTagStateUIControls::OnKeySelectionChange(SelectedKeys& selectedKeys)
{
    bool bAssigned = false;
    if (selectedKeys.keys.size() == 1)
    {
        SelectedKey& selectedKey = selectedKeys.keys[0];
        ESequencerParamType paramType = selectedKey.pTrack->GetParameterType();
        if (paramType == SEQUENCER_PARAM_TAGS)
        {
            CTagKey key;
            selectedKey.pTrack->GetKey(selectedKey.nKey, &key);

            mv_time = key.time;

            m_tagControls.Set(key.tagState);

            //const uint32 numTags = m_parent->m_context.m_controllerDef->m_tags.GetNum();
            //for (uint32 i=0; i<numTags; i++)
            //{
            //  *m_tagState[i] = ((key.tagState&(1<<i)) != 0);
            //}

            bAssigned = true;
        }
    }
    return bAssigned;
}

// Called when UI variable changes.
void CTagStateUIControls::OnUIChange(IVariable* pVar, SelectedKeys& selectedKeys)
{
    bool changed = false;
    for (int keyIndex = 0, num = (int)selectedKeys.keys.size(); keyIndex < num; keyIndex++)
    {
        SelectedKey& selectedKey = selectedKeys.keys[keyIndex];
        const ESequencerParamType paramType = selectedKey.pTrack->GetParameterType();
        if (paramType == SEQUENCER_PARAM_TAGS)
        {
            CTagKey key;
            selectedKey.pTrack->GetKey(selectedKey.nKey, &key);

            key.time = mv_time;

            key.tagState = m_tagControls.Get();

            //const uint32 numTags = m_parent->m_context.m_controllerDef->m_tags.GetNum();
            //for (uint32 i=0; i<numTags; i++)
            //{
            //  if (pVar==m_tagState[i]->GetVar())
            //  {
            //      const uint32 mask = (1<<i);
            //      if (**m_tagState[i])
            //      {
            //          key.tagState |= mask;
            //      }
            //      else
            //      {
            //          key.tagState &= ~mask;
            //      }
            //  }
            //}

            selectedKey.pTrack->SetKey(selectedKey.nKey, &key);

            selectedKey.pTrack->OnChange();
        }
    }

    if (changed)
    {
        m_parent->RedrawProps();
    }

    RefreshSequencerKeys();
}


//////////////////////////////////////////////////////////////////////////
class CMannParamUIControls
    : public CSequencerKeyUIControls
{
public:
    static const uint32 NUM_PARAMS_EXPOSED = 7;

    CMannKeyPropertiesDlgFE* m_parent;

    bool SupportTrackType(ESequencerParamType type) const
    {
        if (type == SEQUENCER_PARAM_PARAMS)
        {
            return true;
        }
        return false;
    }


    virtual bool OnKeySelectionChange(SelectedKeys& selectedKeys);
    virtual void OnUIChange(IVariable* pVar, SelectedKeys& selectedKeys);

protected:
    virtual void OnCreateVars()
    {
        mv_table->DeleteAllVariables();

        AddVariable(mv_table, "Parameter Properties");
        AddVariable(mv_table, mv_paramName, "Name", IVariable::DT_SIMPLE);
        AddVariable(mv_table, mv_paramIsLocator, "Locator", IVariable::DT_SIMPLE);
        AddVariable(mv_table, mv_time, "Time");
        for (uint32 i = 0; i < NUM_PARAMS_EXPOSED; i++)
        {
            AddVariable(*mv_param[i], "Param");
        }
    }

private:
    CSmartVariableArray mv_table;
    CSmartVariableEnum<QString> mv_paramName;
    CSmartVariable<float> mv_param[NUM_PARAMS_EXPOSED];
    CSmartVariable<bool> mv_paramIsLocator;
    CSmartVariable<float> mv_time;
};

//////////////////////////////////////////////////////////////////////////
bool CMannParamUIControls::OnKeySelectionChange(SelectedKeys& selectedKeys)
{
    bool bAssigned = false;
    if (selectedKeys.keys.size() == 1)
    {
        SelectedKey& selectedKey = selectedKeys.keys[0];
        const ESequencerParamType paramType = selectedKey.pTrack->GetParameterType();
        if (paramType == SEQUENCER_PARAM_PARAMS)
        {
            CParamKey key;
            selectedKey.pTrack->GetKey(selectedKey.nKey, &key);

            mv_paramName.SetEnumList(NULL);

            for (size_t i = 0; i < MannUtils::GetParamNameCount(); ++i)
            {
                const char* const paramName = MannUtils::GetParamNameByIndex(i);
                mv_paramName->AddEnumItem(paramName, paramName);
            }

            for (uint32 i = 0; i < eMotionParamID_COUNT; i++)
            {
                mv_paramName->AddEnumItem(MannUtils::GetMotionParamByIndex(i), MannUtils::GetMotionParamByIndex(i));
            }

            mv_paramName = key.name;

            mv_paramIsLocator = key.isLocation;

            mv_time = key.time;

            float* pParamAsFloats = (float*)&key.parameter.value;
            for (uint32 i = 0; i < NUM_PARAMS_EXPOSED; i++)
            {
                *mv_param[i] = pParamAsFloats[i];
            }

            bAssigned = true;
        }
    }
    return bAssigned;
}

// Called when UI variable changes.
void CMannParamUIControls::OnUIChange(IVariable* pVar, SelectedKeys& selectedKeys)
{
    bool changed = false;
    for (int keyIndex = 0, num = (int)selectedKeys.keys.size(); keyIndex < num; keyIndex++)
    {
        SelectedKey& selectedKey = selectedKeys.keys[keyIndex];
        const ESequencerParamType paramType = selectedKey.pTrack->GetParameterType();
        if (paramType == SEQUENCER_PARAM_PARAMS)
        {
            CParamKey key;
            selectedKey.pTrack->GetKey(selectedKey.nKey, &key);

            key.name = mv_paramName;

            key.isLocation = mv_paramIsLocator;

            key.time = mv_time;

            float* pParamAsFloats = (float*)&key.parameter.value;
            for (uint32 i = 0; i < NUM_PARAMS_EXPOSED; i++)
            {
                pParamAsFloats[i] = *mv_param[i];
            }

            selectedKey.pTrack->SetKey(selectedKey.nKey, &key);

            selectedKey.pTrack->OnChange();
        }
    }

    if (changed)
    {
        m_parent->RedrawProps();
    }

    RefreshSequencerKeys();
}

//////////////////////////////////////////////////////////////////////////
CMannKeyPropertiesDlgFE::CMannKeyPropertiesDlgFE(QWidget* parent)
    : CSequencerKeyPropertiesDlg(parent, true)
    , m_contexts(NULL)
    , m_lastPropertyType(-1)
{
    OnInitDialog();
}

CMannKeyPropertiesDlgFE::~CMannKeyPropertiesDlgFE()
{
}

void CMannKeyPropertiesDlgFE::OnInitDialog()
{
    m_contexts = CMannequinDialog::GetCurrentInstance()->Contexts();

    m_keyControls.push_back(new CAnimClipUIControls());
    CProcClipUIControls* pProcControls = new CProcClipUIControls();
    pProcControls->m_parent = this;
    m_keyControls.push_back(pProcControls);
    CTransitionPropertyUIControls* pPropControls = new CTransitionPropertyUIControls();
    pPropControls->m_parent = this;
    m_keyControls.push_back(pPropControls);
    CFragmentIDUIControls* pFragControls = new CFragmentIDUIControls();
    pFragControls->m_parent = this;
    m_keyControls.push_back(pFragControls);
    CTagStateUIControls* pTagStateControls = new CTagStateUIControls();
    pTagStateControls->m_parent = this;
    m_keyControls.push_back(pTagStateControls);
    CMannParamUIControls* pMannParamControls = new CMannParamUIControls();
    pMannParamControls->m_parent = this;
    m_keyControls.push_back(pMannParamControls);
    CreateAllVars();
}

void CMannKeyPropertiesDlgFE::RedrawProps()
{
    m_wndProps->update();
}

void CMannKeyPropertiesDlgFE::RepopulateUI(CSequencerKeyUIControls* controls)
{
    m_pVarBlock->DeleteAllVariables();
    AddVars(controls);
    PopulateVariables();
}

void CMannKeyPropertiesDlgFE::OnKeySelectionChange()
{
    CSequencerUtils::SelectedKeys selectedKeys;
    CSequencerUtils::GetSelectedKeys(GetSequence(), selectedKeys);

    if (m_wndTrackProps)
    {
        m_wndTrackProps->OnKeySelectionChange(selectedKeys);
    }

    bool isInSameTrack = false;
    bool bAssigned = false;
    if (selectedKeys.keys.size() > 0)
    {
        const CSequencerUtils::SelectedKey& selectedKey = selectedKeys.keys[0];
        const CSequencerTrack* track = selectedKey.pTrack;
        for (int i = 0; i < (int)m_keyControls.size(); i++)
        {
            if (m_keyControls[i]->SupportTrackType(track->GetParameterType()))
            {
                isInSameTrack = (m_lastPropertyType == i);
                if (m_keyControls[i]->OnKeySelectionChange(selectedKeys))
                {
                    bAssigned = true;
                }
                if (!isInSameTrack || m_forceUpdate)
                {
                    m_pVarBlock->DeleteAllVariables();
                    AddVars(m_keyControls[i]);
                }

                m_lastPropertyType = i;

                break;
            }
        }

        const bool canEditKey = track->CanEditKey(selectedKey.nKey);
        m_wndProps->SetReadOnly(!canEditKey);

        m_wndProps->setEnabled(true);
    }
    else
    {
        m_pVarBlock->DeleteAllVariables();
        m_lastPropertyType = -1;
        m_wndProps->setEnabled(false);
    }

    if (isInSameTrack && !m_forceUpdate)
    {
        m_wndProps->ClearSelection();
        ReloadValues();
    }
    else
    {
        m_forceUpdate = false;
        PopulateVariables();
    }
}

void CMannKeyPropertiesDlgFE::OnUpdate()
{
    if (m_forceUpdate)
    {
        OnKeySelectionChange();
    }
}

void CMannKeyPropertiesDlgFE::OnTagRenamed(const CTagDefinition* tagDef, TagID tagID, const QString& name)
{
    if (tagDef == &m_contexts->m_controllerDef->m_tags)
    {
        for (auto key : m_keyControls)
        {
            key->RenameTag(tagID, name);
        }

        RepopulateSelectedKeyUI();
    }
}

void CMannKeyPropertiesDlgFE::OnTagRemoved(const CTagDefinition* tagDef, TagID tagID)
{
    if (tagDef == &m_contexts->m_controllerDef->m_tags)
    {
        for (auto key : m_keyControls)
        {
            key->RemoveTag(tagID);
        }

        RepopulateSelectedKeyUI();
    }
}

void CMannKeyPropertiesDlgFE::OnTagAdded(const CTagDefinition* tagDef, const QString& name)
{
    if (tagDef == &m_contexts->m_controllerDef->m_tags)
    {
        for (auto key : m_keyControls)
        {
            key->AddTag(name);
        }

        RepopulateSelectedKeyUI();
    }
}

void CMannKeyPropertiesDlgFE::RepopulateSelectedKeyUI()
{
    CSequencerUtils::SelectedKeys selectedKeys;
    CSequencerUtils::GetSelectedKeys(GetSequence(), selectedKeys);

    if (selectedKeys.keys.size() > 0)
    {
        const CSequencerUtils::SelectedKey& selectedKey = selectedKeys.keys[0];
        const CSequencerTrack* track = selectedKey.pTrack;

        auto it = std::find_if(m_keyControls.begin(), m_keyControls.end(),
                [&](const _smart_ptr<CSequencerKeyUIControls>& key)
                {
                    return key->SupportTrackType(track->GetParameterType());
                });

        if (it != m_keyControls.end())
        {
            RepopulateUI(*it);
        }
    }
}

#include <Mannequin/MannKeyPropertiesDlgFE.moc>
