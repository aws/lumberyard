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

#include <qbytearray.h>

#include <AzCore/Math/Vector2.h>
#include <AzCore/std/containers/unordered_set.h>
#include <AzCore/std/any.h>
#include <AzCore/Serialization/Utils.h>
#include <AzCore/Serialization/IdUtils.h>

#include <GraphCanvas/Types/SceneData.h>

namespace GraphCanvas
{
    //! Class for storing Entities that will be serialized to the Clipboard
    //! This class will delete the stored entities in the destructor therefore
    //! any entities that should not be owned by this class should be removed
    //! before destruction
    class SceneSerialization
    {
    public:
        AZ_TYPE_INFO(SceneSerialization, "{DB95F1F9-BEEA-499F-A6AD-1492435768F8}");
        AZ_CLASS_ALLOCATOR(SceneSerialization, AZ::SystemAllocator, 0);

        static void Reflect(AZ::ReflectContext* context)
        {
            AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
            if (!serializeContext)
            {
                return;
            }

            serializeContext->Class<SceneSerialization>()
                ->Version(1)
                ->Field("UserData", &SceneSerialization::m_userFields)
                ->Field("SceneData", &SceneSerialization::m_sceneData)
                ->Field("Key", &SceneSerialization::m_serializationKey)
                ->Field("AveragePosition", &SceneSerialization::m_averagePosition)
            ;
        }

        SceneSerialization() = default;

        explicit SceneSerialization(AZStd::string serializationKey)
            : m_serializationKey(serializationKey)
        {
        }

        explicit SceneSerialization(const QByteArray& dataArray)
        {
            AZ::SerializeContext* serializeContext = AZ::EntityUtils::GetApplicationSerializeContext();
            AZ::Utils::LoadObjectFromBufferInPlace(dataArray.constData(), (AZStd::size_t)dataArray.size(), *this, serializeContext);
            RegenerateIds();
        }

        explicit SceneSerialization(SceneSerialization&& other)
            : m_serializationKey(other.m_serializationKey)
            , m_sceneData(other.m_sceneData)
            , m_averagePosition(other.m_averagePosition)
            , m_userFields(AZStd::move(other.m_userFields))
            , m_newIdMapping(AZStd::move(other.m_newIdMapping))
        {
        }

        ~SceneSerialization() = default;

        SceneSerialization& operator=(SceneSerialization&& other)
        {
            m_serializationKey = other.m_serializationKey;
            m_sceneData = other.m_sceneData;
            m_averagePosition = other.m_averagePosition;
            m_userFields = AZStd::move(other.m_userFields);
            m_newIdMapping = AZStd::move(other.m_newIdMapping);
            return *this;
        }

        void Clear()
        {
            m_sceneData.Clear();
            m_userFields.clear();
        }

        const AZStd::string& GetSerializationKey() const
        {
            return m_serializationKey;
        }

        void SetAveragePosition(const AZ::Vector2& averagePosition)
        {
            m_averagePosition = averagePosition;
        }

        const AZ::Vector2& GetAveragePosition() const
        {
            return m_averagePosition;
        }

        SceneData& GetSceneData()
        {
            return m_sceneData;
        }

        const SceneData& GetSceneData() const
        {
            return m_sceneData;
        }

        AZStd::unordered_map<AZStd::string, AZStd::any>& GetUserDataMapRef()
        {
            return m_userFields;
        }

        const AZStd::unordered_map<AZStd::string, AZStd::any>& GetUserDataMapRef() const
        {
            return m_userFields;
        }

        AZ::EntityId FindRemappedEntityId(const AZ::EntityId& originalId) const
        {
            AZ::EntityId mappedId;

            auto mappingIter = m_newIdMapping.find(originalId);

            if (mappingIter != m_newIdMapping.end())
            {
                mappedId = mappingIter->second;
            }

            return mappedId;
        }

        void RegenerateIds()
        {
            AZ::IdUtils::Remapper<AZ::EntityId>::GenerateNewIdsAndFixRefs(this, m_newIdMapping);
        }

    private:

        // The key to help decide which targets are valid for this serialized data at the graph canvas level.
        AZStd::string                                   m_serializationKey;

        // The Scene data to be copied.
        SceneData                                       m_sceneData;
        AZ::Vector2                                     m_averagePosition;

        // Custom serializable fields for adding custom user data to the serialization
        AZStd::unordered_map<AZStd::string, AZStd::any> m_userFields;
        AZStd::unordered_map<AZ::EntityId, AZ::EntityId> m_newIdMapping;
    };
}
