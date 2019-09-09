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

#include "StdAfx.h"
#include "TrackViewKeyPropertiesDlg.h"
#include "TrackViewTrack.h"
#include <Maestro/Types/AnimParamType.h>
#include <Maestro/Types/SequenceType.h>

#include "Objects/EntityObject.h"

//////////////////////////////////////////////////////////////////////////
class CLookAtKeyUIControls
    : public CTrackViewKeyUIControls
{
public:
    CSmartVariableArray mv_table;
    CSmartVariableEnum<QString> mv_name;
    CSmartVariableEnum<QString> mv_lookPose;
    CSmartVariable<float> mv_smoothTime;

    virtual void OnCreateVars()
    {
        AddVariable(mv_table, "Key Properties");
        AddVariable(mv_table, mv_name, "Entity");
        AddVariable(mv_table, mv_smoothTime, "Target Smooth Time");
        AddVariable(mv_table, mv_lookPose, "Look Pose");
    }
    bool SupportTrackType(const CAnimParamType& paramType, EAnimCurveType trackType, AnimValueType valueType) const
    {
        return paramType == AnimParamType::LookAt;
    }
    virtual bool OnKeySelectionChange(CTrackViewKeyBundle& selectedKeys);
    virtual void OnUIChange(IVariable* pVar, CTrackViewKeyBundle& selectedKeys);

    virtual unsigned int GetPriority() const { return 1; }

    static const GUID& GetClassID()
    {
        // {9BE53EAD-D36C-4d8d-A6E4-8B31D173B826}
        static const GUID guid =
        {
            0x9be53ead, 0xd36c, 0x4d8d, { 0xa6, 0xe4, 0x8b, 0x31, 0xd1, 0x73, 0xb8, 0x26 }
        };
        return guid;
    }

private:
};

//////////////////////////////////////////////////////////////////////////
bool CLookAtKeyUIControls::OnKeySelectionChange(CTrackViewKeyBundle& selectedKeys)
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
        if (paramType == AnimParamType::LookAt)
        {
            // Add the options for the boneSets to the combo box.
            mv_lookPose.SetEnumList(NULL);

            IEntity* entity = keyHandle.GetTrack()->GetAnimNode()->GetEntity();
            if (entity)
            {
                ICharacterInstance* pCharacter = entity->GetCharacter(0);
                if (pCharacter)
                {
                    IAnimationSet* pAnimations = pCharacter->GetIAnimationSet();
                    assert (pAnimations);

                    uint32 numAnims = pAnimations->GetAnimationCount();
                    for (uint32 i = 0; i < numAnims; ++i)
                    {
                        const char* animName = pAnimations->GetNameByAnimID(i);
                        string strAnimName(animName);
                        strAnimName.MakeLower();

                        if (strstr(strAnimName.c_str(), "lookpose"))
                        {
                            QString str(animName);
                            mv_lookPose->AddEnumItem(str, str);
                        }
                    }
                }
            }
            /*
                        for (int i = 0; i < eLookAtKeyBoneSet_COUNT; ++i)
                            mv_boneSet->AddEnumItem(m_boneSetNamesMap[ELookAtKeyBoneSet(i)],
                            m_boneSetNamesMap[ELookAtKeyBoneSet(i)]);
            */

            mv_name.SetEnumList(NULL);
            std::vector<CBaseObject*> objects;
            // First, add the local player.
            mv_name->AddEnumItem("_LocalPlayer", "_LocalPlayer");
            // Get All entity nodes
            GetIEditor()->GetObjectManager()->GetObjects(objects);
            for (int i = 0; i < objects.size(); ++i)
            {
                if (qobject_cast<CEntityObject*>(objects[i]))
                {
                    mv_name->AddEnumItem(objects[i]->GetName(), objects[i]->GetName());
                }
            }

            ILookAtKey lookAtKey;
            keyHandle.GetKey(&lookAtKey);

            mv_name = lookAtKey.szSelection.c_str();
            mv_lookPose = lookAtKey.lookPose.c_str();
            mv_smoothTime = lookAtKey.smoothTime;

            bAssigned = true;
        }
    }
    return bAssigned;
}

// Called when UI variable changes.
void CLookAtKeyUIControls::OnUIChange(IVariable* pVar, CTrackViewKeyBundle& selectedKeys)
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
        if (paramType == AnimParamType::LookAt)
        {
            ILookAtKey lookAtKey;
            keyHandle.GetKey(&lookAtKey);

            if (pVar == mv_name.GetVar())
            {
                QString sName;
                sName = mv_name;
                lookAtKey.szSelection = sName.toUtf8().data();
            }

            if (pVar == mv_lookPose.GetVar())
            {
                QString sLookPose;
                sLookPose = mv_lookPose;
                lookAtKey.lookPose = sLookPose.toUtf8().data();
            }

            SyncValue(mv_smoothTime, lookAtKey.smoothTime, false, pVar);

            bool isDuringUndo = false;
            AzToolsFramework::ToolsApplicationRequests::Bus::BroadcastResult(isDuringUndo, &AzToolsFramework::ToolsApplicationRequests::Bus::Events::IsDuringUndoRedo);

            if (isDuringUndo)
            {
                keyHandle.SetKey(&lookAtKey);
            }
            else
            {
                AzToolsFramework::ScopedUndoBatch undoBatch("Set Key Value");
                keyHandle.SetKey(&lookAtKey);
                undoBatch.MarkEntityDirty(sequence->GetSequenceComponentEntityId());
            }

        }
    }
}

REGISTER_QT_CLASS_DESC(CLookAtKeyUIControls, "TrackView.KeyUI.LookAt", "TrackViewKeyUI");
