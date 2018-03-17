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

#include <AzCore/RTTI/ReflectContext.h>
#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <SceneAPI/SceneCore/DataTypes/GraphData/IMeshData.h>
#include <SceneAPI/SceneData/Rules/PhysicsRule.h>

namespace AZ
{
    namespace SceneAPI
    {
        namespace SceneData
        {
            AZ_CLASS_ALLOCATOR_IMPL(PhysicsRule, SystemAllocator, 0)

            SceneNodeSelectionList& PhysicsRule::GetNodeSelectionList()
            {
                return m_nodeSelectionList;
            }


            DataTypes::ISceneNodeSelectionList& PhysicsRule::GetSceneNodeSelectionList()
            {
                return m_nodeSelectionList;
            }

            const DataTypes::ISceneNodeSelectionList& PhysicsRule::GetSceneNodeSelectionList() const
            {
                return m_nodeSelectionList;
            }

            void PhysicsRule::Reflect(ReflectContext* context)
            {
                SerializeContext* serializeContext = azrtti_cast<SerializeContext*>(context);
                if (!serializeContext)
                {
                    return;
                }

                serializeContext->Class<PhysicsRule, DataTypes::IPhysicsRule>()->Version(1)
                    ->Field("nodeSelectionList", &PhysicsRule::m_nodeSelectionList);

                EditContext* editContext = serializeContext->GetEditContext();
                if (editContext)
                {
                    editContext->Class<PhysicsRule>("CryPhysics Proxy", "Configure CryPhysics collision data.\nNote: A physics rule will generate an MTL file even if a Material Rule is not applied.")
                        ->ClassElement(Edit::ClassElements::EditorData, "")
                            ->Attribute("AutoExpand", true)
                            ->Attribute(AZ::Edit::Attributes::NameLabelOverride, "")
                        ->DataElement(Edit::UIHandlers::Default, &PhysicsRule::m_nodeSelectionList, "Physics meshes", "Select 1 or more collision meshes.")
                            ->Attribute("FilterName", "physics meshes")
                            ->Attribute("FilterType", DataTypes::IMeshData::TYPEINFO_Uuid());
                }
            }
        } // SceneData
    } // SceneAPI
} // AZ
