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
#include "QBoolWidget.h"
#include "BaseVariableWidget.h"
#include "Utils.h"
#include "AttributeItem.h"

#include <Util/Variable.h>
#include "AttributeView.h"
#include "IEditorParticleUtils.h"
#include "../Utils.h"

QBoolWidget::QBoolWidget(CAttributeItem* parent)
    : CBaseVariableWidget(parent)
    , QCheckBox(parent)
{
    setCheckable(true);
    setChecked(QString(m_var->GetDisplayValue()) == "true");

    connect(this, &QCheckBox::clicked, this, [this](int changedTo)
        {
            bool state = changedTo != 0;
            m_var->Set(state);
            emit m_parent->SignalUndoPoint();
        });

    m_tooltip = new QToolTipWidget(this);
    setLayoutDirection(Qt::RightToLeft);
}

QBoolWidget::~QBoolWidget()
{
}

void QBoolWidget::onVarChanged(IVariable* var)
{
    bool state = QString(var->GetDisplayValue()) == "true";
    QSignalBlocker blocker(this);
    setChecked(state);
    m_previousState = !state;
    SelfCallFence(m_ignoreSetCallback);
    m_parent->UILogicUpdateCallback();
}

bool QBoolWidget::event(QEvent* e)
{
    IVariable* var = m_parent->getVar();
    switch (e->type())
    {
    case QEvent::ToolTip:
    {
        GetIEditor()->GetParticleUtils()->ToolTip_BuildFromConfig(m_tooltip, m_parent->getAttributeView()->GetVariablePath(m_var), "Boolean", QString(m_var->GetDisplayValue()));

        QHelpEvent* event = (QHelpEvent*)e;
        m_tooltip->TryDisplay(event->globalPos(), this, QToolTipWidget::ArrowDirection::ARROW_RIGHT);

        return true;
    }
    case QEvent::Leave:
    {
        if (m_tooltip)
        {
            m_tooltip->close();
        }
        QCoreApplication::instance()->removeEventFilter(this); //Release filter (Only if we have a tooltip?)

        m_parent->getAttributeView()->setToolTip("");
        break;
    }
    default:
        break;
    }
    return QCheckBox::event(e);
}