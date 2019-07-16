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

#include "stdafx.h"
#include "UI/QComponentLevelEntityEditorMainWindow.h"

#include <AzCore/Component/Entity.h>
#include <AzCore/Component/ComponentApplicationBus.h>

#include <AzToolsFramework/API/ToolsApplicationAPI.h>
#include <AzToolsFramework/UI/PropertyEditor/ReflectedPropertyEditor.hxx>
#include <AzToolsFramework/UI/PropertyEditor/EntityPropertyEditor.hxx>
#include <AzToolsFramework/Entity/EditorEntityContextBus.h>

QComponentLevelEntityEditorInspectorWindow::QComponentLevelEntityEditorInspectorWindow(QWidget* parent)
    : QMainWindow(parent)
    , m_propertyEditor(nullptr)
{
    AzToolsFramework::SliceMetadataEntityContextNotificationBus::Handler::BusConnect();

    Init();
}

QComponentLevelEntityEditorInspectorWindow::~QComponentLevelEntityEditorInspectorWindow()
{
    AzToolsFramework::SliceMetadataEntityContextNotificationBus::Handler::BusDisconnect();
}

void QComponentLevelEntityEditorInspectorWindow::Init()
{
    QVBoxLayout* layout = new QVBoxLayout();

    m_propertyEditor = new AzToolsFramework::EntityPropertyEditor(nullptr, 0, true);
    layout->addWidget(m_propertyEditor);

    SetRootMetadataEntity();

    QWidget* window = new QWidget();
    window->setLayout(layout);
    setCentralWidget(window);
}

void QComponentLevelEntityEditorInspectorWindow::OnMetadataEntityAdded(AZ::EntityId entityId)
{
    AZ::EntityId rootSliceMetaDataEntity = GetRootMetaDataEntityId();
    if (rootSliceMetaDataEntity == entityId)
    {
        AzToolsFramework::EntityIdList entities;
        entities.push_back(rootSliceMetaDataEntity);
        m_propertyEditor->SetOverrideEntityIds(entities);
    }
}

void QComponentLevelEntityEditorInspectorWindow::SetRootMetadataEntity()
{
    AZ::EntityId rootSliceMetaDataEntity = GetRootMetaDataEntityId();

    if (rootSliceMetaDataEntity.IsValid())
    {
        AzToolsFramework::EntityIdList entities;
        entities.push_back(rootSliceMetaDataEntity);
        m_propertyEditor->SetOverrideEntityIds(entities);
    }
}

AZ::EntityId QComponentLevelEntityEditorInspectorWindow::GetRootMetaDataEntityId() const
{
    AzFramework::EntityContextId editorEntityContextId = AzFramework::EntityContextId::CreateNull();
    AzToolsFramework::EditorEntityContextRequestBus::BroadcastResult(editorEntityContextId, &AzToolsFramework::EditorEntityContextRequestBus::Events::GetEditorEntityContextId);
    AZ::SliceComponent* rootSliceComponent = nullptr;
    AzFramework::EntityContextRequestBus::EventResult(rootSliceComponent, editorEntityContextId, &AzFramework::EntityContextRequestBus::Events::GetRootSlice);
    if (rootSliceComponent && rootSliceComponent->GetMetadataEntity())
    {
        return rootSliceComponent->GetMetadataEntity()->GetId();
    }

    return AZ::EntityId();
}


///////////////////////////////////////////////////////////////////////////////
// End of context menu handling
///////////////////////////////////////////////////////////////////////////////

#include <UI/QComponentLevelEntityEditorMainWindow.moc>

