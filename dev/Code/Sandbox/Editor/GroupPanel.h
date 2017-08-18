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

#ifndef CRYINCLUDE_EDITOR_GROUPPANEL_H
#define CRYINCLUDE_EDITOR_GROUPPANEL_H
#pragma once

#include "Controls/ToolButton.h"
#include <QWidget>
#include <QScopedPointer>

class CGroup;
class QPushButton;

namespace Ui {
    class CGroupPanel;
}

// CGroupPanel dialog
class CGroupPanel
    : public QWidget
    , public IEditorNotifyListener
{
public:
    CGroupPanel(CGroup* obj, QWidget* pParent = nullptr);    // standard constructor
    virtual ~CGroupPanel();

protected:
    void OnEditorNotifyEvent(EEditorNotifyEvent event) override;

    void OnGroup();
    void OnOpen();
    void OnAttach();
    void OnUngroup();
    void OnCloseGroup();
    void OnDetach();

    void UpdateButtons();

    CGroup* m_obj;

    QPushButton* m_GroupBn;
    QPushButton* m_OpenBn;
    QPushButton* m_AttachBn;
    QPushButton* m_UngroupBn;
    QPushButton* m_CloseBn;
    QPushButton* m_DetachBn;
    QScopedPointer<Ui::CGroupPanel> ui;
};


#endif // CRYINCLUDE_EDITOR_GROUPPANEL_H
