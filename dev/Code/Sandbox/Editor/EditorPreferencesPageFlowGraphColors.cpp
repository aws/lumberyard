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
#include "EditorPreferencesPageFlowGraphColors.h"
#include <AzCore/Serialization/EditContext.h>

void CEditorPreferencesPage_FlowGraphColors::Reflect(AZ::SerializeContext& serialize)
{
    serialize.Class<CEditorPreferencesPage_FlowGraphColors>()
        ->Version(1)
        ->Field("Opacity", &CEditorPreferencesPage_FlowGraphColors::m_opacity)
        ->Field("ColorArrow", &CEditorPreferencesPage_FlowGraphColors::m_colorArrow)
        ->Field("ColorInArrowHighlighted", &CEditorPreferencesPage_FlowGraphColors::m_colorInArrowHighlighted)
        ->Field("ColorOutArrowHighlighted", &CEditorPreferencesPage_FlowGraphColors::m_colorOutArrowHighlighted)
        ->Field("ColorPortEdgeHighlighted", &CEditorPreferencesPage_FlowGraphColors::m_colorPortEdgeHighlighted)
        ->Field("ColorArrowDisabled", &CEditorPreferencesPage_FlowGraphColors::m_colorArrowDisabled)
        ->Field("ColorNodeOutline", &CEditorPreferencesPage_FlowGraphColors::m_colorNodeOutline)
        ->Field("ColorNodeOutlineSelected", &CEditorPreferencesPage_FlowGraphColors::m_colorNodeOutlineSelected)
        ->Field("ColorNodeBkg", &CEditorPreferencesPage_FlowGraphColors::m_colorNodeBkg)
        ->Field("ColorCustomNodeBkg", &CEditorPreferencesPage_FlowGraphColors::m_colorCustomNodeBkg)
        ->Field("ColorCustomSelectedNodeBkg", &CEditorPreferencesPage_FlowGraphColors::m_colorCustomSelectedNodeBkg)
        ->Field("ColorNodeSelected", &CEditorPreferencesPage_FlowGraphColors::m_colorNodeSelected)
        ->Field("ColorTitleText", &CEditorPreferencesPage_FlowGraphColors::m_colorTitleText)
        ->Field("ColorTitleTextSelected", &CEditorPreferencesPage_FlowGraphColors::m_colorTitleTextSelected)
        ->Field("ColorText", &CEditorPreferencesPage_FlowGraphColors::m_colorText)
        ->Field("ColorBackground", &CEditorPreferencesPage_FlowGraphColors::m_colorBackground)
        ->Field("ColorGrid", &CEditorPreferencesPage_FlowGraphColors::m_colorGrid)
        ->Field("ColorBreakPoint", &CEditorPreferencesPage_FlowGraphColors::m_colorBreakPoint)
        ->Field("ColorBreakPointDisabled", &CEditorPreferencesPage_FlowGraphColors::m_colorBreakPointDisabled)
        ->Field("ColorBreakPointArrow", &CEditorPreferencesPage_FlowGraphColors::m_colorBreakPointArrow)
        ->Field("ColorEntityPortNotConnected", &CEditorPreferencesPage_FlowGraphColors::m_colorEntityPortNotConnected)
        ->Field("ColorPort", &CEditorPreferencesPage_FlowGraphColors::m_colorPort)
        ->Field("ColorPortSelected", &CEditorPreferencesPage_FlowGraphColors::m_colorPortSelected)
        ->Field("ColorEntityTextInvalid", &CEditorPreferencesPage_FlowGraphColors::m_colorEntityTextInvalid)
        ->Field("ColorDownArrow", &CEditorPreferencesPage_FlowGraphColors::m_colorDownArrow)
        ->Field("ColorPortDebugging", &CEditorPreferencesPage_FlowGraphColors::m_colorPortDebugging)
        ->Field("ColorPortDebuggingText", &CEditorPreferencesPage_FlowGraphColors::m_colorPortDebuggingText)
        ->Field("ColorQuickSearchBackground", &CEditorPreferencesPage_FlowGraphColors::m_colorQuickSearchBackground)
        ->Field("ColorQuickSearchResultText", &CEditorPreferencesPage_FlowGraphColors::m_colorQuickSearchResultText)
        ->Field("ColorQuickSearchCountText", &CEditorPreferencesPage_FlowGraphColors::m_colorQuickSearchCountText)
        ->Field("ColorQuickSearchBorder", &CEditorPreferencesPage_FlowGraphColors::m_colorQuickSearchBorder)
        ->Field("ColorDebugNodeTitle", &CEditorPreferencesPage_FlowGraphColors::m_colorDebugNodeTitle)
        ->Field("ColorDebugNode", &CEditorPreferencesPage_FlowGraphColors::m_colorDebugNode);


    AZ::EditContext* editContext = serialize.GetEditContext();
    if (editContext)
    {
        editContext->Class<CEditorPreferencesPage_FlowGraphColors>("Flow Graph Expert Preferences", "Flow Graph Expert Preferences")
            ->DataElement(AZ::Edit::UIHandlers::SpinBox, &CEditorPreferencesPage_FlowGraphColors::m_opacity, "Opacity", "Opacity")
            ->DataElement(AZ::Edit::UIHandlers::Color, &CEditorPreferencesPage_FlowGraphColors::m_colorArrow, "Arrow Color", "Arrow Color")
            ->DataElement(AZ::Edit::UIHandlers::Color, &CEditorPreferencesPage_FlowGraphColors::m_colorInArrowHighlighted, "InArrow Highlighted Color", "InArrow Highlighted Color")
            ->DataElement(AZ::Edit::UIHandlers::Color, &CEditorPreferencesPage_FlowGraphColors::m_colorOutArrowHighlighted, "OutArrow Highlighted Color", "OutArrow Highlighted Color")
            ->DataElement(AZ::Edit::UIHandlers::Color, &CEditorPreferencesPage_FlowGraphColors::m_colorPortEdgeHighlighted, "Port Edge Highlighted Color", "Port Edge Highlighted Color")
            ->DataElement(AZ::Edit::UIHandlers::Color, &CEditorPreferencesPage_FlowGraphColors::m_colorArrowDisabled, "Arrow Disabled Color", "Arrow Disabled Color")
            ->DataElement(AZ::Edit::UIHandlers::Color, &CEditorPreferencesPage_FlowGraphColors::m_colorNodeOutline, "Node Outline Color", "Node Outline Color")
            ->DataElement(AZ::Edit::UIHandlers::Color, &CEditorPreferencesPage_FlowGraphColors::m_colorNodeOutlineSelected, "Node Outline Color Selected", "Node Outline Color Selected")
            ->DataElement(AZ::Edit::UIHandlers::Color, &CEditorPreferencesPage_FlowGraphColors::m_colorNodeBkg, "Node Background Color", "Node Background Color")
            ->DataElement(AZ::Edit::UIHandlers::Color, &CEditorPreferencesPage_FlowGraphColors::m_colorNodeSelected, "Node Selected Color", "Node Selected Color")
            ->DataElement(AZ::Edit::UIHandlers::Color, &CEditorPreferencesPage_FlowGraphColors::m_colorTitleText, "Title Text Color", "Title Text Color")
            ->DataElement(AZ::Edit::UIHandlers::Color, &CEditorPreferencesPage_FlowGraphColors::m_colorTitleTextSelected, "Title Text color Selected", "Title Text color Selected")
            ->DataElement(AZ::Edit::UIHandlers::Color, &CEditorPreferencesPage_FlowGraphColors::m_colorText, "Text Color", "Text Color")
            ->DataElement(AZ::Edit::UIHandlers::Color, &CEditorPreferencesPage_FlowGraphColors::m_colorBackground, "Background Color", "Background Color")
            ->DataElement(AZ::Edit::UIHandlers::Color, &CEditorPreferencesPage_FlowGraphColors::m_colorGrid, "Grid Color", "Grid Color")
            ->DataElement(AZ::Edit::UIHandlers::Color, &CEditorPreferencesPage_FlowGraphColors::m_colorBreakPoint, "Breakpoint Color", "Breakpoint Color")
            ->DataElement(AZ::Edit::UIHandlers::Color, &CEditorPreferencesPage_FlowGraphColors::m_colorBreakPointDisabled, "Breakpoint Disabled Color", "Breakpoint Disabled Color")
            ->DataElement(AZ::Edit::UIHandlers::Color, &CEditorPreferencesPage_FlowGraphColors::m_colorBreakPointArrow, "Breakpoint Arrow Color", "Breakpoint Arrow Color")
            ->DataElement(AZ::Edit::UIHandlers::Color, &CEditorPreferencesPage_FlowGraphColors::m_colorEntityPortNotConnected, "Entity Port Not Connected Color", "Entity Port Not Connected Color")
            ->DataElement(AZ::Edit::UIHandlers::Color, &CEditorPreferencesPage_FlowGraphColors::m_colorPort, "Port Color", "Port Color")
            ->DataElement(AZ::Edit::UIHandlers::Color, &CEditorPreferencesPage_FlowGraphColors::m_colorPortSelected, "Port Selected Color", "Port Selected Color")
            ->DataElement(AZ::Edit::UIHandlers::Color, &CEditorPreferencesPage_FlowGraphColors::m_colorEntityTextInvalid, "Entity Invalid Text Color", "Entity Invalid Text Color")
            ->DataElement(AZ::Edit::UIHandlers::Color, &CEditorPreferencesPage_FlowGraphColors::m_colorDownArrow, "Down Arrow Color", "Down Arrow Color")
            ->DataElement(AZ::Edit::UIHandlers::Color, &CEditorPreferencesPage_FlowGraphColors::m_colorCustomNodeBkg, "Custom Node Background Color", "Custom Node Background Color")
            ->DataElement(AZ::Edit::UIHandlers::Color, &CEditorPreferencesPage_FlowGraphColors::m_colorCustomSelectedNodeBkg, "Custom Node Selected Background", "Custom Node Selected Background")
            ->DataElement(AZ::Edit::UIHandlers::Color, &CEditorPreferencesPage_FlowGraphColors::m_colorPortDebugging, "Port Debugging", "Port Debugging")
            ->DataElement(AZ::Edit::UIHandlers::Color, &CEditorPreferencesPage_FlowGraphColors::m_colorPortDebuggingText, "Port Text Debugging", "Port Text Debugging")
            ->DataElement(AZ::Edit::UIHandlers::Color, &CEditorPreferencesPage_FlowGraphColors::m_colorQuickSearchBackground, "Quick Search Background", "Quick Search Background")
            ->DataElement(AZ::Edit::UIHandlers::Color, &CEditorPreferencesPage_FlowGraphColors::m_colorQuickSearchResultText, "Quick Search Result Text", "Quick Search Result Text")
            ->DataElement(AZ::Edit::UIHandlers::Color, &CEditorPreferencesPage_FlowGraphColors::m_colorQuickSearchCountText, "Quick Search Count Text", "Quick Search Count Text")
            ->DataElement(AZ::Edit::UIHandlers::Color, &CEditorPreferencesPage_FlowGraphColors::m_colorQuickSearchBorder, "Quick Search Border", "Quick Search Border")
            ->DataElement(AZ::Edit::UIHandlers::Color, &CEditorPreferencesPage_FlowGraphColors::m_colorDebugNodeTitle, "Debug Node Title", "Debug Node Title")
            ->DataElement(AZ::Edit::UIHandlers::Color, &CEditorPreferencesPage_FlowGraphColors::m_colorDebugNode, "Debug Node Background", "Debug Node Background");
    }
}


CEditorPreferencesPage_FlowGraphColors::CEditorPreferencesPage_FlowGraphColors()
{
    InitializeSettings();
}

void CEditorPreferencesPage_FlowGraphColors::OnApply()
{
    gSettings.hyperGraphColors.opacity = m_opacity;

    SetAZColorToQColor(m_colorArrow, gSettings.hyperGraphColors.colorArrow);

    SetAZColorToQColor(m_colorInArrowHighlighted, gSettings.hyperGraphColors.colorInArrowHighlighted);

    SetAZColorToQColor(m_colorOutArrowHighlighted, gSettings.hyperGraphColors.colorOutArrowHighlighted);

    SetAZColorToQColor(m_colorPortEdgeHighlighted, gSettings.hyperGraphColors.colorPortEdgeHighlighted);

    SetAZColorToQColor(m_colorArrowDisabled, gSettings.hyperGraphColors.colorArrowDisabled);

    SetAZColorToQColor(m_colorNodeOutline, gSettings.hyperGraphColors.colorNodeOutline);

    SetAZColorToQColor(m_colorNodeOutlineSelected, gSettings.hyperGraphColors.colorNodeOutlineSelected);

    SetAZColorToQColor(m_colorNodeOutlineSelected, gSettings.hyperGraphColors.colorNodeOutlineSelected);

    SetAZColorToQColor(m_colorNodeBkg, gSettings.hyperGraphColors.colorNodeBkg);

    SetAZColorToQColor(m_colorNodeSelected, gSettings.hyperGraphColors.colorNodeSelected);

    SetAZColorToQColor(m_colorTitleText, gSettings.hyperGraphColors.colorTitleText);

    SetAZColorToQColor(m_colorTitleTextSelected, gSettings.hyperGraphColors.colorTitleTextSelected);

    SetAZColorToQColor(m_colorText, gSettings.hyperGraphColors.colorText);

    SetAZColorToQColor(m_colorBackground, gSettings.hyperGraphColors.colorBackground);

    SetAZColorToQColor(m_colorGrid, gSettings.hyperGraphColors.colorGrid);

    SetAZColorToQColor(m_colorBreakPoint, gSettings.hyperGraphColors.colorBreakPoint);

    SetAZColorToQColor(m_colorBreakPointDisabled, gSettings.hyperGraphColors.colorBreakPointDisabled);

    SetAZColorToQColor(m_colorBreakPointArrow, gSettings.hyperGraphColors.colorBreakPointArrow);

    SetAZColorToQColor(m_colorEntityPortNotConnected, gSettings.hyperGraphColors.colorEntityPortNotConnected);

    SetAZColorToQColor(m_colorPort, gSettings.hyperGraphColors.colorPort);

    SetAZColorToQColor(m_colorPortSelected, gSettings.hyperGraphColors.colorPortSelected);

    SetAZColorToQColor(m_colorEntityTextInvalid, gSettings.hyperGraphColors.colorEntityTextInvalid);

    SetAZColorToQColor(m_colorDownArrow, gSettings.hyperGraphColors.colorDownArrow);

    SetAZColorToQColor(m_colorCustomNodeBkg, gSettings.hyperGraphColors.colorCustomNodeBkg);

    SetAZColorToQColor(m_colorCustomSelectedNodeBkg, gSettings.hyperGraphColors.colorCustomSelectedNodeBkg);

    SetAZColorToQColor(m_colorPortDebugging, gSettings.hyperGraphColors.colorPortDebugging);

    SetAZColorToQColor(m_colorPortDebuggingText, gSettings.hyperGraphColors.colorPortDebuggingText);

    SetAZColorToQColor(m_colorQuickSearchBackground, gSettings.hyperGraphColors.colorQuickSearchBackground);

    SetAZColorToQColor(m_colorQuickSearchResultText, gSettings.hyperGraphColors.colorQuickSearchResultText);

    SetAZColorToQColor(m_colorQuickSearchCountText, gSettings.hyperGraphColors.colorQuickSearchCountText);

    SetAZColorToQColor(m_colorQuickSearchBorder, gSettings.hyperGraphColors.colorQuickSearchBorder);

    SetAZColorToQColor(m_colorDebugNodeTitle, gSettings.hyperGraphColors.colorDebugNodeTitle);

    SetAZColorToQColor(m_colorDebugNode, gSettings.hyperGraphColors.colorDebugNode);
}

void CEditorPreferencesPage_FlowGraphColors::InitializeSettings()
{
    m_opacity = gSettings.hyperGraphColors.opacity;

    SetQColorToAZColor(gSettings.hyperGraphColors.colorArrow, m_colorArrow);

    SetQColorToAZColor(gSettings.hyperGraphColors.colorInArrowHighlighted, m_colorInArrowHighlighted);

    SetQColorToAZColor(gSettings.hyperGraphColors.colorOutArrowHighlighted, m_colorOutArrowHighlighted);

    SetQColorToAZColor(gSettings.hyperGraphColors.colorPortEdgeHighlighted, m_colorPortEdgeHighlighted);

    SetQColorToAZColor(gSettings.hyperGraphColors.colorArrowDisabled, m_colorArrowDisabled);

    SetQColorToAZColor(gSettings.hyperGraphColors.colorNodeOutline, m_colorNodeOutline);

    SetQColorToAZColor(gSettings.hyperGraphColors.colorNodeOutlineSelected, m_colorNodeOutlineSelected);

    SetQColorToAZColor(gSettings.hyperGraphColors.colorNodeBkg, m_colorNodeBkg);

    SetQColorToAZColor(gSettings.hyperGraphColors.colorNodeSelected, m_colorNodeSelected);

    SetQColorToAZColor(gSettings.hyperGraphColors.colorTitleText, m_colorTitleText);

    SetQColorToAZColor(gSettings.hyperGraphColors.colorTitleTextSelected, m_colorTitleTextSelected);

    SetQColorToAZColor(gSettings.hyperGraphColors.colorText, m_colorText);

    SetQColorToAZColor(gSettings.hyperGraphColors.colorBackground, m_colorBackground);

    SetQColorToAZColor(gSettings.hyperGraphColors.colorGrid, m_colorGrid);

    SetQColorToAZColor(gSettings.hyperGraphColors.colorBreakPoint, m_colorBreakPoint);

    SetQColorToAZColor(gSettings.hyperGraphColors.colorBreakPointDisabled, m_colorBreakPointDisabled);

    SetQColorToAZColor(gSettings.hyperGraphColors.colorBreakPointArrow, m_colorBreakPointArrow);

    SetQColorToAZColor(gSettings.hyperGraphColors.colorEntityPortNotConnected, m_colorEntityPortNotConnected);

    SetQColorToAZColor(gSettings.hyperGraphColors.colorPort, m_colorPort);

    SetQColorToAZColor(gSettings.hyperGraphColors.colorPortSelected, m_colorPortSelected);

    SetQColorToAZColor(gSettings.hyperGraphColors.colorEntityTextInvalid, m_colorEntityTextInvalid);

    SetQColorToAZColor(gSettings.hyperGraphColors.colorDownArrow, m_colorDownArrow);

    SetQColorToAZColor(gSettings.hyperGraphColors.colorCustomNodeBkg, m_colorCustomNodeBkg);

    SetQColorToAZColor(gSettings.hyperGraphColors.colorCustomSelectedNodeBkg, m_colorCustomSelectedNodeBkg);

    SetQColorToAZColor(gSettings.hyperGraphColors.colorPortDebugging, m_colorPortDebugging);

    SetQColorToAZColor(gSettings.hyperGraphColors.colorPortDebuggingText, m_colorPortDebuggingText);

    SetQColorToAZColor(gSettings.hyperGraphColors.colorQuickSearchBackground, m_colorQuickSearchBackground);

    SetQColorToAZColor(gSettings.hyperGraphColors.colorQuickSearchResultText, m_colorQuickSearchResultText);

    SetQColorToAZColor(gSettings.hyperGraphColors.colorQuickSearchCountText, m_colorQuickSearchCountText);

    SetQColorToAZColor(gSettings.hyperGraphColors.colorQuickSearchBorder, m_colorQuickSearchBorder);

    SetQColorToAZColor(gSettings.hyperGraphColors.colorDebugNodeTitle, m_colorDebugNodeTitle);

    SetQColorToAZColor(gSettings.hyperGraphColors.colorDebugNode, m_colorDebugNode);
}

void CEditorPreferencesPage_FlowGraphColors::SetAZColorToQColor(const AZ::Color& source, QColor& dest)
{
    dest.setRgbF(static_cast<qreal>(source.GetR()), // R
        static_cast<qreal>(source.GetG()),          // G
        static_cast<qreal>(source.GetB()),          // B
        static_cast<qreal>(source.GetA()));         // A
}
void CEditorPreferencesPage_FlowGraphColors::SetQColorToAZColor(const QColor& source, AZ::Color& dest)
{
    dest.Set(source.redF(),
        source.greenF(),
        source.blueF(), source.alphaF());
}
