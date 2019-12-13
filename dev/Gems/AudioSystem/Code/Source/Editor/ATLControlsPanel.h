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
// Original file Copyright Crytek GMBH or its affiliates, used under license.

#pragma once

#include <AudioControl.h>
#include <AudioControlFilters.h>
#include <IAudioInterfacesCommonData.h>
#include <QTreeWidgetFilter.h>

#include <QWidget>
#include <QMenu>

#include <Source/Editor/ui_ATLControlsPanel.h>

// Forward declarations
namespace Audio
{
    struct IAudioProxy;
} // namespace Audio

class QStandardItemModel;
class QStandardItem;
class QSortFilterProxyModel;

namespace AudioControls
{
    class CATLControlsModel;
    class QFilterButton;
    class IAudioSystemEditor;
    class QATLTreeModel;

    //-------------------------------------------------------------------------------------------//
    class CATLControlsPanel
        : public QWidget
        , public Ui::ATLControlsPanel
        , public IATLControlModelListener
    {
        Q_OBJECT

    public:
        CATLControlsPanel(CATLControlsModel* pATLModel, QATLTreeModel* pATLControlsModel);
        ~CATLControlsPanel();

        ControlList GetSelectedControls();
        void Reload();

    private:
        // Filtering
        void ResetFilters();
        void ApplyFilter();
        bool ApplyFilter(const QModelIndex parent);
        bool IsValid(const QModelIndex index);
        void ShowControlType(EACEControlType type, bool bShow, bool bExclusive);

        // Helper Functions
        void SelectItem(QStandardItem* pItem);
        void DeselectAll();
        QStandardItem* GetCurrentItem();
        CATLControl* GetControlFromItem(QStandardItem* pItem);
        CATLControl* GetControlFromIndex(QModelIndex index);
        bool IsValidParent(QStandardItem* pParent, const EACEControlType eControlType);

        void HandleExternalDropEvent(QDropEvent* pDropEvent);

        // ------------- IATLControlModelListener ----------------
        virtual void OnControlAdded(CATLControl* pControl) override;
        // -------------------------------------------------------

        // ------------------ QWidget ----------------------------
        bool eventFilter(QObject* pObject, QEvent* pEvent);
        // -------------------------------------------------------

    private slots:
        void ItemModified(QStandardItem* pItem);

        QStandardItem* CreateFolder();
        QStandardItem* AddControl(CATLControl* pControl);

        // Create controls / folders
        void CreateRTPCControl();
        void CreateSwitchControl();
        void CreateStateControl();
        void CreateTriggerControl();
        void CreateEnvironmentsControl();
        void CreatePreloadControl();
        void DeleteSelectedControl();

        void ShowControlsContextMenu(const QPoint& pos);

        // Filtering
        void SetFilterString(const QString& text);
        void ShowTriggers(bool bShow);
        void ShowRTPCs(bool bShow);
        void ShowEnvironments(bool bShow);
        void ShowSwitches(bool bShow);
        void ShowPreloads(bool bShow);
        void ShowUnassigned(bool bShow);

        // Audio Preview
        void ExecuteControl();
        void StopControlExecution();

    signals:
        void SelectedControlChanged();
        void ControlTypeFiltered(EACEControlType type, bool bShow);

    private:
        QSortFilterProxyModel* m_pProxyModel;
        QATLTreeModel* m_pTreeModel;
        CATLControlsModel* const m_pATLModel;

        // Context Menu
        QMenu m_addItemMenu;

        // Filtering
        QString m_sFilter;
        QMenu m_filterMenu;
        QFilterButton* m_pControlTypeFilterButtons[eACET_NUM_TYPES];
        QFilterButton* m_unassignedFilterButton;
        bool m_visibleTypes[AudioControls::EACEControlType::eACET_NUM_TYPES];
        bool m_showUnassignedControls;
    };
} // namespace AudioControls
