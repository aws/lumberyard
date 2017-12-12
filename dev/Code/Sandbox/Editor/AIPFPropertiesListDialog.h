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

#ifndef CRYINCLUDE_EDITOR_AIPFPROPERTIESLISTDIALOG_H
#define CRYINCLUDE_EDITOR_AIPFPROPERTIESLISTDIALOG_H

#pragma once

#include <QDialog>

#include <ui_AIPFPropertiesListDialog.h>

class AIPFPropertiesListModel;

class CAIPFPropertiesListDialog
    : public QDialog
{
    Q_OBJECT
public:
    CAIPFPropertiesListDialog(QWidget* pParent = nullptr);

    QString GetPFPropertiesList() const { return m_sPFPropertiesList; }
    void    SetPFPropertiesList(const QString& sPFPropertiesList) { m_sPFPropertiesList = sPFPropertiesList; }

protected:
    void showEvent(QShowEvent* event) override;

private slots:
    void OnBnClickedEdit();
    void OnBnClickedRefresh();
    void OnTVDataChanged(const QModelIndex& topLeft, const QModelIndex& bottomRight, const QVector<int>& roles);
    void OnTVSelChanged(const QModelIndex& current, const QModelIndex& previous);

private:
    void UpdateDescription(const QModelIndex& index);
    void UpdatePFPropertiesString();

private:
    QString m_sPFPropertiesList;
    QScopedPointer<Ui_CAIPFPropertiesListDialog> m_ui;
    AIPFPropertiesListModel* m_model;
    QString m_scriptFileName;
};

#endif // CRYINCLUDE_EDITOR_AIPFPROPERTIESLISTDIALOG_H
