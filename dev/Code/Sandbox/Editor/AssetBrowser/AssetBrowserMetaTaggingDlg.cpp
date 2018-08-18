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
#include "AssetBrowser/AssetBrowserMetaTaggingDlg.h"
#include "AssetBrowser/ui_AssetBrowserMetaTaggingDlg.h"
#include "AssetBrowser/AssetBrowserManager.h"
#include "Include/IAssetItem.h"

#include <QComboBox>
#include <QKeyEvent>
#include <QMessageBox>

CAutoCompleteEdit::CAutoCompleteEdit(QWidget* parent)
    : QLineEdit(parent)
{
}

CAutoCompleteEdit::~CAutoCompleteEdit()
{
}

///////////////////////////////////////////////////////

void CAutoCompleteEdit::keyPressEvent(QKeyEvent* event)
{
    switch (event->key())
    {
    case Qt::Key_Delete:
    {
        QString text = QLineEdit::text();
        int start = selectionStart(), end = selectionStart() + selectedText().length();
        if (selectedText().isEmpty())
	{
            start = cursorPosition();
            end = cursorPosition();
        }
        bool setSel = false;

        QString left = text.left(start);

        int idx = -1;

        if (start != end)
        {
            idx = text.length() - end;

            if (idx != 0)
            {
                setSel = true;
            }
        }
        else
        {
            if (start != text.length() - 1)
            {
                idx = text.length() - (start + 1);
                setSel = true;
            }
        }

        if (idx != -1)
        {
            QString right = text.right(idx);
            left.append(right);
        }

        setText(left);

        if (setSel)
        {
            setCursorPosition(start);
        }

        event->accept();
        return;
    }
    break;

    case Qt::Key_Backspace:
    {
        QString text = QLineEdit::text();
        int start = selectionStart(), end = selectionStart() + selectedText().length();
        if (selectedText().isEmpty())
        {
            start = cursorPosition();
            end = cursorPosition();
        }

        bool setSel = false;

        QString left = text.left(start - 1);
        QString right = text.right(text.length() - end);
        QString newString = left;

        if (start == text.length())
        {
            event->accept();
            // for some reason this is magically done in MFC...
            setText(newString);
            return;
        }
        else
        {
            if (end != text.length())
            {
                if (start != end)
                {
                    return;
                }
                else
                {
                    newString = text.left(start);
                    newString.append(right);
                    setSel = true;
                }
            }
        }

        setText(newString);

        if (setSel)
        {
            setSelection(start - 1, 1);
        }

        event->accept();
        return;
    }
    break;
    }

    QLineEdit::keyPressEvent(event);
}

// CAssetBrowserMetaTagging dialog

CAssetBrowserMetaTaggingDlg::CAssetBrowserMetaTaggingDlg(TAssetItems* assetlist, QWidget* pParent /*=NULL*/)
    : QDialog(pParent)
    , m_ui(new Ui::AssetBrowserMetaTaggingDlg)
{
    m_ui->setupUi(this);
    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);

    for (TAssetItems::iterator item = assetlist->begin(), end = assetlist->end(); item != end; ++item)
    {
        QString filename = (*item)->GetRelativePath() + (*item)->GetFilename();
        m_assetList.push_back(filename);
    }

    OnInitDialog();

    connect(m_ui->m_descriptionEdit, &QLineEdit::textEdited, this, &CAssetBrowserMetaTaggingDlg::OnEditChanged);
}


CAssetBrowserMetaTaggingDlg::CAssetBrowserMetaTaggingDlg(const QString& filename, QWidget* pParent)
    : QDialog(pParent)
    , m_ui(new Ui::AssetBrowserMetaTaggingDlg)
{
    m_ui->setupUi(this);
    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);

    m_assetList.push_back(filename);

    OnInitDialog();

    connect(m_ui->m_descriptionEdit, &QLineEdit::textEdited, this, &CAssetBrowserMetaTaggingDlg::OnEditChanged);
}

CAssetBrowserMetaTaggingDlg::~CAssetBrowserMetaTaggingDlg()
{
}

// CAssetBrowserMetaTagging message handlers
void CAssetBrowserMetaTaggingDlg::OnInitDialog()
{
    m_bAttemptAutocomplete = true;

    if (m_assetList.size() != 1)
    {
        return;
    }

    QString assetDescription;
    QString filename = m_assetList[0];

    if (CAssetBrowserManager::Instance()->GetAssetDescription(filename, assetDescription) && assetDescription.length() > 0)
    {
        m_ui->m_descriptionEdit->setText(assetDescription);
    }

    m_ui->m_filenameEdit->setText(filename);

    QStringList categories;
    CAssetBrowserManager::Instance()->GetAllTagCategories(categories);

    auto pAssetBrowserMgr = CAssetBrowserManager::Instance();

    QGridLayout* layout = qobject_cast<QGridLayout*>(m_ui->m_groupBox->layout());


    for (QStringList::iterator item = categories.begin(), end = categories.end(); item != end; ++item)
    {
        QLabel* pStatic = new QLabel(*item);
        pStatic->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
        QComboBox* pComboBox = new QComboBox();

        const int row = layout->rowCount();
        layout->addWidget(pStatic, row, 0);
        layout->addWidget(pComboBox, row, 1);

        QString categoryTag;
        pAssetBrowserMgr->GetTagForAssetInCategory(categoryTag, filename, (*item));

        QStringList tags;
        pAssetBrowserMgr->GetTagsForCategory((*item), tags);

        for (QStringList::iterator itemtag = tags.begin(), endtag = tags.end(); itemtag != endtag; ++itemtag)
        {
            pComboBox->addItem(*itemtag);

            if (QString::compare(*itemtag, categoryTag, Qt::CaseInsensitive) == 0)
            {
                pComboBox->setCurrentIndex(pComboBox->count() - 1);
            }
        }

        m_staticControls.push_back(pStatic);
        m_comboControls.push_back(pComboBox);
    }
}

void CAssetBrowserMetaTaggingDlg::accept()
{
    if (m_assetList.size() != 1)
    {
        return;
    }

    if (Validated() == false)
    {
        QMessageBox::critical(this, QString(), tr("Please ensure you have filled out the description and selected a tag from each category."));
        return;
    }

    auto pAssetBrowserMgr = CAssetBrowserManager::Instance();
    QString filename = m_assetList[0];
    QString description = m_ui->m_descriptionEdit->text();
    pAssetBrowserMgr->SetAssetDescription(filename, description);

    int count = m_comboControls.size();

    for (int idx = 0; idx < count; ++idx)
    {
        QString tagText;

        QString categoryText = m_staticControls[idx]->text();
        pAssetBrowserMgr->GetTagForAssetInCategory(tagText, filename, categoryText);
        pAssetBrowserMgr->RemoveTagFromAsset(tagText, categoryText, filename);

        tagText = m_comboControls[idx]->currentText();
        pAssetBrowserMgr->AddTagsToAsset(filename, tagText, categoryText);
    }

    QDialog::accept();
}

bool CAssetBrowserMetaTaggingDlg::Validated()
{
    QString description = m_ui->m_descriptionEdit->text();

    if (description.isEmpty())
    {
        return false;
    }

    int count = m_comboControls.size();

    for (int idx = 0; idx < count; ++idx)
    {
        QString categoryTag = m_comboControls[idx]->currentText();

        if (categoryTag.isEmpty())
        {
            return false;
        }
    }

    return true;
}

void CAssetBrowserMetaTaggingDlg::OnEditChanged()
{
    QString fullDescription;
    QString description = m_ui->m_descriptionEdit->text();

    if (!description.isEmpty() && m_bAttemptAutocomplete)
    {
        int start = 0, end = 0;

        if (CAssetBrowserManager::Instance()->GetAutocompleteDescription(description, fullDescription))
        {
            m_bAttemptAutocomplete = false;
            m_ui->m_descriptionEdit->setText(fullDescription);
            start = description.length();
            end = fullDescription.length();
            m_ui->m_descriptionEdit->setSelection(start, end - start);
            m_bAttemptAutocomplete = true;
        }
    }
}

#include <AssetBrowser/AssetBrowserMetaTaggingDlg.moc>