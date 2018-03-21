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

#ifndef CRYINCLUDE_EDITOR_PANELDISPLAYHIDE_H
#define CRYINCLUDE_EDITOR_PANELDISPLAYHIDE_H

#pragma once
// PanelDisplatHide.h : header file
//

#include <QWidget>
#include <QScopedPointer>

namespace Ui {
    class CPanelDisplayHide;
}

/////////////////////////////////////////////////////////////////////////////
// CPanelDisplayHide dialog

class CPanelDisplayHide
    : public QWidget
    , public IEditorNotifyListener
{
    Q_OBJECT
    // Construction
public:
    CPanelDisplayHide(QWidget* pParent = nullptr);   // standard constructor
    ~CPanelDisplayHide();

    virtual void OnEditorNotifyEvent(EEditorNotifyEvent event);

protected:
    void showEvent(QShowEvent* e) override;

private slots:
    void SetCheckButtons();
    void SetMask();
    void OnHideAll();
    void OnHideNone();
    void OnHideInvert();
    void OnChangeHideMask();

private:
    uint32 m_mask;
    QScopedPointer<Ui::CPanelDisplayHide> ui;

    bool m_initialized;
};

#endif // CRYINCLUDE_EDITOR_PANELDISPLAYHIDE_H
