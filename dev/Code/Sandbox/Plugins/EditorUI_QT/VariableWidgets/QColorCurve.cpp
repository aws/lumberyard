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
#include "QColorCurve.h"
#include "../UIUtils.h"
#include "AttributeItem.h"
#include "Utils.h"

// EditorCore
#include <Util/Variable.h>
#include "CurveEditorContent.h"
#include "QGradientColorDialog.h"
#include "AttributeView.h"
#include "../ContextMenu.h"
#include "IEditorParticleUtils.h"
#include "UIFactory.h"
#include "Utils.h"

QColorCurve::QColorCurve(CAttributeItem* parent, QGradientSwatchWidget* swatch, QGradientSwatchWidget* swatch1)
    : CBaseVariableWidget(parent)
    , m_pSpline(nullptr)
    , m_pSpline1(nullptr)
    , m_alphaSpline(nullptr)
    , m_alphaSpline1(nullptr)
    , m_swatch(swatch)
    , m_swatch1(swatch1)
    , alphaVar(nullptr)
    , m_menu(new ContextMenu(this))
    , m_content(nullptr)
    , m_VarEnableVar1(nullptr)
    , m_Var1(nullptr)
{
    m_pSpline = m_var->GetSpline();

    auto relations = parent->GetRelations();
    TrySetupAlphaCurve(relations["alpha"], 0);
    TrySetupSecondCurve(relations["randomGradient"], relations["randomGradientEnable"]);
    TrySetupSecondAlphaCurve(relations["randomAlpha"], relations["randomAlphaEnable"]);

    m_swatch->SetCurve(m_pSpline);
    m_swatch->show();
    m_swatch->setMinimumHeight(10);

    if (m_pSpline1 == nullptr)
    {
        m_pSpline1 = m_var->GetSpline();
    }

    m_swatch1->SetCurve(m_pSpline1);
    m_swatch1->hide();
    m_swatch1->setMinimumHeight(10);

    buildMenu(false);

    layout.addWidget(m_swatch, Qt::AlignTop);
    layout.addWidget(m_swatch1, Qt::AlignBottom);
    layout.setContentsMargins(0, 0, 0, 1);
    setLayout(&layout);

    callback_update_gradient = [&](QGradientStops stops)
        {
            UpdateGradient(stops, 0);
            UpdateGradient(stops, 1);
        };

    m_defaultStops = swatch->GetStops();
    m_defaultCurve = swatch->GetContent().m_curves.back();

    m_defaultStops1 = swatch1->GetStops();
    m_defaultCurve1 = swatch1->GetContent().m_curves.back();


    UpdateGradient(m_defaultStops, 0);
    UpdateGradient(m_defaultStops1, 1);
    m_swatch->installEventFilter(this);
    m_swatch1->installEventFilter(this);
    setMinimumHeight(25);
    setMinimumWidth(65);
    //m_swatch->setSimpleViewMode(true);
    //m_swatch1->setSimpleViewMode(true);
}

QColorCurve::~QColorCurve()
{
}

bool QColorCurve::TrySetupSecondCurve(QString targetName, QString targetEnableName)
{
    auto funcSet = functor(*this, &CBaseVariableWidget::onVarChanged);
    if (targetName.size() && targetEnableName.size())
    {
        m_VarEnableVar1 = m_parent->getAttributeView()->GetVarFromPath(targetEnableName);
        m_Var1 = m_parent->getAttributeView()->GetVarFromPath(targetName);
        if (m_VarEnableVar1 == nullptr || m_Var1 == nullptr)
        {
            m_VarEnableVar1 = m_Var1 = nullptr;
            m_pSpline1 = nullptr;
            return false;
        }

        m_Var1->AddOnSetCallback(funcSet);
        addOnDestructionEvent([funcSet, this](CBaseVariableWidget* self) { m_Var1->RemoveOnSetCallback(funcSet); });
        m_pSpline1 = m_Var1->GetSpline();

        m_VarEnableVar1->AddOnSetCallback(funcSet);
        addOnDestructionEvent([funcSet, this](CBaseVariableWidget* self) { m_VarEnableVar1->RemoveOnSetCallback(funcSet); });

        if (QString(m_VarEnableVar1->GetDisplayValue()) == "true")
        {
            m_swatch1->show();
        }
        return true;
    }
    return false;
}

bool QColorCurve::TrySetupAlphaCurve(QString targetName, int index)
{
    auto funcSet = functor(*this, &CBaseVariableWidget::onVarChanged);

    IVariable** var = index == 0 ? &alphaVar : &alphaVar1;

    if (targetName.size())
    {
        *var = m_parent->getAttributeView()->GetVarFromPath(targetName);
        if (*var == nullptr)
        {
            return false;
        }

        (*var)->AddOnSetCallback(funcSet);
        if (index == 0)
        {
            addOnDestructionEvent([funcSet, this](CBaseVariableWidget* self) { alphaVar->RemoveOnSetCallback(funcSet); });
        }
        else
        {
            addOnDestructionEvent([funcSet, this](CBaseVariableWidget* self) { alphaVar1->RemoveOnSetCallback(funcSet); });
        }

        return true;
    }
    return false;
}

bool QColorCurve::TrySetupSecondAlphaCurve(QString targetName, QString targetEnableName)
{
    auto funcSet = functor(*this, &CBaseVariableWidget::onVarChanged);
    if (targetName.size() && targetEnableName.size())
    {
        m_AlphaVarEnableVar1 = m_parent->getAttributeView()->GetVarFromPath(targetEnableName);
        bool alpha1Setup = TrySetupAlphaCurve(targetName, 1);
        if (m_AlphaVarEnableVar1 == nullptr || !alpha1Setup)
        {
            m_pSpline1 = nullptr;
            m_AlphaVarEnableVar1 = alphaVar1 = nullptr;
            return false;
        }

        m_AlphaVarEnableVar1->AddOnSetCallback(funcSet);
        addOnDestructionEvent([funcSet, this](CBaseVariableWidget* self) { m_AlphaVarEnableVar1->RemoveOnSetCallback(funcSet); });

        if (QString(m_VarEnableVar1->GetDisplayValue()) == "true")
        {
            m_swatch1->show();
        }
        return true;
    }
    return false;
}

void QColorCurve::buildMenu(bool RandomGradient)
{
    QAction* action;
    m_menu->clear();
    if (m_VarEnableVar1 != nullptr)
    {
        if (RandomGradient == false)
        {
            QAction* action = m_menu->addAction("Random between two gradients");//launch random dlg
            connect(action, &QAction::triggered, this, [&]()
                {
                    m_VarEnableVar1->Set(true);
                    if (m_AlphaVarEnableVar1)
                    {
                        m_AlphaVarEnableVar1->Set(true);
                    }
                    m_swatch1->show();
                    buildMenu(true);
                });
        }
        else
        {
            QAction* action = m_menu->addAction("One gradient over time");//launch random dlg
            connect(action, &QAction::triggered, this, [&]()
                {
                    m_VarEnableVar1->Set(false);
                    if (m_AlphaVarEnableVar1)
                    {
                        m_AlphaVarEnableVar1->Set(false);
                    }
                    m_swatch1->hide();
                    buildMenu(false);
                });
        }
    }
    action = m_menu->addAction("Save to library");//launch save to lib dlg
    connect(action, &QAction::triggered, this, [&]()
        {
            UIFactory::GetGradientEditor(m_swatch->GetContent(), m_swatch->GetStops())->AddGradientToLibrary(QString("Default"));
        });
    action = m_menu->addAction("Reset");//launch export dlg
    connect(action, &QAction::triggered, this, [&]()
        {
            UpdateGradient(m_defaultStops, 0);
            UpdateGradient(m_defaultStops1, 1);

            SCurveEditorContent content = m_swatch->GetContent();
            content.m_curves.clear();
            content.m_curves.push_back(m_defaultCurve);
            UpdateCurve(content, 0);

            SCurveEditorContent content1 = m_swatch1->GetContent();
            content1.m_curves.clear();
            content1.m_curves.push_back(m_defaultCurve);
            UpdateCurve(content1, 1);

            //m_parent->getAttributeView()->UpdateGradientEditor(m_swatch->GetCurve(), m_swatch->GetStops());
            //m_parent->getAttributeView()->UpdateGradientEditor(m_swatch1->GetCurve(), m_swatch1->GetStops());
        });

    m_tooltip = new QToolTipWidget(this);
}



void QColorCurve::setVar(IVariable* var)
{
    if (var == m_VarEnableVar1 || var == m_AlphaVarEnableVar1)
    {
        if (QString(var->GetDisplayValue()) == "true")
        {
            m_swatch1->show();
            buildMenu(true);
        }
        else
        {
            m_swatch1->hide();
            buildMenu(false);
        }
    }
    else if (var == m_var)
    {
        SetGradientForSwatch(var, m_swatch);
    }
    else if (var == m_Var1)
    {
        SetGradientForSwatch(var, m_swatch1);
    }
    else if (var == alphaVar)
    {
        SetAlphaForSwatch(var, m_swatch);
    }
    else if (var == alphaVar1)
	{
        SetAlphaForSwatch(var, m_swatch1);
    }
    update();
    repaint();
}

void QColorCurve::onVarChanged(IVariable* var)
{
    SelfCallFence(m_ignoreSetCallback);
    setVar(var);
}

void QColorCurve::UpdateGradient(QGradientStops stops, int swatchIndex)
{
    ISplineInterpolator* _spline = nullptr;
    QGradientSwatchWidget* _swatch = nullptr;
    IVariable* _var = nullptr;

    if (swatchIndex == 0)
    {
        _var = m_var;
        _spline = _var->GetSpline();
        _swatch = m_swatch;
    }
    else if (swatchIndex == 1)
    {
        _var = m_Var1;
        _spline = _var->GetSpline();
        _swatch = m_swatch1;
    }

    _spline->ClearAllKeys();
    _swatch->SetGradientStops(stops);
    for (QGradientStop stop : stops)
    {
        ISplineInterpolator::ValueType t = { 0, 0, 0, 0 };
        qreal r, g, b, a;
        stop.second.getRgbF(&r, &g, &b, &a);
        t[0] = r;
        t[1] = g;
        t[2] = b;
        t[3] = 1.0f; //alpha is handled separately
        const int keyId = _spline->InsertKey(stop.first, t);
        _spline->SetKeyValue(keyId, t);
        _spline->SetKeyFlags(keyId, (SPLINE_KEY_TANGENT_NONE << SPLINE_KEY_TANGENT_IN_SHIFT)
            | (SPLINE_KEY_TANGENT_NONE << SPLINE_KEY_TANGENT_OUT_SHIFT));
    }
    _spline->Update();

    m_ignoreSetCallback = true;
    _var->OnSetValue(false);
    m_ignoreSetCallback = false;

    update();
}

void QColorCurve::UpdateCurve(SCurveEditorContent content, int swatchIndex)
{
    ISplineInterpolator* _spline = nullptr;
    QGradientSwatchWidget* _swatch = nullptr;
    IVariable* _var = nullptr;

    if (swatchIndex == 0)
    {
        _var = alphaVar;
        _spline = _var->GetSpline();
        _swatch = m_swatch;
    }
    else if (swatchIndex == 1)
    {
        _var = alphaVar1;
        _spline = _var->GetSpline();
        _swatch = m_swatch1;
    }

    _spline->ClearAllKeys();
    SCurveEditorContent con = _swatch->GetContent();
    con.m_curves.back() = content.m_curves.back();
    _swatch->SetContent(con);

    //make alpha out of spline
    if (con.m_curves.back().m_keys.size() > 0)
    {
        for (SCurveEditorKey key : con.m_curves.back().m_keys)
        {
            ISplineInterpolator::ValueType t = { 0, 0, 0, 0 };
            t[0] = key.m_value;
            const int keyId = _spline->InsertKey(key.m_time, t);
            _spline->SetKeyValue(keyId, t);
            // construct flags
            int flags = 0;

            if (key.m_inTangentType == SCurveEditorKey::eTangentType_Linear)
            {
                flags |= (SPLINE_KEY_TANGENT_LINEAR << SPLINE_KEY_TANGENT_IN_SHIFT);
            }
            else if (key.m_inTangentType == SCurveEditorKey::eTangentType_Bezier)
            {
                flags |= (SPLINE_KEY_TANGENT_CUSTOM << SPLINE_KEY_TANGENT_IN_SHIFT);
                EditorSetTangent(_spline, keyId, &key, true);
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
                EditorSetTangent(_spline, keyId, &key, false);
            }
            else
            {
                flags |= (SPLINE_KEY_TANGENT_NONE << SPLINE_KEY_TANGENT_OUT_SHIFT);
            }
            _spline->SetKeyFlags(keyId, flags);
        }
    }
    else
    {
        ISplineInterpolator::ValueType t = { 0, 0, 0, 0 };
        t[0] = con.m_curves.back().m_defaultValue;
        const int keyId = _spline->InsertKey(0.0, t);
        _spline->SetKeyValue(keyId, t);
        const int _keyId = _spline->InsertKey(1.0, t);
        _spline->SetKeyValue(_keyId, t);
        // construct flags
        int flags = 0;

        flags |= (SPLINE_KEY_TANGENT_NONE << SPLINE_KEY_TANGENT_IN_SHIFT);
        flags |= (SPLINE_KEY_TANGENT_NONE << SPLINE_KEY_TANGENT_OUT_SHIFT);

        _spline->SetKeyFlags(keyId, flags);
        _spline->SetKeyFlags(_keyId, flags);
    }
    _spline->Update();

    blockSignals(true);
    _var->OnSetValue(false);
    blockSignals(false);

    update();
}

void QColorCurve::SetDefaultCurve(SCurveEditorCurve defaultCurve)
{
    m_defaultCurve = defaultCurve;
    m_defaultCurve1 = defaultCurve;
}

void QColorCurve::SetDefaultGradient(QGradientStops defaultStops)
{
    m_defaultStops = defaultStops;
    m_defaultStops1 = defaultStops;
}

void QColorCurve::CalculateRandomCurve()
{
    /*if (rand() % 2 == 0)
    {
    ISplineInterpolator::ValueType t = { 0, 0, 0, 0 };
    m_pSpline->Interpolate(0.2,t);
    m_alphaSpline->Interpolate(0.2, t);
    }
    else
    {
    ISplineInterpolator::ValueType t = { 0, 0, 0, 0 };
    m_pSpline1->Interpolate(0.2, t);
    m_alphaSpline1->Interpolate(0.2, t);
    }*/
}

bool QColorCurve::eventFilter(QObject* obj, QEvent* event)
{
    QWidget* objWidget = static_cast<QWidget*>(obj);
    if (objWidget == m_swatch)
    {
        if (event->type() == QEvent::MouseButtonPress)
        {
            QMouseEvent* e = (QMouseEvent*)event;
            if (e->button() == Qt::LeftButton)
            {
                OnSwatchClicked(m_swatch);
            }
            if (e->button() == Qt::RightButton)
            {
                m_menu->exec(e->globalPos());
                e->accept();
            }
        }
        return true;
    }
    if (objWidget == m_swatch1)
    {
        if (event->type() == QEvent::MouseButtonPress)
        {
            QMouseEvent* e = (QMouseEvent*)event;
            if (e->button() == Qt::LeftButton)
            {
                OnSwatchClicked(m_swatch1);
            }
            if (e->button() == Qt::RightButton)
            {
                m_menu->exec(e->globalPos());
                e->accept();
            }
        }
        return true;
    }
    return false;
}

bool QColorCurve::event(QEvent* e)
{
    IVariable* var = m_parent->getVar();
    QToolTipWidget::ArrowDirection arrowDir = QToolTipWidget::ArrowDirection::ARROW_RIGHT;
    switch (e->type())
    {
    case QEvent::ToolTip:
    {
        GetIEditor()->GetParticleUtils()->ToolTip_BuildFromConfig(m_tooltip, m_parent->getAttributeView()->GetVariablePath(m_var), "GradientCurve", QString(m_var->GetDisplayValue()));

        QHelpEvent* event = (QHelpEvent*)e;
        m_tooltip->TryDisplay(event->globalPos(), this, QToolTipWidget::ArrowDirection::ARROW_RIGHT);

        return true;
    }
    case QEvent::Leave:
    {
        if (m_tooltip->isVisible())
        {
            m_tooltip->hide();
        }
        QCoreApplication::instance()->removeEventFilter(this); //Release filter (Only if we have a tooltip?)

        m_parent->getAttributeView()->setToolTip("");
        break;
    }
    default:
        break;
    }
    return QWidget::event(e);
}

void QColorCurve::OnSwatchClicked(QGradientSwatchWidget* swatch)
{
    SCurveEditorCurve _oldCurve = swatch->GetCurve();
    QGradientStops _oldStops = swatch->GetStops();
    QGradientColorDialog* dlg = UIFactory::GetGradientEditor(swatch->GetContent(), swatch->GetStops());
    unsigned int index = (swatch == m_swatch) ? 0 : 1;
    IVariable* _vcolor = nullptr;
    IVariable* _valpha = nullptr;
    if (index == 1)
    {
        _vcolor = m_Var1;
        _valpha = alphaVar1;
    }
    else
    {
        _vcolor = m_var;
        _valpha = alphaVar;
    }
    if (dlg->exec() == QDialog::Accepted)
    {
        //handle the color
        QGradientStops _stops = dlg->GetGradient();
        bool match = true;
        if (_stops.size() == _oldStops.size())
        {
            for (unsigned int i = 0; i < _stops.size(); i++)
            {
                if (_stops[i].first != _oldStops[i].first || _stops[i].second != _oldStops[i].second)
                {
                    match = false;
                    break;
                }
            }
        }
        else
        {
            match = false;
        }
        if (!match)
        {
            UpdateGradient(_stops, (swatch == m_swatch) ? 0 : 1);
        }

        //handle the alpha
        match = true;
        SCurveEditorCurve _curve = dlg->GetCurve();
        if (_curve.m_keys.size() == _oldCurve.m_keys.size())
        {
            for (unsigned int i = 0; i < _curve.m_keys.size(); i++)
            {
                if (_curve.m_keys[i].m_time != _oldCurve.m_keys[i].m_time)
                {
                    match = false;
                    break;
                }
                if (_curve.m_keys[i].m_value != _oldCurve.m_keys[i].m_value)
                {
                    match = false;
                    break;
                }
                if (_curve.m_keys[i].m_inTangent != _oldCurve.m_keys[i].m_inTangent)
                {
                    match = false;
                    break;
                }
                if (_curve.m_keys[i].m_inTangentType != _oldCurve.m_keys[i].m_inTangentType)
                {
                    match = false;
                    break;
                }
                if (_curve.m_keys[i].m_outTangent != _oldCurve.m_keys[i].m_outTangent)
                {
                    match = false;
                    break;
                }
                if (_curve.m_keys[i].m_outTangentType != _oldCurve.m_keys[i].m_outTangentType)
                {
                    match = false;
                    break;
                }
            }
        }
        else
        {
            match = false;
        }
        if (!match)
        {
            SCurveEditorContent _content = swatch->GetContent();
            _content.m_curves.clear();
            _content.m_curves.push_back(_curve);
            UpdateCurve(_content, (swatch == m_swatch) ? 0 : 1);
            emit m_parent->SignalUndoPoint();
        }
    }
}

void QColorCurve::SetGradientForSwatch(IVariable* var, QGradientSwatchWidget* swatch)
{
    ISplineInterpolator* spline = var->GetSpline();
    QGradientStops stops;
    if (spline->GetKeyCount() == 0)
    {
        stops.append(QGradientStop(0.0f, Qt::white));
        stops.append(QGradientStop(1.0f, Qt::white));
        swatch->SetGradientStops(stops);
        return;
    }
    for (int i = 0; i < spline->GetKeyCount(); i++)
    {
        ISplineInterpolator::ValueType v;
        spline->GetKeyValue(i, v);
        const float t = spline->GetKeyTime(i);
        QColor color;
        color = color.fromRgbF(v[0], v[1], v[2], 1.0f);
        stops.append(QGradientStop(t, color));
    }
    swatch->SetGradientStops(stops);
}

void QColorCurve::SetAlphaForSwatch(IVariable* var, QGradientSwatchWidget* swatch)
{
    IVariable* _alphavar = var;
    ISplineInterpolator* _alphaspline;

#define IS_TAN_IN(BITFIELD, FLAG_BIT) \
    ((((BITFIELD & SPLINE_KEY_TANGENT_IN_MASK) >> SPLINE_KEY_TANGENT_IN_SHIFT) & FLAG_BIT) == FLAG_BIT)

#define IS_TAN_OUT(BITFIELD, FLAG_BIT) \
    ((((BITFIELD & SPLINE_KEY_TANGENT_OUT_MASK) >> SPLINE_KEY_TANGENT_OUT_SHIFT) & FLAG_BIT) == FLAG_BIT)

    _alphaspline = _alphavar->GetSpline();
    m_defaultCurve.m_customInterpolator = _alphaspline; //make sure default curve keeps a spline

    SCurveEditorContent content;
    int numKeys = _alphaspline->GetKeyCount();
    if (numKeys > 0)
    {
        content.m_curves.push_back(SCurveEditorCurve());
        content.m_curves.back().m_defaultValue = 1.0f;
        for (int i = 0; i < numKeys; i++)
        {
            const float time = _alphaspline->GetKeyTime(i);
            float value;
            _alphaspline->GetKeyValueFloat(i, value);
            content.m_curves.back().m_keys.push_back(SCurveEditorKey());
            SCurveEditorKey& key = content.m_curves.back().m_keys.back();
            key.m_time = time;
            key.m_value = value;
            key.m_bAdded = true;
            key.m_bModified = true;
            ISplineInterpolator::ValueType inTangent;
            ISplineInterpolator::ValueType outTangent;
            _alphaspline->GetKeyTangents(i, inTangent, outTangent);

            // Map tangent flags
            const int flags = _alphaspline->GetKeyFlags(i);

            if (IS_TAN_IN(flags, SPLINE_KEY_TANGENT_LINEAR))
            {
                key.m_inTangentType = SCurveEditorKey::eTangentType_Linear;
            }
            else if (IS_TAN_IN(flags, SPLINE_KEY_TANGENT_CUSTOM))
            {
                key.m_inTangentType = SCurveEditorKey::eTangentType_Bezier;
                key.m_inTangent = Vec2(1.f, inTangent[0]);
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
                key.m_outTangent = Vec2(1.f, outTangent[0]);
            }
            else
            {
                key.m_outTangentType = SCurveEditorKey::eTangentType_Flat;
            }
        }
    }
    else
    {
        if (content.m_curves.size() == 0)
        {
            content.m_curves.push_back(SCurveEditorCurve());
            content.m_curves.back().m_defaultValue = 1.0f;
        }
        content.m_curves.back().m_keys.push_back(SCurveEditorKey());
        SCurveEditorKey& key = content.m_curves.back().m_keys.back();
        key.m_time = 0.0f;
        key.m_value = 1.0f;
        key.m_bAdded = true;
        key.m_bModified = true;
        key.m_inTangentType = SCurveEditorKey::eTangentType_Bezier;
        key.m_outTangentType = SCurveEditorKey::eTangentType_Bezier;
        content.m_curves.back().m_keys.push_back(SCurveEditorKey());
        SCurveEditorKey& _key = content.m_curves.back().m_keys.back();
        _key.m_time = 1.0f;
        _key.m_value = 1.0f;
        _key.m_bAdded = true;
        _key.m_bModified = true;
        _key.m_inTangentType = SCurveEditorKey::eTangentType_Bezier;
        _key.m_outTangentType = SCurveEditorKey::eTangentType_Bezier;
    }
    swatch->SetContent(content);

}
