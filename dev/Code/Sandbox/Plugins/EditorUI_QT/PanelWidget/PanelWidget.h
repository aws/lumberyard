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

#ifndef PANELWIDGET_H
#define PANELWIDGET_H


#include <functional>
#include <QMainWindow>
#include "../api.h"

class CAttributeItem;
class PanelTitleBar;

namespace Ui {
    class PanelWidget;
}

class EDITOR_QT_UI_API PanelWidget
    : public QMainWindow
{
    Q_OBJECT

public:
    explicit PanelWidget(QWidget* parent = 0);
    ~PanelWidget();

    //! addPanel creates a generic panel wrapper for the contentWidget.
    //! \return QDockWidget (root of the panel).
    QWidget* addPanel(const char* name, QWidget* contentWidget, bool scrollableArea /*=false*/);
    void RemovePanel(QDockWidget* dockPanel);

    void addShowPanelItemsToMenu(QMenu* menu);

    //! addPanel creates a specialized AttributeItem aware panel wrapper for the contentWidget.
    //! \return QDockWidget (root of the panel).
    QWidget* addItemPanel(const char* name, CAttributeItem* attr_item, bool scrollableArea /*=false*/, bool isCustomPanel = false, QString isShowInGroup = "both");

    //simplified method to add a custom panel to the panelWidget.
    void AddCustomPanel(QDockWidget* dockPanel, bool addToTop = true);

    bool isEmpty();
    QByteArray saveAll();
    QString saveCollapsedState();

    void iterateOver(std::function<void(QDockWidget*, PanelTitleBar*)> iteratorReceiver);
    void loadAll(const QByteArray& vdata);
    void loadCollapsedState(const QString& str);
    QString saveAdvancedState();
    void loadAdvancedState(const QString& str);

    void finalizePanels();
    void FixSizing(bool includeRoomForExpansion = false);

    void setAllowedAreas(Qt::DockWidgetArea allowedAreas);

    void ClearAllPanels();
    void ClearCustomPanels();

    void ResetGeometry();

signals:
    void SignalRenamePanel(QDockWidget* panel, QString origName, QString newName);
    void SignalExportPanel(QDockWidget* panel, QString origName);
    void SignalRemovePanel(QDockWidget* panel);
    void SignalRemoveAllParams(QDockWidget* panel);

private:
    virtual void paintEvent(QPaintEvent*) override;
    void ClosePanel(QDockWidget* panel);
    void PassThroughSignalPanelRename(QDockWidget* panel, QString origName, QString newName);
    void PassThroughSignalPanelExport(QDockWidget* panel, QString path);
    void CreateActions(QDockWidget*);

    int GetTotalPanelHeight(int& largestHeight);
    void AdjustCorrectionArea();

    virtual bool eventFilter(QObject*, QEvent*) override;

private:
    // Some of the docking areas have special behavior regarding the order in which new docked widgets are
    // ordered. RightDockWidgetArea is known to order docked widgets from top to bottom.
    // (This is related to changes made around CL357152 as part of JIRA https://jira.agscollab.com/browse/LY-43199)
    const Qt::DockWidgetArea m_mainDockWidgetArea = Qt::DockWidgetArea::RightDockWidgetArea;

    Ui::PanelWidget* ui;
    QByteArray m_defaultSettings;
    AZ_PUSH_DISABLE_DLL_EXPORT_MEMBER_WARNING
    QVector<QDockWidget*> m_widgets;
    AZ_POP_DISABLE_DLL_EXPORT_MEMBER_WARNING
    QDockWidget* m_correctionWidget;
    QDockWidget* m_topWidget;
};

#endif // PANELWIDGET_H
