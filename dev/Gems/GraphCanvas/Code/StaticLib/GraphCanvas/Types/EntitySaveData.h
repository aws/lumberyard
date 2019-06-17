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

#include <GraphCanvas/Components/EntitySaveDataBus.h>
#include <GraphCanvas/Editor/GraphModelBus.h>
#include <GraphCanvas/Components/SceneBus.h>

namespace GraphCanvas
{
    class ComponentSaveData
    {
    public:
        AZ_RTTI(ComponentSaveData, "{359ACEC7-D0FA-4FC0-8B59-3755BB1A9836}");
        AZ_CLASS_ALLOCATOR(ComponentSaveData, AZ::SystemAllocator, 0);

        ComponentSaveData() = default;
        virtual ~ComponentSaveData() = default;
        
        void operator=(const ComponentSaveData& other)
        {
            // Don't want to copy over anything, as the owner/graphid will vary from element to element and we want to maintain
            // whichever one we were created with
        }        

        void RegisterIds(const AZ::EntityId& entityId, const AZ::EntityId& graphId)
        {
            m_ownerId = entityId;
            m_graphId = graphId;
        }

        void SignalDirty()
        {
            if (m_ownerId.IsValid() && m_graphId.IsValid())
            {
                GraphModelRequestBus::Event(m_graphId, &GraphModelRequests::OnSaveDataDirtied, m_ownerId);
            }
        }

    protected:

        const AZ::EntityId& GetOwnerId() const
        {
            return m_ownerId;
        }

        const AZ::EntityId& GetGraphId() const
        {
            return m_graphId;
        }

    private:

        AZ::EntityId m_ownerId;
        AZ::EntityId m_graphId;
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

        template<class DataType>
        DataType* CreateSaveData()
        {
            DataType* retVal = nullptr;
            AZ::Uuid typeId = azrtti_typeid<DataType>();

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

        template<class DataType>
        void RemoveSaveData()
        {
            AZ::Uuid typeId = azrtti_typeid<DataType>();

            auto mapIter = m_entityData.find(typeId);

            if (mapIter != m_entityData.end())
            {
                delete mapIter->second;
                m_entityData.erase(mapIter);
            }
        }
        
        template<class DataType>
        DataType* FindSaveData() const
        {
            auto mapIter = m_entityData.find(azrtti_typeid<DataType>());
            
            if (mapIter != m_entityData.end())
            {
                return azrtti_cast<DataType*>(mapIter->second);
            }
            
            return nullptr;
        }
        
        template<class DataType>
        DataType* FindSaveDataAs() const
        {
            return FindSaveData<DataType>();
        }

        template<class DataType>
        DataType* FindCreateSaveData()
        {
            DataType* dataType = FindSaveDataAs<DataType>();

            if (dataType == nullptr)
            {
                dataType = CreateSaveData<DataType>();
            }

            return dataType;
        }
        
    private:
        AZStd::unordered_map< AZ::Uuid, ComponentSaveData* > m_entityData;
    };

    template<class DataType>
    class SceneMemberComponentSaveData
        : public ComponentSaveData
        , public SceneMemberNotificationBus::Handler
        , public EntitySaveDataRequestBus::Handler
    {
    public:
        AZ_RTTI(ComponentSaveData, "{2DF9A652-DF5D-43B1-932F-B6A838E36E97}");
        AZ_CLASS_ALLOCATOR(ComponentSaveData, AZ::SystemAllocator, 0);

        SceneMemberComponentSaveData()
        {
        }

        ~SceneMemberComponentSaveData() = default;

        void Activate(const AZ::EntityId& memberId)
        {
            SceneMemberNotificationBus::Handler::BusConnect(memberId);
            EntitySaveDataRequestBus::Handler::BusConnect(memberId);
        }

        // SceneMemberNotificationBus::Handler
        void OnSceneSet(const AZ::EntityId& graphId)
        {
            const AZ::EntityId* ownerId = SceneMemberNotificationBus::GetCurrentBusId();

            if (ownerId)
            {
                RegisterIds((*ownerId), graphId);
            }

            SceneMemberNotificationBus::Handler::BusDisconnect();
        }
        ////

        // EntitySaveDataRequestBus
        void WriteSaveData(EntitySaveDataContainer& saveDataContainer) const override
        {
            AZ_Assert(azrtti_cast<const DataType*>(this) != nullptr, "Cannot automatically read/write to non-derived type in SceneMemberComponentSaveData");

            if (RequiresSave())
            {
                DataType* saveData = saveDataContainer.FindCreateSaveData<DataType>();

                if (saveData)
                {
                    (*saveData) = (*azrtti_cast<const DataType*>(this));
                }
            }
            else
            {
                saveDataContainer.RemoveSaveData<DataType>();
            }
        }

        void ReadSaveData(const EntitySaveDataContainer& saveDataContainer) override
        {
            AZ_Assert(azrtti_cast<DataType*>(this) != nullptr, "Cannot automatically read/write to non-derived type in SceneMemberComponentSaveData");

            DataType* saveData = saveDataContainer.FindSaveDataAs<DataType>();

            if (saveData)
            {
                (*azrtti_cast<DataType*>(this)) = (*saveData);
            }
        }
        ////

    protected:
        virtual bool RequiresSave() const = 0;
    };
} // namespace GraphCanvas