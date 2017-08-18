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

// Description : implementation file


#include "StdAfx.h"
#include "ItemDescriptionDlg.h"
#include "SmartObjects/SmartObjectsEditorDialog.h"

#include <QPushButton>
#include <QRegularExpression>

#include "ui_ItemDescriptionDlg.h"

// CItemDescriptionDlg dialog

CItemDescriptionDlg::CItemDescriptionDlg(QWidget* parent /*=nullptr*/, bool bEditName /*=true*/,
    bool bAllowAnyName /*=false*/, bool bLocation /*=false*/, bool bTemplate /*=false*/)
    : QDialog(parent)
    , m_ui(new Ui::ItemDescriptionDialog)
    , m_bAllowAnyName(bAllowAnyName)
    , m_bLocation(bLocation)
{
    m_ui->setupUi(this);
    OnInitDialog(bEditName, bLocation, bTemplate);
}

CItemDescriptionDlg::~CItemDescriptionDlg()
{
}

void CItemDescriptionDlg::setItemCaption(const QString& caption)
{
    m_ui->itemLabel->setText(caption);
}

void CItemDescriptionDlg::setItem(const QString& item)
{
    m_ui->itemEdit->setText(item);
}

void CItemDescriptionDlg::setLocation(const QString& location)
{
    m_ui->locationEdit->setText(location);
}

void CItemDescriptionDlg::setTemplateName(const QString& templateName)
{
    int index = m_ui->templateCombo->findText(templateName, Qt::MatchFixedString);
    if (index == -1)
    {
        m_ui->templateCombo->addItem(templateName);
        index = m_ui->templateCombo->count() - 1;
    }
    m_ui->templateCombo->setCurrentIndex(index);
}

void CItemDescriptionDlg::setDescription(const QString& description)
{
    m_ui->descriptionTextEdit->setPlainText(description);
}

QString CItemDescriptionDlg::item() const
{
    return m_ui->itemEdit->text();
}

QString CItemDescriptionDlg::location() const
{
    return m_ui->locationEdit->text();
}

QString CItemDescriptionDlg::templateName() const
{
    return m_ui->templateCombo->currentText();
}

QString CItemDescriptionDlg::description() const
{
    return m_ui->descriptionTextEdit->toPlainText();
}

void CItemDescriptionDlg::OnEnChangeItemedit()
{
    m_ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(
        (m_bAllowAnyName || ValidateItem(m_ui->itemEdit->text())) && (!m_bLocation || ValidateLocation(m_ui->locationEdit->text())));
}

// CItemDescriptionDlg message handlers

bool CItemDescriptionDlg::ValidateItem(const QString& item)
{
    static const QRegularExpression expression(R"(^[a-z_]\w*$)", QRegularExpression::CaseInsensitiveOption);
    return expression.match(item).hasMatch();
}

bool CItemDescriptionDlg::ValidateLocation(const QString& location)
{
    // Leading & trailing '\' '/' or ' '.
    // Tokens can start with '0', a letter or '_'. After that, they are made of \w or ' ' but can't end with a space.
    // Tokens are separated by '\' or '/'.
    // Empty inputs are allowed (but so are those consisting just of '\', '/' or ' ' !?).
    static const QRegularExpression expression(R"(^[\\/ ]*([0a-z_]([ \w]*\w)?)?([\\/][0a-z_]([ \w]*\w)?)*[\\/ ]*$)", QRegularExpression::CaseInsensitiveOption);
    return expression.match(location).hasMatch();
}

void CItemDescriptionDlg::OnInitDialog(bool bEditName, bool bLocation, bool bTemplate)
{
    m_ui->itemEdit->setEnabled(bEditName);

    if (bEditName)
    {
        connect(m_ui->itemEdit, &QLineEdit::textChanged, this, &CItemDescriptionDlg::OnEnChangeItemedit);
    }

    if (bLocation)
    {
        connect(m_ui->locationEdit, &QLineEdit::textChanged, this, &CItemDescriptionDlg::OnEnChangeItemedit);
    }
    else
    {
        m_ui->locationLabel->hide();
        m_ui->locationEdit->hide();
    }

    if (bTemplate)
    {
        m_ui->templateCombo->addItem(QString());
        CSOLibrary::VectorClassTemplateData& templates = CSOLibrary::GetClassTemplates();
        for (const auto& t : templates)
        {
            m_ui->templateCombo->addItem(t.name.c_str());
        }
    }
    else
    {
        m_ui->templateLabel->hide();
        m_ui->templateCombo->hide();
    }
}

#include <ItemDescriptionDlg.moc>
