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


class CEditorPreferencesPage_MannequinGeneral
    : public IPreferencesPage
{
public:
    AZ_RTTI(CEditorPreferencesPage_MannequinGeneral, "{8257CA63-0DDA-4693-8FAF-3EC6772F4BB0}", IPreferencesPage)

    static void Reflect(AZ::SerializeContext& serialize);

    CEditorPreferencesPage_MannequinGeneral();

    virtual const char* GetCategory() override { return "Mannequin"; }
    virtual const char* GetTitle() override { return "General"; }
    virtual void OnApply() override;
    virtual void OnCancel() override {}
    virtual bool OnQueryCancel() override { return true; }

private:
    void InitializeSettings();

    struct General
    {
        AZ_TYPE_INFO(General, "{E37C118D-7D89-4924-820D-D18F1B4D2C9F}")

        int m_trackSize;
        bool m_ctrlScrubSnapping;
        float m_timelineWheelZoomSpeed;
    };

    General m_general;
};

