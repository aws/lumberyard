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
#include "EntityPropertyEditor.hxx"

#include <AzCore/Asset/AssetManagerBus.h>
#include <AzCore/Asset/AssetTypeInfoBus.h>
#include <AzCore/Component/ComponentApplication.h>
#include <AzCore/Component/ComponentApplicationBus.h>
#include <AzCore/Component/Entity.h>
#include <AzCore/Component/TransformBus.h>
#include <AzCore/Debug/Profiler.h>
#include <AzCore/IO/FileIO.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/Utils.h>
#include <AzCore/Slice/SliceComponent.h>
#include <AzCore/UserSettings/UserSettings.h>
#include <AzCore/std/smart_ptr/unique_ptr.h>
#include <AzCore/std/sort.h>

#include <AzFramework/Entity/EntityContextBus.h>
#include <AzFramework/StringFunc/StringFunc.h>
#include <AzToolsFramework/API/EntityCompositionRequestBus.h>
#include <AzToolsFramework/API/ToolsApplicationAPI.h>
#include <AzToolsFramework/AssetBrowser/AssetBrowserBus.h>
#include <AzToolsFramework/AssetBrowser/AssetBrowserEntry.h>
#include <AzToolsFramework/AssetBrowser/AssetSelectionModel.h>
#include <AzToolsFramework/AssetBrowser/EBusFindAssetTypeByName.h>
#include <AzToolsFramework/Entity/EditorEntityHelpers.h>
#include <AzToolsFramework/Metrics/LyEditorMetricsBus.h>
#include <AzToolsFramework/Slice/SliceDataFlagsCommand.h>
#include <AzToolsFramework/Slice/SliceUtilities.h>
#include <AzToolsFramework/ToolsComponents/ComponentAssetMimeDataContainer.h>
#include <AzToolsFramework/ToolsComponents/ComponentMimeData.h>
#include <AzToolsFramework/ToolsComponents/EditorComponentBase.h>
#include <AzToolsFramework/ToolsComponents/EditorDisabledCompositionBus.h>
#include <AzToolsFramework/ToolsComponents/EditorEntityIconComponentBus.h>
#include <AzToolsFramework/ToolsComponents/EditorInspectorComponentBus.h>
#include <AzToolsFramework/ToolsComponents/EditorPendingCompositionBus.h>
#include <AzToolsFramework/ToolsComponents/GenericComponentWrapper.h>
#include <AzToolsFramework/ToolsMessaging/EntityHighlightBus.h>
#include <AzToolsFramework/UI/ComponentPalette/ComponentPaletteWidget.hxx>
#include <AzToolsFramework/UI/PropertyEditor/ComponentEditor.hxx>
#include <AzToolsFramework/UI/PropertyEditor/InstanceDataHierarchy.h>
#include <AzToolsFramework/UI/PropertyEditor/PropertyEditorAPI.h>
#include <AzToolsFramework/UI/PropertyEditor/PropertyRowWidget.hxx>
#include <AzToolsFramework/UI/PropertyEditor/ReflectedPropertyEditor.hxx>
#include <AzToolsFramework/Undo/UndoSystem.h>

#include <QDrag>
#include <QInputDialog>
#include <QLabel>
#include <QMainWindow>
#include <QMenu>
#include <QMessageBox>
#include <QMimeData>
#include <QPainter>
#include <QPixmap>
#include <QScrollBar>
#include <QTimer>
#include <QContextMenuEvent>
#include <QDragEnterEvent>
#include <QDragMoveEvent>
#include <QMouseEvent>

#include <UI/PropertyEditor/ui_EntityPropertyEditor.h>

namespace AzToolsFramework
{
    static const char* kComponentEditorIndexMimeType = "editor/componentEditorIndices";

    //since component editors are spaced apart to make room for drop indicator,
    //giving drop logic simple buffer so drops between editors don't go to the bottom
    static const int kComponentEditorDropTargetPrecision = 6;

    //we require an overlay widget to act as a canvas to draw on top of everything in the inspector
    //attaching to inspector rather than component editors so we can draw outside of bounds
    class EntityPropertyEditorOverlay : public QWidget
    {
    public:
        EntityPropertyEditorOverlay(EntityPropertyEditor* editor, QWidget* parent)
            : QWidget(parent)
            , m_editor(editor)
            , m_edgeRadius(3)
            , m_dropIndicatorOffset(7)
            , m_dropIndicatorColor("#999999")
            , m_dragIndicatorColor("#E57829")
            , m_selectIndicatorColor("#E57829")
        {
            setPalette(Qt::transparent);
            setWindowFlags(Qt::FramelessWindowHint);
            setAttribute(Qt::WA_NoSystemBackground);
            setAttribute(Qt::WA_TranslucentBackground);
            setAttribute(Qt::WA_TransparentForMouseEvents);
        }

    protected:
        void paintEvent(QPaintEvent* event) override
        {
            QWidget::paintEvent(event);

            QPainter painter(this);
            painter.setCompositionMode(QPainter::CompositionMode_SourceOver);

            QRect currRect;
            bool drag = false;
            bool drop = false;

            for (auto componentEditor : m_editor->m_componentEditors)
            {
                if (!componentEditor->isVisible())
                {
                    continue;
                }

                QRect globalRect = m_editor->GetWidgetGlobalRect(componentEditor);

                currRect = QRect(mapFromGlobal(globalRect.topLeft()), mapFromGlobal(globalRect.bottomRight()));
                currRect.setWidth(currRect.width() - 1);
                currRect.setHeight(currRect.height() - 1);

                if (componentEditor->IsDragged())
                {
                    drawDragIndicator(painter, currRect);
                    drag = true;
                }

                if (componentEditor->IsDropTarget())
                {
                    drawDropIndicator(painter, currRect.left(), currRect.right(), currRect.top() - m_dropIndicatorOffset);
                    drop = true;
                }

                if (componentEditor->IsSelected())
                {
                    drawSelectIndicator(painter, currRect);
                }
            }

            if (drag && !drop)
            {
                drawDropIndicator(painter, currRect.left(), currRect.right(), currRect.bottom() + m_dropIndicatorOffset);
            }
        }

    private:
        void drawDragIndicator(QPainter& painter, const QRect& currRect)
        {
            painter.save();
            painter.setBrush(QBrush(m_dropIndicatorColor));
            painter.drawRoundedRect(currRect, m_edgeRadius, m_edgeRadius);
            painter.restore();
        }

        void drawDropIndicator(QPainter& painter, int x1, int x2, int y)
        {
            painter.save();
            painter.setPen(QPen(m_dragIndicatorColor, 1));
            painter.drawEllipse(QPoint(x1 + m_edgeRadius, y), m_edgeRadius, m_edgeRadius);
            painter.drawLine(QPoint(x1 + m_edgeRadius * 2, y), QPoint(x2 - m_edgeRadius * 2, y));
            painter.drawEllipse(QPoint(x2 - m_edgeRadius, y), m_edgeRadius, m_edgeRadius);
            painter.restore();
        }

        void drawSelectIndicator(QPainter& painter, const QRect& currRect)
        {
            painter.save();
            painter.setPen(QPen(m_selectIndicatorColor, 1));
            painter.drawRoundedRect(currRect, m_edgeRadius, m_edgeRadius);
            painter.restore();
        }

        EntityPropertyEditor* m_editor;
        int m_edgeRadius;
        int m_dropIndicatorOffset;
        QColor m_dragIndicatorColor;
        QColor m_dropIndicatorColor;
        QColor m_selectIndicatorColor;
    };

    EntityPropertyEditor::SharedComponentInfo::SharedComponentInfo(AZ::Component* component, AZ::Component* sliceReferenceComponent)
        : m_sliceReferenceComponent(sliceReferenceComponent)
    {
        m_instances.push_back(component);
    }

    EntityPropertyEditor::EntityPropertyEditor(QWidget* pParent, Qt::WindowFlags flags)
        : QWidget(pParent, flags)
        , m_propertyEditBusy(0)
        , m_componentFilter(AppearsInGameComponentMenu)
        , m_componentPalette(nullptr)
        , m_autoScrollCount(0)
        , m_autoScrollMargin(16)
        , m_autoScrollQueued(false)
        , m_isSystemEntityEditor(false)
    {
        setObjectName("EntityPropertyEditor");
        setAcceptDrops(true);

        m_isAlreadyQueuedRefresh = false;
        m_shouldScrollToNewComponents = false;
        m_shouldScrollToNewComponentsQueued = false;
        m_currentUndoNode = nullptr;
        m_currentUndoOperation = nullptr;
        m_selectionEventAccepted = false;

        m_componentEditorLastSelectedIndex = -1;
        m_componentEditorsUsed = 0;
        m_componentEditors.clear();
        m_componentToEditorMap.clear();

        m_gui = aznew Ui::EntityPropertyEditorUI();
        m_gui->setupUi(this);
        m_gui->m_entityNameEditor->setReadOnly(false);
        m_gui->m_entityDetailsLabel->setObjectName("LabelEntityDetails");
        m_gui->m_entitySearchBox->setReadOnly(false);
        EnableEditor(true);
        m_sceneIsNew = true;

        connect(m_gui->m_entityIcon, &QPushButton::clicked, this, &EntityPropertyEditor::BuildEntityIconMenu);
        connect(m_gui->m_addComponentButton, &QPushButton::clicked, this, &EntityPropertyEditor::OnAddComponent);
        connect(m_gui->m_entitySearchBox, &QLineEdit::textChanged, this, &EntityPropertyEditor::OnSearchTextChanged);
        connect(m_gui->m_buttonClearFilter, &QPushButton::clicked, this, &EntityPropertyEditor::ClearSearchFilter);
        connect(m_gui->m_pinButton, &QPushButton::clicked, this, &EntityPropertyEditor::OpenPinnedInspector);

        m_componentPalette = aznew ComponentPaletteWidget(this, true);
        connect(m_componentPalette, &ComponentPaletteWidget::OnAddComponentEnd, this, [this]()
        {
            QueuePropertyRefresh();
            m_shouldScrollToNewComponents = true;
        });

        HideComponentPalette();

        m_overlay = aznew EntityPropertyEditorOverlay(this, m_gui->m_componentListContents);
        UpdateOverlay();

        ToolsApplicationEvents::Bus::Handler::BusConnect();
        AZ::EntitySystemBus::Handler::BusConnect();

        m_spacer = nullptr;

        m_emptyIcon = QIcon();
        m_clearIcon = QIcon(":/AssetBrowser/Resources/close.png");

        m_serializeContext = nullptr;
        AZ::ComponentApplicationBus::BroadcastResult(m_serializeContext, &AZ::ComponentApplicationRequests::GetSerializeContext);
        AZ_Assert(m_serializeContext, "Failed to acquire application serialize context.");

        m_isBuildingProperties = false;

        setTabOrder(m_gui->m_entityNameEditor, m_gui->m_initiallyActiveCheckbox);
        setTabOrder(m_gui->m_initiallyActiveCheckbox, m_gui->m_addComponentButton);

        // the world editor has a fixed id:

        connect(m_gui->m_entityNameEditor,
            SIGNAL(editingFinished()),
            this,
            SLOT(OnEntityNameChanged()));

        connect(m_gui->m_initiallyActiveCheckbox,
            SIGNAL(stateChanged(int)),
            this,
            SLOT(OnInitiallyActiveChanged(int)));

        CreateActions();
        UpdateContents();

        EditorEntityContextNotificationBus::Handler::BusConnect();

        //forced to register global event filter with application for selection
        //mousePressEvent and other handlers won't trigger if event is accepted by a child widget like a QLineEdit
        //TODO explore other options to avoid referencing qApp and filtering all events even though research says
        //this is the way to do it without overriding or registering with all child widgets
        qApp->installEventFilter(this);
    }

    EntityPropertyEditor::~EntityPropertyEditor()
    {
        qApp->removeEventFilter(this);

        ToolsApplicationEvents::Bus::Handler::BusDisconnect();
        AZ::EntitySystemBus::Handler::BusDisconnect();
        EditorEntityContextNotificationBus::Handler::BusDisconnect();

        for (auto& entityId : m_overrideSelectedEntityIds)
        {
            DisconnectFromEntityBuses(entityId);
        }

        delete m_gui;
    }

    void EntityPropertyEditor::GetSelectedEntities(EntityIdList& selectedEntityIds)
    {
        if (IsLockedToSpecificEntities())
        {
            // Only include entities that currently exist
            selectedEntityIds.clear();
            selectedEntityIds.reserve(m_overrideSelectedEntityIds.size());
            for (AZ::EntityId entityId : m_overrideSelectedEntityIds)
            {
                AZ::Entity* entity = nullptr;
                AZ::ComponentApplicationBus::BroadcastResult(entity, &AZ::ComponentApplicationBus::Events::FindEntity, entityId);
                if (entity)
                {
                    selectedEntityIds.emplace_back(entityId);
                }
            }
        }
        else
        {
            ToolsApplicationRequests::Bus::BroadcastResult(selectedEntityIds, &ToolsApplicationRequests::GetSelectedEntities);
        }
    }

    void EntityPropertyEditor::SetOverrideEntityIds(const AzToolsFramework::EntityIdList& entities)
    {
        m_overrideSelectedEntityIds = entities;

        for (auto& entityId : m_overrideSelectedEntityIds)
        {
            ConnectToEntityBuses(entityId);
        }

        m_gui->m_pinButton->setVisible(m_overrideSelectedEntityIds.size() == 0);
    }

    void EntityPropertyEditor::BeforeEntitySelectionChanged()
    {
        AZ_PROFILE_FUNCTION(AZ::Debug::ProfileCategory::AzToolsFramework);
        if (IsLockedToSpecificEntities())
        {
            return;
        }

        m_lastSelectedEntityIds.clear();
        ToolsApplicationRequests::Bus::BroadcastResult(m_lastSelectedEntityIds, &ToolsApplicationRequests::GetSelectedEntities);

        ClearComponentEditorDragging();
        ClearComponentEditorSelection();
        ClearComponentEditorState();
    }

    void EntityPropertyEditor::AfterEntitySelectionChanged()
    {
        AZ_PROFILE_FUNCTION(AZ::Debug::ProfileCategory::AzToolsFramework);
        if (IsLockedToSpecificEntities())
        {
            return;
        }

        EntityIdList selectedEntityIds;
        GetSelectedEntities(selectedEntityIds);

        EntityIdList selectionRemoved = m_lastSelectedEntityIds;

        for (AZ::EntityId& selectedEntityId : selectedEntityIds)
        {
            auto it = AZStd::find(selectionRemoved.begin(), selectionRemoved.end(), selectedEntityId);
            if (it != selectionRemoved.end())
            {
                selectionRemoved.erase(it);
            }
            else
            {
                // This is a new entity, so need to connect to its notification bus for property updates
                ConnectToEntityBuses(selectedEntityId);
            }
        }

        // Need to disconnect for notifications from entities that are no longer selected
        for (AZ::EntityId& entityId : selectionRemoved)
        {
            DisconnectFromEntityBuses(entityId);
        }

        if (m_lastSelectedEntityIds != selectedEntityIds)
        {
            ClearSearchFilter();
        }

        if (selectedEntityIds.empty())
        {
            // Ensure a prompt refresh when all entities have been removed/deselected.
            UpdateContents();
            return;
        }

        // when entity selection changed, we need to repopulate our GUI
        QueuePropertyRefresh();
    }

    void EntityPropertyEditor::OnEntityInitialized(const AZ::EntityId& entityId)
    {
        if (IsLockedToSpecificEntities())
        {
            // During slice reloading, an entity can be deregistered, then registered again.
            // Refresh if the editor had been locked to that entity.
            if (AZStd::find(m_overrideSelectedEntityIds.begin(), m_overrideSelectedEntityIds.end(), entityId) != m_overrideSelectedEntityIds.end())
            {
                QueuePropertyRefresh();
            }
        }
    }

    void EntityPropertyEditor::OnEntityDestroyed(const AZ::EntityId& entityId)
    {
        if (IsEntitySelected(entityId))
        {
            UpdateContents(); // immediately refresh
        }
    }

    void EntityPropertyEditor::OnEntityActivated(const AZ::EntityId& entityId)
    {
        if (IsEntitySelected(entityId))
        {
            QueuePropertyRefresh();
        }
    }

    void EntityPropertyEditor::OnEntityDeactivated(const AZ::EntityId& entityId)
    {
        if (IsEntitySelected(entityId))
        {
            QueuePropertyRefresh();
        }
    }

    void EntityPropertyEditor::OnEntityNameChanged(const AZ::EntityId& entityId, const AZStd::string& name)
    {
        if (IsSingleEntitySelected(entityId))
        {
            m_gui->m_entityNameEditor->setText(QString(name.c_str()));
        }
    }

    bool EntityPropertyEditor::IsEntitySelected(const AZ::EntityId& id) const
    {
        return AZStd::find(m_selectedEntityIds.begin(), m_selectedEntityIds.end(), id) != m_selectedEntityIds.end();
    }

    bool EntityPropertyEditor::IsSingleEntitySelected(const AZ::EntityId& id) const
    {
        return m_selectedEntityIds.size() == 1 && IsEntitySelected(id);
    }

    void EntityPropertyEditor::OnStartPlayInEditor()
    {
        setEnabled(false);
    }

    void EntityPropertyEditor::OnStopPlayInEditor()
    {
        setEnabled(true);
    }

    void EntityPropertyEditor::UpdateEntityIcon()
    {
        AZStd::string iconSourcePath;
        AzToolsFramework::EditorRequestBus::BroadcastResult(iconSourcePath, &AzToolsFramework::EditorRequestBus::Events::GetDefaultEntityIcon);

        if (!m_selectedEntityIds.empty())
        {
            //Test if every entity has the same icon, if so use that icon, otherwise use the default one.
            AZStd::string firstIconPath;
            EditorEntityIconComponentRequestBus::EventResult(firstIconPath, m_selectedEntityIds.front(), &EditorEntityIconComponentRequestBus::Events::GetEntityIconPath);

            bool haveSameIcon = true;
            for (const auto entityId : m_selectedEntityIds)
            {
                AZStd::string iconPath;
                EditorEntityIconComponentRequestBus::EventResult(iconPath, entityId, &EditorEntityIconComponentRequestBus::Events::GetEntityIconPath);
                if (iconPath != firstIconPath)
                {
                    haveSameIcon = false;
                    break;
                }
            }

            if (haveSameIcon)
            {
                iconSourcePath = AZStd::move(firstIconPath);
            }
        }

        m_gui->m_entityIcon->setIcon(QIcon(iconSourcePath.c_str()));
        m_gui->m_entityIcon->repaint();
    }

    void EntityPropertyEditor::UpdateEntityDisplay()
    {
        UpdateInitiallyActiveDisplay();

        // Generic text for multiple entities selected
        if (m_selectedEntityIds.size() > 1)
        {
            m_gui->m_entityDetailsLabel->setVisible(true);
            m_gui->m_entityDetailsLabel->setText(tr("Only common components shown"));
            m_gui->m_entityNameEditor->setText(tr("%n entities selected", "", static_cast<int>(m_selectedEntityIds.size())));
            m_gui->m_entityNameEditor->setReadOnly(true);
        }
        else if (!m_selectedEntityIds.empty())
        {
            auto entityId = m_selectedEntityIds.front();

            // No entity details for one entity
            m_gui->m_entityDetailsLabel->setVisible(false);

            // If we're in edit mode, make the name field editable.
            m_gui->m_entityNameEditor->setReadOnly(!m_gui->m_componentListContents->isEnabled());

            // get the name of the entity.
            auto entity = GetEntityById(entityId);
            m_gui->m_entityNameEditor->setText(entity ? entity->GetName().data() : "Entity Not Found");
        }
    }

    void EntityPropertyEditor::UpdateContents()
    {
        AZ_PROFILE_FUNCTION(AZ::Debug::ProfileCategory::AzToolsFramework);
        setUpdatesEnabled(false);

        m_isAlreadyQueuedRefresh = false;
        m_isBuildingProperties = true;
        m_sliceCompareToEntity.reset();

        HideComponentPalette();

        ClearInstances(false);
        ClearComponentEditorDragging();

        m_selectedEntityIds.clear();
        GetSelectedEntities(m_selectedEntityIds);

        SourceControlFileInfo scFileInfo;
        ToolsApplicationRequests::Bus::BroadcastResult(scFileInfo, &ToolsApplicationRequests::GetSceneSourceControlInfo);

        if (!m_spacer)
        {
            m_spacer = aznew QSpacerItem(1, 1, QSizePolicy::Fixed, QSizePolicy::Expanding);
        }

        // Hide the entity stuff and add component button if no entities are displayed
        bool hasEntitiesDisplayed = !m_selectedEntityIds.empty();
        m_gui->m_entityDetailsLabel->setText(hasEntitiesDisplayed ? "" : IsLockedToSpecificEntities() ? tr("The entity this inspector was pinned to has been deleted.") : tr("Select an entity to show its properties in the inspector."));
        m_gui->m_entityDetailsLabel->setVisible(!hasEntitiesDisplayed);
        m_gui->m_addComponentButton->setEnabled(hasEntitiesDisplayed);
        m_gui->m_addComponentButton->setVisible(hasEntitiesDisplayed);
        m_gui->m_entityNameEditor->setVisible(hasEntitiesDisplayed);
        m_gui->m_entityNameLabel->setVisible(hasEntitiesDisplayed);
        m_gui->m_entityIcon->setVisible(hasEntitiesDisplayed);
        m_gui->m_pinButton->setVisible(m_overrideSelectedEntityIds.size() == 0 && hasEntitiesDisplayed && !m_isSystemEntityEditor);
        m_gui->m_startActiveLabel->setVisible(hasEntitiesDisplayed && !m_isSystemEntityEditor);
        m_gui->m_initiallyActiveCheckbox->setVisible(hasEntitiesDisplayed && !m_isSystemEntityEditor);
        m_gui->m_entitySearchBox->setVisible(hasEntitiesDisplayed);
        m_gui->m_buttonClearFilter->setVisible(hasEntitiesDisplayed);

        if (hasEntitiesDisplayed)
        {
            // Build up components to display
            SharedComponentArray sharedComponentArray;
            BuildSharedComponentArray(sharedComponentArray);
            BuildSharedComponentUI(sharedComponentArray);

            UpdateEntityIcon();
            UpdateEntityDisplay();
        }

        QueueScrollToNewComponent();
        LoadComponentEditorState();
        UpdateInternalState();

        layout()->update();
        layout()->activate();

        m_isBuildingProperties = false;

        setUpdatesEnabled(true);

        // re-enable all actions so that things like "delete" or "cut" work again
        QList<QAction*> actionList = actions();
        for (QAction* action : actionList)
        {
            action->setEnabled(true);
        }
    }

    void EntityPropertyEditor::GetAllComponentsForEntityInOrder(const AZ::Entity* entity, AZ::Entity::ComponentArrayType& componentsOnEntity)
    {
        componentsOnEntity.clear();

        if (entity)
        {
            const AZ::EntityId entityId = entity->GetId();

            //get all components related to the entity in sorted and fixed order buckets
            AzToolsFramework::GetAllComponentsForEntity(entity, componentsOnEntity);

            RemoveHiddenComponents(componentsOnEntity);
            SortComponentsByOrder(entityId, componentsOnEntity);
            SortComponentsByPriority(componentsOnEntity);
            SaveComponentOrder(entityId, componentsOnEntity);
        }
    }

    void EntityPropertyEditor::RemoveHiddenComponents(AZ::Entity::ComponentArrayType& componentsOnEntity)
    {
        componentsOnEntity.erase(
            AZStd::remove_if(
                componentsOnEntity.begin(),
                componentsOnEntity.end(),
                [this](const AZ::Component* component)
        {
            return !ShouldInspectorShowComponent(component);
        }
            ),
            componentsOnEntity.end()
            );
    }

    void EntityPropertyEditor::SortComponentsByPriority(AZ::Entity::ComponentArrayType& componentsOnEntity)
    {
        //shuffle immovable components back to the front
        AZStd::sort(
            componentsOnEntity.begin(),
            componentsOnEntity.end(),
            [this](const AZ::Component* component1, const AZ::Component* component2)
            {
                // Transform component must be first, always
                // If component 1 is a transform component, it is sorted earlier
                if (component1->RTTI_IsTypeOf(AZ::EditorTransformComponentTypeId))
                {
                    return true;
                }

                // If component 2 is a transform component, component 1 is never sorted earlier
                if (component2->RTTI_IsTypeOf(AZ::EditorTransformComponentTypeId))
                {
                    return false;
                }

                    return !IsComponentRemovable(component1) && IsComponentRemovable(component2);
            });
    }

    void EntityPropertyEditor::SortComponentsByOrder(const AZ::EntityId& entityId, AZ::Entity::ComponentArrayType& componentsOnEntity)
    {
        //sort by component order, shuffling anything not found in component order to the end
        ComponentOrderArray componentOrder;
        EditorInspectorComponentRequestBus::EventResult(componentOrder, entityId, &EditorInspectorComponentRequests::GetComponentOrderArray);

        AZStd::sort(
            componentsOnEntity.begin(),
            componentsOnEntity.end(),
            [&componentOrder](const AZ::Component* component1, const AZ::Component* component2)
            {
                return
                    AZStd::find(componentOrder.begin(), componentOrder.end(), component1->GetId()) <
                    AZStd::find(componentOrder.begin(), componentOrder.end(), component2->GetId());
            });
    }

    void EntityPropertyEditor::SaveComponentOrder(const AZ::EntityId& entityId, const AZ::Entity::ComponentArrayType& componentsInOrder)
    {
        ComponentOrderArray componentOrder;
        componentOrder.clear();
        componentOrder.reserve(componentsInOrder.size());
        for (auto component : componentsInOrder)
        {
            if (component && component->GetEntityId() == entityId)
            {
                componentOrder.push_back(component->GetId());
            }
        }
        EditorInspectorComponentRequestBus::Event(entityId, &EditorInspectorComponentRequests::SetComponentOrderArray, componentOrder);
    }

    bool EntityPropertyEditor::IsComponentRemovable(const AZ::Component* component) const
    {
        // Determine if this component can be removed.
        auto componentClassData = component ? GetComponentClassData(component) : nullptr;
        return componentClassData && m_componentFilter(*componentClassData);
    }

    bool EntityPropertyEditor::AreComponentsRemovable(const AZ::Entity::ComponentArrayType& components) const
    {
        for (auto component : components)
        {
            if (!IsComponentRemovable(component))
            {
                return false;
            }
        }
        return true;
    }

    AZ::Component* EntityPropertyEditor::ExtractMatchingComponent(AZ::Component* referenceComponent, AZ::Entity::ComponentArrayType& availableComponents)
    {
        for (auto availableComponentIterator = availableComponents.begin(); availableComponentIterator != availableComponents.end(); ++availableComponentIterator)
        {
            auto component = *availableComponentIterator;
            auto editorComponent = GetEditorComponent(component);
            auto referenceEditorComponent = GetEditorComponent(referenceComponent);

            // Early out on class data not matching, we require components to be the same class
            if (GetComponentClassData(referenceComponent) != GetComponentClassData(component))
            {
                continue;
            }

            auto referenceEditorComponentDescriptor = GetEditorComponentDescriptor(referenceComponent);
            // If not AZ_EDITOR_COMPONENT, then as long as they match type (checked above) it is considered matching
            // Otherwise, use custom component matching available via AZ_EDITOR_COMPONENT
            if (!referenceEditorComponentDescriptor || referenceEditorComponentDescriptor->DoComponentsMatch(referenceEditorComponent, editorComponent))
            {
                // Remove the component we found from the available set
                availableComponents.erase(availableComponentIterator);
                return component;
            }
        }

        return nullptr;
    }

    bool EntityPropertyEditor::SelectedEntitiesAreFromSameSourceSliceEntity() const
    {
        AZ::EntityId commonAncestorId;

        for (AZ::EntityId id : m_selectedEntityIds)
        {
            AZ::SliceComponent::SliceInstanceAddress sliceInstanceAddress;
            AzFramework::EntityIdContextQueryBus::EventResult(sliceInstanceAddress, id, &AzFramework::EntityIdContextQueries::GetOwningSlice);
            auto sliceReference = sliceInstanceAddress.first;
            if (!sliceReference)
            {
                return false;
            }
            else
            {
                AZ::SliceComponent::EntityAncestorList ancestors;
                sliceReference->GetInstanceEntityAncestry(id, ancestors, 1);
                if ( ancestors.empty() || !ancestors[0].m_entity)
                {
                    return false;
                }

                if (!commonAncestorId.IsValid())
                {
                    commonAncestorId = ancestors[0].m_entity->GetId();
                }
                else if (ancestors[0].m_entity->GetId() != commonAncestorId)
                {
                    return false;
                }
            }
        }

        return true;
    }

    void EntityPropertyEditor::BuildSharedComponentArray(SharedComponentArray& sharedComponentArray)
    {
        AZ_Assert(!m_selectedEntityIds.empty(), "BuildSharedComponentArray should only be called if there are entities being displayed");

        auto entityId = m_selectedEntityIds.front();

        // For single selection of a slice-instanced entity, gather the direct slice ancestor
        // so we can visualize per-component differences.
        m_sliceCompareToEntity.reset();
        if (SelectedEntitiesAreFromSameSourceSliceEntity())
        {
            AZ::SliceComponent::SliceInstanceAddress sliceInstanceAddress;

            AzFramework::EntityIdContextQueryBus::EventResult(sliceInstanceAddress, entityId, &AzFramework::EntityIdContextQueries::GetOwningSlice);
            auto sliceReference = sliceInstanceAddress.first;
            auto sliceInstance = sliceInstanceAddress.second;
            if (sliceReference)
            {
                AZ::SliceComponent::EntityAncestorList ancestors;
                sliceReference->GetInstanceEntityAncestry(entityId, ancestors, 1);

                if (!ancestors.empty())
                {
                    m_sliceCompareToEntity = SliceUtilities::CloneSliceEntityForComparison(*ancestors[0].m_entity, *sliceInstance, *m_serializeContext);
                }
            }
        }

        // Gather initial list of eligible display components from the first entity
        auto entity = GetEntityById(entityId);

        AZ::Entity::ComponentArrayType entityComponents;
        GetAllComponentsForEntityInOrder(entity, entityComponents);

        for (auto component : entityComponents)
        {
            // Skip components that we should not display
            // Only need to do this for the initial set, components without edit data on other entities should not match these
            if (!ShouldInspectorShowComponent(component))
            {
                continue;
            }

            // Grab the slice reference component, if we have a slice compare entity
            auto sliceReferenceComponent = m_sliceCompareToEntity ? m_sliceCompareToEntity->FindComponent(component->GetId()) : nullptr;

            sharedComponentArray.push_back(SharedComponentInfo(component, sliceReferenceComponent));
        }

        // Now loop over the other entities
        for (size_t entityIndex = 1; entityIndex < m_selectedEntityIds.size(); ++entityIndex)
        {
            entity = GetEntityById(m_selectedEntityIds[entityIndex]);
            AZ_Assert(entity, "Entity id selected for display but no such entity exists");
            if (!entity)
            {
                continue;
            }

            entityComponents.clear();
            GetAllComponentsForEntityInOrder(entity, entityComponents);

            // Loop over the known components on all entities (so far)
            // Check to see if they are also on this entity
            // If not, remove them from the list
            for (auto sharedComponentInfoIterator = sharedComponentArray.begin(); sharedComponentInfoIterator != sharedComponentArray.end(); )
            {
                auto firstComponent = sharedComponentInfoIterator->m_instances[0];

                auto matchingComponent = ExtractMatchingComponent(firstComponent, entityComponents);
                if (!matchingComponent)
                {
                    // Remove this as it isn't on all entities so don't bother continuing with it
                    sharedComponentInfoIterator = sharedComponentArray.erase(sharedComponentInfoIterator);
                    continue;
                }

                // Add the matching component and continue
                sharedComponentInfoIterator->m_instances.push_back(matchingComponent);
                ++sharedComponentInfoIterator;
            }
        }
    }

    bool EntityPropertyEditor::ComponentMatchesCurrentFilter(SharedComponentInfo& sharedComponentInfo) const
    {
        if (m_filterString.size() == 0)
        {
            return true;
        }

        if (sharedComponentInfo.m_instances.front())
        {
            AZStd::string componentName = GetFriendlyComponentName(sharedComponentInfo.m_instances.front());

            if (componentName.size() == 0 || AzFramework::StringFunc::Find(componentName.c_str(), m_filterString.c_str()) != AZStd::string::npos)
            {
                return true;
            }
        }

        return false;
    }

    void EntityPropertyEditor::BuildSharedComponentUI(SharedComponentArray& sharedComponentArray)
    {
        // last-known widget, for the sake of tab-ordering
        QWidget* lastTabWidget = m_gui->m_addComponentButton;

        for (auto& sharedComponentInfo : sharedComponentArray)
        {
            bool componentInFilter = ComponentMatchesCurrentFilter(sharedComponentInfo);

            auto componentEditor = CreateComponentEditor();

            // Add instances to componentEditor
            auto& componentInstances = sharedComponentInfo.m_instances;
            for (AZ::Component* componentInstance : componentInstances)
            {
                // non-first instances are aggregated under the first instance
                AZ::Component* aggregateInstance = componentInstance != componentInstances.front() ? componentInstances.front() : nullptr;

                // Reference the slice entity if we are a slice so we can indicate differences from base
                AZ::Component* referenceComponentInstance = sharedComponentInfo.m_sliceReferenceComponent;
                componentEditor->AddInstance(componentInstance, aggregateInstance, referenceComponentInstance);
            }

            // Set tab order for editor
            setTabOrder(lastTabWidget, componentEditor);
            lastTabWidget = componentEditor;

            // Refresh editor
            componentEditor->AddNotifications();
            componentEditor->UpdateExpandability();
            componentEditor->InvalidateAll(!componentInFilter ? m_filterString.c_str() : nullptr);

            if (!componentEditor->GetPropertyEditor()->HasFilteredOutNodes() || componentEditor->GetPropertyEditor()->HasVisibleNodes())
            {
                for (AZ::Component* componentInstance : componentInstances)
                {
                    // Map the component to the editor
                    m_componentToEditorMap[componentInstance] = componentEditor;
                }
                componentEditor->show();
            }
            else
            {
                componentEditor->hide();
                componentEditor->ClearInstances(true);
            }
        }
    }

    ComponentEditor* EntityPropertyEditor::CreateComponentEditor()
    {
        //caching allocated component editors for reuse and to preserve order
        if (m_componentEditorsUsed >= m_componentEditors.size())
        {
            //create a new component editor since cache has been exceeded
            auto componentEditor = aznew ComponentEditor(m_serializeContext, this, this);
            componentEditor->setAcceptDrops(true);

            connect(componentEditor, &ComponentEditor::OnExpansionContractionDone, this, [this]()
                {
                    setUpdatesEnabled(false);
                    layout()->update();
                    layout()->activate();
                    setUpdatesEnabled(true);
                });
            connect(componentEditor, &ComponentEditor::OnDisplayComponentEditorMenu, this, &EntityPropertyEditor::OnDisplayComponentEditorMenu);
            connect(componentEditor, &ComponentEditor::OnRequestRequiredComponents, this, &EntityPropertyEditor::OnRequestRequiredComponents);
            connect(componentEditor, &ComponentEditor::OnRequestRemoveComponents, this, [this](const AZ::Entity::ComponentArrayType& components) {DeleteComponents(components); });
            connect(componentEditor, &ComponentEditor::OnRequestDisableComponents, this, [this](const AZ::Entity::ComponentArrayType& components) {DisableComponents(components); });
            componentEditor->GetPropertyEditor()->SetValueComparisonFunction([this](const InstanceDataNode* source, const InstanceDataNode* target) { return CompareInstanceDataNodeValues(source, target); });

            //move spacer to bottom of editors
            m_gui->m_componentListContents->layout()->removeItem(m_spacer);
            m_gui->m_componentListContents->layout()->addWidget(componentEditor);
            m_gui->m_componentListContents->layout()->addItem(m_spacer);
            m_gui->m_componentListContents->layout()->update();

            //add new editor to cache
            m_componentEditors.push_back(componentEditor);
        }

        //return editor at current index from cache
        return m_componentEditors[m_componentEditorsUsed++];
    }

    void EntityPropertyEditor::InvalidatePropertyDisplay(PropertyModificationRefreshLevel level)
    {
        if (level == Refresh_None)
        {
            return;
        }

        if (level == Refresh_EntireTree)
        {
            QueuePropertyRefresh();
            return;
        }

        if (level == Refresh_EntireTree_NewContent)
        {
            QueuePropertyRefresh();
            m_shouldScrollToNewComponents = true;
            return;
        }

        for (auto componentEditor : m_componentEditors)
        {
            if (componentEditor->isVisible())
            {
                componentEditor->QueuePropertyEditorInvalidation(level);
            }
        }
    }

    void EntityPropertyEditor::BeforeUndoRedo()
    {
        m_currentUndoOperation = nullptr;
        QueuePropertyRefresh();
    }

    void EntityPropertyEditor::AfterUndoRedo()
    {
        m_currentUndoOperation = nullptr;
    }

    // implementation of IPropertyEditorNotify:
    void EntityPropertyEditor::BeforePropertyModified(InstanceDataNode* pNode)
    {
        if (m_isBuildingProperties)
        {
            AZ_Error("PropertyEditor", !m_isBuildingProperties, "A property is being modified while we're building properties - make sure you do not call RequestWrite except in response to user action!");
            return;
        }

        EntityIdList selectedEntityIds;
        GetSelectedEntities(selectedEntityIds);
        if (selectedEntityIds.empty())
        {
            return;
        }

        bool areEntitiesEditable = true;
        ToolsApplicationRequests::Bus::BroadcastResult(areEntitiesEditable, &ToolsApplicationRequests::AreEntitiesEditable, selectedEntityIds);
        if (!areEntitiesEditable)
        {
            return;
        }

        for (AZ::EntityId entityId : selectedEntityIds)
        {
            bool editable = true;
            ToolsApplicationRequests::Bus::BroadcastResult(editable, &ToolsApplicationRequests::IsEntityEditable, entityId);
            if (!editable)
            {
                return;
            }
        }

        if ((m_currentUndoNode == pNode) && (m_currentUndoOperation))
        {
            // attempt to resume the last undo operation:
            ToolsApplicationRequests::Bus::BroadcastResult(m_currentUndoOperation, &ToolsApplicationRequests::ResumeUndoBatch, m_currentUndoOperation, "Modify Entity Property");
        }
        else
        {
            ToolsApplicationRequests::Bus::BroadcastResult(m_currentUndoOperation, &ToolsApplicationRequests::BeginUndoBatch, "Modify Entity Property");
            m_currentUndoNode = pNode;
        }

        // mark the ones' we're editing as dirty, so the user does not have to.
        for (auto entityShownId : selectedEntityIds)
        {
            ToolsApplicationRequests::Bus::Broadcast(&ToolsApplicationRequests::AddDirtyEntity, entityShownId);
        }
    }

    void EntityPropertyEditor::AfterPropertyModified(InstanceDataNode* pNode)
    {
        AZ_Assert(m_currentUndoOperation, "Invalid undo operation in AfterPropertyModified");
        AZ_Assert(m_currentUndoNode == pNode, "Invalid undo operation in AfterPropertyModified - node doesn't match!");

        ToolsApplicationRequests::Bus::Broadcast(&ToolsApplicationRequests::EndUndoBatch);

        InstanceDataNode* componentNode = pNode;
        while (componentNode->GetParent())
        {
            componentNode = componentNode->GetParent();
        }

        if (!componentNode)
        {
            AZ_Warning("Property Editor", false, "Failed to locate component data associated with the affected field. External change event cannot be sent.");
            return;
        }

        // Set our flag to not refresh values based on our own update broadcasts
        m_initiatingPropertyChangeNotification = true;

        // Send EBus notification for the affected entities, along with information about the affected component.
        for (size_t instanceIdx = 0; instanceIdx < componentNode->GetNumInstances(); ++instanceIdx)
        {
            AZ::Component* componentInstance = m_serializeContext->Cast<AZ::Component*>(
                    componentNode->GetInstance(instanceIdx), componentNode->GetClassMetadata()->m_typeId);
            if (componentInstance)
            {
                PropertyEditorEntityChangeNotificationBus::Event(
                    componentInstance->GetEntityId(),
                    &PropertyEditorEntityChangeNotifications::OnEntityComponentPropertyChanged,
                    componentInstance->GetId());
            }
        }

        m_initiatingPropertyChangeNotification = false;
    }

    void EntityPropertyEditor::SetPropertyEditingActive(InstanceDataNode* /*pNode*/)
    {
        MarkPropertyEditorBusyStart();
    }

    void EntityPropertyEditor::SetPropertyEditingComplete(InstanceDataNode* /*pNode*/)
    {
        MarkPropertyEditorBusyEnd();
    }

    void EntityPropertyEditor::OnPropertyRefreshRequired()
    {
        MarkPropertyEditorBusyEnd();
    }

    void EntityPropertyEditor::Reflect(AZ::ReflectContext* /*context*/)
    {
    }

    void EntityPropertyEditor::SetAllowRename(bool allowRename)
    {
        m_gui->m_entityNameEditor->setReadOnly(!allowRename);
    }

    void EntityPropertyEditor::ClearInstances(bool invalidateImmediately)
    {
        for (auto componentEditor : m_componentEditors)
        {
            componentEditor->hide();
            componentEditor->ClearInstances(invalidateImmediately);
        }

        m_componentEditorsUsed = 0;
        m_currentUndoNode = nullptr;
        m_currentUndoOperation = nullptr;
        m_selectedEntityIds.clear();
        m_componentToEditorMap.clear();
    }

    void EntityPropertyEditor::MarkPropertyEditorBusyStart()
    {
        m_currentUndoNode = nullptr;
        m_currentUndoOperation = nullptr;
        m_propertyEditBusy++;
    }

    void EntityPropertyEditor::MarkPropertyEditorBusyEnd()
    {
        m_propertyEditBusy--;
        if (m_propertyEditBusy <= 0)
        {
            m_propertyEditBusy = 0;
            QueuePropertyRefresh();
        }
    }

    void EntityPropertyEditor::SealUndoStack()
    {
        m_currentUndoNode = nullptr;
        m_currentUndoOperation = nullptr;
    }

    void EntityPropertyEditor::QueuePropertyRefresh()
    {
        AZ_PROFILE_FUNCTION(AZ::Debug::ProfileCategory::AzToolsFramework);
        if (!m_isAlreadyQueuedRefresh)
        {
            m_isAlreadyQueuedRefresh = true;
            
            // disable all actions until queue refresh is done, so that long queues of events like
            // click - delete - click - delete - click .... dont all occur while the state is invalid.
            // note that this happens when the data is already in an invalid state before this function is called...
            // pointers are bad, components have been deleted, etc, 
            QList<QAction*> actionList = actions();
            for (QAction* action : actionList)
            {
                action->setEnabled(false);
            }

            // Refresh the properties using a singleShot
            // this makes sure that the properties aren't refreshed while processing
            // other events, and instead runs after the current events are processed
            QTimer::singleShot(0, this, &EntityPropertyEditor::UpdateContents);

            //saving state any time refresh gets queued because requires valid components
            //attempting to call directly anywhere state needed to be preserved always occurred with QueuePropertyRefresh
            SaveComponentEditorState();
        }
    }

    void EntityPropertyEditor::OnAddComponent()
    {
        AZStd::vector<AZ::ComponentServiceType> serviceFilter;
        QRect globalRect = GetWidgetGlobalRect(m_gui->m_addComponentButton) | GetWidgetGlobalRect(m_gui->m_componentList);
        ShowComponentPalette(m_componentPalette, globalRect.topLeft(), globalRect.size(), serviceFilter);
    }

    void EntityPropertyEditor::GotSceneSourceControlStatus(SourceControlFileInfo& fileInfo)
    {
        m_sceneIsNew = false;
        EnableEditor(!fileInfo.IsReadOnly());
    }

    void EntityPropertyEditor::PerformActionsBasedOnSceneStatus(bool sceneIsNew, bool readOnly)
    {
        m_sceneIsNew = sceneIsNew;
        EnableEditor(!readOnly);
    }

    void EntityPropertyEditor::EnableEditor(bool enabled)
    {
        m_gui->m_entityNameEditor->setReadOnly(!enabled);
        m_gui->m_initiallyActiveCheckbox->setEnabled(enabled);
        m_gui->m_addComponentButton->setVisible(enabled);
        m_gui->m_componentListContents->setEnabled(enabled);
    }

    void EntityPropertyEditor::OnEntityNameChanged()
    {
        if (m_gui->m_entityNameEditor->isReadOnly())
        {
            return;
        }

        EntityIdList selectedEntityIds;
        GetSelectedEntities(selectedEntityIds);

        if (selectedEntityIds.size() == 1)
        {
            QByteArray entityNameByteArray = m_gui->m_entityNameEditor->text().toUtf8();

            const AZStd::string entityName(entityNameByteArray.data());

            AZStd::vector<AZ::Entity*> entityList;

            for (AZ::EntityId entityId : selectedEntityIds)
            {
                AZ::Entity* entity = GetEntityById(entityId);

                if (entityName != entity->GetName())
                {
                    entityList.push_back(entity);
                }
            }

            if (entityList.size())
            {
                ScopedUndoBatch undoBatch("ModifyEntityName");

                for (AZ::Entity* entity : entityList)
                {
                    entity->SetName(entityName);
                    ToolsApplicationRequests::Bus::Broadcast(&ToolsApplicationRequests::AddDirtyEntity, entity->GetId());
                }
            }
        }
    }

    void EntityPropertyEditor::RequestPropertyContextMenu(InstanceDataNode* node, const QPoint& position)
    {
        AZ_Assert(node, "Invalid node passed to context menu callback.");

        if (!node)
        {
            return;
        }

        // Locate the owning component and class data corresponding to the clicked node.
        InstanceDataNode* componentNode = node;
        while (componentNode->GetParent())
        {
            componentNode = componentNode->GetParent();
        }

        // Get inner class data for the clicked component.
        AZ::Component* component = m_serializeContext->Cast<AZ::Component*>(
                componentNode->FirstInstance(), componentNode->GetClassMetadata()->m_typeId);
        if (component)
        {
            auto componentClassData = GetComponentClassData(component);
            if (componentClassData)
            {
                QMenu menu;

                // Populate and show the context menu.
                AddMenuOptionsForComponents(menu, position);

                if (!menu.actions().empty())
                {
                    menu.addSeparator();
                }

                AddMenuOptionsForFields(node, componentNode, componentClassData, menu);

                AddMenuOptionForSliceReset(menu);

                if (!menu.actions().empty())
                {
                    menu.exec(position);
                }
            }
        }
    }

    void EntityPropertyEditor::AddMenuOptionsForComponents(QMenu& menu, const QPoint& /*position*/)
    {
        UpdateInternalState();
        menu.addActions(actions());
    }

    void EntityPropertyEditor::AddMenuOptionsForFields(
        InstanceDataNode* fieldNode,
        InstanceDataNode* componentNode,
        const AZ::SerializeContext::ClassData* componentClassData,
        QMenu& menu)
    {
		if (!fieldNode || (!fieldNode->GetComparisonNode() && !fieldNode->IsNewVersusComparison()))
        {
            return;
        }

        // For items marked as new (container elements), we'll synchronize the parent node.
        if (fieldNode->IsNewVersusComparison())
        {
            fieldNode = fieldNode->GetParent();
            AZ_Assert(fieldNode && fieldNode->GetClassMetadata() && fieldNode->GetClassMetadata()->m_container, "New element should be a child of a container.");
        }

        // Generate prefab data push/pull options.
        AZ::Component* componentInstance = m_serializeContext->Cast<AZ::Component*>(
                componentNode->FirstInstance(), componentClassData->m_typeId);
        AZ_Assert(componentInstance, "Failed to cast component instance.");

        // With the entity the property ultimately belongs to, we can look up prefab ancestry for push/pull options.
        AZ::Entity* entity = componentInstance->GetEntity();

        AZ::SliceComponent::SliceInstanceAddress address;
        AzFramework::EntityIdContextQueryBus::EventResult(address, entity->GetId(), &AzFramework::EntityIdContextQueries::GetOwningSlice);
        if (!address.first)
        {
            // This entity does not belong to a prefab, so data push/pull is not relevant.
            return;
        }

        AZ::SliceComponent::EntityAncestorList ancestors;
        address.first->GetInstanceEntityAncestry(entity->GetId(), ancestors);

        if (ancestors.empty())
        {
            AZ_Error("PropertyEditor", false, "Entity \"%s\" belongs to a prefab, but its source entity could not be located.");
            return;
        }

        AZStd::string sliceAssetPath;
        AZ::Data::AssetCatalogRequestBus::BroadcastResult(sliceAssetPath, &AZ::Data::AssetCatalogRequests::GetAssetPathById, ancestors.front().m_sliceAddress.first->GetSliceAsset().GetId());

        AZ::EntityId entityId = entity->GetId();
        QAction* pushAction = menu.addAction(tr("Push to slice..."));
		pushAction->setEnabled(fieldNode->HasChangesVersusComparison(true));
        connect(pushAction, &QAction::triggered, this, [entityId]()
            {
                SliceUtilities::PushEntitiesModal({ entityId }, nullptr);
            }
            );

        menu.addSeparator();

		bool hasChanges = fieldNode->HasChangesVersusComparison(false);
		bool isLeafNode = !fieldNode->GetClassMetadata()->m_container && fieldNode->GetClassMetadata()->m_serializer;

        if (!hasChanges && isLeafNode)
        {
            // Add an option to set the ForceOverride flag for this field
            menu.setToolTipsVisible(true);
            QAction* forceOverrideAction = menu.addAction(tr("Force property override"));
            forceOverrideAction->setToolTip(tr("Prevents a property from inheriting from its source slice"));
            connect(forceOverrideAction, &QAction::triggered, this, [this, fieldNode]()
                {
                    ContextMenuActionSetDataFlag(fieldNode, AZ::DataPatch::Flag::ForceOverride, true);
                }
                );
        }
        else
        {
            // Add an option to pull data from the first level of the slice (clear the override).
            QAction* pullAction = menu.addAction(tr("Revert property override"));
            pullAction->setEnabled(isLeafNode);
            connect(pullAction, &QAction::triggered, this, [this, componentInstance, fieldNode]()
                {
                    ContextMenuActionPullFieldData(componentInstance, fieldNode);
                }
                );
        }
    }

    void EntityPropertyEditor::AddMenuOptionForSliceReset(QMenu& menu)
    {
        bool hasSliceChanges = false;
        bool isPartOfSlice = false;

        const auto& componentsToEdit = GetSelectedComponents();

        // Determine if there are changes for any selected components
        for (auto component : componentsToEdit)
        {
            AZ_Assert(component, "Parent component is invalid.");
            auto componentEditorIterator = m_componentToEditorMap.find(component);
            AZ_Assert(componentEditorIterator != m_componentToEditorMap.end(), "Unable to find a component editor for the given component");
            if (componentEditorIterator != m_componentToEditorMap.end())
            {
                auto componentEditor = componentEditorIterator->second;
                componentEditor->GetPropertyEditor()->EnumerateInstances(
                    [&hasSliceChanges, &isPartOfSlice](InstanceDataHierarchy& hierarchy)
                    {
                        InstanceDataNode* root = hierarchy.GetRootNode();
                        if (root && root->GetComparisonNode())
                        {
                            isPartOfSlice = true;
                            if (root->HasChangesVersusComparison(true))
                            {
                                hasSliceChanges = true;
                            }
                        }
                    }
                    );
            }
        }

        if (isPartOfSlice)
        {
            QAction* resetToSliceAction = menu.addAction(tr("Revert component overrides"));
            resetToSliceAction->setEnabled(hasSliceChanges);
            connect(resetToSliceAction, &QAction::triggered, this, [this]()
                {
                    ResetToSlice();
                }
                );
        }
    }


    void EntityPropertyEditor::ContextMenuActionPullFieldData(AZ::Component* parentComponent, InstanceDataNode* fieldNode)
    {
        AZ_Assert(fieldNode && fieldNode->GetComparisonNode(), "Invalid node for prefab data pull.");
        if (!fieldNode->GetComparisonNode())
        {
            return;
        }

        BeforePropertyModified(fieldNode);

        // Clear any slice data flags for this field
        if (GetEntityDataPatchAddress(fieldNode, m_dataPatchAddressBuffer))
        {
            auto clearDataFlagsCommand = aznew ClearSliceDataFlagsBelowAddressCommand(parentComponent->GetEntityId(), m_dataPatchAddressBuffer, "Clear data flags");
            clearDataFlagsCommand->SetParent(m_currentUndoOperation);
        }

        AZStd::unordered_set<InstanceDataNode*> targetContainerNodesWithNewElements;
        InstanceDataHierarchy::CopyInstanceData(fieldNode->GetComparisonNode(), fieldNode, m_serializeContext,
            // Target container child node being removed callback
            nullptr,

            // Source container child node being created callback
            [&targetContainerNodesWithNewElements](const InstanceDataNode* sourceNode, InstanceDataNode* targetNodeParent)
            {
                (void)sourceNode;

                targetContainerNodesWithNewElements.insert(targetNodeParent);
            }
            );

        // Invoke Add notifiers
        for (InstanceDataNode* targetContainerNode : targetContainerNodesWithNewElements)
        {
            AZ_Assert(targetContainerNode->GetClassMetadata()->m_container, "Target container node isn't a container!");
            for (const AZ::Edit::AttributePair& attribute : targetContainerNode->GetElementEditMetadata()->m_attributes)
            {
                if (attribute.first == AZ::Edit::Attributes::AddNotify)
                {
                    AZ::Edit::AttributeFunction<void()>* func = azdynamic_cast<AZ::Edit::AttributeFunction<void()>*>(attribute.second);
                    if (func)
                    {
                        InstanceDataNode* parentNode = targetContainerNode->GetParent();
                        if (parentNode)
                        {
                            for (size_t idx = 0; idx < parentNode->GetNumInstances(); ++idx)
                            {
                                func->Invoke(parentNode->GetInstance(idx));
                            }
                        }
                    }
                }
            }
        }

        // Make sure change notifiers are invoked for the change across any changed nodes in sub-hierarchy
        // under the affected node.
        AZ_Assert(parentComponent, "Parent component is invalid.");
        auto componentEditorIterator = m_componentToEditorMap.find(parentComponent);
        AZ_Assert(componentEditorIterator != m_componentToEditorMap.end(), "Unable to find a component editor for the given component");
        if (componentEditorIterator != m_componentToEditorMap.end())
        {
            AZStd::vector<PropertyRowWidget*> widgetStack;
            AZStd::vector<PropertyRowWidget*> widgetsToNotify;
            widgetStack.reserve(64);
            widgetsToNotify.reserve(64);

            auto componentEditor = componentEditorIterator->second;
            PropertyRowWidget* widget = componentEditor->GetPropertyEditor()->GetWidgetFromNode(fieldNode);
            if (widget)
            {
                widgetStack.push_back(widget);

                while (!widgetStack.empty())
                {
                    PropertyRowWidget* top = widgetStack.back();
                    widgetStack.pop_back();

                    InstanceDataNode* topWidgetNode = top->GetNode();
                    if (topWidgetNode && topWidgetNode->HasChangesVersusComparison(false))
                    {
                        widgetsToNotify.push_back(top);
                    }

                    // Only add children widgets for notification if the newly updated parent has children
                    // otherwise the instance data nodes will have disappeared out from under the widgets
                    // They will be cleaned up when we refresh the entire tree after the notifications go out
                    if (!topWidgetNode || topWidgetNode->GetChildren().size() > 0)
                    {
                        for (auto* childWidget : top->GetChildrenRows())
                        {
                            widgetStack.push_back(childWidget);
                        }
                    }
                }

                // Issue property notifications, starting with leaf widgets first.
                for (auto iter = widgetsToNotify.rbegin(); iter != widgetsToNotify.rend(); ++iter)
                {
                    (*iter)->DoPropertyNotify();
                }
            }
        }

        AfterPropertyModified(fieldNode);

        // We must refresh the entire tree, because restoring previous entity state may've had major structural effects.
        InvalidatePropertyDisplay(Refresh_EntireTree);
    }

    void EntityPropertyEditor::ContextMenuActionSetDataFlag(InstanceDataNode* node, AZ::DataPatch::Flag flag, bool additive)
    {
        // Attempt to get Entity and DataPatch address from this node.
        AZ::EntityId entityId;
        if (GetEntityDataPatchAddress(node, m_dataPatchAddressBuffer, &entityId))
        {
            BeforePropertyModified(node);

            auto command = aznew SliceDataFlagsCommand(entityId, m_dataPatchAddressBuffer, flag, additive, "Set slice data flag");
            command->SetParent(m_currentUndoOperation);

            AfterPropertyModified(node);
            InvalidatePropertyDisplay(Refresh_AttributesAndValues);
        }
    }

    bool EntityPropertyEditor::GetEntityDataPatchAddress(const InstanceDataNode* node, AZ::DataPatch::AddressType& dataPatchAddressOut, AZ::EntityId* entityIdOut) const
    {
        if (InstanceDataNode* componentNode = node ? node->GetRoot() : nullptr)
        {
            AZ::Component* component = m_serializeContext->Cast<AZ::Component*>(
                    componentNode->FirstInstance(), componentNode->GetClassMetadata()->m_typeId);
            if (component)
            {
                AZ::EntityId entityId = component->GetEntityId();
                if (entityId.IsValid())
                {
                    InstanceDataNode::Address nodeAddress = node->ComputeAddress();

                    // Translate InstanceDataNode address into a DataPatch address that's relative to an entity.
                    // Differences:
                    // 1) They're stored in reverse order. nodeAddress is leaf-first. dataPatchAddress is root-first.
                    // 2) nodeAddress stores a CRC of the component's class name, dataPatchAddress stores the ComponentId instead.
                    // 3) dataPatchAddress is relative to the entity, nodeAddress is relative to the component.

                    AZ_Assert(nodeAddress.back() == AZ::Crc32(componentNode->GetClassMetadata()->m_name),
                        "GetEntityDataPatchAddress has bad assumptions about InstanceDataNode addressing");
                    nodeAddress.pop_back(); // throw out CRC of component name

                    // start the dataPatchAddress with the path to this particular Component in the Entity
                    dataPatchAddressOut.clear();
                    dataPatchAddressOut.reserve(nodeAddress.size() + 2);
                    dataPatchAddressOut.push_back(AZ_CRC("Components", 0xee48f5fd));
                    dataPatchAddressOut.push_back(component->GetId());

                    // the rest of the dataPatchAddress is identical to the reversed nodeAddress
                    for (auto reverseIterator = nodeAddress.rbegin(); reverseIterator != nodeAddress.rend(); ++reverseIterator)
                    {
                        dataPatchAddressOut.push_back(*reverseIterator);
                    }

                    if (entityIdOut)
                    {
                        *entityIdOut = entityId;
                    }

                    return true;
                }
            }
        }

        return false;
    }

    bool EntityPropertyEditor::CompareInstanceDataNodeValues(const InstanceDataNode* sourceNode, const InstanceDataNode* targetNode)
    {
        // If target node has ForceOverride flag set, consider it different from source.
        AZ::EntityId entityId;
        if (GetEntityDataPatchAddress(targetNode, m_dataPatchAddressBuffer, &entityId))
        {
            AZ::SliceComponent::SliceInstanceAddress instanceAddress;
            AzFramework::EntityIdContextQueryBus::EventResult(instanceAddress, entityId, &AzFramework::EntityIdContextQueryBus::Events::GetOwningSlice);
            if (const AZ::SliceComponent::SliceInstance* instance = instanceAddress.second)
            {
                AZ::DataPatch::Flags flags = instance->GetDataFlags().GetEntityDataFlagsAtAddress(entityId, m_dataPatchAddressBuffer);
                if (flags & AZ::DataPatch::Flag::ForceOverride)
                {
                    return false;
                }
            }
        }

        // Otherwise, do the default value comparison.
        return InstanceDataHierarchy::DefaultValueComparisonFunction(sourceNode, targetNode);
    }

    void EntityPropertyEditor::UpdateInitiallyActiveDisplay()
    {
        QSignalBlocker noSignals(m_gui->m_initiallyActiveCheckbox);

        if (m_selectedEntityIds.empty())
        {
            m_gui->m_initiallyActiveCheckbox->setVisible(false);
            return;
        }

        bool allActive = true;
        bool allInactive = true;

        for (AZ::EntityId id : m_selectedEntityIds)
        {
            AZ::Entity* entity = nullptr;
            AZ::ComponentApplicationBus::BroadcastResult(entity, &AZ::ComponentApplicationBus::Events::FindEntity, id);

            const bool isInitiallyActive = entity ? entity->IsRuntimeActiveByDefault() : true;
            allActive &= isInitiallyActive;
            allInactive &= !isInitiallyActive;
        }

        if (allActive)
        {
            m_gui->m_initiallyActiveCheckbox->setCheckState(Qt::CheckState::Checked);
        }
        else if (allInactive)
        {
            m_gui->m_initiallyActiveCheckbox->setCheckState(Qt::CheckState::Unchecked);
        }
        else // Some marked active, some not
        {
            m_gui->m_initiallyActiveCheckbox->setCheckState(Qt::CheckState::PartiallyChecked);
        }

        m_gui->m_initiallyActiveCheckbox->setVisible(!m_isSystemEntityEditor);
    }

    void EntityPropertyEditor::OnDisplayComponentEditorMenu(const QPoint& position)
    {
        QMenu menu;

        AddMenuOptionsForComponents(menu, position);

        if (!menu.actions().empty())
        {
            menu.addSeparator();
        }

        AddMenuOptionForSliceReset(menu);

        if (!menu.actions().empty())
        {
            m_isShowingContextMenu = true;
            menu.exec(position);
            m_isShowingContextMenu = false;
        }
    }

    void EntityPropertyEditor::OnRequestRequiredComponents(const QPoint& position, const QSize& size, const AZStd::vector<AZ::ComponentServiceType>& services)
    {
        ShowComponentPalette(m_componentPalette, position, size, services);
    }

    void EntityPropertyEditor::HideComponentPalette()
    {
        m_componentPalette->hide();
        m_componentPalette->setGeometry(QRect(QPoint(0, 0), QSize(0, 0)));
    }

    void EntityPropertyEditor::ShowComponentPalette(
        ComponentPaletteWidget* componentPalette,
        const QPoint& position,
        const QSize& size,
        const AZStd::vector<AZ::ComponentServiceType>& serviceFilter)
    {
        HideComponentPalette();

        bool areEntitiesEditable = true;
        ToolsApplicationRequests::Bus::BroadcastResult(areEntitiesEditable, &ToolsApplicationRequests::AreEntitiesEditable, m_selectedEntityIds);
        if (areEntitiesEditable)
        {
            componentPalette->Populate(m_serializeContext, m_selectedEntityIds, m_componentFilter, serviceFilter);
            componentPalette->Present();
            componentPalette->setGeometry(QRect(position, size));
        }
    }

    void EntityPropertyEditor::SetAddComponentMenuFilter(ComponentFilter componentFilter)
    {
        m_componentFilter = AZStd::move(componentFilter);
    }

    void EntityPropertyEditor::CreateActions()
    {
        m_actionToAddComponents = aznew QAction(tr("Add component"), this);
        m_actionToAddComponents->setShortcut(QKeySequence::New);
        m_actionToAddComponents->setShortcutContext(Qt::WidgetWithChildrenShortcut);
        connect(m_actionToAddComponents, &QAction::triggered, this, &EntityPropertyEditor::OnAddComponent);
        addAction(m_actionToAddComponents);

        m_actionToDeleteComponents = aznew QAction(tr("Delete component"), this);
        m_actionToDeleteComponents->setShortcut(QKeySequence::Delete);
        m_actionToDeleteComponents->setShortcutContext(Qt::WidgetWithChildrenShortcut);
        connect(m_actionToDeleteComponents, &QAction::triggered, this, [this]() { DeleteComponents(); });
        addAction(m_actionToDeleteComponents);

        QAction* seperator1 = aznew QAction(this);
        seperator1->setSeparator(true);
        addAction(seperator1);

        m_actionToCutComponents = aznew QAction(tr("Cut component"), this);
        m_actionToCutComponents->setShortcut(QKeySequence::Cut);
        m_actionToCutComponents->setShortcutContext(Qt::WidgetWithChildrenShortcut);
        connect(m_actionToCutComponents, &QAction::triggered, this, &EntityPropertyEditor::CutComponents);
        addAction(m_actionToCutComponents);

        m_actionToCopyComponents = aznew QAction(tr("Copy component"), this);
        m_actionToCopyComponents->setShortcut(QKeySequence::Copy);
        m_actionToCopyComponents->setShortcutContext(Qt::WidgetWithChildrenShortcut);
        connect(m_actionToCopyComponents, &QAction::triggered, this, &EntityPropertyEditor::CopyComponents);
        addAction(m_actionToCopyComponents);

        m_actionToPasteComponents = aznew QAction(tr("Paste component"), this);
        m_actionToPasteComponents->setShortcut(QKeySequence::Paste);
        m_actionToPasteComponents->setShortcutContext(Qt::WidgetWithChildrenShortcut);
        connect(m_actionToPasteComponents, &QAction::triggered, this, &EntityPropertyEditor::PasteComponents);
        addAction(m_actionToPasteComponents);

        QAction* seperator2 = aznew QAction(this);
        seperator2->setSeparator(true);
        addAction(seperator2);

        m_actionToEnableComponents = aznew QAction(tr("Enable component"), this);
        m_actionToEnableComponents->setShortcutContext(Qt::WidgetWithChildrenShortcut);
        connect(m_actionToEnableComponents, &QAction::triggered, this, [this]() {EnableComponents(); });
        addAction(m_actionToEnableComponents);

        m_actionToDisableComponents = aznew QAction(tr("Disable component"), this);
        m_actionToDisableComponents->setShortcutContext(Qt::WidgetWithChildrenShortcut);
        connect(m_actionToDisableComponents, &QAction::triggered, this, [this]() {DisableComponents(); });
        addAction(m_actionToDisableComponents);

        m_actionToMoveComponentsUp = aznew QAction(tr("Move component up"), this);
        m_actionToMoveComponentsUp->setShortcut(QKeySequence::MoveToPreviousPage);
        m_actionToMoveComponentsUp->setShortcutContext(Qt::WidgetWithChildrenShortcut);
        connect(m_actionToMoveComponentsUp, &QAction::triggered, this, &EntityPropertyEditor::MoveComponentsUp);
        addAction(m_actionToMoveComponentsUp);

        m_actionToMoveComponentsDown = aznew QAction(tr("Move component down"), this);
        m_actionToMoveComponentsDown->setShortcut(QKeySequence::MoveToNextPage);
        m_actionToMoveComponentsDown->setShortcutContext(Qt::WidgetWithChildrenShortcut);
        connect(m_actionToMoveComponentsDown, &QAction::triggered, this, &EntityPropertyEditor::MoveComponentsDown);
        addAction(m_actionToMoveComponentsDown);

        m_actionToMoveComponentsTop = aznew QAction(tr("Move component to top"), this);
        m_actionToMoveComponentsTop->setShortcut(Qt::Key_Home);
        m_actionToMoveComponentsTop->setShortcutContext(Qt::WidgetWithChildrenShortcut);
        connect(m_actionToMoveComponentsTop, &QAction::triggered, this, &EntityPropertyEditor::MoveComponentsTop);
        addAction(m_actionToMoveComponentsTop);

        m_actionToMoveComponentsBottom = aznew QAction(tr("Move component to bottom"), this);
        m_actionToMoveComponentsBottom->setShortcut(Qt::Key_End);
        m_actionToMoveComponentsBottom->setShortcutContext(Qt::WidgetWithChildrenShortcut);
        connect(m_actionToMoveComponentsBottom, &QAction::triggered, this, &EntityPropertyEditor::MoveComponentsBottom);
        addAction(m_actionToMoveComponentsBottom);

        UpdateInternalState();
    }

    void EntityPropertyEditor::UpdateActions()
    {
        AZ_PROFILE_FUNCTION(AZ::Debug::ProfileCategory::AzToolsFramework);
        const auto& componentsToEdit = GetSelectedComponents();

        bool allowRemove = !m_selectedEntityIds.empty() && !componentsToEdit.empty() && AreComponentsRemovable(componentsToEdit);

        m_actionToAddComponents->setEnabled(!m_selectedEntityIds.empty());
        m_actionToDeleteComponents->setEnabled(allowRemove);
        m_actionToCutComponents->setEnabled(allowRemove);
        m_actionToCopyComponents->setEnabled(allowRemove);
        m_actionToPasteComponents->setEnabled(!m_selectedEntityIds.empty() && ComponentMimeData::GetComponentMimeDataFromClipboard());
        m_actionToMoveComponentsUp->setEnabled(allowRemove && IsMoveComponentsUpAllowed());
        m_actionToMoveComponentsDown->setEnabled(allowRemove && IsMoveComponentsDownAllowed());
        m_actionToMoveComponentsTop->setEnabled(allowRemove && IsMoveComponentsUpAllowed());
        m_actionToMoveComponentsBottom->setEnabled(allowRemove && IsMoveComponentsDownAllowed());

        bool allowEnable = false;
        bool allowDisable = false;

        AZ::Entity::ComponentArrayType disabledComponents;

        //build a working set of all components selected for edit
        AZStd::unordered_set<AZ::Component*> enabledComponents(componentsToEdit.begin(), componentsToEdit.end());
        for (const auto& entityId : m_selectedEntityIds)
        {
            disabledComponents.clear();
            EditorDisabledCompositionRequestBus::Event(entityId, &EditorDisabledCompositionRequests::GetDisabledComponents, disabledComponents);

            //remove all disabled components from the set of enabled components
            for (auto disabledComponent : disabledComponents)
            {
                if (enabledComponents.erase(disabledComponent) > 0)
                {
                    //if any disabled components were found/removed from the selected set, assume they can be enabled
                    allowEnable = true;
                }
            }
        }

        //if any components remain in the selected set, assume they can be disabled
        allowDisable = !enabledComponents.empty();

        //disable actions when not allowed
        m_actionToEnableComponents->setEnabled(allowRemove && allowEnable);
        m_actionToDisableComponents->setEnabled(allowRemove && allowDisable);

        //additional request to hide actions when not allowed so enable and disable aren't shown at the same time
        m_actionToEnableComponents->setVisible(allowRemove && allowEnable);
        m_actionToDisableComponents->setVisible(allowRemove && allowDisable);
    }

    AZ::Entity::ComponentArrayType EntityPropertyEditor::GetCopyableComponents() const
    {
        const auto& selectedComponents = GetSelectedComponents();
        AZ::Entity::ComponentArrayType copyableComponents;
        copyableComponents.reserve(selectedComponents.size());
        for (auto component : selectedComponents)
        {
            const AZ::Entity* entity = component ? component->GetEntity() : nullptr;
            const AZ::EntityId entityId = entity ? entity->GetId() : AZ::EntityId();
            if (entityId == m_selectedEntityIds.front())
            {
                copyableComponents.push_back(component);
            }
        }
        return copyableComponents;
    }

    void EntityPropertyEditor::DeleteComponents(const AZ::Entity::ComponentArrayType& components)
    {
        if (!components.empty() && AreComponentsRemovable(components))
        {
            QueuePropertyRefresh();

            ScopedUndoBatch undoBatch("Removing components.");

            // Navigation triggered - Right Click in the Entity Inspector on the removed component
            EditorMetricsEventsBusAction editorMetricsEventsBusActionWrapper(EditorMetricsEventsBusTraits::NavigationTrigger::RightClickMenu);

            for (auto component : components)
            {
                // Remove Component metrics event (Right Click in the Entity Inspector on the removed component)
                auto componentClassData = GetComponentClassData(component);
                EditorMetricsEventsBus::Broadcast(&EditorMetricsEventsBusTraits::ComponentRemoved, component->GetEntityId(), componentClassData->m_typeId);
            }

            AzToolsFramework::RemoveComponents(components);
        }
    }

    void EntityPropertyEditor::DeleteComponents()
    {
        DeleteComponents(GetSelectedComponents());
    }

    void EntityPropertyEditor::CutComponents()
    {
        const auto& componentsToEdit = GetSelectedComponents();
        if (!componentsToEdit.empty() && AreComponentsRemovable(componentsToEdit))
        {
            QueuePropertyRefresh();

            //intentionally not using EntityCompositionRequests::CutComponents because we only want to copy components from first selected entity
            ScopedUndoBatch undoBatch("Cut components.");
            CopyComponents();
            DeleteComponents();
        }
    }

    void EntityPropertyEditor::CopyComponents()
    {
        const auto& componentsToEdit = GetCopyableComponents();
        if (!componentsToEdit.empty() && AreComponentsRemovable(componentsToEdit))
        {
            EntityCompositionRequestBus::Broadcast(&EntityCompositionRequests::CopyComponents, componentsToEdit);
        }
    }

    void EntityPropertyEditor::PasteComponents()
    {
        if (!m_selectedEntityIds.empty() && ComponentMimeData::GetComponentMimeDataFromClipboard())
        {
            ScopedUndoBatch undoBatch("Paste components.");

            AZ::Entity::ComponentArrayType selectedComponents = GetSelectedComponents();

            AZ::Entity::ComponentArrayType componentsAfterPaste;
            AZ::Entity::ComponentArrayType componentsInOrder;

            //paste to all selected entities
            for (const AZ::EntityId& entityId : m_selectedEntityIds)
            {
                //get components and order before pasting so we can insert new components above the first selected one
                componentsInOrder.clear();
                GetAllComponentsForEntityInOrder(GetEntity(entityId), componentsInOrder);

                //perform the paste operation which should add new components to the entity or pending list
                EntityCompositionRequestBus::Broadcast(&EntityCompositionRequests::PasteComponentsToEntity, entityId);

                //get the post-paste set of components, which should include all prior components plus new ones
                componentsAfterPaste.clear();
                GetAllComponentsForEntity(entityId, componentsAfterPaste);

                //remove pre-existing components from the post-paste set
                for (auto component : componentsInOrder)
                {
                    componentsAfterPaste.erase(AZStd::remove(componentsAfterPaste.begin(), componentsAfterPaste.end(), component), componentsAfterPaste.end());
                }

                for (auto component : componentsAfterPaste)
                {
                    //add the remaining new components before the first selected one
                    auto insertItr = AZStd::find_first_of(componentsInOrder.begin(), componentsInOrder.end(), selectedComponents.begin(), selectedComponents.end());
                    componentsInOrder.insert(insertItr, component);

                    //only scroll to the bottom if added to the back of the first entity
                    if (insertItr == componentsInOrder.end() && entityId == m_selectedEntityIds.front())
                    {
                        m_shouldScrollToNewComponents = true;
                    }
                }

                SortComponentsByPriority(componentsInOrder);
                SaveComponentOrder(entityId, componentsInOrder);
            }

            QueuePropertyRefresh();
        }
    }

    void EntityPropertyEditor::EnableComponents(const AZ::Entity::ComponentArrayType& components)
    {
        EntityCompositionRequestBus::Broadcast(&EntityCompositionRequests::EnableComponents, components);
        QueuePropertyRefresh();
    }

    void EntityPropertyEditor::EnableComponents()
    {
        EnableComponents(GetSelectedComponents());
    }

    void EntityPropertyEditor::DisableComponents(const AZ::Entity::ComponentArrayType& components)
    {
        EntityCompositionRequestBus::Broadcast(&EntityCompositionRequests::DisableComponents, components);
        QueuePropertyRefresh();
    }

    void EntityPropertyEditor::DisableComponents()
    {
        DisableComponents(GetSelectedComponents());
    }

    void EntityPropertyEditor::MoveComponentsUp()
    {
        const auto& componentsToEdit = GetSelectedComponents();
        if (!AreComponentsRemovable(componentsToEdit))
        {
            return;
        }

        ScopedUndoBatch undoBatch("Move components up.");

        ComponentEditorVector componentEditors = m_componentEditors;
        for (size_t sourceComponentEditorIndex = 1; sourceComponentEditorIndex < componentEditors.size(); ++sourceComponentEditorIndex)
        {
            size_t targetComponentEditorIndex = sourceComponentEditorIndex - 1;
            if (IsMoveAllowed(componentEditors, sourceComponentEditorIndex, targetComponentEditorIndex))
            {
                auto& sourceComponentEditor = componentEditors[sourceComponentEditorIndex];
                auto& targetComponentEditor = componentEditors[targetComponentEditorIndex];
                auto sourceComponents = sourceComponentEditor->GetComponents();
                auto targetComponents = targetComponentEditor->GetComponents();
                MoveComponentRowBefore(sourceComponents, targetComponents, undoBatch);
                AZStd::swap(targetComponentEditor, sourceComponentEditor);
            }
        }

        QueuePropertyRefresh();
    }

    void EntityPropertyEditor::MoveComponentsDown()
    {
        const auto& componentsToEdit = GetSelectedComponents();
        if (!AreComponentsRemovable(componentsToEdit))
        {
            return;
        }

        ScopedUndoBatch undoBatch("Move components down.");

        ComponentEditorVector componentEditors = m_componentEditors;
        for (size_t targetComponentEditorIndex = componentEditors.size() - 1; targetComponentEditorIndex > 0; --targetComponentEditorIndex)
        {
            size_t sourceComponentEditorIndex = targetComponentEditorIndex - 1;
            if (IsMoveAllowed(componentEditors, sourceComponentEditorIndex, targetComponentEditorIndex))
            {
                auto& sourceComponentEditor = componentEditors[sourceComponentEditorIndex];
                auto& targetComponentEditor = componentEditors[targetComponentEditorIndex];
                auto sourceComponents = sourceComponentEditor->GetComponents();
                auto targetComponents = targetComponentEditor->GetComponents();
                MoveComponentRowAfter(sourceComponents, targetComponents, undoBatch);
                AZStd::swap(targetComponentEditor, sourceComponentEditor);
            }
        }

        QueuePropertyRefresh();
    }

    void EntityPropertyEditor::MoveComponentsTop()
    {
        const auto& componentsToEdit = GetSelectedComponents();
        if (!AreComponentsRemovable(componentsToEdit))
        {
            return;
        }

        ScopedUndoBatch undoBatch("Move components to top.");

        AZ::Entity::ComponentArrayType componentsOnEntity;

        for (const auto& entityId : m_selectedEntityIds)
        {
            const auto componentsToEditByEntityIdItr = GetSelectedComponentsByEntityId().find(entityId);
            if (componentsToEditByEntityIdItr != GetSelectedComponentsByEntityId().end())
            {
                undoBatch.MarkEntityDirty(entityId);

                //TODO cache components on entities on inspector update
                componentsOnEntity.clear();
                GetAllComponentsForEntityInOrder(AzToolsFramework::GetEntity(entityId), componentsOnEntity);

                const AZ::Entity::ComponentArrayType& componentsToMove = componentsToEditByEntityIdItr->second;
                AZStd::sort(
                    componentsOnEntity.begin(),
                    componentsOnEntity.end(),
                    [&componentsToMove](const AZ::Component* component1, const AZ::Component* component2)
                    {
                        return
                        AZStd::find(componentsToMove.begin(), componentsToMove.end(), component1) != componentsToMove.end() &&
                        AZStd::find(componentsToMove.begin(), componentsToMove.end(), component2) == componentsToMove.end();
                    });

                SortComponentsByPriority(componentsOnEntity);
                SaveComponentOrder(entityId, componentsOnEntity);
            }
        }

        QueuePropertyRefresh();
    }

    void EntityPropertyEditor::MoveComponentsBottom()
    {
        const auto& componentsToEdit = GetSelectedComponents();
        if (!AreComponentsRemovable(componentsToEdit))
        {
            return;
        }

        ScopedUndoBatch undoBatch("Move components to bottom.");

        AZ::Entity::ComponentArrayType componentsOnEntity;

        for (const auto& entityId : m_selectedEntityIds)
        {
            const auto componentsToEditByEntityIdItr = GetSelectedComponentsByEntityId().find(entityId);
            if (componentsToEditByEntityIdItr != GetSelectedComponentsByEntityId().end())
            {
                undoBatch.MarkEntityDirty(entityId);

                //TODO cache components on entities on inspector update
                componentsOnEntity.clear();
                GetAllComponentsForEntityInOrder(AzToolsFramework::GetEntity(entityId), componentsOnEntity);

                const AZ::Entity::ComponentArrayType& componentsToMove = componentsToEditByEntityIdItr->second;
                AZStd::sort(
                    componentsOnEntity.begin(),
                    componentsOnEntity.end(),
                    [&componentsToMove](const AZ::Component* component1, const AZ::Component* component2)
                    {
                        return
                        AZStd::find(componentsToMove.begin(), componentsToMove.end(), component2) != componentsToMove.end() &&
                        AZStd::find(componentsToMove.begin(), componentsToMove.end(), component1) == componentsToMove.end();
                    });

                SortComponentsByPriority(componentsOnEntity);
                SaveComponentOrder(entityId, componentsOnEntity);
            }
        }

        QueuePropertyRefresh();
    }

    void EntityPropertyEditor::MoveComponentBefore(const AZ::Component* sourceComponent, const AZ::Component* targetComponent, ScopedUndoBatch& undo)
    {
        (void)undo;
        if (sourceComponent && sourceComponent != targetComponent)
        {
            const AZ::Entity* entity = sourceComponent->GetEntity();
            const AZ::EntityId entityId = entity->GetId();
            undo.MarkEntityDirty(entityId);

            AZ::Entity::ComponentArrayType componentsOnEntity;
            GetAllComponentsForEntityInOrder(entity, componentsOnEntity);
            componentsOnEntity.erase(AZStd::remove(componentsOnEntity.begin(), componentsOnEntity.end(), sourceComponent), componentsOnEntity.end());
            auto location = AZStd::find(componentsOnEntity.begin(), componentsOnEntity.end(), targetComponent);
            componentsOnEntity.insert(location, const_cast<AZ::Component*>(sourceComponent));

            SortComponentsByPriority(componentsOnEntity);
            SaveComponentOrder(entityId, componentsOnEntity);
        }
    }

    void EntityPropertyEditor::MoveComponentAfter(const AZ::Component* sourceComponent, const AZ::Component* targetComponent, ScopedUndoBatch& undo)
    {
        (void)undo;
        if (sourceComponent && sourceComponent != targetComponent)
        {
            const AZ::Entity* entity = sourceComponent->GetEntity();
            const AZ::EntityId entityId = entity->GetId();
            undo.MarkEntityDirty(entityId);

            AZ::Entity::ComponentArrayType componentsOnEntity;
            GetAllComponentsForEntityInOrder(entity, componentsOnEntity);
            componentsOnEntity.erase(AZStd::remove(componentsOnEntity.begin(), componentsOnEntity.end(), sourceComponent), componentsOnEntity.end());
            auto location = AZStd::find(componentsOnEntity.begin(), componentsOnEntity.end(), targetComponent);
            if (location != componentsOnEntity.end())
            {
                ++location;
            }

            componentsOnEntity.insert(location, const_cast<AZ::Component*>(sourceComponent));

            SortComponentsByPriority(componentsOnEntity);
            SaveComponentOrder(entityId, componentsOnEntity);
        }
    }

    void EntityPropertyEditor::MoveComponentRowBefore(const AZ::Entity::ComponentArrayType& sourceComponents, const AZ::Entity::ComponentArrayType& targetComponents, ScopedUndoBatch& undo)
    {
        //determine if both sets of components have the same number of elements
        //this should always be the case because component editors are only shown for components with equivalents on all selected entities
        //the only time they should be out of alignment is when dragging to space above or below all the component editors
        const bool targetComponentsAlign = sourceComponents.size() == targetComponents.size();

        //move all source components above target
        //this will account for multiple selected entities and components
        //but we may disallow the operation for multi-select or force it to only apply to the first selected entity
        for (size_t componentIndex = 0; componentIndex < sourceComponents.size(); ++componentIndex)
        {
            const AZ::Component* sourceComponent = sourceComponents[componentIndex];
            const AZ::Component* targetComponent = targetComponentsAlign ? targetComponents[componentIndex] : nullptr;
            MoveComponentBefore(sourceComponent, targetComponent, undo);
        }
    }

    void EntityPropertyEditor::MoveComponentRowAfter(const AZ::Entity::ComponentArrayType& sourceComponents, const AZ::Entity::ComponentArrayType& targetComponents, ScopedUndoBatch& undo)
    {
        //determine if both sets of components have the same number of elements
        //this should always be the case because component editors are only shown for components with equivalents on all selected entities
        //the only time they should be out of alignment is when dragging to space above or below all the component editors
        const bool targetComponentsAlign = sourceComponents.size() == targetComponents.size();

        //move all source components below target
        //this will account for multiple selected entities and components
        //but we may disallow the operation for multi-select or force it to only apply to the first selected entity
        for (size_t componentIndex = 0; componentIndex < sourceComponents.size(); ++componentIndex)
        {
            const AZ::Component* sourceComponent = sourceComponents[componentIndex];
            const AZ::Component* targetComponent = targetComponentsAlign ? targetComponents[componentIndex] : nullptr;
            MoveComponentAfter(sourceComponent, targetComponent, undo);
        }
    }

    bool EntityPropertyEditor::IsMoveAllowed(const ComponentEditorVector& componentEditors) const
    {
        for (size_t sourceComponentEditorIndex = 1; sourceComponentEditorIndex < componentEditors.size(); ++sourceComponentEditorIndex)
        {
            size_t targetComponentEditorIndex = sourceComponentEditorIndex - 1;
            if (IsMoveAllowed(componentEditors, sourceComponentEditorIndex, targetComponentEditorIndex))
            {
                return true;
            }
        }
        return false;
    }

    bool EntityPropertyEditor::IsMoveAllowed(const ComponentEditorVector& componentEditors, size_t sourceComponentEditorIndex, size_t targetComponentEditorIndex) const
    {
        const auto sourceComponentEditor = componentEditors[sourceComponentEditorIndex];
        const auto targetComponentEditor = componentEditors[targetComponentEditorIndex];
        const auto& sourceComponents = sourceComponentEditor->GetComponents();
        const auto& targetComponents = targetComponentEditor->GetComponents();
        return
            sourceComponentEditor->isVisible() &&
            targetComponentEditor->isVisible() &&
            sourceComponentEditor->IsSelected() &&
            !targetComponentEditor->IsSelected() &&
            sourceComponents.size() == m_selectedEntityIds.size() &&
            targetComponents.size() == m_selectedEntityIds.size() &&
            AreComponentsRemovable(sourceComponents) &&
            AreComponentsRemovable(targetComponents);
    }

    bool EntityPropertyEditor::IsMoveComponentsUpAllowed() const
    {
        return IsMoveAllowed(m_componentEditors);
    }

    bool EntityPropertyEditor::IsMoveComponentsDownAllowed() const
    {
        ComponentEditorVector componentEditors = m_componentEditors;
        AZStd::reverse(componentEditors.begin(), componentEditors.end());
        return IsMoveAllowed(componentEditors);
    }

    void EntityPropertyEditor::ResetToSlice()
    {
        // Reset selected components to slice values
        const auto& componentsToEdit = GetSelectedComponents();

        if (!componentsToEdit.empty())
        {
            ScopedUndoBatch undoBatch("Resetting components to slice defaults.");

            for (auto component : componentsToEdit)
            {
                AZ_Assert(component, "Component is invalid.");
                auto componentEditorIterator = m_componentToEditorMap.find(component);
                AZ_Assert(componentEditorIterator != m_componentToEditorMap.end(), "Unable to find a component editor for the given component");
                if (componentEditorIterator != m_componentToEditorMap.end())
                {
                    auto componentEditor = componentEditorIterator->second;
                    componentEditor->GetPropertyEditor()->EnumerateInstances(
                        [this, &component](InstanceDataHierarchy& hierarchy)
                        {
                            InstanceDataNode* root = hierarchy.GetRootNode();
                            ContextMenuActionPullFieldData(component, root);
                        }
                        );
                }
            }

            QueuePropertyRefresh();
        }
    }

    bool EntityPropertyEditor::DoesOwnFocus() const
    {
        QWidget* widget = QApplication::focusWidget();
        return this == widget || isAncestorOf(widget);
    }

    QRect EntityPropertyEditor::GetWidgetGlobalRect(const QWidget* widget) const
    {
        return QRect(
            widget->mapToGlobal(widget->rect().topLeft()),
            widget->mapToGlobal(widget->rect().bottomRight()));
    }

    bool EntityPropertyEditor::DoesIntersectWidget(const QRect& globalRect, const QWidget* widget) const
    {
        return widget->isVisible() && globalRect.intersects(GetWidgetGlobalRect(widget));
    }

    bool EntityPropertyEditor::DoesIntersectSelectedComponentEditor(const QRect& globalRect) const
    {
        for (auto componentEditor : GetIntersectingComponentEditors(globalRect))
        {
            if (componentEditor->IsSelected())
            {
                return true;
            }
        }
        return false;
    }

    bool EntityPropertyEditor::DoesIntersectNonSelectedComponentEditor(const QRect& globalRect) const
    {
        for (auto componentEditor : GetIntersectingComponentEditors(globalRect))
        {
            if (!componentEditor->IsSelected())
            {
                return true;
            }
        }
        return false;
    }

    void EntityPropertyEditor::ClearComponentEditorDragging()
    {
        AZ_PROFILE_FUNCTION(AZ::Debug::ProfileCategory::AzToolsFramework);
        for (auto componentEditor : m_componentEditors)
        {
            componentEditor->SetDragged(false);
            componentEditor->SetDropTarget(false);
        }
        UpdateOverlay();
    }

    void EntityPropertyEditor::ClearComponentEditorSelection()
    {
        AZ_PROFILE_FUNCTION(AZ::Debug::ProfileCategory::AzToolsFramework);
        for (auto componentEditor : m_componentEditors)
        {
            componentEditor->SetSelected(false);
        }
        SaveComponentEditorState();
        UpdateInternalState();
    }

    void EntityPropertyEditor::SelectRangeOfComponentEditors(const AZ::s32 index1, const AZ::s32 index2, bool selected)
    {
        if (index1 >= 0 && index2 >= 0)
        {
            const AZ::s32 min = AZStd::min(index1, index2);
            const AZ::s32 max = AZStd::max(index1, index2);
            for (AZ::s32 index = min; index <= max; ++index)
            {
                m_componentEditors[index]->SetSelected(selected);
            }
            m_componentEditorLastSelectedIndex = index2;
        }
        SaveComponentEditorState();
        UpdateInternalState();
    }

    void EntityPropertyEditor::SelectIntersectingComponentEditors(const QRect& globalRect, bool selected)
    {
        for (auto componentEditor : GetIntersectingComponentEditors(globalRect))
        {
            componentEditor->SetSelected(selected);
            m_componentEditorLastSelectedIndex = GetComponentEditorIndex(componentEditor);
        }
        SaveComponentEditorState();
        UpdateInternalState();
    }

    void EntityPropertyEditor::ToggleIntersectingComponentEditors(const QRect& globalRect)
    {
        for (auto componentEditor : GetIntersectingComponentEditors(globalRect))
        {
            componentEditor->SetSelected(!componentEditor->IsSelected());
            m_componentEditorLastSelectedIndex = GetComponentEditorIndex(componentEditor);
        }
        SaveComponentEditorState();
        UpdateInternalState();
    }

    AZ::s32 EntityPropertyEditor::GetComponentEditorIndex(const ComponentEditor* componentEditor) const
    {
        AZ::s32 index = 0;
        for (auto componentEditorToCompare : m_componentEditors)
        {
            if (componentEditorToCompare == componentEditor)
            {
                return index;
            }
            ++index;
        }
        return -1;
    }

    ComponentEditorVector EntityPropertyEditor::GetIntersectingComponentEditors(const QRect& globalRect) const
    {
        ComponentEditorVector intersectingComponentEditors;
        intersectingComponentEditors.reserve(m_componentEditors.size());
        for (auto componentEditor : m_componentEditors)
        {
            if (DoesIntersectWidget(globalRect, componentEditor))
            {
                intersectingComponentEditors.push_back(componentEditor);
            }
        }
        return intersectingComponentEditors;
    }

    const ComponentEditorVector& EntityPropertyEditor::GetSelectedComponentEditors() const
    {
        return m_selectedComponentEditors;
    }

    const AZ::Entity::ComponentArrayType& EntityPropertyEditor::GetSelectedComponents() const
    {
        return m_selectedComponents;
    }

    const AZStd::unordered_map<AZ::EntityId, AZ::Entity::ComponentArrayType>& EntityPropertyEditor::GetSelectedComponentsByEntityId() const
    {
        return m_selectedComponentsByEntityId;
    }

    void EntityPropertyEditor::UpdateSelectionCache()
    {
        AZ_PROFILE_FUNCTION(AZ::Debug::ProfileCategory::AzToolsFramework);
        m_selectedComponentEditors.clear();
        m_selectedComponentEditors.reserve(m_componentEditors.size());
        for (auto componentEditor : m_componentEditors)
        {
            if (componentEditor->isVisible() && componentEditor->IsSelected())
            {
                m_selectedComponentEditors.push_back(componentEditor);
            }
        }

        m_selectedComponents.clear();
        for (auto componentEditor : m_selectedComponentEditors)
        {
            const auto& components = componentEditor->GetComponents();
            m_selectedComponents.insert(m_selectedComponents.end(), components.begin(), components.end());
        }

        m_selectedComponentsByEntityId.clear();
        for (auto component : m_selectedComponents)
        {
            m_selectedComponentsByEntityId[component->GetEntityId()].push_back(component);
        }
    }

    void EntityPropertyEditor::SaveComponentEditorState()
    {
        AZ_PROFILE_FUNCTION(AZ::Debug::ProfileCategory::AzToolsFramework);
        m_componentEditorSaveStateTable.clear();
        for (auto componentEditor : m_componentEditors)
        {
            SaveComponentEditorState(componentEditor);
        }
    }

    void EntityPropertyEditor::SaveComponentEditorState(ComponentEditor* componentEditor)
    {
        for (auto component : componentEditor->GetComponents())
        {
            AZ::ComponentId componentId = component->GetId();
            ComponentEditorSaveState& state = m_componentEditorSaveStateTable[componentId];
            state.m_expanded = componentEditor->IsExpanded();
            state.m_selected = componentEditor->IsSelected();
        }
    }

    void EntityPropertyEditor::LoadComponentEditorState()
    {
        for (auto componentEditor : m_componentEditors)
        {
            ComponentEditorSaveState state;
            if (!componentEditor->GetComponents().empty())
            {
                AZ::ComponentId componentId = componentEditor->GetComponents().front()->GetId();
                state = m_componentEditorSaveStateTable[componentId];
            }

            componentEditor->SetExpanded(state.m_expanded);
            componentEditor->SetSelected(state.m_selected);
        }
        UpdateOverlay();
    }

    void EntityPropertyEditor::ClearComponentEditorState()
    {
        m_componentEditorSaveStateTable.clear();
    }

    void EntityPropertyEditor::ScrollToNewComponent()
    {
        //force new components to be visible, assuming they are added to the end of the list and layout
        auto componentEditor = GetComponentEditorsFromIndex(m_componentEditorsUsed - 1);
        if (componentEditor)
        {
            m_gui->m_componentList->ensureWidgetVisible(componentEditor);
        }
        m_shouldScrollToNewComponents = false;
        m_shouldScrollToNewComponentsQueued = false;
    }

    void EntityPropertyEditor::QueueScrollToNewComponent()
    {
        if (m_shouldScrollToNewComponents && !m_shouldScrollToNewComponentsQueued)
        {
            QTimer::singleShot(100, this, &EntityPropertyEditor::ScrollToNewComponent);
            m_shouldScrollToNewComponentsQueued = true;
        }
    }

    void EntityPropertyEditor::BuildEntityIconMenu()
    {
        QMenu menu(this);
        menu.setToolTipsVisible(true);

        QAction* action = nullptr;
        action = menu.addAction(QObject::tr("Set default icon"));
        action->setToolTip(tr("Sets the icon to the first component icon after the transform or uses the transform icon"));
        QObject::connect(action, &QAction::triggered, this, &EntityPropertyEditor::SetEntityIconToDefault);

        action = menu.addAction(QObject::tr("Set custom icon"));
        action->setToolTip(tr("Choose a custom icon for this entity"));
        QObject::connect(action, &QAction::triggered, this, &EntityPropertyEditor::PopupAssetBrowserForEntityIcon);

        menu.exec(GetWidgetGlobalRect(m_gui->m_entityIcon).bottomLeft());
    }

    void EntityPropertyEditor::SetEntityIconToDefault()
    {
        AzToolsFramework::ScopedUndoBatch undoBatch("Change Entity Icon");

        AZ::Data::AssetId invalidAssetId;
        for (AZ::EntityId entityId : m_selectedEntityIds)
        {
            EditorEntityIconComponentRequestBus::Event(entityId, &EditorEntityIconComponentRequestBus::Events::SetEntityIconAsset, invalidAssetId);
        }

        QueuePropertyRefresh();
    }

    void EntityPropertyEditor::PopupAssetBrowserForEntityIcon()
    {
        /*
        * Request the AssetBrowser Dialog and set a type filter
        */

        if (m_selectedEntityIds.empty())
        {
            return;
        }

        AZ::Data::AssetType entityIconAssetType("{3436C30E-E2C5-4C3B-A7B9-66C94A28701B}");
        AssetSelectionModel selection = AssetSelectionModel::AssetTypeSelection(entityIconAssetType);

        AZ::Data::AssetId currentEntityIconAssetId;
        EditorEntityIconComponentRequestBus::EventResult(currentEntityIconAssetId, m_selectedEntityIds.front(), &EditorEntityIconComponentRequestBus::Events::GetEntityIconAssetId);
        selection.SetSelectedAssetId(currentEntityIconAssetId);
        EditorRequests::Bus::Broadcast(&EditorRequests::BrowseForAssets, selection);

        if (selection.IsValid())
        {
            auto product = azrtti_cast<const ProductAssetBrowserEntry*>(selection.GetResult());
            if (product)
            {
                AzToolsFramework::ScopedUndoBatch undoBatch("Change Entity Icon");

                AZ::Data::AssetId iconAssetId = product->GetAssetId();
                for (AZ::EntityId entityId : m_selectedEntityIds)
                {
                    EditorEntityIconComponentRequestBus::Event(entityId, &EditorEntityIconComponentRequestBus::Events::SetEntityIconAsset, iconAssetId);
                }

                QueuePropertyRefresh();
            }
            else
            {
                AZ_Warning("Property Editor", false, "Incorrect asset selected. Expected entity icon asset.");
            }
        }
    }

    void EntityPropertyEditor::OnInitiallyActiveChanged(int)
    {
        for (AZ::EntityId entityId : m_selectedEntityIds)
        {
            AZ::Entity* entity = nullptr;
            AZ::ComponentApplicationBus::BroadcastResult(entity, &AZ::ComponentApplicationBus::Events::FindEntity, entityId);

            if (entity)
            {
                entity->SetRuntimeActiveByDefault(m_gui->m_initiallyActiveCheckbox->isChecked());
            }
        }

        UpdateInitiallyActiveDisplay();
    }

    void EntityPropertyEditor::UpdateOverlay()
    {
        m_overlay->setVisible(true);
        m_overlay->setGeometry(m_gui->m_componentListContents->rect());
        m_overlay->raise();
        m_overlay->update();
    }

    void EntityPropertyEditor::resizeEvent(QResizeEvent* event)
    {
        QWidget::resizeEvent(event);
        UpdateOverlay();
    }

    void EntityPropertyEditor::contextMenuEvent(QContextMenuEvent* event)
    {
        OnDisplayComponentEditorMenu(event->globalPos());
        event->accept();
    }

    //overridden to intercept application level mouse events for component editor selection
    bool EntityPropertyEditor::eventFilter(QObject* object, QEvent* event)
    {
        HandleSelectionEvents(object, event);
        return false;
    }

    void EntityPropertyEditor::mousePressEvent(QMouseEvent* event)
    {
        ResetDrag(event);
    }

    void EntityPropertyEditor::mouseReleaseEvent(QMouseEvent* event)
    {
        ResetDrag(event);
    }

    void EntityPropertyEditor::mouseMoveEvent(QMouseEvent* event)
    {
        if (m_isAlreadyQueuedRefresh)
        {
            // not allowed to start operations when we are rebuilding the tree
            return;
        }
        StartDrag(event);
    }

    void EntityPropertyEditor::dragEnterEvent(QDragEnterEvent* event)
    {
        if (UpdateDrag(event->pos(), event->mouseButtons(), event->mimeData()))
        {
            event->accept();
        }
        else
        {
            event->ignore();
        }
    }

    void EntityPropertyEditor::dragMoveEvent(QDragMoveEvent* event)
    {
        if (UpdateDrag(event->pos(), event->mouseButtons(), event->mimeData()))
        {
            event->accept();
        }
        else
        {
            event->ignore();
        }
    }

    void EntityPropertyEditor::dropEvent(QDropEvent* event)
    {
        HandleDrop(event);
    }

    bool EntityPropertyEditor::HandleSelectionEvents(QObject* object, QEvent* event)
    {
        // if we're in the middle of a tree rebuild, we can't afford to touch the internals
        if (m_isAlreadyQueuedRefresh)
        {
            return false;
        }

        (void)object;
        if (m_dragStarted || m_selectedEntityIds.empty())
        {
            return false;
        }

        if (event->type() != QEvent::MouseButtonPress &&
            event->type() != QEvent::MouseButtonDblClick &&
            event->type() != QEvent::MouseButtonRelease)
        {
            return false;
        }

        QMouseEvent* mouseEvent = static_cast<QMouseEvent*>(event);

        //selection now occurs on mouse released
        //reset selection flag when mouse is clicked to allow additional selection changes
        if (event->type() == QEvent::MouseButtonPress)
        {
            m_selectionEventAccepted = false;
            return false;
        }

        //reject input if selection already occurred for this click
        if (m_selectionEventAccepted)
        {
            return false;
        }

        //reject input if a popup or modal window is active
        //this does not apply to menues, because then we can not select an item
        //while right clicking on it (context menu pops up first, selection won't happen)
        if (QApplication::activeModalWidget() != nullptr ||
            (QApplication::activePopupWidget() != nullptr &&
             qobject_cast<QMenu*>(QApplication::activePopupWidget()) == nullptr) ||
            QApplication::focusWidget() == m_componentPalette)
        {
            return false;
        }

        // if we were clicking on a menu (just has been opened) remove the event filter
        // as long as it is opened, otherwise clicking on the menu might result in a change
        // of selection before the action is fired
        if (QMenu* menu = qobject_cast<QMenu*>(QApplication::activePopupWidget()))
        {
            qApp->removeEventFilter(this);
            connect(menu, &QMenu::aboutToHide, this, [this]() {
                qApp->installEventFilter(this);
            });
            return false; // also get out of here without eating this specific event.
        }

        const QRect globalRect(mouseEvent->globalPos(), mouseEvent->globalPos());

        //reject input outside of the inspector's component list
        if (!DoesOwnFocus() ||
            !DoesIntersectWidget(globalRect, m_gui->m_componentList))
        {
            return false;
        }

        //reject input from other buttons
        if ((mouseEvent->button() != Qt::LeftButton) &&
            (mouseEvent->button() != Qt::RightButton))
        {
            return false;
        }

        //right click is allowed if the component editor under the mouse is not selected
        if (mouseEvent->button() == Qt::RightButton)
        {
            if (DoesIntersectSelectedComponentEditor(globalRect))
            {
                return false;
            }

            ClearComponentEditorSelection();
            SelectIntersectingComponentEditors(globalRect, true);
        }
        else if (mouseEvent->button() == Qt::LeftButton)
        {
            //if shift or control is pressed this is a multi=-select operation, otherwise reset the selection
            if (mouseEvent->modifiers() & Qt::ControlModifier)
            {
                ToggleIntersectingComponentEditors(globalRect);
            }
            else if (mouseEvent->modifiers() & Qt::ShiftModifier)
            {
                ComponentEditorVector intersections = GetIntersectingComponentEditors(globalRect);
                if (!intersections.empty())
                {
                    SelectRangeOfComponentEditors(m_componentEditorLastSelectedIndex, GetComponentEditorIndex(intersections.front()), true);
                }
            }
            else
            {
                ClearComponentEditorSelection();
                SelectIntersectingComponentEditors(globalRect, true);
            }
        }

        UpdateInternalState();

        //ensure selection logic executes only once per click since eventFilter may execute multiple times for a single click
        m_selectionEventAccepted = true;
        return true;
    }


    QRect EntityPropertyEditor::GetInflatedRectFromPoint(const QPoint& point, int radius) const
    {
        return QRect(point - QPoint(radius, radius), point + QPoint(radius, radius));
    }

    bool EntityPropertyEditor::IsDragAllowed(const ComponentEditorVector& componentEditors) const
    {
        if (m_dragStarted || m_selectedEntityIds.empty() || componentEditors.empty() || !DoesOwnFocus())
        {
            return false;
        }

        for (auto componentEditor : componentEditors)
        {
            if (!componentEditor ||
                !componentEditor->isVisible() ||
                !AreComponentsRemovable(componentEditor->GetComponents()))
            {
                return false;
            }
        }

        return true;
    }

    // this helper function checks what can be spawned in mimedata and only calls the callback for the creatable ones.
    void EntityPropertyEditor::GetCreatableAssetEntriesFromMimeData(const QMimeData* mimeData, ProductCallback callbackFunction) const
    {
        if (mimeData->hasFormat(AssetBrowser::AssetBrowserEntry::GetMimeType()))
        {
            // extra special case:  MTLs from FBX drags are ignored.  are we dragging a FBX file?
            bool isDraggingFBXFile = false;
            AssetBrowser::AssetBrowserEntry::ForEachEntryInMimeData<AssetBrowser::SourceAssetBrowserEntry>(mimeData, [&](const AssetBrowser::SourceAssetBrowserEntry* source)
            {
                isDraggingFBXFile = isDraggingFBXFile || AzFramework::StringFunc::Equal(source->GetExtension().c_str(), ".fbx", false);
            });

            // the usual case - we only allow asset browser drops of assets that have actually been associated with a kind of component.
            AssetBrowser::AssetBrowserEntry::ForEachEntryInMimeData<AssetBrowser::ProductAssetBrowserEntry>(mimeData, [&](const AssetBrowser::ProductAssetBrowserEntry* product)
            {
                bool canCreateComponent = false;
                AZ::AssetTypeInfoBus::EventResult(canCreateComponent, product->GetAssetType(), &AZ::AssetTypeInfo::CanCreateComponent, product->GetAssetId());

                AZ::Uuid componentTypeId = AZ::Uuid::CreateNull();
                AZ::AssetTypeInfoBus::EventResult(componentTypeId, product->GetAssetType(), &AZ::AssetTypeInfo::GetComponentTypeId);

                if (canCreateComponent && !componentTypeId.IsNull())
                {
                    // we have a component type that handles this asset.
                    // but we disallow it if its a MTL file from a FBX and the FBX itself is being dragged.  Its still allowed
                    // to drag the actual MTL.
                    EBusFindAssetTypeByName materialAssetTypeResult("Material");
                    AZ::AssetTypeInfoBus::BroadcastResult(materialAssetTypeResult, &AZ::AssetTypeInfo::GetAssetType);
                    AZ::Data::AssetType materialAssetType = materialAssetTypeResult.GetAssetType();

                    if ((!isDraggingFBXFile) || (product->GetAssetType() != materialAssetType))
                    {
                        callbackFunction(product);
                    }
                }
            });
        }
    }

    bool EntityPropertyEditor::IsDropAllowed(const QMimeData* mimeData, const QPoint& globalPos) const
    {
        if (m_isAlreadyQueuedRefresh)
        {
            return false; // can't drop while tree is rebuilding itself!
        }

        const QRect globalRect(globalPos, globalPos);
        if (!mimeData || m_selectedEntityIds.empty() || !DoesIntersectWidget(globalRect, this))
        {
            return false;
        }

        if (mimeData->hasFormat(kComponentEditorIndexMimeType))
        {
            return IsDropAllowedForComponentReorder(mimeData, globalPos);
        }

        if (!mimeData->hasFormat(ComponentTypeMimeData::GetMimeType()) &&
            !mimeData->hasFormat(ComponentAssetMimeDataContainer::GetMimeType()) &&
            !mimeData->hasFormat(AssetBrowser::AssetBrowserEntry::GetMimeType()))
        {
            return false;
        }

        if (mimeData->hasFormat(AssetBrowser::AssetBrowserEntry::GetMimeType()))
        {
            bool hasAny = false;
            GetCreatableAssetEntriesFromMimeData(mimeData, [&](const AssetBrowser::ProductAssetBrowserEntry* /*entry*/)
            {
                hasAny = true;
            });

            if (!hasAny)
            {
                return false;
            }
        }
        return true;
    }

    bool EntityPropertyEditor::IsDropAllowedForComponentReorder(const QMimeData* mimeData, const QPoint& globalPos) const
    {
        //drop requires a buffer for intersection tests due to spacing between editors
        const QRect globalRect(GetInflatedRectFromPoint(globalPos, kComponentEditorDropTargetPrecision));
        if (!mimeData || !mimeData->hasFormat(kComponentEditorIndexMimeType) || !DoesIntersectWidget(globalRect, this))
        {
            return false;
        }

        return true;
    }

    bool EntityPropertyEditor::CreateComponentWithAsset(const AZ::Uuid& componentType, const AZ::Data::AssetId& assetId)
    {
        if (m_selectedEntityIds.empty() || componentType.IsNull())
        {
            return false;
        }

        EntityCompositionRequests::AddComponentsOutcome addComponentsOutcome = AZ::Failure(AZStd::string("AddComponentsToEntities is not handled"));
        EntityCompositionRequestBus::BroadcastResult(addComponentsOutcome, &EntityCompositionRequests::AddComponentsToEntities, m_selectedEntityIds, AZ::ComponentTypeList({ componentType }));
        if (!addComponentsOutcome)
        {
            return false;
        }

        for (auto entityId : m_selectedEntityIds)
        {
            // Add Component metrics event (Drag+Drop from File Browser/Asset Browser to Entity Inspector)
            EBUS_EVENT(EditorMetricsEventsBus, ComponentAdded, entityId, componentType);

            auto addedComponent = addComponentsOutcome.GetValue()[entityId].m_componentsAdded[0];
            AZ_Assert(GetComponentTypeId(addedComponent) == componentType, "Added component returned was not the type requested to add");

            auto editorComponent = GetEditorComponent(addedComponent);
            if (editorComponent)
            {
                editorComponent->SetPrimaryAsset(assetId);
            }
        }

        QueuePropertyRefresh();
        return true;
    }

    ComponentEditor* EntityPropertyEditor::GetReorderDropTarget(const QRect& globalRect) const
    {
        const ComponentEditorVector& targetComponentEditors = GetIntersectingComponentEditors(globalRect);
        ComponentEditor* targetComponentEditor = !targetComponentEditors.empty() ? targetComponentEditors.back() : nullptr;

        //continue to move to the next editor if the current target is being dragged, not removable, or we overlap its lower half
        auto targetItr = AZStd::find(m_componentEditors.begin(), m_componentEditors.end(), targetComponentEditor);
        while (targetComponentEditor
            && (targetComponentEditor->IsDragged()
                || !AreComponentsRemovable(targetComponentEditor->GetComponents())
                || (globalRect.center().y() > GetWidgetGlobalRect(targetComponentEditor).center().y())))
        {
            if (targetItr == m_componentEditors.end() || targetComponentEditor == m_componentEditors.back() || !targetComponentEditor->isVisible())
            {
                targetComponentEditor = nullptr;
                break;
            }

            targetComponentEditor = *(++targetItr);
        }
        return targetComponentEditor;
    }

    bool EntityPropertyEditor::ResetDrag(QMouseEvent* event)
    {
        const QPoint globalPos(event->globalPos());
        const QRect globalRect(globalPos, globalPos);

        //additional checks since handling is done in event filter
        m_dragStartPosition = globalPos;
        m_dragStarted = false;
        ClearComponentEditorDragging();
        ResetAutoScroll();
        return true;
    }

    bool EntityPropertyEditor::UpdateDrag(const QPoint& localPos, Qt::MouseButtons mouseButtons, const QMimeData* mimeData)
    {
        const QPoint globalPos(mapToGlobal(localPos));
        const QRect globalRect(globalPos, globalPos);

        //reset drop indicators
        for (auto componentEditor : m_componentEditors)
        {
            componentEditor->SetDropTarget(false);
        }
        UpdateOverlay();

        //additional checks since handling is done in event filter
        if ((mouseButtons& Qt::LeftButton) && DoesIntersectWidget(globalRect, this))
        {
            if (IsDropAllowed(mimeData, globalPos))
            {
                QueueAutoScroll();

                //update drop indicators for detected drop targets
                if (IsDropAllowedForComponentReorder(mimeData, globalPos))
                {
                    ComponentEditor* targetComponentEditor = GetReorderDropTarget(
                        GetInflatedRectFromPoint(globalPos, kComponentEditorDropTargetPrecision));

                    if (targetComponentEditor)
                    {
                        targetComponentEditor->SetDropTarget(true);
                        UpdateOverlay();
                    }
                }

                return true;
            }
        }
        return false;
    }

    bool EntityPropertyEditor::StartDrag(QMouseEvent* event)
    {
        const QPoint globalPos(event->globalPos());
        const QRect globalRect(globalPos, globalPos);

        //additional checks since handling is done in event filter
        if (m_dragStarted || !(event->buttons() & Qt::LeftButton) || !DoesIntersectWidget(globalRect, this))
        {
            return false;
        }

        //determine if the mouse moved enough to register as a drag
        if ((globalPos - m_dragStartPosition).manhattanLength() < QApplication::startDragDistance())
        {
            return false;
        }

        //if dragging a selected editor, drag all selected
        //otherwise, only drag the one under the cursor
        const QRect dragRect(m_dragStartPosition, m_dragStartPosition);
        const bool dragSelected = DoesIntersectSelectedComponentEditor(dragRect);
        const auto& componentEditors = dragSelected ? GetSelectedComponentEditors() : GetIntersectingComponentEditors(dragRect);
        if (!IsDragAllowed(componentEditors))
        {
            return false;
        }

        //drag must be initiated from a component header since property editor may have conflicting widgets
        bool intersectsHeader = false;
        for (auto componentEditor : componentEditors)
        {
            if (DoesIntersectWidget(dragRect, reinterpret_cast<QWidget*>(componentEditor->GetHeader())))
            {
                intersectsHeader = true;
            }
        }

        if (!intersectsHeader)
        {
            return false;
        }

        m_dragStarted = true;

        QDrag* drag = new QDrag(this);
        QMimeData* mimeData = new QMimeData();

        QRect dragImageRect;

        AZStd::vector<AZ::s32> componentEditorIndices;
        componentEditorIndices.reserve(componentEditors.size());
        for (auto componentEditor : componentEditors)
        {
            //compute the drag image size
            if (componentEditorIndices.empty())
            {
                dragImageRect = componentEditor->rect();
            }
            else
            {
                dragImageRect.setHeight(dragImageRect.height() + componentEditor->rect().height());
            }

            //add component editor index to drag data
            auto componentEditorIndex = GetComponentEditorIndex(componentEditor);
            if (componentEditorIndex >= 0)
            {
                componentEditorIndices.push_back(GetComponentEditorIndex(componentEditor));
            }
        }

        //build image from dragged editor UI
        QImage dragImage(dragImageRect.size(), QImage::Format_ARGB32_Premultiplied);
        QPainter painter(&dragImage);
        painter.setCompositionMode(QPainter::CompositionMode_Source);
        painter.fillRect(dragImageRect, Qt::transparent);
        painter.setCompositionMode(QPainter::CompositionMode_SourceOver);
        painter.setOpacity(0.5f);

        //render a vertical stack of component editors, may change to render just the headers
        QPoint dragImageOffset(0, 0);
        for (AZ::s32 index : componentEditorIndices)
        {
            auto componentEditor = GetComponentEditorsFromIndex(index);
            if (componentEditor)
            {
                if (DoesIntersectWidget(dragRect, componentEditor))
                {
                    //offset drag image from the drag start position
                    drag->setHotSpot(dragImageOffset + (m_dragStartPosition - GetWidgetGlobalRect(componentEditor).topLeft()));
                }

                //render the component editor to the drag image
                componentEditor->render(&painter, dragImageOffset);

                //update the render offset by the component editor height
                dragImageOffset.setY(dragImageOffset.y() + componentEditor->rect().height());
            }
        }
        painter.end();

        //mark dragged components after drag initiated to draw indicators
        for (AZ::s32 index : componentEditorIndices)
        {
            auto componentEditor = GetComponentEditorsFromIndex(index);
            if (componentEditor)
            {
                componentEditor->SetDragged(true);
            }
        }
        UpdateOverlay();

        //encode component editor indices as internal drag data
        mimeData->setData(
            kComponentEditorIndexMimeType,
            QByteArray(reinterpret_cast<char*>(componentEditorIndices.data()), static_cast<int>(componentEditorIndices.size() * sizeof(AZ::s32))));

        drag->setMimeData(mimeData);
        drag->setPixmap(QPixmap::fromImage(dragImage));
        drag->exec(Qt::MoveAction, Qt::MoveAction);

        //mark dragged components after drag completed to stop drawing indicators
        for (AZ::s32 index : componentEditorIndices)
        {
            auto componentEditor = GetComponentEditorsFromIndex(index);
            if (componentEditor)
            {
                componentEditor->SetDragged(false);
            }
        }
        UpdateOverlay();
        return true;
    }

    bool EntityPropertyEditor::HandleDrop(QDropEvent* event)
    {
        const QPoint globalPos(mapToGlobal(event->pos()));
        const QMimeData* mimeData = event->mimeData();
        if (IsDropAllowed(mimeData, globalPos))
        {
            // Navigation triggered - Drag+Drop from Component Palette/File Browser/Asset Browser to Entity Inspector
            EditorMetricsEventsBusAction editorMetricsEventsBusActionWrapper(EditorMetricsEventsBusTraits::NavigationTrigger::DragAndDrop);

            //handle drop for supported mime types
            HandleDropForComponentTypes(event);
            HandleDropForComponentAssets(event);
            HandleDropForAssetBrowserEntries(event);
            HandleDropForComponentReorder(event);
            event->acceptProposedAction();
            return true;
        }
        return false;
    }

    bool EntityPropertyEditor::HandleDropForComponentTypes(QDropEvent* event)
    {
        const QMimeData* mimeData = event->mimeData();
        if (mimeData && mimeData->hasFormat(ComponentTypeMimeData::GetMimeType()))
        {
            ComponentTypeMimeData::ClassDataContainer classDataContainer;
            ComponentTypeMimeData::Get(mimeData, classDataContainer);

            ScopedUndoBatch undo("Add component with type");

            //create new components from dropped component types
            for (auto componentClass : classDataContainer)
            {
                if (componentClass)
                {
                    CreateComponentWithAsset(componentClass->m_typeId, AZ::Data::AssetId());
                }
            }
            return true;
        }
        return false;
    }

    bool EntityPropertyEditor::HandleDropForComponentAssets(QDropEvent* event)
    {
        const QMimeData* mimeData = event->mimeData();
        if (mimeData && mimeData->hasFormat(ComponentAssetMimeDataContainer::GetMimeType()))
        {
            ComponentAssetMimeDataContainer mimeContainer;
            if (mimeContainer.FromMimeData(event->mimeData()))
            {
                ScopedUndoBatch undo("Add component with asset");

                //create new components from dropped assets
                for (const auto& asset : mimeContainer.m_assets)
                {
                    CreateComponentWithAsset(asset.m_classId, asset.m_assetId);
                }
            }
            return true;
        }
        return false;
    }

    bool EntityPropertyEditor::HandleDropForAssetBrowserEntries(QDropEvent* event)
    {
        // do we need to create an undo?
        bool createdAny = false;
        const QMimeData* mimeData = event->mimeData();
        if ((!mimeData) || (!mimeData->hasFormat(AssetBrowser::AssetBrowserEntry::GetMimeType())))
        {
            return false;
        }

        // before we start, find out whether any in the mimedata are even appropriate so we can execute an undo
        // note that the product pointers are only valid during this callback, so there is no point in caching them here.
        GetCreatableAssetEntriesFromMimeData(mimeData,
            [&](const AssetBrowser::ProductAssetBrowserEntry* /*product*/)
            {
                createdAny = true;
            });

        if (createdAny)
        {
            // create one undo to wrap the entire operation since we detected a valid asset.
            ScopedUndoBatch undo("Add component from asset browser");
            GetCreatableAssetEntriesFromMimeData(mimeData,
                [&](const AssetBrowser::ProductAssetBrowserEntry* product)
                {
                    AZ::Uuid componentType;
                    AZ::AssetTypeInfoBus::EventResult(componentType, product->GetAssetType(), &AZ::AssetTypeInfo::GetComponentTypeId);
                    CreateComponentWithAsset(componentType, product->GetAssetId());
                });
            return true;
        }

        return false;
    }

    bool EntityPropertyEditor::HandleDropForComponentReorder(QDropEvent* event)
    {
        const QMimeData* mimeData = event->mimeData();
        if (mimeData && mimeData->hasFormat(kComponentEditorIndexMimeType))
        {
            ScopedUndoBatch undo("Reorder components");

            const QPoint globalPos(mapToGlobal(event->pos()));
            const QRect globalRect(GetInflatedRectFromPoint(globalPos, kComponentEditorDropTargetPrecision));

            //get component editor(s) to drop on targets, moving the associated components above the target
            const ComponentEditorVector& sourceComponentEditors = GetComponentEditorsFromIndices(ExtractComponentEditorIndicesFromMimeData(mimeData));

            //get component editor(s) where drop will occur
            ComponentEditor* targetComponentEditor = GetReorderDropTarget(
                GetInflatedRectFromPoint(globalPos, kComponentEditorDropTargetPrecision));

            const AZ::Entity::ComponentArrayType& targetComponents = targetComponentEditor ? targetComponentEditor->GetComponents() : AZ::Entity::ComponentArrayType();

            for (const ComponentEditor* sourceComponentEditor : sourceComponentEditors)
            {
                const AZ::Entity::ComponentArrayType& sourceComponents = sourceComponentEditor->GetComponents();
                MoveComponentRowBefore(sourceComponents, targetComponents, undo);
            }

            QueuePropertyRefresh();
            return true;
        }
        return false;
    }

    AZStd::vector<AZ::s32> EntityPropertyEditor::ExtractComponentEditorIndicesFromMimeData(const QMimeData* mimeData) const
    {
        //build vector of indices from mime data
        const QByteArray sourceComponentEditorData = mimeData->data(kComponentEditorIndexMimeType);
        const auto indexCount = sourceComponentEditorData.size() / sizeof(AZ::s32);
        const auto indexStart = reinterpret_cast<const AZ::s32*>(sourceComponentEditorData.data());
        const auto indexEnd = indexStart + indexCount;
        return AZStd::vector<AZ::s32>(indexStart, indexEnd);
    }

    ComponentEditorVector EntityPropertyEditor::GetComponentEditorsFromIndices(const AZStd::vector<AZ::s32>& indices) const
    {
        ComponentEditorVector componentEditors;
        componentEditors.reserve(indices.size());
        for (auto index : indices)
        {
            auto componentEditor = GetComponentEditorsFromIndex(index);
            if (componentEditor)
            {
                componentEditors.push_back(componentEditor);
            }
        }
        return componentEditors;
    }

    ComponentEditor* EntityPropertyEditor::GetComponentEditorsFromIndex(const AZ::s32 index) const
    {
        return index >= 0 && index < m_componentEditors.size() ? m_componentEditors[index] : nullptr;
    }

    void EntityPropertyEditor::ResetAutoScroll()
    {
        m_autoScrollCount = 0;
        m_autoScrollQueued = false;
    }

    void EntityPropertyEditor::QueueAutoScroll()
    {
        if (!m_autoScrollQueued)
        {
            m_autoScrollQueued = true;
            QTimer::singleShot(0, this, &EntityPropertyEditor::UpdateAutoScroll);
        }
    }

    void EntityPropertyEditor::UpdateAutoScroll()
    {
        if (m_autoScrollQueued)
        {
            m_autoScrollQueued = false;

            // find how much we should scroll with
            QScrollBar* verticalScroll = m_gui->m_componentList->verticalScrollBar();
            QScrollBar* horizontalScroll = m_gui->m_componentList->horizontalScrollBar();

            int verticalStep = verticalScroll->pageStep();
            int horizontalStep = horizontalScroll->pageStep();
            if (m_autoScrollCount < qMax(verticalStep, horizontalStep))
            {
                ++m_autoScrollCount;
            }

            int verticalValue = verticalScroll->value();
            int horizontalValue = horizontalScroll->value();

            QPoint pos = QCursor::pos();
            QRect area = GetWidgetGlobalRect(m_gui->m_componentList);

            // do the scrolling if we are in the scroll margins
            if (pos.y() - area.top() < m_autoScrollMargin)
            {
                verticalScroll->setValue(verticalValue - m_autoScrollCount);
            }
            else if (area.bottom() - pos.y() < m_autoScrollMargin)
            {
                verticalScroll->setValue(verticalValue + m_autoScrollCount);
            }
            if (pos.x() - area.left() < m_autoScrollMargin)
            {
                horizontalScroll->setValue(horizontalValue - m_autoScrollCount);
            }
            else if (area.right() - pos.x() < m_autoScrollMargin)
            {
                horizontalScroll->setValue(horizontalValue + m_autoScrollCount);
            }

            // if nothing changed, stop scrolling
            if (verticalValue != verticalScroll->value() || horizontalValue != horizontalScroll->value())
            {
                UpdateOverlay();
                QueueAutoScroll();
            }
            else
            {
                ResetAutoScroll();
            }
        }
    }

    void EntityPropertyEditor::UpdateInternalState()
    {
        UpdateSelectionCache();
        UpdateActions();
        UpdateOverlay();
    }

    void EntityPropertyEditor::OnSearchTextChanged()
    {
        m_filterString = m_gui->m_entitySearchBox->text().toLatin1().data();
        UpdateContents();
        m_gui->m_buttonClearFilter->setIcon(m_filterString.size() == 0 ?
            m_emptyIcon :
            m_clearIcon);
    }

    void EntityPropertyEditor::ClearSearchFilter()
    {
        m_gui->m_entitySearchBox->setText("");
    }

    void EntityPropertyEditor::OpenPinnedInspector()
    {
        AzToolsFramework::EntityIdList selectedEntities;
        GetSelectedEntities(selectedEntities);

        EBUS_EVENT(AzToolsFramework::EditorRequests::Bus, OpenPinnedInspector, selectedEntities);
    }

    void EntityPropertyEditor::OnContextReset()
    {
        if (IsLockedToSpecificEntities())
        {
            CloseInspectorWindow();
        }
    }

    void EntityPropertyEditor::CloseInspectorWindow()
    {
        for (auto& entityId : m_overrideSelectedEntityIds)
        {
            DisconnectFromEntityBuses(entityId);
        }
        EBUS_EVENT(AzToolsFramework::EditorRequests::Bus, ClosePinnedInspector, this);
    }

    void EntityPropertyEditor::SetSystemEntityEditor(bool isSystemEntityEditor)
    {
        m_isSystemEntityEditor = isSystemEntityEditor;
        UpdateContents();
    }

    void EntityPropertyEditor::OnEditorEntitiesReplacedBySlicedEntities(const AZStd::unordered_map<AZ::EntityId, AZ::EntityId>& replacedEntitiesMap)
    {
        if (IsLockedToSpecificEntities())
        {
            bool entityIdChanged = false;
            EntityIdList newOverrideSelectedEntityIds;
            newOverrideSelectedEntityIds.reserve(m_overrideSelectedEntityIds.size());

            for (AZ::EntityId& entityId : m_overrideSelectedEntityIds)
            {
                auto found = replacedEntitiesMap.find(entityId);
                if (found != replacedEntitiesMap.end())
                {
                    entityIdChanged = true;
                    newOverrideSelectedEntityIds.push_back(found->second);

                    DisconnectFromEntityBuses(entityId);
                    ConnectToEntityBuses(found->second);
                }
                else
                {
                    newOverrideSelectedEntityIds.push_back(entityId);
                }
            }

            if (entityIdChanged)
            {
                m_overrideSelectedEntityIds = newOverrideSelectedEntityIds;
                UpdateContents();
            }
        }
    }

    void EntityPropertyEditor::OnEntityComponentPropertyChanged(AZ::ComponentId componentId)
    {
        // When m_initiatingPropertyChangeNotification is true, it means this EntityPropertyEditor was
        // the one that is broadcasting this property change, so no need to refresh our values
        if (m_initiatingPropertyChangeNotification)
        {
            return;
        }

        for (auto componentEditor : m_componentEditors)
        {
            if (componentEditor->isVisible() && componentEditor->HasComponentWithId(componentId))
            {
                componentEditor->QueuePropertyEditorInvalidation(AzToolsFramework::PropertyModificationRefreshLevel::Refresh_Values);
                break;
            }
        }
    }

    void EntityPropertyEditor::OnComponentOrderChanged()
    {
        QueuePropertyRefresh();
        m_shouldScrollToNewComponents = true;
    }

    void EntityPropertyEditor::ConnectToEntityBuses(const AZ::EntityId& entityId)
    {
        AzToolsFramework::EditorInspectorComponentNotificationBus::MultiHandler::BusConnect(entityId);
        AzToolsFramework::PropertyEditorEntityChangeNotificationBus::MultiHandler::BusConnect(entityId);
    }

    void EntityPropertyEditor::DisconnectFromEntityBuses(const AZ::EntityId& entityId)
    {
        AzToolsFramework::EditorInspectorComponentNotificationBus::MultiHandler::BusDisconnect(entityId);
        AzToolsFramework::PropertyEditorEntityChangeNotificationBus::MultiHandler::BusDisconnect(entityId);
    }


}

#include <UI/PropertyEditor/EntityPropertyEditor.moc>