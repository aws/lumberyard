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


// CSmartObjectActionDialog dialog
#ifndef CRYINCLUDE_EDITOR_SMARTOBJECTACTIONDIALOG_H
#define CRYINCLUDE_EDITOR_SMARTOBJECTACTIONDIALOG_H

#include <QDialog>

namespace Ui
{
    class SmartObjectActionDialog;
}

class CSmartObjectActionDialog
    : public QDialog
{
    Q_OBJECT
private:
    QString m_sSOAction;

public:
    CSmartObjectActionDialog(QWidget* pParent = nullptr);   // standard constructor
    virtual ~CSmartObjectActionDialog();

    void SetSOAction(const QString& sSOAction);
    QString GetSOAction() const { return m_sSOAction; };


protected:
    void OnLbnDblClk();
    void OnLbnSelchangeAction();
    void OnNewBtn();
    void OnEditBtn();
    void OnRefreshBtn();
public:
    void OnInitDialog();

private:
    QScopedPointer<Ui::SmartObjectActionDialog> m_ui;
};

#endif // CRYINCLUDE_EDITOR_SMARTOBJECTACTIONDIALOG_H
