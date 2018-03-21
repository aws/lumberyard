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
#ifdef EDITOR_QT_UI_EXPORTS
#include "stdafx.h"
#include "AttributeItem.h"
#include "AttributeView.h"
#endif

#include "QWidgetVector.h"
#include <VariableWidgets/QWidgetVector.moc>

#include <Util/Variable.h>
#include "Util/VariablePropertyType.h"

#include "Utils.h"
#include "../../../../CryEngine/CryCommon/ParticleParams.h"
#include "IEditorParticleUtils.h"


QWidgetVector::QWidgetVector(CAttributeItem* parent, unsigned int size, bool allowLiveUpdate /*=false*/)
    : QWidget(parent)
    , CBaseVariableWidget(parent)
    , m_spinBoxWidth(80) // 80 is a good width to display up to 7 digits
    , layout(new QHBoxLayout(this))
    , m_ignoreSliderChange(false)
    , m_ignoreSpinnerChange(false)
    , slider(new QSlider)
{
    layout->setMargin(0);
    layout->setSizeConstraint(QLayout::SetNoConstraint);
    layout->setAlignment(Qt::AlignRight);
    setAttribute(Qt::WA_TranslucentBackground, true);


    for (unsigned int i = 0; i < size; i++)
    {
        VectorEntry* entry = new VectorEntry(this);
        entry->m_edit->setFixedWidth(m_spinBoxWidth);
        entry->m_edit->installEventFilter(parent);
        entry->m_edit->setFocusPolicy(Qt::FocusPolicy::StrongFocus);
        list.push_back(entry);
        m_StoredValue.push_back(entry->m_edit->value());
    }

    if (list.size() == 1)
    {
        slider->setParent(this);
        slider->setOrientation(Qt::Horizontal);
        slider->installEventFilter(parent);
        slider->setFocusPolicy(Qt::FocusPolicy::ClickFocus);
        layout->addWidget(slider);

        connect(slider, &QSlider::valueChanged, this, &QWidgetVector::onSliderChanged);
        connect(slider, &QSlider::sliderReleased, this, &QWidgetVector::onSliderReleased);
        slider->installEventFilter(this);
    }

    for (int i = 0; i < list.size(); i++)
    {
        VectorEntry& entry = *list[i];
        layout->addWidget(entry.m_edit);
        connect(entry.m_edit, SIGNAL(valueChanged(double)), this, SLOT(onSetValue()));
        QAmazonDoubleSpinBox* edit = entry.m_edit;
        connect(edit, &QAmazonDoubleSpinBox::editingFinished, this, [=]()
            {
                double finalValue = edit->value();
                setVariable();
                if (m_StoredValue[i] != finalValue)
                {
                    emit m_parent->SignalUndoPoint();
                    m_StoredValue[i] = finalValue;
                }
            });
        entry.m_edit->installEventFilter(this);

        connect(entry.m_edit, &QAmazonDoubleSpinBox::spinnerDragged, this, &QWidgetVector::onSpinnerDragged);
        connect(entry.m_edit, &QAmazonDoubleSpinBox::spinnerDragFinished, this, &QWidgetVector::onSpinnerDragFinished);
    }

    setLayout(layout);
    m_tooltip = new QToolTipWidget(this);
}

QWidgetVector::~QWidgetVector()
{
    for (unsigned int i = 0; i < list.size(); i++)
    {
        delete list[i];
    }
}

void QWidgetVector::SetComponent(unsigned int n, float val, float min, float max, bool hardMin, bool hardMax, float stepSize)
{
    if (stepSize <= 0.0f)
    {
        stepSize = 0.0001f;
    }

    VectorEntry& entry = *list[n];
    entry.m_resolution = stepSize;//needs to be before setValue because that will trigger onEditChanged.

    // Initialize the slider's configuration before calling setValue for the spinner box (m_edit) as that call will set the value of the slider.
    if (list.size() == 1)
    {
        slider->setRange(min / stepSize, max / stepSize);
        slider->setSingleStep(1);
        if (!hardMax)
        {
            slider->hide();
            slider->setFixedWidth(0);
            layout->setAlignment(Qt::AlignRight);
        }
    }

    //For SoftMin/SoftMax variables, we need to enforce some soft of limit so picking something arbitrarily small/large
    const int softLimit = fHUGE;

    entry.m_edit->setRange(hardMin ? min : -softLimit, hardMax ? max : softLimit);
    entry.m_edit->setSingleStep(stepSize);
    entry.m_edit->setValue(val);
}

void QWidgetVector::onVarChanged(IVariable* var)
{
    SelfCallFence(m_ignoreSetCallback);

    Prop::Description desc(var);
    switch (desc.m_type)
    {
    case ePropertyFloat:
    {
        float v;
        var->Get(v);
        VectorEntry& entry = *list[0];
        //this will also set the value of the slider
        entry.m_edit->setValue(v);
    } break;
    case ePropertyVector2:
    {
        // Fetch data as string instead of Vec2_tpl to address the case where there might be a mismatch between the data-type (TRangeParam) from
        // which the value is being fetched, and a Vec2_tpl
        QString str;
        var->Get(str);
        Vec2 v;
        int numScanned = azsscanf(str.toUtf8().data(), "%f,%f", &v.x, &v.y);
        if (numScanned != 2)
        {
            return;
        }

        list[0]->m_edit->setValue(v.x);
        list[1]->m_edit->setValue(v.y);
    } break;
    case ePropertyVector:
    {
        Vec3 v;
        var->Get(v);
        list[0]->m_edit->setValue(v.x);
        list[1]->m_edit->setValue(v.y);
        list[2]->m_edit->setValue(v.z);
    } break;
    case ePropertyVector4:
    {
        Vec4 v;
        var->Get(v);
        list[0]->m_edit->setValue(v.x);
        list[1]->m_edit->setValue(v.y);
        list[2]->m_edit->setValue(v.z);
        list[3]->m_edit->setValue(v.w);
    } break;
    }
    setVariable();
}

float QWidgetVector::getGuiValue(int idx)
{
    return list[idx]->m_edit->value();
}

// Updates QDoubleSpinBox, applies the change, plus update all hooked IVariable
void QWidgetVector::onSliderReleased()
{
    SelfCallFence(m_ignoreSliderChange);

    VectorEntry& entry = *list.front();
    const int valEdit = static_cast<int>(ceil(entry.m_edit->value() / entry.m_resolution));
    const int valSlider = slider->value();
    if (valSlider != valEdit)
    {
        if (valSlider == slider->maximum())
        {
            entry.m_edit->setValue(entry.m_edit->maximum());
        }
        else if (valSlider == slider->minimum())
        {
            entry.m_edit->setValue(entry.m_edit->minimum());
        }
        else
        { 
            entry.m_edit->setValue(valSlider * entry.m_resolution);
        }
        // Since slider only exist when the size of vector is 1, we stored the slider's value into m_StoredValue[0].
        qreal curVal = entry.m_edit->value();
        if (m_StoredValue[0] != curVal)
        {
            emit m_parent->SignalUndoPoint();
            m_StoredValue[0] = curVal;
        }
    }

    setVariable();
    m_tooltip->hide();

    m_parent->UILogicUpdateCallback();

    emit m_parent->SignalUndoPoint();
}

// Updates QSlider, applies the change, plus update all hooked IVariable
void QWidgetVector::onSetValue()
{
    SelfCallFence(m_ignoreSetCallback);

    //when slider is disabled we set the width to 0
    //this check prevents the slider from overriding the edit's value
    if (!slider->isHidden() && list.size() == 1)
    {
        VectorEntry& entry = *list.front();
        const int valEdit = (int)ceil(entry.m_edit->value() / entry.m_resolution);
        const int valSlider = slider->value();
        if (valEdit != valSlider)
        {
            //Set the fence to prevent the slider's update from re-updating the spinner box which A) isn't necessary and B) breaks "Soft Min/Max" parameters.
            SelfCallFence(m_ignoreSliderChange);
            slider->setValue(valEdit);
        }
    }

    setVariable();

    m_tooltip->hide();

    m_parent->UILogicUpdateCallback();
}

void QWidgetVector::setVariable()
{
    // Do callbacks to the IVariable
    if (!m_var)
    {
        return;
    }

    if (m_ignoreSpinnerChange)
    {
        return;
    }

    // set event is not ignored, and there is a m_var..
    if (list.size() == 1)
    {
        m_var->Set((float)list[0]->m_edit->value());
    }
    if (list.size() == 2)
    {
        Vec2 v;
        v.x = (float)list[0]->m_edit->value();
        v.y = (float)list[1]->m_edit->value();
        // must be set as a string representation to address the case when TRangeParam may be needed to set from a Vec2_tpl,
        // in which case the type compatibility function may fail even though they are compatible types
        QString str = QString("%1,%2").arg(v.x).arg(v.y);

        m_var->Set(str);
    }
    if (list.size() == 3)
    {
        Vec3 v;
        v.x = (float)list[0]->m_edit->value();
        v.y = (float)list[1]->m_edit->value();
        v.z = (float)list[2]->m_edit->value();
        m_var->Set(v);
    }
    if (list.size() == 4)
    {
        Vec4 v;
        v.x = (float)list[0]->m_edit->value();
        v.y = (float)list[1]->m_edit->value();
        v.z = (float)list[2]->m_edit->value();
        v.w = (float)list[3]->m_edit->value();
        m_var->Set(v);
    }
}

//Updates QDoubleSpinBox value to QSlider value but doesn't apply the change.  That will happen when the mouse is released.
void QWidgetVector::onSliderChanged()
{
    SelfCallFence(m_ignoreSliderChange);

    if (list.size() != 1)
    {
        return;
    }

    VectorEntry& entry = *list.front();
    slider->blockSignals(true);
    entry.m_edit->blockSignals(true);
    const int valEdit = (int)ceil(entry.m_edit->value() / entry.m_resolution);
    const int valSlider = slider->value();
    if (valSlider != valEdit)
    {
        if (valSlider == slider->maximum())
        {
            entry.m_edit->setValue(entry.m_edit->maximum());
        }
        else if (valSlider == slider->minimum())
        {
            entry.m_edit->setValue(entry.m_edit->minimum());
        }
        else
        {
            entry.m_edit->setValue(valSlider * entry.m_resolution);
        }
    }
    entry.m_edit->blockSignals(false);
    slider->blockSignals(false);

    m_parent->UILogicUpdateCallback();
}

// Prevents applying the change until the mouse is released.
void QWidgetVector::onSpinnerDragged()
{
    // Warning, I'm not sure how well this will work with the undo system. I'm delaying making this work well since
    // we are planning significant refactoring for the undo system soon. (csantora 3/6/2017)
    m_ignoreSpinnerChange = true;
}

// Applies the changes made when dragging a spinner.
void QWidgetVector::onSpinnerDragFinished()
{
    m_ignoreSpinnerChange = false;
    setVariable();
}


bool QWidgetVector::event(QEvent* e)
{
    switch (e->type())
    {
    case QEvent::ToolTip: 
    {
        QHelpEvent* event = (QHelpEvent*)e;

        GetIEditor()->GetParticleUtils()->ToolTip_BuildFromConfig(m_tooltip, m_parent->getAttributeView()->GetVariablePath(m_var), "Vector", QString(m_var->GetDisplayValue()), isEnabled());

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

bool QWidgetVector::eventFilter(QObject* object, QEvent* e)
{
    for (unsigned int i = 0; i < list.size(); i++)
    {
        if (object == (QObject*)list[i]->m_edit)
        {
            if (e->type() == QEvent::KeyPress)
            {
                QKeyEvent* kev = (QKeyEvent*)e;
                if (kev->key() == Qt::Key_Return)
                {
                    if (list[i]->m_edit->text().isEmpty())
                    {
                        list[i]->m_edit->setValue(0.0f);
                    }
                }
            }
            if (e->type() == QEvent::FocusOut)
            {
                if (list[i]->m_edit->text().isEmpty())
                {
                    list[i]->m_edit->setValue(0.0f);
                }
            }
            if (e->type() == QEvent::FocusIn)
            {
                double valEdit = list[i]->m_edit->value();
                m_StoredValue[i] = valEdit;
            }
        }
    }
    return false;
}
