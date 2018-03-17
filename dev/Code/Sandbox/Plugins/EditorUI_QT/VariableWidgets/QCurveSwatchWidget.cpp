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
#include "stdafx.h"
#include "QCurveSwatchWidget.h"
#include "CurveEditorContent.h"
#include <QPainter>
#include "SplineInterpolators.h"
#include "../Utils.h"

QCurveSwatchWidget::QCurveSwatchWidget(SCurveEditorContent content, QWidget* parent)
    : m_spline(new FinalizingOptSpline<float>())
{
    SetContent(content);
}

QCurveSwatchWidget::~QCurveSwatchWidget()
{
}

SCurveEditorCurve& QCurveSwatchWidget::GetCurve()
{
    return m_content.m_curves.back();
}

SCurveEditorContent QCurveSwatchWidget::GetContent()
{
    return m_content;
}

void QCurveSwatchWidget::SetContent(SCurveEditorContent content)
{
    m_content = content;
}

void QCurveSwatchWidget::showEvent(QShowEvent* event)
{
    // Defer initialization of content until the first showEvent
    InitializeContent();
}

void QCurveSwatchWidget::InitializeContent()
{
    if (m_contentInitialized || m_content.m_curves.size() == 0)
    {
        return;
    }

    auto keys = GetCurve().m_keys;
    for (int i = 0; i < keys.size(); i++)
    {
        SCurveEditorKey& key = keys[i];

        // Restrict only to flat or linear splines,
        // implementation of ISpline does not support custom tangents

        key.m_bModified = true;
        GetCurve().m_bModified = true;

        const int keyId = m_spline->InsertKeyFloat(key.m_time, key.m_value);
        m_spline->SetKeyValueFloat(keyId, key.m_value);

        // construct flags
        int flags = 0;

        if (key.m_inTangentType == SCurveEditorKey::eTangentType_Linear)
        {
            flags |= (SPLINE_KEY_TANGENT_LINEAR << SPLINE_KEY_TANGENT_IN_SHIFT);
        }
        else if (key.m_inTangentType == SCurveEditorKey::eTangentType_Bezier)
        {
            flags |= (SPLINE_KEY_TANGENT_CUSTOM << SPLINE_KEY_TANGENT_IN_SHIFT);
            EditorSetTangent(m_spline, keyId, &key, true);
        }
        else
        {
            flags |= (SPLINE_KEY_TANGENT_NONE << SPLINE_KEY_TANGENT_IN_SHIFT);
        }

        if (key.m_outTangentType == SCurveEditorKey::eTangentType_Linear)
        {
            flags |= (SPLINE_KEY_TANGENT_LINEAR << SPLINE_KEY_TANGENT_OUT_SHIFT);
        }
        else if (key.m_outTangentType == SCurveEditorKey::eTangentType_Bezier)
        {
            flags |= (SPLINE_KEY_TANGENT_CUSTOM << SPLINE_KEY_TANGENT_OUT_SHIFT);
            EditorSetTangent(m_spline, keyId, &key, false);
        }
        else
        {
            flags |= (SPLINE_KEY_TANGENT_NONE << SPLINE_KEY_TANGENT_OUT_SHIFT);
        }

        m_spline->SetKeyFlags(keyId, flags);
    }

    m_spline->Update();
    m_contentInitialized = true;    // only initialize content once
}

void QCurveSwatchWidget::paintEvent(QPaintEvent* event)
{
    if (m_content.m_curves.size() != 0)
    {
        SCurveEditorCurve curve = m_content.m_curves.back();
        QPainter painter(this);
        for (unsigned int i = 0; i < width(); i++)
        {
            float value;
            float time = (float)i / width();
            m_spline->InterpolateFloat(time, value);
            float h = height() - (value * height());
            QColor oldPen = painter.pen().color();

            //draw semi-transparent tip
            painter.setPen(QColor(oldPen.red(), oldPen.green(), oldPen.blue(), (int(h) + 1 - (h)) * 255));
            painter.drawPoint(i, h);

            painter.setPen(oldPen);
            painter.drawLine(QLine(i, std::min(int(h + 1), height()), i, height()));
        }
    }
    QWidget::paintEvent(event);
}

