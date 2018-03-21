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
#include "Camera_precompiled.h"
#include "ViewportCameraSelectorWindow.h"
#include <AzToolsFramework/Entity/EditorEntityContextBus.h>
#include <AzToolsFramework/API/ToolsApplicationAPI.h>
#include <AzFramework/Components/CameraBus.h>
#include <AzCore/Component/ComponentApplicationBus.h>
#include <AzToolsFramework/API/EditorCameraBus.h>
#include <AzCore/Component/EntityBus.h>
#include <IEditor.h>
#include <qmetatype.h>
#include <ViewManager.h>

namespace Qt
{
    enum
    {
        CameraIdRole = Qt::UserRole + 1,
    };
}
Q_DECLARE_METATYPE(AZ::EntityId);

namespace Camera
{
    struct ViewportCameraSelectorWindow;
    struct CameraListItem;
    struct CameraListModel;
    struct ViewportSelectorHolder;


    // Each item in the list holds the camera's entityId and name
    struct CameraListItem
        : public AZ::EntityBus::Handler
    {
    public:
        CameraListItem(const AZ::EntityId& cameraId)
            : m_cameraId(cameraId)
        {
            if (cameraId.IsValid())
            {
                AZ::ComponentApplicationBus::BroadcastResult(m_cameraName, &AZ::ComponentApplicationRequests::GetEntityName, cameraId);
                AZ::EntityBus::Handler::BusConnect(cameraId);
            }
            else
            {
                m_cameraName = "Editor camera";
            }
        }
        void OnEntityNameChanged(const AZStd::string& name) override { m_cameraName = name; }

        bool operator<(const CameraListItem& rhs)
        {
            return m_cameraId < rhs.m_cameraId;
        }

        AZ::EntityId m_cameraId;
        AZStd::string m_cameraName;
    };

    // holds a list of camera items
    struct CameraListModel
        : public QAbstractListModel
        , public CameraNotificationBus::Handler
    {
    public:
        CameraListModel(QObject* myParent)
            : QAbstractListModel(myParent)
        {
            m_cameraItems.push_back(AZ::EntityId());
            CameraNotificationBus::Handler::BusConnect();
        }
        ~CameraListModel()
        {
            // set the view entity id back to Invalid, thus enabling the editor camera
            EditorCameraRequests::Bus::Broadcast(&EditorCameraRequests::SetViewFromEntityPerspective, AZ::EntityId());
        }
        int rowCount(const QModelIndex& parent = QModelIndex()) const override
        {
            return m_cameraItems.size();
        }
        QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override
        {
            if (role == Qt::DisplayRole)
            {
                return m_cameraItems[index.row()].m_cameraName.c_str();
            }
            else if (role == Qt::CameraIdRole)
            {
                return QVariant::fromValue(m_cameraItems[index.row()].m_cameraId);
            }
            return QVariant();
        }

        void OnCameraAdded(const AZ::EntityId& cameraId) override
        {
            beginInsertRows(QModelIndex(), rowCount(), rowCount() + 1);
            m_cameraItems.push_back(cameraId);
            endInsertRows();
        }

        void OnCameraRemoved(const AZ::EntityId& cameraId) override
        {
            int index = 0;
            for (const CameraListItem& cameraItem : m_cameraItems)
            {
                if (cameraItem.m_cameraId == cameraId)
                {
                    break;
                }
                ++index;
            }
            beginRemoveRows(QModelIndex(), index, index + 1);
            m_cameraItems.erase(m_cameraItems.begin() + index);
            endRemoveRows();
        }

        QModelIndex GetIndexForEntityId(const AZ::EntityId entityId)
        {
            int row = 0;
            for (const CameraListItem& cameraItem : m_cameraItems)
            {
                if (cameraItem.m_cameraId == entityId)
                {
                    break;
                }
                ++row;
            }
            return index(row, 0);
        }
    private:
        AZStd::vector<CameraListItem> m_cameraItems;
    };

    struct ViewportCameraSelectorWindow
        : public QListView
        , public EditorCameraNotificationBus::Handler
        , public AzToolsFramework::EditorEntityContextNotificationBus::Handler
    {
    public:
        ViewportCameraSelectorWindow(QWidget* parent = nullptr)
        {
            qRegisterMetaType<AZ::EntityId>("AZ::EntityId");
            setParent(parent);
            setSelectionMode(QAbstractItemView::SingleSelection);
            setViewMode(ViewMode::ListMode);

            // display camera list
            CameraListModel* cameraList = new CameraListModel(this);

            // sort by entity id
            QSortFilterProxyModel* sortedProxyModel = new QSortFilterProxyModel(this);
            sortedProxyModel->setSourceModel(cameraList);
            setModel(sortedProxyModel);
            sortedProxyModel->setSortRole(Qt::CameraIdRole);

            // use the stylesheet for elements in a set where one item must be selected at all times
            setProperty("class", "SingleRequiredSelection");
            connect(cameraList, &CameraListModel::rowsInserted, this, [sortedProxyModel](const QModelIndex&, int, int) { sortedProxyModel->sortColumn(); });

            // highlight the current selected camera entity
            AZ::EntityId currentSelection;
            EditorCameraRequestBus::BroadcastResult(currentSelection, &EditorCameraRequestBus::Events::GetCurrentViewEntityId);
            OnViewportViewEntityChanged(currentSelection);

            // bus connections
            EditorCameraNotificationBus::Handler::BusConnect();
            AzToolsFramework::EditorEntityContextNotificationBus::Handler::BusConnect();
        }

        void currentChanged(const QModelIndex& current, const QModelIndex& previous) override
        {
            if (current.row() != previous.row())
            {
                AZ::EntityId entityId = selectionModel()->currentIndex().data(Qt::CameraIdRole).value<AZ::EntityId>();
                EditorCameraRequests::Bus::Broadcast(&EditorCameraRequests::SetViewFromEntityPerspective, entityId);
            }
        }

        //////////////////////////////////////////////////////////////////////////
        /// EditorCameraNotificationBus::Handler
        void OnViewportViewEntityChanged(const AZ::EntityId& newViewId) override
        {
            QModelIndex potentialIndex = static_cast<CameraListModel*>(qobject_cast<QSortFilterProxyModel*>(model())->sourceModel())->GetIndexForEntityId(newViewId);
            if (model()->hasIndex(potentialIndex.row(), potentialIndex.column()))
            {
                selectionModel()->setCurrentIndex(potentialIndex, QItemSelectionModel::SelectionFlag::ClearAndSelect);
            }
        }

        //////////////////////////////////////////////////////////////////////////
        /// AzToolsFramework::EditorEntityContextRequestBus::Handler
        // make sure we can only use this window while in Edit mode
        void OnStartPlayInEditor() override
        {
            setDisabled(true);
        }

        void OnStopPlayInEditor() override
        {
            setDisabled(false);
        }

        // swallow mouse move events so we can disable sloppy selection
        void mouseMoveEvent(QMouseEvent*) override {}

        // double click selects the entity
        void mouseDoubleClickEvent(QMouseEvent* event) override
        {
            AZ::EntityId entityId = selectionModel()->currentIndex().data(Qt::CameraIdRole).value<AZ::EntityId>();
            if (entityId.IsValid())
            {
                AzToolsFramework::ToolsApplicationRequestBus::Broadcast(&AzToolsFramework::ToolsApplicationRequestBus::Events::SetSelectedEntities, AzToolsFramework::EntityIdList { entityId });
            }
            else
            {
                AzToolsFramework::ToolsApplicationRequestBus::Broadcast(&AzToolsFramework::ToolsApplicationRequestBus::Events::SetSelectedEntities, AzToolsFramework::EntityIdList {});
            }
        }

        // handle up/down arrows to make a circular list
        QModelIndex moveCursor(CursorAction cursorAction, Qt::KeyboardModifiers modifiers) override
        {
            switch (cursorAction)
            {
            case CursorAction::MoveUp:
            {
                return GetPreviousIndex();
            }
            case CursorAction::MoveDown:
            {
                return GetNextIndex();
            }
            case CursorAction::MoveNext:
            {
                return GetNextIndex();
            }
            case CursorAction::MovePrevious:
            {
                return GetPreviousIndex();
            }
            default:
                return currentIndex();
            }
        }

        QModelIndex GetPreviousIndex() const
        {
            QModelIndex current = currentIndex();
            int previousRow = current.row() - 1;
            int rowCount = qobject_cast<QSortFilterProxyModel*>(model())->sourceModel()->rowCount();
            if (previousRow < 0)
            {
                previousRow = rowCount - 1;
            }
            return model()->index(previousRow, 0);
        }

        QModelIndex GetNextIndex() const
        {
            QModelIndex current = currentIndex();
            int nextRow = current.row() + 1;
            int rowCount = qobject_cast<QSortFilterProxyModel*>(model())->sourceModel()->rowCount();
            if (nextRow >= rowCount)
            {
                nextRow = 0;
            }
            return model()->index(nextRow, 0);
        }
    };

    // wrapper for the ViewportCameraSelectorWindow so that we can add some descriptive helpful text
    struct ViewportSelectorHolder
        : public QWidget
    {
        ViewportSelectorHolder(QWidget* parent = nullptr)
            : QWidget(parent)
        {
            setLayout(new QVBoxLayout(this));
            auto label = new QLabel("Select the camera you wish to view and navigate through.  Closing this window will return you to the default editor camera.", this);
            label->setWordWrap(true);
            layout()->addWidget(label);
            layout()->addWidget(new ViewportCameraSelectorWindow(this));
        }
    };

    // simple factory method
    static ViewportSelectorHolder* CreateNewSelectionWindow()
    {
        return new ViewportSelectorHolder();
    }


    void RegisterViewportCameraSelectorWindow()
    {
        QtViewOptions viewOptions;
        viewOptions.isPreview = true;
        viewOptions.showInMenu = true;
        viewOptions.preferedDockingArea = Qt::DockWidgetArea::LeftDockWidgetArea;
        AzToolsFramework::EditorRequestBus::Broadcast(&AzToolsFramework::EditorRequestBus::Events::RegisterViewPane, s_viewportCameraSelectorName, "Viewport", viewOptions, &CreateNewSelectionWindow);
    }
} // namespace Camera