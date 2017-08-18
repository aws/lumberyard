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

#ifndef CRYINCLUDE_EDITOR_SELECTGAMETOKENDIALOG_H
#define CRYINCLUDE_EDITOR_SELECTGAMETOKENDIALOG_H
#pragma once

#include <QDialog>

#include "ui_SelectGameTokenDialog.h"

struct IDataBaseItem;
class SelectGameTokenModel;

// CSelectGameToken dialog

class CSelectGameTokenDialog
    : public QDialog
{
    Q_OBJECT
public:
    CSelectGameTokenDialog(QWidget* pParent = nullptr);   // standard constructor
    ~CSelectGameTokenDialog();

    QString GetSelectedGameToken() const;
    void PreSelectGameToken(const QString& name)
    {
        m_preselect = name;
    }

	// Dialog Data
protected:
    void OnTvnDoubleClick(const QModelIndex& index);
    void showEvent(QShowEvent* event) override;

private:
    QScopedPointer<Ui_CSelectGameTokenDialog> m_ui;
    SelectGameTokenModel* m_model;
    QString m_preselect;
};

#endif // CRYINCLUDE_EDITOR_SELECTGAMETOKENDIALOG_H
