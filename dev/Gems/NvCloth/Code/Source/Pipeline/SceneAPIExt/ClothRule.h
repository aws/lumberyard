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

#pragma once

#include <AzCore/Memory/Memory.h>
#include <SceneAPI/SceneCore/DataTypes/Rules/IClothRule.h>

namespace AZ
{
    class ReflectContext;
}

namespace NvCloth
{
    namespace Pipeline
    {
        //! This class represents the data of a cloth rule (aka cloth modifier).
        class ClothRule
            : public AZ::SceneAPI::DataTypes::IClothRule
        {
        public:
            AZ_RTTI(ClothRule, "{2F5AC324-314A-4C53-AFFF-DDFA46605DDB}", AZ::SceneAPI::DataTypes::IClothRule);
            AZ_CLASS_ALLOCATOR_DECL

            static void Reflect(AZ::ReflectContext* context);

            // IClothRule overrides
            const AZStd::string& GetMeshNodeName() const override;
            const AZStd::string& GetVertexColorStreamName() const override;
            bool IsVertexColorStreamDisabled() const override;

            void SetMeshNodeName(const AZStd::string& name);
            void SetVertexColorStreamName(const AZStd::string& name);

        protected:
            AZStd::string m_meshNodeName;
            AZStd::string m_vertexColorStreamName;

            static const char* m_defaultChooseNodeName;
            static const char* m_defaultInverseMassString;

            friend class ClothRuleBehavior;
        };
    } // namespace Pipeline
} // namespace NvCloth
