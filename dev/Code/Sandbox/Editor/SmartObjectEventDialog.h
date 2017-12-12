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

#pragma once


// CSmartObjectEventDialog dialog
#ifndef CRYINCLUDE_EDITOR_SMARTOBJECTEVENTDIALOG_H
#define CRYINCLUDE_EDITOR_SMARTOBJECTEVENTDIALOG_H

#include <QDialog>

namespace Ui
{
    class SmartObjectEventDialog;
}

class CSmartObjectEventDialog
    : public QDialog
{
    Q_OBJECT
private:
    QString m_sSOEvent;

public:
    CSmartObjectEventDialog(QWidget* pParent = NULL);     // standard constructor
    virtual ~CSmartObjectEventDialog();

    void SetSOEvent(const QString& sSOEvent) { m_sSOEvent = sSOEvent; OnRefreshBtn();  }
    QString GetSOEvent() { return m_sSOEvent; };

protected:
    void OnLbnDblClk();
    void OnLbnSelchangeEvent();
    void OnNewBtn();
    void OnEditBtn();
    void OnDeleteBtn();
    void OnRefreshBtn();

    void UpdateDescription();

public:
    void OnInitDialog();

private:
    QScopedPointer<Ui::SmartObjectEventDialog> m_ui;
};

#endif // CRYINCLUDE_EDITOR_SMARTOBJECTEVENTDIALOG_H
