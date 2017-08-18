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

#include <AzCore/Component/ComponentApplicationBus.h>
#include <AzCore/Component/Entity.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <SceneAPI/SceneCore/Components/Utilities/EntityConstructor.h>

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
            } // namespace EntityConstructor
        } // namespace SceneCore
    } // namespace SceneAPI
} // namespace AZ