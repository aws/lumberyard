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

#include <QtWidgets/QWidget>
#include <QtGui/QIcon>

#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Slice/SliceComponent.h>
#include <AzCore/Asset/AssetCommon.h>

#include <AzToolsFramework/API/ToolsApplicationAPI.h>

class QTreeWidget;
class QTreeWidgetItem;
class QRadioButton;
class QLabel;
class QVBoxLayout;

namespace AZ
{
    class SliceAsset;
}

namespace AzToolsFramework
{
    class InstanceDataNode;
    class FieldTreeItem;

    using SliceAssetPtr = AZ::Data::Asset<AZ::SliceAsset>;

    /**
     * Overlay to display if no data changes were detected.
     */
    class NoChangesOverlay
        : public QWidget
    {
        Q_OBJECT

    public:
        explicit NoChangesOverlay(QWidget* pParent);
        ~NoChangesOverlay() override;

    Q_SIGNALS:
        void OnCloseClicked();

    private:
        bool eventFilter(QObject* obj, QEvent* event) override;
        void UpdateGeometry();
    };

    /**
     * Widget for pushing multiple entities/fields to slices.
     */
    class SlicePushWidget
        : public QWidget
        , public AZ::Data::AssetBus::MultiHandler
    {
        Q_OBJECT

    public:

        /**
         * \param entities - set of entity Ids to analyze/push.
         */
        explicit SlicePushWidget(const EntityIdList& entities, AZ::SerializeContext* serializeContext, QWidget* parent = 0);
        ~SlicePushWidget() override;

        //////////////////////////////////////////////////////////////////////////
        // AssetBus
        void OnAssetReloaded(AZ::Data::Asset<AZ::Data::AssetData> asset) override;
        //////////////////////////////////////////////////////////////////////////

    Q_SIGNALS:

        void OnFinished();  ///< The push operation finished successfully.
        void OnCanceled();  ///< The push operation failed with errors.

    public Q_SLOTS:

        /// A field has been selected in the field tree.
        void OnFieldSelectionChanged();

        /// The user has selected a new slice target.
        void OnSliceRadioButtonChanged(QRadioButton* selectButton, const AZ::Data::AssetId assetId, bool checked);

        /// The user has checked/unchecked a field.
        void OnFieldDataChanged(QTreeWidgetItem* item, int column);

        /// The 'Push Selected' button has been clicked.
        void OnPushClicked();

        /// The 'Cancel' button has been clicked.
        void OnCancelClicked();

    private:

        /// Conduct initial analysis for entities.
        void Setup(const EntityIdList& entities, AZ::SerializeContext* serializeContext);

        /// Sets up handlers for asset changes on target entities. Invoked by Setup().
        void SetupAssetMonitoring();

        /// Populate field tree items and cache InstanceDataHierarchies. Invoked by Setup().
        void PopulateFieldTree(const EntityIdList& entities);

        /// Gather new entities added to the current slice instance and populate the field tree accordingly.
        /// @param      entities                     a set of ids of entities 
        /// @param[out] unpushableNewChildEntityIds  ids of new entities that cannot be pushed, for example entities from different instances of the same slice asset you're pushing to
        void PopulateFieldTreeAddedEntities(const EntityIdSet& entities, EntityIdSet& unpushableNewChildEntityIds);

        void PopulateFieldTreeRemovedEntities();

        /// Determine the number of total references to this asset, providing an approximate
        /// measurement of the impact of pushing changes to this slice.
        size_t CalculateReferenceCount(const AZ::Data::AssetId& assetId) const;

        /// Event filter for key presses.
        bool eventFilter(QObject* target, QEvent *event);

        /// Conduct the push operation for all selected fields.
        bool PushSelectedFields();

        bool CheckoutAndGatherAffectedAssets(AZStd::unordered_map<AZ::Data::AssetId, SliceAssetPtr>& assets);

        /// Gather valid target assets for a given field entry.
        AZStd::vector<SliceAssetPtr> GetValidTargetAssetsForField(const FieldTreeItem& item) const;

        AZ::Data::Asset<AZ::SliceAsset> CloneAsset(const AZ::Data::Asset<AZ::SliceAsset>& asset) const;

        /// Status message data types
        enum class StatusMessageType : int
        {
            Warning
        };

        struct StatusMessage
        {
            StatusMessageType       m_type;
            QString                 m_text;
        };

        void AddStatusMessage(StatusMessageType messageType, const QString& messageText);
        void DisplayStatusMessages();

        QTreeWidget*                                                m_fieldTree;            ///< Tree widget for fields (left side)
        QTreeWidget*                                                m_sliceTree;            ///< Tree widget for slice targets (right side)
        QLabel*                                                     m_infoLabel;            ///< Label above slice tree describing selection
        AZ::SerializeContext*                                       m_serializeContext;     ///< Serialize context set for the widget
        AZStd::vector<AZ::SliceComponent::SliceInstanceAddress>     m_sliceInstances;       ///< Cached slice instances for transform fixup
        QVBoxLayout*                                                m_bottomLayout;         ///< Bottom layout containing optional status messages, legend and buttons
        AZStd::vector<StatusMessage>                                m_statusMessages;       ///< Status messages to be displayed

        QIcon                                                       m_iconGroup;            ///< Icon to use for field groups (parents)
        QIcon                                                       m_iconChangedDataItem;  ///< Icon to use for modified field leafs that're pushable.
        QIcon                                                       m_iconNewDataItem;      ///< Icon to use for new child elements/data under pushable parent fields.
        QIcon                                                       m_iconRemovedDataItem;  ///< Icon to use for removed field leafs that're pushable.
        QIcon                                                       m_iconSliceItem;        ///< Icon to use for slice targets in the slice tree
        QIcon                                                       m_iconWarning;          ///< Icon to use for warnings
    };

} // namespace AzToolsFramework
