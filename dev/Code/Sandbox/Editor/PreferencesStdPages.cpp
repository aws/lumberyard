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

#include "StdAfx.h"
#include "PreferencesStdPages.h"
#include "EditorPreferencesPageGeneral.h"
#include "EditorPreferencesPageFiles.h"
#include "EditorPreferencesPageViewportGeneral.h"
#include "EditorPreferencesPageViewportGizmo.h"
#include "EditorPreferencesPageViewportMovement.h"
#include "EditorPreferencesPageViewportDebug.h"
#include "EditorPreferencesPageMannequinGeneral.h"
#include "EditorPreferencesPageExperimentalLighting.h"
#include "EditorPreferencesPageFlowGraphGeneral.h"
#include "EditorPreferencesPageFlowGraphColors.h"


//////////////////////////////////////////////////////////////////////////
// Implementation of ClassDesc for standard Editor preferences.
//////////////////////////////////////////////////////////////////////////

CStdPreferencesClassDesc::CStdPreferencesClassDesc()
    : m_refCount(0)
{
    m_pageCreators = {
        [](){ return new CEditorPreferencesPage_General(); },
        [](){ return new CEditorPreferencesPage_Files(); },
        [](){ return new CEditorPreferencesPage_ViewportGeneral(); },
        [](){ return new CEditorPreferencesPage_ViewportMovement(); },
        [](){ return new CEditorPreferencesPage_ViewportGizmo(); },
        [](){ return new CEditorPreferencesPage_ViewportDebug(); }
    };

    if (GetIEditor()->IsLegacyUIEnabled())
    {
        m_pageCreators.push_back([]() { return new CEditorPreferencesPage_FlowGraphGeneral(); });
        m_pageCreators.push_back([]() { return new CEditorPreferencesPage_FlowGraphColors(); });
    }

#ifdef ENABLE_LEGACY_ANIMATION
    m_pageCreators.push_back([]() { return new CEditorPreferencesPage_MannequinGeneral(); });
#endif //ENABLE_LEGACY_ANIMATION
    m_pageCreators.push_back([]() { return new CEditorPreferencesPage_ExperimentalLighting(); });
    m_pageCreators.push_back([]() { return new CEditorPreferencesPage_FragLabGeneral(); });
}

HRESULT CStdPreferencesClassDesc::QueryInterface(const IID& riid, void** ppvObj)
{
    if (riid == __uuidof(IPreferencesPageCreator))
    {
        *ppvObj = (IPreferencesPageCreator*)this;
        return S_OK;
    }
    return E_NOINTERFACE;
}

//////////////////////////////////////////////////////////////////////////
ULONG CStdPreferencesClassDesc::AddRef()
{
    m_refCount++;
    return m_refCount;
};

//////////////////////////////////////////////////////////////////////////
ULONG CStdPreferencesClassDesc::Release()
{
    ULONG refs = --m_refCount;
    if (m_refCount <= 0)
    {
        delete this;
    }
    return refs;
}

//////////////////////////////////////////////////////////////////////////
REFGUID CStdPreferencesClassDesc::ClassID()
{
    // {95FE3251-796C-4e3b-82F0-AD35F7FFA267}
    static const GUID guid = {
        0x95fe3251, 0x796c, 0x4e3b, { 0x82, 0xf0, 0xad, 0x35, 0xf7, 0xff, 0xa2, 0x67 }
    };
    return guid;
}

//////////////////////////////////////////////////////////////////////////
int CStdPreferencesClassDesc::GetPagesCount()
{
    return m_pageCreators.size();
}

IPreferencesPage* CStdPreferencesClassDesc::CreateEditorPreferencesPage(int index)
{
    return (index >= m_pageCreators.size()) ? nullptr : m_pageCreators[index]();
}

void CEditorPreferencesPage_FragLabGeneral::Reflect(AZ::SerializeContext& serialize)
{
    serialize.Class<FragLabGeneralSettings>()
        ->Version(1)
        ->Field("LockInstantiatedSlice", &FragLabGeneralSettings::m_lockInstantiatedSlice)
        ->Field("ParticlesSortingMode", &FragLabGeneralSettings::m_particlesSortingMode);

    serialize.Class<CEditorPreferencesPage_FragLabGeneral>()
        ->Version(0)
        ->Field("GeneralSettings", &CEditorPreferencesPage_FragLabGeneral::m_generalSettings);

    AZ::EditContext* editContext = serialize.GetEditContext();
    if (editContext)
    {
        editContext->Class<FragLabGeneralSettings>("General Settings", "")
            ->DataElement(AZ::Edit::UIHandlers::CheckBox, &FragLabGeneralSettings::m_lockInstantiatedSlice, "Lock Instantiated Slice", "Leave only root slice entity unlocked on 'Instantiate slice'")
            ->DataElement(AZ::Edit::UIHandlers::ComboBox, &FragLabGeneralSettings::m_particlesSortingMode, "Particles Sorting Mode", "Toggle particles sorting in Particle Editor's Library")
                ->EnumAttribute(ParticlesSortingMode::Off, "Off")
                ->EnumAttribute(ParticlesSortingMode::Ascending, "Ascending (A-Z)")
                ->EnumAttribute(ParticlesSortingMode::Descending, "Descending (Z-A)");

        editContext->Class<CEditorPreferencesPage_FragLabGeneral>("FragLab Preferences", "FragLab Preferences")
            ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
            ->Attribute(AZ::Edit::Attributes::Visibility, AZ_CRC("PropertyVisibility_ShowChildrenOnly", 0xef428f20))
            ->DataElement(AZ::Edit::UIHandlers::Default, &CEditorPreferencesPage_FragLabGeneral::m_generalSettings, "General Settings", "General Settings");
    }
}

CEditorPreferencesPage_FragLabGeneral::CEditorPreferencesPage_FragLabGeneral()
{
    InitializeSettings();
}

void CEditorPreferencesPage_FragLabGeneral::OnApply()
{
    gSettings.sFragLabSettings.lockInstantiatedSlice = m_generalSettings.m_lockInstantiatedSlice;
    gSettings.sFragLabSettings.particlesSortingMode = m_generalSettings.m_particlesSortingMode;
}

void CEditorPreferencesPage_FragLabGeneral::InitializeSettings()
{
    m_generalSettings.m_lockInstantiatedSlice = gSettings.sFragLabSettings.lockInstantiatedSlice;
    m_generalSettings.m_particlesSortingMode = gSettings.sFragLabSettings.particlesSortingMode;
}
