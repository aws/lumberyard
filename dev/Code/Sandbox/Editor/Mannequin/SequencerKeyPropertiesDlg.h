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

#ifndef CRYINCLUDE_EDITOR_MANNEQUIN_SEQUENCERKEYPROPERTIESDLG_H
#define CRYINCLUDE_EDITOR_MANNEQUIN_SEQUENCERKEYPROPERTIESDLG_H
#pragma once

#pragma once

#include "SequencerUtils.h"
//#include "tcbpreviewctrl.h"
#include "Plugin.h"
#include "SequencerDopeSheetBase.h"
#include "SequencerSequence.h"

#include <QWidget>

class ReflectedPropertyControl;

class CSequencerKeyPropertiesDlg;

namespace Ui
{
    class SequencerTrackPropsDlg;
}

//////////////////////////////////////////////////////////////////////////
class CSequencerKeyUIControls
    : public _i_reference_target_t
{
public:
    typedef CSequencerUtils::SelectedKey  SelectedKey;
    typedef CSequencerUtils::SelectedKeys SelectedKeys;

    CSequencerKeyUIControls() { m_pVarBlock = new CVarBlock; };

    void SetKeyPropertiesDlg(CSequencerKeyPropertiesDlg* pDlg) { m_pKeyPropertiesDlg = pDlg; }

    // Return internal variable block.
    CVarBlock* GetVarBlock() const { return m_pVarBlock; }

    //////////////////////////////////////////////////////////////////////////
    // Callbacks that must be implemented in derived class
    //////////////////////////////////////////////////////////////////////////
    // Returns true if specified animation track type is supported by this UI.
    virtual bool SupportTrackType(ESequencerParamType type) const = 0;

    // Called when UI variable changes.
    void CreateVars()
    {
        m_pVarBlock->DeleteAllVariables();
        OnCreateVars();
    }

    // Called when user changes selected keys.
    // Return true if control update UI values
    virtual bool OnKeySelectionChange(SelectedKeys& keys) = 0;

    // Called when UI variable changes.
    virtual void OnUIChange(IVariable* pVar, SelectedKeys& keys) = 0;

    void OnInternalVariableChange(IVariable* pVar);

    virtual void RenameTag(TagID tagID, const QString& name) { }
    virtual void RemoveTag(TagID tagID) { }
    virtual void AddTag(const QString& name) { }

protected:

    virtual void OnCreateVars() = 0;

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
    void AddVariable(CVariableBase& varArray, IVariable& var, const char* varName, unsigned char dataType = IVariable::DT_SIMPLE)
    {
        if (varName)
        {
            var.SetName(varName);
        }
        var.SetDataType(dataType);
        var.AddOnSetCallback(functor(*this, &CSequencerKeyUIControls::OnInternalVariableChange));
        varArray.AddVariable(&var);
    }
    //////////////////////////////////////////////////////////////////////////
    void AddVariable(CVariableBase& var, const char* varName, unsigned char dataType = IVariable::DT_SIMPLE)
    {
        if (varName)
        {
            var.SetName(varName);
        }
        var.SetDataType(dataType);
        var.AddOnSetCallback(functor(*this, &CSequencerKeyUIControls::OnInternalVariableChange));
        m_pVarBlock->AddVariable(&var);
    }

    void RefreshSequencerKeys();

protected:
    _smart_ptr<CVarBlock> m_pVarBlock;
    CSequencerKeyPropertiesDlg* m_pKeyPropertiesDlg;
};

//////////////////////////////////////////////////////////////////////////
class CSequencerTrackPropsDlg
    : public QWidget
{
    Q_OBJECT
public:
    CSequencerTrackPropsDlg(QWidget* parent = nullptr);   // standard constructor
    ~CSequencerTrackPropsDlg();

    void SetSequence(CSequencerSequence* pSequence);
    bool OnKeySelectionChange(CSequencerUtils::SelectedKeys& keys);
    void ReloadKey();

protected:
    void SetCurrKey(int nkey);

protected slots:
    void OnDeltaposPrevnext();
    void OnUpdateTime();

protected:
    void OnDestroy();

    _smart_ptr<CSequencerTrack> m_track;
    int m_key;

    QScopedPointer<Ui::SequencerTrackPropsDlg> m_ui;

    bool m_inOnKeySelectionChange;
};


//////////////////////////////////////////////////////////////////////////
class SequencerKeys;
class CSequencerKeyPropertiesDlg
    : public QWidget
    , public ISequencerEventsListener
{
    Q_OBJECT
public:
    CSequencerKeyPropertiesDlg(QWidget* parent = nullptr, bool overrideConstruct = false);
    ~CSequencerKeyPropertiesDlg();

    void SetKeysCtrl(CSequencerDopeSheetBase* pKeysCtrl)
    {
        m_keysCtrl = pKeysCtrl;
        if (m_keysCtrl)
        {
            m_keysCtrl->SetKeyPropertiesDlg(this);
        }
    }

    void SetSequence(CSequencerSequence* pSequence);
    CSequencerSequence* GetSequence() { return m_pSequence; };

    void PopulateVariables();
    void PopulateVariables(ReflectedPropertyControl& propCtrl);

    //////////////////////////////////////////////////////////////////////////
    // ISequencerEventsListener interface implementation
    //////////////////////////////////////////////////////////////////////////
    virtual void OnKeySelectionChange();
    //////////////////////////////////////////////////////////////////////////

    void CreateAllVars();

    void SetVarValue(const char* varName, const char* varValue);

protected:
    //////////////////////////////////////////////////////////////////////////
    void OnVarChange(IVariable* pVar);

    void AddVars(CSequencerKeyUIControls* pUI);
    void ReloadValues();

protected:
    std::vector< _smart_ptr<CSequencerKeyUIControls> > m_keyControls;

    _smart_ptr<CVarBlock> m_pVarBlock;
    _smart_ptr<CSequencerSequence> m_pSequence;

    ReflectedPropertyControl* m_wndProps;
    CSequencerTrackPropsDlg* m_wndTrackProps;

    CSequencerDopeSheetBase* m_keysCtrl;

    CSequencerTrack* m_pLastTrackSelected;
};

#endif // CRYINCLUDE_EDITOR_MANNEQUIN_SEQUENCERKEYPROPERTIESDLG_H
