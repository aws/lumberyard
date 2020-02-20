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
#include "NvCloth_precompiled.h"

#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>

#include <SceneAPI/SceneCore/DataTypes/GraphData/IMeshData.h>
#include <SceneAPI/SceneCore/DataTypes/GraphData/IMeshVertexColorData.h>

#include <Pipeline/SceneAPIExt/ClothRule.h>

namespace NvCloth
{
    namespace Pipeline
    {
        // It's necessary for the rule to specify the system allocator, otherwise
        // the editor crashes when deleting the cloth modifier from FBX Settings.
        AZ_CLASS_ALLOCATOR_IMPL(ClothRule, AZ::SystemAllocator, 0)

        const char* ClothRule::m_defaultChooseNodeName = "Choose a node";
        const char* ClothRule::m_defaultInverseMassString = "Default: 1.0";

        const AZStd::string& ClothRule::GetMeshNodeName() const
        {
            return m_meshNodeName;
        }

        const AZStd::string& ClothRule::GetVertexColorStreamName() const
        {
            return m_vertexColorStreamName;
        }

        bool ClothRule::IsVertexColorStreamDisabled() const
        {
            return m_vertexColorStreamName == m_defaultInverseMassString;
        }

        void ClothRule::SetMeshNodeName(const AZStd::string& name)
        {
            m_meshNodeName = name;
        }

        void ClothRule::SetVertexColorStreamName(const AZStd::string& name)
        {
            m_vertexColorStreamName = name;
        }

        void ClothRule::Reflect(AZ::ReflectContext* context)
        {
            AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
            if (!serializeContext)
            {
                return;
            }

            serializeContext->Class<AZ::SceneAPI::DataTypes::IClothRule, AZ::SceneAPI::DataTypes::IRule>()->Version(1);

            serializeContext->Class<ClothRule, AZ::SceneAPI::DataTypes::IClothRule>()->Version(1)
                ->Field("meshNodeName", &ClothRule::m_meshNodeName)
                ->Field("vertexColorStreamName", &ClothRule::m_vertexColorStreamName);

            AZ::EditContext* editContext = serializeContext->GetEditContext();
            if (editContext)
            {
                editContext->Class<ClothRule>("Cloth", "Adds cloth data to the exported CGF asset. The cloth data will be used to determine what meshes to use for cloth simulation.")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                        ->Attribute(AZ::Edit::Attributes::NameLabelOverride, "")
                    ->DataElement("NodeListSelection", &ClothRule::m_meshNodeName, "Select Cloth Mesh", "Mesh used for cloth simulation.")
                        ->Attribute("ClassTypeIdFilter", AZ::SceneAPI::DataTypes::IMeshData::TYPEINFO_Uuid())
                        ->Attribute("DisabledOption", m_defaultChooseNodeName)
                    ->DataElement("NodeListSelection", &ClothRule::m_vertexColorStreamName, "Cloth Inverse Masses",
                        "Select the vertex color stream that contains cloth inverse masses or 'Default: 1.0' to use mass 1.0 for all vertices.")
                        ->Attribute("ClassTypeIdFilter", AZ::SceneAPI::DataTypes::IMeshVertexColorData::TYPEINFO_Uuid())
                        ->Attribute("DisabledOption", m_defaultInverseMassString)
                        ->Attribute("UseShortNames", true);
            }
        }
    } // namespace Pipeline
} // namespace NvCloth
