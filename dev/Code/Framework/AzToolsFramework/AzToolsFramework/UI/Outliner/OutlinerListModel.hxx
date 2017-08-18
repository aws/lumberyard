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

#ifndef OUTLINER_VIEW_MODEL_H
#define OUTLINER_VIEW_MODEL_H

#include "OutlinerCacheBus.h"

#include <AzCore/base.h>
#include <QtWidgets/QWidget>
#include <QtWidgets/QStyledItemDelegate>
#include <QtWidgets/QCheckBox>
#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/Component/EntityBus.h>
#include <AzToolsFramework/API/ToolsApplicationAPI.h>
#include <AzToolsFramework/UI/SearchWidget/SearchCriteriaWidget.hxx>

#include <AzToolsFramework/ToolsComponents/EditorLockComponentBus.h>
#include <AzToolsFramework/ToolsComponents/EditorVisibilityBus.h>
#include <AzToolsFramework/Entity/EditorEntityContextBus.h>

#pragma once
namespace AzToolsFramework
{
    class OutlinerCache;

    // A class representing an outliner slice handle
    // This currently assumes that all the roots in a multi-root slice MUST share the same parent.
    // Additional data will be required if we allow multi-root slices to share different parents.
    class OutlinerSliceHandle
    {
    public:
        using SliceHandleAddress = AZ::Uuid;

        OutlinerSliceHandle(const AZStd::string& name, const AZ::SliceComponent::SliceInstanceAddress& instance, SliceHandleAddress handleId = SliceHandleAddress::CreateRandom());
        OutlinerSliceHandle(const OutlinerSliceHandle& rhs) = delete;
        OutlinerSliceHandle& operator=(const OutlinerSliceHandle& rhs) = delete;

        OutlinerSliceHandle(OutlinerSliceHandle&& rhs);
        OutlinerSliceHandle& operator=(OutlinerSliceHandle&& rhs);

        const AZStd::string& GetName() const;
        const SliceHandleAddress& GetId() const;
        const SliceHandleAddress& GetInstanceId() const;
        const EntityIdList& GetRootEntities() const;

        // Functions for building the slice handle information
        void AddRootEntity(const AZ::EntityId& rootId);
        void SetParentEntity(const AZ::EntityId& parentId);
    private:
        EntityIdList m_rootEntities; // Root entities of the slice (As of the current push, this should be a single entity) - The slice handle appears as the parent of all these entities.
        AZ::EntityId m_parentEntity; // The parent entity of the slice entity roots - The slice handle will appear as a child of this entity in the outliner.
        AZStd::string m_name;        // The name of the slice handle. This is the filename, without the extension, of the slice derived from the path of the asset it was instantiated from.
                                     // In a cascaded slice, this is the name of the lowest level of the cascade that the root entities appear in.

        // Identifying Information
        AZ::SliceComponent::SliceInstanceAddress m_instance;
        SliceHandleAddress m_handleId; // The slice instance that this slice was instantiated from
    };

    //! Model for items in the OutlinerTreeView.
    //! Each item represents an Entity.
    //! Items are parented in the tree according to their transform hierarchy.
    class OutlinerListModel
        : public QAbstractItemModel
        , private ToolsApplicationEvents::Bus::Handler
        , private AzToolsFramework::EditorEntityContextNotificationBus::Handler
        , private OutlinerCacheNotificationBus::Handler
    {
        Q_OBJECT;

    public:
        AZ_CLASS_ALLOCATOR(OutlinerListModel, AZ::SystemAllocator, 0)

            //! Columns of data to display about each Entity.
            enum Column
        {
            ColumnName,                 //!< Entity name
            ColumnVisibilityToggle,     //!< Visibility Icons
            ColumnLockToggle,           //!< Lock Icons
            ColumnEntityId,             //!< Entity ID (To stabilize Sorting)
            ColumnSliceMembership,      //!< Slice Entity Column (Used to facilitate proper sorting)
            ColumnSortIndex,            //!< Index of sort order
            ColumnCount                 //!< Total number of columns
        };

        enum EntryType
        {
            EntityType,
            SliceEntityType,
            SliceHandleType
        };

        enum Roles
        {
            VisibilityRole = Qt::UserRole + 1,
            SliceBackgroundRole,
            EntityTypeRole,
            SelectedRole,
            ChildSelectedRole,
            ExpandedRole,
        };

        enum EntityIcon
        {
            SliceHandleIcon,            // Icon used to decorate slice handles
            BrokenSliceHandleIcon,      // Icon used to decorate broken slice handles
            SliceEntityIcon,            // Icon used to decorate entities that are part of a slice instantiation
            StandardEntityIcon          // Icon used to decorate entities that are not part of a slice instantiation
        };

        // 8 Pixels of spacing is appropriate and matches the outliner concept work from the UI team.
        static const int s_OutlinerSpacing = 8;

        OutlinerListModel(QObject* parent = nullptr);
        ~OutlinerListModel();

        void Initialize();

        // Qt overrides.
        int rowCount(const QModelIndex& parent = QModelIndex()) const override;
        int columnCount(const QModelIndex&) const override { return ColumnCount; }
        QVariant data(const QModelIndex& index, int role) const override;
        bool setData(const QModelIndex& index, const QVariant& value, int role = Qt::EditRole) override;
        QModelIndex index(int row, int column, const QModelIndex& parent = QModelIndex()) const override;
        QModelIndex parent(const QModelIndex& index) const override;
        Qt::ItemFlags flags(const QModelIndex& index) const override;
        Qt::DropActions supportedDropActions() const override;
        Qt::DropActions supportedDragActions() const override;
        bool dropMimeData(const QMimeData* data, Qt::DropAction action, int row, int column, const QModelIndex& parent) override;
        bool canDropMimeData(const QMimeData* data, Qt::DropAction action, int row, int column, const QModelIndex& parent) const override;

        bool IsSelected(const AZ::EntityId& entityId) const;
        bool IsEligibleForDeselect(const AZ::EntityId& entityId) const;

        QMimeData* mimeData(const QModelIndexList& indexes) const override;
        QStringList mimeTypes() const override;

        AZ::EntityId GetEntityAtIndex(const QModelIndex& index) const;
        QModelIndex GetIndexForEntity(const AZ::EntityId& entityId) const;
        //! Given a list of entities, returns the model indices for those entities.
        //! Useful for matching selection between the outliner and the 3D view.
        QModelIndexList GetEntityIndexList(const AZStd::vector<AZ::EntityId>& entities);

        QString GetSliceAssetName(const AZ::EntityId& entityId) const;
    Q_SIGNALS:
        void ExpandItem(const QModelIndex& index);

    public Q_SLOTS:
        void SearchCriteriaChanged(QStringList& criteriaList, AzToolsFramework::FilterOperatorType filterOperator);
        void OnItemExpanded(const QModelIndex& index);
        void OnItemCollapsed(const QModelIndex& index);

        // Buffer Processing Slots - These are called using single-shot events when the buffers begin to fill.
        void ProcessEventBuffers();
        bool CanReparentEntities(const AZ::EntityId& newParentId, const AzToolsFramework::EntityIdList& selectedEntityIds) const;
        bool ReparentEntities(const AZ::EntityId& newParentId, const AzToolsFramework::EntityIdList& selectedEntityIds);
    protected:
        using SliceHandleList = AZStd::vector<OutlinerSliceHandle>;
        using SliceHandleMap = AZStd::unordered_map<AZ::SliceComponent::SliceInstanceId, SliceHandleList>;
        using EntityCacheMap = AZStd::unordered_map<AZ::EntityId, OutlinerCache*>;

        //! After data is sent in through searchCriteriaChanged, these functions process and apply the filter.
        void BuildFilter(QStringList& criteriaList, AzToolsFramework::FilterOperatorType filterOperator);
        void SetFilterRegExp(const AZStd::string& filterType, const QRegExp& regExp);
        void ClearFilterRegExp(const AZStd::string& filterType = AZStd::string());
        //! Use the current filter setting and re-evaluate the filter.
        void InvalidateFilter();

        //! Editor component lock interface to enable/disable selection of entity in the viewport.
        //! Setting the editor lock state on a parent will recursively set the flag on all descendants as well. (to match visibility)
        void ToggleEditorLockState(const QModelIndex& index);
        void SetEditorLockState(const QModelIndex& index, bool locked);
        void SetEditorLockStateOnAllChildren(const QModelIndex& index, bool locked);

        //! Editor Visibility interface to enable/disable rendering in the viewport.
        //! Setting the editor visibility on a parent will recursively set the flag on all descendants as well.
        void ToggleEditorVisibility(const QModelIndex& index);
        void SetEditorVisibility(const QModelIndex& index, bool visibleFlag);
        void SetEditorVisibilityOnAllChildren(const QModelIndex& index, bool visibleFlag);

        OutlinerCache* GetCacheForSliceHandle(OutlinerSliceHandle::SliceHandleAddress handleId);
        const OutlinerCache* GetCacheForSliceHandle(OutlinerSliceHandle::SliceHandleAddress handleId) const;
        OutlinerCache* GetCacheForEntity(const AZ::EntityId& entityId);
        const OutlinerCache* GetCacheForEntity(const AZ::EntityId& entityId) const;
        OutlinerCache* GetValidCacheFromIndex(const QModelIndex& modelIndex) const;
        QModelIndex GetIndexFromCache(const OutlinerCache* cache, int column = 0) const;

        //! Get all editor entities and ensure they're registered.
        void RegisterExistingEntities();

        // QT Function Wrappers
        // Several operations during the adding and removal of objects from the outliner require
        // parents and other items be redrawn. This can't happen during a qt insertion, removal,
        // or restructuring operation. These wrappers set a flag that queues up all invalidation
        // requests and processes them when the blocking operations are complete.
        void BeginInsertRows(const QModelIndex& parent, int first, int last);
        void EndInsertRows();
        void BeginMoveRows(const QModelIndex &sourceParent, int sourceFirst, int sourceLast, const QModelIndex &destinationParent, int destinationChild);
        void EndMoveRows();
        void BeginRemoveRows(const QModelIndex &parent, int first, int last);
        void EndRemoveRows();

        // Initiate a buffer processing operation
        void RequestEventBufferProcessing();

        // Buffer processing functions
        // Events are buffered when they are received. These functions will be called
        // to flush the buffers when it's appropriate to do so.
        void ProcessRegistrationBuffer();
        void ProcessDeregistrationBuffer();
        void ProcessReparentingBuffer();
        void ProcessSelectionChangeBuffers();

        ////////////////////////////////////////////////////////////////////////
        // ToolsApplicationEvents::Bus
        // Outliner ignores entities not owned by the editor context
        ////////////////////////////////////////////////////////////////////////
        virtual void EntityDeregistered(AZ::EntityId entityId) override;
        virtual void EntityRegistered(AZ::EntityId entityId) override;
        virtual void EntityParentChanged(AZ::EntityId entityId, AZ::EntityId newParentId, AZ::EntityId oldParent = AZ::EntityId()) override;
        virtual void EntityCreatedAsChild(AZ::EntityId entityId, AZ::EntityId parentId) override;

        //////////////////////////////////////////////////////////////////////////
        // AzToolsFramework::EditorEntityContextNotificationBus::Handler
        // Used to identify and properly draw entities that are part of a slice,
        // insert slice handles, and facilitate the processing of new entities.
        //////////////////////////////////////////////////////////////////////////
        void OnEditorEntitiesReplacedBySlicedEntities(const AZStd::unordered_map<AZ::EntityId, AZ::EntityId>& /*replacedEntitiesMap*/) override;
        void OnEditorEntitiesSliceOwnershipChanged(const AzToolsFramework::EntityIdList& entityIdList) override;

        //////////////////////////////////////////////////////////////////////////
        // AzToolsFramework::OutlinerCacheNotificationBus::Handler
        // Receive notification from cache entries that they need to be redrawn
        // in the outliner pane
        //////////////////////////////////////////////////////////////////////////
        virtual void EntityCacheChanged(const AZ::EntityId& entityId) override;
        virtual void EntityCacheSelectionRequest(const AZ::EntityId& entityId);
        virtual void EntityCacheDeselectionRequest(const AZ::EntityId& entityId);

        //! Given a slice component, this will recursively dig into its slice references and instances to find the transform roots of every cascaded slice.
        //! The map provided is used to map the root entity IDs found to actual instantiated IDs. This should be the entity ID map of the instantiated slice you're
        //! compiling the information for.
        //! Note: The roots structure will not be completely filled out. Further processing is required to find the transform parents of each root entity.
        void FindAllRootEntities(AZ::SliceComponent& sliceComponent, const AZ::SliceComponent::SliceInstanceAddress& instance, SliceHandleMap& sliceHandles, const AZ::SliceComponent::EntityIdToEntityIdMap& parentEntityMap);

        //! Takes the ID of a newly created entity
        //! This function will find its parent and any existing children and make the appropriate parent-child links between them.
        //! If its parent doesn't exist yet, the relationship is stored for later.
        void FindFamily(const AZ::EntityId& entityId);

        //! Register an entity as an orphan
        //! This is typically occurs when an entity is created before it's parent during a streaming operation
        void RegisterOrphan(const AZ::EntityId& entityId, const AZ::EntityId& parentId);

        //! Register all children of an entity as orphans
        void MarkChildrenAsOrphans(OutlinerCache* entityCache);

        //! This will mark all entities which belong to the given prefab instance so that they appear as such in the outliner.
        //! It will also locate the roots of this and all nested prefabs so they can be displayed properly as well.
        void MarkUpSliceEntities(const AZ::Data::AssetId& sliceAssetId, const AZ::SliceComponent::SliceInstanceAddress& sliceAddress);

        //! Request a hierarchy expansion
        void ExpandItemHierarchy(const AZ::EntityId& entityId);

        //! Processes and clears the expansion and invalidation request buffers.
        //! Call at the end of any function that could invalidate items in the tree view.
        void ProcessRequestBuffers();

        // Drag and Drop
        bool dropMimeDataComponentPalette(const QMimeData* data, Qt::DropAction action, int row, int column, const QModelIndex& dropModelIndex, const AZ::EntityId& dropTargetId);
        bool dropMimeDataComponentAssets(const QMimeData* data, Qt::DropAction action, int row, int column, const QModelIndex& dropModelIndex, const AZ::EntityId& dropTargetId);
        bool dropMimeDataEntities(const QMimeData* data, Qt::DropAction action, int row, int column, const QModelIndex& dropModelIndex, const AZ::EntityId& dropTargetId);

        bool canDropMimeDataForEntityIds(const QMimeData* data, const QModelIndex& inddx) const;

        QVariant dataForAll(const QModelIndex& index, int role) const;
        QVariant dataForName(const QModelIndex& index, int role) const;
        QVariant dataForVisibility(const QModelIndex& index, int role) const;
        QVariant dataForLock(const QModelIndex& index, int role) const;
        QVariant dataForSortIndex(const QModelIndex& index, int role) const;

        // While true, invalidation requests are blocked and will be queued up.
        // If you clear this flag, be sure to process the invalidation requet buffer.
        bool m_treeModificationUnderway;

        // Used to store information for displaying artificially inserted slice handles into the hierarchy
        SliceHandleMap m_SliceHandles;

        //! "Root" item exists to simplify parenting operations,
        //! it is the parent of all Entities with no actual parent.
        //! It does not represent any actual Entity and does not appear in the view.
        OutlinerCache* m_entitiesCacheRoot;

        //! Cached data, mapped by EntityId (does not contain m_entitiesCacheRoot).
        EntityCacheMap m_entityToCacheMap;

        //! Data used in the filtering process.
        AzToolsFramework::FilterByCategoryMap m_filtersRegExp;
        AzToolsFramework::FilterOperatorType m_filterOperator;

        //! Hierarchy Expansion Tracking
        //! When entities are destroyed, we may need to know later if they were
        //! expanded in the outliner so we can restore that state.
        AZStd::unordered_map<AZ::EntityId, bool> m_removedExpansionState;

        // Data BuffersEntityParentChanged
        // Due to the way QT modifies its trees using begin and end functions, some requests
        // that occur in between need to be buffered for processing when it's safe to do so.

        //! Buffer for hierarchy expansion requests while invalidation is blocked
        AZStd::unordered_set<AZ::EntityId> m_expansionRequestBuffer;

        //! Buffer for invalidation requests while invalidation is blocked
        AZStd::unordered_set<AZ::EntityId> m_invalidationRequestBuffer;

        //! Buffer for invalidation requests while invalidation is blocked
        AZStd::unordered_set<AZ::EntityId> m_selectionRequestBuffer;

        //! Buffer for invalidation requests while invalidation is blocked
        AZStd::unordered_set<AZ::EntityId> m_deselectionRequestBuffer;

        //! Buffer request tracking flag. If this is true, then a buffer processing function is already queued up
        bool m_bufferProcessingPending;

        // Event Buffers
        // For more efficient processing of groups of entity events, we can buffer
        // groups of registration, de-registration, and re-parenting operations to
        // facilitate better performance of the outliner during these processes

        //! Buffer for entity registration requests
        AZStd::unordered_set<AZ::EntityId> m_entityRegistrationBuffer;

        //! Buffer for entity deregistration requests
        AZStd::unordered_set<AZ::EntityId> m_entityDeregistrationBuffer;

        //! Buffer for change-parent requests. The first entity is becoming a child of the second entity
        AZStd::unordered_set<AZStd::pair<AZ::EntityId, AZ::EntityId>> m_entityParentChangeBuffer;

        //! When streaming in entities, there's no guarantee that a transform parent has
        //! already been loaded. If we store a mapping from these absent parents to their
        //! orphans, we can properly re-parent children as their parents arrive.
        AZStd::unordered_map<AZ::EntityId, EntityIdList> m_absentParentMap;
    };

    /*!
    * OutlinerItemDelegate exists to render custom item-types.
    * Other item-types render in the default fashion.
    */
    class OutlinerItemDelegate
        : public QStyledItemDelegate
    {
    public:
        OutlinerItemDelegate(QWidget* parent = nullptr);

        // Qt overrides
        void paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const override;
        QSize sizeHint(const QStyleOptionViewItem& option, const QModelIndex& index) const override;
    private:
        // Mutability added because these are being used ONLY as renderers
        // for custom check boxes. The decision of whether or not to draw
        // them checked is tracked by the individual entities and items in
        // the hierarchy cache.
        mutable QCheckBox m_visibilityCheckBox;
        mutable QCheckBox m_lockCheckBox;
    };
} // namespace AzToolsFramework

Q_DECLARE_METATYPE(AZ::ComponentTypeList); // allows type to be stored by QVariable

#endif
