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

#ifdef ENABLE_LEGACY_ANIMATION
#include "EditorPreferencesPageMannequinGeneral.h"
#include <AzCore/Serialization/EditContext.h>

void CEditorPreferencesPage_MannequinGeneral::Reflect(AZ::SerializeContext& serialize)
{
    serialize.Class<General>()
        ->Version(1)
        ->Field("TrackSize", &General::m_trackSize)
        ->Field("CtrlScrubSnapping", &General::m_ctrlScrubSnapping)
        ->Field("TimeLineWheelZoomSpeed", &General::m_timelineWheelZoomSpeed);

    serialize.Class<CEditorPreferencesPage_MannequinGeneral>()
        ->Version(1)
        ->Field("General", &CEditorPreferencesPage_MannequinGeneral::m_general);

    AZ::EditContext* editContext = serialize.GetEditContext();
    if (editContext)
    {
        editContext->Class<General>("General", "")
            ->DataElement(AZ::Edit::UIHandlers::SpinBox, &General::m_trackSize, "Size of Tracks", "Size of Tracks")
                ->Attribute(AZ::Edit::Attributes::Min, kMannequinTrackSizeMin)
                ->Attribute(AZ::Edit::Attributes::Max, kMannequinTrackSizeMax)
            ->DataElement(AZ::Edit::UIHandlers::CheckBox, &General::m_ctrlScrubSnapping, "Ctrl to Snap Scrubbing", "Hold Control to Snap Scrubbing")
            ->DataElement(AZ::Edit::UIHandlers::SpinBox, &General::m_timelineWheelZoomSpeed, "Timeline Wheel Zoom Speed", "Timeline Wheel Zoom Speed")
                ->Attribute(AZ::Edit::Attributes::Min, 0.1f)
                ->Attribute(AZ::Edit::Attributes::Max, 5.0f);

        editContext->Class<CEditorPreferencesPage_MannequinGeneral>("Mannequin General Preferences", "Mannequin General Preferences")
            ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
            ->Attribute(AZ::Edit::Attributes::Visibility, AZ_CRC("PropertyVisibility_ShowChildrenOnly", 0xef428f20))
            ->DataElement(AZ::Edit::UIHandlers::Default, &CEditorPreferencesPage_MannequinGeneral::m_general, "General", "Mannequin General Options");
    }
}


CEditorPreferencesPage_MannequinGeneral::CEditorPreferencesPage_MannequinGeneral()
{
    InitializeSettings();
}

void CEditorPreferencesPage_MannequinGeneral::OnApply()
{
    gSettings.mannequinSettings.bCtrlForScrubSnapping = m_general.m_ctrlScrubSnapping;
    gSettings.mannequinSettings.timelineWheelZoomSpeed = m_general.m_timelineWheelZoomSpeed;
    gSettings.mannequinSettings.trackSize = m_general.m_trackSize;
}

void CEditorPreferencesPage_MannequinGeneral::InitializeSettings()
{
    m_general.m_ctrlScrubSnapping = gSettings.mannequinSettings.bCtrlForScrubSnapping;
    m_general.m_timelineWheelZoomSpeed = gSettings.mannequinSettings.timelineWheelZoomSpeed;
    m_general.m_trackSize = gSettings.mannequinSettings.trackSize;
}
#endif //ENABLE_LEGACY_ANIMATION