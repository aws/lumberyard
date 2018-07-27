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
#include "PropertyEntityIdCtrl.hxx"
#include "PropertyQTConstants.h"

#include <AzCore/Component/Entity.h>
#include <AzCore/Component/ComponentApplicationBus.h>
#include <AzCore/Casting/lossy_cast.h>

#include <AzToolsFramework/ToolsComponents/EditorEntityIdContainer.h>
#include <AzToolsFramework/UI/PropertyEditor/EntityIdQLabel.hxx>
#include <AzToolsFramework/Entity/EditorEntityContextPickingBus.h>
#include <AzToolsFramework/Entity/EditorEntityHelpers.h>
#include <AzToolsFramework/ToolsComponents/EditorComponentBase.h>

#include <QtWidgets/QCheckBox>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QFrame>
#include <QtCore/QMimeData>
#include <QDragEnterEvent>
#include <QDropEvent>

//just a test to see how it would work to pop a dialog

namespace AzToolsFramework
{
    PropertyEntityIdCtrl::PropertyEntityIdCtrl(QWidget* pParent)
        : QWidget(pParent)
        , m_acceptedEntityContextId(AzFramework::EntityContextId::CreateNull())
    {
        // create the gui, it consists of a layout, and in that layout, a text field for the value
        // and then a slider for the value.
        QHBoxLayout* pLayout = new QHBoxLayout(this);

        pLayout->setContentsMargins(0, 0, 0, 0);
        pLayout->setSpacing(0);

        m_entityIdLabel = aznew EntityIdQLabel(this);
        m_entityIdLabel->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Fixed);
        m_entityIdLabel->setMinimumWidth(PropertyQTConstant_MinimumWidth);
        m_entityIdLabel->setFixedHeight(PropertyQTConstant_DefaultHeight);
        m_entityIdLabel->setFrameShape(QFrame::Panel);
        m_entityIdLabel->setFrameShadow(QFrame::Sunken);
        m_entityIdLabel->setFocusPolicy(Qt::StrongFocus);

        m_pickButton = aznew QPushButton(this);
        m_pickButton->setCheckable(true);
        m_pickButton->setChecked(false);
        m_pickButton->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
        m_pickButton->setFixedSize(QSize(18, 18));
        m_pickButton->setContentsMargins(0, 0, 0, 0);
        m_pickButton->setIcon(QIcon(":/PropertyEditor/Resources/EntityPicker"));
        m_pickButton->setToolTip("Pick an object in the viewport");
        m_pickButton->setMouseTracking(true);

        m_clearButton = aznew QPushButton(this);
        m_clearButton->setFlat(true);
        m_clearButton->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
        m_clearButton->setFixedSize(QSize(18, 18));
        m_clearButton->setStyleSheet("border: none; padding: 0ex;");
        m_clearButton->setIcon(QIcon(":/PropertyEditor/Resources/cross-small"));
        m_clearButton->setToolTip("Clear entity reference");
        m_clearButton->setMouseTracking(true);

        pLayout->addWidget(m_entityIdLabel);
        pLayout->addWidget(m_pickButton);
        pLayout->addWidget(m_clearButton);

        setStyleSheet("QToolTip { padding: 5px; }");

        setFocusProxy(m_entityIdLabel);
        setFocusPolicy(m_entityIdLabel->focusPolicy());

        setLayout(pLayout);

        setAcceptDrops(true);

        connect(m_entityIdLabel, &EntityIdQLabel::RequestPickObject, this, [this]()
        {
            if (!m_pickButton->isChecked())
            {
                InitObjectPickMode();
            }
            else
            {
                CancelObjectPickMode();
            }
        });

        connect(m_pickButton, &QPushButton::clicked, this, [this]()
        {
            if (m_pickButton->isChecked())
            {
                InitObjectPickMode();
            }
            else
            {
                CancelObjectPickMode();
            }
        });

        connect(m_clearButton, &QPushButton::clicked, this, [this]()
        {
            SetCurrentEntityId(AZ::EntityId(), true, "");
        });
    }

    void PropertyEntityIdCtrl::InitObjectPickMode()
    {
        EBUS_EVENT(AzToolsFramework::EditorPickModeRequests::Bus, StopObjectPickMode);
        if (!EditorPickModeRequests::Bus::Handler::BusIsConnected())
        {
            emit OnPickStart();
            m_pickButton->setChecked(true);
            EditorPickModeRequests::Bus::Handler::BusConnect();
            AzToolsFramework::EditorEvents::Bus::Handler::BusConnect();
        }
        EBUS_EVENT(AzToolsFramework::EditorPickModeRequests::Bus, StartObjectPickMode);
    }

    void PropertyEntityIdCtrl::CancelObjectPickMode()
    {
        if (EditorPickModeRequests::Bus::Handler::BusIsConnected())
        {
            EBUS_EVENT(AzToolsFramework::EditorPickModeRequests::Bus, StopObjectPickMode);
        }
    }

    void PropertyEntityIdCtrl::StopObjectPickMode()
    {
        if (EditorPickModeRequests::Bus::Handler::BusIsConnected())
        {
            m_pickButton->setChecked(false);
            EditorPickModeRequests::Bus::Handler::BusDisconnect();
            AzToolsFramework::EditorEvents::Bus::Handler::BusDisconnect();
            emit OnPickComplete();
        }
    }

    void PropertyEntityIdCtrl::OnPickModeSelect(AZ::EntityId id)
    {
        if (id.IsValid())
        {
            SetCurrentEntityId(id, true, "");
        }
    }

    void PropertyEntityIdCtrl::OnEscape()
    {
        CancelObjectPickMode();
    }

    void PropertyEntityIdCtrl::dragLeaveEvent(QDragLeaveEvent* event)
    {
        (void)event;
        m_entityIdLabel->setProperty("DropHighlight", QVariant());
        m_entityIdLabel->style()->unpolish(m_entityIdLabel);
        m_entityIdLabel->style()->polish(m_entityIdLabel);
    }

    void PropertyEntityIdCtrl::dragEnterEvent(QDragEnterEvent* event)
    {
        if (event != nullptr &&
            IsCorrectMimeData(event->mimeData()) &&
            EntityIdFromMimeData(*event->mimeData()).IsValid())
        {
            m_entityIdLabel->setProperty("DropHighlight", true);
            m_entityIdLabel->style()->unpolish(m_entityIdLabel);
            m_entityIdLabel->style()->polish(m_entityIdLabel);

            event->acceptProposedAction();
        }
    }

    void PropertyEntityIdCtrl::dropEvent(QDropEvent* event)
    {
        m_entityIdLabel->setProperty("DropHighlight", QVariant());
        m_entityIdLabel->style()->unpolish(m_entityIdLabel);
        m_entityIdLabel->style()->polish(m_entityIdLabel);

        if (event == nullptr)
        {
            return;
        }
        if (!IsCorrectMimeData(event->mimeData()))
        {
            return;
        }

        AZ::EntityId droppedEntityId = EntityIdFromMimeData(*event->mimeData());
        if (!droppedEntityId.IsValid())
        {
            return;
        }

        SetCurrentEntityId(droppedEntityId, true, "");

        event->acceptProposedAction();
    }

    bool PropertyEntityIdCtrl::IsCorrectMimeData(const QMimeData* data_) const
    {
        if (data_ == nullptr ||
            !data_->hasFormat(AzToolsFramework::EditorEntityIdContainer::GetMimeType()))
        {
            return false;
        }

        return true;
    }

    AZ::EntityId PropertyEntityIdCtrl::EntityIdFromMimeData(const QMimeData& data_) const
    {
        AZ::EntityId entityIdResult;

        if (!IsCorrectMimeData(&data_))
        {
            return entityIdResult;
        }

        QByteArray arrayData = data_.data(AzToolsFramework::EditorEntityIdContainer::GetMimeType());

        AzToolsFramework::EditorEntityIdContainer entityIdListContainer;
        if (!entityIdListContainer.FromBuffer(arrayData.constData(), arrayData.size()))
        {
            return entityIdResult;
        }
        // The behavior of dragging an entity onto this control is much less confusing,
        // and much more predictable, if only one entity can be dropped.
        if (entityIdListContainer.m_entityIds.empty() ||
            entityIdListContainer.m_entityIds.size() > 1)
        {
            return entityIdResult;
        }

        if (!m_acceptedEntityContextId.IsNull())
        {
            // Check that the entity's owning context matches the one that this control accepts
            AzFramework::EntityContextId contextId = AzFramework::EntityContextId::CreateNull();
            EBUS_EVENT_ID_RESULT(contextId, entityIdListContainer.m_entityIds[0], AzFramework::EntityIdContextQueryBus, GetOwningContextId);

            if (contextId != m_acceptedEntityContextId)
            {
                return entityIdResult;
            }
        }

        entityIdResult = entityIdListContainer.m_entityIds[0];

        return entityIdResult;
    }

    AZ::EntityId PropertyEntityIdCtrl::GetEntityId() const
    {
        return m_entityIdLabel->GetEntityId();
    }

    PropertyEntityIdCtrl::~PropertyEntityIdCtrl()
    {
        CancelObjectPickMode();
    }

    QWidget* PropertyEntityIdCtrl::GetFirstInTabOrder()
    {
        return m_entityIdLabel;
    }
    QWidget* PropertyEntityIdCtrl::GetLastInTabOrder()
    {
        return m_entityIdLabel;
    }

    void PropertyEntityIdCtrl::UpdateTabOrder()
    {
        // There's only one QT widget on this property.
    }

    bool PropertyEntityIdCtrl::SetChildWidgetsProperty(const char* name, const QVariant& variant)
    {
        bool success = m_entityIdLabel->setProperty(name, variant);
        success = m_pickButton->setProperty(name, variant) && success;
        success = m_clearButton->setProperty(name, variant) && success;

        return success;
    }

    void PropertyEntityIdCtrl::SetCurrentEntityId(const AZ::EntityId& newEntityId, bool emitChange, const AZStd::string& nameOverride)
    {
        m_entityIdLabel->SetEntityId(newEntityId, nameOverride);
        m_componentsSatisfyingServices.clear();

        if (!m_requiredServices.empty() || !m_incompatibleServices.empty())
        {
            bool servicesMismatch = false;

            AZ::Entity* entity = nullptr;
            EBUS_EVENT_RESULT(entity, AZ::ComponentApplicationBus, FindEntity, newEntityId);

            if (entity)
            {
                AZStd::vector<AZ::ComponentServiceType> unmatchedServices(m_requiredServices);
                bool foundIncompatibleService = false;
                const auto& components = entity->GetComponents();
                for (const AZ::Component* component : components)
                {
                    AZ::ComponentDescriptor* componentDescriptor = nullptr;
                    EBUS_EVENT_ID_RESULT(componentDescriptor, component->RTTI_GetType(), AZ::ComponentDescriptorBus, GetDescriptor);
                    AZ::ComponentDescriptor::DependencyArrayType providedServices;
                    componentDescriptor->GetProvidedServices(providedServices, component);
                    for (int serviceIdx = azlossy_cast<int>(unmatchedServices.size() - 1); serviceIdx >= 0; --serviceIdx)
                    {
                        AZ::ComponentServiceType service = unmatchedServices[serviceIdx];
                        if (AZStd::find(providedServices.begin(), providedServices.end(), service) != providedServices.end())
                        {
                            m_componentsSatisfyingServices.emplace_back(AzToolsFramework::GetFriendlyComponentName(component));
                            unmatchedServices.erase(unmatchedServices.begin() + serviceIdx);
                        }
                    }
                    
                    for (const auto& service : providedServices)
                    {
                        if (AZStd::find(m_incompatibleServices.begin(), m_incompatibleServices.end(), service) != m_incompatibleServices.end())
                        {
                            foundIncompatibleService = true;
                            break;
                        }
                    }

                    if (foundIncompatibleService)
                    {
                        break;
                    }
                }

                servicesMismatch = !unmatchedServices.empty() || foundIncompatibleService;
            }

            SetMismatchedServices(servicesMismatch);
        }

        QString tip = BuildTooltip();
        if (!tip.isEmpty())
        {
            m_entityIdLabel->setToolTip(tip);
        }

        if (emitChange)
        {
            emit OnEntityIdChanged(newEntityId);
        }
    }

    QString PropertyEntityIdCtrl::BuildTooltip()
    {
        AZ::Entity* entity = nullptr;
        EBUS_EVENT_RESULT(entity, AZ::ComponentApplicationBus, FindEntity, m_entityIdLabel->GetEntityId());

        if (!entity)
        {
            return QString();
        }

        QString tooltip;

        QVariant mismatchedServices = property("MismatchedServices");
        if (mismatchedServices == true)
        {
            tooltip = "Service requirements are not met by the referenced entity.";
        }
        else if (!m_componentsSatisfyingServices.empty())
        {
            tooltip = tr("Required services are satisfied by the following components on ");
            tooltip += entity->GetName().c_str();
            tooltip += ":\n";

            for (const AZStd::string& componentName : m_componentsSatisfyingServices)
            {
                tooltip += "\n\t";
                tooltip += componentName.c_str();
            }
        }

        return tooltip;
    }

    void PropertyEntityIdCtrl::SetRequiredServices(const AZStd::vector<AZ::ComponentServiceType>& requiredServices)
    {
        m_requiredServices = requiredServices;
    }

    void PropertyEntityIdCtrl::SetIncompatibleServices(const AZStd::vector<AZ::ComponentServiceType>& incompatibleServices)
    {
        m_incompatibleServices = incompatibleServices;
    }

    void PropertyEntityIdCtrl::SetMismatchedServices(bool mismatchedServices)
    {
        if (property("MismatchedServices") != mismatchedServices)
        {
            setProperty("MismatchedServices", mismatchedServices);
            style()->unpolish(this);
            style()->polish(this);
            update();

            m_entityIdLabel->style()->unpolish(m_entityIdLabel);
            m_entityIdLabel->style()->polish(m_entityIdLabel);
            m_entityIdLabel->update();
        }
    }

    void PropertyEntityIdCtrl::SetAcceptedEntityContext(AzFramework::EntityContextId contextId)
    {
        m_acceptedEntityContextId = contextId;

        // Update pick button visibility depending on whether the entity context supports viewport picking
        bool supportsViewportEntityIdPicking = true;
        if (!m_acceptedEntityContextId.IsNull())
        {
            EBUS_EVENT_ID_RESULT(supportsViewportEntityIdPicking, m_acceptedEntityContextId, AzToolsFramework::EditorEntityContextPickingRequestBus, SupportsViewportEntityIdPicking);
        }
        m_pickButton->setVisible(supportsViewportEntityIdPicking);
    }

    QWidget* EntityIdPropertyHandler::CreateGUI(QWidget* pParent)
    {
        PropertyEntityIdCtrl* newCtrl = aznew PropertyEntityIdCtrl(pParent);
        connect(newCtrl, &PropertyEntityIdCtrl::OnEntityIdChanged, this, [newCtrl](AZ::EntityId)
            {
                EBUS_EVENT(PropertyEditorGUIMessages::Bus, RequestWrite, newCtrl);
            });
        return newCtrl;
    }

    void EntityIdPropertyHandler::ConsumeAttribute(PropertyEntityIdCtrl* GUI, AZ::u32 attrib, PropertyAttributeReader* attrValue, const char* debugName)
    {
        (void)GUI;
        (void)attrib;
        (void)attrValue;
        (void)debugName;

        if (attrib == AZ::Edit::Attributes::RequiredService)
        {
            AZ::ComponentServiceType requiredService = 0;
            AZStd::vector<AZ::ComponentServiceType> requiredServices;
            if (attrValue->template Read<AZ::ComponentServiceType>(requiredService))
            {
                GUI->SetRequiredServices(AZStd::vector<AZ::ComponentServiceType>({ requiredService }));
            }
            else if (attrValue->template Read<decltype(requiredServices)>(requiredServices))
            {
                GUI->SetRequiredServices(requiredServices);
            }
        }
        else if (attrib == AZ::Edit::Attributes::IncompatibleService)
        {
            AZ::ComponentServiceType incompatibleService = 0;
            AZStd::vector<AZ::ComponentServiceType> incompatibleServices;
            if (attrValue->template Read<AZ::ComponentServiceType>(incompatibleService))
            {
                GUI->SetIncompatibleServices(AZStd::vector<AZ::ComponentServiceType>({ incompatibleService }));
            }
            else if (attrValue->template Read<decltype(incompatibleServices)>(incompatibleServices))
            {
                GUI->SetIncompatibleServices(incompatibleServices);
            }
        }
    }

    void EntityIdPropertyHandler::WriteGUIValuesIntoProperty(size_t index, PropertyEntityIdCtrl* GUI, property_t& instance, InstanceDataNode* node)
    {
        (int)index;
        (void)node;
        instance = GUI->GetEntityId();
    }

    bool EntityIdPropertyHandler::ReadValuesIntoGUI(size_t index, PropertyEntityIdCtrl* GUI, const property_t& instance, InstanceDataNode* node)
    {
        // Find the entity that is associated with the property
        AZ::EntityId entityId;
        while (node)
        {
            if ((node->GetClassMetadata()) && (node->GetClassMetadata()->m_azRtti))
            {
                AZ::IRttiHelper* rtti = node->GetClassMetadata()->m_azRtti;
                if (rtti->IsTypeOf<AZ::Component>())
                {
                    AZ::Entity* entity = rtti->Cast<AZ::Component>(node->GetInstance(index))->GetEntity();
                    if (entity)
                    {
                        entityId = entity->GetId();
                        if (entityId.IsValid())
                        {
                            break;
                        }
                    }
                }
            }

            node = node->GetParent();
        }

        // Tell the GUI which entity context it should accept
        AzFramework::EntityContextId contextId = AzFramework::EntityContextId::CreateNull();
        if (entityId.IsValid())
        {            
            EBUS_EVENT_ID_RESULT(contextId, entityId, AzFramework::EntityIdContextQueryBus, GetOwningContextId);
        }
        GUI->SetAcceptedEntityContext(contextId);

        GUI->SetCurrentEntityId(instance, false, "");
        return false;
    }

    void RegisterEntityIdPropertyHandler()
    {
        EBUS_EVENT(PropertyTypeRegistrationMessages::Bus, RegisterPropertyType, aznew EntityIdPropertyHandler());
    }
}

#include <UI/PropertyEditor/PropertyEntityIdCtrl.moc>
