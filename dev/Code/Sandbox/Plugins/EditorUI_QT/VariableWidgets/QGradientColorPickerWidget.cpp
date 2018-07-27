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
#include "QGradientColorPickerWidget.h"
#include "QCustomGradientWidget.h"
#include "ISplines.h"
#include "SplineInterpolators.h"
#include "CurveEditorContent.h"

#include <QGradientStops>
#include <CurveEditor.h>
#include <QBrush>

#include "qpolygon.h"
#include <QtMath>
#include "qtooltip.h"
#include "CurveEditorControl.h"
#include "QMenu"
#include "../ContextMenu.h"
#include "qlayoutitem.h"
#include "QCustomColorDialog.h"
#include "UIFactory.h"
#include "CurveEditorControl.h"
#include "../Utils.h"
#include "IEditorParticleUtils.h"

QGradientColorPickerWidget::QGradientColorPickerWidget(SCurveEditorContent content, QGradientStops gradientHueInfo, QWidget* parent)
    : QWidget(parent)
    , m_gradient(new QCustomGradientWidget(this))
    , m_spline(nullptr)
    , m_curveEditor(new CCurveEditor(this))
    , m_gradientMenuBtn(new QPushButton(this))
    , m_gradientMenuEdit(new QAmazonLineEdit(this))
    , m_gradientMenu(new ContextMenu(this))
    , m_gradientMenuBtnAction(new QWidgetAction(this))
    , m_gradientMenuEditAction(new QWidgetAction(this))
    , m_curveMenu(nullptr)
    , m_gradientSize(QSize(20, 12))
    , m_gradientKeyInactive(Qt::gray)
    , m_gradientKeyHovered(Qt::yellow)
    , m_gradientKeySelected(Qt::blue)
{
    //this rect will be updated on resize, however it is incorrect at this point we just need
    //something to fill in the gradients with
    QRect usableRegion = GetUsableRegion();
    for (unsigned int i = 0; i < gradientHueInfo.count(); i++)
    {
        m_hueStops.push_back(GradientKey(gradientHueInfo[i], usableRegion, m_gradientSize));
    }

    m_curveEditor->EnforceTimeRange(0, 1);
    m_curveEditor->SetValueRange(0, 1);
    m_curveEditor->ZoomToTimeRange(0, 1);
    m_curveEditor->ZoomToValueRange(0, 1);
    setLayout(&layout);

    m_curveMenu = new QMenu(this);

    m_gradientMenuBtn->installEventFilter(this);
    m_gradientMenuEdit->installEventFilter(this);
    m_gradientMenuBtn->setText(tr("Add To Library"));
    m_gradientMenuEditAction->setDefaultWidget(m_gradientMenuEdit);
    m_gradientMenuBtnAction->setDefaultWidget(m_gradientMenuBtn);

    connect(m_gradientMenuBtn, &QPushButton::pressed, this, [&] {
            if ((bool)callback_add_gradient_to_library)
            {
                callback_add_gradient_to_library(m_gradientMenuEdit->text());
            }
            m_gradientMenu->hide();
        });

    m_gradientMenu->addAction(m_gradientMenuEditAction);
    m_gradientMenu->addAction(m_gradientMenuBtnAction);

    int optOutFlags = 0;
    optOutFlags |= CCurveEditor::EOptOutFree;
    optOutFlags |= CCurveEditor::EOptOutStep;
    optOutFlags |= CCurveEditor::EOptOutRuler;
    optOutFlags |= CCurveEditor::EOptOutTimeSlider;
    optOutFlags |= CCurveEditor::EOptOutBackground;
    optOutFlags |= CCurveEditor::EOptOutDefaultTooltip;
    optOutFlags |= CCurveEditor::EOptOutFitCurvesContextMenuOptions;
    m_curveEditor->SetOptOutFlags(optOutFlags);

    //create alpha gradient
    m_gradient->AddGradient(new QLinearGradient(0, 0, 0, 1),
        QPainter::CompositionMode::CompositionMode_SourceOver);
    //create hue gradient to display when it is above the source
    m_gradient->AddGradient(new QLinearGradient(0, 0, 1, 0),
        QPainter::CompositionMode::CompositionMode_SourceAtop);

    m_content.m_curves.push_back(SCurveEditorCurve());
    SCurveEditorCurve& curve = m_content.m_curves.back();
    curve.m_defaultValue = 0.5f;
    curve.m_bModified = true;

    // Make sure the curve has the same color as the rest of the palette
    const QColor& colorHighlight = palette().highlight().color();
    curve.m_color = ColorB(colorHighlight.red(), colorHighlight.green(), colorHighlight.blue());
    m_curveEditor->SetContent(&m_content);
    m_curveEditor->SetHandlesVisible(false);
    m_curveEditor->SetRulerVisible(false);
    m_curveEditor->SetTimeSliderVisible(false);
    m_defaultCurve = m_content.m_curves.back();

    // This should be only retrieved once
    if (m_spline == nullptr)
    {
        m_spline = new FinalizingOptSpline<float>;
        m_curveEditor->Content()->m_curves.back().m_customInterpolator = m_spline;
        syncToInterpolator();
    }
    // Updates internal spline, to make sure the curve is updated
    // also during dragging of the key
    connect(m_curveEditor, &CCurveEditor::SignalKeyMoved, this, [=]()
        {
            syncToInterpolator();
            UpdateIcons();
            PassThroughtSignalSelectCurveKey();
        });

    // This event is triggered when the key is released
    connect(m_curveEditor, &CCurveEditor::SignalContentChanged, this, [=]()
        {
            syncToInterpolator();
            m_curveEditor->update();
            m_curveEditor->repaint();
            update();
            repaint();
            UpdateIcons();
            if ((bool)callback_curve_changed)
            {
                m_curveEditor->blockSignals(true);
                callback_curve_changed(m_content.m_curves.back());
                m_curveEditor->blockSignals(false);
            }
            PassThroughtSignalSelectCurveKey();
        });

    // This event is triggered when the curve key is selected
    //Get time(u) and value(v) from key
    connect(m_curveEditor, &CCurveEditor::SignalKeySelected, this, &QGradientColorPickerWidget::PassThroughtSignalSelectCurveKey);

    layout.addWidget(m_gradient, 0, 0, 1, 1);
    layout.addWidget(m_curveEditor, 0, 0, 1, 1);
    layout.setContentsMargins(0, 0, 0, 0);
    layout.addItem(new QSpacerItem(0, 16), 1, 0);
    m_gradient->stackUnder(m_curveEditor);
    m_curveEditor->setPenColor(Qt::white);
    m_gradient->SetBackgroundImage(":/particleQT/icons/color_btn_bkgn.png");
    OnGradientChanged();
    UpdateAlpha();
    UpdateIcons();

    m_curveEditor->installEventFilter(this);
    m_tooltip = new QToolTipWidget(this);
    m_dropShadow = new QGraphicsDropShadowEffect(m_curveEditor);
    m_dropShadow->setBlurRadius(4);
    m_dropShadow->setColor(Qt::black);
    m_dropShadow->setOffset(0);
    m_curveEditor->setGraphicsEffect(m_dropShadow);
    setMouseTracking(true);
}

QGradientColorPickerWidget::~QGradientColorPickerWidget()
{
}

void QGradientColorPickerWidget::SetCurve(SCurveEditorCurve curve)
{
    m_content.m_curves.back().m_keys.clear();
    if (curve.m_keys.size() == 0)
    {
        m_content.m_curves.back().m_defaultValue = curve.m_defaultValue;
    }
    for (SCurveEditorKey const& key : curve.m_keys)
    {
        addKey(key);
    }
    m_curveEditor->ContentChanged();
    UpdateIcons();
}

unsigned int QGradientColorPickerWidget::AddGradientStop(float stop, QColor color)
{
    m_gradient->AddGradientStop((unsigned int)Gradients::HUE_RANGE, stop, color);
    unsigned int index = m_hueStops.count();
    m_hueStops.push_back(GradientKey(QGradientStop(stop, color), GetUsableRegion(), m_gradientSize));
    //set the stop to the selected
    for (unsigned int i = 0; i < m_hueStops.count(); i++)
    {
        m_hueStops[i].SetSelected(false);
    }
    m_hueStops.back().SetSelected(true);
    return index;
}

void QGradientColorPickerWidget::RemoveGradientStop(const QGradientStop& stop)
{
    for (unsigned int i = 0; i < m_hueStops.count(); i++)
    {
        if (m_hueStops[i].GetStop().first == stop.first && m_hueStops[i].GetStop().second == stop.second)
        {
            m_hueStops.removeAt(i);
            OnGradientChanged();
            return;
        }
    }
}

SCurveEditorKey& QGradientColorPickerWidget::addKey(const float& time, const float& value)
{
    SCurveEditorCurve& curve = m_content.m_curves.back();
    curve.m_keys.push_back(SCurveEditorKey());
    SCurveEditorKey& key = curve.m_keys.back();
    key.m_time = time;
    key.m_value = value;
    key.m_bModified = true;
    key.m_bAdded = true;
    key.m_inTangentType = SCurveEditorKey::eTangentType_Bezier;
    key.m_outTangentType = SCurveEditorKey::eTangentType_Bezier;

    m_content.m_curves.back().m_bModified = true;

    m_curveEditor->SortKeys(m_content.m_curves.back());

    syncToInterpolator();
    return key;
}

SCurveEditorKey& QGradientColorPickerWidget::addKey(const SCurveEditorKey& addKey)
{
    SCurveEditorCurve& curve = m_content.m_curves.back();
    curve.m_keys.push_back(SCurveEditorKey());
    SCurveEditorKey& key = curve.m_keys.back();
    key = addKey;

    m_content.m_curves.back().m_bModified = true;

    m_curveEditor->SortKeys(m_content.m_curves.back());

    syncToInterpolator();
    return key;
}

void QGradientColorPickerWidget::syncToInterpolator()
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

    auto keys = m_content.m_curves.back().m_keys;
    for (int i = 0; i < keys.size(); i++)
    {
        SCurveEditorKey& key = keys[i];

        key.m_bModified = true;
        m_content.m_curves.back().m_bModified = true;

        ISplineInterpolator::ValueType t = { 0, 0, 0, 0 };
        t[0] = key.m_value;
        const int keyId = m_spline->InsertKey(key.m_time, t);
        m_spline->SetKeyValue(keyId, t);

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
    UpdateIcons();
}

SCurveEditorCurve QGradientColorPickerWidget::GetCurve()
{
    return m_content.m_curves.back();
}

SCurveEditorContent QGradientColorPickerWidget::GetContent()
{
    return m_content;
}

void QGradientColorPickerWidget::SetContent(SCurveEditorContent content)
{
    m_content.m_curves.back().m_keys = content.m_curves.back().m_keys;
    m_content.m_curves.back().m_defaultValue = content.m_curves.back().m_defaultValue;
    m_content.m_curves.back().m_bModified = true;
    m_curveEditor->SetContent(&m_content);
    syncToInterpolator();
}

void QGradientColorPickerWidget::SetGradientStops(QGradientStops stops)
{
    m_hueStops.clear();
    QRect usableRegion = GetUsableRegion();
    for (QGradientStop stop : stops)
    {
        m_hueStops.push_back(GradientKey(stop, usableRegion, m_gradientSize));
    }
    OnGradientChanged();
}

QCustomGradientWidget& QGradientColorPickerWidget::GetGradient()
{
    return *m_gradient;
}

QGradientStops QGradientColorPickerWidget::GetStops()
{
    QGradientStops stops;
    //loop through the stops and then sort them
    for (unsigned int i = 0; i < m_hueStops.count(); i++)
    {
        stops.push_back(m_hueStops[i].GetStop());
    }

    qSort(stops.begin(), stops.end(), [=](const QGradientStop& s1, const QGradientStop& s2) -> bool{ return s1.first < s2.first; });

    return stops;
}

void QGradientColorPickerWidget::UpdateAlpha()
{
    QGradientStops stops;
    stops.push_back(QGradientStop(0.0, QColor(0, 0, 0, int(255))));
    stops.push_back(QGradientStop(1.0, QColor(0, 0, 0, int(0))));
    m_gradient->SetGradientStops((unsigned int)Gradients::ALPHA_RANGE, stops);
}

void QGradientColorPickerWidget::paintEvent(QPaintEvent* event)
{
    UpdateIcons();
    QPainter painter(this);
    //draw gradient keys
    for (GradientKey stop : m_hueStops)
    {
        stop.Paint(painter, m_gradientKeyInactive, m_gradientKeyHovered, m_gradientKeySelected);
    }
}

QColor QGradientColorPickerWidget::GetKeyColor(unsigned int id)
{
    //use value to get alpha, and time for hue
    SCurveEditorCurve& curve = m_content.m_curves.back();
    if (id < curve.m_keys.size())
    {
        float value = curve.m_keys[id].m_value;
        float time = curve.m_keys[id].m_time;

        QColor result = GetHueAt(time);

        float alpha = value * 255;

        result.setAlpha(alpha);
        if (result.isValid())
        {
            return result;
        }
    }
    return QColor(255, 255, 255, 255);
}

QColor QGradientColorPickerWidget::GetHueAt(float x)
{
    if (m_hueStops.count() <= 0)
    {
        return Qt::white; // return default hue if no hue is selected
    }
    assert(x >= 0 && x <= 1.0f);
    QPropertyAnimation interpolator;
    const qreal resolution = width();
    interpolator.setEasingCurve(QEasingCurve::Linear);
    interpolator.setDuration(resolution);
    //use the sorted stops
    QGradientStops stops = GetStops();
    if (stops.first().first != 0)
    {
        interpolator.setKeyValueAt(0, stops.first().second);
    }
    if (stops.back().first != 1.0f)
    {
        interpolator.setKeyValueAt(1.0f, stops.back().second);
    }

    for (int i = 0; i < m_hueStops.count(); i++)
    {
        if (m_hueStops[i].GetStop().first == x)
        {
            return m_hueStops[i].GetStop().second;
        }
        interpolator.setKeyValueAt(m_hueStops[i].GetStop().first, m_hueStops[i].GetStop().second);
    }

    interpolator.setCurrentTime(x * resolution);

    QColor color = interpolator.currentValue().value<QColor>();

    return QColor(std::min(std::max(0, color.red()), 255),
        std::min(std::max(0, color.green()), 255),
        std::min(std::max(0, color.blue()), 255),
        std::min(std::max(0, color.alpha()), 255));
}

void QGradientColorPickerWidget::BuildCurveMenu(bool curveKeyMenu)
{
    m_curveMenu->clear();

    if (curveKeyMenu)
    {
        m_curveEditor->PopulateControlContextMenu(m_curveMenu);
        m_curveMenu->addSeparator();
    }

    QAction* action = m_curveMenu->addAction("Add Curve to presets");
    connect(action, &QAction::triggered, this, [&] {
            if ((bool)callback_add_curve_to_library)
            {
                callback_add_curve_to_library();
            }
        });
    action = m_curveMenu->addAction("Reset curve");
    connect(action, &QAction::triggered, this, [&] {
            SCurveEditorContent content = m_content;
            content.m_curves.clear();
            content.m_curves.push_back(m_defaultCurve);
            SetContent(content);
        });
}

void QGradientColorPickerWidget::UpdateIcons()
{
    //set key style
    m_curveEditor->SetIconImage(":/particleQT/icons/color_picker_placement_ico.png");
    m_curveEditor->SetIconFillMask(Qt::white);
    m_curveEditor->SetIconShapeMask(Qt::transparent);
    m_curveEditor->updateCurveKeyShapeColor();
    for (unsigned int i = 0; i < m_curveEditor->m_pControlKeys.size(); i++)
    {
        QColor currentKeyColor = GetKeyColor(i);
        m_curveEditor->SetIconFillColor(i, currentKeyColor);
        m_curveEditor->SetIconSize(i, 16);

        QString R, G, B, A, AA;
        int r, g, b, a;
        currentKeyColor.getRgb(&r, &g, &b, &a);
        m_curveEditor->SetIconToolTip(i, QString("R: %1\tG: %2\tB: %3\tA: %4").arg(r, 3).arg(g, 3).arg(b, 3).arg(int(a / 2.55), 3));
    }
}

void QGradientColorPickerWidget::SyncEditor()
{
#define IS_TAN_IN(BITFIELD, FLAG_BIT) \
    ((((BITFIELD & SPLINE_KEY_TANGENT_IN_MASK) >> SPLINE_KEY_TANGENT_IN_SHIFT) & FLAG_BIT) == FLAG_BIT)

#define IS_TAN_OUT(BITFIELD, FLAG_BIT) \
    ((((BITFIELD & SPLINE_KEY_TANGENT_OUT_MASK) >> SPLINE_KEY_TANGENT_OUT_SHIFT) & FLAG_BIT) == FLAG_BIT)
    if (!m_spline)
    {
        return;
    }

    m_content.m_curves.back().m_keys.clear();
    m_content.m_curves.back().m_bModified = true;
    m_spline = m_curveEditor->Content()->m_curves.back().m_customInterpolator;

    int numKeys = m_spline->GetKeyCount();
    if (numKeys > 0)
    {
        for (int i = 0; i < numKeys; i++)
        {
            const float time = m_spline->GetKeyTime(i);
            float value;
            m_spline->GetKeyValueFloat(i, value);
            SCurveEditorKey& key = addKey(time, value);

            // Map tangent flags
            const int flags = m_spline->GetKeyFlags(i);
            ISplineInterpolator::ValueType inTangent;
            ISplineInterpolator::ValueType outTangent;
            m_spline->GetKeyTangents(i, inTangent, outTangent);

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
    UpdateIcons();
}

float QGradientColorPickerWidget::GetAlphaAt(float y)
{
    float alpha = 0.0f;
    m_spline->InterpolateFloat(y, alpha);
    if (m_spline->GetKeyCount() == 0)
    {
        return m_content.m_curves.back().m_defaultValue;
    }
    return alpha;
}

bool QGradientColorPickerWidget::eventFilter(QObject* obj, QEvent* event)
{
    bool curveHit = false;
    if (obj == (QObject*)m_curveEditor)
    {
        if (event->type() == QEvent::Leave)
        {
            if (m_tooltip->isVisible())
            {
                m_tooltip->hide();
            }
        }
        if (event->type() == QEvent::Type::Wheel)
        {
            return true;
        }
        if (event->type() == QEvent::Type::MouseMove)
        {
            QMouseEvent* e = (QMouseEvent*)event;
            QPoint intPos(e->localPos().x(), e->localPos().y());
            if (m_curveEditor->HitDetectKey(intPos))
            {
                CCurveEditorControl* control = m_curveEditor->HitDetectKey(intPos);
                QString _t, _v;
                _t = QString::number(((double)control->GetKey().m_time), 'f', 4);
                _v = QString::number(((double)control->GetKey().m_value), 'f', 4);
                GetIEditor()->GetParticleUtils()->ToolTip_BuildFromConfig(m_tooltip, "Curve Editor", "Key", control->GetToolTip() + "\nu: " + _t + " \tv: " + _v + "\t");

                QPoint ep = e->globalPos();
                QRect _rect = control->GetRect();
                QPoint p = mapToGlobal(_rect.topLeft());
                if (ep.x() <= p.x() + _rect.width() && ep.x() >= p.x() && ep.y() <= p.y() + _rect.height() && ep.y() >= p.y())
                {
                    QToolTipWidget::ArrowDirection dir = QToolTipWidget::ArrowDirection::ARROW_LEFT;
                    QPoint _p = mapToGlobal(control->GetRect().topLeft());

                    m_tooltip->Display(QRect(_p, control->GetRect().size()), dir);
                }
                return false;
            }
            else if (m_tooltip->isVisible())
            {
                m_tooltip->hide();
            }
        }
        if (event->type() == QEvent::Type::MouseButtonPress)
        {
            QMouseEvent* e = (QMouseEvent*)event;
            CCurveEditorControl* pCurveKey = m_curveEditor->HitDetectKey(e->pos());
            if (pCurveKey)
            {
                m_curveEditor->SelectKey(pCurveKey, false);
            }

            QPoint intPos(e->localPos().x(), e->localPos().y());
            SCurveEditorCurve* curve = m_curveEditor->HitDetectCurve(intPos).first;
            CCurveEditorTangentControl* pCurveTangent = m_curveEditor->HitDetectTangent(e->pos());
            if (curve != nullptr || pCurveTangent != nullptr || pCurveKey != nullptr)
            {
                if (e->button() == Qt::RightButton)
                {
                    BuildCurveMenu(pCurveKey != nullptr);
                    m_curveMenu->exec(e->globalPos());
                    curveHit = true;
                    return true; //consume menu event
                }
            }
        }
        if (curveHit == false && event->type() == QEvent::Type::MouseButtonRelease)
        {
            QMouseEvent* e = (QMouseEvent*)event;
            if (e->button() == Qt::RightButton)
            {
                m_gradientMenu->exec(e->globalPos());
            }
        }
        if (event->type() == QEvent::Type::KeyPress)
        {
            // Delete event
            QKeyEvent* e = (QKeyEvent*)event;

            if (e->key() == Qt::Key_Delete)
            {
                for (unsigned int i = 0; i < m_hueStops.count(); i++)
                {
                    if (m_hueStops[i].IsSelected())
                    {
                        RemoveGradientStop(m_hueStops[i].GetStop());
                        if ((bool)callback_gradient_changed)
                        {
                            blockSignals(true);
                            callback_gradient_changed(GetStops());
                            blockSignals(false);
                        }
                    }
                }
                update();
            }
        }
        if (event->type() == QEvent::Type::MouseButtonDblClick)
        {
            QMouseEvent* e = (QMouseEvent*)event;
            if (e->button() == Qt::RightButton)
            {
                return false;
            }
            QPoint intPos(e->localPos().x(), e->localPos().y());
            if (m_curveEditor->HitDetectCurve(intPos).first != nullptr)
            {
                return false;
            }
            if (m_curveEditor->HitDetectKey(intPos))
            {
                return false;
            }
            if (m_curveEditor->HitDetectTangent(intPos))
            {
                return false;
            }
            //add selected color to gradient here
            QColor color = Qt::transparent;
            if ((bool)callback_color_required)
            {
                color = callback_color_required();
            }
            color.setAlpha(255);
            float x = e->localPos().x();
            float w = width();
            x /= w;
            int X = x * 1000;
            //for checking key existence
            bool keyExists = false;
            for (unsigned int i = 0; i < m_hueStops.count(); i++)
            {
                int Y = m_hueStops[i].GetStop().first * 1000;
                if (Y == X)
                {
                    m_hueStops.removeAt(i);
                    OnGradientChanged();
                    return false;
                }
            }
            m_gradient->AddGradientStop((unsigned int)Gradients::HUE_RANGE, QGradientStop(x, color));

            m_hueStops.push_back(GradientKey(QGradientStop(x, color), GetUsableRegion(), m_gradientSize));

            if ((bool)callback_gradient_changed)
            {
                blockSignals(true);
                callback_gradient_changed(GetStops());
                blockSignals(false);
            }

            if ((bool)callback_location_changed)
            {
                GradientKey* key = GetSelectedKey();
                if (key != nullptr)
                {
                    callback_location_changed(key->GetStop().first * 100);
                }
            }

            UpdateIcons();
        }
        obj->event(event);
    }

    return QWidget::eventFilter(obj, event);
}

void QGradientColorPickerWidget::mouseMoveEvent(QMouseEvent* event)
{
    int toolTipDigits = 3;
    int toolTipFloatDigits = 4;
    int digitBase = 10;
    QChar filler = '0';
    bool toolTipShouldBeHidden = true;
    //handle hovering over gradient keys, including showing tooltips
    for (unsigned int i = 0; i < m_hueStops.count(); i++)
    {
        if (m_hueStops[i].ContainsPoint(event->localPos().toPoint()))
        {
            m_hueStops[i].SetHovered(true);

            const float t = m_hueStops[i].GetStop().first;
            const float x = t * width();
            const QColor& c = m_hueStops[i].GetStop().second;
            QPoint pos = mapToGlobal(m_hueStops[i].LocalPos());
            GetIEditor()->GetParticleUtils()->ToolTip_BuildFromConfig(m_tooltip, "Gradient Editor", "Hue Key", QString("u: %1\nR: %2\tG: %3\tB:%4").
                    arg(t, toolTipFloatDigits, 'g', toolTipFloatDigits, filler).
                    arg(c.red(), toolTipDigits, digitBase, filler).
                    arg(c.green(), toolTipDigits, digitBase, filler).
                    arg(c.blue(), toolTipDigits, digitBase, filler));
            m_tooltip->Display(QRect(pos, m_gradientSize), QToolTipWidget::ArrowDirection::ARROW_UP);
            toolTipShouldBeHidden = false;
        }
        else
        {
            m_hueStops[i].SetHovered(false);
        }
    }

    if (m_tooltip->isVisible() && toolTipShouldBeHidden)
    {
        m_tooltip->hide();
    }

    GradientKey* key = GetSelectedKey();
    //move the selected key
    if (key != nullptr && m_leftDown)
    {
        QGradientStop stop = key->GetStop();
        stop.first = event->localPos().x() / width();

        // Clamp range
        stop.first = qMax(float(stop.first), 0.0f);
        stop.first = qMin(float(stop.first), 1.0f);
        if ((bool)callback_location_changed)
        {
            callback_location_changed(stop.first * 100);
        }
        key->SetStop(stop);
        OnGradientChanged();
    }
    else
    {
        //force an update to keep things looking clean
        update();
    }
    QWidget::mouseMoveEvent(event);
}

void QGradientColorPickerWidget::mousePressEvent(QMouseEvent* event)
{
    QWidget::mousePressEvent(event);

    switch (event->button())
    {
    case Qt::MouseButton::LeftButton:
    {
        m_leftDown = true;
        for (unsigned int i = 0; i < m_hueStops.count(); i++)
        {
            m_hueStops[i].SetSelected(false);
        }
        for (unsigned int i = 0; i < m_hueStops.count(); i++)
        {
            if (m_hueStops[i].IsHovered())
            {
                if (!m_hueStops[i].IsSelected())
                {
                    m_hueStops[i].SetSelected(true);
                    setCursor(Qt::SizeHorCursor);
                    if ((bool)callback_location_changed)
                    {
                        callback_location_changed(m_hueStops[i].GetStop().first * 100);
                    }
                    //force an update to keep things looking clean
                    update();
                    break;
                }
            }
        }
        break;
    }
    case Qt::MouseButton::RightButton:
    {
        m_rightDown = true;
        break;
    }
    default:
    {
        break;
    }
    }
}

void QGradientColorPickerWidget::mouseReleaseEvent(QMouseEvent* event)
{
    QWidget::mouseReleaseEvent(event);

    switch (event->button())
    {
    case Qt::MouseButton::LeftButton:
    {
        m_leftDown = false;
        setCursor(Qt::ArrowCursor);
        //force an update to keep things looking clean
        update();
        break;
    }
    case Qt::MouseButton::RightButton:
    {
        m_rightDown = false;
        break;
    }
    default:
    {
        break;
    }
    }
}


void QGradientColorPickerWidget::mouseDoubleClickEvent(QMouseEvent* event)
{
    // Delete event
    //assumption - if you double click something it will be selected before this event fires.
    GradientKey* key = GetSelectedKey();

    if (key != nullptr)
    {
        if (event->button() == Qt::MouseButton::RightButton)
        {
            RemoveGradientStop(key->GetStop());
        }
        else if (event->button() == Qt::MouseButton::LeftButton)
        {
            onPickColor(key);
        }
    }
}

void QGradientColorPickerWidget::leaveEvent(QEvent* event)
{
    if (m_tooltip->isVisible())
    {
        m_tooltip->hide();
    }
    for (unsigned int i = 0; i < m_hueStops.count(); i++)
    {
        m_hueStops[i].SetHovered(false);
    }
    //one last update to repaint the removal of the hover
    update();
}

void QGradientColorPickerWidget::resizeEvent(QResizeEvent* event)
{
    QWidget::resizeEvent(event);
    QRect usableRegion = GetUsableRegion();
    for (unsigned int i = 0; i < m_hueStops.count(); i++)
    {
        m_hueStops[i].SetUsableRegion(usableRegion);
    }
}

void QGradientColorPickerWidget::onPickColor(GradientKey* key)
{
    QCustomColorDialog* dlg = UIFactory::GetColorPicker(key->GetStop().second, false);
    //disable mouse tracking to prevent an infinite loop
    if (dlg->exec() == QDialog::Accepted)
    {
        QGradientStop stop = key->GetStop();
        stop.second = dlg->GetColor();
        key->SetStop(stop);
        OnGradientChanged();
    }
}

void QGradientColorPickerWidget::SetSelectedGradientPosition(int location)
{
    float loc = location / 100.0f;
    GradientKey* key = GetSelectedKey();
    if (key != nullptr)
    {
        key->SetTime(loc);
        OnGradientChanged();
    }
}

void QGradientColorPickerWidget::SetSelectedCurveKeyPosition(float u, float v)
{
    CCurveEditorControl* key = m_curveEditor->GetSelectedCurveKey();
    if (key)
    {
        key->GetKey().m_time = u;
        key->GetKey().m_value = v;
        m_curveEditor->ContentChanged();
    }
}

void QGradientColorPickerWidget::PassThroughtSignalSelectCurveKey(CCurveEditorControl* key /*= nullptr*/)
{
    CCurveEditorControl* updatedKey = (key == nullptr) ? m_curveEditor->GetSelectedCurveKey() : key;
    if (updatedKey)
    {
        emit SignalSelectCurveKey(updatedKey->GetKey().m_time, updatedKey->GetKey().m_value);
    }
}

void QGradientColorPickerWidget::SetCallbackLocationChanged(std::function<void(short)> callback)
{
    callback_location_changed = callback;
}

void QGradientColorPickerWidget::SetCallbackColorChanged(std::function<void(QColor)> callback)
{
    callback_color_changed = callback;
}

void QGradientColorPickerWidget::SetCallbackCurrentColorRequired(std::function<QColor()> callback)
{
    callback_color_required = callback;
}

void QGradientColorPickerWidget::SetCallbackGradientChanged(std::function<void(QGradientStops)> callback)
{
    callback_gradient_changed = callback;
}

void QGradientColorPickerWidget::SetCallbackCurveChanged(std::function<void(struct SCurveEditorCurve)> callback)
{
    callback_curve_changed = callback;
}

void QGradientColorPickerWidget::SetCallbackAddGradientToLibary(std::function<void(QString)> callback)
{
    callback_add_gradient_to_library = callback;
}

void QGradientColorPickerWidget::SetCallbackAddCurveToLibary(std::function<void()> callback)
{
    callback_add_curve_to_library = callback;
}

float QGradientColorPickerWidget::GetMaxAlpha()
{
    return alphaMax;
}

float QGradientColorPickerWidget::GetMinAlpha()
{
    return alphaMin;
}

QRect QGradientColorPickerWidget::GetUsableRegion()
{
    QPoint topLeft(0, height() - m_gradientSize.height());
    QPoint bottomRight(width(), height());
    return QRect(topLeft, bottomRight);
}

QGradientColorPickerWidget::GradientKey* QGradientColorPickerWidget::GetSelectedKey()
{
    for (unsigned int i = 0; i < m_hueStops.count(); i++)
    {
        if (m_hueStops[i].IsSelected())
        {
            return &m_hueStops[i];
        }
    }
    return nullptr;
}

void QGradientColorPickerWidget::OnGradientChanged()
{
    QGradientStops stops = GetStops();
    if (!signalsBlocked())
    {
        if (callback_gradient_changed)
        {
            //this callback will attempt to update the gradient,
            //so we block the signals to prevent an infinite loop
            blockSignals(true);
            callback_gradient_changed(stops);
            blockSignals(false);
        }
        m_gradient->SetGradientStops((unsigned int)Gradients::HUE_RANGE, stops);
        update();
    }
}

void QGradientColorPickerWidget::UpdateColors(const QMap<QString, QColor>& colorMap)
{
    m_gradientKeyInactive = colorMap.contains("CGradientHueKey") ? colorMap["CGradientHueKey"] : Qt::gray;
    m_gradientKeyHovered = colorMap.contains("CGradientHueKeyHovered") ? colorMap["CGradientHueKeyHovered"] : Qt::yellow;
    m_gradientKeySelected = colorMap.contains("CGradientHueKeySelected") ? colorMap["CGradientHueKeySelected"] : Qt::blue;
}

#pragma region GradientKeyFunctions
QGradientColorPickerWidget::GradientKey::GradientKey()
    : GradientKey(QGradientStop(0, Qt::white), QRect(0, 0, 0, 0))
{
}

QGradientColorPickerWidget::GradientKey::GradientKey(const GradientKey& other)
    : m_stop(other.m_stop)
    , m_usableRegion(other.m_usableRegion)
    , m_size(other.m_size)
    , m_isSelected(other.m_isSelected)
    , m_isHovered(other.m_isHovered)
{
}

QGradientColorPickerWidget::GradientKey::GradientKey(QGradientStop stop, QRect rect, QSize size /*= QSize(12.0f, 12.0f)*/)
    : m_stop(stop)
    , m_usableRegion(rect)
    , m_size(size)
    , m_isSelected(false)
    , m_isHovered(false)
{
}

bool QGradientColorPickerWidget::GradientKey::ContainsPoint(const QPoint& p)
{
    QPolygonF poly = createControlPolygon();
    return poly.containsPoint(p, Qt::FillRule::WindingFill);
}

void QGradientColorPickerWidget::GradientKey::Paint(QPainter& painter, const QColor& inactive, const QColor& hovered, const QColor& selected)
{
    QPen pen;
    QColor borderColor = inactive;
    if (m_isSelected)
    {
        borderColor = selected;
    }
    else if (m_isHovered)
    {
        borderColor = hovered;
    }
    painter.setRenderHint(QPainter::Antialiasing, true);
    pen.setWidth(2);
    painter.setBrush(m_stop.second);
    pen.setColor(borderColor);
    painter.setPen(pen);
    painter.drawPolygon(createControlPolygon());
    pen.setWidth(1);
    painter.setPen(pen);
    painter.setRenderHint(QPainter::Antialiasing, false);
}

QGradientStop QGradientColorPickerWidget::GradientKey::GetStop()
{
    return m_stop;
}

void QGradientColorPickerWidget::GradientKey::SetStop(QGradientStop val)
{
    m_stop = val;
}

bool QGradientColorPickerWidget::GradientKey::IsSelected() const
{
    return m_isSelected;
}

void QGradientColorPickerWidget::GradientKey::SetSelected(bool val)
{
    m_isSelected = val;
}

bool QGradientColorPickerWidget::GradientKey::IsHovered() const
{
    return m_isHovered;
}

void QGradientColorPickerWidget::GradientKey::SetHovered(bool val)
{
    m_isHovered = val;
}

QPolygonF QGradientColorPickerWidget::GradientKey::createControlPolygon()
{
    int offsetX = m_stop.first * m_usableRegion.width();
    int halfWidth = m_size.width() / 2;
    QVector<QPointF> vertex;
    //we're drawing a triangle, so we need 3 points
    vertex.reserve(3);
    vertex.push_back(QPointF(offsetX, m_usableRegion.top()));
    vertex.push_back(QPointF(offsetX - halfWidth, m_usableRegion.top() + m_size.height()));
    vertex.push_back(QPointF(offsetX + halfWidth, m_usableRegion.top() + m_size.height()));
    return QPolygonF(vertex);
}

void QGradientColorPickerWidget::GradientKey::SetUsableRegion(QRect rect)
{
    m_usableRegion = rect;
}

QPoint QGradientColorPickerWidget::GradientKey::LocalPos()
{
    QPolygonF poly = createControlPolygon();
    return poly.boundingRect().topLeft().toPoint();
}

void QGradientColorPickerWidget::GradientKey::SetTime(qreal val)
{
    m_stop.first = val;
}

#include <VariableWidgets/QGradientColorPickerWidget.moc>
