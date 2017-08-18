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
#include "EditSliderPair.h"

void CEditSliderPair::SubclassWidgets(QSlider* hwndSlider, QLineEdit* hwndEdit)
{
    m_slider.SetDocument(&m_value);
    m_slider.SubclassWidget(hwndSlider);
    m_slider.UpdatePosition();
    m_edit.SetDocument(&m_value);
    m_edit.SubclassWidget(hwndEdit);
    m_edit.UpdateText();
}

void CEditSliderPair::SetValue(float fValue)
{
    m_value.SetValue(fValue);
}

float CEditSliderPair::GetValue() const
{
    return m_value.GetValue();
}

void CEditSliderPair::SetRange(float fMin, float fMax)
{
    m_value.SetMin(fMin);
    m_value.SetMax(fMax);
}

TinyDocument<float>& CEditSliderPair::GetDocument()
{
    return m_value;
}
