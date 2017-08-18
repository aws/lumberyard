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

#ifndef CRYINCLUDE_EDITOR_SELECTENTITYCLSDIALOG_H
#define CRYINCLUDE_EDITOR_SELECTENTITYCLSDIALOG_H
#pragma once

#include <QDialog>

#include "ui_SelectEntityClsDialog.h"

class SelectEntityClsModel;

// CSelectEntityClsDialog dialog

class CSelectEntityClsDialog
    : public QDialog
{
    Q_OBJECT
public:
    CSelectEntityClsDialog(QWidget* pParent = nullptr);   // standard constructor
    ~CSelectEntityClsDialog();

    QString GetEntityClass() const;

// Dialog Data
protected:
    void OnTvnDoubleClick(const QModelIndex& index);
	
    void ReloadEntities();

    void showEvent(QShowEvent* event) override;

private:
    QScopedPointer<Ui_CSelectEntityClsDialog> m_ui;
    SelectEntityClsModel* m_model;
    QString m_entityClass;
};

#endif // CRYINCLUDE_EDITOR_SELECTENTITYCLSDIALOG_H
