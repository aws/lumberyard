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

#ifndef PANELTITLEBAR_H
#define PANELTITLEBAR_H

#include <QWidget>
#include "../api.h"
#include "VariableWidgets/QCopyableWidget.h"

//#define _ENABLE_ADVANCED_BASIC_BUTTONS_

struct IVariable;
class QLineEdit;
class QDockWidget;

namespace Ui {
    class PanelTitleBar;
}

// General PanelTitleBar
class EDITOR_QT_UI_API PanelTitleBar
    : public QCopyableWidget
{
    Q_OBJECT

public:
    explicit PanelTitleBar(QWidget* parent);
    virtual ~PanelTitleBar();

    bool getCollapsed() const { return m_collapsed;  }
    void setCollapsed(bool vCollapsed);
    void setAllTabsCollapsed(bool collapsed);
    void toggleCollapsed();

    void SetCallbackOnCollapseEvent(std::function<void()> callback);

    void repaintAll();
    virtual void updateColor(); //Used to update the label text color on_pushButton_clicked/after init
    virtual void toggleAdvanced(){};

    void StartRename();
    void EndRename();

signals:
    void SignalPanelClosed(QDockWidget*);
protected:
    virtual bool onEventFilter(QObject* obj, QEvent* ev) { return false; }
    void MatchCollapsedStateToOtherTabs();
    void closePanel();
    Ui::PanelTitleBar* ui;
    QWidget* storedContents;
    bool m_collapsed = false;
private slots:
    void on_pushButton_clicked();
private:
    std::function<void()> m_callbackOnCollapse;
    virtual bool eventFilter(QObject* obj, QEvent* ev) override;
    virtual void paintEvent(QPaintEvent*) override;

    virtual QSize sizeHint() const override;
    virtual QSize minimumSizeHint() const override;
};

// Specialized PanelTitleBar that has extra features for attribute items.
class EDITOR_QT_UI_API ItemPanelTitleBar
    : public PanelTitleBar
{
    Q_OBJECT
public:
    explicit ItemPanelTitleBar(QWidget* parent, class CAttributeItem* item, bool isCustomPanel = false);

    void onVarChanged();
    void updateLabel();

    virtual ~ItemPanelTitleBar();
    bool isDefaultValue();


    virtual void updateColor() override;
    virtual void toggleAdvanced() override;
    void showAdvanced(bool show);
    bool isAdvancedVisible() const{return m_advancedVisible; }
    void RenamePanelName(QString name);
    QWidget* getStoredWidget();

signals:
    void SignalPanelRenameStart(QDockWidget* panel, QString origName, QString newName);
    void SignalPanelExportPanel(QDockWidget* panel, QString path);
    void SignalRemoveAllParams(QDockWidget* panel);
protected:
    void BuildCustomMenu(QMenu *);
    CAttributeItem* item;
    IVariable* m_variable;
    virtual bool onEventFilter(QObject* obj, QEvent* ev);
    void RenameRequest();
    void LaunchMenu(QPoint pos) override;
    bool m_advancedVisible;
    bool m_isCustom;
};

#endif // PANELTITLEBAR_H
