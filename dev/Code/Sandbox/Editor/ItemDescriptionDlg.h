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

#ifndef CRYINCLUDE_EDITOR_ITEMDESCRIPTIONDLG_H
#define CRYINCLUDE_EDITOR_ITEMDESCRIPTIONDLG_H
#pragma once

#include <QDialog>

namespace Ui
{
    class ItemDescriptionDialog;
}

class CItemDescriptionDlg
    : public QDialog
{
public:
    CItemDescriptionDlg(QWidget* parent = nullptr,
        bool bEditName = true,
        bool bAllowAnyName = false,
        bool bLocation = false,
        bool bTemplate = false);
    virtual ~CItemDescriptionDlg();

    void setItemCaption(const QString& caption);
    void setItem(const QString& item);
    void setLocation(const QString& location);
    void setTemplateName(const QString& templateName);
    void setDescription(const QString& description);

    QString item() const;
    QString location() const;
    QString templateName() const;
    QString description() const;

    static bool ValidateItem(const QString& item);
    static bool ValidateLocation(const QString& location);

private:
    void OnInitDialog(bool bEditName, bool bLocation, bool bTemplate);
    void OnEnChangeItemedit();

    QScopedPointer<Ui::ItemDescriptionDialog> m_ui;

    bool m_bAllowAnyName;
    bool m_bLocation;
};

#endif // CRYINCLUDE_EDITOR_ITEMDESCRIPTIONDLG_H
