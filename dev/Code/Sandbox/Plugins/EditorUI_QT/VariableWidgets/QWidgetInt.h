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

#ifndef CRYINCLUDE_EDITORUI_QT_VARIABLE_WIDGETS_QWIDGETINT_H
#define CRYINCLUDE_EDITORUI_QT_VARIABLE_WIDGETS_QWIDGETINT_H
#pragma once

#include "BaseVariableWidget.h"

#include <QWidget>
#include <QtWidgets/QSpinBox>
#include <QtWidgets/QSlider>
#include <QHBoxLayout>
#include <QValidator>
#include <Controls/QToolTipWidget.h>

struct IVariable;

class QWidgetInt
    : public QWidget
    , public CBaseVariableWidget
{
    Q_OBJECT
public:
    QWidgetInt(CAttributeItem* parent);
    virtual ~QWidgetInt(){}

    void Set(int val, int min, int max, bool hardMin, bool hardMax, int stepSize);

    virtual void onVarChanged(IVariable* var) override;

public slots:
    void onSetValue(int value);
    void onSliderChanged(int value);
    virtual bool eventFilter(QObject* obj, QEvent* e) override;

protected:
    virtual bool event(QEvent* e) override;

private:
    QHBoxLayout layout;
    QSpinBox edit;
    QSlider slider;
    int stepResolution;
    QToolTipWidget* m_tooltip;
    int m_StoredValue;  // Value stored for undo
};

#endif // CRYINCLUDE_EDITORUI_QT_VARIABLE_WIDGETS_QWIDGETINT_H
