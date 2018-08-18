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
#include "QCurveEditorImp.h"
#include "AttributeItem.h"
#include "Utils.h"

// Qt
#include <QAction>
#include <QMenu>
#include <qmath.h>

// Cry
#include <Cry_Color.h>

// EditorCore
#include <Util/Variable.h>
#include "AttributeView.h"
#include "QGradientColorDialog.h"
#include "CurveEditorControl.h"
#include "CurveEditor.h"
#include "IEditorParticleUtils.h"

QCurveEditorImp::QCurveEditorImp(CAttributeItem* parent)
    : CCurveEditor(parent)
    , CBaseVariableWidget(parent)
    , m_pSpline(nullptr)
    , m_splineCache(nullptr)
{
    m_tooltip = new QToolTipWidget(this);
    EnforceTimeRange(0, 1);
    SetValueRange(0, 1);
    ZoomToTimeRange(-0.1f, 1.1f);
    ZoomToValueRange(-0.1f, 1.1f);

    int optOutFlags = 0;
    optOutFlags |= CCurveEditor::EOptOutFree;
    optOutFlags |= CCurveEditor::EOptOutStep;
    optOutFlags |= CCurveEditor::EOptOutSelectionInOutTangent;
    optOutFlags |= CCurveEditor::EOptOutRuler;
    optOutFlags |= CCurveEditor::EOptOutTimeSlider;
    //optOutFlags |= CCurveEditor::EOptOutBackground;
    optOutFlags |= CCurveEditor::EOptOutKeyIcon;
    optOutFlags |= CCurveEditor::EOptOutCustomPenColor;
    optOutFlags |= CCurveEditor::EOptOutZoomingAndPanning;
    SetOptOutFlags(optOutFlags);

    m_content.m_curves.push_back(SCurveEditorCurve());
    SCurveEditorCurve& curve = m_content.m_curves.back();
    curve.m_defaultValue = 0.5f;
    curve.m_bModified = true;

    // Make sure the curve has the same color as the rest of the palette
    const QColor& colorHighlight = palette().highlight().color();
    curve.m_color = ColorB(colorHighlight.red(), colorHighlight.green(), colorHighlight.blue());

    SetContent(&m_content);

    // This should be only retrieved once
    if (!m_pSpline)
    {
        m_pSpline = m_var->GetSpline();
        getCurve().m_customInterpolator = m_pSpline;
        syncToInterpolator();
    }

    //create a backup of the spline before the movement,
    //we need to do this because the movement will trick the undo into comparing the two most recent moves
    connect(this, &QCurveEditorImp::SignalKeyMoveStarted, this, [this]()
        {
            m_splineCache = m_pSpline->Backup();
        });

    // Updates internal spline, to make sure the curve is updated
    // also during dragging of the key
    connect(this, &QCurveEditorImp::SignalKeyMoved, this, &QCurveEditorImp::syncToInterpolator);

    // This event is triggered when the key is released
    // we restore the spline to it's pre-move state to ensure undo gets recorded properly
    connect(this, &QCurveEditorImp::SignalContentChanged, this, [=]()
        {
            if (m_splineCache)
            {
                m_pSpline->Restore(m_splineCache);
                m_splineCache->Release();
                m_splineCache = NULL;
            }
            syncToVar();
            update();
        });
    setPenColor(Qt::white);
}

QCurveEditorImp::~QCurveEditorImp()
{
}

void QCurveEditorImp::setVar(IVariable* var)
{
    CBaseVariableWidget::setVar(var);
    
    SelfCallFence(m_ignoreSetCallback);


    m_pSpline = m_var->GetSpline();
    getCurve().m_customInterpolator = m_pSpline;


    syncFromVar();
}

SCurveEditorKey& QCurveEditorImp::addKey(const float& time, const float& value)
{
    //removed the syncToVar and syncToInterpolator calls since they would overwrite the
    //keys stored in the spline. this function is called in syncFromVar and the combination of the two
    //would result in n number keys being assigned to the first key's value.
    SCurveEditorCurve& curve = m_content.m_curves.back();
    curve.m_keys.push_back(SCurveEditorKey());
    SCurveEditorKey& key = curve.m_keys.back();
    key.m_time = time;
    key.m_value = value;
    key.m_bModified = true;
    key.m_bAdded = true;
    key.m_inTangentType = SCurveEditorKey::eTangentType_Flat;
    key.m_outTangentType = SCurveEditorKey::eTangentType_Flat;
    getCurve().m_bModified = true;
    SortKeys(getCurve());
    return key;
}

void QCurveEditorImp::onVarChanged(IVariable* var)
{
    setVar(var);
    ContentChanged();
}

void QCurveEditorImp::clear()
{
    getCurve().m_keys.clear();
    getCurve().m_bModified = true;
    ContentChanged();
}

void QCurveEditorImp::syncToInterpolator()
{
    if (!m_pSpline)
    {
        return;
    }

    m_pSpline->ClearAllKeys();

    auto keys = getCurve().m_keys;
    for (int i = 0; i < keys.size(); i++)
    {
        SCurveEditorKey& key = keys[i];

        // Restrict only to flat or linear splines,
        // implementation of ISpline does not support custom tangents
        if (key.m_inTangentType != SCurveEditorKey::eTangentType_Linear)
        {
            key.m_inTangentType = SCurveEditorKey::eTangentType_Flat;
        }

        if (key.m_outTangentType != SCurveEditorKey::eTangentType_Linear)
        {
            key.m_outTangentType = SCurveEditorKey::eTangentType_Flat;
        }

        key.m_bModified = true;
        getCurve().m_bModified = true;

        const int keyId = m_pSpline->InsertKeyFloat(key.m_time, key.m_value);
        m_pSpline->SetKeyValueFloat(keyId, key.m_value);

        // construct flags
        int flags = 0;

        if (key.m_inTangentType == SCurveEditorKey::eTangentType_Linear)
        {
            flags |= (SPLINE_KEY_TANGENT_LINEAR << SPLINE_KEY_TANGENT_IN_SHIFT);
        }
        else
        {
            flags |= (SPLINE_KEY_TANGENT_NONE << SPLINE_KEY_TANGENT_IN_SHIFT);
        }

        if (key.m_outTangentType == SCurveEditorKey::eTangentType_Linear)
        {
            flags |= (SPLINE_KEY_TANGENT_LINEAR << SPLINE_KEY_TANGENT_OUT_SHIFT);
        }
        else
        {
            flags |= (SPLINE_KEY_TANGENT_NONE << SPLINE_KEY_TANGENT_OUT_SHIFT);
        }

        m_pSpline->SetKeyFlags(keyId, flags);
    }
    m_pSpline->Update();
}

void QCurveEditorImp::syncToVar()
{
    if (!m_var || !m_pSpline)
    {
        return;
    }

    SelfCallFence(m_ignoreSetCallback);

    //suspend undo so for widgets share this same IVariable won't trigger same undo multiple times
    EBUS_EVENT(EditorLibraryUndoRequestsBus, Suspend, true);

    syncToInterpolator();
    m_var->OnSetValue(false);
    m_parent->UILogicUpdateCallback();

    EBUS_EVENT(EditorLibraryUndoRequestsBus, Suspend, false);

    emit m_parent->SignalUndoPoint();
}

void QCurveEditorImp::addKey(const SCurveEditorKey& addKey)
{
    SCurveEditorCurve& curve = m_content.m_curves.back();
    curve.m_keys.push_back(addKey);
    curve.m_keys.back().m_bAdded = true;
    curve.m_keys.back().m_bModified = true;
    m_content.m_curves.back().m_bModified = true;

    SortKeys(m_content.m_curves.back());
}

SCurveEditorCurve const& QCurveEditorImp::getCurveConst() const
{
    return m_content.m_curves.back();
}

void QCurveEditorImp::SetCurve(SCurveEditorCurve const& curve)
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
    syncToInterpolator();
    ContentChanged();
}

SCurveEditorCurve& QCurveEditorImp::getCurve()
{
    return m_content.m_curves.back();
}

#define IS_TAN_IN(BITFIELD, FLAG_BIT) \
    ((((BITFIELD & SPLINE_KEY_TANGENT_IN_MASK) >> SPLINE_KEY_TANGENT_IN_SHIFT) & FLAG_BIT) == FLAG_BIT)

#define IS_TAN_OUT(BITFIELD, FLAG_BIT) \
    ((((BITFIELD & SPLINE_KEY_TANGENT_OUT_MASK) >> SPLINE_KEY_TANGENT_OUT_SHIFT) & FLAG_BIT) == FLAG_BIT)

void QCurveEditorImp::syncFromVar()
{
    if (!m_var || !m_pSpline)
    {
        return;
    }

    clear();
    int numKeys = m_pSpline->GetKeyCount();
    if (numKeys > 0)
    {
        for (int i = 0; i < numKeys; i++)
        {
            const float time = m_pSpline->GetKeyTime(i);
            float value;
            m_pSpline->GetKeyValueFloat(i, value);

            SCurveEditorKey& key = addKey(time, value);

            // Map tangent flags
            const int flags = m_pSpline->GetKeyFlags(i);

            if (IS_TAN_IN(flags, SPLINE_KEY_TANGENT_LINEAR))
            {
                key.m_inTangentType = SCurveEditorKey::eTangentType_Linear;
            }
            else
            {
                key.m_inTangentType = SCurveEditorKey::eTangentType_Flat;
            }

            if (IS_TAN_OUT(flags, SPLINE_KEY_TANGENT_LINEAR))
            {
                key.m_outTangentType = SCurveEditorKey::eTangentType_Linear;
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
    syncToInterpolator();
    ContentChanged();
}

void QCurveEditorImp::mouseMoveEvent(QMouseEvent* pEvent)
{
    if (m_optOutFlags & EOptOutControls)
    {
        return CCurveEditor::mouseMoveEvent(pEvent);
    }
    const CCurveEditorControl* control = HitDetectKey(pEvent->pos());
    if (control)
    {
        QString _t, _v;
        _t.setNum(control->GetKey().m_time);
        _v.setNum(control->GetKey().m_value);
        GetIEditor()->GetParticleUtils()->ToolTip_BuildFromConfig(m_tooltip, "Curve Editor", "Key", control->GetToolTip() + "\nt:" + _t + "\tv:" + _v + "\t");

        QPoint ep = pEvent->globalPos();
        QRect _rect = control->GetRect();
        QPoint p = mapToGlobal(_rect.topLeft());
        if (m_tooltip->isVisible() == false)
        {
            if (ep.x() <= p.x() + _rect.width() && ep.x() >= p.x() && ep.y() <= p.y() + _rect.height() && ep.y() >= p.y())
            {
                m_tooltip->Display(QRect(mapToGlobal(QPoint(0, 0)), size()), QToolTipWidget::ArrowDirection::ARROW_RIGHT);
            }
        }
        setCursor(QCursor(Qt::SizeAllCursor));
    }
    else
    {
        if (m_tooltip->isVisible())
        {
            m_tooltip->hide();
        }
        setCursor(QCursor());
    }

    if (m_pMouseHandler)
    {
        m_pMouseHandler->mouseMoveEvent(pEvent);
    }

    update();
}


void QCurveEditorImp::hideEvent(QHideEvent* e)
{
    if (m_tooltip)
    {
        m_tooltip->hide();
    }
    return CCurveEditor::hideEvent(e);
}

void QCurveEditorImp::leaveEvent(QEvent* e)
{
    if (m_tooltip)
    {
        m_tooltip->hide();
    }
    return CCurveEditor::leaveEvent(e);
}
