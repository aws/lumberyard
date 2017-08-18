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
#include "stdafx.h"

#include "OutlinerListModel.hxx"

#include <QtCore/QMimeData>
#include <QtGui/QPainter>
#include <QtWidgets/QStyleOption>
#include <QtWidgets/QCheckBox>
#include <QtWidgets/qstyle.h>
#include <QGuiApplication.h>
#include <QTimer.h>

#include <AzCore/Asset/AssetManager.h>
#include <AzCore/Asset/AssetManagerBus.h>
#include <AzCore/Casting/numeric_cast.h>
#include <AzCore/Component/Entity.h>
#include <AzCore/Component/TransformBus.h>
#include <AzCore/Component/ComponentApplication.h>
#include <AzCore/Debug/Profiler.h>
#include <AzCore/IO/FileIO.h>
#include <AzCore/Slice/SliceAsset.h>
#include <AzCore/std/sort.h>

#include <AzToolsFramework/API/EntityCompositionRequestBus.h>
#include <AzToolsFramework/Entity/EditorEntityContextBus.h>
#include <AzToolsFramework/Entity/EditorEntityHelpers.h>
#include <AzToolsFramework/Entity/EditorEntityInfoBus.h>
#include <AzToolsFramework/Metrics/LyEditorMetricsBus.h>
#include <AzToolsFramework/Slice/SliceUtilities.h>
#include <AzToolsFramework/ToolsComponents/ComponentAssetMimeDataContainer.h>
#include <AzToolsFramework/ToolsComponents/ComponentMimeData.h>
#include <AzToolsFramework/ToolsComponents/EditorEntityIdContainer.h>
#include <AzToolsFramework/ToolsComponents/EditorLockComponentBus.h>
#include <AzToolsFramework/ToolsComponents/EditorVisibilityBus.h>
#include <AzToolsFramework/ToolsComponents/GenericComponentWrapper.h>
#include <AzToolsFramework/ToolsComponents/SelectionComponent.h>
#include <AzToolsFramework/ToolsComponents/TransformComponent.h>

#define VISIBILITY_CHECK_BOX_STYLE_SHEET_PATH "Code/Framework/AzToolsFramework/AzToolsFramework/UI/Outliner/Resources/VisibilityCheckBox.qss"
#define LOCK_CHECK_BOX_STYLE_SHEET_PATH "Code/Framework/AzToolsFramework/AzToolsFramework/UI/Outliner/Resources/LockCheckBox.qss"

namespace AzToolsFramework
{
    // Cached entity data for each entry in Outliner
    class OutlinerCache
        : private AZ::EntityBus::Handler
        , private EntitySelectionEvents::Bus::Handler
        , private OutlinerCacheRequestBus::Handler
        , private EditorLockComponentNotificationBus::Handler
        , private EditorVisibilityNotificationBus::Handler
    {
    public:
        AZ_CLASS_ALLOCATOR(OutlinerCache, AZ::SystemAllocator, 0);

        OutlinerCache(AZ::EntityId, OutlinerListModel* model, OutlinerCache* parent = nullptr);
        ~OutlinerCache();

        OutlinerCache(const OutlinerCache&) = delete;
        OutlinerCache& operator=(const OutlinerCache&) = delete;

        bool IsValidCache() const { return m_entityId.IsValid(); }

        /// Filter Visibility Methods
        bool GetVisible() const { return m_isVisible; }
        void SetVisible(bool visible);
        //! A recursive call that evaluates the visibility all of its children, based on a filter.
        //! Visible children will ensure that their parents are also visible, so that a QTreeView will show them.
        void CalculateVisibility(const FilterByCategoryMap& visibilityFilter, FilterOperatorType filterOperator);

        ////////////////////////////////////////////////////////////////////////
        // EditorLockComponentNotificationBus::Handler
        // Outliner ignores entities not owned by the editor context
        ////////////////////////////////////////////////////////////////////////
        void OnEntityLockChanged(bool /*locked*/);

        ////////////////////////////////////////////////////////////////////////
        // EditorVisibilityNotificationBus::Handler
        // Outliner ignores entities not owned by the editor context
        ////////////////////////////////////////////////////////////////////////
        void OnEntityVisibilityFlagChanged(bool /*visibility*/);

        ////////////////////////////////////////////////////////////////////////
        // AZ::EntityBus
        // Connect for each entity shown in the Outliner
        ////////////////////////////////////////////////////////////////////////
        void OnEntityNameChanged(const AZStd::string& name) override;

        /// Hierarchy Methods
        const OutlinerCache* GetChild(int row) const;
        int GetChildCount() const;
        int GetColumnCount() const { return 1; }
        AZ::EntityId GetData() const { return m_entityId; }
        int GetRow() const { return m_row; }
        bool IsSelected() const { return m_selected; }

        void SetParent(OutlinerCache* p);
        const OutlinerCache* GetParent() const { return m_parentItem; }
        OutlinerCache* GetParent() { return m_parentItem; }

        void AppendChild(OutlinerCache* child);
        void AdoptChildren(OutlinerCache* previousParent); //!< take children from previousParent
        void RemoveChild(OutlinerCache* child);
        void RenumberChildren();

        /// Tree Node Methods
        // Expansion state of the node in the hierarchy.
        // Used when an object is deleted to store it's expansion state for use if the object is restored.
        bool IsExpanded() const { return m_isExpanded; }

        // Expansion State Tracking
        void SetExpanded(bool expansionState);
        void OnParentExpanded();
        void OnParentCollapsed();

        // Selection State Tracking
        void OnDescendantSelected();
        void OnDescendantDeselected();
        bool HasSelectedChildren() const;

        //////////////////////////////////////////////////////////////////////////
        // AzToolsFramework::EntitySelectionEvents::Handler
        // Used to handle selection changes on a per-entity basis.
        //////////////////////////////////////////////////////////////////////////
        void OnSelected() override;                                 // Called when the entity associated with this cache is selected
        void OnDeselected() override;                               // Called when the entity associated with this cache is deselected

        // Resend the current selection information to the outliner
        // This should be used when an object is moved around in the hierarchy and it's model index information changes.
        void ResendSelectionInformation();

        //////////////////////////////////////////////////////////////////////////
        // AzToolsFramework::OutlinerCacheRequestBus::Handler
        // Used to handle requests made by the tree view.
        //////////////////////////////////////////////////////////////////////////
        void SelectOutlinerCache(QModelIndex index) override;
        void DeselectOutlinerCache(QModelIndex index) override;

        /// Entity Methods
        const QString& GetEntityName() const { return m_entityName; }

        /// Slice Information
        void SetSliceInformation(QString name);
        void ClearSliceInformation();
        void SetAsSliceRoot(bool isSliceRoot) { m_isSliceRoot = isSliceRoot; }
        bool IsSliceRoot() const { return m_isSliceRoot; }
        bool IsSliceEntity() const { return m_isSliceEntity; }
        const QString& GetSliceAssetName() const { return m_sliceAssetName; }

    private:
        void ResetEntityName();

        bool m_isSliceRoot;
        bool m_isSliceEntity;

        AZ::EntityId m_entityId;
        AZStd::vector<OutlinerCache*> m_children;
        OutlinerCache* m_parentItem;
        int m_row;

        // Status Flags:
        // Set to false if the item is hidden by a filter
        bool m_isVisible;
        // Set to true  if the item is selected
        bool m_selected;
        // Set to true if a parent in the hierarchy is collapsed
        bool m_hiddenByHierarchy;
        // A counter of the number of children of this item that are selected
        unsigned int m_selectedDescendantCount;
        // True if the item has children and is currently expanded in the tree to show them
        bool m_isExpanded;

        OutlinerListModel* m_model;

        // cached entity data
        QString m_entityName;
        QString m_sliceAssetName;
        QString m_filterString;
    };

    ////////////////////////////////////////////////////////////////////////////
    // OutlinerCache
    ////////////////////////////////////////////////////////////////////////////

    OutlinerCache::OutlinerCache(AZ::EntityId data, OutlinerListModel* model, OutlinerCache* parent)
        : m_entityId(data)
        , m_row(0)
        , m_selectedDescendantCount(0)
        , m_parentItem(parent)
        , m_isVisible(true)
        , m_selected(false)
        , m_hiddenByHierarchy(false)
        , m_isExpanded(true)            // When a new cache entry is created, it will be assigned children and then told to collapse. Initializing this to true guarantees that information will be propgated to children.
        , m_isSliceEntity(false)
        , m_isSliceRoot(false)
        , m_model(model)
    {
        ResetEntityName();
        OutlinerCacheRequestBus::Handler::BusConnect();

        if (IsValidCache())
        {
            // Connect to various ebus interfaces to listen for information about selection and data changes
            EditorLockComponentNotificationBus::Handler::BusConnect(m_entityId);
            EntitySelectionEvents::Bus::Handler::BusConnect(m_entityId);
            EditorVisibilityNotificationBus::Handler::BusConnect(m_entityId);
            AZ::EntityBus::Handler::BusConnect(m_entityId);
        }
    }

    OutlinerCache::~OutlinerCache()
    {
        OutlinerCacheRequestBus::Handler::BusDisconnect();

        if (IsValidCache())
        {
            EntitySelectionEvents::Bus::Handler::BusDisconnect();
            EditorLockComponentNotificationBus::Handler::BusDisconnect();
            EditorVisibilityNotificationBus::Handler::BusDisconnect();
            AZ::EntityBus::Handler::BusDisconnect(m_entityId);
        }

        for (size_t i = 0; i < m_children.size(); i++)
        {
            if (m_children[i] != nullptr)
            {
                delete m_children[i];
            }
        }
    }

    void OutlinerCache::SetSliceInformation(QString name)
    {
        m_sliceAssetName = name;
        m_isSliceEntity = true;
    }

    void OutlinerCache::ClearSliceInformation()
    {
        m_sliceAssetName = "";
        m_isSliceRoot = false;
        m_isSliceEntity = false;
    }

    void OutlinerCache::SetVisible(bool visible)
    {
        if (m_entityId.IsValid())
        {
            if (m_isVisible != visible)
            {
                m_isVisible = visible;
            }
        }
    }

    void OutlinerCache::CalculateVisibility(const AzToolsFramework::FilterByCategoryMap& visibilityFilter, AzToolsFramework::FilterOperatorType filterOperator)
    {
        //  Set the visibility as a breadth-first search of the tree.
        //  This will allow us to also set our parents visible if we are visible
        //  without a later search overriding us.
        bool matchesFilter = true;
        OutlinerCache* visiblityParent = GetParent();
        //  Only one OutlinerCache has no parent: the root. It must remain visible, no matter the filter.
        //  The root item does not represent an actual entity, so this is okay.
        if (visiblityParent)
        {
            //  It may look like more work to check these for every item, but the checks are needed anyway to keep from
            //  attempting to use valid but empty filters.
            bool validNameFilter = visibilityFilter.count("name") > 0 && !visibilityFilter.at("name").isEmpty();

            //  "And" checks need to start out True so that one False will fail.
            //  "Or" checks must start False so that any True will pass them.
            //  But only start false if there are valid filters to check against, otherwise you will never pass.
            if ((validNameFilter) && filterOperator == FilterOperatorType::Or)
            {
                matchesFilter = false;
            }

            if (validNameFilter)
            {
                QString assetName = GetEntityName();
                bool matchesName = assetName.contains(visibilityFilter.at("name"));

                if (filterOperator == FilterOperatorType::And)
                {
                    matchesFilter &= matchesName;
                }
                else
                {
                    matchesFilter |= matchesName;
                }
            }
        }
        SetVisible(matchesFilter);

        if (matchesFilter && visiblityParent)
        {
            //  Set all parents to visible.
            bool isVisible = visiblityParent->GetVisible();
            while (!isVisible)    //  Checking isVisible gives us a short circuit for already visible items.
            {
                visiblityParent->SetVisible(true);
                visiblityParent = visiblityParent->GetParent();

                isVisible = visiblityParent ? visiblityParent->GetVisible() : true;
            }
        }

        //  Recurse through the children
        for (OutlinerCache* child : m_children)
        {
            child->CalculateVisibility(visibilityFilter, filterOperator);
        }
    }

    void OutlinerCache::ResetEntityName()
    {
        if (m_entityId.IsValid())
        {
            AZ::Entity* entity = nullptr;
            AZ::ComponentApplicationBus::BroadcastResult(entity, &AZ::ComponentApplicationRequests::FindEntity, m_entityId);
            AZ_Assert(entity, "Outliner entry cannot locate its entity");

            if (entity)
            {
                m_entityName = entity->GetName().c_str();
            }
        }
        else
        {
            // For Debug Purposes
            m_entityName = "Root";
        }
    }

    void OutlinerCache::OnEntityLockChanged(bool /*locked*/)
    {
        if (IsValidCache())
        {
            OutlinerCacheNotificationBus::Broadcast(&OutlinerCacheNotifications::EntityCacheChanged, m_entityId);
        }
    }

    void OutlinerCache::OnEntityVisibilityFlagChanged(bool /*visibility*/)
    {
        if (IsValidCache())
        {
            OutlinerCacheNotificationBus::Broadcast(&OutlinerCacheNotifications::EntityCacheChanged, m_entityId);
        }
    }

    void OutlinerCache::OnEntityNameChanged(const AZStd::string& newName)
    {
        if (IsValidCache())
        {
            m_entityName = newName.c_str();
            OutlinerCacheNotificationBus::Broadcast(&OutlinerCacheNotifications::EntityCacheChanged, m_entityId);
        }
    }

    void OutlinerCache::AppendChild(OutlinerCache* child)
    {
        child->m_row = aznumeric_caster(m_children.size());
        m_children.push_back(child);
        child->SetParent(this);

        // The model index for this item is changing.
        // Make sure the outliner knows to maintain selection on this entity.
        ResendSelectionInformation();

        if (IsValidCache())
        {
            // Fix Child Selected values
            if (child->IsSelected())
            {
                OnDescendantSelected();
            }
            for (unsigned i = 0; i < child->m_selectedDescendantCount; ++i)
            {
                OnDescendantSelected();
            }

            // If we've added our very first child, we need to invalidate.
            if (m_children.size() == 1)
            {
                OutlinerCacheNotificationBus::Broadcast(&OutlinerCacheNotifications::EntityCacheChanged, m_entityId);
            }
        }
    }

    void OutlinerCache::AdoptChildren(OutlinerCache* previousParent)
    {
        size_t previousChildCount = m_children.size();

        for (OutlinerCache* child : previousParent->m_children)
        {
            AppendChild(child);
        }
        previousParent->m_children.clear();

        if (IsValidCache())
        {
            if (previousChildCount == 0)
            {
                OutlinerCacheNotificationBus::Broadcast(&OutlinerCacheNotifications::EntityCacheChanged, m_entityId);
            }
        }
    }

    void OutlinerCache::RemoveChild(OutlinerCache* child)
    {
        auto childIt = AZStd::find(m_children.begin(), m_children.end(), child);
        if (childIt != m_children.end())
        {
            // erase child entry and renumber the 'm_row' of all subsequent children.
            m_children.erase(childIt);
            RenumberChildren();
        }

        child->SetParent(nullptr);
        child->m_row = 0;

        if (IsValidCache())
        {
            // Fix Child Selected values
            if (child->IsSelected())
            {
                OnDescendantDeselected();
            }
            for (unsigned i = 0; i < child->m_selectedDescendantCount; ++i)
            {
                OnDescendantDeselected();
            }

            // Invalidation occurs when the number of children is reduced to zero.
            if (m_children.empty())
            {
                OutlinerCacheNotificationBus::Broadcast(&OutlinerCacheNotifications::EntityCacheChanged, m_entityId);
            }
        }
    }

    void OutlinerCache::RenumberChildren()
    {
        for (int i = 0; i < m_children.size(); ++i)
        {
            OutlinerCache* childCache = m_children[i];
            childCache->m_row = i;
        }
    }

    const OutlinerCache* OutlinerCache::GetChild(int row) const
    {
        if (row < 0 || row >= static_cast<int>(m_children.size()))
        {
            return nullptr;
        }
        return m_children[row];
    }

    int OutlinerCache::GetChildCount() const
    {
        return (int)m_children.size();
    }

    void OutlinerCache::SetParent(OutlinerCache* parent)
    {
        m_parentItem = parent;
        if (parent)
        {
            if (m_parentItem->IsExpanded())
            {
                OnParentExpanded();
            }
            else
            {
                OnParentCollapsed();
            }
        }
    }

    void OutlinerCache::SetExpanded(bool expansionState)
    {
        if (m_isExpanded != expansionState)
        {
            m_isExpanded = expansionState;

            for (OutlinerCache* child : m_children)
            {
                if (m_isExpanded)
                {
                    child->OnParentExpanded();
                }
                else
                {
                    child->OnParentCollapsed();
                }
            }
        }
    }

    void OutlinerCache::OnParentExpanded()
    {
        if (m_entityId.IsValid())
        {
            m_hiddenByHierarchy = false;

            // Only mark children if expanded if this node is expanded as well.
            if (m_isExpanded)
            {
                for (OutlinerCache* child : m_children)
                {
                    child->OnParentExpanded();
                }
            }
        }
    }

    void OutlinerCache::OnParentCollapsed()
    {
        if (m_entityId.IsValid())
        {
            m_hiddenByHierarchy = true;

            for (OutlinerCache* child : m_children)
            {
                child->OnParentCollapsed();
            }
        }
    }

    void OutlinerCache::OnSelected()
    {
        if (m_entityId.IsValid())
        {
            if (m_selected)
            {
                return;
            }

            m_selected = true;
            OutlinerCache* parent = GetParent();
            if (parent->IsValidCache())
            {
                parent->OnDescendantSelected();
            }

            OutlinerCacheNotificationBus::Broadcast(&OutlinerCacheNotifications::EntityCacheSelectionRequest, m_entityId);
            if (!m_hiddenByHierarchy && m_isVisible)
            {
                OutlinerCacheNotificationBus::Broadcast(&OutlinerCacheNotifications::EntityCacheChanged, m_entityId);
            }
        }
    }

    void OutlinerCache::OnDeselected()
    {
        if (m_entityId.IsValid())
        {
            if (!m_selected)
            {
                return;
            }

            m_selected = false;
            OutlinerCache* parent = GetParent();
            // The root is a special case.
            if (parent->IsValidCache())
            {
                parent->OnDescendantDeselected();
            }

            OutlinerCacheNotificationBus::Broadcast(&OutlinerCacheNotifications::EntityCacheDeselectionRequest, m_entityId);
            if (!m_hiddenByHierarchy && m_isVisible)
            {
                OutlinerCacheNotificationBus::Broadcast(&OutlinerCacheNotifications::EntityCacheChanged, m_entityId);
            }
        }
    }

    void OutlinerCache::ResendSelectionInformation()
    {
        if (m_entityId.IsValid())
        {
            // We only have to resend information for objects that are selected.
            if (m_selected)
            {
                OutlinerCacheNotificationBus::Broadcast(&OutlinerCacheNotifications::EntityCacheSelectionRequest, m_entityId);

                if (!m_hiddenByHierarchy && m_isVisible)
                {
                    OutlinerCacheNotificationBus::Broadcast(&OutlinerCacheNotifications::EntityCacheChanged, m_entityId);
                }
            }
        }
    }

    void OutlinerCache::OnDescendantSelected()
    {
        if (m_entityId.IsValid())
        {
            ++m_selectedDescendantCount;

            OutlinerCache* parent = GetParent();
            // The root is a special case and is the only entry in the cache that has no parent.
            // It isn't drawn and has no entity associated with it so we don't manage it's data.
            if (parent->IsValidCache())
            {
                parent->OnDescendantSelected();
            }

            if (!m_isExpanded && m_selectedDescendantCount == 1)
            {
                OutlinerCacheNotificationBus::Broadcast(&OutlinerCacheNotifications::EntityCacheChanged, m_entityId);
            }
        }
    }

    void OutlinerCache::OnDescendantDeselected()
    {
        if (m_entityId.IsValid())
        {
            AZ_Assert(m_selectedDescendantCount, "Trying to decrement a child selection count that's already zero on entity %s", m_entityName.toStdString().c_str());

            --m_selectedDescendantCount;

            OutlinerCache* parent = GetParent();
            // The root is a special case.
            if (parent->IsValidCache())
            {
                parent->OnDescendantDeselected();
            }

            if (!m_isExpanded && m_selectedDescendantCount == 0)
            {
                OutlinerCacheNotificationBus::Broadcast(&OutlinerCacheNotifications::EntityCacheChanged, m_entityId);
            }
        }
    }

    bool OutlinerCache::HasSelectedChildren() const
    {
        return m_selectedDescendantCount > 0;
    }

    void OutlinerCache::SelectOutlinerCache(QModelIndex index)
    {
        if (m_entityId.IsValid())
        {
            // Check to see if this is intended for us...
            if (this == static_cast<OutlinerCache*>(index.internalPointer()))
            {
                // Check to see if the item is in a state to be selected
                if (!m_selected)
                {
                    ToolsApplicationRequestBus::Broadcast(&ToolsApplicationRequests::MarkEntitySelected, m_entityId);
                }
            }
        }
    }

    void OutlinerCache::DeselectOutlinerCache(QModelIndex index)
    {
        if (m_entityId.IsValid())
        {
            // Check to see if this is intended for us...
            if (this == static_cast<OutlinerCache*>(index.internalPointer()))
            {
                // Check to see if the item is in a state to be deselected
                if (m_selected)
                {
                    ToolsApplicationRequestBus::Broadcast(&ToolsApplicationRequests::MarkEntityDeselected, m_entityId);
                }
            }
        }
    }

    ////////////////////////////////////////////////////////////////////////////
    // OutlinerSliceHandle
    // The Outliner Slice Handle is currently only used to store results
    // when finding all the roots of a slice. It stores all the information
    // needed when injecting an artificial entry into the outliner that
    // behaves as a slice handle.
    ////////////////////////////////////////////////////////////////////////////

    OutlinerSliceHandle::OutlinerSliceHandle(const AZStd::string& name, const AZ::SliceComponent::SliceInstanceAddress& instanceId, SliceHandleAddress handleId)
        : m_name(name)
        , m_instance(instanceId)
        , m_handleId(handleId)
    {
        m_parentEntity.SetInvalid();
    }

    OutlinerSliceHandle::OutlinerSliceHandle(OutlinerSliceHandle&& rhs)
        : m_name(AZStd::move(rhs.m_name))
        , m_instance(rhs.m_instance)
        , m_handleId(rhs.m_handleId)
        , m_parentEntity(rhs.m_parentEntity)
        , m_rootEntities(AZStd::move(rhs.m_rootEntities))
    {
        rhs.m_instance = AZStd::make_pair(nullptr, nullptr);
        m_handleId = SliceHandleAddress::CreateNull();
        rhs.m_parentEntity.SetInvalid();
        rhs.m_rootEntities.clear();
    }

    OutlinerSliceHandle& OutlinerSliceHandle::operator=(OutlinerSliceHandle&& rhs)
    {
        m_name = AZStd::move(rhs.m_name);
        m_instance = rhs.m_instance;
        m_handleId = rhs.m_handleId;
        m_parentEntity = rhs.m_parentEntity;
        m_rootEntities = AZStd::move(rhs.m_rootEntities);

        rhs.m_instance = AZStd::make_pair(nullptr, nullptr);
        m_handleId = SliceHandleAddress::CreateNull();
        rhs.m_parentEntity.SetInvalid();
        rhs.m_rootEntities.clear();

        return *this;
    }

    const AZStd::string& OutlinerSliceHandle::GetName() const
    {
        return m_name;
    }

    const OutlinerSliceHandle::SliceHandleAddress& OutlinerSliceHandle::GetId() const
    {
        return m_handleId;
    }

    const AZ::SliceComponent::SliceInstanceId& OutlinerSliceHandle::GetInstanceId() const
    {
        return m_instance.second->GetId();
    }


    const EntityIdList& OutlinerSliceHandle::GetRootEntities() const
    {
        return m_rootEntities;
    }

    // Functions for building the slice handle information
    void OutlinerSliceHandle::AddRootEntity(const AZ::EntityId& rootId)
    {
        AZ_Assert(AZStd::find(m_rootEntities.begin(), m_rootEntities.end(), rootId) == m_rootEntities.end(), "Attempting to add a root entity to a slice handle that is already registered as a root entity");
        m_rootEntities.push_back(rootId);
    }

    void OutlinerSliceHandle::SetParentEntity(const AZ::EntityId& parentId)
    {
        m_parentEntity = parentId;
    }

    ////////////////////////////////////////////////////////////////////////////
    // OutlinerListModel
    ////////////////////////////////////////////////////////////////////////////

    OutlinerListModel::OutlinerListModel(QObject* parent)
        : QAbstractItemModel(parent)
        , m_bufferProcessingPending(false)
    {
        m_entitiesCacheRoot = aznew OutlinerCache(AZ::EntityId(), this);
    }

    OutlinerListModel::~OutlinerListModel()
    {
        ToolsApplicationEvents::Bus::Handler::BusDisconnect();
        EditorEntityContextNotificationBus::Handler::BusDisconnect();
        OutlinerCacheNotificationBus::Handler::BusDisconnect();

        if (m_entitiesCacheRoot)
        {
            delete m_entitiesCacheRoot;
            m_entitiesCacheRoot = nullptr;
        }
    }

    void OutlinerListModel::Initialize()
    {
        ToolsApplicationEvents::Bus::Handler::BusConnect();
        EditorEntityContextNotificationBus::Handler::BusConnect();
        OutlinerCacheNotificationBus::Handler::BusConnect();

        RegisterExistingEntities();
    }

    int OutlinerListModel::rowCount(const QModelIndex& parent) const
    {
        const OutlinerCache* parentItem = GetValidCacheFromIndex(parent);
        return parentItem ? parentItem->GetChildCount() : 0;
    }

    QModelIndex OutlinerListModel::index(int row, int column, const QModelIndex& parent) const
    {
        // sanity check
        if (!hasIndex(row, column, parent))
        {
            return QModelIndex();
        }

        // can only be parented to first column
        if (parent.isValid() && parent.column() != 0)
        {
            return QModelIndex();
        }

        const OutlinerCache* parentItem = GetValidCacheFromIndex(parent);
        const OutlinerCache* childItem = parentItem->GetChild(row);
        return childItem ? GetIndexFromCache(childItem, column) : QModelIndex();
    }

    QVariant OutlinerListModel::data(const QModelIndex& index, int role) const
    {
        const OutlinerCache* item = GetValidCacheFromIndex(index);
        if (!item || (item == m_entitiesCacheRoot))
        {
            return QVariant();
        }

        switch (index.column())
        {
        case ColumnName:
            return dataForName(index, role);

        case ColumnVisibilityToggle:
            return dataForVisibility(index, role);

        case ColumnLockToggle:
            return dataForLock(index, role);

        case ColumnSortIndex:
            return dataForSortIndex(index, role);

        case ColumnEntityId:    // Used for Sorting
            return AZ::u64(GetEntityAtIndex(index));

        case ColumnSliceMembership:
            return (!item->IsSliceEntity());    // Used for Sorting (Slice entities should appear above non-slice entities for outliner clarity)
        }

        return QVariant();
    }

    QVariant OutlinerListModel::dataForAll(const QModelIndex& index, int role) const
    {
        const OutlinerCache* item = GetValidCacheFromIndex(index);

        switch (role)
        {
        case SliceBackgroundRole:
            return item->IsSliceEntity() && item->IsSliceRoot();

        case SelectedRole:
            return item->IsSelected();

        case ChildSelectedRole:
            return item->HasSelectedChildren();

        case ExpandedRole:
            return item->IsExpanded();

        case VisibilityRole:
            return item->GetVisible();
        }

        return QVariant();
    }

    QVariant OutlinerListModel::dataForName(const QModelIndex& index, int role) const
    {
        const OutlinerCache* item = GetValidCacheFromIndex(index);

        switch (role)
        {
        case Qt::EditRole:
            return item->GetEntityName();

        case Qt::DisplayRole:
            return item->GetEntityName();

        case Qt::ToolTipRole:
            if (!item->GetSliceAssetName().isEmpty())
            {
                return QObject::tr("Slice asset: %1").arg(item->GetSliceAssetName());
            }

            return QString("Slice asset: This entity is not part of a slice.");

        case EntityTypeRole:
            if (item->IsSliceEntity())
            {
                return item->IsSliceRoot() ? SliceHandleType : SliceEntityType;
            }

            // Guaranteed to return a valid type.
            return EntityType;

        case Qt::ForegroundRole:
        {
            // We use the parent's palette because the GUI Application palette is returning the wrong colors
            auto parentWidgetPtr = static_cast<QWidget*>(QObject::parent());
            return QBrush(parentWidgetPtr->palette().color(item->IsSliceEntity() ? QPalette::Link : QPalette::Text));
        }

        case Qt::DecorationRole:
            return QIcon(item->IsSliceEntity() ? ":/Icons/Slice_Entity_Icon.tif" : ":/Icons/Entity_Icon.tif");
        }

        return dataForAll(index, role);
    }

    QVariant OutlinerListModel::dataForVisibility(const QModelIndex& index, int role) const
    {
        switch (role)
        {
        case Qt::CheckStateRole:
        {
            bool visible = true;
            AzToolsFramework::EditorVisibilityRequestBus::EventResult(visible, GetEntityAtIndex(index), &AzToolsFramework::EditorVisibilityRequests::GetCurrentVisibility);
            return static_cast<int>(visible ? Qt::Checked : Qt::Unchecked);
        }

        case Qt::ToolTipRole:
            return QString("Show/Hide Entity");
        }

        return dataForAll(index, role);
    }

    QVariant OutlinerListModel::dataForLock(const QModelIndex& index, int role) const
    {
        switch (role)
        {
        case Qt::CheckStateRole:
        {
            bool locked = false;
            AzToolsFramework::EditorLockComponentRequestBus::EventResult(locked, GetEntityAtIndex(index), &AzToolsFramework::EditorLockComponentRequests::GetLocked);
            return static_cast<int>(locked ? Qt::Checked : Qt::Unchecked);
        }

        case Qt::ToolTipRole:
            return QString("Lock/Unlock Entity (Locked means the entity is not selectable)");
        }

        return dataForAll(index, role);
    }

    QVariant OutlinerListModel::dataForSortIndex(const QModelIndex& index, int role) const
    {
        const OutlinerCache* item = GetValidCacheFromIndex(index);

        switch (role)
        {
        case Qt::DisplayRole:
            AZ::u64 sortIndex = 0;
            AzToolsFramework::EditorEntityInfoRequestBus::EventResult(sortIndex, item->GetData(), &AzToolsFramework::EditorEntityInfoRequestBus::Events::SiblingSortIndex);
            return sortIndex;
        }

        return dataForAll(index, role);
    }

    bool OutlinerListModel::setData(const QModelIndex& index, const QVariant& value, int role)
    {
        switch (role)
        {
        case Qt::CheckStateRole:
        {
            if (value.canConvert<Qt::CheckState>())
            {
                switch (index.column())
                {
                case ColumnVisibilityToggle:
                {
                    ToggleEditorVisibility(index);
                }
                break;

                case ColumnLockToggle:
                {
                    ToggleEditorLockState(index);
                }
                break;
                }

                // Explicitly let the outliner know that this particular entry in the tree has changed
                // forcing the tree to redraw it
                Q_EMIT dataChanged(index, index, { role });
            }
        }
        break;

        case Qt::EditRole:
        {
            if (index.column() == ColumnName && !value.toString().isEmpty())
            {
                OutlinerCache* item = static_cast<OutlinerCache*>(index.internalPointer());
                if (item)
                {
                    AZ::EntityId id = item->GetData();
                    if (id.IsValid())
                    {
                        AZ::Entity* entity = nullptr;
                        AZ::ComponentApplicationBus::BroadcastResult(entity, &AZ::ComponentApplicationRequests::FindEntity, id);
                        AZ_Assert(entity, "Outliner entry cannot locate entity with cached name: %s", item->GetEntityName().toStdString().c_str());

                        if (entity)
                        {
                            ScopedUndoBatch undo("Rename Entity");

                            entity->SetName(value.toString().toStdString().c_str());
                            undo.MarkEntityDirty(entity->GetId());

                            EBUS_EVENT(ToolsApplicationEvents::Bus, InvalidatePropertyDisplay, Refresh_EntireTree);
                        }
                    }
                }
            }
        }
        break;
        }

        return QAbstractItemModel::setData(index, value, role);
    }

    QModelIndex OutlinerListModel::parent(const QModelIndex& index) const
    {
        if (!index.isValid())
        {
            return QModelIndex();
        }

        const OutlinerCache* childItem = GetValidCacheFromIndex(index);
        const OutlinerCache* parentItem = childItem != nullptr ? childItem->GetParent() : nullptr;

        if (parentItem == m_entitiesCacheRoot || parentItem == nullptr)
        {
            return QModelIndex();
        }

        return GetIndexFromCache(parentItem);
    }

    AZ::EntityId OutlinerListModel::GetEntityAtIndex(const QModelIndex& index) const
    {
        if (index.isValid())
        {
            // QModelIndex contains pointer to cache
            OutlinerCache* item = static_cast<OutlinerCache*>(index.internalPointer());
            if (item)
            {
                return item->GetData();
            }
        }
        return AZ::EntityId();
    }

    bool OutlinerListModel::IsSelected(const AZ::EntityId& entityId) const
    {
        if (entityId.IsValid())
        {
            auto cache = GetCacheForEntity(entityId);

            if (cache)
            {
                return cache->IsSelected();
            }
        }
        return false;
    }

    bool OutlinerListModel::IsEligibleForDeselect(const AZ::EntityId& entityId) const
    {
        // If we're requesting a deselect but already have a register and select request, don't deselect this
        // Because we queue up actions, this prevents us from deselecting something that should have ended up selected if processed serially
        auto registerIt = m_entityRegistrationBuffer.find(entityId);
        if (registerIt != m_entityRegistrationBuffer.end())
        {
            auto selectIt = m_selectionRequestBuffer.find(entityId);
            if (selectIt != m_selectionRequestBuffer.end())
            {
                return false;
            }
        }

        return true;
    }

    Qt::ItemFlags OutlinerListModel::flags(const QModelIndex& index) const
    {
        if (index.column() == ColumnVisibilityToggle || index.column() == ColumnLockToggle)
        {
            return Qt::ItemIsEnabled | Qt::ItemIsEditable | Qt::ItemIsUserCheckable;
        }

        Qt::ItemFlags itemFlags = QAbstractItemModel::flags(index) | Qt::ItemIsDropEnabled | Qt::ItemIsEditable | Qt::ItemIsSelectable | Qt::ItemIsEnabled;

        AZ::EntityId entityId = GetEntityAtIndex(index);
        if (entityId.IsValid())
        {
            bool entitySelectable = true;
            EBUS_EVENT_RESULT(entitySelectable, AzToolsFramework::ToolsApplicationRequests::Bus, IsSelectable, entityId);
            if (entitySelectable)
            {
                itemFlags |= Qt::ItemIsDragEnabled;
            }
            else
            {
                itemFlags &= ~(Qt::ItemIsSelectable | Qt::ItemIsEditable | Qt::ItemIsEnabled);
            }
        }

        return itemFlags;
    }

    Qt::DropActions OutlinerListModel::supportedDropActions() const
    {
        return (Qt::MoveAction | Qt::CopyAction);
    }

    Qt::DropActions OutlinerListModel::supportedDragActions() const
    {
        return (Qt::MoveAction | Qt::CopyAction);
    }


    bool OutlinerListModel::canDropMimeData(const QMimeData* data, Qt::DropAction, int, int, const QModelIndex& index) const
    {
        if (data)
        {
            if (index.isValid())
            {
                if (data->hasFormat(ComponentTypeMimeData::GetMimeType()) ||
                    data->hasFormat(ComponentAssetMimeDataContainer::GetMimeType()))
                {
                    return true;
                }
            }

            if (canDropMimeDataForEntityIds(data, index))
            {
                return true;
            }
        }

        return false;
    }

    bool OutlinerListModel::canDropMimeDataForEntityIds(const QMimeData* data, const QModelIndex& index) const
    {
        if (!data || !data->hasFormat(EditorEntityIdContainer::GetMimeType()))
        {
            return false;
        }

        QByteArray arrayData = data->data(EditorEntityIdContainer::GetMimeType());

        EditorEntityIdContainer entityIdListContainer;
        if (!entityIdListContainer.FromBuffer(arrayData.constData(), arrayData.size()))
        {
            return false;
        }

        AZ::EntityId dropTargetId = GetEntityAtIndex(index);
        if (!CanReparentEntities(dropTargetId, entityIdListContainer.m_entityIds))
        {
            return false;
        }

        return true;
    }

    // Expected behavior when dropping an entity in the outliner view model:
    //  DragEntity onto DropEntity, neither in each other's hierarchy:
    //      DropEntity becomes parent of DragEntity
    //  DragEntity onto DropEntity, DropEntity is DragEntity's parent:
    //      DragEntity sets its parent to DropEntity's parent.
    //  DragEntity onto DropEntity, DragEntity is DropEntity's parent:
    //      No change occurs.
    //  DragEntity onto DropEntity, DropEntity is DragEntity's grandfather (or deeper):
    //      DragEntity's parent becomes DropEntity.
    //  DragEntity onto DropEntity, DragEntity is DropEntity's grandfather (or deeper):
    //      No change occurs.
    //  DragEntity onto DragEntity
    //      No change occurs.
    //  DragEntity and DragEntityChild onto DropEntity:
    //      DragEntity behaves as define previously.
    //      DragEntityChild behaves as a second DragEntity, following normal drag rules.
    //      Example: DragEntity and DragEntityChild dropped onto DragEntity:
    //          DragEntity has no change occur.
    //          DragEntityChild follows the rule "DragEntity onto DropEntity, DropEntity is DragEntity's parent"
    bool OutlinerListModel::dropMimeData(const QMimeData* data, Qt::DropAction action, int row, int column, const QModelIndex& dropModelIndex)
    {
        if (action == Qt::IgnoreAction)
        {
            return true;
        }

        // Navigation triggered - Drag+Drop from Component Palette to Entity Outliner / Drag+Drop from File Browser to Entity Outliner
        EditorMetricsEventsBusAction editorMetricsEventsBusActionWrapper(EditorMetricsEventsBusTraits::NavigationTrigger::DragAndDrop);

        AZ::EntityId dropTargetId = GetEntityAtIndex(dropModelIndex);
        if (dropTargetId.IsValid())
        {
            // Handle drop from the component palette window.
            if (data->hasFormat(ComponentTypeMimeData::GetMimeType()))
            {
                return dropMimeDataComponentPalette(data, action, row, column, dropModelIndex, dropTargetId);
            }

            if (data->hasFormat(ComponentAssetMimeDataContainer::GetMimeType()))
            {
                return dropMimeDataComponentAssets(data, action, row, column, dropModelIndex, dropTargetId);
            }
        }

        if (data->hasFormat(EditorEntityIdContainer::GetMimeType()))
        {
            return dropMimeDataEntities(data, action, row, column, dropModelIndex, dropTargetId);
        }

        return QAbstractItemModel::dropMimeData(data, action, row, column, dropModelIndex);
    }

    bool OutlinerListModel::dropMimeDataComponentPalette(const QMimeData* data, Qt::DropAction action, int row, int column, const QModelIndex& dropModelIndex, const AZ::EntityId& dropTargetId)
    {
        EntityIdList entityIds{ dropTargetId };

        ComponentTypeMimeData::ClassDataContainer classDataContainer;
        ComponentTypeMimeData::Get(data, classDataContainer);

        AZ::ComponentTypeList componentsToAdd;
        for (auto classData : classDataContainer)
        {
            if (classData)
            {
                componentsToAdd.push_back(classData->m_typeId);
            }
        }

        EntityCompositionRequests::AddComponentsOutcome addedComponentsResult = AZ::Failure(AZStd::string("Failed to call AddComponentsToEntities on EntityCompositionRequestBus"));
        EntityCompositionRequestBus::BroadcastResult(addedComponentsResult, &EntityCompositionRequests::AddComponentsToEntities, entityIds, componentsToAdd);

        if (addedComponentsResult.IsSuccess())
        {
            for (auto componentsAddedToEntity : addedComponentsResult.GetValue())
            {
                auto entityId = componentsAddedToEntity.first;
                AZ_Assert(entityId == dropTargetId, "Only asked to add components to one entity, the id returned is not the right one");
                for (auto componentAddedToEntity : componentsAddedToEntity.second.m_componentsAdded)
                {
                    EditorMetricsEventsBus::Broadcast(&EditorMetricsEventsBusTraits::ComponentAdded, entityId, GetComponentTypeId(componentAddedToEntity));
                }
            }

            ToolsApplicationEvents::Bus::Broadcast(&ToolsApplicationEvents::InvalidatePropertyDisplay, Refresh_EntireTree_NewContent);
        }

        return QAbstractItemModel::dropMimeData(data, action, row, column, dropModelIndex);
    }

    bool OutlinerListModel::dropMimeDataComponentAssets(const QMimeData* data, Qt::DropAction /*action*/, int /*row*/, int /*column*/, const QModelIndex& /*dropModelIndex*/, const AZ::EntityId& dropTargetId)
    {
        ComponentAssetMimeDataContainer componentAssetContainer;
        if (componentAssetContainer.FromMimeData(data))    //  This returns false if the mime data doesn't contain the right type.
        {
            // Build up all the components we want to add
            AZ::ComponentTypeList componentsToAdd;
            for (auto asset : componentAssetContainer.m_assets)
            {
                AZ::Uuid componentType = asset.m_classId;

                // add type for asset with known component class
                if (!componentType.IsNull())
                {
                    componentsToAdd.push_back(componentType);
                }
            }

            // Add them all at once
            EntityCompositionRequests::AddComponentsOutcome addComponentsOutcome = AZ::Failure(AZStd::string(""));
            EntityCompositionRequestBus::BroadcastResult(addComponentsOutcome, &EntityCompositionRequests::AddComponentsToEntities, EntityIdList{ dropTargetId }, componentsToAdd);

            if (!addComponentsOutcome.IsSuccess())
            {
                return false;
            }

            // Patch up the assets for each component added
            auto& componentsAdded = addComponentsOutcome.GetValue()[dropTargetId].m_componentsAdded;
            size_t componentAddedIndex = 0;
            for (auto asset : componentAssetContainer.m_assets)
            {
                auto componentAdded = componentsAdded[componentAddedIndex];
                auto componentType = asset.m_classId;
                auto assetId = asset.m_assetId;

                AZ_Assert(componentType == GetComponentTypeId(componentAdded), "Type of component added does not match the type required by asset");

                // Add Component Metrics Event (Drag+Drop from File Browser to Entity Outliner)
                EBUS_EVENT(EditorMetricsEventsBus, ComponentAdded, dropTargetId, componentType);

                auto editorComponent = GetEditorComponent(componentAdded);
                if (editorComponent)
                {
                    editorComponent->SetPrimaryAsset(assetId);
                }

                ++componentAddedIndex;
            }

            EntityIdList selected;
            EBUS_EVENT_RESULT(selected, ToolsApplicationRequests::Bus, GetSelectedEntities);
            bool isSelected = (selected.end() != AZStd::find(selected.begin(), selected.end(), dropTargetId));
            if (isSelected)
            {
                EBUS_EVENT(ToolsApplicationEvents::Bus, InvalidatePropertyDisplay, Refresh_EntireTree_NewContent);
            }
            return true;
        }

        return false;
    }

    bool OutlinerListModel::dropMimeDataEntities(const QMimeData* data, Qt::DropAction action, int row, int column, const QModelIndex& dropModelIndex, const AZ::EntityId& dropTargetId)
    {
        // these aren't used; don't want a warning
        (void)dropModelIndex;
        (void)column;
        (void)row;
        (void)action;

        QByteArray arrayData = data->data(EditorEntityIdContainer::GetMimeType());

        EditorEntityIdContainer entityIdListContainer;
        if (!entityIdListContainer.FromBuffer(arrayData.constData(), arrayData.size()))
        {
            return false;
        }

        return ReparentEntities(dropTargetId, entityIdListContainer.m_entityIds);
    }

    bool OutlinerListModel::CanReparentEntities(const AZ::EntityId& newParentId, const AzToolsFramework::EntityIdList &selectedEntityIds) const
    {
        if (selectedEntityIds.empty())
        {
            return false;
        }

        // Ignore entities not owned by the editor context. It is assumed that all entities belong
        // to the same context since multiple selection doesn't span across views.
        for (const AZ::EntityId& entityId : selectedEntityIds)
        {
            bool isEditorEntity = false;
            EBUS_EVENT_RESULT(isEditorEntity, EditorEntityContextRequestBus, IsEditorEntity, entityId);
            if (!isEditorEntity)
            {
                return false;
            }
        }

        bool areEntitiesEditable = true;
        EBUS_EVENT_RESULT(areEntitiesEditable, ToolsApplicationRequests::Bus, AreEntitiesEditable, selectedEntityIds);
        if (!areEntitiesEditable)
        {
            return false;
        }

        //Only check the entity pointer if the entity id is valid because
        //we want to allow dragging items to unoccupied parts of the tree to un-parent them
        if (newParentId.IsValid())
        {
            AZ::Entity* newParentEntity = nullptr;
            AZ::ComponentApplicationBus::BroadcastResult(newParentEntity, &AZ::ComponentApplicationRequests::FindEntity, newParentId);
            if (!newParentEntity)
            {
                return false;
            }
        }

        //reject dragging on to yourself or your children
        AZ::EntityId currParentId = newParentId;
        while (currParentId.IsValid())
        {
            if (AZStd::find(selectedEntityIds.begin(), selectedEntityIds.end(), currParentId) != selectedEntityIds.end())
            {
                return false;
            }

            AZ::TransformBus::EventResult(currParentId, currParentId, &AZ::TransformBus::Events::GetParentId);
        }

        return true;
    }

    bool OutlinerListModel::ReparentEntities(const AZ::EntityId& newParentId, const AzToolsFramework::EntityIdList &selectedEntityIds)
    {
        if (!CanReparentEntities(newParentId, selectedEntityIds))
        {
            return false;
        }

        ScopedUndoBatch undo("Alter Hierarchy");

        for (const AZ::EntityId& entityId : selectedEntityIds)
        {
            AZ::EntityId oldParentId;
            EBUS_EVENT_ID_RESULT(oldParentId, entityId, AZ::TransformBus, GetParentId);

            //  Guarding this to prevent the entity from being marked dirty when the parent doesn't change.
            EBUS_EVENT_ID(entityId, AZ::TransformBus, SetParent, newParentId);

            // Allow for metrics collection
            EBUS_EVENT(EditorMetricsEventsBus, EntityParentChanged, entityId, newParentId, oldParentId);

            undo.MarkEntityDirty(entityId);
        }

        EBUS_EVENT(ToolsApplicationEvents::Bus, InvalidatePropertyDisplay, Refresh_Values);
        return true;
    }

    QMimeData* OutlinerListModel::mimeData(const QModelIndexList& indexes) const
    {
        AZ::Uuid uuid1 = AZ::AzTypeInfo<AZ::Entity>::Uuid();
        AZ::Uuid uuid2 = AZ::AzTypeInfo<EditorEntityIdContainer>::Uuid();

        EditorEntityIdContainer entityIdList;
        for (const QModelIndex& index : indexes)
        {
            if (index.column() == 0) // ignore all but one cell in row
            {
                AZ::EntityId entityId = GetEntityAtIndex(index);
                if (entityId.IsValid())
                {
                    entityIdList.m_entityIds.push_back(entityId);
                }
            }
        }
        if (entityIdList.m_entityIds.empty())
        {
            return nullptr;
        }

        AZStd::vector<char> encoded;
        if (!entityIdList.ToBuffer(encoded))
        {
            return nullptr;
        }

        QMimeData* mimeDataPtr = aznew QMimeData();
        QByteArray encodedData;
        encodedData.resize((int)encoded.size());
        memcpy(encodedData.data(), encoded.data(), encoded.size());

        mimeDataPtr->setData(EditorEntityIdContainer::GetMimeType(), encodedData);
        return mimeDataPtr;
    }

    QStringList OutlinerListModel::mimeTypes() const
    {
        QStringList list = QAbstractItemModel::mimeTypes();
        list.append(EditorEntityIdContainer::GetMimeType());
        list.append(ComponentTypeMimeData::GetMimeType());
        list.append(ComponentAssetMimeDataContainer::GetMimeType());
        return list;
    }

    void OutlinerListModel::EntityDeregistered(AZ::EntityId entityId)
    {
        // If this entity is pending registration, it has been created and destroyed in
        // a single frame. We need to remove it from the registration buffer and
        // ignore this deregistration request.
        auto registrationIter = m_entityRegistrationBuffer.find(entityId);
        if (registrationIter != m_entityRegistrationBuffer.end())
        {
            m_entityRegistrationBuffer.erase(registrationIter);
            return;
        }

        // Ignore entities that don't have a corresponding cache in the outliner
        auto entityIter = m_entityToCacheMap.find(entityId);
        if (entityIter == m_entityToCacheMap.end())
        {
            return;
        }

        m_entityDeregistrationBuffer.insert(entityId);

        RequestEventBufferProcessing();
    }

    void OutlinerListModel::ProcessDeregistrationBuffer()
    {
        AZ_PROFILE_FUNCTION(AZ::Debug::ProfileCategory::AzToolsFramework);

        for (const auto& entityId : m_entityDeregistrationBuffer)
        {
            auto iter = m_entityToCacheMap.find(entityId);
            AZ_Assert(iter != m_entityToCacheMap.end(), "Trying to deregister an entity with no cach entry.");
            OutlinerCache* entityCache = iter->second;

            // Store the previous expansion state in case we need to restore this entity later
            // or it's being de-registered as part of a slice operation.
            m_removedExpansionState.insert(AZStd::make_pair(entityId, entityCache->IsExpanded()));

            // m_entitiesCacheRoot adopts all of the entity's children
            if (entityCache->GetChildCount() > 0)
            {
                BeginMoveRows(GetIndexFromCache(entityCache),
                    0,
                    entityCache->GetChildCount() - 1,
                    GetIndexFromCache(m_entitiesCacheRoot),
                    m_entitiesCacheRoot->GetChildCount()); // new children go to back of list

                MarkChildrenAsOrphans(entityCache);

                m_entitiesCacheRoot->AdoptChildren(entityCache);
                EndMoveRows();
            }

            OutlinerCache* parentCache = entityCache->GetParent();
            if (parentCache == nullptr)
            {
                parentCache = m_entitiesCacheRoot;
            }

            // The root node is a special case, and is the invalid model index.
            QModelIndex parentIndex = GetIndexFromCache(parentCache);

            // Remove entity
            const int entityRow = entityCache->GetRow();
            BeginRemoveRows(parentIndex, entityRow, entityRow);
            parentCache->RemoveChild(entityCache);
            m_entityToCacheMap.erase(entityId);
            EndRemoveRows();

            delete entityCache;
        }

        m_entityDeregistrationBuffer.clear();

        InvalidateFilter();
    }

    void OutlinerListModel::EntityRegistered(AZ::EntityId entityId)
    {
        // If we already have a cache entry for this entity Id, verify that there
        // is already a queued deregistration event. If there isn't, something went wrong.
        if (m_entityToCacheMap.find(entityId) != m_entityToCacheMap.end())
        {
            if (m_entityDeregistrationBuffer.find(entityId) == m_entityDeregistrationBuffer.end())
            {
                AZ_Assert(false, "Outliner received an entity registration event for an existing entity ID.");
                return;
            }
        }

        // Ignore entities not owned by the editor context.
        bool isEditorEntity = false;
        EBUS_EVENT_RESULT(isEditorEntity, EditorEntityContextRequestBus, IsEditorEntity, entityId);
        if (!isEditorEntity)
        {
            return;
        }

        m_entityRegistrationBuffer.insert(entityId);

        RequestEventBufferProcessing();
    }

    void OutlinerListModel::ProcessRegistrationBuffer()
    {
        AZ_PROFILE_FUNCTION(AZ::Debug::ProfileCategory::AzToolsFramework);

        // Abort if no entities to register (could happen on rapid creation/removal)
        if (m_entityRegistrationBuffer.empty())
        {
            return;
        }

        // Insert entity as child of root
        // If the entity's Transform is parented to another entity
        // we'll get an EntityParentChanged message about it later.
        int newRowLocation = m_entitiesCacheRoot->GetChildCount();
        BeginInsertRows(QModelIndex(), newRowLocation, newRowLocation + int(m_entityRegistrationBuffer.size() - 1));

        for (const auto& entityId : m_entityRegistrationBuffer)
        {
            OutlinerCache* newEntity = aznew OutlinerCache(entityId, this, m_entitiesCacheRoot);
            m_entityToCacheMap[entityId] = newEntity;
            m_entitiesCacheRoot->AppendChild(newEntity);

            // Now make sure this entity ends up at the proper place in the hierarchy
            FindFamily(entityId);

            // If this entity has been seen before and it's expansion state has been stored
            // restore it.
            auto expandStateIter = m_removedExpansionState.find(entityId);
            // Restore the previous expanded state of the entity if it exists
            if (expandStateIter != m_removedExpansionState.end())
            {
                newEntity->SetExpanded((*expandStateIter).second);
                if (newEntity->IsExpanded())
                {
                    ExpandItemHierarchy(entityId);
                }

                m_removedExpansionState.erase(expandStateIter);
            }
            else
            {
                // Until we maintain the outliner hierarchy collapse state, collapse everything when it's created for now...
                newEntity->SetExpanded(false);
            }
        }
        EndInsertRows();

        // Now go through all the newly created entities and process all the slices associated with them
        for (const auto& entityId : m_entityRegistrationBuffer)
        {
            // We only care about cache entries that don't have slice information yet.
            if (!GetCacheForEntity(entityId)->IsSliceEntity())
            {
                AZ::SliceComponent::SliceInstanceAddress sliceAddress;
                AzFramework::EntityIdContextQueryBus::EventResult(sliceAddress, entityId, &AzFramework::EntityIdContextQueries::GetOwningSlice);

                // If the entity is part of a slice...
                if (sliceAddress.first && sliceAddress.second)
                {
                    // Process the discovered slice.
                    MarkUpSliceEntities(sliceAddress.first->GetSliceAsset().GetId(), sliceAddress);
                }
            }
        }

        m_entityRegistrationBuffer.clear();

        InvalidateFilter();
    }

    void OutlinerListModel::RegisterExistingEntities()
    {
        AZ_PROFILE_FUNCTION(AZ::Debug::ProfileCategory::AzToolsFramework);

        // If we don't have a cache entry for an entity, create one now.
        EBUS_EVENT(AZ::ComponentApplicationBus, EnumerateEntities,
            [this](AZ::Entity* entity)
        {
            if (nullptr == GetCacheForEntity(entity->GetId()))
            {
                EntityRegistered(entity->GetId());
            }
        });
    }

    void OutlinerListModel::BeginInsertRows(const QModelIndex& parent, int first, int last)
    {
        beginInsertRows(parent, first, last);
        m_treeModificationUnderway = true;
    }

    void OutlinerListModel::EndInsertRows()
    {
        endInsertRows();
        m_treeModificationUnderway = false;
        ProcessRequestBuffers();
    }

    void OutlinerListModel::BeginMoveRows(const QModelIndex &sourceParent, int sourceFirst, int sourceLast, const QModelIndex &destinationParent, int destinationChild)
    {
        beginMoveRows(sourceParent, sourceFirst, sourceLast, destinationParent, destinationChild);
        m_treeModificationUnderway = true;
    }

    void OutlinerListModel::EndMoveRows()
    {
        endMoveRows();
        m_treeModificationUnderway = false;
        ProcessRequestBuffers();
    }

    void OutlinerListModel::BeginRemoveRows(const QModelIndex &parent, int first, int last)
    {
        beginRemoveRows(parent, first, last);
        m_treeModificationUnderway = true;
    }

    void OutlinerListModel::EndRemoveRows()
    {
        endRemoveRows();
        m_treeModificationUnderway = false;
        ProcessRequestBuffers();
    }


    QModelIndex OutlinerListModel::GetIndexForEntity(const AZ::EntityId& entityId) const
    {
        if (!entityId.IsValid())
        {
            return QModelIndex();
        }

        auto entityIter = m_entityToCacheMap.find(entityId);
        if (entityIter == m_entityToCacheMap.end())
        {
            return QModelIndex();
        }

        return GetIndexFromCache(entityIter->second);
    }

    OutlinerCache* OutlinerListModel::GetCacheForEntity(const AZ::EntityId& entityId)
    {
        auto entityIter = m_entityToCacheMap.find(entityId);
        return entityIter != m_entityToCacheMap.end() ? entityIter->second : nullptr;
    }

    const OutlinerCache* OutlinerListModel::GetCacheForEntity(const AZ::EntityId& entityId) const
    {
        auto entityIter = m_entityToCacheMap.find(entityId);
        return entityIter != m_entityToCacheMap.end() ? entityIter->second : nullptr;
    }

    // Takes the ID of a newly created entity
    // This function will find its parent and any existing children and make the appropriate parent-child links between them.
    // If its parent doesn't exist yet, the relationship is stored for later.
    void OutlinerListModel::FindFamily(const AZ::EntityId& entityId)
    {
        // Find out if the entity has a parent...
        AZ::EntityId parentId;
        AZ::TransformBus::EventResult(parentId, entityId, &AZ::TransformBus::Events::GetParentId);

        if (parentId.IsValid())
        {
            // Now determine if the entity's parent exists yet
            auto parentCacheIter = m_entityToCacheMap.find(parentId);
            if (parentCacheIter != m_entityToCacheMap.end())
            {
                // The entity's parent has changed.
                EntityParentChanged(entityId, parentId);
            }
            else
            {
                RegisterOrphan(entityId, parentId);
            }
        }

        // Now find out if this entity has any orphaned children
        auto absentParentIter = m_absentParentMap.find(entityId);
        if (absentParentIter != m_absentParentMap.end())
        {
            // If we do have orphaned children, reclaim them.
            for (const auto& childId : absentParentIter->second)
            {
                EntityParentChanged(childId, entityId);
            }

            m_absentParentMap.erase(absentParentIter);
        }
    }

    void OutlinerListModel::MarkChildrenAsOrphans(OutlinerCache* entityCache)
    {
        AZ::EntityId entityId = entityCache->GetData();
        int childCount = entityCache->GetChildCount();

        for (int i = 0; i < childCount; ++i)
        {
            const OutlinerCache* childCache = entityCache->GetChild(i);
            RegisterOrphan(childCache->GetData(), entityId);
        }
    }

    void OutlinerListModel::RegisterOrphan(const AZ::EntityId& entityId, const AZ::EntityId& parentId)
    {
        m_absentParentMap[parentId].push_back(entityId);
    }

    void OutlinerListModel::EntityParentChanged(AZ::EntityId entityId, AZ::EntityId newParentId, AZ::EntityId /*oldParent*/)
    {
        OutlinerCache* entityCache = GetCacheForEntity(entityId); // might be null

        // Ignore entities with no outliner entry
        if (!entityCache)
        {
            return;
        }

        // We don't need any information about the old parent because we've already got it in the cache.
        m_entityParentChangeBuffer.insert(AZStd::make_pair(entityId, newParentId));

        RequestEventBufferProcessing();
    }

    void OutlinerListModel::EntityCreatedAsChild(AZ::EntityId /*entityId*/, AZ::EntityId parentId)
    {
        // When the user explicitly creates an entity as a child of another entity, expand the parent to display the child in the outliner.
        auto parentCache = GetCacheForEntity(parentId);
        AZ_Assert(parentCache, "Received child creation event for an entity that doesn't have a cache in the outliner");
        if (parentCache && !parentCache->IsExpanded())
        {
            parentCache->SetExpanded(true);
            ExpandItemHierarchy(parentId);
        }
    }

    void OutlinerListModel::ProcessReparentingBuffer()
    {
        AZ_PROFILE_FUNCTION(AZ::Debug::ProfileCategory::AzToolsFramework);

        emit layoutAboutToBeChanged();
        bool wasBlockingSignals = blockSignals(true);

        for (auto childParentPair : m_entityParentChangeBuffer)
        {
            OutlinerCache* entityCache = GetCacheForEntity(childParentPair.first);

            if (entityCache)
            {
                OutlinerCache* previousParentCache = entityCache->GetParent(); // might be root
                OutlinerCache* newParentCache = GetCacheForEntity(childParentPair.second); // might be null

                if (previousParentCache == nullptr)
                {
                    previousParentCache = m_entitiesCacheRoot;
                }
                if (newParentCache == nullptr)
                {
                    newParentCache = m_entitiesCacheRoot;
                }
                if (previousParentCache != newParentCache)
                {
                    QModelIndex previousParentIndex = GetIndexFromCache(previousParentCache);
                    QModelIndex newParentIndex = GetIndexFromCache(newParentCache);
                    const int previousEntityRow = entityCache->GetRow();
                    const int newEntityRow = newParentCache->GetChildCount(); // will go to end of parent's child list

                    BeginMoveRows(previousParentIndex, previousEntityRow, previousEntityRow, newParentIndex, newEntityRow);
                    previousParentCache->RemoveChild(entityCache);
                    newParentCache->AppendChild(entityCache);
                    EndMoveRows();
                }
            }
        }

        blockSignals(wasBlockingSignals);
        emit layoutChanged();

        m_entityParentChangeBuffer.clear();

        // Depending on how we want to handle entities with parents that don't exist in the future, we
        // may want to keep the absent parent map around and keep it 
        m_absentParentMap.clear();

        InvalidateFilter();
    }

    void OutlinerListModel::RequestEventBufferProcessing()
    {
        if (!m_bufferProcessingPending)
        {
            // Tell QT to execute our event buffer processing function during it's next event loop.
            QTimer::singleShot(0, this, SLOT(ProcessEventBuffers()));
            m_bufferProcessingPending = true;
        }
    }

    void OutlinerListModel::ProcessEventBuffers()
    {
        AZ_PROFILE_FUNCTION(AZ::Debug::ProfileCategory::AzToolsFramework);

        if (!m_entityDeregistrationBuffer.empty())
        {
            ProcessDeregistrationBuffer();
        }

        if (!m_entityRegistrationBuffer.empty())
        {
            ProcessRegistrationBuffer();
        }

        if (!m_entityParentChangeBuffer.empty())
        {
            ProcessReparentingBuffer();
        }

        if (!m_selectionRequestBuffer.empty() || !m_deselectionRequestBuffer.empty())
        {
            ProcessSelectionChangeBuffers();
        }

        m_bufferProcessingPending = false;
    }

    void OutlinerListModel::MarkUpSliceEntities(const AZ::Data::AssetId& sliceAssetId, const AZ::SliceComponent::SliceInstanceAddress& sliceAddress)
    {
        AZ_PROFILE_FUNCTION(AZ::Debug::ProfileCategory::AzToolsFramework);

        // If we can get the prefab asset, we can get the slice component.
        auto asset = AZ::Data::AssetManager::Instance().FindAsset(sliceAssetId);

        AZStd::string sliceAssetPath;
        AZ::Data::AssetCatalogRequestBus::BroadcastResult(sliceAssetPath, &AZ::Data::AssetCatalogRequests::GetAssetPathById, sliceAssetId);

        AZ_Assert(asset.IsReady(), "Asset must have been loaded for the slice to be instantiated");
        // From the slice component, we can get all the entities that are part of the slice.
        AZ::SliceComponent* sliceComponent = asset.GetAs<AZ::SliceAsset>()->GetComponent();

        auto sliceEntityMap = sliceAddress.second->GetEntityIdMap();

        // Now that we have all the entities that belong to this slice, we can mark them all as belonging to a slice
        for (const auto& entityIdPair : sliceEntityMap)
        {
            auto cache = GetCacheForEntity(entityIdPair.second);

            if (cache)
            {
                AZStd::string prefabPath;
                AZ::Data::AssetCatalogRequestBus::BroadcastResult(prefabPath, &AZ::Data::AssetCatalogRequests::GetAssetPathById, sliceAssetId);
                cache->SetSliceInformation(QString(prefabPath.c_str()));
            }
        }

        // Build the information we need to insert slice handles into the hierarchy
        // A list of slice handle hierarchy information to fill out

        // Find the root of every slice referenced in the current prefab.
        // We pass in the EntityIdMap of the instantiated prefab so that the discovered roots
        // get mapped back to the live entities of our prefab instance.
        FindAllRootEntities(*sliceComponent, sliceAddress, m_SliceHandles, sliceAddress.second->GetEntityIdMap());

        auto newSliceHandleSetIter = m_SliceHandles.find(sliceAddress.second->GetId());
        if (newSliceHandleSetIter != m_SliceHandles.end())
        {
            const auto& newSliceHandleSet = newSliceHandleSetIter->second;

            // Process our new slice root structure.
            // We now have all the roots for each prefab.
            // We need to find the common parent of each root.

            // Now that we have all the entities that belong to this slice, we can mark them all as belonging to a slice
            for (const auto& sliceHandle : newSliceHandleSet)
            {
                for (const auto& rootId : sliceHandle.GetRootEntities())
                {
                    auto cache = GetCacheForEntity(rootId);

                    if (cache)
                    {
                        AZStd::string entityName;
                        AZ::ComponentApplicationBus::BroadcastResult(entityName, &AZ::ComponentApplicationRequests::GetEntityName, rootId);
                        AZStd::string prefabPath;
                        AZ::Data::AssetCatalogRequestBus::BroadcastResult(prefabPath, &AZ::Data::AssetCatalogRequests::GetAssetPathById, sliceAssetId);
                        cache->SetAsSliceRoot(true);
                    }
                    else
                    {
                        AZStd::string entityName;
                        AZ::ComponentApplicationBus::BroadcastResult(entityName, &AZ::ComponentApplicationRequests::GetEntityName, rootId);
                    }
                }
            }
        }
    }

    // This will analyze the entire dependency hierarchy of nested prefabs that make up the given prefab and find the
    // transform hierarchy root(s) of each one. The final result is a list of all of the asset entity IDs belonging to
    // the given prefab which are root entities of the given prefab or inherited from the root entities of referenced prefabs.
    // The final argument provides a mapping from entity IDs discovered here to those Entity IDs that the calling function is concerned about. No mapping takes place if the provided map is empty.
    void OutlinerListModel::FindAllRootEntities(AZ::SliceComponent& sliceComponent, const AZ::SliceComponent::SliceInstanceAddress& sliceInstance, SliceHandleMap& roots, const AZ::SliceComponent::EntityIdToEntityIdMap& parentEntityMap)
    {
        AZ_PROFILE_FUNCTION(AZ::Debug::ProfileCategory::AzToolsFramework);

        AZ::SliceComponent::EntityList entities;
        if (sliceComponent.GetEntities(entities))
        {
            bool wasCommonRootFound = false;

            AZ::EntityId commonRootEntityId;

            // A list of top-level entities just in case there are multiple roots.
            EntityList topLevelEntities;

            ToolsApplicationRequests::Bus::BroadcastResult(wasCommonRootFound, &ToolsApplicationRequests::FindCommonRootInactive, entities, commonRootEntityId, &topLevelEntities);


            AZStd::string sliceAssetPath;
            AZ::Data::AssetCatalogRequestBus::BroadcastResult(sliceAssetPath, &AZ::Data::AssetCatalogRequests::GetAssetPathById, sliceComponent.GetMyAsset()->GetId());

            if (!topLevelEntities.empty())
            {
                // There can be a multiple slice handles associate with a single instance. See if we have a container to store them and use that if we do.
                auto instanceIter = roots.find(sliceInstance.second->GetId());
                if (instanceIter == roots.end())
                {
                    // If we don't have a container, create a new one.
                    auto emplaceResult = roots.emplace(AZStd::make_pair(sliceInstance.second->GetId(), SliceHandleList()));

                    AZ_Assert(emplaceResult.second, "Slice Handle Instance Container Creation Failed for slice asset: %s", sliceAssetPath);
                    instanceIter = emplaceResult.first;
                }

                // Add a new slice handle to the instance list
                instanceIter->second.emplace_back(OutlinerSliceHandle(sliceAssetPath, sliceInstance));
                auto& newHandle = instanceIter->second.back();

                for (auto const& entity : topLevelEntities)
                {
                    if (!parentEntityMap.empty())
                    {
                        AZ_Assert(parentEntityMap.find(entity->GetId()) != parentEntityMap.end(), "Outliner Root Detection: The Provided Entity Map does not contain a mapping for entity: %s", entity->GetName().c_str());
                        newHandle.AddRootEntity(parentEntityMap.at(entity->GetId()));
                    }
                    else
                    {
                        newHandle.AddRootEntity(entity->GetId());
                    }
                }
            }
        }

        // Recurse down the hierarchy.
        for (const auto& prefab : sliceComponent.GetSlices())
        {
            // Start by building a new map of parent entities so the deeper
            // base prefab instances know which root-level entities theirs
            // map to for the final list
            for (const auto& instance : prefab.GetInstances())
            {
                AZ::SliceComponent::EntityIdToEntityIdMap nextLevelIDMapping;

                for (const auto& idPair : instance.GetEntityIdMap())
                {
                    if (!parentEntityMap.empty())
                    {
                        if (parentEntityMap.find(idPair.second) != parentEntityMap.end())
                        {
                            nextLevelIDMapping.emplace(AZStd::make_pair(idPair.first, parentEntityMap.at(idPair.second)));
                        }
                    }
                    else
                    {
                        nextLevelIDMapping.emplace(idPair);
                    }
                }

                // Pass the corrected map to the next level of the slice hierarchy.
                FindAllRootEntities(*(prefab.GetSliceAsset().Get()->GetComponent()), sliceInstance, roots, nextLevelIDMapping);
            }
        }
    }

    void OutlinerListModel::ExpandItemHierarchy(const AZ::EntityId& entityId)
    {
        m_expansionRequestBuffer.insert(entityId);

        // If we're not currently modifying the tree, process this request immediately
        if (!m_treeModificationUnderway)
        {
            ProcessRequestBuffers();
        }
    }

    void OutlinerListModel::EntityCacheChanged(const AZ::EntityId& entityId)
    {
        m_invalidationRequestBuffer.insert(entityId);

        // If we're not currently modifying the tree, process this request immediately
        if (!m_treeModificationUnderway)
        {
            ProcessRequestBuffers();
        }
    }
    void OutlinerListModel::EntityCacheSelectionRequest(const AZ::EntityId&  entityId)
    {
        // Remove any previously queued up deselection requests since the last one in should always win
        m_deselectionRequestBuffer.erase(entityId);

        m_selectionRequestBuffer.insert(entityId);

        RequestEventBufferProcessing();
    }

    void OutlinerListModel::EntityCacheDeselectionRequest(const AZ::EntityId&  entityId)
    {
        // Remove any previously queued up selection requests since the last one in should always win
        m_selectionRequestBuffer.erase(entityId);

        m_deselectionRequestBuffer.insert(entityId);

        RequestEventBufferProcessing();
    }

    void OutlinerListModel::ProcessSelectionChangeBuffers()
    {
        AZ_PROFILE_FUNCTION(AZ::Debug::ProfileCategory::AzToolsFramework);

        OutlinerModelNotificationBus::Broadcast(&OutlinerModelNotifications::ModelEntitySelectionChanged, m_selectionRequestBuffer, m_deselectionRequestBuffer);

        m_selectionRequestBuffer.clear();
        m_deselectionRequestBuffer.clear();

        InvalidateFilter();
    }

    // Processes and clears the invalidation buffer.
    // Call at the end of any function that could invalidate items in the tree view.
    void OutlinerListModel::ProcessRequestBuffers()
    {
        AZ_Assert(m_treeModificationUnderway == false, "Invalidatons must be unblocked to process the invalidation buffer.");

        for (const auto& entityId : m_expansionRequestBuffer)
        {
            auto cache = GetCacheForEntity(entityId);
            emit ExpandItem(GetIndexFromCache(cache));
        }

        m_expansionRequestBuffer.clear();

        for (const auto& entityId : m_invalidationRequestBuffer)
        {
            auto modelIndex = GetIndexForEntity(entityId);
            AZ_Assert(modelIndex.isValid(), "Received a cache changed event for an entity with no corresponding cache");
            Q_EMIT dataChanged(modelIndex, modelIndex);
        }

        m_invalidationRequestBuffer.clear();
    }

    void OutlinerListModel::OnEditorEntitiesReplacedBySlicedEntities(const AZStd::unordered_map<AZ::EntityId, AZ::EntityId>& replacedEntitiesMap)
    {
        AZ_PROFILE_FUNCTION(AZ::Debug::ProfileCategory::AzToolsFramework);

        for (const auto& replacedPair : replacedEntitiesMap)
        {
            auto newItemCache = GetCacheForEntity(replacedPair.second);
            auto expandStateIter = m_removedExpansionState.find(replacedPair.first);
            if (expandStateIter != m_removedExpansionState.end())
            {
                newItemCache->SetExpanded((*expandStateIter).second);
                if (newItemCache->IsExpanded())
                {
                    ExpandItemHierarchy(replacedPair.second);
                }

                m_removedExpansionState.erase(expandStateIter);
            }
        }
    }


    void OutlinerListModel::OnEditorEntitiesSliceOwnershipChanged(const AzToolsFramework::EntityIdList& entityIdList)
    {
        AZ_PROFILE_FUNCTION(AZ::Debug::ProfileCategory::AzToolsFramework);

        if (entityIdList.empty())
        {
            return;
        }

        AZ::SliceComponent::SliceInstanceAddress sliceAddress;
        AzFramework::EntityIdContextQueryBus::EventResult(sliceAddress, entityIdList.front(), &AzFramework::EntityIdContextQueries::GetOwningSlice);

        // Remove the prefab information for all the given entities
        for (const auto& entityId : entityIdList)
        {
            OutlinerCache* entityCache = GetCacheForEntity(entityId);
            if (entityCache)
            {
                entityCache->ClearSliceInformation();
            }
        }

        if (sliceAddress.first && sliceAddress.second)
        {
            // This will run through the slice and flag all the appropriate entities as part of a slice and as a slice root as appropriate.
            // This includes the entities here.
            OnSliceInstantiated(sliceAddress.first->GetSliceAsset().GetId(), sliceAddress, AzFramework::SliceInstantiationTicket());
        }
    }

    QModelIndexList OutlinerListModel::GetEntityIndexList(const AZStd::vector<AZ::EntityId>& entities)
    {
        QModelIndexList list;
        for (size_t searchIndex = 0; searchIndex < entities.size(); searchIndex++)
        {
            const OutlinerCache* cache = GetCacheForEntity(entities[searchIndex]);
            if (cache != nullptr)
            {
                // select all columns in row
                for (int column = 0; column < ColumnCount; ++column)
                {
                    list.append(GetIndexFromCache(cache, column));
                }
            }
        }
        return list;
    }

    QString OutlinerListModel::GetSliceAssetName(const AZ::EntityId& entityId) const
    {
        const OutlinerCache* cache = GetCacheForEntity(entityId);
        if (cache)
        {
            return cache->GetSliceAssetName();
        }
        else
        {
            return QString();
        }
    }

    OutlinerCache* OutlinerListModel::GetValidCacheFromIndex(const QModelIndex& modelIndex) const
    {
        if (!modelIndex.isValid())
        {
            return m_entitiesCacheRoot;
        }
        else
        {
            // QModelIndex contains pointer to cache
            return static_cast<OutlinerCache*>(modelIndex.internalPointer());
        }
    }

    QModelIndex OutlinerListModel::GetIndexFromCache(const OutlinerCache* cache, int column) const
    {
        // The root is a special case.
        // This seems to be standard practice in QTree tutorials, to create a root that
        // doesn't actually show up in the tree.
        if (cache == nullptr || cache == m_entitiesCacheRoot)
        {
            return QModelIndex();
        }

        // QModelIndex contains pointer to cache
        return createIndex(cache->GetRow(), column, const_cast<void*>(static_cast<const void*>(cache)));
    }

    void OutlinerListModel::SearchCriteriaChanged(QStringList& criteriaList, FilterOperatorType filterOperator)
    {
        m_filterOperator = filterOperator;
        BuildFilter(criteriaList, m_filterOperator);
        InvalidateFilter();
    }

    void OutlinerListModel::OnItemExpanded(const QModelIndex& index)
    {
        auto itemCache = GetValidCacheFromIndex(index);
        itemCache->SetExpanded(true);
    }

    void OutlinerListModel::OnItemCollapsed(const QModelIndex& index)
    {
        auto itemCache = GetValidCacheFromIndex(index);
        itemCache->SetExpanded(false);
    }

    void OutlinerListModel::BuildFilter(QStringList& criteriaList, FilterOperatorType filterOperator)
    {
        ClearFilterRegExp();

        if (criteriaList.size() > 0)
        {
            QString filter, tag, text;
            for (int i = 0; i < criteriaList.size(); i++)
            {
                SearchCriteriaButton::SplitTagAndText(criteriaList[i], tag, text);
                if (tag.isEmpty())
                {
                    tag = "null";
                }

                filter = m_filtersRegExp[tag.toStdString().c_str()].pattern();

                if (filterOperator == FilterOperatorType::Or)
                {
                    if (filter.isEmpty())
                    {
                        filter = text;
                    }
                    else
                    {
                        filter += "|" + text;
                    }
                }
                else if (filterOperator == FilterOperatorType::And)
                {
                    filter += "(?=.*" + text + ")"; //  Using Lookaheads to produce an "and" effect.
                }

                SetFilterRegExp(tag.toStdString().c_str(), QRegExp(filter, Qt::CaseInsensitive));
            }
        }
    }

    void OutlinerListModel::SetFilterRegExp(const AZStd::string& filterType, const QRegExp& regExp)
    {
        m_filtersRegExp[filterType] = regExp;
    }

    void OutlinerListModel::ClearFilterRegExp(const AZStd::string& filterType)
    {
        if (filterType.empty())
        {
            for (auto& it : m_filtersRegExp)
            {
                it.second = QRegExp();
            }
        }
        else
        {
            m_filtersRegExp[filterType] = QRegExp();
        }
    }

    void OutlinerListModel::InvalidateFilter()
    {
        m_entitiesCacheRoot->CalculateVisibility(m_filtersRegExp, m_filterOperator);
    }

    //! Editor lock component interface to enable/disable selection capabilities in the viewport.
    void OutlinerListModel::ToggleEditorLockState(const QModelIndex& index)
    {
        bool locked = false;
        AzToolsFramework::EditorLockComponentRequestBus::EventResult(locked, GetEntityAtIndex(index), &AzToolsFramework::EditorLockComponentRequests::GetLocked);
        SetEditorLockState(index, !locked);
    }

    void OutlinerListModel::SetEditorLockState(const QModelIndex& index, bool locked)
    {
        auto id = GetEntityAtIndex(index);
        AzToolsFramework::EditorLockComponentRequestBus::Event(id, &AzToolsFramework::EditorLockComponentRequests::SetLocked, locked);
        SetEditorLockStateOnAllChildren(index, locked);
    }

    void OutlinerListModel::SetEditorLockStateOnAllChildren(const QModelIndex& index, bool locked)
    {
        OutlinerCache* item = static_cast<OutlinerCache*>(index.internalPointer());
        for (int childIndex = 0; childIndex < item->GetChildCount(); ++childIndex)
        {
            SetEditorLockState(GetIndexFromCache(item->GetChild(childIndex)), locked);
        }
    }

    //! Editor Visibility interface to enable/disable rendering in the viewport.
    void OutlinerListModel::ToggleEditorVisibility(const QModelIndex& index)
    {
        bool visibleInEditor = false;
        AzToolsFramework::EditorVisibilityRequestBus::EventResult(visibleInEditor, GetEntityAtIndex(index), &AzToolsFramework::EditorVisibilityRequests::GetVisibilityFlag);
        SetEditorVisibility(index, !visibleInEditor);
    }

    void OutlinerListModel::SetEditorVisibility(const QModelIndex& index, bool visibleInEditor)
    {
        auto id = GetEntityAtIndex(index);
        AzToolsFramework::EditorVisibilityRequestBus::Event(id, &AzToolsFramework::EditorVisibilityRequests::SetVisibilityFlag, visibleInEditor);
        SetEditorVisibilityOnAllChildren(index, visibleInEditor);
    }

    void OutlinerListModel::SetEditorVisibilityOnAllChildren(const QModelIndex& index, bool visibleInEditor)
    {
        OutlinerCache* item = static_cast<OutlinerCache*>(index.internalPointer());
        for (int childIndex = 0; childIndex < item->GetChildCount(); ++childIndex)
        {
            SetEditorVisibility(GetIndexFromCache(item->GetChild(childIndex)), visibleInEditor);
        }
    }

    ////////////////////////////////////////////////////////////////////////////
    // OutlinerItemDelegate
    ////////////////////////////////////////////////////////////////////////////

    OutlinerItemDelegate::OutlinerItemDelegate(QWidget* parent)
        : QStyledItemDelegate(parent)
    {
        // Customizing the rendering of check boxes requires style sheets.
        // Load them here to apply the custom style to a pair of check boxes
        // used for rendering later.
        QString eyeToggleStyleSheet;
        QString lockToggleStyleSheet;

        QFile visibileStyleFile(VISIBILITY_CHECK_BOX_STYLE_SHEET_PATH);
        if (visibileStyleFile.open(QFile::ReadOnly))
        {
            eyeToggleStyleSheet = QLatin1String(visibileStyleFile.readAll());

            m_visibilityCheckBox.setStyleSheet(eyeToggleStyleSheet);
        }
        QFile lockStyleFile(LOCK_CHECK_BOX_STYLE_SHEET_PATH);
        if (lockStyleFile.open(QFile::ReadOnly))
        {
            lockToggleStyleSheet = QLatin1String(lockStyleFile.readAll());

            m_lockCheckBox.setStyleSheet(lockToggleStyleSheet);
        }
    }

    void OutlinerItemDelegate::paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const
    {
        // We're only using these check boxes as renderers so their actual state doesn't matter.
        // We can set it right before we draw using information from the model data.
        if (index.column() == OutlinerListModel::ColumnVisibilityToggle)
        {
            auto checkstate = index.data(Qt::CheckStateRole);

            if (checkstate.canConvert<Qt::CheckState>())
            {
                m_visibilityCheckBox.setChecked(checkstate.value<Qt::CheckState>() == Qt::Checked);
            }

            painter->save();
            painter->translate(option.rect.topLeft());
            m_visibilityCheckBox.render(painter);
            painter->restore();

            return; // painted, maybe
        }

        if (index.column() == OutlinerListModel::ColumnLockToggle)
        {
            QVariant checkstate = index.data(Qt::CheckStateRole);
            if (checkstate.canConvert<Qt::CheckState>())
            {
                m_lockCheckBox.setChecked(checkstate.value<Qt::CheckState>() == Qt::Checked);
            }

            painter->save();
            painter->translate(option.rect.topLeft());
            m_lockCheckBox.render(painter);
            painter->restore();
            return; // painted
        }

        QStyleOptionViewItem customOptions{ option };
        if (customOptions.state & QStyle::State_HasFocus)
        {
            //  Do not draw the focus rectangle in this column.
            customOptions.state ^= QStyle::State_HasFocus;
        }

        // Draw this Slice Handle Accent if the item is not selected before the
        // entry is drawn.
        if (!(option.state & QStyle::State_Selected))
        {
            if (index.data(OutlinerListModel::SliceBackgroundRole).value<bool>())
            {
                auto boxRect = option.rect;

                boxRect.setX(boxRect.x() + 0.5);
                boxRect.setY(boxRect.y() + 3.5);
                boxRect.setWidth(boxRect.width() - 1.0);
                boxRect.setHeight(boxRect.height() - 4.0);

                // These colors need to be moved into a style sheet in the future.
                // The new editor design will likely include style sheet colors for
                // slices and relevant selection of such.
                QColor background(30, 37, 47);
                QColor line(30, 50, 80);

                painter->save();
                painter->setRenderHint(QPainter::Antialiasing);
                QPainterPath path;
                path.addRoundedRect(boxRect, 6, 6);
                QPen pen(line, 1);
                painter->setPen(pen);
                painter->fillPath(path, background);
                painter->drawPath(path);
                painter->restore();
            }
            // Draw a dashed line around any visible, collapsed entry in the outnliner that has
            // children underneath it currently selected.
            if (!index.data(OutlinerListModel::ExpandedRole).template value<bool>())
            {
                if (index.data(OutlinerListModel::ChildSelectedRole).template value<bool>())
                {
                    auto boxRect = option.rect;

                    boxRect.setX(boxRect.x() + 0.5);
                    boxRect.setY(boxRect.y() + 3.5);
                    boxRect.setWidth(boxRect.width() - 1.0);
                    boxRect.setHeight(boxRect.height() - 4.0);

                    QPainterPath path;
                    path.addRoundedRect(boxRect, 6, 6);

                    // Get the foreground color of the current object to draw our sub-object-selected box
                    auto targetColorBrush = index.data(Qt::ForegroundRole).value<QBrush>();
                    QPen pen(targetColorBrush.color(), 2);

                    // Alter the dash pattern available for better visual appeal
                    QVector<qreal> dashes;
                    dashes << 3 << 5;
                    pen.setStyle(Qt::PenStyle::DashLine);
                    pen.setDashPattern(dashes);

                    painter->save();
                    painter->setRenderHint(QPainter::Antialiasing);
                    painter->setPen(pen);
                    painter->drawPath(path);
                    painter->restore();
                }
            }
        }

        QStyledItemDelegate::paint(painter, customOptions, index);

        // Draw this Slice Handle Accent if the item is selected after the entry is drawn.
        if (option.state & QStyle::State_Selected)
        {
            if (index.data(OutlinerListModel::SliceBackgroundRole).value<bool>())
            {
                auto boxRect = option.rect;

                boxRect.setX(boxRect.x() + 0.5);
                boxRect.setY(boxRect.y() + 3.5);
                boxRect.setWidth(boxRect.width() - 1.0);
                boxRect.setHeight(boxRect.height() - 4.0);

                // These colors need to be moved into a style sheet in the future.
                // The new editor design will likely include style sheet colors for
                // slices and relevant selection of such.
                // ----------------------------------------------------------------------
                // The accent is drawn a very transparent blue color. to allow the entry text and icon to show through while
                // providing a subtle accent that is distinctly different from the standard slice handle accent.
                QColor background(0, 0, 255, 50);
                // The accent outline is a dark blue to keep the contrast down.
                QColor line(0, 0, 120);

                painter->save();
                painter->setRenderHint(QPainter::Antialiasing);
                QPainterPath path;
                path.addRoundedRect(boxRect, 6, 6);
                QPen pen(line, 1);
                painter->setPen(pen);
                painter->fillPath(path, background);
                painter->drawPath(path);
                painter->restore();
            }
        }
    }

    QSize OutlinerItemDelegate::sizeHint(const QStyleOptionViewItem& option, const QModelIndex&) const
    {
        // Get the height of a tall character...
        auto textRect = option.fontMetrics.boundingRect("|");
        // And add 8 to it gives the outliner roughly the visible spacing we're looking for.
        return QSize(0, textRect.height() + OutlinerListModel::s_OutlinerSpacing);
    }

} // AzToolsFramework

#include <UI/Outliner/OutlinerListModel.moc>
