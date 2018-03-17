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
#include <AzCore/Component/EntityBus.h>
#include <AzFramework/Components/CameraBus.h>
#include <AzCore/Component/Entity.h>
#include <AzToolsFramework/ToolsComponents/GenericComponentWrapper.h>
#include <AzToolsFramework/Entity/EditorEntityContextComponent.h>
#include <AzToolsFramework/Metrics/LyEditorMetricsBus.h>

#include "TrackViewKeyPropertiesDlg.h"
#include "TrackViewTrack.h"
#include "TrackViewUndo.h"
#include <Maestro/Types/AnimValueType.h>
#include <Maestro/Types/SequenceType.h>
#include <Maestro/Types/SequenceType.h>

#include "objects/CameraObject.h"

#include <QObject>

//////////////////////////////////////////////////////////////////////////
class CSelectKeyUIControls
    : public CTrackViewKeyUIControls
    , protected Camera::CameraNotificationBus::Handler
    , protected AzToolsFramework::EditorMetricsEventsBus::Handler
    , protected AZ::EntitySystemBus::Handler
{
public:
    CSelectKeyUIControls()
        : m_isLegacyCamera(true) {}

    ~CSelectKeyUIControls() override;

    CSmartVariableArray mv_table;
    CSmartVariableEnum<QString> mv_camera;
    CSmartVariable<float> mv_BlendTime;

    virtual void OnCreateVars()
    {
        AddVariable(mv_table, "Key Properties");
        AddVariable(mv_table, mv_camera, "Camera");
        AddVariable(mv_table, mv_BlendTime, "Blend time");

        Camera::CameraNotificationBus::Handler::BusConnect();
        AzToolsFramework::EditorMetricsEventsBus::Handler::BusConnect();
        AZ::EntitySystemBus::Handler::BusConnect();
    }
    bool SupportTrackType(const CAnimParamType& paramType, EAnimCurveType trackType, AnimValueType valueType) const
    {
        return valueType == AnimValueType::Select;
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

protected:
    ////////////////////////////////////////////////////////////////////////
    // CameraNotificationBus interface implementation
    void OnCameraAdded(const AZ::EntityId& cameraId) override;
    void OnCameraRemoved(const AZ::EntityId& cameraId) override;

    ////////////////////////////////////////////////////////////////////////
    // LyEditorMetricsRequestBus interface implementation
    void LegacyEntityCreated(const char* entityType, const char* scriptEntityType) override;

    //////////////////////////////////////////////////////////////////////////
    // AZ::EntitySystemBus::Handler
    void OnEntityNameChanged(const AZ::EntityId& entityId, const AZStd::string& name) override;

private:
    bool                        m_isLegacyCamera;

    void ResetCameraEntries();
    SequenceType GetSequenceType() const;
};

CSelectKeyUIControls::~CSelectKeyUIControls()
{
    AZ::EntitySystemBus::Handler::BusDisconnect();
    Camera::CameraNotificationBus::Handler::BusDisconnect();
    AzToolsFramework::EditorMetricsEventsBus::Handler::BusDisconnect();
}

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

        AnimValueType valueType = keyHandle.GetTrack()->GetValueType();
        if (valueType == AnimValueType::Select)
        {
            ResetCameraEntries();

            // Get All cameras.
            CTrackViewSequence* sequence = GetIEditor()->GetAnimation()->GetSequence();
            SequenceType sequenceType = sequence ? sequence->GetSequenceType() : SequenceType::Legacy;

            mv_camera.SetEnumList(NULL);

            // Insert '<None>' empty enum
            if (sequenceType == SequenceType::Legacy)
            {
                mv_camera->AddEnumItem(QObject::tr("<None>"), "");
            }
            else if (sequenceType == SequenceType::SequenceComponent)
            {
                mv_camera->AddEnumItem(QObject::tr("<None>"), QString::number(static_cast<AZ::u64>(AZ::EntityId::InvalidEntityId)));
            }

            if (sequenceType == SequenceType::Legacy)
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
            else if (sequenceType == SequenceType::SequenceComponent)
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

            if (sequenceType == SequenceType::Legacy)
            {
                mv_camera = selectKey.szSelection.c_str();
            }
            else if (sequenceType == SequenceType::SequenceComponent)
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
    CTrackViewSequence* sequence = GetIEditor()->GetAnimation()->GetSequence();

    if (!sequence || !selectedKeys.AreAllKeysOfSameType())
    {
        return;
    }

    for (unsigned int keyIndex = 0; keyIndex < selectedKeys.GetKeyCount(); ++keyIndex)
    {
        CTrackViewKeyHandle keyHandle = selectedKeys.GetKey(keyIndex);

        AnimValueType valueType = keyHandle.GetTrack()->GetValueType();
        if (valueType == AnimValueType::Select)
        {
            ISelectKey selectKey;
            keyHandle.GetKey(&selectKey);

            if (pVar == mv_camera.GetVar())
            {
                if (m_isLegacyCamera)
                {
                    selectKey.szSelection = ((QString)mv_camera).toUtf8().data();
                }
                else
                {
                    QString entityIdString = mv_camera;
                    selectKey.cameraAzEntityId = AZ::EntityId(entityIdString.toULongLong());
                    selectKey.szSelection =  mv_camera.GetVar()->GetDisplayValue().toUtf8().data();
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
                IAnimSequence* pSequence = GetIEditor()->GetSystem()->GetIMovieSystem()->FindLegacySequenceByName(selectKey.szSelection.c_str());
                if (pSequence)
                {
                    selectKey.fDuration = pSequence->GetTimeRange().Length();
                }
            }

            bool isDuringUndo = false;
            AzToolsFramework::ToolsApplicationRequests::Bus::BroadcastResult(isDuringUndo, &AzToolsFramework::ToolsApplicationRequests::Bus::Events::IsDuringUndoRedo);

            if (sequence->GetSequenceType() == SequenceType::Legacy)
            {
                CUndo::Record(new CUndoTrackObject(keyHandle.GetTrack()));
                keyHandle.SetKey(&selectKey);
            }
            else if (isDuringUndo)
            {
                keyHandle.SetKey(&selectKey);
            }
            else
            {
                AzToolsFramework::ScopedUndoBatch undoBatch("Set Key Value");
                keyHandle.SetKey(&selectKey);
                undoBatch.MarkEntityDirty(sequence->GetSequenceComponentEntityId());
            }

        }
    }
}

void CSelectKeyUIControls::OnCameraAdded(const AZ::EntityId & cameraId)
{
    if (GetSequenceType() != SequenceType::SequenceComponent)
    {
        return;
    }

    // Add a single camera component
    AZ::Entity* entity = nullptr;
    AZ::ComponentApplicationBus::BroadcastResult(entity, &AZ::ComponentApplicationBus::Events::FindEntity, cameraId);
    if (entity)
    {
        // For Camera Components the enum value is the stringified AZ::EntityId of the entity with the Camera Component
        QString entityIdString = QString::number(static_cast<AZ::u64>(entity->GetId()));
        mv_camera->AddEnumItem(entity->GetName().c_str(), entityIdString);
    }
}

void CSelectKeyUIControls::OnCameraRemoved(const AZ::EntityId & cameraId)
{
    if (GetSequenceType() != SequenceType::SequenceComponent)
    {
        return;
    }

    mv_camera->EnableUpdateCallbacks(false);

    // We can't iterate or remove an item from the enum list, and Camera::CameraRequests::GetCameras
    // still includes the deleted camera at this point. Reset the list anyway and filter out the
    // deleted camera.
    IVarEnumList* oldList = mv_camera->GetEnumList();
    mv_camera->SetEnumList(NULL);
    mv_camera->AddEnumItem(QObject::tr("<None>"), QString::number(static_cast<AZ::u64>(AZ::EntityId::InvalidEntityId)));

    AZ::EBusAggregateResults<AZ::EntityId> cameraComponentEntities;
    Camera::CameraBus::BroadcastResult(cameraComponentEntities, &Camera::CameraRequests::GetCameras);
    for (int i = 0; i < cameraComponentEntities.values.size(); i++)
    {
        if (cameraId == cameraComponentEntities.values[i])
        {
            continue;
        }
        OnCameraAdded(cameraComponentEntities.values[i]);
    }

    mv_camera->EnableUpdateCallbacks(true);
}

void CSelectKeyUIControls::LegacyEntityCreated(const char * entityType, const char * scriptEntityType)
{
    if (GetSequenceType() != SequenceType::Legacy)
    {
        return;
    }

    if (strcmp(entityType, "Camera") != 0 || strcmp(scriptEntityType, "CameraSource") != 0)
    {
        return;
    }

    mv_camera->EnableUpdateCallbacks(false);
    ResetCameraEntries();
    mv_camera->EnableUpdateCallbacks(true);
}

void CSelectKeyUIControls::OnEntityNameChanged(const AZ::EntityId & entityId, const AZStd::string & name)
{
    if (GetSequenceType() != SequenceType::SequenceComponent)
    {
        return;
    }

    AZ::Entity* entity = nullptr;
    AZ::ComponentApplicationBus::BroadcastResult(entity, &AZ::ComponentApplicationBus::Events::FindEntity, entityId);
    if (entity == nullptr)
    {
        return;
    }

    AZ::Entity::ComponentArrayType cameraComponents = entity->FindComponents(EditorCameraComponentTypeId);
    if (cameraComponents.size() == 0)
    {
        return;
    }

    mv_camera->EnableUpdateCallbacks(false);
    ResetCameraEntries();
    mv_camera->EnableUpdateCallbacks(true);
}

void CSelectKeyUIControls::ResetCameraEntries()
{
    // Get All cameras.
    const SequenceType sequenceType = GetSequenceType();

    mv_camera.SetEnumList(NULL);

    // Insert '<None>' empty enum
    if (sequenceType == SequenceType::Legacy)
    {
        mv_camera->AddEnumItem(QObject::tr("<None>"), "");
    }
    else if (sequenceType == SequenceType::SequenceComponent)
    {
        mv_camera->AddEnumItem(QObject::tr("<None>"), QString::number(static_cast<AZ::u64>(AZ::EntityId::InvalidEntityId)));
    }

    if (sequenceType == SequenceType::Legacy)
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
    else if (sequenceType == SequenceType::SequenceComponent)
    {
        // new sequences use camera components
        m_isLegacyCamera = false;

        // Find all Component Entity Cameras
        AZ::EBusAggregateResults<AZ::EntityId> cameraComponentEntities;
        Camera::CameraBus::BroadcastResult(cameraComponentEntities, &Camera::CameraRequests::GetCameras);

        // add names of all found entities with Camera Components
        for (int i = 0; i < cameraComponentEntities.values.size(); i++)
        {
            OnCameraAdded(cameraComponentEntities.values[i]);
        }
    }
}

SequenceType CSelectKeyUIControls::GetSequenceType() const
{
    CTrackViewSequence* sequence = GetIEditor()->GetAnimation()->GetSequence();
    return sequence ? sequence->GetSequenceType() : SequenceType::Legacy;
}

REGISTER_QT_CLASS_DESC(CSelectKeyUIControls, "TrackView.KeyUI.Select", "TrackViewKeyUI");
