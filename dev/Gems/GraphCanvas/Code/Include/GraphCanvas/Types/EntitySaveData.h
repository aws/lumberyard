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

#include <AzCore/Math/Uuid.h>
#include <AzCore/std/containers/unordered_map.h>

namespace GraphCanvas
{
    //! Dummy interface to simplify down the logic.
    //! And standardize the interaction patterns with the meta data objects
    class ComponentSaveData
    {
    public:
        AZ_RTTI(ComponentSaveData, "{359ACEC7-D0FA-4FC0-8B59-3755BB1A9836}");
        AZ_CLASS_ALLOCATOR(ComponentSaveData, AZ::SystemAllocator, 0);

        ComponentSaveData() = default;
        virtual ~ComponentSaveData() = default;
    };

    //! This data structure provides a hook for serializing and unserializing whatever data is necessary
    //! For a particular GraphCanvas Entity.
    //!
    //! Used for only writing out pertinent information in saving systems where graphs can be entirely reconstructed
    //! from the saved values.
    class EntitySaveDataContainer
    {
    private:
        friend class GraphCanvasSystemComponent;

    public:
        AZ_TYPE_INFO(EntitySaveDataContainer, "{DCCDA882-AF72-49C3-9AAD-BA601322BFBC}");
        AZ_CLASS_ALLOCATOR(EntitySaveDataContainer, AZ::SystemAllocator, 0);
        
        ~EntitySaveDataContainer()
        {
            Clear();
        }

        void Clear()
        {
            for (auto& mapPair : m_entityData)
            {
                delete mapPair.second;
            }
            
            m_entityData.clear();
        }

        template<class KeyType, class DataType>
        DataType* CreateSaveData()
        {
            DataType* retVal = nullptr;
            AZ::Uuid typeId = azrtti_typeid<KeyType>();

            auto mapIter = m_entityData.find(typeId);

            if (mapIter == m_entityData.end())
            {
                retVal = aznew DataType();
                m_entityData[typeId] = retVal;
            }
            else
            {
                AZ_Error("Graph Canvas", false, "Trying to create two save data sources for KeyType (%s)", typeId.ToString<AZStd::string>().c_str());
            }
            
            return retVal;
        }
        
        template<class KeyType>
        ComponentSaveData* FindSaveData() const
        {
            auto mapIter = m_entityData.find(azrtti_typeid<KeyType>());
            
            if (mapIter != m_entityData.end())
            {
                return mapIter->second;
            }
            
            return nullptr;
        }
        
        template<class KeyType, class DataType>
        DataType* FindSaveDataAs() const
        {
            return azrtti_cast<DataType*>(FindSaveData<KeyType>());
        }

        template<class KeyType, class DataType>
        DataType* FindCreateSaveData()
        {
            DataType* dataType = FindSaveDataAs<KeyType, DataType>();

            if (dataType == nullptr)
            {
                dataType = CreateSaveData<KeyType, DataType>();
            }

            return dataType;
        }
        
    private:
        AZStd::unordered_map< AZ::Uuid, ComponentSaveData* > m_entityData;
    };
} // namespace GraphCanvas