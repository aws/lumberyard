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
#ifndef CRYINCLUDE_EDITORUI_QT_VARIABLE_WIDGETS_QSTRINGWIDGET_H
#define CRYINCLUDE_EDITORUI_QT_VARIABLE_WIDGETS_QSTRINGWIDGET_H
#pragma once

#include "BaseVariableWidget.h"
#include "QAmazonLineEdit.h"

class QStringWidget
    : public QAmazonLineEdit
    , public CBaseVariableWidget
{
public:
    QStringWidget(CAttributeItem* parent);
    virtual ~QStringWidget();

    virtual void onVarChanged(IVariable* var) override;
};

#endif // CRYINCLUDE_EDITORUI_QT_VARIABLE_WIDGETS_QSTRINGWIDGET_H
