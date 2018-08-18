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
#include "SliderView.h"

#include <QSlider>

void SliderView::OnTinyDocumentChanged(TinyDocument<float>* pDocument)
{
    this->UpdatePosition();
}

void SliderView::UpdatePosition()
{
    float fValue = pDocument->GetValue();
    fValue -= pDocument->GetMin();
    fValue /= pDocument->GetMax() - pDocument->GetMin();
    slider->setValue(fValue * 1000.0f);
}

void SliderView::OnScroll()
{
    if (this->pDocument != 0)
    {
        float fValue = slider->value() / 1000.0f;
        fValue *= this->pDocument->GetMax() - this->pDocument->GetMin();
        fValue += this->pDocument->GetMin();
        this->pDocument->SetValue(fValue);
    }
}

SliderView::SliderView()
    :   pDocument(0)
    , slider(nullptr)
{
}

SliderView::~SliderView()
{
    this->ClearDocument();
}

void SliderView::SetDocument(TinyDocument<float>* pDocument)
{
    this->ClearDocument();

    if (pDocument != 0)
    {
        this->pDocument = pDocument;
        this->pDocument->AddListener(this);
    }
}

void SliderView::ClearDocument()
{
    if (this->pDocument != 0)
    {
        this->pDocument->RemoveListener(this);
        this->pDocument = 0;
    }
}

void SliderView::SubclassWidget(QSlider* hWnd)
{
    // Call the base class implementation.
    slider = hWnd;

    // Set the maximum value.
    slider->setRange(0, 1000);

    QObject::connect(slider, &QSlider::sliderMoved, slider, [this]() { OnScroll(); });
}
