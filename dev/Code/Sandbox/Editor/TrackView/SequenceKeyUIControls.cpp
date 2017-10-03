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
#include "TrackViewSequence.h"
#include "TrackViewTrack.h"
#include "TrackViewSequenceManager.h"
#include "TrackViewUndo.h"

//////////////////////////////////////////////////////////////////////////
class CSequenceKeyUIControls
    : public CTrackViewKeyUIControls
{
public:
    CSmartVariableArray mv_table;
    CSmartVariableEnum<QString> mv_name;
    CSmartVariable<bool> mv_overrideTimes;
    CSmartVariable<float> mv_startTime;
    CSmartVariable<float> mv_endTime;

    virtual void OnCreateVars()
    {
        AddVariable(mv_table, "Key Properties");
        AddVariable(mv_table, mv_name, "Sequence");
        AddVariable(mv_table, mv_overrideTimes, "Override Start/End Times");
        AddVariable(mv_table, mv_startTime, "Start Time");
        AddVariable(mv_table, mv_endTime, "End Time");
    }
    bool SupportTrackType(const CAnimParamType& paramType, EAnimCurveType trackType, EAnimValue valueType) const
    {
        return paramType == eAnimParamType_Sequence;
    }
    virtual bool OnKeySelectionChange(CTrackViewKeyBundle& selectedKeys);
    virtual void OnUIChange(IVariable* pVar, CTrackViewKeyBundle& selectedKeys);

    virtual unsigned int GetPriority() const { return 1; }

    static const GUID& GetClassID()
    {
        // {68030C46-1402-45d1-91B3-8EC6F29C0FED}
        static const GUID guid =
        {
            0x68030c46, 0x1402, 0x45d1, { 0x91, 0xb3, 0x8e, 0xc6, 0xf2, 0x9c, 0xf, 0xed }
        };
        return guid;
    }

private:
};

//////////////////////////////////////////////////////////////////////////
bool CSequenceKeyUIControls::OnKeySelectionChange(CTrackViewKeyBundle& selectedKeys)
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
        if (paramType == eAnimParamType_Sequence)
        {
            std::vector<CBaseObject*> objects;
            CTrackViewSequence* pSequence = GetIEditor()->GetAnimation()->GetSequence();

            // fill list with available items
            mv_name.SetEnumList(NULL);

            const CTrackViewSequenceManager* pSequenceManager = GetIEditor()->GetSequenceManager();

            for (int i = 0; i < pSequenceManager->GetCount(); ++i)
            {
                CTrackViewSequence* pCurrentSequence = pSequenceManager->GetSequenceByIndex(i);
                bool bNotMe = pCurrentSequence != pSequence;
                bool bNotParent = !bNotMe || pCurrentSequence->IsAncestorOf(pSequence) == false;
                if (bNotMe && bNotParent)
                {
                    string seqName = pCurrentSequence->GetName();
                    mv_name->AddEnumItem(seqName.c_str(), seqName.c_str());
                }
            }

            ISequenceKey sequenceKey;
            keyHandle.GetKey(&sequenceKey);

            mv_name = sequenceKey.szSelection.c_str();
            mv_overrideTimes = sequenceKey.bOverrideTimes;
            if (!sequenceKey.bOverrideTimes)
            {
                IAnimSequence* pSequence = GetIEditor()->GetSystem()->GetIMovieSystem()->FindSequence(sequenceKey.szSelection.c_str());
                if (pSequence)
                {
                    sequenceKey.fStartTime = pSequence->GetTimeRange().start;
                    sequenceKey.fEndTime = pSequence->GetTimeRange().end;
                }
                else
                {
                    sequenceKey.fStartTime = 0.0f;
                    sequenceKey.fEndTime = 0.0f;
                }
            }

            mv_startTime = sequenceKey.fStartTime;
            mv_endTime = sequenceKey.fEndTime;

            bAssigned = true;
        }
    }
    return bAssigned;
}

// Called when UI variable changes.
void CSequenceKeyUIControls::OnUIChange(IVariable* pVar, CTrackViewKeyBundle& selectedKeys)
{
    CTrackViewSequence* pSequence = GetIEditor()->GetAnimation()->GetSequence();

    if (!pSequence || !selectedKeys.AreAllKeysOfSameType())
    {
        return;
    }

    for (unsigned int keyIndex = 0; keyIndex < selectedKeys.GetKeyCount(); ++keyIndex)
    {
        CTrackViewKeyHandle keyHandle = selectedKeys.GetKey(keyIndex);

        CAnimParamType paramType = keyHandle.GetTrack()->GetParameterType();
        if (paramType == eAnimParamType_Sequence)
        {
            ISequenceKey sequenceKey;
            keyHandle.GetKey(&sequenceKey);

            if (pVar == mv_name.GetVar())
            {
                QString sName;
                sName = mv_name;
                sequenceKey.szSelection = sName.toLatin1().data();
            }

            SyncValue(mv_overrideTimes, sequenceKey.bOverrideTimes, false, pVar);

            IAnimSequence* pSequence = GetIEditor()->GetSystem()->GetIMovieSystem()->FindSequence(sequenceKey.szSelection.c_str());

            if (!sequenceKey.bOverrideTimes)
            {
                if (pSequence)
                {
                    sequenceKey.fStartTime = pSequence->GetTimeRange().start;
                    sequenceKey.fEndTime = pSequence->GetTimeRange().end;
                }
                else
                {
                    sequenceKey.fStartTime = 0.0f;
                    sequenceKey.fEndTime = 0.0f;
                }
            }
            else
            {
                SyncValue(mv_startTime, sequenceKey.fStartTime, false, pVar);
                SyncValue(mv_endTime, sequenceKey.fEndTime, false, pVar);
            }

            sequenceKey.fDuration = sequenceKey.fEndTime - sequenceKey.fStartTime > 0 ? sequenceKey.fEndTime - sequenceKey.fStartTime : 0.0f;

            IMovieSystem* pMovieSystem = GetIEditor()->GetSystem()->GetIMovieSystem();

            if (pMovieSystem != NULL)
            {
                pMovieSystem->SetStartEndTime(pSequence, sequenceKey.fStartTime, sequenceKey.fEndTime);
            }

            CUndo::Record(new CUndoTrackObject(keyHandle.GetTrack()));
            keyHandle.SetKey(&sequenceKey);
        }
    }
}

REGISTER_QT_CLASS_DESC(CSequenceKeyUIControls, "TrackView.KeyUI.Sequence", "TrackViewKeyUI");
