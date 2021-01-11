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
#include "QWidgetInt.h"

#ifdef EDITOR_QT_UI_EXPORTS
    #include "EditorUI_QT_Precompiled.h"
    #include "AttributeItem.h"
#endif

#include <Util/Variable.h>
#include "AttributeView.h"
#include "Utils.h"
#include "IEditorParticleUtils.h"

QWidgetInt::QWidgetInt(CAttributeItem* parent)
    : QWidget(parent)
    , CBaseVariableWidget(parent)
    , m_stepResolution(1)
{
    m_edit.setParent(this);
    m_slider.setParent(this);
    m_slider.setOrientation(Qt::Horizontal);
    m_slider.installEventFilter(parent);
    m_slider.setFocusPolicy(Qt::FocusPolicy::ClickFocus);

    m_layout.setMargin(0);
    m_layout.setAlignment(Qt::AlignRight);
    setAttribute(Qt::WA_TranslucentBackground, true);

    m_edit.setFixedWidth(64);
    m_edit.setValue(0);
    m_edit.installEventFilter(parent);

    m_layout.addWidget(&m_edit, 1);
    m_layout.addWidget(&m_slider, 2);

    setLayout(&m_layout);
    m_StoredValue = m_edit.value();
    connect(&m_slider, &AzQtComponents::SliderInt::valueChanged, this, &QWidgetInt::onSliderChanged);
    connect(&m_slider, &AzQtComponents::SliderInt::sliderReleased, this, [=]()
        {
            onSetValue(m_slider.value()); 
            if (m_StoredValue != m_slider.value())
            {
                m_StoredValue = m_slider.value();
                emit m_parent->SignalUndoPoint();
            }
        });
    connect(&m_edit, SIGNAL(valueChanged(int)), this, SLOT(onSetValue(int)));
    connect(&m_edit, &QSpinBox::editingFinished, this, [=]()
        {
            SelfCallFence(m_ignoreSetCallback);
            int finalValue = m_edit.value();
            m_var->Set(finalValue);
            if (m_StoredValue != finalValue)
            {
                m_StoredValue = finalValue;
                emit m_parent->SignalUndoPoint();
            }
        });
    m_tooltip = new QToolTipWrapper(this);
    m_edit.installEventFilter(this);
}

void QWidgetInt::Set(int val, int min, int max, bool hardMin, bool hardMax, int stepSize)
{
    m_stepResolution = (int)stepSize;

    // Make sure the stepResolution is not lower than 1
    m_stepResolution = m_stepResolution < 1 ? 1 : m_stepResolution;

    // Initialize the slider's configuration before calling setValue for the spinner box (edit) as that call will set the value of the slider.

    m_slider.setRange(min, max);
    if (!hardMax)
    {
        m_slider.hide();
        m_slider.setFixedWidth(0);
    }

    m_edit.setRange(hardMin ? min : std::numeric_limits<int>::min(),
        hardMax ? max : std::numeric_limits<int>::max());
    m_edit.setSingleStep(m_stepResolution);
    m_edit.setValue(val);
}

void QWidgetInt::onVarChanged(IVariable* var)
{
    m_tooltip->hide();
    SelfCallFence(m_ignoreSetCallback);

    QString v;
    var->Get(v);
    int value = v.toInt();
    QSignalBlocker sliderBlocker(&m_slider);
    QSignalBlocker editBlocker(&m_edit);
    m_slider.setValue(value);
    m_edit.setValue(value);
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
    if (!m_slider.isHidden())
    {
        m_slider.setValue(value);
    }
    m_edit.setValue(value);

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
    m_edit.blockSignals(true);
    m_edit.setValue(value);
    m_edit.blockSignals(false);
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
    if (obj == &m_edit)
    {
        if (e->type() == QEvent::KeyPress)
        {
            QKeyEvent* kev = (QKeyEvent*)e;
            if (kev->key() == Qt::Key_Return)
            {
                if (m_edit.text().isEmpty())
                {
                    m_edit.setValue(0);
                }
            }
        }
        if (e->type() == QEvent::FocusOut)
        {
            if (m_edit.text().isEmpty())
            {
                m_edit.setValue(0);
            }
        }
        if (e->type() == QEvent::FocusIn)
        {
            m_StoredValue = m_edit.value();
        }
    }
    return QWidget::eventFilter(obj, e);
}

#include <VariableWidgets/QWidgetInt.moc>
