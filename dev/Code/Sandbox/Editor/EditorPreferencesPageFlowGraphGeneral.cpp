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
#include "StdAfx.h"
#include "EditorPreferencesPageFlowGraphGeneral.h"
#include <AzCore/Serialization/EditContext.h>

void CEditorPreferencesPage_FlowGraphGeneral::Reflect(AZ::SerializeContext& serialize)
{
    serialize.Class<ExpertOptions>()
        ->Version(1)
        ->Field("DefaultPreviewFile", &ExpertOptions::m_enableMigration)
        ->Field("DefaultPreviewFile", &ExpertOptions::m_showNodeIDs)
        ->Field("DefaultPreviewFile", &ExpertOptions::m_showToolTip)
        ->Field("DefaultPreviewFile", &ExpertOptions::m_edgesOnTopOfNodes)
        ->Field("DefaultPreviewFile", &ExpertOptions::m_highlightEdges);

    serialize.Class<CEditorPreferencesPage_FlowGraphGeneral>()
        ->Version(1)
        ->Field("ExpertOptions", &CEditorPreferencesPage_FlowGraphGeneral::m_expertOptions);


    AZ::EditContext* editContext = serialize.GetEditContext();
    if (editContext)
    {
        editContext->Class<ExpertOptions>("Expert Options", "")
            ->DataElement(AZ::Edit::UIHandlers::CheckBox, &ExpertOptions::m_enableMigration, "Automatic Migration", "Automatic Migration")
            ->DataElement(AZ::Edit::UIHandlers::CheckBox, &ExpertOptions::m_showNodeIDs, "Show NodeIDs", "Show NodeIDs")
            ->DataElement(AZ::Edit::UIHandlers::CheckBox, &ExpertOptions::m_showToolTip, "Show ToolTip", "Show ToolTip")
            ->DataElement(AZ::Edit::UIHandlers::CheckBox, &ExpertOptions::m_edgesOnTopOfNodes, "Edges On Top of Nodes", "Edges on Top of Nodes")
            ->DataElement(AZ::Edit::UIHandlers::CheckBox, &ExpertOptions::m_highlightEdges, "Highlight Edges of Selected Nodes", "Highling incoming/outgoing edges for selected nodes");

        editContext->Class<CEditorPreferencesPage_FlowGraphGeneral>("Flow Graph Expert Preferences", "Flow Graph Expert Preferences")
            ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
            ->Attribute(AZ::Edit::Attributes::Visibility, AZ_CRC("PropertyVisibility_ShowChildrenOnly", 0xef428f20))
            ->DataElement(AZ::Edit::UIHandlers::Default, &CEditorPreferencesPage_FlowGraphGeneral::m_expertOptions, "Expert", "Flow Graph Expert Options");
    }
}


CEditorPreferencesPage_FlowGraphGeneral::CEditorPreferencesPage_FlowGraphGeneral()
{
    InitializeSettings();
}

void CEditorPreferencesPage_FlowGraphGeneral::OnApply()
{
    gSettings.bFlowGraphMigrationEnabled = m_expertOptions.m_enableMigration;
    gSettings.bFlowGraphShowNodeIDs = m_expertOptions.m_showNodeIDs;
    gSettings.bFlowGraphShowToolTip = m_expertOptions.m_showToolTip;
    gSettings.bFlowGraphEdgesOnTopOfNodes = m_expertOptions.m_edgesOnTopOfNodes;
    gSettings.bFlowGraphHighlightEdges = m_expertOptions.m_highlightEdges;
}

void CEditorPreferencesPage_FlowGraphGeneral::InitializeSettings()
{
    m_expertOptions.m_enableMigration = gSettings.bFlowGraphMigrationEnabled;
    m_expertOptions.m_showNodeIDs = gSettings.bFlowGraphShowNodeIDs;
    m_expertOptions.m_showToolTip = gSettings.bFlowGraphShowToolTip;
    m_expertOptions.m_edgesOnTopOfNodes = gSettings.bFlowGraphEdgesOnTopOfNodes;
    m_expertOptions.m_highlightEdges = gSettings.bFlowGraphHighlightEdges;
}
