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

#include "StdAfx.h"

#include <QtWidgets/QDialog>
#include <QtWidgets/QDialogButtonBox>
#include <QtWidgets/QTreeWidget>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QHeaderView>
#include <QtWidgets/QRadioButton>
#include <QtWidgets/QSplitter>
#include <QtWidgets/QMessageBox>
#include <QKeyEvent>

#include <AzCore/Math/Transform.h>
#include <AzCore/Math/Quaternion.h>
#include <AzCore/Math/Vector3.h>
#include <AzCore/Asset/AssetManagerBus.h>
#include <AzCore/Asset/AssetManager.h>
#include <AzCore/Component/ComponentApplicationBus.h>
#include <AzCore/Serialization/Utils.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Slice/SliceBus.h>
#include <AzCore/IO/FileIO.h>

#include <AzFramework/Entity/EntityContextBus.h>
#include <AzFramework/StringFunc/StringFunc.h>
#include <AzFramework/Asset/AssetSystemBus.h>

#include <AzToolsFramework/API/EditorAssetSystemAPI.h>
#include <AzToolsFramework/UI/Slice/SlicePushWidget.hxx>
#include <AzToolsFramework/UI/PropertyEditor/InstanceDataHierarchy.h>
#include <AzToolsFramework/UI/PropertyEditor/PropertyEditorAPI.h>
#include <AzToolsFramework/UI/UICore/ProgressShield.hxx>
#include <AzToolsFramework/Slice/SliceUtilities.h>
#include <AzToolsFramework/Entity/EditorEntityContextBus.h>
#include <AzToolsFramework/Slice/SliceTransaction.h>

namespace AzToolsFramework
{
    /**
     * Simple overlay control for when no data changes are present.
     */
    NoChangesOverlay::NoChangesOverlay(QWidget* pParent)
        : QWidget(pParent)
    {
        AZ_Assert(pParent, "There must be a parent.");
        setAttribute(Qt::WA_StyledBackground, true);
        int width = parentWidget()->geometry().width();
        int height =  parentWidget()->geometry().height();
        this->setGeometry(0, 0, width, height);
        pParent->installEventFilter(this);

        QLabel* label = new QLabel(tr("No saveable changes detected"), this);
        label->setAlignment(Qt::AlignCenter);
        QVBoxLayout* layout = new QVBoxLayout();
        layout->setAlignment(Qt::AlignHCenter);
        layout->addWidget(label);
        QPushButton* button = new QPushButton(tr("Close"), this);
        button->setFixedSize(200, 40);
        layout->addWidget(button);

        connect(button, &QPushButton::released, this, &NoChangesOverlay::OnCloseClicked);

        setLayout(layout);

        UpdateGeometry();
    }

    NoChangesOverlay::~NoChangesOverlay()
    {
        parentWidget()->removeEventFilter(this);
    }

    void NoChangesOverlay::UpdateGeometry()
    {
        int width = this->parentWidget()->geometry().width();
        int height =  this->parentWidget()->geometry().height();
        this->setGeometry(0, 0, width, height);
    }

    bool NoChangesOverlay::eventFilter(QObject* obj, QEvent* event)
    {
        if (event->type() == QEvent::Resize)
        {
            UpdateGeometry();
        }

        return QObject::eventFilter(obj, event);
    }
   
    static const int kRowSize = 22;

    using SliceAssetPtr = AZ::Data::Asset<AZ::SliceAsset>;

     /**
     * Represents an item in the slice target tree (right pane).
     * Each item represents a single valid slice asset target for the selected node in the left pane/tree.
     */
    class SliceTargetTreeItem
        : public QTreeWidgetItem
    {
    public:

        explicit SliceTargetTreeItem(const SliceAssetPtr& asset, QTreeWidget* view = nullptr, int type = QTreeWidgetItem::Type)
            : QTreeWidgetItem(view, type)
            , m_selectButton(nullptr)
        {
            m_asset = asset;
            m_selectButton = new QRadioButton();
            setSizeHint(0, QSize(0, kRowSize));
        }

        explicit SliceTargetTreeItem(const SliceAssetPtr& asset, QTreeWidgetItem* item = nullptr, int type = QTreeWidgetItem::Type)
            : QTreeWidgetItem(item, type)
            , m_selectButton(nullptr)
        {
            m_asset = asset;
            m_selectButton = new QRadioButton();
            setSizeHint(0, QSize(0, kRowSize));
        }

        SliceAssetPtr m_asset;
        QRadioButton* m_selectButton;
    };

    /**
     * Represents an entry in the data tree. There will be a root item for each entity included in the push operation,
     * with child hierarchies for all fields that differ from the base slice.
     * Root items that represent entities store slightly different data from leaf entities. See comments below
     * on class members for details.
     */
    class FieldTreeItem
        : public QTreeWidgetItem
    {
    private:
        explicit FieldTreeItem(QTreeWidget* parent, int type = QTreeWidgetItem::Type)
            : QTreeWidgetItem(parent, type)
            , m_pushedHiddenAutoPushNodes(false)
            , m_entity(nullptr)
            , m_entityItem(nullptr)
            , m_node(nullptr)
            , m_isEntityRemoval(false)
            , m_isEntityAdd(false)
        {
            setSizeHint(0, QSize(0, kRowSize));
        }
        explicit FieldTreeItem(FieldTreeItem* parent, int type = QTreeWidgetItem::Type)
            : QTreeWidgetItem(parent, type)
            , m_pushedHiddenAutoPushNodes(false)
            , m_entity(nullptr)
            , m_entityItem(nullptr)
            , m_node(nullptr)
            , m_isEntityRemoval(false)
            , m_isEntityAdd(false)
        {
            setSizeHint(0, QSize(0, kRowSize));
        }

    public:

        static FieldTreeItem* CreateEntityRootItem(AZ::Entity* entity, QTreeWidget* view = nullptr, int type = QTreeWidgetItem::Type)
        {
            FieldTreeItem* item = new FieldTreeItem(view, type);
            item->m_entity = entity;
            item->m_entityItem = item;
            item->m_node = &item->m_hierarchy;
            return item;
        }
        
        static FieldTreeItem* CreateFieldItem(AzToolsFramework::InstanceDataNode* node, FieldTreeItem* entityItem, FieldTreeItem* parentItem, int type = QTreeWidgetItem::Type)
        {
            FieldTreeItem* item = new FieldTreeItem(parentItem, type);
            item->m_entity = entityItem->m_entity;
            item->m_entityItem = entityItem;
            item->m_node = node;
            item->m_selectedAsset = entityItem->m_selectedAsset;
            return item;
        }
        
        static FieldTreeItem* CreateEntityRemovalItem(AZ::Entity* assetEntityToRemove, 
                                                      const SliceAssetPtr& targetAsset, 
                                                      const AZ::SliceComponent::SliceInstanceAddress& sliceAddress, 
                                                      int type = QTreeWidgetItem::Type)
        {
            FieldTreeItem* item = new FieldTreeItem(static_cast<QTreeWidget*>(nullptr), type);
            item->m_entity = assetEntityToRemove;
            item->m_entityItem = item;
            item->m_isEntityRemoval = true;
            item->m_sliceAddress = sliceAddress;
            item->m_selectedAsset = targetAsset.GetId();
            return item;
        }

        static FieldTreeItem* CreateEntityAddItem(AZ::Entity* entity, int type = QTreeWidgetItem::Type)
        {
            FieldTreeItem* item = new FieldTreeItem(static_cast<QTreeWidget*>(nullptr), type);
            item->m_entity = entity;
            item->m_entityItem = item;
            item->m_isEntityAdd = true;
            item->m_node = &item->m_hierarchy;
            return item;
        }

        bool IsPushableItem() const 
        {
            return m_isEntityAdd || m_isEntityRemoval || childCount() == 0; // Either entity add/removal, or leaf item (for updates)
        }

        bool AllLeafNodesShareSelectedAsset(AZ::Data::AssetId targetAssetId) const
        {
            if (IsPushableItem())
            {
                return m_selectedAsset == targetAssetId;
            }

            for (int childIndex = 0; childIndex < childCount(); childIndex++)
            {
                FieldTreeItem* childItem = static_cast<FieldTreeItem*>(child(childIndex));
                if (childItem)
                {
                    if (!childItem->AllLeafNodesShareSelectedAsset(targetAssetId))
                    {
                        return false;
                    }
                }
            }

            return true;
        }

        bool AllLeafNodesArePushableToThisAncestor(AZ::Entity& entity) const
        {
            if (IsPushableItem())
            {
                if (m_isEntityRemoval)
                {
                    return true; // Removal of entities is guaranteed to be pushable.
                }
                else
                {
                    return SliceUtilities::IsNodePushable(*m_node, SliceUtilities::IsRootEntity(entity));
                }
                
            }

            for (int childIndex = 0; childIndex < childCount(); childIndex++)
            {
                FieldTreeItem* childItem = static_cast<FieldTreeItem*>(child(childIndex));
                if (childItem)
                {
                    if (!childItem->AllLeafNodesArePushableToThisAncestor(entity))
                    {
                        return false;
                    }
                }
            }

            return true;
        }

        // For (root) Entity-level items, contains the entities built IDH.
        AzToolsFramework::InstanceDataHierarchy m_hierarchy;

        // For (root) Entity-level item, points to hierarchy. For child items, points to the IDH node for the field.
        AzToolsFramework::InstanceDataNode* m_node;

        // For (root) Entity-level item, hidden-but-pushable field nodes (these aren't displayed, so if a push is happening, they're automatically included in push).
        AZStd::unordered_set<AzToolsFramework::InstanceDataNode*> m_hiddenAutoPushNodes;

        // For (root) Entity-level item, whether the hidden auto-push nodes have been pushed yet
        bool m_pushedHiddenAutoPushNodes;

        // Entity associated with this node, or the entity parent of this node.
        AZ::Entity* m_entity;

        // Set if this an entity removal from the target slice.
        bool m_isEntityRemoval;

        // Set if this a potential entity addition to an existing slice.
        bool m_isEntityAdd;

        // For entity-level items only, contains slice ancestor list.
        AZ::SliceComponent::EntityAncestorList m_ancestors;
        AZ::SliceComponent::SliceInstanceAddress m_sliceAddress;

        // Relevant for leaf items and entity add/removals, stores Id of target slice asset.
        AZ::Data::AssetId m_selectedAsset;

        // Points to item at root of this entity hieararchy.
        FieldTreeItem* m_entityItem;
    };

    //=========================================================================
    // SlicePushWidget
    //=========================================================================
    SlicePushWidget::SlicePushWidget(const EntityIdList& entities, AZ::SerializeContext* serializeContext, QWidget* parent)
        : QWidget(parent)
    {
        QVBoxLayout* layout = new QVBoxLayout();

        // Window layout:
        // ===============================================
        // ||-------------------------------------------||
        // || (Data tree label)    (slice tree label)   ||
        // ||-------------------------------------------||
        // ||                     |                     ||
        // ||                (splitter)                 ||
        // ||                     |                     ||
        // ||    (Data tree)      | (Slice target tree) ||
        // ||                     |                     ||
        // ||                     |                     ||
        // ||                     |                     ||
        // ||-------------------------------------------||
        // || (optional status messages)                ||
        // || (legend)                     (button box) ||
        // ===============================================

        setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

        m_fieldTree = new QTreeWidget();
        m_fieldTree->setColumnCount(2);
        m_fieldTree->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
        m_fieldTree->setDragDropMode(QAbstractItemView::DragDropMode::NoDragDrop);
        m_fieldTree->setDragEnabled(false);
        m_fieldTree->setSelectionMode(QAbstractItemView::SingleSelection);
        m_fieldTree->header()->setStretchLastSection(false);
        m_fieldTree->header()->setSectionResizeMode(0, QHeaderView::Stretch);
        m_fieldTree->header()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
        m_fieldTree->installEventFilter(this);

        m_sliceTree = new QTreeWidget();
        m_sliceTree->setColumnCount(3);
        m_sliceTree->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
        m_sliceTree->setDragDropMode(QAbstractItemView::DragDropMode::NoDragDrop);
        m_sliceTree->setDragEnabled(false);
        m_sliceTree->header()->setStretchLastSection(false);
        m_sliceTree->header()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
        m_sliceTree->header()->setSectionResizeMode(1, QHeaderView::Stretch);
        m_sliceTree->header()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
        m_sliceTree->setSelectionMode(QAbstractItemView::SelectionMode::NoSelection);
        m_sliceTree->setFocusPolicy(Qt::FocusPolicy::NoFocus);
        connect(m_sliceTree, &QTreeWidget::itemClicked, this, 
            [](QTreeWidgetItem* item, int)
            {
                SliceTargetTreeItem* sliceItem = static_cast<SliceTargetTreeItem*>(item);
                sliceItem->m_selectButton->setChecked(true);
            });

        QStringList headers;
        headers << tr("Property");
        headers << tr("Target Slice Asset");
        m_fieldTree->setHeaderLabels(headers);
        m_fieldTree->setToolTip(tr("Choose which properties to push, and to which assets."));

        headers = QStringList();
        headers << tr("Selected");
        headers << tr("Slice Asset");
        headers << tr("Level References"); // Count is only relevant in level scope. Revisit as part of Slice-Editor effort.
        m_sliceTree->setHeaderLabels(headers);
        m_sliceTree->setToolTip(tr("Choose target assets for selected property groups."));

        m_iconGroup = QIcon(":/PropertyEditor/Resources/browse_on.png");
        m_iconChangedDataItem = QIcon(":/PropertyEditor/Resources/changed_data_item.png");
        m_iconNewDataItem = QIcon(":/PropertyEditor/Resources/new_data_item.png");
        m_iconRemovedDataItem = QIcon(":/PropertyEditor/Resources/removed_data_item.png");
        m_iconSliceItem = QIcon(":/PropertyEditor/Resources/slice_item.png");
        m_iconWarning = QIcon(":/PropertyEditor/Resources/warning.png");

        // Main splitter contains left & right widgets (the two trees).
        QSplitter* splitter = new QSplitter();

        QWidget* leftWidget = new QWidget();
        QVBoxLayout* leftLayout = new QVBoxLayout();
        QLabel* fieldTreeLabel = new QLabel(tr("Select fields and assign target slices:"));
        fieldTreeLabel->setProperty("Title", true);
        leftLayout->addWidget(fieldTreeLabel);
        leftLayout->addWidget(m_fieldTree);
        leftWidget->setLayout(leftLayout);

        QWidget* rightWidget = new QWidget();
        QVBoxLayout* rightLayout = new QVBoxLayout();
        m_infoLabel = new QLabel();
        m_infoLabel->setProperty("Title", true);
        rightLayout->addWidget(m_infoLabel);
        rightLayout->addWidget(m_sliceTree);
        rightWidget->setLayout(rightLayout);

        splitter->addWidget(leftWidget);
        splitter->addWidget(rightWidget);
        
        QList<int> sizes;
        const int totalWidth = size().width();
        const int firstColumnSize = totalWidth / 2;
        sizes << firstColumnSize << (totalWidth - firstColumnSize);
        splitter->setSizes(sizes);
        splitter->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Ignored);

        QHBoxLayout* bottomLegendAndButtonsLayout = new QHBoxLayout();

        // Create/add legend to bottom layout.
        {
            QLabel* imageChanged = new QLabel();
            if (!m_iconChangedDataItem.availableSizes().empty())
            {
                imageChanged->setPixmap(m_iconChangedDataItem.pixmap(m_iconChangedDataItem.availableSizes().first()));
            }
            QLabel* labelChanged = new QLabel(tr("Changed"));
            QLabel* imageAdded = new QLabel();
            if (!m_iconNewDataItem.availableSizes().empty())
            {
                imageAdded->setPixmap(m_iconNewDataItem.pixmap(m_iconNewDataItem.availableSizes().first()));
            }
            QLabel* labelAdded = new QLabel(tr("Added"));
            QLabel* imageRemoved = new QLabel();
            if (!m_iconRemovedDataItem.availableSizes().empty())
            {
                imageRemoved->setPixmap(m_iconRemovedDataItem.pixmap(m_iconRemovedDataItem.availableSizes().first()));
            }
            QLabel* labelRemoved = new QLabel(tr("Removed"));
            bottomLegendAndButtonsLayout->addWidget(imageChanged, 0);
            bottomLegendAndButtonsLayout->addWidget(labelChanged, 0);
            bottomLegendAndButtonsLayout->addWidget(imageAdded, 0);
            bottomLegendAndButtonsLayout->addWidget(labelAdded, 0);
            bottomLegendAndButtonsLayout->addWidget(imageRemoved, 0);
            bottomLegendAndButtonsLayout->addWidget(labelRemoved, 0);
        }

        // Create/populate button box.
        {
            QDialogButtonBox* buttonBox = new QDialogButtonBox(this);
            QPushButton* pushSelectedButton = new QPushButton(tr(" Save Selected Overrides"));
            pushSelectedButton->setToolTip(tr("Save selected overrides to the specified slice(es)."));
            pushSelectedButton->setDefault(false);
            pushSelectedButton->setAutoDefault(false);
            QPushButton* cancelButton = new QPushButton(tr("Cancel"));
            cancelButton->setDefault(true);
            cancelButton->setAutoDefault(true);
            buttonBox->addButton(cancelButton, QDialogButtonBox::RejectRole);
            buttonBox->addButton(pushSelectedButton, QDialogButtonBox::ApplyRole);
            connect(pushSelectedButton, &QPushButton::clicked, this, &SlicePushWidget::OnPushClicked);
            connect(cancelButton, &QPushButton::clicked, this, &SlicePushWidget::OnCancelClicked);

            bottomLegendAndButtonsLayout->addWidget(buttonBox, 1);
        }

        m_bottomLayout = new QVBoxLayout();
        m_bottomLayout->setAlignment(Qt::AlignBottom);
        m_bottomLayout->addLayout(bottomLegendAndButtonsLayout);

        // Add everything to main layout.
        layout->addWidget(splitter, 1);
        layout->addLayout(m_bottomLayout, 0);

        connect(m_fieldTree, &QTreeWidget::itemSelectionChanged, this, &SlicePushWidget::OnFieldSelectionChanged);
        connect(m_fieldTree, &QTreeWidget::itemChanged, this, &SlicePushWidget::OnFieldDataChanged);

        setLayout(layout);

        Setup(entities, serializeContext);
    }
    
    //=========================================================================
    // ~SlicePushWidget
    //=========================================================================
    SlicePushWidget::~SlicePushWidget()
    {
    }

    //=========================================================================
    // SlicePushWidget::OnAssetReloaded
    //=========================================================================
    void SlicePushWidget::OnAssetReloaded(AZ::Data::Asset<AZ::Data::AssetData> asset)
    {
        // Slice has changed underneath us, transaction invalidated
        AZStd::string sliceAssetPath;
        EBUS_EVENT_RESULT(sliceAssetPath, AZ::Data::AssetCatalogRequestBus, GetAssetPathById, asset.GetId());

        QMessageBox::warning(this, QStringLiteral("Push to Slice Aborting"), 
            QString("Slice asset changed on disk during push configuration, transaction canceled.\r\n\r\nAsset:\r\n%1").arg(sliceAssetPath.c_str()), 
            QMessageBox::Ok);
        
        // Force cancel the push
        emit OnCanceled();
    }

    //=========================================================================
    // SlicePushWidget::Setup
    //=========================================================================
    void SlicePushWidget::Setup(const EntityIdList& entities, AZ::SerializeContext* serializeContext)
    {
        m_serializeContext = serializeContext;

        if (!m_serializeContext)
        {
            EBUS_EVENT_RESULT(m_serializeContext, AZ::ComponentApplicationBus, GetSerializeContext);
        }

        PopulateFieldTree(entities);

        SetupAssetMonitoring();

        // Update asset name column for all pushable items (leaf update items, add/remove entity items)
        QTreeWidgetItemIterator itemIter(m_fieldTree);
        for (; *itemIter; ++itemIter)
        {
            FieldTreeItem* item = static_cast<FieldTreeItem*>(*itemIter);
            if (item->IsPushableItem())
            {
                AZStd::vector<SliceAssetPtr> validSliceAssets = GetValidTargetAssetsForField(*item);
                if (!validSliceAssets.empty())
                {
                    item->m_selectedAsset = validSliceAssets.front().GetId();

                    AZStd::string sliceAssetPath;
                    EBUS_EVENT_RESULT(sliceAssetPath, AZ::Data::AssetCatalogRequestBus, GetAssetPathById, item->m_selectedAsset);

                    if (item->m_node)
                    {
                        if (item->m_node->IsNewVersusComparison()) // New data vs. slice
                        {
                            item->setIcon(0, m_iconNewDataItem);
                        }
                        else if (item->m_node->IsRemovedVersusComparison()) // Removed data vs. slice
                        {
                            item->setIcon(0, m_iconRemovedDataItem);
                        }
                        else if (item->m_sliceAddress.first) // Changed data vs. slice
                        {
                            item->setIcon(0, m_iconChangedDataItem);
                        }
                    }

                    const char* shortPath = sliceAssetPath.c_str();
                    auto pos = sliceAssetPath.find_last_of('/');
                    if (pos == AZStd::string::npos)
                    {
                        pos = sliceAssetPath.find_last_of('\\');
                    }
                    if (pos != AZStd::string::npos)
                    {
                        shortPath += pos + 1;
                    }

                    item->setText(1, shortPath);

                    const QString tooltip =
                        tr("Field \"") + item->text(0) + tr("\" is assigned to asset \"") + sliceAssetPath.c_str() + "\".";
                    item->setToolTip(0, tooltip);
                    item->setToolTip(1, tooltip);
                }
            }
        }

        // Auto-select the first entity.
        const QModelIndex firstIndex = m_fieldTree->model()->index(0, 0);
        if (firstIndex.isValid())
        {
            m_fieldTree->selectionModel()->select(firstIndex, QItemSelectionModel::Select);
        }

        m_fieldTree->expandAll();

        // Display an overlay if no pushable options were found.
        if (0 == m_fieldTree->topLevelItemCount())
        {
            NoChangesOverlay* overlay = new NoChangesOverlay(this);
            connect(overlay, &NoChangesOverlay::OnCloseClicked, this, &SlicePushWidget::OnCancelClicked);
        }

        DisplayStatusMessages();
    }

    //=========================================================================
    // SlicePushWidget::SetupAssetMonitoring
    //=========================================================================
    void SlicePushWidget::SetupAssetMonitoring()
    {
        for (const auto& sliceAddress : m_sliceInstances)
        {
            AZ::Data::AssetBus::MultiHandler::BusConnect(sliceAddress.first->GetSliceAsset().GetId());
        }
    }

    //=========================================================================
    // SlicePushWidget::GetValidTargetAssetsForField
    //=========================================================================
    AZStd::vector<SliceAssetPtr> SlicePushWidget::GetValidTargetAssetsForField(const FieldTreeItem& item) const
    {
        AZStd::vector<SliceAssetPtr> validSliceAssets;
        validSliceAssets.reserve(m_fieldTree->model()->rowCount() * 2);

        const AZ::SliceComponent::EntityAncestorList& ancestors = item.m_entityItem->m_ancestors;
        for (auto iter = ancestors.begin(); iter != ancestors.end(); ++iter)
        {
            auto& ancestor = *iter;
            if (item.AllLeafNodesArePushableToThisAncestor(*ancestor.m_entity))
            {
                validSliceAssets.push_back(ancestor.m_sliceAddress.first->GetSliceAsset());
            }
        }

        // If the entity doesn't yet belong to a slice instance, allow targeting any of
        // the slices in the current slice push operation.
        if (ancestors.empty())
        {
            QTreeWidgetItemIterator itemIter(m_fieldTree);
            for (; *itemIter; ++itemIter)
            {
                FieldTreeItem* pItem = static_cast<FieldTreeItem*>(*itemIter);
                if (!pItem->parent())
                {
                    for (auto iter = pItem->m_ancestors.begin(); iter != pItem->m_ancestors.end(); ++iter)
                    {
                        auto& ancestor = *iter;
                        const AZ::Data::AssetId& sliceAssetId = ancestor.m_sliceAddress.first->GetSliceAsset().GetId();

                        auto assetCompare =
                            [ &sliceAssetId ] (const SliceAssetPtr& asset)
                        {
                            return asset.GetId() == sliceAssetId;
                        };

                        if (validSliceAssets.end() == AZStd::find_if(validSliceAssets.begin(), validSliceAssets.end(), assetCompare))
                        {
                            validSliceAssets.push_back(ancestor.m_sliceAddress.first->GetSliceAsset());
                        }
                    }
                }
            }
        }

        return validSliceAssets;
    }

    //=========================================================================
    // SlicePushWidget::OnFieldSelectionChanged
    //=========================================================================
    void SlicePushWidget::OnFieldSelectionChanged()
    {
        m_sliceTree->blockSignals(true);

        m_sliceTree->clear();
        m_infoLabel->setText("");

        if (!m_fieldTree->selectedItems().isEmpty())
        {
            FieldTreeItem* item = static_cast<FieldTreeItem*>(m_fieldTree->selectedItems().front());

            bool isLeaf = (item->childCount() == 0);

            if (item->m_isEntityRemoval)
            {
                m_infoLabel->setText(QString("Remove entity \"%1\" from slice:").arg(item->m_entity->GetName().c_str()));
            }
            else if (item->m_isEntityAdd)
            {
                m_infoLabel->setText(QString("Add entity \"%1\" %2to slice:").arg(item->m_entity->GetName().c_str()).arg((isLeaf && item->parent() == nullptr) ? "" : "and its hierarchy "));
            }
            else
            {
                m_infoLabel->setText(QString("Choose target slice for %1\"%2\":")
                    .arg(isLeaf ? "" : "children of ")
                    .arg((item->parent() == nullptr) ? item->m_entity->GetName().c_str() : GetNodeDisplayName(*item->m_node).c_str()));
            }

            SliceTargetTreeItem* parent = nullptr;

            AZStd::vector<SliceAssetPtr> validSliceAssets = GetValidTargetAssetsForField(*item);

            // For the selected item populate the tree of all valid slice targets.
            size_t level = 0;
            for (const SliceAssetPtr& sliceAsset : validSliceAssets)
            {
                const AZ::Data::AssetId& sliceAssetId = sliceAsset.GetId();

                AZStd::string assetPath;
                AZStd::string itemText;
                EBUS_EVENT_RESULT(assetPath, AZ::Data::AssetCatalogRequestBus, GetAssetPathById, sliceAssetId);
                AzFramework::StringFunc::Path::GetFullFileName(assetPath.c_str(), itemText);
                if (itemText.empty())
                {
                    itemText = sliceAssetId.ToString<AZStd::string>();
                }

                // We don't actually embed dependents as children, but just indent them.
                // Overall it's visually more clear alongside the radio buttons.

                SliceTargetTreeItem* sliceItem = new SliceTargetTreeItem(sliceAsset, m_sliceTree);

                for (size_t space = 0; space < level; ++space)
                {
                    itemText.insert(0, "    ");
                }

                sliceItem->setText(1, itemText.c_str());
                sliceItem->setIcon(1, m_iconSliceItem);

                const QString tooltip =
                    tr("Check to save field \"") + item->text(0) + tr("\" to asset \"") + assetPath.c_str() + "\".";
                sliceItem->setToolTip(0, tooltip);
                sliceItem->setToolTip(1, tooltip);

                const size_t instanceCount = CalculateReferenceCount(sliceAssetId);
                const AZStd::string instanceCountDescription = AZStd::string::format("%u", instanceCount);
                sliceItem->setText(2, instanceCountDescription.c_str());

                QRadioButton* selectButton = sliceItem->m_selectButton;
                connect(selectButton, &QRadioButton::toggled, this, [sliceAssetId, this, selectButton] (bool checked) { this->OnSliceRadioButtonChanged(selectButton, sliceAssetId, checked); });
                m_sliceTree->setItemWidget(sliceItem, 0, selectButton);

                // Auto-select the slice item associated with the currently assigned target slice Id.
                if ((item->IsPushableItem() && item->m_selectedAsset == sliceAssetId) || item->AllLeafNodesShareSelectedAsset(sliceAssetId))
                {
                    selectButton->setChecked(true);
                }

                parent = sliceItem;
                ++level;
            }
        }
        else
        {
            m_infoLabel->setText(tr("Select a leaf field to assign target slice."));
        }

        m_sliceTree->blockSignals(false);

        m_sliceTree->expandAll();
    }

    //=========================================================================
    // SlicePushWidget::OnFieldDataChanged
    //=========================================================================
    void SlicePushWidget::OnFieldDataChanged(QTreeWidgetItem* item, int column)
    {
        (void)column;

        // The user updated a checkbox for a data field. Propagate to child items.

        FieldTreeItem* fieldItem = static_cast<FieldTreeItem*>(item);

        const Qt::CheckState checkState = fieldItem->checkState(0);

        m_fieldTree->blockSignals(true);

        AZStd::vector<FieldTreeItem*> stack;
        for (int i = 0; i < fieldItem->childCount(); ++i)
        {
            stack.push_back(static_cast<FieldTreeItem*>(fieldItem->child(i)));
        }

        while (!stack.empty())
        {
            FieldTreeItem* pItem = stack.back();
            stack.pop_back();

            pItem->setCheckState(0, checkState);

            for (int i = 0; i < pItem->childCount(); ++i)
            {
                stack.push_back(static_cast<FieldTreeItem*>(pItem->child(i)));
            }
        }

        // For additions, we also need to sync "checked" state up the tree and prevent any children 
        // from being added whose parents aren't also added. You can't add a child item and not its parent
        // For removals, we need to sync "unchecked" state up the tree and prevent any parents
        // from being removed whose children aren't also being removed. No orphaning.
        if (fieldItem->m_isEntityAdd || fieldItem->m_isEntityRemoval)
        {
            FieldTreeItem* hierarchyRootItem = fieldItem;
            FieldTreeItem* currentItem = hierarchyRootItem;
            while (currentItem != nullptr)
            {
                hierarchyRootItem = currentItem;
                if (fieldItem->m_isEntityAdd)
                {
                    if (checkState == Qt::CheckState::Checked)
                    {
                        currentItem->setCheckState(0, checkState);
                    }
                }
                else if (fieldItem->m_isEntityRemoval)
                {
                    if (checkState == Qt::CheckState::Unchecked)
                    {
                        currentItem->setCheckState(0, checkState);
                    }
                }

                currentItem = static_cast<FieldTreeItem*>(currentItem->parent());
            }

            // Go through all children in this hierarchy and:
            // for addition trees, ensure if a parent is not being added, the child should not be added
            // for removal trees, ensure if a parent is being removed, the children should also be removed
            if (hierarchyRootItem)
            {
                AZStd::vector<FieldTreeItem*> verificationStack;
                for (int i = 0; i < hierarchyRootItem->childCount(); ++i)
                {
                    verificationStack.push_back(static_cast<FieldTreeItem*>(hierarchyRootItem->child(i)));
                }

                while (!verificationStack.empty())
                {
                    FieldTreeItem* pItem = verificationStack.back();
                    verificationStack.pop_back();

                    FieldTreeItem* parentItem = static_cast<FieldTreeItem*>(pItem->parent());
                    if (pItem->m_isEntityAdd)
                    {
                        if (parentItem->checkState(0) == Qt::CheckState::Unchecked)
                        {
                            pItem->setCheckState(0, Qt::CheckState::Unchecked);
                        }
                    }
                    else if (pItem->m_isEntityRemoval)
                    {
                        if (parentItem->checkState(0) == Qt::CheckState::Checked)
                        {
                            pItem->setCheckState(0, Qt::CheckState::Checked);
                        }
                    }

                    for (int i = 0; i < pItem->childCount(); ++i)
                    {
                        verificationStack.push_back(static_cast<FieldTreeItem*>(pItem->child(i)));
                    }
                }
            }
        }

        m_fieldTree->blockSignals(false);
    }

    //=========================================================================
    // SlicePushWidget::OnSliceRadioButtonChanged
    //=========================================================================
    void SlicePushWidget::OnSliceRadioButtonChanged(QRadioButton* selectButton, const AZ::Data::AssetId assetId, bool checked)
    {
        (void)checked;

        // The user selected a new target slice asset. Assign to the select data item and sync related items in the tree

        if (!m_fieldTree->selectedItems().isEmpty())
        {
            if (m_sliceTree->model()->rowCount() == 1)
            {
                selectButton->blockSignals(true);
                selectButton->setChecked(true);
                selectButton->blockSignals(false);
                return;
            }

            FieldTreeItem* currentItem = static_cast<FieldTreeItem*>(m_fieldTree->selectedItems().front());
            AZStd::vector<FieldTreeItem*> stack;
            if (currentItem->m_isEntityAdd || currentItem->m_isEntityRemoval)
            {
                // Adds/removals have their entire hierarchy synced
                FieldTreeItem* highestLevelParent = currentItem;
                while (FieldTreeItem* parentItem = static_cast<FieldTreeItem*>(highestLevelParent->parent()))
                {
                    highestLevelParent = parentItem;
                }
                stack.push_back(highestLevelParent);
            }
            else
            {
                // Other updates sync data to their children only
                stack.push_back(currentItem);
            }

            while (!stack.empty())
            {
                FieldTreeItem* item = stack.back();
                stack.pop_back();

                if (item->IsPushableItem())
                {
                    item->m_selectedAsset = assetId;

                    AZStd::string assetPath;
                    EBUS_EVENT_RESULT(assetPath, AZ::Data::AssetCatalogRequestBus, GetAssetPathById, assetId);
            
                    item->setText(1, assetPath.c_str());

                    QString tooltip;
                    if (item->m_isEntityRemoval)
                    {
                        tooltip = QString("Entity \"%1\" will be deleted from asset \"%2\".").arg(item->text(0)).arg(assetPath.c_str());
                    }
                    else if (item->m_isEntityAdd)
                    {
                        tooltip = QString("Entity \"%1\" will be added to asset \"%2\".").arg(item->text(0)).arg(assetPath.c_str());
                    }
                    else
                    {
                        tooltip = QString("Field \"%1\" is assigned to asset \"%2\".").arg(item->text(0)).arg(assetPath.c_str());
                    }

                    item->setToolTip(0, tooltip);
                    item->setToolTip(1, tooltip);
                }

                for (int childIndex = 0, childCount = item->childCount(); childIndex < childCount; ++childIndex)
                {
                    stack.push_back(static_cast<FieldTreeItem*>(item->child(childIndex)));
                }
            }
       }
    }

    //=========================================================================
    // SlicePushWidget::OnPushClicked
    //=========================================================================
    void SlicePushWidget::OnPushClicked()
    {
        if (PushSelectedFields())
        {
            emit OnFinished();
        }
    }
    
    //=========================================================================
    // SlicePushWidget::OnCancelClicked
    //=========================================================================
    void SlicePushWidget::OnCancelClicked()
    {
        emit OnCanceled();
    }

    //=========================================================================
    // SlicePushWidget::PopulateFieldTree
    //=========================================================================
    void SlicePushWidget::PopulateFieldTree(const EntityIdList& entities)
    {
        // Gather full hierarchy for analysis.
        EntityIdSet processEntityIds;
        EBUS_EVENT_RESULT(processEntityIds, ToolsApplicationRequests::Bus, GatherEntitiesAndAllDescendents, entities);

        EntityIdSet unpushableEntityIds;
        PopulateFieldTreeAddedEntities(processEntityIds, unpushableEntityIds);

        // Add a root item for each entity:
        // - Pre-calculate the IDH for each entity, comparing against all ancestors.
        // When encountering a pushable leaf (differs from one or more ancestors):
        // - Insert items for all levels of the hierarchy between the entity and the leaf.
        
        AZStd::vector<AZStd::unique_ptr<AZ::Entity>> tempCompareEntities;

        for (const AZ::EntityId& entityId : processEntityIds)
        {
            AZ::Entity* entity = nullptr;
            EBUS_EVENT_RESULT(entity, AZ::ComponentApplicationBus, FindEntity, entityId);

            if (entity)
            {
                AZ::SliceComponent::EntityAncestorList referenceAncestors;
                AZ::SliceComponent::SliceInstanceAddress sliceAddress(nullptr, nullptr);
                EBUS_EVENT_ID_RESULT(sliceAddress, entityId, AzFramework::EntityIdContextQueryBus, GetOwningSlice);

                AZ::SliceComponent::EntityAncestorList ancestors;
                if (sliceAddress.first)
                {
                    // Pre-gather all unique referenced assets.
                    if (m_sliceInstances.end() == AZStd::find(m_sliceInstances.begin(), m_sliceInstances.end(), sliceAddress))
                    {
                        m_sliceInstances.push_back(sliceAddress);
                    }

                    // Any entities that are unpushable don't need any further processing beyond gathering
                    // their slice instance (used in detecting entity removals)
                    if (unpushableEntityIds.find(entityId) != unpushableEntityIds.end())
                    {
                        continue;
                    }

                    sliceAddress.first->GetInstanceEntityAncestry(entityId, ancestors);
                }
                else
                {
                    // Entities not in slices will be handled in PopulateFieldTreeAddedEntities.
                    continue;
                }

                FieldTreeItem* entityItem = FieldTreeItem::CreateEntityRootItem(entity, m_fieldTree);

                // Generate the IDH for the entity.
                entityItem->m_hierarchy.AddRootInstance<AZ::Entity>(entity);

                // Use immediate ancestor as comparison instance.
                AZ::Entity* compareClone = nullptr;
                if (!ancestors.empty())
                {
                    const AZ::SliceComponent::Ancestor& ancestor = ancestors.front();

                    tempCompareEntities.push_back(
                        AzToolsFramework::SliceUtilities::CloneSliceEntityForComparison(*ancestor.m_entity, *sliceAddress.second, *m_serializeContext));

                    compareClone = tempCompareEntities.back().get();
                    entityItem->m_hierarchy.AddComparisonInstance<AZ::Entity>(compareClone);
                }

                entityItem->m_hierarchy.Build(m_serializeContext, AZ::SerializeContext::ENUM_ACCESS_FOR_READ);
                entityItem->m_ancestors = AZStd::move(ancestors);
                entityItem->m_sliceAddress = sliceAddress;
                
                entityItem->setText(0, QString("%1 (changed)").arg(entity->GetName().c_str()));
                entityItem->setIcon(0, m_iconChangedDataItem);
                entityItem->setCheckState(0, Qt::CheckState::Checked);

                AZStd::unordered_map<AzToolsFramework::InstanceDataNode*, FieldTreeItem*> nodeItemMap;
                nodeItemMap[&entityItem->m_hierarchy] = entityItem;

                AZStd::vector<AzToolsFramework::InstanceDataNode*> nodeStack;
                AZStd::vector<FieldTreeItem*> itemStack;

                // For determining whether an entity is a root entity of a slice, we check its live version 
                // and if available the comparison instance root. We check the comparison because it's the asset
                // version of the slice - the live version may have a parentId to it (slice is childed to another entity)
                // which would fail the RootEntity check, but the clone will be free of those modifications
                const bool isRootEntity = SliceUtilities::IsRootEntity(*entity) || (compareClone && SliceUtilities::IsRootEntity(*compareClone));
                bool entityHasPushableChanges = false;

                //
                // Recursively traverse IDH and add all modified fields to the tree.
                //

                for (InstanceDataNode& childNode : entityItem->m_hierarchy.GetChildren())
                {
                    nodeStack.push_back(&childNode);
                }

                while (!nodeStack.empty())
                {
                    AzToolsFramework::InstanceDataNode* node = nodeStack.back();
                    nodeStack.pop_back();

                    if (!SliceUtilities::IsNodePushable(*node, isRootEntity))
                    {
                        continue;
                    }

                    // Determine if this node should be pushed and not displayed (which applies to all child nodes)
                    // Because they are hidden pushes (used by hidden editor-only components that users shouldn't
                    // have to even know about in the push UI) we don't want to count them as "pushable changes" -
                    // it would be confusing to see that you can push changes to an entity but you can't see what
                    // any of those changes are!
                    {
                        const AZ::u32 sliceFlags = SliceUtilities::GetNodeSliceFlags(*node);

                        bool hidden = false;
                        if ((sliceFlags & AZ::Edit::SliceFlags::HideOnAdd) != 0 && node->IsNewVersusComparison())
                        {
                            hidden = true;
                        }

                        if ((sliceFlags & AZ::Edit::SliceFlags::HideOnChange) != 0 && node->IsDifferentVersusComparison())
                        {
                            hidden = true;
                        }

                        if ((sliceFlags & AZ::Edit::SliceFlags::HideOnRemove) != 0 && node->IsRemovedVersusComparison())
                        {
                            hidden = true;
                        }

                        if (hidden && (sliceFlags & AZ::Edit::SliceFlags::PushWhenHidden) != 0)
                        {
                            entityItem->m_hiddenAutoPushNodes.insert(node);
                        }

                        if (hidden)
                        {
                            continue;
                        }
                    }

                    for (InstanceDataNode& childNode : node->GetChildren())
                    {
                        nodeStack.push_back(&childNode);
                    }

                    if (node->HasChangesVersusComparison(false))
                    {
                        AZStd::vector<AzToolsFramework::InstanceDataNode*> walkStack;
                        walkStack.push_back(node);

                        AzToolsFramework::InstanceDataNode* runner = node->GetParent();
                        while (runner)
                        {
                            walkStack.push_back(runner);
                            runner = runner->GetParent();
                        }

                        while (!walkStack.empty())
                        {
                            AzToolsFramework::InstanceDataNode* walkNode = walkStack.back();
                            walkStack.pop_back();

                            if (nodeItemMap.find(walkNode) != nodeItemMap.end())
                            {
                                continue;
                            }

                            // Use the same visibility determination as the inspector.
                            AzToolsFramework::NodeDisplayVisibility visibility = CalculateNodeDisplayVisibility(*walkNode, true);

                            if (visibility == AzToolsFramework::NodeDisplayVisibility::Visible)
                            {
                                AZStd::string fieldName = GetNodeDisplayName(*walkNode);

                                if (!fieldName.empty())
                                {
                                    // Find closest ancestor with a display item.
                                    AzToolsFramework::InstanceDataNode* parent = walkNode->GetParent();
                                    FieldTreeItem* displayParent = entityItem;
                                    while (parent)
                                    {
                                        auto iter = nodeItemMap.find(parent);
                                        if (iter != nodeItemMap.end())
                                        {
                                            displayParent = iter->second;
                                            break;
                                        }
                                        parent = parent->GetParent();
                                    }

                                    FieldTreeItem* item = FieldTreeItem::CreateFieldItem(walkNode, entityItem, displayParent);
                                    item->setText(0, fieldName.c_str());
                                    item->setCheckState(0, Qt::CheckState::Checked);
                                    item->setIcon(0, m_iconGroup);
                                    item->m_sliceAddress = sliceAddress;
                                    nodeItemMap[walkNode] = item;

                                    entityHasPushableChanges = true;
                                }
                            }
                        }
                    }
                }

                if (!entityHasPushableChanges)
                {
                    // Don't display the entity if no pushable changes were present.
                    delete entityItem;
                }
            }
        }

        PopulateFieldTreeRemovedEntities();
    }

    using GetParentIdCB = AZStd::function<AZ::EntityId(const AZ::EntityId& entityId, FieldTreeItem* fieldTreeItem)>;
    void InsertHierarchicalItemsToFieldTree(const AZStd::unordered_map<AZ::EntityId, FieldTreeItem*>& entityToFieldTreeItemMap, const GetParentIdCB& getParentCB, QTreeWidget* fieldTree, AZStd::unordered_set<AZ::EntityId>& rootLevelEntitiesOut)
    {
        for (auto& entityItemPair : entityToFieldTreeItemMap)
        {
            AZ::EntityId entityId = entityItemPair.first;
            FieldTreeItem* entityItem = entityItemPair.second;
            AZ::EntityId parentId = getParentCB(entityId, entityItem);
            auto parentIt = entityToFieldTreeItemMap.find(parentId);
            if (parentIt != entityToFieldTreeItemMap.end())
            {
                FieldTreeItem* parentItem = parentIt->second;
                parentItem->addChild(entityItem);
            }
            else
            {
                fieldTree->insertTopLevelItem(0, entityItem);
                rootLevelEntitiesOut.insert(entityId);
            }
        }
    }

    //=========================================================================
    // SlicePushWidget::PopulateFieldTreeAddedEntities
    //=========================================================================
    void SlicePushWidget::PopulateFieldTreeAddedEntities(const EntityIdSet& entities, EntityIdSet& unpushableNewChildEntityIds)
    {
        AZStd::vector<AZStd::pair<AZ::EntityId, AZ::SliceComponent::EntityAncestorList>> newChildEntityIdAncestorPairs;
        AZStd::unordered_map<AZ::EntityId, FieldTreeItem*> entityToFieldTreeItemMap;

        bool hasUnpushableSliceEntityAdditions = false;
        for (const AZ::EntityId& entityId : entities)
        {
            AZ::Entity* entity = nullptr;
            EBUS_EVENT_RESULT(entity, AZ::ComponentApplicationBus, FindEntity, entityId);

            if (entity)
            {
                AZ::SliceComponent::SliceInstanceAddress entitySliceAddress(nullptr, nullptr);
                EBUS_EVENT_ID_RESULT(entitySliceAddress, entityId, AzFramework::EntityIdContextQueryBus, GetOwningSlice);

                // Determine which slice ancestry this entity should be considered for addition to, currently determined by the nearest
                // transform ancestor entity in the current selection of entities.
                AZ::SliceComponent::EntityAncestorList sliceAncestryToPushTo;
                AZ::SliceComponent::SliceInstanceAddress transformAncestorSliceAddress(nullptr, nullptr);
                AZ::EntityId parentId;
                AZ::SliceEntityHierarchyRequestBus::EventResult(parentId, entityId, &AZ::SliceEntityHierarchyRequestBus::Events::GetSliceEntityParentId);
                while (parentId.IsValid())
                {
                    // If we find a transform ancestor that's not part of the selected entities
                    // before we find a transform ancestor that has a relevant slice to consider 
                    // pushing this entity to, we skip the consideration of this entity for addition 
                    // because that would mean trying to add the entity to something we don't have selected
                    if (entities.find(parentId) == entities.end())
                    {
                        break;
                    }

                    AZ::Entity* parentEntity = nullptr;
                    AZ::ComponentApplicationBus::BroadcastResult(parentEntity, &AZ::ComponentApplicationRequests::FindEntity, parentId);
                    if (!parentEntity)
                    {
                        break;
                    }

                    EBUS_EVENT_ID_RESULT(transformAncestorSliceAddress, parentId, AzFramework::EntityIdContextQueryBus, GetOwningSlice);
                    if (transformAncestorSliceAddress.first && transformAncestorSliceAddress != entitySliceAddress)
                    {
                        transformAncestorSliceAddress.first->GetInstanceEntityAncestry(parentId, sliceAncestryToPushTo);
                        break;
                    }
                    AZ::SliceEntityHierarchyRequestBus::EventResult(parentId, parentId, &AZ::SliceEntityHierarchyRequestBus::Events::GetSliceEntityParentId);
                }

                // No slice ancestry found in any selected transform ancestors, so we don't have any slice to push
                // this entity to - skip it
                if (sliceAncestryToPushTo.empty())
                {
                    continue;
                }

                if (entitySliceAddress.first)
                {
                    // This is an entity that already belongs to a slice, need to verify it's a valid add
                    if (entitySliceAddress == transformAncestorSliceAddress)
                    {
                        // Entity shares slice instance address with transform ancestor, so it doesn't need to be added - it's already there!
                        continue;
                    }

                    if (!SliceUtilities::CheckSliceAdditionCyclicDependencySafe(entitySliceAddress, transformAncestorSliceAddress))
                    {
                        // Adding this entity's slice instance to the target slice would create a cyclic asset dependency which is strictly not allowed
                        hasUnpushableSliceEntityAdditions = true;
                        unpushableNewChildEntityIds.insert(entityId);

                        FieldTreeItem* item = FieldTreeItem::CreateEntityAddItem(entity);
                        item->setText(0, QString("%1 (added, unsaveable [see (A) below])").arg(entity->GetName().c_str()));
                        item->setIcon(0, m_iconNewDataItem);
                        item->m_ancestors = AZStd::move(sliceAncestryToPushTo);
                        item->setCheckState(0, Qt::CheckState::Unchecked);
                        item->setDisabled(true);
                        entityToFieldTreeItemMap[entity->GetId()] = item;
                    }
                    else
                    {
                        // Otherwise, this is a slice-owned entity that we want to push.
                        // At this point we have verified that it'd be safe to push to the immediate slice instance,
                        // but in the PushWidget the user will have the option of pushing to any slice asset
                        // in the sliceAncestryToPushTo. We need to check each ancestry entry and cull out any
                        // pushes that would result in cyclic asset dependencies.
                        // Example: Slice1 contains Slice2. I have a separate instance of Slice2, call it Slice2b. 
                        // It is valid to push Slice2b to Slice1, since Slice1 would then have two instances of Slice2.
                        // But it would be invalid to push the addition of Slice2b to Slice2, since then Slice2 would
                        // reference itself.
                        for (auto ancestorIt = sliceAncestryToPushTo.begin(); ancestorIt != sliceAncestryToPushTo.end(); ++ancestorIt)
                        {
                            const AZ::SliceComponent::SliceInstanceAddress& targetInstanceAddress = ancestorIt->m_sliceAddress;
                            if (!SliceUtilities::CheckSliceAdditionCyclicDependencySafe(entitySliceAddress, targetInstanceAddress))
                            {
                                // Once you find one invalid ancestor, all the rest will be as well
                                sliceAncestryToPushTo.erase(ancestorIt, sliceAncestryToPushTo.end());
                                break;
                            }
                        }
                        newChildEntityIdAncestorPairs.emplace_back(entityId, AZStd::move(sliceAncestryToPushTo));
                    }

                    continue;
                }
                else
                {
                    // This is an entity that doesn't belong to a slice yet, consider it for addition
                    newChildEntityIdAncestorPairs.emplace_back(entityId, AZStd::move(sliceAncestryToPushTo));
                }
            }
        }

        bool hasUnpushableTransformDescendants = false;
        for (const auto& entityIdAncestorPair: newChildEntityIdAncestorPairs)
        {
            const AZ::EntityId& entityId = entityIdAncestorPair.first;
            const AZ::SliceComponent::EntityAncestorList& ancestors = entityIdAncestorPair.second;

            // Test if the newly added loose entity is a descendant of an entity that's unpushable, if so mark it as unpushable too.
            AZ::EntityId unpushableAncestorId(AZ::EntityId::InvalidEntityId);
            AZ::EntityId parentId;
            AZ::SliceEntityHierarchyRequestBus::EventResult(parentId, entityId, &AZ::SliceEntityHierarchyRequestBus::Events::GetSliceEntityParentId);
            while (parentId.IsValid())
            {
                EntityIdSet::iterator foundItr = unpushableNewChildEntityIds.find(parentId);
                if (foundItr != unpushableNewChildEntityIds.end())
                {
                    unpushableAncestorId = *foundItr;
                    break;
                }

                AZ::EntityId currentParentId = parentId;
                parentId.SetInvalid();
                AZ::SliceEntityHierarchyRequestBus::EventResult(parentId, currentParentId, &AZ::SliceEntityHierarchyRequestBus::Events::GetSliceEntityParentId);
            }

            AZ::Entity* entity = nullptr;
            AZ::ComponentApplicationBus::BroadcastResult(entity, &AZ::ComponentApplicationRequests::FindEntity, entityId);

            if (unpushableAncestorId.IsValid())
            {
                hasUnpushableTransformDescendants = true;

                AZStd::string parentName;
                AZ::ComponentApplicationBus::BroadcastResult(parentName, &AZ::ComponentApplicationRequests::GetEntityName, parentId);

                FieldTreeItem* item = FieldTreeItem::CreateEntityAddItem(entity);
                item->setText(0, QString("%1 (added, unsaveable [see (B) below])").arg(entity->GetName().c_str()));
                item->setIcon(0, m_iconNewDataItem);
                item->m_ancestors = AZStd::move(ancestors);
                item->setCheckState(0, Qt::CheckState::Unchecked);
                item->setDisabled(true);
                entityToFieldTreeItemMap[entity->GetId()] = item;
            }
            else
            {
                // Create field entry to add this entity to the asset.
                FieldTreeItem* item = FieldTreeItem::CreateEntityAddItem(entity);
                item->setText(0, QString("%1 (added)").arg(entity->GetName().c_str()));
                item->setIcon(0, m_iconNewDataItem);
                item->m_ancestors = AZStd::move(ancestors);

                // By default, adds are not checked.
                item->setCheckState(0, Qt::CheckState::Unchecked);
                entityToFieldTreeItemMap[entity->GetId()] = item;
            }
        }

        // We set up the "add tree" so that you can see the transform hierarchy you're pushing in the push widget
        auto getParentIdCB = [](const AZ::EntityId& entityId, FieldTreeItem* fieldTreeItem) -> AZ::EntityId
        {
            (void)fieldTreeItem;

            AZ::EntityId parentId;
            AZ::SliceEntityHierarchyRequestBus::EventResult(parentId, entityId, &AZ::SliceEntityHierarchyRequestBus::Events::GetSliceEntityParentId);
            return parentId;
        };
        AZStd::unordered_set<AZ::EntityId> rootLevelAdditions;
        InsertHierarchicalItemsToFieldTree(entityToFieldTreeItemMap, getParentIdCB, m_fieldTree, rootLevelAdditions);

        // Show bottom warnings explaining unpushables
        {
            if (hasUnpushableSliceEntityAdditions)
            {
                AddStatusMessage(AzToolsFramework::SlicePushWidget::StatusMessageType::Warning, 
                                    QObject::tr("(A) Invalid additions detected. These are unsaveable because slices cannot contain instances of themselves. "
                                                "Saving these additions would create a cyclic asset dependency, causing infinite recursion."));
            }
            if (hasUnpushableTransformDescendants)
            {
                AddStatusMessage(AzToolsFramework::SlicePushWidget::StatusMessageType::Warning, 
                                    QObject::tr("(B) Invalid additions detected. These are unsaveable because they are transform children/descendants of other "
                                                "invalid-to-add entities."));
            }
        }
    }

    //=========================================================================
    // SlicePushWidget::PopulateFieldTreeRemovedEntities
    //=========================================================================
    void SlicePushWidget::PopulateFieldTreeRemovedEntities()
    {
        AZStd::unordered_map<AZ::EntityId, FieldTreeItem*> entityToFieldTreeItemMap;

        // For all slice instances we encountered, compare the entities it contains to the entities
        // contained in the underlying asset. If it's missing any entities that exist in the asset,
        // we can add fields to allow removal of the entity from the base slice.
        AZStd::unordered_set<AZ::EntityId> uniqueRemovedEntities;
        AZ::SliceComponent::EntityAncestorList ancestorList;
        AZ::SliceComponent::EntityList assetEntities;
        for (const AZ::SliceComponent::SliceInstanceAddress& instanceAddr : m_sliceInstances)
        {
            if (instanceAddr.first && instanceAddr.first->GetSliceAsset() && 
                instanceAddr.second && instanceAddr.second->GetInstantiated())
            {
                const AZ::SliceComponent::EntityList& instanceEntities = instanceAddr.second->GetInstantiated()->m_entities;
                assetEntities.clear();
                instanceAddr.first->GetSliceAsset().Get()->GetComponent()->GetEntities(assetEntities);
                if (assetEntities.size() > instanceEntities.size())
                {
                    // The removed entity is already gone from the instance's map, so we have to do a reverse-lookup
                    // to pin down which specific entities have been removed in the instance vs the asset.
                    for (auto assetEntityIter = assetEntities.begin(); assetEntityIter != assetEntities.end(); ++assetEntityIter)
                    {
                        AZ::Entity* assetEntity = (*assetEntityIter);
                        const AZ::EntityId assetEntityId = assetEntity->GetId();

                        if (uniqueRemovedEntities.end() != uniqueRemovedEntities.find(assetEntityId))
                        {
                            continue;
                        }

                        // Iterate over the entities left in the instance and if none of them have this
                        // asset entity as its ancestor, then we want to remove it.
                        // \todo - Investigate ways to make this non-linear time. Tricky since removed entities
                        // obviously aren't maintained in any maps.
                        bool foundAsAncestor = false;
                        for (const AZ::Entity* instanceEntity : instanceEntities)
                        {
                            ancestorList.clear();
                            instanceAddr.first->GetInstanceEntityAncestry(instanceEntity->GetId(), ancestorList, 1);
                            if (!ancestorList.empty() && ancestorList.begin()->m_entity == assetEntity)
                            {
                                foundAsAncestor = true;
                                break;
                            }
                        }

                        if (!foundAsAncestor)
                        {
                            // Grab ancestors, which determines which slices the removal can be pushed to.
                            uniqueRemovedEntities.insert(assetEntityId);

                            // Create field entry to remove this entity from the asset.
                            FieldTreeItem* item = FieldTreeItem::CreateEntityRemovalItem(assetEntity, instanceAddr.first->GetSliceAsset(), instanceAddr);
                            item->setText(0, QString("%1 (deleted)").arg(assetEntity->GetName().c_str()));
                            item->setIcon(0, m_iconRemovedDataItem);

                            // By default, removals are not checked for push.
                            item->setCheckState(0, Qt::CheckState::Unchecked);

                            item->m_ancestors.push_back(AZ::SliceComponent::Ancestor(assetEntity, instanceAddr));

                            AZ::SliceComponent::SliceInstanceAddress assetInstanceAddress = instanceAddr.first->GetSliceAsset().Get()->GetComponent()->FindSlice(assetEntityId);
                            if (assetInstanceAddress.first)
                            {
                                assetInstanceAddress.first->GetInstanceEntityAncestry(assetEntityId, item->m_ancestors);
                            }
                            else
                            {
                                // This is a loose entity of the slice
                                instanceAddr.first->GetInstanceEntityAncestry(assetEntityId, item->m_ancestors);
                            }

                            entityToFieldTreeItemMap[assetEntityId] = item;
                        }
                    }
                }
            }
        }


        // We set up the "removal tree" so that you can see the removal transform hierarchy you're pushing in the push widget
        auto getParentIdCB = [](const AZ::EntityId& entityId, FieldTreeItem* fieldTreeItem) -> AZ::EntityId
        {
            (void)entityId;

            // These are asset entities - not live entities, so we need to retrieve their parent id manually (they're not connected to buses)
            AZ::EntityId parentId;
            AZ::SliceEntityHierarchyInterface* sliceHierarchyInterface = AZ::EntityUtils::FindFirstDerivedComponent<AZ::SliceEntityHierarchyInterface>(fieldTreeItem->m_entity);
            if (sliceHierarchyInterface)
            {
                parentId = sliceHierarchyInterface->GetSliceEntityParentId();
            }
            else
            {
                AZ_Warning("Slice Push", false, "Attempting to get SliceEntityHierarchy parent id from entity that has no SliceEntityHierarchyInterface.");
            }
            return parentId;
        };
        AZStd::unordered_set<AZ::EntityId> rootLevelRemovals;
        InsertHierarchicalItemsToFieldTree(entityToFieldTreeItemMap, getParentIdCB, m_fieldTree, rootLevelRemovals);

        // For removal hierarchies we sync up target slice assets, so that if you're removing an entity in a transform hierarchy of
        // removals, any other removed entities also get removed from the same target asset. This constraint in the push UI is to help 
        // prevent accidentally orphaning entities in referenced slice assets. This isn't fool-proof yet, since if you re-arrange your entity
        // transform hierarchy with intermixed slices, you could still end up orphaning, but so far we've found most people
        // naturally keep slices and transform hierarchy pretty well in sync. The "target slice asset sync" is done in OnSliceRadioButtonChanged,
        // right here we need to make sure a given removal transform hierarchy shares a common set of ancestry to push to
        {
            for (auto& rootLevelRemovalEntityId : rootLevelRemovals)
            {
                // First, find the common set of ancestors that all removals in this hierarchy share
                AZStd::unordered_set<AZ::SliceComponent::SliceInstanceAddress> commonSlices;
                FieldTreeItem* rootFieldItem = entityToFieldTreeItemMap[rootLevelRemovalEntityId];
                {
                    for (auto& rootLevelAncestor : rootFieldItem->m_ancestors)
                    {
                        commonSlices.insert(rootLevelAncestor.m_sliceAddress);
                    }

                    AZStd::vector<FieldTreeItem*> stack;
                    for (int i = 0; i < rootFieldItem->childCount(); ++i)
                    {
                        stack.push_back(static_cast<FieldTreeItem*>(rootFieldItem->child(i)));
                    }

                    while (!stack.empty())
                    {
                        FieldTreeItem* item = stack.back();
                        stack.pop_back();

                        // Remove any items from the commonSlices that this child does not have
                        for (auto commonSliceIt = commonSlices.begin(); commonSliceIt != commonSlices.end(); )
                        {
                            bool found = false;
                            for (auto& ancestor : item->m_ancestors)
                            {
                                if (*commonSliceIt == ancestor.m_sliceAddress)
                                {
                                    found = true;
                                    break;
                                }
                            }
                            if (found)
                            {
                                ++commonSliceIt;
                            }
                            else
                            {
                                commonSliceIt = commonSlices.erase(commonSliceIt);
                            }
                        }

                        for (int i = 0; i < item->childCount(); ++i)
                        {
                            stack.push_back(static_cast<FieldTreeItem*>(item->child(i)));
                        }
                    }
                }

                // Next, cull all non-common ancestors from everyone in the hierarchy
                {
                    AZStd::vector<FieldTreeItem*> stack;
                    stack.push_back(rootFieldItem);

                    while (!stack.empty())
                    {
                        FieldTreeItem* item = stack.back();
                        stack.pop_back();

                        for (auto ancestorIt = item->m_ancestors.begin(); ancestorIt != item->m_ancestors.end(); )
                        {
                            auto foundIt = commonSlices.find(ancestorIt->m_sliceAddress);
                            if (foundIt == commonSlices.end())
                            {
                                ancestorIt = item->m_ancestors.erase(ancestorIt);
                            }
                            else
                            {
                                ++ancestorIt;
                            }
                        }

                        for (int i = 0; i < item->childCount(); ++i)
                        {
                            stack.push_back(static_cast<FieldTreeItem*>(item->child(i)));
                        }
                    }
                }
            }
        }
    }

    //=========================================================================
    // SlicePushWidget::CalculateReferenceCount
    //=========================================================================
    size_t SlicePushWidget::CalculateReferenceCount(const AZ::Data::AssetId& assetId) const
    {
        // This is an approximate measurement of how much this slice proliferates within the currently-loaded level.
        // Down the line we'll actually query the asset DB's dependency tree, summing up instances.

        AZ::SliceComponent* levelSlice = nullptr;
        EBUS_EVENT_RESULT(levelSlice, EditorEntityContextRequestBus, GetEditorRootSlice);
        size_t instanceCount = 0;
        AZ::Data::AssetBus::EnumerateHandlersId(assetId,
            [&instanceCount, assetId, levelSlice] (AZ::Data::AssetEvents* handler) -> bool
            {
                AZ::SliceComponent* component = azrtti_cast<AZ::SliceComponent*>(handler);
                if (component)
                {
                    AZ::SliceComponent::SliceReference* reference = component->GetSlice(assetId);

                    size_t count = reference->GetInstances().size();

                    // If the listener is a dependent slice, also multiple by the number of instances
                    // of that slice within the level.
                    if (levelSlice && levelSlice != component)
                    {
                        AZ::SliceComponent::SliceReference* levelReference = levelSlice->GetSlice(assetId);
                        if (levelReference)
                        {
                            count *= levelReference->GetInstances().size();
                        }
                    }

                    instanceCount += count;
                }
                else
                {
                    ++instanceCount;
                }
                return true;
            });

        return instanceCount;
    }
    
    //=========================================================================
    // SlicePushWidget::CloneAsset
    //=========================================================================
    SliceAssetPtr SlicePushWidget::CloneAsset(const SliceAssetPtr& asset) const
    {
        AZ::Entity* clonedAssetEntity = aznew AZ::Entity(asset.Get()->GetEntity()->GetId());
        AZ::SliceComponent* clonedSlice = asset.Get()->GetComponent()->Clone(*m_serializeContext);
        clonedAssetEntity->AddComponent(clonedSlice);
        SliceAssetPtr clonedAsset(aznew AZ::SliceAsset(asset.GetId()));
        clonedAsset.Get()->SetData(clonedAssetEntity, clonedSlice);
        return clonedAsset;
    }

    //=========================================================================
    // SlicePushWidget::PushSelectedFields
    //=========================================================================
    bool SlicePushWidget::PushSelectedFields()
    {
        // Identify all assets affected by the push, and check them out.
        AZStd::unordered_map<AZ::Data::AssetId, SliceAssetPtr> uniqueAffectedAssets;
        if (!CheckoutAndGatherAffectedAssets(uniqueAffectedAssets))
        {
            return false;
        }

        using namespace AzToolsFramework::SliceUtilities;

        // Start a push transaction for each asset.
        AZStd::unordered_map<AZ::Data::AssetId, SliceTransaction::TransactionPtr> transactionMap;
        for (const auto& pair : uniqueAffectedAssets)
        {
            const SliceAssetPtr& asset = pair.second;
            transactionMap[asset.GetId()] = SliceTransaction::BeginSlicePush(asset);
            if (!transactionMap[asset.GetId()])
            {
                QMessageBox::warning(this, QStringLiteral("Cannot Push to Slice"),
                                     QString("Unable to create transaction for slice asset %1").arg(asset.ToString<AZStd::string>().c_str()),
                                     QMessageBox::Ok);
                return false;
            }
        }

        QString pushError;
        EntityIdList entitiesToRemove;

        // Iterate over selected fields and synchronize data to target.
        QTreeWidgetItemIterator itemIter(m_fieldTree);
        for (; *itemIter; ++itemIter)
        {
            FieldTreeItem* item = static_cast<FieldTreeItem*>(*itemIter);
            if (item->IsPushableItem())
            {
                if (item->checkState(0) == Qt::Checked)
                {
                    SliceTransaction::TransactionPtr transaction = transactionMap[item->m_selectedAsset];

                    // Add entity
                    if (item->m_isEntityAdd)
                    {
                        AZ::EntityId parentId;
                        AZ::SliceEntityHierarchyRequestBus::EventResult(parentId, item->m_entity->GetId(), &AZ::SliceEntityHierarchyRequestBus::Events::GetSliceEntityParentId);
                        if (!parentId.IsValid())
                        {
                            pushError = QString("A new root cannot be pushed to an existing slice. New entities should be added as children or descendants of the slice root.");
                            break;
                        }

                        entitiesToRemove.push_back(item->m_entity->GetId());
                        SliceTransaction::Result result = transaction->AddEntity(item->m_entity);
                        if (!result)
                        {
                            pushError = QString("Entity \"%1\" could not be added to the target slice.\r\n\r\nDetails:\r\n%2")
                                .arg(item->m_entity->GetName().c_str()).arg(result.GetError().c_str());
                            break;
                        }
                    }
                    // Remove entity
                    else if (item->m_isEntityRemoval)
                    {
                        // Need to provide the transaction with the correct entity to remove based on target slice asset
                        AZ::Entity* removalEntity = item->m_entity;
                        for (auto& ancestor : item->m_ancestors)
                        {
                            if (ancestor.m_sliceAddress.first->GetSliceAsset().GetId() == item->m_selectedAsset)
                            {
                                removalEntity = ancestor.m_entity;
                            }
                        }

                        SliceTransaction::Result result = transaction->RemoveEntity(removalEntity);
                        if (!result)
                        {
                            pushError = QString("Entity \"%1\" could not be removed from the target slice.\r\n\r\nDetails:\r\n%2")
                                .arg(item->m_entity->GetName().c_str()).arg(result.GetError().c_str());
                            break;
                        }
                    }
                    // Update entity field
                    else
                    {
                        SliceTransaction::Result result = transaction->UpdateEntityField(item->m_entity, item->m_node->ComputeAddress());
                        if (!result)
                        {
                            pushError = QString("Unable to add entity for push.\r\n\r\nDetails:\r\n%1").arg(result.GetError().c_str());
                            break;
                        }

                        // Auto-push any hidden auto-push fields for the root entity item that need pushed
                        FieldTreeItem* entityRootItem = item->m_entityItem;
                        if (entityRootItem && !entityRootItem->m_hiddenAutoPushNodes.empty() && !entityRootItem->m_pushedHiddenAutoPushNodes)
                        {
                            // We also need to ensure we're only pushing hidden pushes to the direct slice ancestor of the entity; we don't want to
                            // be pushing things like component order to an ancestor that wouldn't have the same components
                            AZ::SliceComponent::SliceInstanceAddress sliceAddress(nullptr, nullptr);
                            EBUS_EVENT_ID_RESULT(sliceAddress, entityRootItem->m_entity->GetId(), AzFramework::EntityIdContextQueryBus, GetOwningSlice);
                            if (item->m_selectedAsset == sliceAddress.first->GetSliceAsset().GetId())
                            {
                                for (auto& hiddenAutoPushNode : entityRootItem->m_hiddenAutoPushNodes)
                                {
                                    result = transaction->UpdateEntityField(entityRootItem->m_entity, hiddenAutoPushNode->ComputeAddress());

                                    // Explicitly not warning of hidden field pushes - a user cannot act or change these failures, and we don't want
                                    // to prevent valid non-hidden pushes to fail because of something someone can't change.
                                }
                                entityRootItem->m_pushedHiddenAutoPushNodes = true;
                            }
                        }
                    }
                }
            }
        }

        if (!pushError.isEmpty())
        {
            QMessageBox::warning(this, QStringLiteral("Cannot Push to Slice"),
                                 QString("Internal during slice push. No assets have been affected.\r\n\r\nDetails:\r\n%1").arg(pushError),
                                 QMessageBox::Ok);
            return false;
        }

        // Commit all transactions.
        for (auto& transactionPair : transactionMap)
        {
            SliceTransaction::TransactionPtr& transaction = transactionPair.second;
            SliceTransaction::Result result = transaction->Commit(
                transactionPair.first,
                SliceUtilities::SlicePreSaveCallbackForWorldEntities, 
                nullptr);

            if (!result)
            {
                QMessageBox::warning(this, QStringLiteral("Cannot Push to Slice"),
                                     QString("Unable to commit changes for slice asset %1.\r\n\r\nDetails:\r\n%2")
                                     .arg(transactionPair.first.ToString<AZStd::string>().c_str()).arg(result.GetError().c_str()),
                                     QMessageBox::Ok);
            }
        }

        // Remove any entities that are now owned by other slices.
        EBUS_EVENT(AzToolsFramework::ToolsApplicationRequests::Bus, DeleteEntities, entitiesToRemove);

        return true;
    }

    //=========================================================================
    // SlicePushWidget::CheckoutAndGatherAffectedAssets
    //=========================================================================
    bool SlicePushWidget::CheckoutAndGatherAffectedAssets(AZStd::unordered_map<AZ::Data::AssetId, SliceAssetPtr>& assets)
    {
        AZ::IO::FileIOBase* fileIO = AZ::IO::FileIOBase::GetInstance();
        if (!fileIO)
        {
            AZ_Error("Slice Push", false, "File IO is not initialized.");
            return false;
        }

        auto& assetDb = AZ::Data::AssetManager::Instance();

        size_t pendingCheckouts = 0;
        bool checkoutSuccess = true;

        // Iterate over selected leaves. Identify all target slice assets, and check them out.
        QTreeWidgetItemIterator itemIter(m_fieldTree);
        for (; *itemIter; ++itemIter)
        {
            FieldTreeItem* item = static_cast<FieldTreeItem*>(*itemIter);
            if (item->IsPushableItem())
            {
                if (item->checkState(0) == Qt::Checked)
                {
                    SliceAssetPtr asset = assetDb.GetAsset<AZ::SliceAsset>(item->m_selectedAsset, false);

                    if (assets.find(asset.GetId()) != assets.end())
                    {
                        continue;
                    }

                    AZStd::string sliceAssetPath;
                    EBUS_EVENT_RESULT(sliceAssetPath, AZ::Data::AssetCatalogRequestBus, GetAssetPathById, asset.GetId());

                    if (sliceAssetPath.empty())
                    {
                        QMessageBox::warning(this, QStringLiteral("Cannot Push to Slice"), 
                            QString("Failed to resolve path for asset \"%1\".").arg(asset.GetId().ToString<AZStd::string>().c_str()), 
                            QMessageBox::Ok);

                        return false;
                    }

                    assets[asset.GetId()] = asset;

                    AZStd::string assetFullPath;
                    EBUS_EVENT(AzToolsFramework::AssetSystemRequestBus, GetFullSourcePathFromRelativeProductPath, sliceAssetPath, assetFullPath);
                    if (fileIO->IsReadOnly(assetFullPath.c_str()))
                    {
                        // Issue checkout order for each affected slice.
                        ++pendingCheckouts;

                        using ApplicationBus = AzToolsFramework::ToolsApplicationRequestBus;

                        ApplicationBus::Broadcast(&ApplicationBus::Events::RequestEditForFile,
                            assetFullPath.c_str(),
                            [&pendingCheckouts, &checkoutSuccess](bool success)
                            {
                                checkoutSuccess &= success;
                                --pendingCheckouts;
                            }
                        );
                    }
                }
            }
        }

        if (pendingCheckouts > 0)
        {
            const size_t totalAssets = assets.size();
            AzToolsFramework::ProgressShield::LegacyShowAndWait(this, this->tr("Checking out slices for edit..."),
                [&pendingCheckouts, totalAssets] (int& current, int& max)
                {
                    current = static_cast<int>(totalAssets) - static_cast<int>(pendingCheckouts);
                    max = static_cast<int>(totalAssets);
                    return pendingCheckouts == 0;
                }
            );
        }
        
        if (!checkoutSuccess)
        {
            QMessageBox::warning(this, QStringLiteral("Cannot Push to Slice"), 
                QString("Failed to check out one or more target slice files."), 
                QMessageBox::Ok);
            return false;
        }

        return (!assets.empty());
    }

    //=========================================================================
    // SlicePushWidget::eventFilter
    //=========================================================================
    bool SlicePushWidget::eventFilter(QObject* target, QEvent* event)
    {
        if (target == m_fieldTree)
        {
            // If the user hits the spacebar with a field selected, toggle its checkbox.
            if (event->type() == QEvent::KeyPress)
            {
                QKeyEvent* keyEvent = static_cast<QKeyEvent*>(event);
                if (keyEvent->key() == Qt::Key_Space)
                {
                    if (!m_fieldTree->selectedItems().isEmpty())
                    {
                        FieldTreeItem* item = static_cast<FieldTreeItem*>(m_fieldTree->selectedItems().front());
                        const Qt::CheckState currentState = item->checkState(0);
                        const Qt::CheckState newState = (currentState == Qt::CheckState::Checked) ? 
                            Qt::CheckState::Unchecked : Qt::CheckState::Checked;
                        item->setCheckState(0, newState);

                        return true; // Eat the event
                    }
                }
            }
        }
        
        return QWidget::eventFilter(target, event);
    }

    //=========================================================================
    // SlicePushWidget::AddStatusMessage
    //=========================================================================
    void SlicePushWidget::AddStatusMessage(StatusMessageType messageType, const QString& messageText)
    {
        m_statusMessages.push_back();
        AzToolsFramework::SlicePushWidget::StatusMessage& newMessage = m_statusMessages.back();
        newMessage.m_type = messageType;
        newMessage.m_text = messageText;
    }

    //=========================================================================
    // SlicePushWidget::DisplayStatusMessages
    //=========================================================================
    void SlicePushWidget::DisplayStatusMessages()
    {
        int messageCount = 0;
        for (AzToolsFramework::SlicePushWidget::StatusMessage& message : m_statusMessages)
        {
            QHBoxLayout* bottomStatusMessagesLayout = new QHBoxLayout();
            {
                bottomStatusMessagesLayout->setAlignment(Qt::AlignBottom | Qt::AlignLeft);

                QLabel* statusIconLabel = new QLabel();
                switch (message.m_type)
                {
                case StatusMessageType::Warning:
                default:
                {
                    if (!m_iconWarning.availableSizes().empty())
                    {
                        statusIconLabel->setPixmap(m_iconWarning.pixmap(m_iconWarning.availableSizes().first()));
                    }
                    break;
                }
                }
                statusIconLabel->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Maximum);
                statusIconLabel->setMargin(3);
                statusIconLabel->setAlignment(Qt::AlignCenter);

                QLabel* statusLabel = new QLabel();
                statusLabel->setText(message.m_text);
                statusLabel->setAlignment(Qt::AlignVCenter | Qt::AlignLeft);
                statusLabel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Maximum);
                statusLabel->setWordWrap(true);

                bottomStatusMessagesLayout->setAlignment(Qt::AlignBottom);

                bottomStatusMessagesLayout->addWidget(statusIconLabel);
                bottomStatusMessagesLayout->addWidget(statusLabel);
            }

            m_bottomLayout->insertLayout(messageCount++, bottomStatusMessagesLayout, 1);
        }
    }

} // namespace AzToolsFramework

#include <UI/Slice/SlicePushWidget.moc>

