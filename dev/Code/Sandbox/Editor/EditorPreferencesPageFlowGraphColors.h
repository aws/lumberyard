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
#pragma once

#include "Include/IPreferencesPage.h"
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/RTTI/RTTI.h>
#include <AzCore/Math/Color.h>


class CEditorPreferencesPage_FlowGraphColors
    : public IPreferencesPage
{
public:
    AZ_RTTI(CEditorPreferencesPage_FlowGraphColors, "{6E4776A3-4D7F-4CD5-8C05-784F5CD020A8}", IPreferencesPage)

    static void Reflect(AZ::SerializeContext& serialize);

    CEditorPreferencesPage_FlowGraphColors();

    virtual const char* GetCategory() override { return "Flow Graph"; }
    virtual const char* GetTitle() override { return "Colors"; }
    virtual void OnApply() override;
    virtual void OnCancel() override {}
    virtual bool OnQueryCancel() override { return true; }

private:
    void InitializeSettings();
    static void SetAZColorToQColor(const AZ::Color& source, QColor& dest);
    static void SetQColorToAZColor(const QColor& source, AZ::Color& dest);

    float m_opacity;
    AZ::Color m_colorArrow;
    AZ::Color m_colorInArrowHighlighted;
    AZ::Color m_colorOutArrowHighlighted;
    AZ::Color m_colorPortEdgeHighlighted;
    AZ::Color m_colorArrowDisabled;
    AZ::Color m_colorNodeOutline;
    AZ::Color m_colorNodeOutlineSelected;
    AZ::Color m_colorNodeBkg;
    AZ::Color m_colorCustomNodeBkg;
    AZ::Color m_colorCustomSelectedNodeBkg;
    AZ::Color m_colorNodeSelected;
    AZ::Color m_colorTitleText;
    AZ::Color m_colorTitleTextSelected;
    AZ::Color m_colorText;
    AZ::Color m_colorBackground;
    AZ::Color m_colorGrid;
    AZ::Color m_colorBreakPoint;
    AZ::Color m_colorBreakPointDisabled;
    AZ::Color m_colorBreakPointArrow;
    AZ::Color m_colorEntityPortNotConnected;
    AZ::Color m_colorPort;
    AZ::Color m_colorPortSelected;
    AZ::Color m_colorEntityTextInvalid;
    AZ::Color m_colorDownArrow;
    AZ::Color m_colorPortDebugging;
    AZ::Color m_colorPortDebuggingText;
    AZ::Color m_colorQuickSearchBackground;
    AZ::Color m_colorQuickSearchResultText;
    AZ::Color m_colorQuickSearchCountText;
    AZ::Color m_colorQuickSearchBorder;
    AZ::Color m_colorDebugNodeTitle;
    AZ::Color m_colorDebugNode;
};

