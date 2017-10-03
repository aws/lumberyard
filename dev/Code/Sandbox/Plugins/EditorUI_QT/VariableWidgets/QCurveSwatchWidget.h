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
#ifndef QCurveSwatchWidget_h__
#define QCurveSwatchWidget_h__


#include <QWidget>
#include "qgridlayout.h"
#include "CurveEditorContent.h"

class CCurveEditor;
struct ISplineInterpolator;
class QCustomGradientWidget;
struct SCurveEditorKey;
struct SCurveEditorCurve;
struct SCurveEditorContent;

class QCurveSwatchWidget
    : public QWidget
{
public:
    QCurveSwatchWidget(SCurveEditorContent content, QWidget* parent);
    //QGradientSwatchWidget(XmlNodeRef spline, SCurveEditorContent content, QGradientStops gradientHueInfo, QWidget* parent);

    ~QCurveSwatchWidget();

    SCurveEditorCurve& GetCurve();

    SCurveEditorContent GetContent();

    void SetContent(SCurveEditorContent content);

    virtual void paintEvent(QPaintEvent* event);
protected:
    void showEvent(QShowEvent* event) override;
    void InitializeContent();

    SCurveEditorContent m_content;
    ISplineInterpolator* m_spline;

    bool m_contentInitialized = false;
};

#endif // QCurveSwatchWidget_h__
