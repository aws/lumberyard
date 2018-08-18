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
#include "IAnimatedCharacter.h"
#include "ICryMannequin.h"
#include "Maestro/Types/AnimParamType.h"

//////////////////////////////////////////////////////////////////////////
class CMannequinKeyUIControls
    : public CTrackViewKeyUIControls
{
public:
    CSmartVariableArray mv_table;
    CSmartVariable<QString> mv_fragment;
    CSmartVariable<QString> mv_tags;
    CSmartVariable<int> mv_priority;
    CSmartVariable<bool> mv_trump;

    virtual void OnCreateVars()
    {
        AddVariable(mv_table, "Key Properties");
        AddVariable(mv_table, mv_fragment, "mannequin fragment");
        AddVariable(mv_table, mv_tags, "fragment tags");
        AddVariable(mv_table, mv_priority, "priority");
    }
    bool SupportTrackType(const CAnimParamType& paramType, EAnimCurveType trackType, AnimValueType valueType) const
    {
        return paramType == AnimParamType::Mannequin;
    }
    virtual bool OnKeySelectionChange(CTrackViewKeyBundle& selectedKeys);
    virtual void OnUIChange(IVariable* pVar, CTrackViewKeyBundle& selectedKeys);

    virtual unsigned int GetPriority() const { return 1; }

    static const GUID& GetClassID()
    {
        static const GUID guid = {
            0x1a7867f0, 0xf20a, 0x4f88, { 0xa5, 0xff, 0x80, 0xf0, 0x25, 0xd1, 0x6, 0x8f }
        };

        return guid;
    }
};

//////////////////////////////////////////////////////////////////////////
bool CMannequinKeyUIControls::OnKeySelectionChange(CTrackViewKeyBundle& selectedKeys)
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
        if (paramType == AnimParamType::Mannequin)
        {
            IMannequinKey key;
            keyHandle.GetKey(&key);

            mv_fragment = key.m_fragmentName.c_str();
            mv_tags = key.m_tags.c_str();
            mv_priority = key.m_priority;

            bAssigned = true;
        }
    }
    return bAssigned;
}

// Called when UI variable changes.
void CMannequinKeyUIControls::OnUIChange(IVariable* pVar, CTrackViewKeyBundle& selectedKeys)
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
        if (paramType == AnimParamType::Mannequin)
        {
            IMannequinKey key;
            keyHandle.GetKey(&key);

            if (mv_fragment.GetVar() == pVar)
            {
                key.m_fragmentName = ((QString)mv_fragment).toUtf8().data();
            }

            if (!key.m_fragmentName.empty())
            {
                IEntity* entity = keyHandle.GetTrack()->GetAnimNode()->GetEntity();
                if (entity)
                {
                    ICharacterInstance* pCharacter = entity->GetCharacter(0);
                    if (pCharacter)
                    {
                        //find fragment duration

                        IGameObject* pGameObject = gEnv->pGame->GetIGameFramework()->GetGameObject(entity->GetId());
                        if (pGameObject)
                        {
                            IAnimatedCharacter* pAnimChar = (IAnimatedCharacter*) pGameObject->QueryExtension("AnimatedCharacter");
                            if (pAnimChar)
                            {
                                float fFragDuration = 0.0f;
                                float fTransDuration = 0.0f;
                                QString animation;
                                mv_fragment->Get(animation);
                                const uint32  valueName = CCrc32::ComputeLowercase(animation.toUtf8().data());

                                const FragmentID fragID = pAnimChar->GetActionController()->GetContext().controllerDef.m_fragmentIDs.Find(valueName);

                                bool isValid = !(FRAGMENT_ID_INVALID == fragID);

                                IActionPtr pAction = new TAction<SAnimationContext>(3, fragID, TAG_STATE_EMPTY, 0u, 0xffffffff);
                                pAnimChar->GetActionController()->QueryDuration(*pAction, fFragDuration, fTransDuration);
                                key.m_duration = fFragDuration + fTransDuration;
                                key.m_priority = mv_priority;
                                key.m_tags = ((QString)mv_tags).toUtf8().data();
                            }
                        }
                    }
                }
            }

            keyHandle.SetKey(&key);
        }
    }
}

REGISTER_QT_CLASS_DESC(CMannequinKeyUIControls, "TrackView.KeyUI.Mannequin", "TrackViewKeyUI");
