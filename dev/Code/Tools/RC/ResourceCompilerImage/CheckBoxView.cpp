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
#include "CheckBoxView.h"

#include <QCheckBox>

void CheckBoxView::OnTinyDocumentChanged(TinyDocument<bool>* pDocument)
{
    this->UpdateView();
}

CheckBoxView::CheckBoxView()
    :   pDocument(0)
    , check(nullptr)
{
}

CheckBoxView::~CheckBoxView()
{
    this->ClearDocument();
}

void CheckBoxView::SetDocument(TinyDocument<bool>* pDocument)
{
    this->ClearDocument();

    if (pDocument != 0)
    {
        this->pDocument = pDocument;
        this->pDocument->AddListener(this);
        this->UpdateView();
    }
}

void CheckBoxView::ClearDocument()
{
    if (this->pDocument != 0)
    {
        this->pDocument->RemoveListener(this);
        this->pDocument = 0;
    }
}


void CheckBoxView::OnChange()
{
    if (this->pDocument != 0)
    {
        const bool value = check->isChecked();

        this->pDocument->SetValue(value);
    }
}

void CheckBoxView::SubclassWidget(QCheckBox* hWnd)
{
    check = hWnd;

    this->UpdateView();

    QObject::connect(check, &QCheckBox::clicked, check, [&](){OnChange(); });
}

void CheckBoxView::UpdateView()
{
    if (this->check != 0 && this->pDocument != 0)
    {
        const bool value = this->pDocument->GetValue();
        check->setChecked(value);
    }
}
