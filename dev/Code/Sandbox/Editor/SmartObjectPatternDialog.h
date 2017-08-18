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


// CSmartObjectPatternDialog dialog
#ifndef CRYINCLUDE_EDITOR_SMARTOBJECTPATTERNDIALOG_H
#define CRYINCLUDE_EDITOR_SMARTOBJECTPATTERNDIALOG_H

#include <QDialog>

namespace Ui
{
    class SmartObjectPatternDialog;
}

class CSmartObjectPatternDialog
    : public QDialog
{
    Q_OBJECT
public:
    CSmartObjectPatternDialog(QWidget* pParent = nullptr);
    virtual ~CSmartObjectPatternDialog();

    void SetPattern(const QString& sPattern);
    QString GetPattern() const;

protected:
    void OnLbnDblClk();
    void OnLbnSelchange();
    void OnNewBtn();
    void OnEditBtn();
    void OnDeleteBtn();

    void OnInitDialog();

private:
    QScopedPointer<Ui::SmartObjectPatternDialog> m_ui;
};

#endif // CRYINCLUDE_EDITOR_SMARTOBJECTPATTERNDIALOG_H
