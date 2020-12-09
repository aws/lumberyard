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
#include <AzQtComponents/Components/Widgets/BrowseEdit.h>

class QBitmapPreviewDialogImp;
class CAttributeView;

class QFileSelectResourceWidget
    : public QWidget
    , public CBaseVariableWidget
{
    Q_OBJECT
public:
    explicit QFileSelectResourceWidget(CAttributeItem* parent, CAttributeView* attributeView,
        int propertyType);

    virtual void setPath(const QString& path);
    virtual void setVar(IVariable* var);
    virtual void onVarChanged(IVariable* var);

protected:
    virtual void onOpenSelectDialog();
    virtual void onReturnPressed();
    virtual void onClearButtonClicked();
    virtual QString pathFilter(const QString& path);
    virtual bool eventFilter(QObject* obj, QEvent* event);

private:
    void OpenSourceFile();
    QPushButton* addButton(const QString& caption, const QString& tooltip, int row, int col, int rowspan, int colspan, const QIcon* icon = nullptr);
    void UpdatePreviewTooltip(QString filePath, QPoint position, bool showTooltip);

    QVector<QPushButton*> m_btns; //added to allow event testing for custom tooltips
    AzQtComponents::BrowseEdit* m_browseEdit;
    QGridLayout* m_gridLayout;
    QToolTipWrapper* m_tooltip;
    bool m_ignoreSetVar;
    int m_propertyType;
    bool m_hasFocus;
    CAttributeView* m_attributeView;
    QPoint m_lastTooltipPos;
};

#endif // QFILESELECTMODELWIDGET_H
