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
#ifndef QCOLORCURVE_H
#define QCOLORCURVE_H

#include "QHGradientWidget.h"
#include "BaseVariableWidget.h"
#include "QGradientSwatchWidget.h"
#include "QMenu"
#include "CurveEditorContent.h"
#include <Controls/QToolTipWidget.h>

// Cry
struct ISplineInterpolator;
struct SCurveEditorCurve;

class QColorCurve
    : public CBaseVariableWidget
    , public QWidget
{
public:
    QColorCurve(CAttributeItem* parent, QGradientSwatchWidget* swatch, QGradientSwatchWidget* swatch1);
    virtual ~QColorCurve();
    bool TrySetupSecondCurve(QString targetName, QString targetEnableName);
    bool TrySetupAlphaCurve(QString targetName, int index);
    bool TrySetupSecondAlphaCurve(QString targetName, QString targetEnableName);

    void buildMenu(bool RandomGradient);
    virtual void setVar(IVariable* var) override;
    virtual void onVarChanged(IVariable* var) override;
    void UpdateGradient(QGradientStops stops, int swatchIndex);
    void UpdateCurve(SCurveEditorContent content, int swatchIndex);
    void SetDefaultCurve(SCurveEditorCurve defaultCurve);
    void SetDefaultGradient(QGradientStops defaultStops);
    void CalculateRandomCurve();
    virtual bool eventFilter(QObject*, QEvent*) override;
protected:
    virtual bool event(QEvent* e) override;
    void SetGradientForSwatch(IVariable* var, QGradientSwatchWidget* swatch);
    void SetAlphaForSwatch(IVariable* var, QGradientSwatchWidget* swatch);
    void OnSwatchClicked(QGradientSwatchWidget* swatch);
    std::function<void(QGradientStops)> callback_update_gradient;
protected:
    enum Flags
    {
        FLAG_UPDATE_DISPLAY_GRADIENT = 1 << 0,
        FLAG_UPDATE_KEYS = 1 << 2,
    };
    QGradientSwatchWidget* m_swatch;
    QGradientSwatchWidget* m_swatch1;
    QVBoxLayout layout;
    //void onUpdateGradient(const int flags);
private:
    IVariable* m_Var1;
    IVariable* alphaVar;
    IVariable* alphaVar1;
    IVariable* m_VarEnableVar1;
    IVariable* m_AlphaVarEnableVar1;
    ISplineInterpolator* m_alphaSpline;
    ISplineInterpolator* m_alphaSpline1;
    ISplineInterpolator* m_pSpline;
    ISplineInterpolator* m_pSpline1;
    QGradientStops m_defaultStops;
    QGradientStops m_defaultStops1;
    SCurveEditorCurve m_defaultCurve;
    SCurveEditorCurve m_defaultCurve1;
    QMenu* m_menu;
    QToolTipWidget* m_tooltip;
    QGradientStops m_stops;
    SCurveEditorContent* m_content;
};

#endif // QCOLORCURVE_H
