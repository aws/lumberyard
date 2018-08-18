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

#include "stdafx.h"
#include "FloatEditView.h"

#include <QLineEdit>

void FloatEditView::OnTinyDocumentChanged(TinyDocument<float>* pDocument)
{
    this->UpdateText();
}

FloatEditView::FloatEditView()
    :   pDocument(0)
    , edit(nullptr)
{
}

FloatEditView::~FloatEditView()
{
    this->ClearDocument();
}

void FloatEditView::SetDocument(TinyDocument<float>* pDocument)
{
    this->ClearDocument();

    if (pDocument != 0)
    {
        this->pDocument = pDocument;
        this->pDocument->AddListener(this);
        this->UpdateText();
    }
}

void FloatEditView::ClearDocument()
{
    if (this->pDocument != 0)
    {
        this->pDocument->RemoveListener(this);
        this->pDocument = 0;
    }
}

void FloatEditView::OnKillFocus()
{
    if (this->pDocument != 0)
    {
        QString szText = edit->text();

        bool ok;
        float fValue = szText.toFloat(&ok);
        if (ok)
        {
            if (fValue < this->pDocument->GetMin())
            {
                fValue = this->pDocument->GetMin();
            }
            if (fValue > this->pDocument->GetMax())
            {
                fValue = this->pDocument->GetMax();
            }
            this->pDocument->SetValue(fValue);
        }
    }
}

void FloatEditView::SubclassWidget(QLineEdit* hwnd)
{
    edit = hwnd;

    // Set the text.
    this->UpdateText();

    QObject::connect(edit, &QLineEdit::editingFinished, edit, [&]() {OnKillFocus();} );
    QObject::connect(edit, &QLineEdit::returnPressed, edit, [&]() {edit->nextInFocusChain()->setFocus(); });
}

void FloatEditView::UpdateText()
{
    if (this->edit != 0 && this->pDocument != 0)
    {
        // Update the text of the window.
        edit->setText(QString::number(pDocument->GetValue()));
    }
}
