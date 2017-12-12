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

#ifndef CRYINCLUDE_EDITOR_PREFERENCESPROPERTYPAGE_H
#define CRYINCLUDE_EDITOR_PREFERENCESPROPERTYPAGE_H
#pragma once

#include "Include/IPreferencesPage.h"

//////////////////////////////////////////////////////////////////////////
// Class description implementation.
//////////////////////////////////////////////////////////////////////////
class CPreferencesPropertyPageClassDesc
    : public IPreferencesPageClassDesc
{
public:
    IPreferencesPage* CreatePage(const CRect& rc, CWnd* pParentWnd);
};


//////////////////////////////////////////////////////////////////////////
// Implementation of PreferencePage with properties control.
//////////////////////////////////////////////////////////////////////////
class CPreferencesPropertyPage
    : public CWnd
    , public IPreferencesPage
{
    DECLARE_DYNAMIC(CPreferencesPropertyPage);
public:
    CPreferencesPropertyPage();

    BOOL Create(const CRect& rc, CWnd* pParentWnd);

    virtual const char* GetCategory() { return "aa"; }
    virtual const char* GetTitle()  { return "bb"; }

    //////////////////////////////////////////////////////////////////////////
    // Implements IPreferencesPage.
    //////////////////////////////////////////////////////////////////////////
    virtual CWnd* GetWindow() { return this; }
    virtual void Release() { delete this; }

    virtual void OnApply();
    virtual void OnCancel();
    virtual bool OnQueryCancel();
    virtual void OnSetActive(bool bActive);
    virtual void OnOK();

    virtual CVarBlock* GetVars() { return m_pVars; };

    //////////////////////////////////////////////////////////////////////////
    // CVariable helper functions.
    //////////////////////////////////////////////////////////////////////////
    void AddVariable(CVariableBase& var, const char* varName, unsigned char dataType = IVariable::DT_SIMPLE);
    void AddVariable(CVariableArray& table, CVariableBase& var, const char* varName, unsigned char dataType = IVariable::DT_SIMPLE);

    template <class T>
    void CopyVar(CVariable<T>& var, T& value, bool bSet)
    {
        if (bSet)
        {
            var = value;
        }
        else
        {
            value = var;
        }
    }
    void CopyVar(CVariable<int>& var, COLORREF& value, bool bSet)
    {
        if (bSet)
        {
            var = value;
        }
        else
        {
            value = var;
            value |= 0xFF000000;
        }
    }

protected:
    DECLARE_MESSAGE_MAP()

    afx_msg void OnSize(UINT nType, int cx, int cy);

    //////////////////////////////////////////////////////////////////////////
protected:
    CPropertyCtrl m_wndProps;
    _smart_ptr<CVarBlock> m_pVars;
};

#endif // CRYINCLUDE_EDITOR_PREFERENCESPROPERTYPAGE_H

