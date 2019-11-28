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

#ifndef CRYINCLUDE_EDITOR_PREFERENCESSTDPAGES_H
#define CRYINCLUDE_EDITOR_PREFERENCESSTDPAGES_H
#pragma once

#include "Include/IPreferencesPage.h"
#include <AzCore/std/functional.h>
#include <AzCore/std/containers/vector.h>

#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/RTTI/RTTI.h>

class CEditorPreferencesPage_FragLabGeneral
    : public IPreferencesPage
{
public:
    AZ_RTTI(CEditorPreferencesPage_FragLabGeneral, "{E50D0D5D-2A67-453D-BCFE-2694D8A3EF6E}", IPreferencesPage)

    static void Reflect(AZ::SerializeContext& serialize);

    CEditorPreferencesPage_FragLabGeneral();
    virtual ~CEditorPreferencesPage_FragLabGeneral() = default;

    virtual const char* GetCategory() override { return "FragLab"; }
    virtual const char* GetTitle() override { return "General"; }
    virtual void OnApply() override;
    virtual void OnCancel() override {}
    virtual bool OnQueryCancel() override { return true; }

private:
    void InitializeSettings();

    struct FragLabGeneralSettings
    {
        AZ_TYPE_INFO(FragLabGeneralSettings, "{86BD4B58-551D-44C1-BDEF-4DFA0D41B46D}")

        bool m_lockInstantiatedSlice;
        ParticlesSortingMode m_particlesSortingMode;
    };

    FragLabGeneralSettings m_generalSettings;
};

//////////////////////////////////////////////////////////////////////////
class CStdPreferencesClassDesc
    : public IPreferencesPageClassDesc
    , public IPreferencesPageCreator
{
    ULONG m_refCount;
    AZStd::vector< AZStd::function< IPreferencesPage*() > > m_pageCreators;
public:
    CStdPreferencesClassDesc();
    virtual ~CStdPreferencesClassDesc() = default;

    //////////////////////////////////////////////////////////////////////////
    // IUnkown implementation.
    virtual HRESULT STDMETHODCALLTYPE QueryInterface(const IID& riid, void** ppvObj);
    virtual ULONG STDMETHODCALLTYPE AddRef();
    virtual ULONG STDMETHODCALLTYPE Release();
    //////////////////////////////////////////////////////////////////////////

    virtual REFGUID ClassID();

    //////////////////////////////////////////////////////////////////////////
    virtual int GetPagesCount();
    virtual IPreferencesPage* CreateEditorPreferencesPage(int index) override;
};

#endif // CRYINCLUDE_EDITOR_PREFERENCESSTDPAGES_H

