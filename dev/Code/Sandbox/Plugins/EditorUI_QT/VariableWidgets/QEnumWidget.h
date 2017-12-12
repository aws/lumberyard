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

#ifndef CRYINCLUDE_EDITORUI_QT_VARIABLE_WIDGETS_QENUMWIDGET_H
#define CRYINCLUDE_EDITORUI_QT_VARIABLE_WIDGETS_QENUMWIDGET_H
#pragma once

#include "BaseVariableWidget.h"
#include <QComboBox>
#include <Controls/QToolTipWidget.h>

class QEnumWidget
    : public QComboBox
    , public CBaseVariableWidget
{
public:
    QEnumWidget(CAttributeItem* parent);
    virtual ~QEnumWidget();

    void onVarChanged(IVariable* var) override;
    void showPopup() override;

    void SetItemEnable(int itemIdx, bool enable);

protected:
    bool event(QEvent* e) override;
    bool eventFilter(QObject* watched, QEvent* event) override;

    void onUserChange(const QString& text);

    bool IsPopupShowing();

    //Whether this widget was shown before.
    bool m_isShown;
    //Whether the mouse left button was pressed on line edit 
    bool m_lineEditPressed;
    QToolTipWidget* m_tooltip;
};

#endif // CRYINCLUDE_EDITORUI_QT_VARIABLE_WIDGETS_QENUMWIDGET_H
