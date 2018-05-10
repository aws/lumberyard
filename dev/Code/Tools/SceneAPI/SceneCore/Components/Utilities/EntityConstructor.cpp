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

#include <AzCore/Component/ComponentApplication.h>
#include <AzCore/Component/ComponentApplicationBus.h>
#include <AzCore/Component/Entity.h>
#include <AzCore/IO/FileIO.h>
#include <AzCore/Module/ModuleManager.h>
#include <AzCore/RTTI/RTTI.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/ObjectStream.h>
#include <SceneAPI/SceneCore/Components/SceneSystemComponent.h>
#include <SceneAPI/SceneCore/Components/Utilities/EntityConstructor.h>
#include <SceneAPI/SceneCore/Utilities/Reporting.h>

namespace AZ
{
    namespace SceneAPI
    {
        namespace SceneCore
        {
            namespace EntityConstructor
            {
                EntityPointer BuildEntity(const char* entityName, const AZ::Uuid& baseComponentType)
                {
                    return EntityPointer(BuildEntityRaw(entityName, baseComponentType), [](AZ::Entity* entity)
                    {
                        entity->Deactivate();
                        delete entity;
                    });
                }

                Entity* BuildEntityRaw(const char* entityName, const AZ::Uuid& baseComponentType)
                {
                    SerializeContext* context = nullptr;
                    ComponentApplicationBus::BroadcastResult(context, &ComponentApplicationBus::Events::GetSerializeContext);

                    Entity* entity = aznew AZ::Entity(entityName);
                    if (context)
                    {
                        context->EnumerateDerived(
                            [entity](const AZ::SerializeContext::ClassData* data, const AZ::Uuid& /*typeId*/) -> bool
                            {
                                entity->CreateComponent(data->m_typeId);
                                return true;
                            }, baseComponentType, baseComponentType);
                    }
                    entity->Init();
                    entity->Activate();
                    
                    return entity;
                }

                Entity* BuildSceneSystemEntity(const char* configFilePath)
                {
                    SerializeContext* context = nullptr;
                    ComponentApplicationBus::BroadcastResult(context, &ComponentApplicationBus::Events::GetSerializeContext);
                    if (!context)
                    {
                        AZ_TracePrintf(SceneAPI::Utilities::ErrorWindow, "Unable to retrieve serialize context.");
                        return nullptr;
                    }

                    // Starting all system components would be too expensive for a builder/ResourceCompiler, so only the system components needed
                    // for the SceneAPI will be created. Some of these components can be configured through the Project Configurator's Advanced
                    // Settings, so this is a two step process. First load the entire configuration for the project, but throw everything but the
                    // specific needed components out. This still leaves components that are not configured so look over the SerializeContext for
                    // all components that haven't been added to the final entity yet.
                    AZStd::unique_ptr<Entity> entity(aznew AZ::Entity("Scene System"));

                    AZ::ObjectStream::InplaceLoadRootInfoCB inplaceLoadCb = [](void** rootAddress, const AZ::SerializeContext::ClassData**, const AZ::Uuid& classId, AZ::SerializeContext*)
                    {
                        if (!rootAddress)
                        {
                            return;
                        }

                        if (classId == azrtti_typeid<AZ::ComponentApplication::Descriptor>())
                        {
                            // ComponentApplication::Descriptor is normally a singleton.
                            // Force a unique instance to be created.
                            *rootAddress = aznew AZ::ComponentApplication::Descriptor();
                        }
                    };
                    AZ::ObjectStream::ClassReadyCB classReadyCb = [&entity](void* classPtr, const AZ::Uuid& classId, AZ::SerializeContext* context)
                    {
                        if (AZ::ModuleEntity* moduleEntity = context->Cast<AZ::ModuleEntity*>(classPtr, classId))
                        {
                            AZ::Entity::ComponentArrayType components = moduleEntity->GetComponents();
                            for (AZ::Component* component : components)
                            {
                                if (component->RTTI_IsTypeOf(azrtti_typeid<AZ::SceneAPI::SceneCore::SceneSystemComponent>()))
                                {
                                    moduleEntity->RemoveComponent(component);
                                    entity->AddComponent(component);
                                }
                            }
                            // All components that are needed have been safely moved to the new entity so delete the loaded one, which will also 
                            // take care of all the remaining components it has.
                            delete moduleEntity;
                        }
                        // The only required information is the deserialized SceneAPI system components, the rest can be deleted again.
                        else
                        {
                            const AZ::SerializeContext::ClassData* classData = context->FindClassData(classId);
                            if (classData && classData->m_factory)
                            {
                                classData->m_factory->Destroy(classPtr);
                            }
                            else
                            {
                                AZ_TracePrintf(SceneAPI::Utilities::ErrorWindow,
                                    "Unable to destroy class (%s) that was deserialized from the project configuration.", classId.ToString<AZStd::string>().c_str());
                            }
                        }
                    };

                    AZ::IO::FileIOStream fileStream;
                    if (!fileStream.Open(configFilePath, IO::OpenMode::ModeRead | IO::OpenMode::ModeText))
                    {
                        AZ_TracePrintf(SceneAPI::Utilities::ErrorWindow, "Unable to open gem config file.");
                        return nullptr;
                    }

                    // do not actually try to LOAD assets here, just load their Id's.  We're still booting up
                    AZ::ObjectStream::FilterDescriptor loadFilter(AZ::ObjectStream::AssetFilterNoAssetLoading, AZ::ObjectStream::FILTERFLAG_IGNORE_UNKNOWN_CLASSES);
                    if (!AZ::ObjectStream::LoadBlocking(&fileStream, *context, classReadyCb, loadFilter, inplaceLoadCb))
                    {
                        AZ_TracePrintf(SceneAPI::Utilities::ErrorWindow, "Failed to load gem config file.");
                        return nullptr;
                    }

                    const Uuid sceneSystemComponentType = azrtti_typeid<AZ::SceneAPI::SceneCore::SceneSystemComponent>();
                    context->EnumerateDerived(
                        [&entity](const AZ::SerializeContext::ClassData* data, const AZ::Uuid& typeId) -> bool
                    {
                        AZ_UNUSED(typeId);
                        // Before adding a new instance of a SceneSystemComponent, first check if the entity already has
                        // a component of the same type. Just like regular system components, there should only ever be
                        // a single instance of a SceneSystemComponent.
                        bool alreadyAdded = false;
                        for (const Component* component : entity->GetComponents())
                        {
                            if (component->RTTI_GetType() == data->m_typeId)
                            {
                                alreadyAdded = true;
                                break;
                            }
                        }
                        if (!alreadyAdded)
                        {
                            entity->CreateComponent(data->m_typeId);
                        }
                        return true;
                    }, sceneSystemComponentType, sceneSystemComponentType);
                    return entity.release();

                }
            } // namespace EntityConstructor
        } // namespace SceneCore
    } // namespace SceneAPI
} // namespace AZ