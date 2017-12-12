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

#ifndef CRYINCLUDE_EDITOR_AICHARACTERSDIALOG_H
#define CRYINCLUDE_EDITOR_AICHARACTERSDIALOG_H

#pragma once

#ifdef USE_DEPRECATED_AI_CHARACTER_SYSTEM

#include <QDialog>
#include <ui_AiCharactersDialog.h>

class QStringListModel;

// CAICharactersDialog dialog

//////////////////////////////////////////////////////////////////////////
class CAICharactersDialog
    : public QDialog
{
    Q_OBJECT
public:
    CAICharactersDialog(QWidget* pParent = nullptr);   // standard constructor
    ~CAICharactersDialog();

    void SetAICharacter(const QString& chr);
    QString GetAICharacter() const { return m_aiCharacter; }

protected:
    void ReloadCharacters();

protected slots:
    void OnBnClickedEdit();
    void OnBnClickedReload();
    void OnLbnSelchange(const QModelIndex& current, const QModelIndex& previous);
    void OnLbnDblClk(const QModelIndex& index);

private:
    //////////////////////////////////////////////////////////////////////////
    // FIELDS
    //////////////////////////////////////////////////////////////////////////
    QString m_aiCharacter;
    QScopedPointer<Ui_CAICharactersDialog> m_ui;
    QStringListModel* m_model;
};
#endif

#endif // CRYINCLUDE_EDITOR_AICHARACTERSDIALOG_H
