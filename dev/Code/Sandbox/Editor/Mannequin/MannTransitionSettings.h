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

#ifndef CRYINCLUDE_EDITOR_MANNEQUIN_MANNTRANSITIONSETTINGS_H
#define CRYINCLUDE_EDITOR_MANNEQUIN_MANNTRANSITIONSETTINGS_H
#pragma once

#include "MannequinBase.h"
#include <Controls/ReflectedPropertyControl/ReflectedPropertiesPanel.h>

#include <QDialog>
#include <QScopedPointer>

// fwd decl'
struct SScopeContextData;

namespace Ui {
    class CMannTransitionSettingsDlg;
}


class CMannTransitionSettingsDlg
    : public QDialog
{
    Q_OBJECT
public:
    CMannTransitionSettingsDlg(const QString& windowCaption, FragmentID& fromFragID, FragmentID& toFragID, SFragTagState& fromFragTag, SFragTagState& toFragTag, QWidget* pParent = NULL);
    virtual ~CMannTransitionSettingsDlg();

public slots:
    void OnOk();
    void OnComboChange();

protected:
    bool OnInitDialog();

private:
    void UpdateFragmentIDs();
    void PopulateControls();
    void PopulateComboBoxes();
    void PopulateTagPanels();

    //
    FragmentID& m_IDFrom;
    FragmentID& m_IDTo;

    //
    SFragTagState& m_TagsFrom;
    SFragTagState& m_TagsTo;

    // "From" tag editing
    ReflectedPropertiesPanel m_tagsFromPanel;
    FragmentID m_tagFromPanelFragID;
    TSmartPtr<CVarBlock> m_tagFromVars;
    CTagControl m_tagFromControls;
    CTagControl m_tagFromFragControls;

    // "To" tag editing
    ReflectedPropertiesPanel m_tagsToPanel;
    FragmentID m_tagToPanelFragID;
    TSmartPtr<CVarBlock> m_tagToVars;
    CTagControl m_tagToControls;
    CTagControl m_tagToFragControls;

    QScopedPointer<Ui::CMannTransitionSettingsDlg> ui;
};

#endif // CRYINCLUDE_EDITOR_MANNEQUIN_MANNTRANSITIONSETTINGS_H
