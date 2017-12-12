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

//////////////////////////////////////////////////////////////////////////
class CMusicKeyUIControls
    : public CTrackViewKeyUIControls
{
public:
    CSmartVariableArray mv_table;
    CSmartVariable<QString> mv_mood;
    CSmartVariable<float> mv_time;
    CSmartVariable<bool> mv_type;

    virtual void OnCreateVars()
    {
        mv_time.GetVar()->SetLimits(0, 60.0f);

        AddVariable(mv_table, "Key Properties");
        AddVariable(mv_table, mv_type, "Mood(T) or Volume(F)?");
        AddVariable(mv_table, mv_mood, "Mood (if Mood)");
        AddVariable(mv_table, mv_time, "Time (if Volume)");
    }
    bool SupportTrackType(const CAnimParamType& paramType, EAnimCurveType trackType, EAnimValue valueType) const
    {
        return paramType == eAnimParamType_Music;
    }
    virtual bool OnKeySelectionChange(CTrackViewKeyBundle& selectedKeys);
    virtual void OnUIChange(IVariable* pVar, CTrackViewKeyBundle& selectedKeys);

    virtual unsigned int GetPriority() const { return 1; }

    static const GUID& GetClassID()
    {
        // {351BEE0D-9D29-4fca-ADE7-6C16B607D666}
        static const GUID guid =
        {
            0x351bee0d, 0x9d29, 0x4fca, { 0xad, 0xe7, 0x6c, 0x16, 0xb6, 0x7, 0xd6, 0x66 }
        };
        return guid;
    }
};


//////////////////////////////////////////////////////////////////////////
bool CMusicKeyUIControls::OnKeySelectionChange(CTrackViewKeyBundle& selectedKeys)
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
        if (paramType == eAnimParamType_Music)
        {
            IMusicKey musicKey;
            keyHandle.GetKey(&musicKey);

            mv_type = musicKey.eType == eMusicKeyType_SetMood ? true : false;
            mv_mood = musicKey.szMood;
            mv_time = musicKey.fTime;

            bAssigned = true;
        }
    }
    return bAssigned;
}

// Called when UI variable changes.
void CMusicKeyUIControls::OnUIChange(IVariable* pVar, CTrackViewKeyBundle& selectedKeys)
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
        if (paramType == eAnimParamType_Music)
        {
            IMusicKey musicKey;
            keyHandle.GetKey(&musicKey);

            if (pVar == mv_type.GetVar())
            {
                musicKey.eType = mv_type ? eMusicKeyType_SetMood : eMusicKeyType_VolumeRamp;
            }
            if (pVar == mv_mood.GetVar())
            {
                QString sMood;
                sMood = mv_mood;
                cry_strcpy(musicKey.szMood, sMood.toLatin1().data());
            }
            SyncValue(mv_time, musicKey.fTime, false, pVar);

            CUndo::Record(new CUndoTrackObject(keyHandle.GetTrack()));
            keyHandle.SetKey(&musicKey);
        }
    }
}

REGISTER_QT_CLASS_DESC(CMusicKeyUIControls, "TrackView.KeyUI.Music", "TrackViewKeyUI");
