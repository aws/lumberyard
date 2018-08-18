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
#include "BaseVariableWidget.h"
#include "AttributeItem.h"

#include <Util/Variable.h>

CBaseVariableWidget::CBaseVariableWidget(CAttributeItem* parent)
    : m_ignoreSetCallback(false)
    , m_var(parent->getVar())
    , m_parent(parent)
{
    auto funcSet = functor(*this, &CBaseVariableWidget::onVarChanged);
    m_var->AddOnSetCallback(funcSet);
    addOnDestructionEvent([funcSet, this](CBaseVariableWidget* self) { m_var->RemoveOnSetCallback(funcSet); });
}

CBaseVariableWidget::~CBaseVariableWidget()
{
    // call destruction events so things get cleaned up that need to be.
    for (auto & it : m_onDestructionEvents)
    {
        it(this);
    }
}
