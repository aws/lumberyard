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
#ifndef CRYINCLUDE_EDITORUI_QT_VARIABLE_WIDGETS_QBOOLWIDGET_H
#define CRYINCLUDE_EDITORUI_QT_VARIABLE_WIDGETS_QBOOLWIDGET_H
#pragma once

#include "BaseVariableWidget.h"
#include <Controls/QToolTipWidget.h>
#include <QCheckBox>

class QBoolWidget
    : public QCheckBox
    , public CBaseVariableWidget
{
public:
    QBoolWidget(CAttributeItem* parent);
    virtual ~QBoolWidget();

    virtual void onVarChanged(IVariable* var) override;
    bool m_previousState;
protected:
    virtual bool event(QEvent* e) override;
    QToolTipWidget* m_tooltip;
};

#endif // CRYINCLUDE_EDITORUI_QT_VARIABLE_WIDGETS_QBOOLWIDGET_H
