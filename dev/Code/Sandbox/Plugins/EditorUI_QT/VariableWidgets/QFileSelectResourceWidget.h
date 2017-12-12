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

#ifndef QFILESELECTRESOURCEWIDGET_H
#define QFILESELECTRESOURCEWIDGET_H

#include "QFileSelectWidget.h"
#include "BaseVariableWidget.h"
#include <Controls/QToolTipWidget.h>


class QBitmapPreviewDialogImp;
class CAttributeView;

class QFileSelectResourceWidget
    : public QFileSelectWidget
    , public CBaseVariableWidget
{
    Q_OBJECT
public:
    explicit QFileSelectResourceWidget(CAttributeItem* parent, CAttributeView* attributeView,
        int propertyType);

    virtual void setPath(const QString& path) override;
    virtual void setVar(IVariable* var) override;
    virtual void onVarChanged(IVariable* var) override;

protected:
    virtual void onOpenSelectDialog() override;
    virtual void onSelectedIndexChanged(int index) override;
    virtual void onReturnPressed() override;
    virtual QString pathFilter(const QString& path) override;
    virtual bool eventFilter(QObject* obj, QEvent* event) override;

private:
    void OpenSourceFile();

    QVector<QPushButton*> m_btns; //added to allow event testing for custom tooltips
    QToolTipWidget* m_tooltip;
    bool m_ignoreSetVar;
    int m_propertyType;
    bool m_hasFocus;
    CAttributeView* m_attributeView;
};

#endif // QFILESELECTMODELWIDGET_H
