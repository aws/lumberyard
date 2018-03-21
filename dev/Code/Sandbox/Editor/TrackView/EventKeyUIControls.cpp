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

#include "stdafx.h"
#include "TrackViewKeyPropertiesDlg.h"
#include "TrackViewTrack.h"
#include "TrackViewUndo.h"
#include "CryEditDoc.h"
#include "Objects/EntityScript.h"
#include "Mission.h"
#include "MissionScript.h"
#include <Maestro/Types/AnimNodeType.h>
#include <Maestro/Types/AnimParamType.h>
#include <Maestro/Types/SequenceType.h>

//////////////////////////////////////////////////////////////////////////
class CEventKeyUIControls
    : public CTrackViewKeyUIControls
{
public:
    CSmartVariableArray mv_table;
    CSmartVariableArray mv_deprecated;

    CSmartVariableEnum<QString> mv_animation;
    CSmartVariableEnum<QString> mv_event;
    CSmartVariable<QString> mv_value;
    CSmartVariable<bool> mv_notrigger_in_scrubbing;

    virtual void OnCreateVars()
    {
        AddVariable(mv_table, "Key Properties");
        AddVariable(mv_table, mv_event, "Event");
        AddVariable(mv_table, mv_value, "Value");
        AddVariable(mv_table, mv_notrigger_in_scrubbing, "No trigger in scrubbing");
        AddVariable(mv_deprecated, "Deprecated");
        AddVariable(mv_deprecated, mv_animation, "Animation");
    }
    bool SupportTrackType(const CAnimParamType& paramType, EAnimCurveType trackType, AnimValueType valueType) const
    {
        return paramType == AnimParamType::Event;
    }
    virtual bool OnKeySelectionChange(CTrackViewKeyBundle& selectedKeys);
    virtual void OnUIChange(IVariable* pVar, CTrackViewKeyBundle& selectedKeys);

    virtual unsigned int GetPriority() const { return 1; }

    static const GUID& GetClassID()
    {
        // {ED5A2023-EDE1-4a47-BBE6-7D7BA0E4001D}
        static const GUID guid =
        {
            0xed5a2023, 0xede1, 0x4a47, { 0xbb, 0xe6, 0x7d, 0x7b, 0xa0, 0xe4, 0x0, 0x1d }
        };
        return guid;
    }

private:
};

//////////////////////////////////////////////////////////////////////////
bool CEventKeyUIControls::OnKeySelectionChange(CTrackViewKeyBundle& selectedKeys)
{
    if (!selectedKeys.AreAllKeysOfSameType())
    {
        return false;
    }

    bool bAssigned = false;
    if (selectedKeys.GetKeyCount() == 1)
    {
        const CTrackViewKeyHandle& keyHandle = selectedKeys.GetKey(0);

        CAnimParamType paramType = keyHandle.GetTrack()->GetParameterType();
        if (paramType == AnimParamType::Event)
        {
            mv_event.SetEnumList(NULL);
            mv_animation.SetEnumList(NULL);

            // Add <None> for empty, unset event
            mv_event->AddEnumItem(QObject::tr("<None>"), "");
            mv_animation->AddEnumItem(QObject::tr("<None>"), "");

            if (keyHandle.GetTrack()->GetAnimNode()->GetType() == AnimNodeType::Director)
            {
                CMission* pMission = GetIEditor()->GetDocument()->GetCurrentMission();
                if (pMission)
                {
                    CMissionScript* pScript = pMission->GetScript();
                    if (pScript)
                    {
                        for (int i = 0; i < pScript->GetEventCount(); i++)
                        {
                            mv_event->AddEnumItem(pScript->GetEvent(i),
                                pScript->GetEvent(i));
                        }
                    }
                }
            }
            else
            {
                // Find editor object who owns this node.
                IEntity* pEntity = keyHandle.GetTrack()->GetAnimNode()->GetEntity();
                if (pEntity)
                {
                    // Add events.
                    // Find EntityClass.
                    CEntityScript* script = CEntityScriptRegistry::Instance()->Find(pEntity->GetClass()->GetName());
                    if (script)
                    {
                        for (int i = 0; i < script->GetEventCount(); i++)
                        {
                            mv_event->AddEnumItem(script->GetEvent(i), script->GetEvent(i));
                        }
                    }

                    // Add available animations.
                    ICharacterInstance* pCharacter = pEntity->GetCharacter(0);
                    if (pCharacter)
                    {
                        IAnimationSet* pAnimations = pCharacter->GetIAnimationSet();
                        assert (pAnimations);

                        uint32 numAnims = pAnimations->GetAnimationCount();
                        for (int i = 0; i < numAnims; ++i)
                        {
                            mv_animation->AddEnumItem(pAnimations->GetNameByAnimID(i),
                                pAnimations->GetNameByAnimID(i));
                        }
                    }
                }
            }

            IEventKey eventKey;
            keyHandle.GetKey(&eventKey);

            mv_event = eventKey.event.c_str();
            mv_value = eventKey.eventValue.c_str();
            mv_animation = eventKey.animation.c_str();
            mv_notrigger_in_scrubbing = eventKey.bNoTriggerInScrubbing;

            bAssigned = true;
        }
    }
    return bAssigned;
}

// Called when UI variable changes.
void CEventKeyUIControls::OnUIChange(IVariable* pVar, CTrackViewKeyBundle& selectedKeys)
{
    CTrackViewSequence* sequence = GetIEditor()->GetAnimation()->GetSequence();

    if (!sequence || !selectedKeys.AreAllKeysOfSameType())
    {
        return;
    }

    for (unsigned int keyIndex = 0; keyIndex < selectedKeys.GetKeyCount(); ++keyIndex)
    {
        CTrackViewKeyHandle keyHandle = selectedKeys.GetKey(keyIndex);

        CAnimParamType paramType = keyHandle.GetTrack()->GetParameterType();
        if (paramType == AnimParamType::Event)
        {
            IEventKey eventKey;
            keyHandle.GetKey(&eventKey);

            QByteArray event, value, animation;
            event = static_cast<QString>(mv_event).toUtf8();
            value = static_cast<QString>(mv_value).toUtf8();
            animation = static_cast<QString>(mv_animation).toUtf8();

            if (pVar == mv_event.GetVar())
            {
                eventKey.event = event.data();
            }
            if (pVar == mv_value.GetVar())
            {
                eventKey.eventValue = value.data();
            }
            if (pVar == mv_animation.GetVar())
            {
                eventKey.animation = animation.data();
            }
            SyncValue(mv_notrigger_in_scrubbing, eventKey.bNoTriggerInScrubbing, false, pVar);

            if (!eventKey.animation.empty())
            {
                IEntity* pEntity = keyHandle.GetTrack()->GetAnimNode()->GetEntity();
                if (pEntity)
                {
                    ICharacterInstance* pCharacter = pEntity->GetCharacter(0);
                    if (pCharacter)
                    {
                        IAnimationSet* pAnimations = pCharacter->GetIAnimationSet();
                        assert (pAnimations);
                        int id = pAnimations->GetAnimIDByName(eventKey.animation.c_str());
                        eventKey.duration = pAnimations->GetDuration_sec(id);
                    }
                }
            }

            bool isDuringUndo = false;
            AzToolsFramework::ToolsApplicationRequests::Bus::BroadcastResult(isDuringUndo, &AzToolsFramework::ToolsApplicationRequests::Bus::Events::IsDuringUndoRedo);

            if (sequence->GetSequenceType() == SequenceType::Legacy)
            {
                CUndo::Record(new CUndoTrackObject(keyHandle.GetTrack()));
                keyHandle.SetKey(&eventKey);
            }
            else if (isDuringUndo)
            {
                keyHandle.SetKey(&eventKey);
            }
            else
            {
                AzToolsFramework::ScopedUndoBatch undoBatch("Set Key Value");
                keyHandle.SetKey(&eventKey);
                undoBatch.MarkEntityDirty(sequence->GetSequenceComponentEntityId());
            }
        }
    }
}

REGISTER_QT_CLASS_DESC(CEventKeyUIControls, "TrackView.KeyUI.Event", "TrackViewKeyUI");
