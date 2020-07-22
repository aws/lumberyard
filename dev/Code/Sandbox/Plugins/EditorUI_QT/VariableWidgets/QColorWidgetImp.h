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
#ifndef QCOLORWIDGET_IMP_H
#define QCOLORWIDGET_IMP_H

#include "QColorWidget.h"
#include "BaseVariableWidget.h"
#include "QMenu"
#include <Controls/QToolTipWidget.h>

namespace AzQtComponents
{
    class ColorPicker;
} // namespace AzQtCpomponents

class QColorWidgetImp
    : public QColorWidget
    , public CBaseVariableWidget
{
    Q_OBJECT

public:
    QColorWidgetImp(CAttributeItem* parent);
    virtual ~QColorWidgetImp();

    bool TrySetupSecondColor(QString targetName, QString targetEnableName);
    bool TrySetupAlpha(QString targetName);
    bool TrySetupSecondAlpha(QString targetName, QString targetEnableName);

    virtual void setColor(const QColor& color, int index) override;
    virtual void setAlpha(float alpha, int index) override;
    virtual void setVar(IVariable* var) override;
    virtual void onVarChanged(IVariable* var) override;

    virtual void mousePressEvent(QMouseEvent* e);

    bool isDefaultValue();
    
signals:
    void SingalSubVarChanged(IVariable* var);

protected:
    virtual void setInternalColor(const QColor& color, int index, bool enableUndo = true);
    virtual void setInternalAlpha(float alpha, int index, bool enableUndo = true);
    void CreateTooltipForRect(QRect widgetRect, const QString& option = "", const QString& extraDataForTooltip = "");
    QString CreateToolTipString(const QColor& color);
    virtual void onSelectColor(int index) override;
    virtual bool event(QEvent* e) override;
    virtual void buildMenu();

    void ResetToDefault();

    float GetAlphaValue(int index);
    QColor GetColor(int index);
    void AddVarSetCallBack(IVariable* var);
private:
    QMenu* m_menu;
    bool m_ignoreSetVar;
    QToolTipWidget* m_tooltip;
    IVariable* m_Var1;
    IVariable* m_VarEnableVar1;
    IVariable* m_AlphaVar;
    IVariable* m_AlphaVar1;
    IVariable* m_AlphaVarEnableVar1;

    QColor m_OriginColor;       // Save color for m_Var and m_AlphaVar to undo
    QColor m_OriginColor1;      // Save color for m_Var1 and m_AlphaVar to undo
    AzQtComponents::ColorPicker* m_colorPicker = nullptr;
};

#endif // QCOLORWIDGET_IMP_H
