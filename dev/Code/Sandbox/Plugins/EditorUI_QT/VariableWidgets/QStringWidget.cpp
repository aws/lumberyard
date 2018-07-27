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
#include "QStringWidget.h"
#include "BaseVariableWidget.h"
#include "Utils.h"
#include "AttributeItem.h"

#include <Util/Variable.h>

QStringWidget::QStringWidget(CAttributeItem* parent)
    : CBaseVariableWidget(parent)
    , QAmazonLineEdit(parent)
{
    setText(QString(m_var->GetDisplayValue()));

    connect(this, &QAmazonLineEdit::textChanged, this, [this](const QString& text)
        {
            SelfCallFence(m_ignoreSetCallback);
            m_var->SetDisplayValue(text.toUtf8().data());
            emit m_parent->SignalUndoPoint();
        });
}

QStringWidget::~QStringWidget()
{
}

void QStringWidget::onVarChanged(IVariable* var)
{
    SelfCallFence(m_ignoreSetCallback);
    QSignalBlocker blocker(this);
    setText(QString(var->GetDisplayValue()));
}
