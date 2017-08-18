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

#ifndef CRYINCLUDE_EDITOR_AIDIALOG_H
#define CRYINCLUDE_EDITOR_AIDIALOG_H

#pragma once

#include <QDialog>

class AIBehaviorsModel;

namespace Ui {
    class CAIDialog;
}

// CAIDialog dialog

class CAIDialog
    : public QDialog
{
    Q_OBJECT
public:
    CAIDialog(QWidget* pParent = nullptr);   // standard constructor
    virtual ~CAIDialog();

    void SetAIBehavior(const QString& behavior);
    QString GetAIBehavior() { return m_aiBehavior; };

protected:
    virtual void OnInitDialog();

    void ReloadBehaviors();

    //////////////////////////////////////////////////////////////////////////
    // FIELDS
    //////////////////////////////////////////////////////////////////////////
    QString m_aiBehavior;

    void OnBnClickedEdit();
    void OnBnClickedReload();
    void OnLbnSelchange();
    void OnLbnDblClk();

    AIBehaviorsModel* m_behaviorsModel;
    QScopedPointer<Ui::CAIDialog> ui;
};

#endif // CRYINCLUDE_EDITOR_AIDIALOG_H
