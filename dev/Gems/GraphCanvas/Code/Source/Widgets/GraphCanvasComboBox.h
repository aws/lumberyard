/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright EntityRef license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/
#pragma once

#include <QCompleter>
#include <QDialog>
#include <QLineEdit>
#include <QMenu>
#include <QSortFilterProxyModel>
#include <QTableView>
#include <QTimer>
#include <QWidget>

#include <AzCore/Memory/SystemAllocator.h>

#include <GraphCanvas/Components/ViewBus.h>
#include <GraphCanvas/Editor/EditorTypes.h>
#include <GraphCanvas/Utils/StateControllers/StackStateController.h>
#include <GraphCanvas/Widgets/ComboBox/ComboBoxModel.h>

namespace GraphCanvas
{
    class GraphCanvasComboBoxFilterProxyModel
        : public QSortFilterProxyModel        
    {
        Q_OBJECT
    public:
        AZ_CLASS_ALLOCATOR(GraphCanvasComboBoxFilterProxyModel, AZ::SystemAllocator, 0);

        GraphCanvasComboBoxFilterProxyModel(QObject* parent = nullptr);

        // QSortfilterProxyModel
        bool filterAcceptsRow(int sourceRow, const QModelIndex& sourceParent) const override;
        ////

        void SetFilter(const QString& filter);

    private:
        QString m_filter;
        QRegExp m_testRegex;
    };

    class GraphCanvasComboBoxMenu
        : public QDialog        
    {
        Q_OBJECT
    public:
        GraphCanvasComboBoxMenu(ComboBoxModelInterface* model, QWidget* parent = nullptr);
        ~GraphCanvasComboBoxMenu() override;

        ComboBoxModelInterface* GetInterface();
        const ComboBoxModelInterface* GetInterface() const;

        GraphCanvasComboBoxFilterProxyModel* GetProxyModel();
        const GraphCanvasComboBoxFilterProxyModel* GetProxyModel() const;

        void ShowMenu();
        void HideMenu();

        void reject() override;
        bool eventFilter(QObject* object, QEvent* event) override;

        void focusInEvent(QFocusEvent* focusEvent) override;
        void focusOutEvent(QFocusEvent* focusEvent) override;

        void showEvent(QShowEvent* showEvent) override;
        void hideEvent(QHideEvent* hideEvent) override;

        GraphCanvas::StateController<bool>* GetDisableHidingStateController();

        void SetSelectedIndex(QModelIndex index);
        QModelIndex GetSelectedIndex() const;

        void OnTableClicked(const QModelIndex& modelIndex);

    Q_SIGNALS:

        void OnIndexSelected(QModelIndex index);
        void VisibilityChanged(bool visibility);

        void CancelMenu();

        void OnFocusIn();
        void OnFocusOut();

    private:

        void HandleFocusIn();
        void HandleFocusOut();

        QTimer m_closeTimer;
        QMetaObject::Connection m_closeConnection;

        QTableView m_tableView;

        ComboBoxModelInterface* m_modelInterface;
        GraphCanvasComboBoxFilterProxyModel m_filterProxyModel;

        GraphCanvas::StateSetter<bool> m_disableHidingStateSetter;
        GraphCanvas::StackStateController<bool> m_disableHiding;

        bool m_ignoreNextFocusIn;
    };

    class GraphCanvasComboBox
        : public QLineEdit
        , public ViewNotificationBus::Handler
    {
        Q_OBJECT
    public:
        AZ_CLASS_ALLOCATOR(GraphCanvasComboBox, AZ::SystemAllocator, 0);
        
        GraphCanvasComboBox(ComboBoxModelInterface* comboBoxModel, QWidget* parent = nullptr);
        ~GraphCanvasComboBox();

        void RegisterViewId(const ViewId& viewId);
        void SetAnchorPoint(QPoint globalPoint);
        void SetMenuWidth(qreal width);
        
        void SetSelectedIndex(const QModelIndex& selectedIndex);
        QModelIndex GetSelectedIndex() const;

        void ResetComboBox();
        void CancelInput();
        void HideMenu();        

        bool IsMenuVisible() const;

        // QLineEdit
        void focusInEvent(QFocusEvent* focusEvent) override;
        void focusOutEvent(QFocusEvent* focusEvent) override;

        void keyPressEvent(QKeyEvent* keyEvent) override;
        ////

        // ViewNotificationBus
        void OnViewScrolled() override;
        void OnViewCenteredOnArea() override;
        void OnZoomChanged(qreal zoomLevel) override;
        ////
        
    Q_SIGNALS:
       
        void SelectedIndexChanged(const QModelIndex& index);        

        void OnMenuAboutToDisplay();

        void OnFocusIn();
        void OnFocusOut();
        
    protected:
        
        void OnTextChanged();
        void OnOptionsClicked();
        void OnReturnPressed();
        void OnEditComplete();
        
        void UpdateFilter();
        void CloseMenu();

        void OnMenuFocusIn();
        void OnMenuFocusOut();

        void HandleFocusState();
        
    private:

        enum class CloseMenuState
        {
            Reject,
            Accept
        };

        bool DisplayIndex(const QModelIndex& index);
    
        bool SubmitData(bool allowRest = true);
        
        void DisplayMenu();
        QString GetUserInputText();

        void UpdateMenuPosition();

        QPoint m_anchorPoint;
        qreal m_displayWidth;

        bool m_lineEditInFocus;
        bool m_popUpMenuInFocus;

        QTimer m_focusTimer;
        bool m_hasFocus;
        
        QTimer m_filterTimer;

        QTimer m_closeTimer;
        CloseMenuState m_closeState;

        ViewId m_viewId;
        
        bool m_ignoreNextComplete;
        bool m_recursionBlocker;
    
        QModelIndex m_selectedIndex;
        
        QCompleter m_completer;
        GraphCanvasComboBoxMenu m_comboBoxMenu;

        ComboBoxModelInterface* m_modelInterface;
        
        GraphCanvas::StateSetter<bool> m_disableHidingStateSetter;
    };
    
    
}