/*
* All or Portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/
#include <qevent.h>

#include <AzCore/Component/Entity.h>

#include <GraphCanvas/Widgets/GraphCanvasEditor/GraphCanvasEditorDockWidget.h>
#include <StaticLib/GraphCanvas/Widgets/GraphCanvasEditor/ui_GraphCanvasEditorDockWidget.h>

namespace GraphCanvas
{
    /////////////////////
    // EditorDockWidget
    /////////////////////
    
    EditorDockWidget::EditorDockWidget(const EditorId& editorId, QWidget* parent)
        : QDockWidget(parent)
        , m_editorId(editorId)
        , m_ui(new Ui::GraphCanvasEditorDockWidget())        
    {
        m_ui->setupUi(this);
        
        m_dockWidgetId = AZ::Entity::MakeId();

        SetAssetId(AZ::EntityId(1234));
    }
    
    EditorDockWidget::~EditorDockWidget()
    {
    }
    
    const DockWidgetId& EditorDockWidget::GetDockWidgetId() const
    {
        return m_dockWidgetId;
    }
    
    const AZ::Crc32& EditorDockWidget::GetEditorId() const
    {
        return m_editorId;
    }
    
    AZ::EntityId EditorDockWidget::GetAssetId() const
    {
        return m_assetId;
    }   
    
    void EditorDockWidget::SetAssetId(const AZ::EntityId& assetId)
    {
        m_assetId = assetId;
    }

#include <StaticLib/GraphCanvas/Widgets/GraphCanvasEditor/GraphCanvasEditorDockWidget.moc>
}