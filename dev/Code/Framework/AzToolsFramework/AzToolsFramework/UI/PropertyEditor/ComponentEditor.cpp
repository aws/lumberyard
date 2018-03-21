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
#include "ComponentEditor.hxx"
#include "ComponentEditorHeader.hxx"
#include "ComponentEditorNotification.hxx"

#include <AzCore/RTTI/AttributeReader.h>
#include <AzCore/std/containers/map.h>
#include <AzCore/std/containers/unordered_set.h>
#include <AzToolsFramework/API/EntityCompositionRequestBus.h>
#include <AzToolsFramework/API/ToolsApplicationAPI.h>
#include <AzToolsFramework/Entity/EditorEntityHelpers.h>
#include <AzToolsFramework/ToolsComponents/GenericComponentWrapper.h>
#include <AzToolsFramework/UI/PropertyEditor/ReflectedPropertyEditor.hxx>

#include "EntityIdQLabel.hxx"
#include <QDesktopWidget>
#include <QMenu>
#include <QPushButton>
#include <QVBoxLayout>

namespace AzToolsFramework
{
    namespace ComponentEditorConstants
    {
        static const int kPropertyLabelWidth = 160; // Width of property label column in property grid.
        static const char* kUnknownComponentTitle = "-";

        // names for widgets so they can be found in stylesheet
        static const char* kHeaderId = "Header";
        static const char* kPropertyEditorId = "PropertyEditor";

        static const int kAddComponentMenuHeight = 150;
    }

    template<typename T>
    void AddUniqueItemToContainer(AZStd::vector<T>& container, const T& element)
    {
        if (AZStd::find(container.begin(), container.end(), element) == container.end())
        {
            container.push_back(element);
        }
    }

    template<typename ContainerType1, typename ContainerType2>
    bool DoContainersIntersect(const ContainerType1& container1, const ContainerType2& container2)
    {
        return AZStd::find_first_of(container1.begin(), container1.end(), container2.begin(), container2.end()) != container1.end();
    }

    //generate a list of all valid or pending sibling components that are service-incompatible with the specified component
    AZStd::unordered_set<AZ::Component*> GetRelatedIncompatibleComponents(const AZ::Component* component1)
    {
        AZStd::unordered_set<AZ::Component*> incompatibleComponents;

        auto entity = component1 ? component1->GetEntity() : nullptr;
        if (entity)
        {
            auto componentDescriptor1 = AzToolsFramework::GetComponentDescriptor(component1);
            if (componentDescriptor1)
            {
                AZ::ComponentDescriptor::DependencyArrayType providedServices1;
                AZ::ComponentDescriptor::DependencyArrayType providedServices2;
                AZ::ComponentDescriptor::DependencyArrayType incompatibleServices1;
                AZ::ComponentDescriptor::DependencyArrayType incompatibleServices2;

                //get the list of required and incompatible services from the primary component
                componentDescriptor1->GetProvidedServices(providedServices1, nullptr);
                componentDescriptor1->GetIncompatibleServices(incompatibleServices1, nullptr);

                //build a list of all components attached to the entity
                AZ::Entity::ComponentArrayType allComponents = entity->GetComponents();

                //also include invalid components waiting for requirements to be met
                EditorPendingCompositionRequestBus::Event(entity->GetId(), &EditorPendingCompositionRequests::GetPendingComponents, allComponents);

                //for every component related to the entity, determine if its services are incompatible with the primary component
                for (auto component2 : allComponents)
                {
                    //don't test against itself
                    if (component1 == component2)
                    {
                        continue;
                    }

                    auto componentDescriptor2 = AzToolsFramework::GetComponentDescriptor(component2);
                    if (!componentDescriptor2)
                    {
                        continue;
                    }

                    //get the list of required and incompatible services for the comparison component
                    providedServices2.clear();
                    componentDescriptor2->GetProvidedServices(providedServices2, nullptr);

                    incompatibleServices2.clear();
                    componentDescriptor2->GetIncompatibleServices(incompatibleServices2, nullptr);

                    //if provided services overlap incompatible services for either component then add it to the list
                    if (DoContainersIntersect(providedServices1, incompatibleServices2) ||
                        DoContainersIntersect(providedServices2, incompatibleServices1))
                    {
                        incompatibleComponents.insert(component2);
                    }
                }
            }
        }
        return incompatibleComponents;
    }


    //generate a list of all valid or pending sibling components that are service-incompatible with the specified set of component
    AZStd::unordered_set<AZ::Component*> GetRelatedIncompatibleComponents(const AZStd::vector<AZ::Component*>& components)
    {
        AZStd::unordered_set<AZ::Component*> allIncompatibleComponents;
        for (auto component : components)
        {
            const AZStd::unordered_set<AZ::Component*> incompatibleComponents = GetRelatedIncompatibleComponents(component);
            allIncompatibleComponents.insert(incompatibleComponents.begin(), incompatibleComponents.end());
        }
        return allIncompatibleComponents;
    }

    ComponentEditor::ComponentEditor(AZ::SerializeContext* context, IPropertyEditorNotify* notifyTarget /* = nullptr */, QWidget* parent /* = nullptr */)
        : QFrame(parent)
        , m_serializeContext(context)
    {
        setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Maximum);

        m_warningIcon = QIcon(":/PropertyEditor/Resources/warning.png");

        // create header bar
        m_header = aznew ComponentEditorHeader(this);
        m_header->setObjectName(ComponentEditorConstants::kHeaderId);
        m_header->SetTitle(ComponentEditorConstants::kUnknownComponentTitle);
        m_header->SetWarningIcon(m_warningIcon);
        m_header->SetWarning(false);
        m_header->SetReadOnly(false);
        m_header->SetExpandable(true);
        m_header->SetHasContextMenu(true);
        m_header->setStyleSheet("QToolTip { padding: 5px; }");

        // create property editor
        m_propertyEditor = aznew ReflectedPropertyEditor(this);
        m_propertyEditor->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Maximum);
        m_propertyEditor->setObjectName(ComponentEditorConstants::kPropertyEditorId);
        m_propertyEditor->Setup(context, notifyTarget, false, ComponentEditorConstants::kPropertyLabelWidth);
        m_propertyEditor->SetHideRootProperties(true);
        m_propertyEditor->setProperty("ComponentBlock", true); // used by stylesheet
        m_savedKeySeed = AZ_CRC("WorldEditorEntityEditor_Component", 0x926c865f);

        m_mainLayout = aznew QVBoxLayout(this);
        m_mainLayout->setSizeConstraint(QLayout::SetMinimumSize);
        m_mainLayout->setMargin(0);
        m_mainLayout->setContentsMargins(0, 0, 0, 0);
        m_mainLayout->addWidget(m_header);
        m_mainLayout->addWidget(m_propertyEditor);
        setLayout(m_mainLayout);

        connect(m_header, &ComponentEditorHeader::OnExpanderChanged, this, &ComponentEditor::OnExpanderChanged);
        connect(m_header, &ComponentEditorHeader::OnContextMenuClicked, this, &ComponentEditor::OnContextMenuClicked);
        connect(m_propertyEditor, &ReflectedPropertyEditor::OnExpansionContractionDone, this, &ComponentEditor::OnExpansionContractionDone);

        SetExpanded(true);
        SetSelected(false);
        SetDragged(false);
        SetDropTarget(false);
    }

    void ComponentEditor::AddInstance(AZ::Component* componentInstance, AZ::Component* aggregateInstance, AZ::Component* compareInstance)
    {
        if (!componentInstance)
        {
            return;
        }

        // Note that in the case of a GenericComponentWrapper we give the editor
        // the GenericComponentWrapper rather than the underlying type.
        AZ::Uuid instanceTypeId = azrtti_typeid(componentInstance);

        m_components.push_back(componentInstance);

        // Use our seed to make a unique save state key for each entity, so the property editor is stored per-entity for the component editor
        auto entityUniqueSavedStateKey = m_savedKeySeed;
        auto entityId = m_components[0]->GetEntityId();
        entityUniqueSavedStateKey.Add(reinterpret_cast<void*>(&entityId), sizeof(entityId));
        m_propertyEditor->SetSavedStateKey(entityUniqueSavedStateKey);

        m_propertyEditor->AddInstance(componentInstance, instanceTypeId, aggregateInstance, compareInstance);

        // When first instance is set, use its data to fill out the header.
        if (m_componentType.IsNull())
        {
            SetComponentType(*componentInstance);
        }

        m_header->setToolTip(BuildHeaderTooltip());
    }

    void ComponentEditor::ClearInstances(bool invalidateImmediately)
    {
        m_propertyEditor->SetDynamicEditDataProvider(nullptr);

        //clear warning flag and icon
        m_header->SetWarning(false);
        m_header->SetReadOnly(false);

        ClearNotifications();
        //clear component cache
        m_components.clear();

        m_propertyEditor->ClearInstances();
        if (invalidateImmediately)
        {
            m_propertyEditor->InvalidateAll();
        }

        InvalidateComponentType();

        m_header->setToolTip(BuildHeaderTooltip());
    }

    void ComponentEditor::AddNotifications()
    {
        ClearNotifications();

        const auto& forwardPendingComponentInfo = GetPendingComponentInfoForAllComponents();
        bool hasForwardPCI =
            !forwardPendingComponentInfo.m_validComponentsThatAreIncompatible.empty() ||
            !forwardPendingComponentInfo.m_missingRequiredServices.empty() ||
            !forwardPendingComponentInfo.m_pendingComponentsWithRequiredServices.empty();

        const auto& reversePendingComponentInfo = GetPendingComponentInfoForAllComponentsInReverse();
        bool hasReversePCI =
            !reversePendingComponentInfo.m_validComponentsThatAreIncompatible.empty() ||
            !reversePendingComponentInfo.m_missingRequiredServices.empty() ||
            !reversePendingComponentInfo.m_pendingComponentsWithRequiredServices.empty();

        //if there were any issues then treat them as warnings
        bool isWarning = hasForwardPCI;
        bool isReadOnly = isWarning || AreAnyComponentsDisabled();

        //display warning icon if component isn't valid
        m_header->SetWarning(isWarning);
        m_header->SetReadOnly(isReadOnly);

        //disable property editor if component isn't valid
        m_propertyEditor->setDisabled(isReadOnly);

        if (m_components.empty())
        {
            return;
        }

        if (m_components.size() > 1)
        {
            //if multiple components are represented then display a generic message
            if (hasForwardPCI || hasReversePCI)
            {
                CreateNotification("There is a component error on one or more entities in the selection. The selection cannot be modified until all errors have been resolved.");
            }
            return;
        }

        AZStd::map<QString, AZStd::vector<AZ::Component*> > uniqueMessages;

        //display notification for conflicting components, including duplicates
        if (!reversePendingComponentInfo.m_validComponentsThatAreIncompatible.empty())
        {
            CreateNotificationForConflictingComponents(tr("This component conflicts with one or more duplicate or incompatible components on this entity. Those components have been disabled."));
        }

        //display notification for conflicting components, including duplicates
        for (auto conflict : forwardPendingComponentInfo.m_validComponentsThatAreIncompatible)
        {
            QString message = tr("This component has been disabled because it is incompatible with \"%1\".").arg(AzToolsFramework::GetFriendlyComponentName(conflict).c_str());
            uniqueMessages[message].push_back(conflict);
        }
        for (const auto& message : uniqueMessages)
        {
            CreateNotificationForConflictingComponents(message.first);
        }
        uniqueMessages.clear();

        //display notification that one pending component might have requirements satisfied by another
        for (auto conflict : forwardPendingComponentInfo.m_pendingComponentsWithRequiredServices)
        {
            QString message = tr("This component is disabled pending \"%1\". This component will be enabled when the required component is resolved.").arg(AzToolsFramework::GetFriendlyComponentName(conflict).c_str());
            uniqueMessages[message].push_back(conflict);
        }
        for (const auto& message : uniqueMessages)
        {
            CreateNotification(message.first);
        }
        uniqueMessages.clear();

        //display notification with option to add components with required services
        if (!forwardPendingComponentInfo.m_missingRequiredServices.empty())
        {
            QString message = tr("This component is missing a required component service and has been disabled.");
            CreateNotificationForMissingComponents(message, forwardPendingComponentInfo.m_missingRequiredServices);
        }
    }

    void ComponentEditor::ClearNotifications()
    {
        for (auto notification : m_notifications)
        {
            delete notification;
        }

        m_notifications.clear();
    }

    ComponentEditorNotification* ComponentEditor::CreateNotification(const QString& message)
    {
        auto notification = aznew ComponentEditorNotification(this, message, m_warningIcon);
        notification->setVisible(IsExpanded());
        m_notifications.push_back(notification);
        layout()->addWidget(m_notifications.back());
        return notification;
    }

    ComponentEditorNotification* ComponentEditor::CreateNotificationForConflictingComponents(const QString& message)
    {
        //TODO ask about moving all of these to the context menu instead
        auto notification = CreateNotification(message);
        auto featureButton = aznew QPushButton(tr("Delete component"), notification);
        connect(featureButton, &QPushButton::clicked, this, [this]()
        {
            emit OnRequestRemoveComponents(GetComponents());
        });
        notification->AddFeature(featureButton);

#if 0 //disabling features until UX/UI is reworked or actions added to menu as stated above
        featureButton = aznew QPushButton(tr("Disable this component"), notification);
        connect(featureButton, &QPushButton::clicked, this, [this]()
        {
            emit OnRequestDisableComponents(GetComponents());
        });
        notification->AddFeature(featureButton);

        featureButton = aznew QPushButton("Remove incompatible components", notification);
        connect(featureButton, &QPushButton::clicked, this, [this]()
        {
            const AZStd::unordered_set<AZ::Component*> incompatibleComponents = GetRelatedIncompatibleComponents(GetComponents());
            emit OnRequestRemoveComponents(AZStd::vector<AZ::Component*>(incompatibleComponents.begin(), incompatibleComponents.end()));
        });
        notification->AddFeature(featureButton);
#endif

        //Activate/Enable are overloaded terms. "Disable incompatible components"
        featureButton = aznew QPushButton("Activate this component", notification);
        connect(featureButton, &QPushButton::clicked, this, [this]()
        {
            const AZStd::unordered_set<AZ::Component*> incompatibleComponents = GetRelatedIncompatibleComponents(GetComponents());
            emit OnRequestDisableComponents(AZStd::vector<AZ::Component*>(incompatibleComponents.begin(), incompatibleComponents.end()));
        });
        notification->AddFeature(featureButton);

        return notification;
    }

    ComponentEditorNotification* ComponentEditor::CreateNotificationForMissingComponents(const QString& message, const AZStd::vector<AZ::ComponentServiceType>& services)
    {
        auto notification = CreateNotification(message);
        auto featureButton = aznew QPushButton(tr("Add Required Component \342\226\276"), notification);
        connect(featureButton, &QPushButton::clicked, this, [this, featureButton, services]()
        {
            QRect screenRect(qApp->desktop()->availableGeometry(featureButton));
            QRect menuRect(
                featureButton->mapToGlobal(featureButton->rect().topLeft()),
                featureButton->mapToGlobal(featureButton->rect().bottomRight()));

            //place palette above or below button depending on how close to screen edge
            if (menuRect.bottom() + ComponentEditorConstants::kAddComponentMenuHeight > screenRect.bottom())
            {
                menuRect.setTop(menuRect.top() - ComponentEditorConstants::kAddComponentMenuHeight);
            }
            else
            {
                menuRect.setBottom(menuRect.bottom() + ComponentEditorConstants::kAddComponentMenuHeight);
            }

            emit OnRequestRequiredComponents(menuRect.topLeft(), menuRect.size(), services);
        });
        notification->AddFeature(featureButton);

        return notification;
    }

    bool ComponentEditor::AreAnyComponentsDisabled() const
    {
        for (auto component : m_components)
        {
            auto entity = component->GetEntity();
            if (entity)
            {
                AZ::Entity::ComponentArrayType disabledComponents;
                EditorDisabledCompositionRequestBus::Event(entity->GetId(), &EditorDisabledCompositionRequests::GetDisabledComponents, disabledComponents);
                if (AZStd::find(disabledComponents.begin(), disabledComponents.end(), component) != disabledComponents.end())
                {
                    return true;
                }
            }
        }

        return false;
    }

    AzToolsFramework::EntityCompositionRequests::PendingComponentInfo ComponentEditor::GetPendingComponentInfoForAllComponents() const
    {
        AzToolsFramework::EntityCompositionRequests::PendingComponentInfo combinedPendingComponentInfo;
        for (auto component : m_components)
        {
            AzToolsFramework::EntityCompositionRequests::PendingComponentInfo pendingComponentInfo;
            AzToolsFramework::EntityCompositionRequestBus::BroadcastResult(pendingComponentInfo, &AzToolsFramework::EntityCompositionRequests::GetPendingComponentInfo, component);

            for (auto conflict : pendingComponentInfo.m_validComponentsThatAreIncompatible)
            {
                AddUniqueItemToContainer(combinedPendingComponentInfo.m_validComponentsThatAreIncompatible, conflict);
            }

            for (auto conflict : pendingComponentInfo.m_pendingComponentsWithRequiredServices)
            {
                AddUniqueItemToContainer(combinedPendingComponentInfo.m_pendingComponentsWithRequiredServices, conflict);
            }

            for (auto service : pendingComponentInfo.m_missingRequiredServices)
            {
                AddUniqueItemToContainer(combinedPendingComponentInfo.m_missingRequiredServices, service);
            }
        }

        return combinedPendingComponentInfo;
    }

    AzToolsFramework::EntityCompositionRequests::PendingComponentInfo ComponentEditor::GetPendingComponentInfoForAllComponentsInReverse() const
    {
        AzToolsFramework::EntityCompositionRequests::PendingComponentInfo combinedPendingComponentInfo;

        //gather all entities from current selection to find any components with issues referring to components represented by this editor
        EntityIdList selectedEntityIds;
        ToolsApplicationRequests::Bus::BroadcastResult(selectedEntityIds, &ToolsApplicationRequests::GetSelectedEntities);

        AZ::Entity::ComponentArrayType componentsOnEntity;

        for (auto entityId : selectedEntityIds)
        {
            auto entity = GetEntityById(entityId);
            if (!entity)
            {
                continue;
            }

            //build a set of all active and pending components associated with the current entity
            componentsOnEntity.clear();
            AzToolsFramework::GetAllComponentsForEntity(entity, componentsOnEntity);

            for (auto component : componentsOnEntity)
            {
                //for each component, get any pending info/warnings
                AzToolsFramework::EntityCompositionRequests::PendingComponentInfo pendingComponentInfo;
                AzToolsFramework::EntityCompositionRequestBus::BroadcastResult(pendingComponentInfo, &AzToolsFramework::EntityCompositionRequests::GetPendingComponentInfo, component);

                //if pending info/warnings references displayed components then save the current component for reverse lookup
                if (DoContainersIntersect(pendingComponentInfo.m_validComponentsThatAreIncompatible, m_components))
                {
                    AddUniqueItemToContainer(combinedPendingComponentInfo.m_validComponentsThatAreIncompatible, component);
                }

                if (DoContainersIntersect(pendingComponentInfo.m_pendingComponentsWithRequiredServices, m_components))
                {
                    AddUniqueItemToContainer(combinedPendingComponentInfo.m_pendingComponentsWithRequiredServices, component);
                }
            }
        }

        return combinedPendingComponentInfo;
    }

    void ComponentEditor::InvalidateAll(const char* filter)
    {
        m_propertyEditor->InvalidateAll(filter);
    }

    void ComponentEditor::QueuePropertyEditorInvalidation(PropertyModificationRefreshLevel refreshLevel)
    {
        m_propertyEditor->QueueInvalidation(refreshLevel);
    }

    void ComponentEditor::SetComponentType(const AZ::Component& componentInstance)
    {
        const AZ::Uuid componentType = GetUnderlyingComponentType(componentInstance);

        auto classData = m_serializeContext->FindClassData(componentType);
        if (!classData || !classData->m_editData)
        {
            AZ_Warning("ComponentEditor", false, "Could not find reflection for component '%s'", classData->m_name);
            InvalidateComponentType();
            return;
        }

        m_componentType = componentType;

        m_propertyEditor->SetSavedStateKey(AZ::Crc32(componentType.ToString<AZStd::string>().data()));

        m_header->SetTitle(GetFriendlyComponentName(&componentInstance).c_str());

        m_header->ClearHelpURL();

        if (classData->m_editData)
        {
            if (auto editorData = classData->m_editData->FindElementData(AZ::Edit::ClassElements::EditorData))
            {
                if (auto nameAttribute = editorData->FindAttribute(AZ::Edit::Attributes::HelpPageURL))
                {
                    AZStd::string helpUrl;
                    AZ::AttributeReader nameReader(const_cast<AZ::Component*>(&componentInstance), nameAttribute);
                    nameReader.Read<AZStd::string>(helpUrl);
                    m_header->SetHelpURL(helpUrl);
                }
            }
        }

        AZStd::string iconPath;
        EBUS_EVENT_RESULT(iconPath, AzToolsFramework::EditorRequests::Bus, GetComponentEditorIcon, componentType, const_cast<AZ::Component*>(&componentInstance));
        m_header->SetIcon(QIcon(iconPath.c_str()));
    }

    void ComponentEditor::InvalidateComponentType()
    {
        m_componentType = AZ::Uuid::CreateNull();

        m_header->SetTitle(ComponentEditorConstants::kUnknownComponentTitle);

        AZStd::string iconPath;
        EBUS_EVENT_RESULT(iconPath, AzToolsFramework::EditorRequests::Bus, GetDefaultComponentEditorIcon);
        m_header->SetIcon(QIcon(iconPath.c_str()));
    }

    QString ComponentEditor::BuildHeaderTooltip()
    {
        // Don't display tooltip if multiple entities / component instances are being displayed.
        if (m_components.size() != 1)
        {
            return QString();
        }

        // Prepare a tooltip showing useful metadata about this particular component,
        // such as dependencies on other services (and components) on the entity.

        AZ::Component* thisComponent = m_components.front();

        if (!thisComponent)
        {
            return QString();
        }

        AZ::ComponentDescriptor* componentDescriptor = nullptr;
        EBUS_EVENT_ID_RESULT(componentDescriptor, thisComponent->RTTI_GetType(), AZ::ComponentDescriptorBus, GetDescriptor);

        if (!componentDescriptor)
        {
            return QString();
        }

        AZ::Entity* entity = thisComponent->GetEntity();

        if (!entity)
        {
            return QString();
        }

        const AZStd::vector<AZ::Component*>& components = entity->GetComponents();

        // Gather required services for the component.
        AZ::ComponentDescriptor::DependencyArrayType requiredServices;
        componentDescriptor->GetRequiredServices(requiredServices, thisComponent);

        QString tooltip;

        AZStd::list<AZStd::string> dependsOnComponents;

        // For each service, identify the component on the entity that is currently satisfying the requirement.
        for (const AZ::ComponentServiceType service : requiredServices)
        {
            for (AZ::Component* otherComponent : components)
            {
                if (otherComponent == thisComponent)
                {
                    continue;
                }

                AZ::ComponentDescriptor* otherDescriptor = nullptr;
                EBUS_EVENT_ID_RESULT(otherDescriptor, otherComponent->RTTI_GetType(), AZ::ComponentDescriptorBus, GetDescriptor);

                if (otherDescriptor)
                {
                    AZ::ComponentDescriptor::DependencyArrayType providedServices;
                    otherDescriptor->GetProvidedServices(providedServices, otherComponent);

                    if (AZStd::find(providedServices.begin(), providedServices.end(), service) != providedServices.end())
                    {
                        dependsOnComponents.emplace_back(GetFriendlyComponentName(otherComponent));
                        break;
                    }
                }
            }
        }

        if (!dependsOnComponents.empty())
        {
            tooltip += tr("Component has required services satisfied by the following components on this entity:");
            tooltip += "\n";

            for (const AZStd::string& componentName : dependsOnComponents)
            {
                tooltip += "\n\t";
                tooltip += componentName.c_str();
            }
        }

        return tooltip;
    }

    void ComponentEditor::OnExpanderChanged(bool expanded)
    {
        SetExpanded(expanded);
        emit OnExpansionContractionDone();
    }

    void ComponentEditor::OnContextMenuClicked(const QPoint& position)
    {
        emit OnDisplayComponentEditorMenu(position);
    }

    void ComponentEditor::contextMenuEvent(QContextMenuEvent *event)
    {
        OnContextMenuClicked(event->globalPos());
        event->accept();
    }

    void ComponentEditor::UpdateExpandability()
    {
        //updating whether or not expansion is allowed and forcing collapse if it's not
        bool isExpandable = IsExpandable();
        m_header->SetExpandable(isExpandable);
        SetExpanded(IsExpanded() && isExpandable);
    }

    void ComponentEditor::SetExpanded(bool expanded)
    {
        setUpdatesEnabled(false);

        m_header->SetExpanded(expanded);

        //toggle property editor visibility
        m_propertyEditor->setVisible(expanded);

        //toggle notification visibility
        for (auto notification : m_notifications)
        {
            notification->setVisible(expanded);
        }

        setUpdatesEnabled(true);
    }

    bool ComponentEditor::IsExpanded() const
    {
        return m_header->IsExpanded();
    }

    bool ComponentEditor::IsExpandable() const
    {
        //if there are any notifications, expansion must be allowed
        if (!m_notifications.empty())
        {
            return true;
        }

        //if any component class data has data elements or fields then it should be expandable
        int classElementCount = 0;
        int dataElementCount = 0;

        for (auto component : m_components)
        {
            auto typeId = GetComponentTypeId(component);

            //build a vector of all class info associated with typeId
            //EnumerateBase doesn't include class data for requested type so it must be added manually
            AZStd::vector<const AZ::SerializeContext::ClassData*> classDataVec = { GetComponentClassData(component) };

            //get all class info for any base classes (light components share a common base class where all properties are set)
            m_serializeContext->EnumerateBase(
                [&classDataVec](const AZ::SerializeContext::ClassData* classData, AZ::Uuid) { classDataVec.push_back(classData); return true; }, typeId);

            for (auto classData : classDataVec)
            {
                if (!classData || !classData->m_editData)
                {
                    continue;
                }

                //count all visible class and data elements
                for (const auto& element : classData->m_editData->m_elements)
                {
                    bool visible = true;
                    if (auto visibilityAttribute = element.FindAttribute(AZ::Edit::Attributes::Visibility))
                    {
                        AzToolsFramework::PropertyAttributeReader reader(component, visibilityAttribute);
                        AZ::u32 visibilityValue;
                        if (reader.Read<AZ::u32>(visibilityValue))
                        {
                            if (visibilityValue == AZ_CRC("PropertyVisibility_Hide", 0x32ab90f7))
                            {
                                visible = false;
                            }
                        }
                    }

                    if (visible)
                    {
                        if (element.IsClassElement())
                        {
                            classElementCount++;
                        }
                        else
                        {
                            dataElementCount++;
                        }
                    }

                    if (classElementCount > 1 || dataElementCount > 0)
                    {
                        return true;
                    }
                }
            }
        }

        return false;
    }

    void ComponentEditor::SetSelected(bool selected)
    {
        if (m_selected != selected)
        {
            m_selected = selected;
            setProperty("selected", m_selected);
            style()->unpolish(this);
            style()->polish(this);
        }
    }

    bool ComponentEditor::IsSelected() const
    {
        return m_selected;
    }

    void ComponentEditor::SetDragged(bool dragged)
    {
        m_dragged = dragged;
    }

    bool ComponentEditor::IsDragged() const
    {
        return m_dragged;
    }

    void ComponentEditor::SetDropTarget(bool dropTarget)
    {
        m_dropTarget = dropTarget;
    }

    bool ComponentEditor::IsDropTarget() const
    {
        return m_dropTarget;
    }

    AzToolsFramework::ComponentEditorHeader* ComponentEditor::GetHeader()
    {
        return m_header;
    }

    AzToolsFramework::ReflectedPropertyEditor* ComponentEditor::GetPropertyEditor()
    {
        return m_propertyEditor;
    }

    AZStd::vector<AZ::Component*>& ComponentEditor::GetComponents()
    {
        return m_components;
    }

    const AZStd::vector<AZ::Component*>& ComponentEditor::GetComponents() const
    {
        return m_components;
    }

    bool ComponentEditor::HasComponentWithId(AZ::ComponentId componentId)
    {
        for (AZ::Component* component : m_components)
        {
            if (component->GetId() == componentId)
            {
                return true;
            }
        }

        return false;
    }
}

#include <UI/PropertyEditor/ComponentEditor.moc>
