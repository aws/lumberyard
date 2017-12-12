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

#include <AzCore/EBus/EBus.h>
#include <AzCore/Component/ComponentApplicationBus.h>
#include <AzToolsFramework/Debug/TraceContext.h>
#include <SceneAPI/SceneCore/DataTypes/Groups/IGroup.h>
#include <SceneAPI/SceneCore/Utilities/Reporting.h>
#include <SceneAPI/SceneUI/RowWidgets/ManifestVectorHandler.h>

namespace AZ
{
    namespace SceneAPI
    {
        namespace UI
        {
            AZ_CLASS_ALLOCATOR_IMPL(ManifestVectorHandler, SystemAllocator, 0)

            SerializeContext* ManifestVectorHandler::s_serializeContext = nullptr;
            ManifestVectorHandler* ManifestVectorHandler::s_instance = nullptr;

            QWidget* ManifestVectorHandler::CreateGUI(QWidget* parent)
            {
                if(ManifestVectorHandler::s_serializeContext)
                {
                    ManifestVectorWidget* instance = aznew ManifestVectorWidget(ManifestVectorHandler::s_serializeContext, parent);
                    connect(instance, &ManifestVectorWidget::valueChanged, this,
                        [instance]()
                        {
                            EBUS_EVENT(AzToolsFramework::PropertyEditorGUIMessages::Bus, RequestWrite, instance);
                        });
                    return instance;
                }
                else
                {
                    return nullptr;
                }
            }

            u32 ManifestVectorHandler::GetHandlerName() const
            {
                return AZ_CRC("ManifestVector", 0x895aa9aa);
            }

            bool ManifestVectorHandler::AutoDelete() const
            {
                return false;
            }

            void ManifestVectorHandler::ConsumeAttribute(ManifestVectorWidget* widget, u32 attrib,
                AzToolsFramework::PropertyAttributeReader* attrValue, const char* debugName)
            {
                AZ_TraceContext("Attribute name", debugName);

                if (attrib == AZ_CRC("ObjectTypeName", 0x6559e0c0))
                {
                    AZStd::string name;
                    if (attrValue->Read<AZStd::string>(name))
                    {
                        widget->SetCollectionTypeName(name);
                    }
                }
                else if (attrib == AZ_CRC("CollectionName", 0xbbc1c898))
                {
                    AZStd::string name;
                    if (attrValue->Read<AZStd::string>(name))
                    {
                        widget->SetCollectionName(name);
                    }
                }
                // Sets the number of entries the user can add through this widget. It doesn't limit
                //      the amount of entries that can be stored.
                else if (attrib == AZ_CRC("Cap", 0x993387b1))
                {
                    size_t cap;
                    if (attrValue->Read<size_t>(cap))
                    {
                        widget->SetCapSize(cap);
                    }
                }
            }

            void ManifestVectorHandler::WriteGUIValuesIntoProperty(size_t /*index*/, ManifestVectorWidget* GUI,
                property_t& instance, AzToolsFramework::InstanceDataNode* /*node*/)
            {
                instance = GUI->GetManifestVector();
            }

            bool ManifestVectorHandler::ReadValuesIntoGUI(size_t /*index*/, ManifestVectorWidget* GUI, const property_t& instance,
                AzToolsFramework::InstanceDataNode* node)
            {
                AzToolsFramework::InstanceDataNode* parentNode = node->GetParent();

                if (parentNode && parentNode->GetClassMetadata() && parentNode->GetClassMetadata()->m_azRtti)
                {
                    AZ_TraceContext("Parent UUID", parentNode->GetClassMetadata()->m_azRtti->GetTypeId());
                    if (parentNode->GetClassMetadata()->m_azRtti->IsTypeOf(DataTypes::IManifestObject::RTTI_Type()))
                    {
                        DataTypes::IManifestObject* owner = static_cast<DataTypes::IManifestObject*>(parentNode->FirstInstance());
                        GUI->SetManifestVector(instance, owner);
                    }
                    else if (parentNode->GetClassMetadata()->m_azRtti->IsTypeOf(Containers::RuleContainer::RTTI_Type()))
                    {
                        AzToolsFramework::InstanceDataNode* manifestObject = parentNode->GetParent();
                        if (manifestObject && manifestObject->GetClassMetadata()->m_azRtti->IsTypeOf(DataTypes::IManifestObject::RTTI_Type()))
                        {
                            DataTypes::IManifestObject* owner = static_cast<DataTypes::IManifestObject*>(manifestObject->FirstInstance());
                            GUI->SetManifestVector(instance, owner);
                        }
                        else
                        {
                            AZ_TracePrintf(Utilities::WarningWindow, "RuleContainer requires a ManifestObject parent.");
                        }
                    }
                    else
                    {
                        AZ_TracePrintf(Utilities::WarningWindow, "ManifestVectorWidget requires a ManifestObject parent.");
                    }
                }
                else
                {
                    AZ_TracePrintf(Utilities::WarningWindow, "ManifestVectorWidget requires valid parent with RTTI data specified");
                }

                return false;
            }

            void ManifestVectorHandler::Register()
            {
                if (!s_instance)
                {
                    s_instance = aznew ManifestVectorHandler();
                    EBUS_EVENT(AzToolsFramework::PropertyTypeRegistrationMessages::Bus, RegisterPropertyType, s_instance);
                    EBUS_EVENT_RESULT(s_serializeContext, AZ::ComponentApplicationBus, GetSerializeContext);
                    AZ_Assert(s_serializeContext, "Serialization context not available");
                }
            }

            void ManifestVectorHandler::Unregister()
            {
                if (s_instance)
                {
                    EBUS_EVENT(AzToolsFramework::PropertyTypeRegistrationMessages::Bus, UnregisterPropertyType, s_instance);
                    delete s_instance;
                    s_instance = nullptr;
                }
            }
        } // UI
    } // SceneAPI
} // AZ

#include <RowWidgets/ManifestVectorHandler.moc>