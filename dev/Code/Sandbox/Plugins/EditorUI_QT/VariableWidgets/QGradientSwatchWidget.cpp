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

#include "EditorUI_QT_Precompiled.h"
#include "QGradientSwatchWidget.h"
#include <QGradientStops>
#include <CurveEditor.h>
#include "QCustomGradientWidget.h"
#include "ISplines.h"
#include "qgridlayout.h"
#include "CurveEditorContent.h"
#include "qbrush.h"
#include "../Utils.h"

#include "SplineInterpolators.h"
#include "qpropertyanimation.h"


#define QTUI_SIMPLE_GRADIENT_PREVIEW_ALPHA_STEP 0.05f

QGradientSwatchWidget::QGradientSwatchWidget(SCurveEditorContent content, QGradientStops gradientHueInfo, QWidget* parent)
    : QWidget(parent)
    , m_gradient(new QCustomGradientWidget())
    , m_simpleGradient(new QCustomGradientWidget())
    , m_spline(NULL)
    , m_curveContent(new SCurveEditorContent())
{
    alphaMin = 1.0f;
    alphaMax = 0.0f;
    setLayout(&layout);
    m_curveEditor = new CCurveEditor(this);
    m_curveEditor->EnforceTimeRange(0, 1);
    m_curveEditor->SetValueRange(0, 1);
    m_curveEditor->ZoomToTimeRange(-0.1f, 1.1f);
    m_curveEditor->ZoomToValueRange(-0.1f, 1.1f);
    int optOutFlags = 0;
    optOutFlags |= CCurveEditor::EOptOutFree;
    optOutFlags |= CCurveEditor::EOptOutStep;
    //optOutFlags |= CCurveEditor::EOptOutSelectionKey;
    optOutFlags |= CCurveEditor::EOptOutRuler;
    optOutFlags |= CCurveEditor::EOptOutTimeSlider;
    optOutFlags |= CCurveEditor::EOptOutBackground;
    optOutFlags |= CCurveEditor::EOptOutControls;
    optOutFlags |= CCurveEditor::eOptOutDashedPath;


    //create alpha gradient
    m_gradient->AddGradient(new QLinearGradient(0, 0, 0, 1),
        QPainter::CompositionMode::CompositionMode_SourceOver);
    //create hue gradient to display when it is above the source
    m_gradient->AddGradient(new QLinearGradient(0, 0, 1, 0),
        QPainter::CompositionMode::CompositionMode_SourceAtop);
    //create simple gradient
    m_simpleGradient->AddGradient(new QLinearGradient(0, 0, 1, 0),
        QPainter::CompositionMode::CompositionMode_Source);

    m_curveEditor->SetOptOutFlags(optOutFlags);
    SetContent(content);

    m_curveEditor->SetContent(m_curveContent);
    m_curveEditor->SetHandlesVisible(false);
    m_curveEditor->SetRulerVisible(false);
    m_curveEditor->SetTimeSliderVisible(false);
    // This should be only retrieved once
    if (!m_spline)
    {
        m_spline = new FinalizingOptSpline<float>;
        GetCurve().m_customInterpolator = m_spline;
        syncToInterpolator();
    }

    m_hueStops = gradientHueInfo;
    m_curveEditor->setPenColor(Qt::white);
    m_gradient->SetBackgroundImage(":/particleQT/icons/color_btn_bkgn.png");
    m_simpleGradient->SetBackgroundImage(":/particleQT/icons/color_btn_bkgn.png");

    layout.addWidget(m_gradient, 0, 0, 1, 1);
    layout.addWidget(m_curveEditor, 0, 0, 1, 1);
    layout.addWidget(m_simpleGradient, 0, 0, 1, 1);
    layout.setContentsMargins(0, 0, 0, 0);
    m_gradient->stackUnder(m_curveEditor);
    SortStops();
    m_gradient->SetGradientStops(HUE_RANGE, m_hueStops);
    UpdateAlpha();
    setSimpleViewMode(true);
    UpdateSimpleGradient();
}

void QGradientSwatchWidget::SetCurve(ISplineInterpolator* spline)
{
#define IS_TAN_IN(BITFIELD, FLAG_BIT) \
    ((((BITFIELD & SPLINE_KEY_TANGENT_IN_MASK) >> SPLINE_KEY_TANGENT_IN_SHIFT) & FLAG_BIT) == FLAG_BIT)

#define IS_TAN_OUT(BITFIELD, FLAG_BIT) \
    ((((BITFIELD & SPLINE_KEY_TANGENT_OUT_MASK) >> SPLINE_KEY_TANGENT_OUT_SHIFT) & FLAG_BIT) == FLAG_BIT)
    if (!m_spline)
    {
        return;
    }

    GetCurve().m_keys.clear();
    GetCurve().m_bModified = true;
    m_curveEditor->ContentChanged();


    int numKeys = spline->GetKeyCount();
    if (numKeys > 0)
    {
        for (int i = 0; i < numKeys; i++)
        {
            const float time = spline->GetKeyTime(i);
            float value;
            spline->GetKeyValueFloat(i, value);
            SCurveEditorKey& key = addKey(time, value);

            ISplineInterpolator::ValueType inTangent;
            ISplineInterpolator::ValueType outTangent;
            spline->GetKeyTangents(i, inTangent, outTangent);

            // Map tangent flags
            const int flags = spline->GetKeyFlags(i);

            if (IS_TAN_IN(flags, SPLINE_KEY_TANGENT_LINEAR))
            {
                key.m_inTangentType = SCurveEditorKey::eTangentType_Linear;
            }
            else if (IS_TAN_IN(flags, SPLINE_KEY_TANGENT_CUSTOM))
            {
                key.m_inTangentType = SCurveEditorKey::eTangentType_Bezier;
                key.m_inTangent = Vec2(1.0f, inTangent[0]);
            }
            else
            {
                key.m_inTangentType = SCurveEditorKey::eTangentType_Flat;
            }

            if (IS_TAN_OUT(flags, SPLINE_KEY_TANGENT_LINEAR))
            {
                key.m_outTangentType = SCurveEditorKey::eTangentType_Linear;
            }
            else if (IS_TAN_OUT(flags, SPLINE_KEY_TANGENT_CUSTOM))
            {
                key.m_outTangentType = SCurveEditorKey::eTangentType_Bezier;
                key.m_outTangent = Vec2(1.0f, outTangent[0]);
            }
            else
            {
                key.m_outTangentType = SCurveEditorKey::eTangentType_Flat;
            }
        }
    }
    else
    {
        addKey(0.0f, 1.0f);
        addKey(1.0f, 1.0f);
    }

    m_curveEditor->ContentChanged();
    UpdateSimpleGradient();
}

unsigned int QGradientSwatchWidget::AddGradientStop(float stop, QColor color)
{
    m_gradient->AddGradientStop(HUE_RANGE, stop, color);
    unsigned int index = m_hueStops.count();
    m_hueStops.push_back(QGradientStop(stop, color));
    SortStops();
    return index;
}

void QGradientSwatchWidget::RemoveGradientStop(unsigned int stop)
{
    m_hueStops.removeAt(stop);
    SortStops();
    m_gradient->SetGradientStops(HUE_RANGE, m_hueStops);
}

void QGradientSwatchWidget::EditGradientStop(unsigned int stop, QColor color)
{
    m_hueStops[stop].second = color;
    SortStops();
    m_gradient->SetGradientStops(HUE_RANGE, m_hueStops);
}

SCurveEditorKey& QGradientSwatchWidget::addKey(const float& time, const float& value)
{
    SCurveEditorCurve& curve = m_curveContent->m_curves.back();
    curve.m_keys.push_back(SCurveEditorKey());
    SCurveEditorKey& key = curve.m_keys.back();
    key.m_time = time;
    key.m_value = value;
    key.m_bModified = true;
    key.m_bAdded = true;
    key.m_inTangentType = SCurveEditorKey::eTangentType_Flat;
    key.m_outTangentType = SCurveEditorKey::eTangentType_Flat;
    GetCurve().m_bModified = true;

    m_curveEditor->SortKeys(GetCurve());

    syncToInterpolator();

    m_curveEditor->ContentChanged();
    return key;
}

void QGradientSwatchWidget::syncToInterpolator()
{
    if (!m_spline)
    {
        return;
    }

    //Remove all the keys
    while (m_spline->GetKeyCount())
    {
        m_spline->RemoveKey(0);
    }
    m_spline->Update();

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
}

void QGradientSwatchWidget::paintEvent(QPaintEvent* event)
{
    return QWidget::paintEvent(event);
}

QGradientStops& QGradientSwatchWidget::GetStops()
{
    return m_hueStops;
}

QCustomGradientWidget& QGradientSwatchWidget::GetGradient()
{
    return *m_gradient;
}

SCurveEditorContent& QGradientSwatchWidget::GetContent()
{
    return *m_curveContent;
}

SCurveEditorCurve& QGradientSwatchWidget::GetCurve()
{
    return m_curveContent->m_curves.back();
}

QGradientSwatchWidget::~QGradientSwatchWidget()
{
}

void QGradientSwatchWidget::SetContent(SCurveEditorContent content, bool scale /*= true*/)
{
    assert(m_curveContent != nullptr);
    if (content.m_curves.size() == 0)
    {
        m_curveContent->m_curves.clear();
    }
    if (m_curveContent->m_curves.size() == 0)
    {
        m_curveContent->m_curves.push_back(SCurveEditorCurve());
        SCurveEditorCurve& curve = m_curveContent->m_curves.back();
        curve.m_defaultValue = 1.0f;
        curve.m_bModified = true;
        // Make sure the curve has the same color as the rest of the palette
        const QColor& colorHighlight = palette().highlight().color();
        curve.m_color = ColorB(colorHighlight.red(), colorHighlight.green(), colorHighlight.blue());
    }
    alphaMax = m_curveContent->m_curves.back().m_defaultValue;
    alphaMin = m_curveContent->m_curves.back().m_defaultValue;
    if (content.m_curves.size() > 0)
    {
        //get min and max values
        m_curveContent->m_curves.back() = content.m_curves.back();
        if (m_curveContent->m_curves.back().m_keys.size() == 0)
        {
            alphaMin = alphaMax = m_curveContent->m_curves.back().m_defaultValue * 255;
            m_curveEditor->SetContent(m_curveContent);
            UpdateAlpha();
            return;
        }
        if (m_curveContent->m_curves.back().m_keys.size() == 1)
        {
            alphaMin = alphaMax = m_curveContent->m_curves.back().m_keys[0].m_value * 255;
            m_curveEditor->SetContent(m_curveContent);
            UpdateAlpha();
            return;
        }
        for (SCurveEditorKey key : m_curveContent->m_curves.back().m_keys)
        {
            if (key.m_value < alphaMin)
            {
                alphaMin = key.m_value;
            }
            if (key.m_value > alphaMax)
            {
                alphaMax = key.m_value;
            }
        }
        if (scale)
        {
            float alphaRange = alphaMax - alphaMin;
            float scale = 1.0 / alphaRange;
            //rescale values so alphaRange = 1.0
            for (unsigned int i = 0; i < m_curveContent->m_curves.back().m_keys.size(); i++)
            {
                m_curveContent->m_curves.back().m_keys[i].m_value -= alphaMin;
                m_curveContent->m_curves.back().m_keys[i].m_value *= scale;
            }
        }
    }
    //set alphaMax and Min to real alpha values
    alphaMax *= 255.0;
    alphaMin *= 255.0;
    m_curveEditor->SetContent(m_curveContent);
    syncToInterpolator();
    UpdateAlpha();
}

ISplineInterpolator* QGradientSwatchWidget::GetSpline()
{
    return m_spline;
}

XmlNodeRef QGradientSwatchWidget::GetSplineXml()
{
    XmlNodeRef node;
    m_spline->SerializeSpline(node, false);
    return node;
}
void QGradientSwatchWidget::UpdateAlpha()
{
    QGradientStops stops;
    stops.push_back(QGradientStop(0.0, QColor(0, 0, 0, int(alphaMax))));
    stops.push_back(QGradientStop(1.0, QColor(0, 0, 0, int(alphaMin))));
    m_gradient->SetGradientStops(ALPHA_RANGE, stops);
    UpdateSimpleGradient();
}

void QGradientSwatchWidget::SetGradientStops(QGradientStops stops)
{
    m_hueStops = stops;
    m_gradient->SetGradientStops(HUE_RANGE, stops);
    UpdateSimpleGradient();
}

QColor QGradientSwatchWidget::GetHueAt(float x)
{
    assert(m_hueStops.count() > 0);
    assert(x >= 0 && x <= 1.0f);
    QPropertyAnimation interpolator;
    const qreal resolution = width();
    interpolator.setEasingCurve(QEasingCurve::Linear);
    interpolator.setDuration(resolution);
    if (m_hueStops[0].first != 0)
    {
        interpolator.setKeyValueAt(0, m_hueStops[0].second);
    }
    if (m_hueStops.back().first != 1.0f)
    {
        interpolator.setKeyValueAt(1.0f, m_hueStops.back().second);
    }

    for (int i = 0; i < m_hueStops.count(); i++)
    {
        if (m_hueStops[i].first == x)
        {
            return m_hueStops[i].second;
        }
        interpolator.setKeyValueAt(m_hueStops[i].first, m_hueStops[i].second);
    }

    interpolator.setCurrentTime(x * resolution);

    QColor color = interpolator.currentValue().value<QColor>();

    return QColor(std::min(std::max(0, color.red()), 255),
        std::min(std::max(0, color.green()), 255),
        std::min(std::max(0, color.blue()), 255),
        std::min(std::max(0, color.alpha()), 255));
}

void QGradientSwatchWidget::SortStops()
{
    QGradientStops orderedStops;
    float lowest = 1.1f, lastLowest = -1.0f;
    for (int i = 0; i < m_hueStops.count(); i++)
    {
        int indexOfLowest = -1;
        for (int j = 0; j < m_hueStops.count(); j++)
        {
            if (m_hueStops[j].first <= lowest)
            {
                if (m_hueStops[j].first > lastLowest)
                {
                    lowest = m_hueStops[j].first;
                    indexOfLowest = j;
                }
            }
        }
        if (indexOfLowest >= 0 && indexOfLowest < m_hueStops.count())
        {
            orderedStops.push_back(m_hueStops[indexOfLowest]);
            lastLowest = lowest;
        }
        lowest = 1.1f;
    }
    m_hueStops = orderedStops;
    UpdateSimpleGradient();
}

void QGradientSwatchWidget::UpdateSimpleGradient()
{
    if (m_hueStops.count() <= 0)
    {
        m_hueStops = { QGradientStop(0.0f, Qt::white), QGradientStop(1.0f, Qt::white) };
    }
    QGradientStops stops;
    QHash<float, int> stopsList;
    // Get curve keys
    for (float time = 0.f; time <= 1.f; time += QTUI_SIMPLE_GRADIENT_PREVIEW_ALPHA_STEP)
    {
        stopsList.insert(time, 1);
    }
    // Get Hue stops
    for (auto hue : m_hueStops)
    {
        stopsList.insert(hue.first, 1);
    }

    QHash<float, int>::iterator hashmap;
    for (hashmap = stopsList.begin(); hashmap != stopsList.end(); ++hashmap)
    {
        float time = hashmap.key();
        float _alpha = 0.0f;
        //get alpha
        _alpha = GetAlphaAt(time) * 255;

        QColor color = GetHueAt(time);
        color.setAlpha(_alpha);
        //get color
        if (stops.count() == 0 || stops.back().second != color)
        {
            stops.push_back(QGradientStop(time, color));
        }
    }
    m_simpleGradient->SetGradientStops(0, stops);
    repaint();
}

void QGradientSwatchWidget::setSimpleViewMode(bool val)
{
    displaySimpleGradient = val;
    if (displaySimpleGradient)
    {
        m_curveEditor->hide();
        m_gradient->hide();
        m_simpleGradient->show();
    }
    else
    {
        m_curveEditor->show();
        m_gradient->show();
        m_simpleGradient->hide();
        m_simpleGradient->setBaseSize(m_gradient->size());
    }
}

float QGradientSwatchWidget::GetAlphaAt(float x)
{
    if (m_curveContent->m_curves.back().m_keys.size() == 0)
    {
        return 1.0f;
    }
    assert(x >= 0 && x <= 1.0f);
    float alpha = 0.0f;
    if (!m_spline) // If there is no alpha spline. Create a default spline
    {
        QPropertyAnimation interpolator;
        const qreal resolution = width();
        interpolator.setEasingCurve(QEasingCurve::BezierSpline);
        interpolator.setDuration(resolution);
        if (m_curveContent->m_curves.back().m_keys[0].m_time != 0)
        {
            interpolator.setKeyValueAt(0, m_curveContent->m_curves.back().m_keys[0].m_value);
        }
        if (m_curveContent->m_curves.back().m_keys.back().m_time != 1.0f)
        {
            interpolator.setKeyValueAt(1.0f, m_curveContent->m_curves.back().m_keys.back().m_value);
        }
        for (int i = 0; i < m_curveContent->m_curves.back().m_keys.size(); i++)
        {
            if (m_curveContent->m_curves.back().m_keys[i].m_time == x)
            {
                return m_curveContent->m_curves.back().m_keys[i].m_value;
            }
            interpolator.setKeyValueAt(m_curveContent->m_curves.back().m_keys[i].m_time, m_curveContent->m_curves.back().m_keys[i].m_value);
        }

        interpolator.setCurrentTime(x * resolution);

        alpha = interpolator.currentValue().value<float>();
        return alpha;
    }
    else //Get alpha value from spline
    {
        m_spline->InterpolateFloat(x, alpha);
        if (m_spline->GetKeyCount() == 0)
        {
            return m_curveContent->m_curves.back().m_defaultValue;
        }
    }
    return alpha;
}

#include <VariableWidgets/QGradientSwatchWidget.moc>
