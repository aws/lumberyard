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

#ifndef CRYINCLUDE_EDITOR_PROPERTIESPANEL_H
#define CRYINCLUDE_EDITOR_PROPERTIESPANEL_H

#pragma once
// PropertiesPanel.h : header file
//

#include "Util/Variable.h"

/////////////////////////////////////////////////////////////////////////////
// CPropertiesPanel dialog

class SANDBOX_API CPropertiesPanel
    : public CDialog
{
public:
    CPropertiesPanel(CWnd* pParent = NULL);   // standard constructor

    typedef Functor1<IVariable*> UpdateCallback;

    //{{AFX_DATA(CPropertiesPanel)
    enum
    {
        IDD = IDD_PANEL_PROPERTIES
    };
    // NOTE: the ClassWizard will add data members here
    //}}AFX_DATA

    void ReloadValues();

    void DeleteVars();
    void AddVars(class CVarBlock* vb, const UpdateCallback& func = NULL);

    void SetVarBlock(class CVarBlock* vb, const UpdateCallback& func = NULL, const bool resizeToFit = true);

    void SetMultiSelect(bool bEnable);
    bool IsMultiSelect() const { return m_multiSelect; };

    void SetTitle(const char* title);
    virtual void ResizeToFitProperties();

    //////////////////////////////////////////////////////////////////////////
    // Helper functions.
    //////////////////////////////////////////////////////////////////////////
    template <class T>
    void SyncValue(CSmartVariable<T>& var, T& value, bool bCopyToUI, IVariable* pSrcVar = NULL)
    {
        if (bCopyToUI)
        {
            var = value;
        }
        else
        {
            if (!pSrcVar || pSrcVar == var.GetVar())
            {
                value = var;
            }
        }
    }
    void SyncValueFlag(CSmartVariable<bool>& var, int& nFlags, int flag, bool bCopyToUI, IVariable* pVar = NULL)
    {
        if (bCopyToUI)
        {
            var = (nFlags & flag) != 0;
        }
        else
        {
            if (!pVar || var.GetVar() == pVar)
            {
                nFlags = (var) ? (nFlags | flag) : (nFlags & (~flag));
            }
        }
    }

    //////////////////////////////////////////////////////////////////////////
    void AddVariable(CVariableBase& varArray, CVariableBase& var, const char* varName, unsigned char dataType = IVariable::DT_SIMPLE)
    {
        if (varName)
        {
            var.SetName(varName);
        }
        var.SetDataType(dataType);
        varArray.AddVariable(&var);
    }
    //////////////////////////////////////////////////////////////////////////
    void AddVariable(CVarBlock* vars, CVariableBase& var, const char* varName, unsigned char dataType = IVariable::DT_SIMPLE)
    {
        if (varName)
        {
            var.SetName(varName);
        }
        var.SetDataType(dataType);
        vars->AddVariable(&var);
    }
    void UpdateVarBlock(CVarBlock* vars);

    //////////////////////////////////////////////////////////////////////////
    CPropertyCtrl* GetPropertyCtrl() { return m_pWndProps.get(); }

protected:
    void OnPropertyChanged(IVariable* pVar);

    virtual void OnOK() {};
    virtual void OnCancel() {};

    //{{AFX_VIRTUAL(CPropertiesPanel)
    virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
    //}}AFX_VIRTUAL

    //{{AFX_MSG(CPropertiesPanel)
    virtual BOOL OnInitDialog();
    afx_msg void OnDestroy();
    afx_msg void OnKillFocus(CWnd* pNewWnd);
    afx_msg void OnSize(UINT nType, int cx, int cy);
    //}}AFX_MSG
    DECLARE_MESSAGE_MAP()

protected:
    std::unique_ptr<CPropertyCtrl> m_pWndProps;
    XmlNodeRef m_template;
    bool m_multiSelect;
    TSmartPtr<CVarBlock> m_varBlock;

    std::list<UpdateCallback> m_updateCallbacks;
    int m_titleAdjust;
};


#endif // CRYINCLUDE_EDITOR_PROPERTIESPANEL_H
