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

#ifndef CRYINCLUDE_EDITORUI_QT_VARIABLE_WIDGETS_QWIDGETVECTOR_H
#define CRYINCLUDE_EDITORUI_QT_VARIABLE_WIDGETS_QWIDGETVECTOR_H
#pragma once

#include <AzCore/PlatformDef.h>
#include "BaseVariableWidget.h"

#include <QtWidgets>
#include <QHBoxLayout>
#include <QValidator>
#include <vector>
#include <Controls/QToolTipWidget.h>
#include "QAmazonDoubleSpinBox.h"

class QWidgetVector
    : public QWidget
    , public CBaseVariableWidget
{
    Q_OBJECT
public:
    QWidgetVector(CAttributeItem* parent, unsigned int size, bool allowLiveUpdate = false);
    virtual  ~QWidgetVector();

    void SetComponent(unsigned int n, float val, float min, float max, bool hardMin, bool hardMax, float stepSize);

    virtual void onVarChanged(IVariable* var) override;

    float getGuiValue(int idx);

public slots:
    virtual bool event(QEvent* e) override;
    virtual bool eventFilter(QObject* object, QEvent* e) override;
    void onSliderReleased();
    void onSliderChanged();
    void onSpinnerDragged();
    void onSpinnerDragFinished();
    void onSetValue();

private:
    struct VectorEntry
    {
        VectorEntry(QWidget* parent)
            : m_edit(new QAmazonDoubleSpinBox(parent))
            , m_resolution(DBL_EPSILON){}

        QAmazonDoubleSpinBox*   m_edit;
        double                  m_resolution;
    };

    void setVariable();

private:
    const int m_spinBoxWidth;

    QHBoxLayout* layout;
    QSlider* slider;
    QVector<VectorEntry*> list;

    bool m_ignoreSliderChange;
    bool m_ignoreSpinnerChange;
    QToolTipWidget* m_tooltip;
    QVector<double> m_StoredValue;
};

#endif // CRYINCLUDE_EDITORUI_QT_VARIABLE_WIDGETS_QWIDGETVECTOR_H
