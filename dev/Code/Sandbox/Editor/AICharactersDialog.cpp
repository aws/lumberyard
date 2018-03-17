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

#include "StdAfx.h"
#ifdef USE_DEPRECATED_AI_CHARACTER_SYSTEM
#include "AICharactersDialog.h"

#include "AI/AIManager.h"
#include "AI/AiGoalLibrary.h"
#include "AI/AiBehaviorLibrary.h"

#include <QStringListModel>

// CAICharactersDialog dialog

CAICharactersDialog::CAICharactersDialog(QWidget* pParent /*=nullptr*/)
    : QDialog(pParent)
    , m_ui(new Ui_CAICharactersDialog)
    , m_model(new QStringListModel(this))
{
    m_ui->setupUi(this);

    m_ui->listView->setModel(m_model);

    connect(m_ui->listView->selectionModel(), &QItemSelectionModel::currentChanged,
        this, &CAICharactersDialog::OnLbnSelchange);
    connect(m_ui->listView, &QListView::doubleClicked,
        this, &CAICharactersDialog::OnLbnDblClk);
    connect(m_ui->editBtn, &QPushButton::clicked,
        this, &CAICharactersDialog::OnBnClickedEdit);
    connect(m_ui->refreshBtn, &QPushButton::clicked,
        this, &CAICharactersDialog::OnBnClickedReload);

    setWindowTitle(tr("AI Characters"));
    m_ui->newBtn->setEnabled(false);
    m_ui->deleteBtn->setEnabled(false);
    m_ui->listViewLabel->setText(tr("&Choose AI character:"));
    m_ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(false);

    ReloadCharacters();
}

CAICharactersDialog::~CAICharactersDialog()
{
}

// CAICharactersDialog message handlers

void CAICharactersDialog::OnLbnDblClk(const QModelIndex& index)
{
    if (index.isValid())
    {
        accept();
    }
}

void CAICharactersDialog::ReloadCharacters()
{
    QStringList chars;

    CAIBehaviorLibrary* lib = GetIEditor()->GetAI()->GetBehaviorLibrary();
    std::vector<CAICharacterPtr> characters;
    lib->GetCharacters(characters);
    for (int i = 0; i < characters.size(); i++)
    {
        chars.append(QtUtil::ToQString(characters[i]->GetName()));
    }

    m_model->setStringList(chars);

    // select aiCharacter
    QModelIndexList indexes =
        m_model->match(m_model->index(0, 0), Qt::DisplayRole, m_aiCharacter, Qt::MatchFixedString);
    if (!indexes.isEmpty())
    {
        m_ui->listView->setCurrentIndex(indexes.first());
    }
}

void CAICharactersDialog::SetAICharacter(const QString& character)
{
    m_aiCharacter = character;
}

void CAICharactersDialog::OnBnClickedEdit()
{
    CAICharacter* character = GetIEditor()->GetAI()->GetBehaviorLibrary()->FindCharacter(QtUtil::ToCString(m_aiCharacter));
    if (!character)
    {
        return;
    }

    character->Edit();
}

void CAICharactersDialog::OnBnClickedReload()
{
    ReloadCharacters();
    CAICharacter* character = GetIEditor()->GetAI()->GetBehaviorLibrary()->FindCharacter(QtUtil::ToCString(m_aiCharacter));
    if (!character)
    {
        return;
    }
    character->ReloadScript();
}

void CAICharactersDialog::OnLbnSelchange(const QModelIndex& current, const QModelIndex& previous)
{
    m_ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(true);

    if (!current.isValid())
    {
        {
            return;
        }

        m_aiCharacter = current.data().toString();

        CAICharacter* character = GetIEditor()->GetAI()->GetBehaviorLibrary()->FindCharacter(QtUtil::ToCString(m_aiCharacter));
        if (character)
        {
            m_ui->descriptionEdit->setPlainText(QtUtil::ToQString(character->GetDescription()));
        }
        else
        {
            m_ui->descriptionEdit->setPlainText(QString::Null());
        }
    }

#include <AICharactersDialog.moc>

#endif
