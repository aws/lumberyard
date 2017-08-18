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
#ifndef QCURVEEDITOR_IMP_H
#define QCURVEEDITOR_IMP_H

#include <CurveEditor.h>
#include "BaseVariableWidget.h"
#include <Controls/QToolTipWidget.h>

// CryCommon
struct ISplineInterpolator;
struct ISplineBackup;

class QCurveEditorImp
    : public CCurveEditor
    , public CBaseVariableWidget
{
public:
    explicit QCurveEditorImp(CAttributeItem* parent);
    virtual ~QCurveEditorImp();

    virtual void setVar(IVariable* var) override;

    // Node after a key has been added, make sure all modification to returned
    // key are done before adding a next key. std::vector may resize, resulting in
    // an incorrect address of returned reference.
    SCurveEditorKey& addKey(const float& time, const float& value);

    virtual void clear();

    virtual void onVarChanged(IVariable* var) override;

    SCurveEditorCurve const& getCurveConst() const;
    void SetCurve(SCurveEditorCurve const& curve);
private:
    SCurveEditorCurve& getCurve();
    void syncFromVar();
    void syncToInterpolator();
    void syncToVar();

    void addKey(const SCurveEditorKey& addKey);

    virtual void mouseMoveEvent(QMouseEvent* pEvent) override;

    virtual void hideEvent(QHideEvent* e) override;

    virtual void leaveEvent(QEvent* e) override;
private:
    SCurveEditorContent     m_content;
    ISplineInterpolator*    m_pSpline;
    ISplineBackup*          m_splineCache;
    QToolTipWidget*         m_tooltip;
};

#endif // QCURVEWIDGET_IMP_H
