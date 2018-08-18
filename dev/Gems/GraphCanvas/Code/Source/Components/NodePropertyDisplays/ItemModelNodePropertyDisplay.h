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
#pragma once

#include <qabstractitemmodel.h>
#include <qboxlayout.h>
#include <qpushbutton.h>
#include <qcombobox.h>
#include <qcompleter.h>
#include <qdialog.h>
#include <qevent.h>
#include <qlineedit.h>
#include <qobject.h>
#include <qsortfilterproxymodel.h>
#include <qtableview.h>
#include <qtimer.h>

#include <AzToolsFramework/API/ToolsApplicationAPI.h>

#include <Components/NodePropertyDisplays/StringNodePropertyDisplay.h>
#include <GraphCanvas/Components/NodePropertyDisplay/ItemModelDataInterface.h>

namespace Ui
{
    class ItemModelInspectionDialog;
}

namespace GraphCanvas
{
    class GraphCanvasLabel;

    class ItemModelInspectionFilterModel
        : public QSortFilterProxyModel
    {
    public:
        AZ_CLASS_ALLOCATOR(ItemModelInspectionFilterModel, AZ::SystemAllocator, 0);

        ItemModelInspectionFilterModel() = default;
        ~ItemModelInspectionFilterModel() = default;

        bool filterAcceptsRow(int sourceRow, const QModelIndex& sourceParent) const override;

        void SetFilter(const QString& filter);
        void ClearFilter();

    private:

        QString m_filter;
        QRegExp m_filterRegex;
    };
    
    class ItemModelInspectionDialog
        : public QDialog
    {
        Q_OBJECT
    public:
        AZ_CLASS_ALLOCATOR(ItemModelInspectionDialog, AZ::SystemAllocator, 0);

        ItemModelInspectionDialog(QWidget* parent, ItemModelDataInterface* itemPropertyDisplay);
        ~ItemModelInspectionDialog();

    public Q_SLOTS:

        void OnSubmitSelection(const QModelIndex& index);

        void OnFilterChanged(const QString& filter);
        void ForceUpdateFilter();
        void UpdateFilter();

    Q_SIGNALS:

        void OnIndexSelected(const QModelIndex& index);

    private:

        QTimer                  m_delayTimer;
        ItemModelDataInterface* m_dataInterface;

        Ui::ItemModelInspectionDialog* m_ui;
    };

    class ItemModelSelectionWidget
        : public QWidget
        , public AzToolsFramework::EditorEvents::Bus::Handler
    {
        Q_OBJECT

    public:
        AZ_CLASS_ALLOCATOR(ItemModelSelectionWidget, AZ::SystemAllocator, 0);

        ItemModelSelectionWidget(ItemModelDataInterface* dataInterface);
        ~ItemModelSelectionWidget() override;
        
        void UpdateDisplay();

        // AzToolsFramework::EditorEvents::Bus
        void OnEscape() override;
        ////        

    public Q_SLOTS:

        // Line Edit
        void HandleFocusIn();
        void HandleFocusOut();
        void SubmitSelection();
        ////

        // ToolButton
        void InspectModel();
        ////

        // ItemModelInspectionDialog
        void OnDialogSelection(const QModelIndex& modelIndex);
        ////

    Q_SIGNALS:

        void OnFocusIn();
        void OnFocusOut();

        void OnIndexSelected(const QModelIndex& modelIndex);

    private:

        void HandleIndexSubmission(const QModelIndex& modelIndex);    
        void SetCurrentIndex(const QModelIndex& modelIndex);

        Internal::FocusableLineEdit*        m_lineEdit;
        ItemModelDataInterface*             m_dataInterface;

        QCompleter*                         m_completer;
        QHBoxLayout*                        m_layout;
        
        QToolButton*                        m_inspectModelButton;

        QPersistentModelIndex               m_currentIndex;
    };

    class ItemModelNodePropertyDisplay
        : public NodePropertyDisplay
    {
        friend class ItemModelInspectionDialog;

    public:
        AZ_CLASS_ALLOCATOR(ItemModelNodePropertyDisplay, AZ::SystemAllocator, 0);
        ItemModelNodePropertyDisplay(ItemModelDataInterface* dataInterface);
        ~ItemModelNodePropertyDisplay() override;
        
        // NodePropertyDisplay
        void RefreshStyle() override;
        void UpdateDisplay() override;
        
        QGraphicsLayoutItem* GetDisabledGraphicsLayoutItem() override;
        QGraphicsLayoutItem* GetDisplayGraphicsLayoutItem() override;
        QGraphicsLayoutItem* GetEditableGraphicsLayoutItem() override;
        ////
       
    private:

        void EditStart();
        void AssignIndex(const QModelIndex& variableId);
        void EditFinished();
        void SetupProxyWidget();
        void CleanupProxyWidget();

        ItemModelDataInterface*     m_itemModelDataInterface;
        
        GraphCanvasLabel*           m_disabledLabel;
        GraphCanvasLabel*           m_displayLabel;

        QGraphicsProxyWidget*       m_proxyWidget;
        ItemModelSelectionWidget*   m_itemModelSelectionWidget;        
    };
}