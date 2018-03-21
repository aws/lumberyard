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
#include "TrackViewNewSequenceDialog.h"
#include "TrackView/TrackViewSequenceManager.h"
#include "Maestro/Types/SequenceType.h"
#include <ui_TrackViewNewSequenceDialog.h>
#include <QMessageBox>
#include "QtUtil.h"

namespace
{
    struct seqTypeComboPair
    {
        const char*    name;
        SequenceType   type;
    };

    seqTypeComboPair g_seqTypeComboPairs[] = {
        { "Object Entity Sequence (Legacy)", SequenceType::Legacy },
        { "Component Entity Sequence (PREVIEW)", SequenceType::SequenceComponent }
    };
}

// TrackViewNewSequenceDialog dialog
CTVNewSequenceDialog::CTVNewSequenceDialog(QWidget* parent)
    : QDialog(parent)
    , ui(new Ui::CTVNewSequenceDialog)
    , m_inputFocusSet(false)
{
    ui->setupUi(this);
    connect(ui->buttonBox, &QDialogButtonBox::accepted, this, &CTVNewSequenceDialog::OnOK);
    connect(ui->NAME, &QLineEdit::returnPressed, this, &CTVNewSequenceDialog::OnOK);
    setWindowTitle("Add New Sequence");

    OnInitDialog();
}

CTVNewSequenceDialog::~CTVNewSequenceDialog()
{
}

void CTVNewSequenceDialog::OnInitDialog()
{
    // Fill in seq type combo box
    for (int i = 0; i < arraysize(g_seqTypeComboPairs); ++i)
    {
        ui->m_seqNodeTypeCombo->addItem(tr(g_seqTypeComboPairs[i].name));
    }
    ui->m_seqNodeTypeCombo->setCurrentIndex(static_cast<int>(SequenceType::SequenceComponent));         // default choice is the Director Component Entity
}

void CTVNewSequenceDialog::OnOK()
{
    m_sequenceType = static_cast<SequenceType>(ui->m_seqNodeTypeCombo->currentIndex());

    m_sequenceName = ui->NAME->text();

    if (m_sequenceName.isEmpty())
    {
        QMessageBox::warning(this, "New Sequence", "A sequence name cannot be empty!");
        return;
    }
    else if (m_sequenceName.contains('/'))
    {
        QMessageBox::warning(this, "New Sequence", "A sequence name cannot contain a '/' character!");
        return;
    }
    else if (m_sequenceName == LIGHT_ANIMATION_SET_NAME)
    {
        QMessageBox::warning(this, "New Sequence", QString("The sequence name '%1' is reserved.\n\nPlease choose a different name.").arg(LIGHT_ANIMATION_SET_NAME));
        return;
    }

    for (int k = 0; k < GetIEditor()->GetSequenceManager()->GetCount(); ++k)
    {
        CTrackViewSequence* pSequence = GetIEditor()->GetSequenceManager()->GetSequenceByIndex(k);
        QString fullname = QtUtil::ToQString(pSequence->GetName());

        if (fullname.compare(m_sequenceName, Qt::CaseInsensitive) == 0)
        {
            QMessageBox::warning(this, "New Sequence", "Sequence with this name already exists!");
            return;
        }
    }

    accept();
}

void CTVNewSequenceDialog::showEvent(QShowEvent* event)
{
    if (!m_inputFocusSet)
    {
        ui->NAME->setFocus();
        m_inputFocusSet = true;
    }

    QDialog::showEvent(event);
}

#include <TrackViewNewSequenceDialog.moc>
