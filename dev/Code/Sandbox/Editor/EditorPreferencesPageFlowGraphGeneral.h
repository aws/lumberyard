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


class CEditorPreferencesPage_FlowGraphGeneral
    : public IPreferencesPage
{
public:
    AZ_RTTI(CEditorPreferencesPage_FlowGraphGeneral, "{C2559A93-B954-4ACE-BE61-6FFC006CA7C4}", IPreferencesPage)

    static void Reflect(AZ::SerializeContext& serialize);

    CEditorPreferencesPage_FlowGraphGeneral();

    virtual const char* GetCategory() override { return "Flow Graph"; }
    virtual const char* GetTitle() override { return "General"; }
    virtual void OnApply() override;
    virtual void OnCancel() override {}
    virtual bool OnQueryCancel() override { return true; }

private:
    void InitializeSettings();

    struct ExpertOptions
    {
        AZ_TYPE_INFO(ExpertOptions, "{E08006EE-639B-44C7-A5D4-D7BB1E491C78}")

        bool m_enableMigration;
        bool m_showNodeIDs;
        bool m_showToolTip;
        bool m_edgesOnTopOfNodes;
        bool m_highlightEdges;
    };

    ExpertOptions m_expertOptions;
};

