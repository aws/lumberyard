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
#include "QWidgetInt.h"

#ifdef EDITOR_QT_UI_EXPORTS
    #include "stdafx.h"
    #include "AttributeItem.h"
#endif

#include <Util/Variable.h>
#include "AttributeView.h"
#include "Utils.h"
#include "IEditorParticleUtils.h"

QWidgetInt::QWidgetInt(CAttributeItem* parent)
    : QWidget(parent)
    , CBaseVariableWidget(parent)
    , stepResolution(1)
{
    edit.setParent(this);
    slider.setParent(this);
    slider.setOrientation(Qt::Horizontal);
    slider.installEventFilter(parent);
    slider.setFocusPolicy(Qt::FocusPolicy::ClickFocus);

    layout.setMargin(0);
    layout.setAlignment(Qt::AlignRight);
    setAttribute(Qt::WA_TranslucentBackground, true);

    edit.setFixedWidth(64);
    edit.setValue(0);
    edit.installEventFilter(parent);

    layout.addWidget(&slider);
    layout.addWidget(&edit);

    setLayout(&layout);
    m_StoredValue = edit.value();
    connect(&slider, &QSlider::valueChanged, this, &QWidgetInt::onSliderChanged);
    connect(&slider, &QSlider::sliderReleased, this, [=]()
        {
            onSetValue(slider.value()); 
            if (m_StoredValue != slider.value())
            {
                m_StoredValue = slider.value();
                emit m_parent->SignalUndoPoint();
            }
        });
    connect(&edit, SIGNAL(valueChanged(int)), this, SLOT(onSetValue(int)));
    connect(&edit, &QSpinBox::editingFinished, this, [=]()
        {
            SelfCallFence(m_ignoreSetCallback);
            int finalValue = edit.value();
            m_var->Set(finalValue);
            if (m_StoredValue != finalValue)
            {
                m_StoredValue = finalValue;
                emit m_parent->SignalUndoPoint();
            }
        });
    m_tooltip = new QToolTipWidget(this);
    edit.installEventFilter(this);
}

void QWidgetInt::Set(int val, int min, int max, bool hardMin, bool hardMax, int stepSize)
{
    stepResolution = (int)stepSize;

    // Make sure the stepResolution is not lower than 1
    stepResolution = stepResolution < 1 ? 1 : stepResolution;

    // Initialize the slider's configuration before calling setValue for the spinner box (edit) as that call will set the value of the slider.

    slider.setRange(min, max);
    slider.setSingleStep(stepResolution);
    if (!hardMax)
    {
        slider.hide();
        slider.setFixedWidth(0);
    }

    edit.setRange(hardMin ? min : std::numeric_limits<int>::min(),
        hardMax ? max : std::numeric_limits<int>::max());
    edit.setSingleStep(stepResolution);
    edit.setValue(val);
}

void QWidgetInt::onVarChanged(IVariable* var)
{
    m_tooltip->hide();
    SelfCallFence(m_ignoreSetCallback);

    QString v;
    var->Get(v);
    int value = v.toInt();
    QSignalBlocker sliderBlocker(&slider);
    QSignalBlocker editBlocker(&edit);
    slider.setValue(value);
    edit.setValue(value);
    assert(m_var);
    if (!m_var)
    {
        return;
    }

}

void QWidgetInt::onSetValue(int value)
{
    SelfCallFence(m_ignoreSetCallback);

    //when slider is disabled we set the width to 0
    //this check prevents the slider from overriding the edit's value
    if (!slider.isHidden())
    {
        slider.setValue(value);
    }
    edit.setValue(value);

    assert(m_var);
    if (!m_var)
    {
        return;
    }

    m_var->Set(value);
}

void QWidgetInt::onSliderChanged(int value)
{
    SelfCallFence(m_ignoreSetCallback);
    edit.blockSignals(true);
    edit.setValue(value);
    edit.blockSignals(false);
    emit m_parent->SignalUndoPoint();
}

bool QWidgetInt::event(QEvent* e)
{
    switch (e->type())
    {
    case QEvent::ToolTip:
    {
        QHelpEvent* event = (QHelpEvent*)e;

        GetIEditor()->GetParticleUtils()->ToolTip_BuildFromConfig(m_tooltip, m_parent->getAttributeView()->GetVariablePath(m_var), "Int", QString(m_var->GetDisplayValue()));

        m_tooltip->TryDisplay(event->globalPos(), this, QToolTipWidget::ArrowDirection::ARROW_RIGHT);

        return true;

        break;
    }
    case QEvent::Leave:
    {
        if (m_tooltip)
        {
            m_tooltip->close();
        }
        QCoreApplication::instance()->removeEventFilter(this); //Release filter (Only if we have a tooltip?)
        break;
    }
    default:
        break;
    }
    return QWidget::event(e);
}

bool QWidgetInt::eventFilter(QObject* obj, QEvent* e)
{
    if (obj == &edit)
    {
        if (e->type() == QEvent::KeyPress)
        {
            QKeyEvent* kev = (QKeyEvent*)e;
            if (kev->key() == Qt::Key_Return)
            {
                if (edit.text().isEmpty())
                {
                    edit.setValue(0);
                }
            }
        }
        if (e->type() == QEvent::FocusOut)
        {
            if (edit.text().isEmpty())
            {
                edit.setValue(0);
            }
        }
        if (e->type() == QEvent::FocusIn)
        {
            m_StoredValue = edit.value();
        }
    }
    return QWidget::eventFilter(obj, e);
}

#include <VariableWidgets/QWidgetInt.moc>
