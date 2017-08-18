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

#include <QDialog>

class QItemSelection;
class QMouseEvent;

// CSmartObjectTemplateDialog dialog
#ifndef CRYINCLUDE_EDITOR_SMARTOBJECTTEMPLATEDIALOG_H
#define CRYINCLUDE_EDITOR_SMARTOBJECTTEMPLATEDIALOG_H

namespace Ui
{
    class SmartObjectTemplateDialog;
}

class SmartObjectTemplateModel;

class CSmartObjectTemplateDialog
    : public QDialog
{
    Q_OBJECT

private:
    QScopedPointer<Ui::SmartObjectTemplateDialog> m_ui;
    SmartObjectTemplateModel* m_model;
    int m_idSOTemplate;

public:
    CSmartObjectTemplateDialog(QWidget* parent = nullptr);     // standard constructor
    virtual ~CSmartObjectTemplateDialog();

    void SetSOTemplate(const QString& sSOTemplate);
    QString GetSOTemplate() const;
    int GetSOTemplateId() const { return m_idSOTemplate; }

protected:
    void OnLbnSelchangeTemplate(const QItemSelection& selected, const QItemSelection& deselected);
    void OnNewBtn();
    void OnEditBtn();
    void OnDeleteBtn();
    void OnRefreshBtn();

public:
    void OnInitDialog();
};

#endif // CRYINCLUDE_EDITOR_SMARTOBJECTTEMPLATEDIALOG_H
