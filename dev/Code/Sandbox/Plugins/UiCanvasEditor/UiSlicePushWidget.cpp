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

#include <QtWidgets/QDialog>
#include <QtWidgets/QDialogButtonBox>
#include <QtWidgets/QTreeWidget>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QHeaderView>
#include <QtWidgets/QRadioButton>
#include <QtWidgets/QSplitter>
#include <QtWidgets/QMessageBox>

#include <AzCore/Math/Transform.h>
#include <AzCore/Math/Quaternion.h>
#include <AzCore/Math/Vector3.h>
#include <AzCore/Asset/AssetManagerBus.h>
#include <AzCore/Asset/AssetManager.h>
#include <AzCore/Component/ComponentApplicationBus.h>
#include <AzCore/Serialization/Utils.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/IO/FileIO.h>

#include <AzFramework/Entity/EntityContextBus.h>
#include <AzFramework/StringFunc/StringFunc.h>
#include <AzFramework/Asset/AssetSystemBus.h>

#include <AzToolsFramework/API/EditorAssetSystemAPI.h>
#include "UiSlicePushWidget.h"
#include <AzToolsFramework/UI/PropertyEditor/InstanceDataHierarchy.h>
#include <AzToolsFramework/UI/PropertyEditor/PropertyEditorAPI.h>
#include <AzToolsFramework/UI/UICore/ProgressShield.hxx>
#include <AzToolsFramework/Slice/SliceUtilities.h>
#include "UiSliceManager.h"
#include <AzToolsFramework/Entity/EditorEntityContextBus.h>
#include "UiEditorEntityContextBus.h"

using AzToolsFramework::EntityIdList;
using AzToolsFramework::EntityIdSet;

namespace UiCanvasEditor
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

        QLabel* label = new QLabel(tr("No changes detected"), this);
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
                                                      QTreeWidget* view = nullptr, int type = QTreeWidgetItem::Type)
        {
            FieldTreeItem* item = new FieldTreeItem(static_cast<QTreeWidget*>(nullptr), type);
            item->m_entity = assetEntityToRemove;
            item->m_entityItem = item;
            item->m_isEntityRemoval = true;
            item->m_sliceAddress = sliceAddress;
            item->m_selectedAsset = targetAsset.GetId();
            view->insertTopLevelItem(0, item);
            return item;
        }

        static FieldTreeItem* CreateEntityAddItem(AZ::Entity* entity, QTreeWidget* view = nullptr, int type = QTreeWidgetItem::Type)
        {
            FieldTreeItem* item = new FieldTreeItem(static_cast<QTreeWidget*>(nullptr), type);
            item->m_entity = entity;
            item->m_entityItem = item;
            item->m_isEntityAdd = true;
            item->m_node = &item->m_hierarchy;
            view->insertTopLevelItem(0, item);
            return item;
        }

        // For (root) Entity-level items, contains the entities built IDH.
        AzToolsFramework::InstanceDataHierarchy m_hierarchy;

        // For (root) Entity-level item, points to hierarchy. For child items, points to the IDH node for the field.
        AzToolsFramework::InstanceDataNode* m_node;

        // Entity associated with this node, or the entity parent of this node.
        AZ::Entity* m_entity;

        // Set if this an entity removal from the target slice.
        bool m_isEntityRemoval;

        // Set if this a potential entity addition to an existing slice.
        bool m_isEntityAdd;

        // For entity-level items only, contains slice ancestor list.
        AZ::SliceComponent::EntityAncestorList m_ancestors;
        AZ::SliceComponent::SliceInstanceAddress m_sliceAddress;

        // Relevant for leaf items only, stores Id of target slice asset.
        AZ::Data::AssetId m_selectedAsset;

        // Points to item at root of this entity hieararchy.
        FieldTreeItem* m_entityItem;
    };

    //=========================================================================
    bool IsNodePushable(const AzToolsFramework::InstanceDataNode* node, bool isRootEntity)
    {
        AZ_Assert(node, "Invalid instance data node");

        const AZ::u32 sliceFlags = AzToolsFramework::SliceUtilities::GetNodeSliceFlags(*node);

        if (0 != (sliceFlags & AZ::Edit::SliceFlags::NotPushable))
        {
            return false;
        }

        if (isRootEntity && 0 != (sliceFlags & AZ::Edit::SliceFlags::NotPushableOnSliceRoot))
        {
            return false;
        }

        return true;
    }

    //=========================================================================
    // UiSlicePushWidget
    //=========================================================================
    UiSlicePushWidget::UiSlicePushWidget(UiSliceManager* sliceManager, const EntityIdList& entities, AZ::SerializeContext* serializeContext, QWidget* parent)
        : QWidget(parent)
        , m_sliceManager(sliceManager)
    {
        QVBoxLayout* layout = new QVBoxLayout();

        // Window layout:
        // =============================================
        // -------------------------------------------||
        // (Data tree label)    (slice tree label)    ||
        // -------------------------------------------||
        // ||                   |                     ||
        // ||              (splitter)                 ||
        // ||                   |                     ||
        // ||    (Data tree)    | (Slice target tree) ||
        // ||                   |                     ||
        // ||                   |                     ||
        // ||                   |                     ||
        // ---------------------------------------------
        //  (legend)                     (button box) ||
        // =============================================

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

        QHBoxLayout* bottomLayout = new QHBoxLayout();

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
            bottomLayout->addWidget(imageChanged, 0);
            bottomLayout->addWidget(labelChanged, 0);
            bottomLayout->addWidget(imageAdded, 0);
            bottomLayout->addWidget(labelAdded, 0);
            bottomLayout->addWidget(imageRemoved, 0);
            bottomLayout->addWidget(labelRemoved, 0);
        }

        // Create/populate button box.
        {
            QDialogButtonBox* buttonBox = new QDialogButtonBox(this);
            QPushButton* pushSelectedButton = new QPushButton(tr(" Push Selected Fields"));
            pushSelectedButton->setToolTip(tr("Push selected elements to the specified slice(es)."));
            pushSelectedButton->setDefault(false);
            pushSelectedButton->setAutoDefault(false);
            QPushButton* cancelButton = new QPushButton(tr("Cancel"));
            cancelButton->setDefault(true);
            cancelButton->setAutoDefault(true);
            buttonBox->addButton(cancelButton, QDialogButtonBox::RejectRole);
            buttonBox->addButton(pushSelectedButton, QDialogButtonBox::ApplyRole);
            connect(pushSelectedButton, &QPushButton::clicked, this, &UiSlicePushWidget::OnPushClicked);
            connect(cancelButton, &QPushButton::clicked, this, &UiSlicePushWidget::OnCancelClicked);

            bottomLayout->addWidget(buttonBox, 1);
        }
  
        // Add everything to main layout.
        layout->addWidget(splitter);
        layout->addLayout(bottomLayout);

        connect(m_fieldTree, &QTreeWidget::itemSelectionChanged, this, &UiSlicePushWidget::OnFieldSelectionChanged);
        connect(m_fieldTree, &QTreeWidget::itemChanged, this, &UiSlicePushWidget::OnFieldDataChanged);

        setLayout(layout);

        Setup(entities, serializeContext);
    }
    
    //=========================================================================
    // ~UiSlicePushWidget
    //=========================================================================
    UiSlicePushWidget::~UiSlicePushWidget()
    {
    }

    //=========================================================================
    // UiSlicePushWidget::Setup
    //=========================================================================
    void UiSlicePushWidget::Setup(const EntityIdList& entities, AZ::SerializeContext* serializeContext)
    {
        m_serializeContext = serializeContext;

        if (!m_serializeContext)
        {
            EBUS_EVENT_RESULT(m_serializeContext, AZ::ComponentApplicationBus, GetSerializeContext);
        }

        PopulateFieldTree(entities);

        // Update asset name column for all leaf (pushable) items.
        QTreeWidgetItemIterator itemIter(m_fieldTree, QTreeWidgetItemIterator::NoChildren);
        for (; *itemIter; ++itemIter)
        {
            FieldTreeItem* item = static_cast<FieldTreeItem*>(*itemIter);
        
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
            connect(overlay, &NoChangesOverlay::OnCloseClicked, this, &UiSlicePushWidget::OnCancelClicked);
        }
    }

    AZStd::vector<SliceAssetPtr> UiSlicePushWidget::GetValidTargetAssetsForField(const FieldTreeItem& item) const
    {
        AZStd::vector<SliceAssetPtr> validSliceAssets;
        validSliceAssets.reserve(m_fieldTree->model()->rowCount() * 2);

        const AZ::SliceComponent::EntityAncestorList& ancestors = item.m_entityItem->m_ancestors;
        for (auto iter = ancestors.begin(); iter != ancestors.end(); ++iter)
        {
            auto& ancestor = *iter;
            validSliceAssets.push_back(ancestor.m_sliceAddress.first->GetSliceAsset());
        }

        // If the entity doesn't yet belong to a slice instance, allow targeting any of
        // the slices in the current slice push operation.
        if (ancestors.empty())
        {
            QTreeWidgetItemIterator itemIter(m_fieldTree);
            for (; *itemIter; ++itemIter)
            {
                FieldTreeItem* item = static_cast<FieldTreeItem*>(*itemIter);
                if (!item->parent())
                {
                    for (auto iter = item->m_ancestors.begin(); iter != item->m_ancestors.end(); ++iter)
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
    // UiSlicePushWidget::OnFieldSelectionChanged
    //=========================================================================
    void UiSlicePushWidget::OnFieldSelectionChanged()
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
                    tr("Check to push field \"") + item->text(0) + tr("\" to asset \"") + assetPath.c_str() + "\".";
                sliceItem->setToolTip(0, tooltip);
                sliceItem->setToolTip(1, tooltip);

                const size_t instanceCount = CalculateReferenceCount(sliceAssetId);
                const AZStd::string instanceCountDescription = AZStd::string::format("%u", instanceCount);
                sliceItem->setText(2, instanceCountDescription.c_str());

                QRadioButton* selectButton = sliceItem->m_selectButton;
                connect(selectButton, &QRadioButton::toggled, this, [sliceAssetId, this, selectButton] (bool checked) { this->OnSliceRadioButtonChanged(selectButton, sliceAssetId, checked); });
                m_sliceTree->setItemWidget(sliceItem, 0, selectButton);

                // Auto-select the slice item associated with the currently assigned target slice Id.
                if (isLeaf && item->m_selectedAsset == sliceAssetId)
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
    // UiSlicePushWidget::OnFieldDataChanged
    //=========================================================================
    void UiSlicePushWidget::OnFieldDataChanged(QTreeWidgetItem* item, int column)
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
            FieldTreeItem* item = stack.back();
            stack.pop_back();

            item->setCheckState(0, checkState);

            for (int i = 0; i < item->childCount(); ++i)
            {
                stack.push_back(static_cast<FieldTreeItem*>(item->child(i)));
            }
        }

        m_fieldTree->blockSignals(false);
    }

    //=========================================================================
    // UiSlicePushWidget::OnSliceRadioButtonChanged
    //=========================================================================
    void UiSlicePushWidget::OnSliceRadioButtonChanged(QRadioButton* selectButton, const AZ::Data::AssetId assetId, bool checked)
    {
        (void)checked;

        // The user selected a new target slice asset. Assign to the select data item and its children.

        if (!m_fieldTree->selectedItems().isEmpty())
        {
            if (m_sliceTree->model()->rowCount() == 1)
            {
                selectButton->blockSignals(true);
                selectButton->setChecked(true);
                selectButton->blockSignals(false);
                return;
            }

            AZStd::vector<FieldTreeItem*> stack;
            stack.push_back(static_cast<FieldTreeItem*>(m_fieldTree->selectedItems().front()));

            while (!stack.empty())
            {
                FieldTreeItem* item = stack.back();
                stack.pop_back();

                if (item->childCount() == 0)
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
    // UiSlicePushWidget::OnPushClicked
    //=========================================================================
    void UiSlicePushWidget::OnPushClicked()
    {
        if (PushSelectedFields())
        {
            emit OnFinished();
        }
    }
    
    //=========================================================================
    // UiSlicePushWidget::OnCancelClicked
    //=========================================================================
    void UiSlicePushWidget::OnCancelClicked()
    {
        emit OnCanceled();
    }

    //=========================================================================
    // UiSlicePushWidget::PopulateFieldTree
    //=========================================================================
    void UiSlicePushWidget::PopulateFieldTree(const EntityIdList& entities)
    {        
        // Gather full hierarchy for analysis.
        EntityIdSet processEntityIds = m_sliceManager->GatherEntitiesAndAllDescendents(entities);

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
                if (!ancestors.empty())
                {
                    const AZ::SliceComponent::Ancestor& ancestor = ancestors.front();

                    tempCompareEntities.push_back(
                        AzToolsFramework::SliceUtilities::CloneSliceEntityForComparison(*ancestor.m_entity, *sliceAddress.second, *m_serializeContext));

                    AZ::Entity* compareClone = tempCompareEntities.back().get();
                    entityItem->m_hierarchy.AddComparisonInstance<AZ::Entity>(compareClone);
                }

                entityItem->m_hierarchy.Build(m_serializeContext, AZ::SerializeContext::ENUM_ACCESS_FOR_READ);
                entityItem->m_ancestors = AZStd::move(ancestors);
                entityItem->m_sliceAddress = sliceAddress;
                
                entityItem->setText(0, QString("%1 (changed)").arg(entity->GetName().c_str()));
                entityItem->setIcon(0, m_iconGroup);
                entityItem->setCheckState(0, Qt::CheckState::Checked);

                AZStd::unordered_map<AzToolsFramework::InstanceDataNode*, FieldTreeItem*> nodeItemMap;
                nodeItemMap[&entityItem->m_hierarchy] = entityItem;

                AZStd::vector<AzToolsFramework::InstanceDataNode*> nodeStack;
                AZStd::vector<FieldTreeItem*> itemStack;

                const bool isRootEntity = m_sliceManager->IsRootEntity(*entity);
                bool entityHasPushableChanges = false;

                //
                // Recursively traverse IDH and add all modified fields to the tree.
                //

                for (AzToolsFramework::InstanceDataNode& childNode : entityItem->m_hierarchy.GetChildren())
                {
                    nodeStack.push_back(&childNode);
                }

                while (!nodeStack.empty())
                {
                    AzToolsFramework::InstanceDataNode* node = nodeStack.back();
                    nodeStack.pop_back();

                    if (!AzToolsFramework::SliceUtilities::IsNodePushable(*node, isRootEntity))
                    {
                        continue;
                    }

                    for (AzToolsFramework::InstanceDataNode& childNode : node->GetChildren())
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
                            AzToolsFramework::NodeDisplayVisibility visibility = (walkNode == node) ? 
                                AzToolsFramework::NodeDisplayVisibility::Visible : CalculateNodeDisplayVisibility(*walkNode, true);

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

        PopulateFieldTreeAddedEntities(processEntityIds);
        PopulateFieldTreeRemovedEntities();
    }

    //=========================================================================
    // UiSlicePushWidget::PopulateFieldTreeAddedEntities
    //=========================================================================
    void UiSlicePushWidget::PopulateFieldTreeAddedEntities(const EntityIdSet& entities)
    {
        for (const AZ::EntityId& entityId : entities)
        {
            AZ::Entity* entity = nullptr;
            EBUS_EVENT_RESULT(entity, AZ::ComponentApplicationBus, FindEntity, entityId);

            if (entity)
            {
                AZ::SliceComponent::EntityAncestorList referenceAncestors;
                AZ::SliceComponent::SliceInstanceAddress sliceAddress(nullptr, nullptr);
                EBUS_EVENT_ID_RESULT(sliceAddress, entityId, AzFramework::EntityIdContextQueryBus, GetOwningSlice);

                if (sliceAddress.first)
                {
                    continue;
                }

                // Find the nearest ancestor that's part of a slice and use its ancestry.
                AZ::SliceComponent::EntityAncestorList ancestors;
                AZ::Entity* parent = nullptr;
                EBUS_EVENT_ID_RESULT(parent, entityId, UiElementBus, GetParent);
                while (parent)
                {
                    EBUS_EVENT_ID_RESULT(sliceAddress, parent->GetId(), AzFramework::EntityIdContextQueryBus, GetOwningSlice);
                    if (sliceAddress.first)
                    {
                        sliceAddress.first->GetInstanceEntityAncestry(parent->GetId(), ancestors);
                        break;
                    }
                    AZ::EntityId parentId = parent->GetId();
                    parent = nullptr;   // in case entity is not listening on bus
                    EBUS_EVENT_ID_RESULT(parent, parentId, UiElementBus, GetParent);
                }

                if (ancestors.empty())
                {
                    continue;
                }

                // Create field entry to remove this entity from the asset.
                FieldTreeItem* item = FieldTreeItem::CreateEntityAddItem(entity, m_fieldTree);
                item->setText(0, QString("%1 (added)").arg(entity->GetName().c_str()));
                item->setIcon(0, m_iconNewDataItem);
                item->m_ancestors = AZStd::move(ancestors);

                // UI_CHANGE: adds should be checked (and probably not be allowed to be unchecked)
                item->setCheckState(0, Qt::CheckState::Checked);
            }
        }
    }

    //=========================================================================
    // UiSlicePushWidget::PopulateFieldTreeRemovedEntities
    //=========================================================================
    void UiSlicePushWidget::PopulateFieldTreeRemovedEntities()
    {
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
                            uniqueRemovedEntities.insert(assetEntityId);

                            // Create field entry to remove this entity from the asset.
                            FieldTreeItem* item = FieldTreeItem::CreateEntityRemovalItem(assetEntity, instanceAddr.first->GetSliceAsset(), instanceAddr, m_fieldTree);
                            item->setText(0, QString("%1 (deleted)").arg(assetEntity->GetName().c_str()));
                            item->setIcon(0, m_iconRemovedDataItem);

                            // Grab ancestors, which determines which slices the removal can be pushed to.
                            instanceAddr.first->GetInstanceEntityAncestry(assetEntity->GetId(), item->m_ancestors);

                            // By default, removals are checked for push (different from main window)
                            // This is to keep them in sync with the Children container in parent
                            item->setCheckState(0, Qt::CheckState::Checked);
                        }
                    }
                }
            }
        }
    }

    //=========================================================================
    // UiSlicePushWidget::CalculateReferenceCount
    //=========================================================================
    size_t UiSlicePushWidget::CalculateReferenceCount(const AZ::Data::AssetId& assetId) const
    {
        // This is an approximate measurement of how much this slice proliferates within the currently-loaded level.
        // Down the line we'll actually query the asset DB's dependency tree, summing up instances.

        AZ::SliceComponent* levelSlice = m_sliceManager->GetRootSlice();
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
    // UiSlicePushWidget::CloneAsset
    //=========================================================================
    SliceAssetPtr UiSlicePushWidget::CloneAsset(const SliceAssetPtr& asset) const
    {
        AZ::Entity* clonedAssetEntity = aznew AZ::Entity();
        AZ::SliceComponent* clonedSlice = asset.Get()->GetComponent()->Clone(*m_serializeContext);
        clonedAssetEntity->AddComponent(clonedSlice);
        SliceAssetPtr clonedAsset(aznew AZ::SliceAsset(asset.GetId()));
        clonedAsset.Get()->SetData(clonedAssetEntity, clonedSlice);
        return clonedAsset;
    }

    //=========================================================================
    // UiSlicePushWidget::PushSelectedFields
    //=========================================================================
    bool UiSlicePushWidget::PushSelectedFields()
    {
        // Identify all assets affected by the push, and check them out.
        SliceAssetMap uniqueAffectedAssets;
        if (!CheckoutAndGatherAffectedAssets(uniqueAffectedAssets))
        {
            return false;
        }

        // Clone every target slice asset. We push changes to the cloned assets/entities.
        SliceAssetMap clonedAssets;
        for (const auto& pair : uniqueAffectedAssets)
        {
            const SliceAssetPtr& asset = pair.second;
            clonedAssets[asset.GetId()] = CloneAsset(asset);
        }

        using EntityIdMap = AZStd::unordered_map<AZ::EntityId, AZ::EntityId>;
        using AssetTargetIdMap = AZStd::unordered_map<AZ::Data::AssetId, EntityIdMap>;
        AssetTargetIdMap assetLiveToTargetIdMap;
        EntityIdList removeLooseEntities;
        AZ::SliceComponent::EntityAncestorList sourceAncestors;
        AZStd::set<const AzToolsFramework::InstanceDataNode*> nodesPushed;

        QString pushError;

        for (const AZ::SliceComponent::SliceInstanceAddress& instance : m_sliceInstances)
        {
            EntityIdMap& liveToTargetIdMap = assetLiveToTargetIdMap[instance.first->GetSliceAsset().GetId()];
            liveToTargetIdMap = instance.second->GetEntityIdToBaseMap();
        }

        // Iterate over selected fields and synchronize data to target.
        QTreeWidgetItemIterator itemIter(m_fieldTree, QTreeWidgetItemIterator::NoChildren);
        for (; *itemIter; ++itemIter)
        {
            FieldTreeItem* item = static_cast<FieldTreeItem*>(*itemIter);

            if (item->checkState(0) == Qt::Checked)
            {
                AZ::Entity* targetEntity = nullptr;

                EntityIdMap& liveToTargetIdMap = assetLiveToTargetIdMap[item->m_selectedAsset];

                // If the entity is loose and being added to a slice, we simply clone & add it the cloned slice.
                // Note: New entities being added to the slice cannot be designated as the root.
                if (item->m_isEntityAdd)
                {
                    // We know that this is a UI element so we can use the UiElementInterface to get the parent
                    if (UiElementInterface* element = UiElementBus::FindFirstHandler(item->m_entity->GetId()))
                    {
                        bool isElementTopLevel = false;
                        AZ::Entity* parent = element->GetParent();
                        if (!parent)
                        {
                            isElementTopLevel = true;   // this is actually the canvas root element which should never happen
                        }
                        else
                        {
                            // if the parent has a null parent ptr then it is the rootElement which makes this a top level element
                            UiElementInterface* parentElement = UiElementBus::FindFirstHandler(parent->GetId());
                            AZ_Assert(parentElement, "The parent entity must have an element component.");

                            if (!parentElement->GetParent())
                            {
                                isElementTopLevel = true;
                            }
                        }

                        if (isElementTopLevel)
                        {
                            pushError = QString("A new root cannot be pushed to an existing slice. New entities should be added as children or descendents of the slice root.");
                            break;
                        }
                    }

                    AZ::Entity* clonedEntity = m_serializeContext->CloneObject(item->m_entity);
                    clonedEntity->SetId(AZ::Entity::MakeId());

                    clonedAssets[item->m_selectedAsset].Get()->GetComponent()->AddEntity(clonedEntity);
                    liveToTargetIdMap[item->m_entity->GetId()] = clonedEntity->GetId();
                    removeLooseEntities.push_back(item->m_entity->GetId());

                    continue;
                }

                // Otherwise, locate the target entity in the slice asset.
                targetEntity = UiSliceManager::FindAncestorInTargetSlice(clonedAssets[item->m_selectedAsset], item->m_entityItem->m_ancestors);

                // If this is a removal push, pluck the entity from the target slice.
                if (item->m_isEntityRemoval)
                {
                    if (!targetEntity)
                    {
                        targetEntity = item->m_entity;
                    }

                    if (!clonedAssets[item->m_selectedAsset].Get()->GetComponent()->RemoveEntity(targetEntity))
                    {
                        pushError = QString("Failed to remove entity \"%1\" [%llu] from slice.")
                            .arg(GetNodeDisplayName(*item->m_node).c_str())
                            .arg(static_cast<AZ::u64>(targetEntity->GetId()));
                        break;
                    }

                    continue;
                }

                // Check that a valid target entity was found
                if (!targetEntity)
                {
                    AZStd::string nodeDisplayName;
                    if (item->m_node)
                    {
                        nodeDisplayName = GetNodeDisplayName(*item->m_node);
                    }
                    pushError = QString("Failed to locate target entity in slice for \"%1.\"").arg(nodeDisplayName.c_str());
                    break;
                }

                // Generate target IDH.
                AzToolsFramework::InstanceDataHierarchy* targetHierarchy = new AzToolsFramework::InstanceDataHierarchy();
                targetHierarchy->AddRootInstance<AZ::Entity>(targetEntity);
                targetHierarchy->Build(m_serializeContext, AZ::SerializeContext::ENUM_ACCESS_FOR_READ);

                // For new or removed elements in a container, patch the parent.
                AzToolsFramework::InstanceDataNode* dataNode = item->m_node;
                while (dataNode->IsNewVersusComparison() || dataNode->IsRemovedVersusComparison())
                {
                    dataNode = dataNode->GetParent();
                }

                //  Find field in source hierarchy.
                const AzToolsFramework::InstanceDataHierarchy::Address nodeAddress = dataNode->ComputeAddress();
                const AzToolsFramework::InstanceDataNode* sourceNode = item->m_entityItem->m_hierarchy.FindNodeByAddress(nodeAddress);

                //  Find field in target hierarchy.
                AzToolsFramework::InstanceDataNode* targetNode = targetHierarchy->FindNodeByAddress(nodeAddress);

                // Skip duplicate pushes to the same nodes. Multiple fields may've resolved to the same parent node.
                if (nodesPushed.find(targetNode) != nodesPushed.end())
                {
                    continue;
                }

                if (!sourceNode)
                {
                    pushError = QString("Failed to resolve source data node for \"%1.\"").arg(GetNodeDisplayName(*item->m_node).c_str());
                    break;
                }
                if (!targetNode)
                {
                    pushError = QString("Failed to resolve target data node for \"%1.\"").arg(GetNodeDisplayName(*item->m_node).c_str());
                    break;
                }

                nodesPushed.insert(sourceNode);

                // Push source instance data to target instance.
                bool copyResult = AzToolsFramework::InstanceDataHierarchy::CopyInstanceData(sourceNode, targetNode, m_serializeContext);
                if (!copyResult)
                {
                    pushError = QString("Failed to copy instance data for \"%1.\"").arg(GetNodeDisplayName(*item->m_node).c_str());
                    break;
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

        // Conduct final preparation on all affected assets. If any fails, the whole push fails.
        for (auto& pair : clonedAssets)
        {
            SliceAssetPtr& asset = pair.second;
            const EntityIdMap& liveToTargetIdMap = assetLiveToTargetIdMap[pair.first];

            // Patch entity Ids back to asset's original values.
            AZ::SliceComponent::InstantiatedContainer assetEntities;
            asset.Get()->GetComponent()->GetEntities(assetEntities.m_entities);
            AZ::EntityUtils::ReplaceEntityIdsAndEntityRefs(&assetEntities,
                                                           [&liveToTargetIdMap](const AZ::EntityId& originalId, bool /*isEntityId*/) -> AZ::EntityId
            {
                auto findIter = liveToTargetIdMap.find(originalId);
                if (findIter != liveToTargetIdMap.end())
                {
                    return findIter->second;
                }

                return originalId;
            },
                m_serializeContext);
            assetEntities.m_entities.clear();

            if (!m_sliceManager->RootEntityTransforms(asset, liveToTargetIdMap))
            {
                AZStd::string sliceAssetPath;
                EBUS_EVENT_RESULT(sliceAssetPath, AZ::Data::AssetCatalogRequestBus, GetAssetPathById, asset.GetId());
                QMessageBox::warning(this, QStringLiteral("Cannot Push to Slice"),
                                     QString(tr("Failed to resolve path for slice asset \"%1\". Aborting slice push. No assets have been affected.")).arg(sliceAssetPath.c_str()),
                                     QMessageBox::Ok);
                return false;
            }
        }

        // Before actually changing anything on disk do some validation tests on the cloned assets now that all of the
        // references have been fixed up.
        // If they fail a message box will be displayed and when it is closed the Push to Slice dialog will remain up
        // so that errors may be corrected.
        ValidateWhenPushPressed(uniqueAffectedAssets, clonedAssets, pushError);
        if (!pushError.isEmpty())
        {
            QMessageBox::warning(this, QStringLiteral("Cannot Push to Slice"),
                                 QString("Internal during slice push. No assets have been affected.\r\n\r\nDetails:\r\n%1").arg(pushError),
                                 QMessageBox::Ok);
            return false;
        }

        // Finally, write every modified asset to file.
        for (auto& pair : clonedAssets)
        {
            SliceAssetPtr& asset = pair.second;

            AZStd::string sliceAssetPath;
            EBUS_EVENT_RESULT(sliceAssetPath, AZ::Data::AssetCatalogRequestBus, GetAssetPathById, asset.GetId());
            if (sliceAssetPath.empty())
            {
                QMessageBox::warning(this, QStringLiteral("Cannot Push to Slice"),
                                     QString(tr("Failed to resolve path for slice asset %1. Aborting slice push. No assets have been affected.")).arg(asset.ToString<AZStd::string>().c_str()),
                                     QMessageBox::Ok);
                return false;
            }

            AZStd::string fullPath;
            bool fullPathFound = false;
            using AssetSysReqBus = AzToolsFramework::AssetSystemRequestBus;
            AssetSysReqBus::BroadcastResult(fullPathFound, &AssetSysReqBus::Events::GetFullSourcePathFromRelativeProductPath, sliceAssetPath, fullPath);

            if (!fullPathFound)
            {
                QMessageBox::warning(this, QStringLiteral("Cannot Push to Slice"),
                                     QString(tr("Failed to resolve path for slice asset %1. Aborting slice push. No assets have been affected.")).arg(sliceAssetPath.c_str()),
                                     QMessageBox::Ok);
                return false;
            }
            
            if (!UiSliceManager::SaveSlice(asset, fullPath.c_str(), m_serializeContext))
            {
                QMessageBox::warning(this, QStringLiteral("Cannot Push to Slice"),
                                     QString(tr("Failed to write to path \"%1\". Aborting slice push. No assets have been affected.")).arg(sliceAssetPath.c_str()),
                                     QMessageBox::Ok);
                return false;
            }
        }

        // Remove any loose entities that are now owned by slices.
        // It is important that we use DestroyElement to do this so that they remove themselves from their
        // parents m_children array. DeleteElements does this via an undoable command.
        EBUS_EVENT_ID(m_sliceManager->GetEntityContextId(), UiEditorEntityContextRequestBus, DeleteElements, removeLooseEntities);

        return true;
    }
    //=========================================================================
    // UiSlicePushWidget::CheckoutAndGatherAffectedAssets
    //=========================================================================
    bool UiSlicePushWidget::CheckoutAndGatherAffectedAssets(AZStd::unordered_map<AZ::Data::AssetId, SliceAssetPtr>& assets)
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
        QTreeWidgetItemIterator itemIter(m_fieldTree, QTreeWidgetItemIterator::NoChildren);
        for (; *itemIter; ++itemIter)
        {
            FieldTreeItem* item = static_cast<FieldTreeItem*>(*itemIter);

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
                        QString("Failed to resolve path for asset %1").arg(asset.ToString<AZStd::string>().c_str()), 
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

                    using SCCommandBus = AzToolsFramework::SourceControlCommandBus;
                    SCCommandBus::Broadcast(&SCCommandBus::Events::RequestEdit, assetFullPath.c_str(), true,
                        [&pendingCheckouts, &checkoutSuccess] (bool success, const AzToolsFramework::SourceControlFileInfo& info)
                        {
                            checkoutSuccess &= (success || !info.IsReadOnly());
                            --pendingCheckouts;
                        }
                    );
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

        if (assets.empty())
        {
            QMessageBox::warning(this, QStringLiteral("Cannot Push to Slice"), 
                QString("There were no changes to push."), 
                QMessageBox::Ok);
            return false;
        }

        return true;
    }

    //=========================================================================
    // UiSlicePushWidget::eventFilter
    //=========================================================================
    bool UiSlicePushWidget::eventFilter(QObject* target, QEvent* event)
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
    // UiSlicePushWidget::ValidateWhenPushPressed
    //=========================================================================
    void UiSlicePushWidget::ValidateWhenPushPressed(SliceAssetMap& currentAssets, SliceAssetMap& clonedAssets, QString& pushError)
    {
        // we want to ensure that "bad" data never gets pushed to a slice
        // This mostly relates to the m_children array since this is something that
        // the UI Editor manages closely and requires to be consistent.

        for (auto& pair : clonedAssets)
        {
            SliceAssetPtr& clonedAsset = pair.second;
            SliceAssetPtr& currentAsset = currentAssets[pair.first];

            AZ::SliceComponent* clonedSliceComponent = clonedAsset.Get()->GetComponent();
            AZ::SliceComponent* currentSliceComponent = currentAsset.Get()->GetComponent();

            AZ::SliceComponent::EntityList clonedEntities;
            clonedSliceComponent->GetEntities(clonedEntities);

            AZ::SliceComponent::EntityList currentEntities;
            currentSliceComponent->GetEntities(currentEntities);

            AZStd::unordered_set<AZ::EntityId> referencedEntities;
            AZStd::unordered_set<AZ::EntityId> referencedChildEntities;
            AZStd::unordered_set<AZ::EntityId> clonedEntityIds;
            AZStd::unordered_set<AZ::EntityId> addedEntities;

            for (auto clonedEntity : clonedEntities)
            {
                clonedEntityIds.insert(clonedEntity->GetId());

                auto iter = AZStd::find_if(currentEntities.begin(), currentEntities.end(),
                    [clonedEntity](AZ::Entity* entity) -> bool
                    {
                        return entity->GetId() == clonedEntity->GetId();
                    });

                if (iter == currentEntities.end())
                {
                    // this clonedEntity is an addition to the slice
                    addedEntities.insert(clonedEntity->GetId());
                }

                AZ::EntityUtils::EnumerateEntityIds(clonedEntity,
                    [&referencedEntities, &referencedChildEntities]
                    (const AZ::EntityId& id, bool isEntityId, const AZ::SerializeContext::ClassElement* elementData) -> void
                    {
                        if (!isEntityId && id.IsValid())
                        {
                            // Include this id.
                            referencedEntities.insert(id);

                            // Check if this is a child reference
                            // Unfortunately that is hard because the elementData is for the EntityId field within the m_children
                            // container and that has no useful editData and thus no attributes.
                            // For now we just assume that any elementData named "element" is a member of a container and
                            // that it is the m_children container.
                            if (elementData && !elementData->m_editData)
                            {
                                if (strcmp(elementData->m_name, "element") == 0)
                                {
                                    referencedChildEntities.insert(id);
                                }
                            }
                        }
                    }, m_serializeContext);
            }

            // Issue a warning if any referenced entities are not in the slice being created
            for (auto entityId : referencedEntities)
            {
                if (clonedEntityIds.count(entityId) == 0)
                {
                    AZStd::string name = EntityHelpers::GetHierarchicalElementName(entityId);
                    pushError = QString("There are external references."
                        "Entities in the slice being pushed reference other entities that will not be in the slice after the push."
                        "Referenced entity is '%1'.").arg(name.c_str());
                    return;
                }
            }

            // Issue a warning if there are any added entities that are not referenced as children of entities in the slice
            for (auto entityId : addedEntities)
            {
                if (referencedChildEntities.count(entityId) == 0)
                {
                    AZStd::string name = EntityHelpers::GetHierarchicalElementName(entityId);
                    pushError = QString("There are added entities that are unreferenced. "
                        "An entity is being added to the slice but it is not referenced as "
                        "the child of another entity in the slice."
                        "The added entity that is unreferenced is '%1'.").arg(name.c_str());
                    return;
                }
            }

            // Check for any entites in the slice that have become orphaned. This can happen is a remove if pushed
            // but the entity removal is unchacked while the removal from the m_children array is checked
            int parentlessEntityCount = 0;
            for (auto entityId : clonedEntityIds)
            {
                if (referencedChildEntities.count(entityId) == 0)
                {
                    // this entity is not a child of any entity
                    ++parentlessEntityCount;
                }
            }

            // There can only be one "root" entity in a slice - i.e. one entity which is not referenced as a child of another
            // entity in the slice
            if (parentlessEntityCount > 1)
            {
                pushError = QString("There is more than one root entity. "
                    "Possibly a child reference is being removed in this push but the child entity is not.");
                return;
            }
        }
    }

} // namespace AzToolsFramework

#include <UiSlicePushWidget.moc>

