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

#include "EditorCommon.h"
#include <AzToolsFramework/ToolsComponents/ScriptEditorComponent.h>

#define UICANVASEDITOR_PROPERTIES_UPDATE_DELAY_IN_MILLISECONDS  (100)

PropertiesWidget::PropertiesWidget(EditorWindow* editorWindow,
    QWidget* parent)
    : QWidget(parent)
    , AzToolsFramework::IPropertyEditorNotify()
    , m_editorWindow(editorWindow)
    , m_refreshTimer(this)
    , m_preValueChanges()
    , m_propertiesContainer(new PropertiesContainer(this, m_editorWindow))
{
    // PropertiesContainer.
    {
        QVBoxLayout* vbLayout = new QVBoxLayout();
        setLayout(vbLayout);

        vbLayout->setContentsMargins(0, 0, 0, 0);
        vbLayout->setSpacing(0);

        vbLayout->addWidget(m_propertiesContainer);
    }

    // Refresh timer.
    {
        QObject::connect(&m_refreshTimer,
            &QTimer::timeout,
            this,
            [ this ]()
            {
                Refresh(m_refreshLevel, (!m_componentTypeToRefresh.IsNull() ? &m_componentTypeToRefresh : nullptr));
            });

        m_refreshTimer.setInterval(UICANVASEDITOR_PROPERTIES_UPDATE_DELAY_IN_MILLISECONDS);

        m_refreshTimer.setSingleShot(true);
    }

    setMinimumWidth(250);

    ToolsApplicationEvents::Bus::Handler::BusConnect();
}

PropertiesWidget::~PropertiesWidget()
{
    ToolsApplicationEvents::Bus::Handler::BusDisconnect();
}

void PropertiesWidget::UserSelectionChanged(HierarchyItemRawPtrList* items)
{
    // Tell the properties container that the selection has changed but don't actually update
    // it. The refresh will do that.
    m_propertiesContainer->SelectionChanged(items);

    TriggerRefresh();
}

void PropertiesWidget::TriggerRefresh(AzToolsFramework::PropertyModificationRefreshLevel refreshLevel, const AZ::Uuid* componentType)
{
    if (!m_refreshTimer.isActive())
    {
        m_componentTypeToRefresh = componentType ? *componentType : AZ::Uuid::CreateNull();
        m_refreshLevel = refreshLevel;

        m_refreshTimer.start();
    }
    else
    {
        // Set component type to null if there are multiple component types due to refresh
        if (!componentType || m_componentTypeToRefresh != *componentType)
        {
            m_componentTypeToRefresh = AZ::Uuid::CreateNull();
        }

        if ((int)refreshLevel > m_refreshLevel)
        {
            m_refreshLevel = refreshLevel;
        }
    }
}

void PropertiesWidget::TriggerImmediateRefresh(AzToolsFramework::PropertyModificationRefreshLevel refreshLevel, const AZ::Uuid* componentType)
{
    TriggerRefresh(refreshLevel, componentType);

    m_refreshTimer.stop();

    Refresh(m_refreshLevel, (!m_componentTypeToRefresh.IsNull() ? &m_componentTypeToRefresh : nullptr));
}

void PropertiesWidget::SelectedEntityPointersChanged()
{
    m_propertiesContainer->SelectedEntityPointersChanged();
}

void PropertiesWidget::SetSelectedEntityDisplayNameWidget(QLabel * selectedEntityDisplayNameWidget)
{
    m_propertiesContainer->SetSelectedEntityDisplayNameWidget(selectedEntityDisplayNameWidget);
}

float PropertiesWidget::GetScrollValue()
{
    return m_propertiesContainer->verticalScrollBar() ? m_propertiesContainer->verticalScrollBar()->value() : 0.0f;
}

void PropertiesWidget::SetScrollValue(float scrollValue)
{
    if (m_propertiesContainer->verticalScrollBar())
    {
        m_propertiesContainer->verticalScrollBar()->setValue(scrollValue);
    }
}

void PropertiesWidget::Refresh(AzToolsFramework::PropertyModificationRefreshLevel refreshLevel, const AZ::Uuid* componentType)
{
    m_propertiesContainer->Refresh(refreshLevel, componentType);
}

void PropertiesWidget::BeforePropertyModified(AzToolsFramework::InstanceDataNode* pNode)
{
    if (m_propertiesContainer->IsCanvasSelected())
    {
        m_canvasUndoXml.clear();
        EBUS_EVENT_ID_RESULT(m_canvasUndoXml, m_editorWindow->GetCanvas(), UiCanvasBus, SaveToXmlString);
        AZ_Assert(!m_canvasUndoXml.empty(), "Failed to serialize");
    }
    else
    {
        // This is used to save the "before" undo data.
        HierarchyClipboard::Serialize(m_editorWindow->GetHierarchy(),
            m_editorWindow->GetHierarchy()->selectedItems(),
            nullptr,
            m_preValueChanges,
            true);
    }
}

void PropertiesWidget::AfterPropertyModified(AzToolsFramework::InstanceDataNode* pNode)
{
    if (m_propertiesContainer->IsCanvasSelected())
    {
        AZStd::string canvasRedoXml;
        EBUS_EVENT_ID_RESULT(canvasRedoXml, m_editorWindow->GetCanvas(), UiCanvasBus, SaveToXmlString);
        AZ_Assert(!canvasRedoXml.empty(), "Failed to serialize");

        CommandCanvasPropertiesChange::Push(m_editorWindow->GetActiveStack(), m_canvasUndoXml, canvasRedoXml, m_editorWindow);
    }
    else
    {
        // This is used to save the "after" undo data.
        CommandPropertiesChange::Push(m_editorWindow->GetActiveStack(),
            m_editorWindow->GetHierarchy(),
            m_preValueChanges);

        // We've consumed m_preValueChanges.
        m_preValueChanges.clear();

        // Notify other systems (e.g. Animation) for each UI entity that changed
        LyShine::EntityArray selectedElements = SelectionHelpers::GetTopLevelSelectedElements(
                m_editorWindow->GetHierarchy(), m_editorWindow->GetHierarchy()->selectedItems());
        for (auto element : selectedElements)
        {
            EBUS_EVENT_ID(element->GetId(), UiElementChangeNotificationBus, UiElementPropertyChanged);
        }
    }

    // trigger the viewport window to refresh (it may be a long delay if it waits for an editor idle message)
    m_editorWindow->GetViewport()->Refresh();
}

void PropertiesWidget::SetPropertyEditingActive(AzToolsFramework::InstanceDataNode* pNode)
{
}

void PropertiesWidget::SetPropertyEditingComplete(AzToolsFramework::InstanceDataNode* pNode)
{
}

void PropertiesWidget::SealUndoStack()
{
}

void PropertiesWidget::RequestPropertyContextMenu(AzToolsFramework::InstanceDataNode* node, const QPoint& globalPos)
{
    m_propertiesContainer->RequestPropertyContextMenu(node, globalPos);
}

void PropertiesWidget::InvalidatePropertyDisplay(AzToolsFramework::PropertyModificationRefreshLevel level)
{
    // This event is sent when the main editor's properties pane should refresh. We only care about script changes triggering
    // this event. In this case we want the UI Editor's properties pane to refresh and display any new script properties
    TriggerRefresh(AzToolsFramework::PropertyModificationRefreshLevel::Refresh_EntireTree, &AZ::AzTypeInfo<AzToolsFramework::Components::ScriptEditorComponent>::Uuid());
}

#include <PropertiesWidget.moc>
