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
#ifndef QGradientSwatchWidget_h__
#define QGradientSwatchWidget_h__

#include <QWidget>
#include "qgridlayout.h"

class CCurveEditor;
struct ISplineInterpolator;
class QCustomGradientWidget;
struct SCurveEditorKey;
struct SCurveEditorCurve;
struct SCurveEditorContent;

class QGradientSwatchWidget
    : public QWidget
{
    Q_OBJECT
public:
    QGradientSwatchWidget(SCurveEditorContent content, QGradientStops gradientHueInfo, QWidget* parent);

    ~QGradientSwatchWidget();

    void SetCurve(ISplineInterpolator* spline);

    unsigned int AddGradientStop(float stop, QColor color);

    void RemoveGradientStop(unsigned int stop);

    void EditGradientStop(unsigned int stop, QColor color);

    SCurveEditorKey& addKey(const float& time, const float& value);

    void syncToInterpolator();

    SCurveEditorCurve& GetCurve();

    SCurveEditorContent& GetContent();

    void SetContent(SCurveEditorContent content, bool scale = false);

    void SetGradientStops(QGradientStops stops);

    QCustomGradientWidget& GetGradient();

    QGradientStops& GetStops();

    QColor GetHueAt(float time);

    ISplineInterpolator* GetSpline();

    class XmlNodeRef GetSplineXml();

    float GetAlphaAt(float time);

    void UpdateAlpha();

    float GetMinAlpha() { return alphaMin; }
    float GetMaxAlpha() { return alphaMax; }

    virtual void paintEvent(QPaintEvent* event);
    void setSimpleViewMode(bool val);
protected:
    void UpdateSimpleGradient();
    void SortStops();
    enum Gradients
    {
        ALPHA_RANGE = 0,
        HUE_RANGE,
        NUM_GRADIENTS
    };
    CCurveEditor* m_curveEditor;
    SCurveEditorContent* m_curveContent;
    ISplineInterpolator* m_spline;
    QCustomGradientWidget* m_gradient;
    QCustomGradientWidget* m_simpleGradient;
    QGradientStops m_hueStops;
    QGridLayout layout;
    float alphaMin, alphaMax;
    bool displaySimpleGradient;
};
#endif // QGradientSwatchWidget_h__
