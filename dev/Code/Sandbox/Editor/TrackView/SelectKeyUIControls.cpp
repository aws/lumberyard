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

#include <AzCore/Component/ComponentApplicationBus.h>
#include <AzFramework/Components/CameraBus.h>
#include <AzCore/Component/Entity.h>
#include <AzToolsFramework/ToolsComponents/GenericComponentWrapper.h>
#include <AzToolsFramework/Entity/EditorEntityContextComponent.h>

#include "TrackViewKeyPropertiesDlg.h"
#include "TrackViewTrack.h"
#include "TrackViewUndo.h"

#include "objects/CameraObject.h"

#include <QObject>

//////////////////////////////////////////////////////////////////////////
class CSelectKeyUIControls
    : public CTrackViewKeyUIControls
{
public:
    CSelectKeyUIControls()
        : m_isLegacyCamera(true) {}

    CSmartVariableArray mv_table;
    CSmartVariableEnum<QString> mv_camera;
    CSmartVariable<float> mv_BlendTime;

    virtual void OnCreateVars()
    {
        AddVariable(mv_table, "Key Properties");
        AddVariable(mv_table, mv_camera, "Camera");
        AddVariable(mv_table, mv_BlendTime, "Blend time");
    }
    bool SupportTrackType(const CAnimParamType& paramType, EAnimCurveType trackType, EAnimValue valueType) const
    {
        return valueType == eAnimValue_Select;
    }
    virtual bool OnKeySelectionChange(CTrackViewKeyBundle& selectedKeys);
    virtual void OnUIChange(IVariable* pVar, CTrackViewKeyBundle& selectedKeys);

    virtual unsigned int GetPriority() const { return 1; }

    static const GUID& GetClassID()
    {
        // {9018D0D1-24CC-45e5-9D3D-16D3F9E591B2}
        static const GUID guid =
        {
            0x9018d0d1, 0x24cc, 0x45e5, { 0x9d, 0x3d, 0x16, 0xd3, 0xf9, 0xe5, 0x91, 0xb2 }
        };
        return guid;
    }
private:
    bool                        m_isLegacyCamera;
};

//////////////////////////////////////////////////////////////////////////
bool CSelectKeyUIControls::OnKeySelectionChange(CTrackViewKeyBundle& selectedKeys)
{
    if (!selectedKeys.AreAllKeysOfSameType())
    {
        return false;
    }

    bool bAssigned = false;
    if (selectedKeys.GetKeyCount() == 1)
    {
        const CTrackViewKeyHandle& keyHandle = selectedKeys.GetKey(0);

        EAnimValue valueType = keyHandle.GetTrack()->GetValueType();
        if (valueType == eAnimValue_Select)
        {
            // Get All cameras.
            CTrackViewSequence* sequence = GetIEditor()->GetAnimation()->GetSequence();
            ESequenceType sequenceType = sequence ? sequence->GetSequenceType() : eSequenceType_Legacy;

            mv_camera.SetEnumList(NULL);

            // Insert '<None>' empty enum
            if (sequenceType == eSequenceType_Legacy)
            {
                mv_camera->AddEnumItem(QObject::tr("<None>"), "");
            }
            else if (sequenceType == eSequenceType_SequenceComponent)
            {
                mv_camera->AddEnumItem(QObject::tr("<None>"), QString::number(static_cast<AZ::u64>(AZ::EntityId::InvalidEntityId)));
            }

            if (sequenceType == eSequenceType_Legacy)
            {
                // legacy sequences use legacy camera entities
                m_isLegacyCamera = true;

                // Search for all CCameraObject entities
                std::vector<CBaseObject*> objects;
                GetIEditor()->GetObjectManager()->GetObjects(objects);
                for (int i = 0; i < objects.size(); ++i)
                {
                    if (qobject_cast<CCameraObject*>(objects[i]))
                    {
                        mv_camera->AddEnumItem(objects[i]->GetName(), objects[i]->GetName());
                    }
                }
            }
            else if (sequenceType == eSequenceType_SequenceComponent)
            {
                // new sequences use camera components
                m_isLegacyCamera = false;

                // Find all Component Entity Cameras
                AZ::EBusAggregateResults<AZ::EntityId> cameraComponentEntities;
                Camera::CameraBus::BroadcastResult(cameraComponentEntities, &Camera::CameraRequests::GetCameras);

                // add names of all found entities with Camera Components
                for (int i = 0; i < cameraComponentEntities.values.size(); i++)
                {
                    AZ::Entity* entity = nullptr;
                    AZ::ComponentApplicationBus::BroadcastResult(entity, &AZ::ComponentApplicationBus::Events::FindEntity, cameraComponentEntities.values[i]);
                    if (entity)
                    {
                        // For Camera Components the enum value is the stringified AZ::EntityId of the entity with the Camera Component
                        QString entityIdString = QString::number(static_cast<AZ::u64>(entity->GetId()));
                        mv_camera->AddEnumItem(entity->GetName().c_str(), entityIdString);
                    }
                }
            }

            ISelectKey selectKey;
            keyHandle.GetKey(&selectKey);

            if (sequenceType == eSequenceType_Legacy)
            {
                mv_camera = selectKey.szSelection.c_str();
            }
            else if (sequenceType == eSequenceType_SequenceComponent)
            {
                mv_camera = QString::number(static_cast<AZ::u64>(selectKey.cameraAzEntityId));
            }
            mv_BlendTime.GetVar()->SetLimits(0.0f, selectKey.fDuration > .0f ? selectKey.fDuration : 1.0f, 0.1f, true, false);
            mv_BlendTime = selectKey.fBlendTime;

            bAssigned = true;        
        }
    }
    return bAssigned;
}

// Called when UI variable changes.
void CSelectKeyUIControls::OnUIChange(IVariable* pVar, CTrackViewKeyBundle& selectedKeys)
{
    CTrackViewSequence* pSequence = GetIEditor()->GetAnimation()->GetSequence();

    if (!pSequence || !selectedKeys.AreAllKeysOfSameType())
    {
        return;
    }

    for (unsigned int keyIndex = 0; keyIndex < selectedKeys.GetKeyCount(); ++keyIndex)
    {
        CTrackViewKeyHandle keyHandle = selectedKeys.GetKey(keyIndex);

        EAnimValue valueType = keyHandle.GetTrack()->GetValueType();
        if (valueType == eAnimValue_Select)
        {
            ISelectKey selectKey;
            keyHandle.GetKey(&selectKey);

            if (pVar == mv_camera.GetVar())
            {
                if (m_isLegacyCamera)
                {
                    selectKey.szSelection = ((QString)mv_camera).toLatin1().data();
                }
                else
                {
                    QString entityIdString = mv_camera;
                    selectKey.cameraAzEntityId = AZ::EntityId(entityIdString.toULongLong());
                    selectKey.szSelection =  mv_camera.GetVar()->GetDisplayValue().toLatin1().data();
                }
            }

            if (pVar == mv_BlendTime.GetVar())
            {
                if (mv_BlendTime < 0.0f)
                {
                    mv_BlendTime = 0.0f;
                }

                selectKey.fBlendTime = mv_BlendTime;
            }

            if (!selectKey.szSelection.empty())
            {
                IAnimSequence* pSequence = GetIEditor()->GetSystem()->GetIMovieSystem()->FindSequence(selectKey.szSelection.c_str());
                if (pSequence)
                {
                    selectKey.fDuration = pSequence->GetTimeRange().Length();
                }
            }

            CUndo::Record(new CUndoTrackObject(keyHandle.GetTrack()));
            keyHandle.SetKey(&selectKey);
        }
    }
}

REGISTER_QT_CLASS_DESC(CSelectKeyUIControls, "TrackView.KeyUI.Select", "TrackViewKeyUI");
