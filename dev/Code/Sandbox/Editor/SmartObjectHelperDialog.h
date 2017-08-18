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


// CSmartObjectHelperDialog dialog
#ifndef CRYINCLUDE_EDITOR_SMARTOBJECTHELPERDIALOG_H
#define CRYINCLUDE_EDITOR_SMARTOBJECTHELPERDIALOG_H

#include <QDialog>

namespace Ui
{
    class SmartObjectHelperDialog;
};

class CSmartObjectHelperDialog
    : public QDialog
{
    Q_OBJECT
private:
    QString m_sClassName;
    QString m_sSOHelper;
    bool m_bAllowEmpty;
    bool m_bFromTemplate;
    bool m_bShowAuto;

public:
    CSmartObjectHelperDialog(QWidget* pParent = nullptr, bool bAllowEmpty = true, bool bFromTemplate = false, bool bShowAuto = false);
    virtual ~CSmartObjectHelperDialog();

    void SetSOHelper(const QString& sClassName, const QString& sSOHelper) { m_sClassName = sClassName; m_sSOHelper = sSOHelper; OnRefreshBtn(); }
    QString GetSOHelper() { return m_sSOHelper; };

protected:
    void OnLbnDblClk();
    void OnLbnSelchangeHelper();
    void OnNewBtn();
    void OnEditBtn();
    void OnDeleteBtn();
    void OnRefreshBtn();

    void UpdateDescription();

public:
    void OnInitDialog();

private:
    QScopedPointer<Ui::SmartObjectHelperDialog> m_ui;
};

#endif // CRYINCLUDE_EDITOR_SMARTOBJECTHELPERDIALOG_H
