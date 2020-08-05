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

#include <AzFramework/Physics/Collision.h>
#include <AzFramework/Physics/CollisionBus.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Module/Environment.h>
#include <AzCore/Component/Entity.h>
#include <AzCore/std/algorithm.h>
#include "AzCore/Interface/Interface.h"


//This bit is defined in the TouchBending Gem wscript.
//Make sure the bit has a valid value.
#ifdef TOUCHBENDING_LAYER_BIT
#if (TOUCHBENDING_LAYER_BIT < 1) || (TOUCHBENDING_LAYER_BIT > 63)
#error Invalid Bit Definition For the TouchBending Layer Bit
#endif
#endif //#ifdef TOUCHBENDING_LAYER_BIT

namespace Physics
{
    const CollisionLayer CollisionLayer::Default = 0;

    const CollisionGroup CollisionGroup::None = 0x0000000000000000ULL;
    const CollisionGroup CollisionGroup::All = 0xFFFFFFFFFFFFFFFFULL;

#ifdef TOUCHBENDING_LAYER_BIT
    const CollisionLayer CollisionLayer::TouchBend = TOUCHBENDING_LAYER_BIT;
    const CollisionGroup CollisionGroup::All_NoTouchBend = CollisionGroup::All.GetMask() & ~CollisionLayer::TouchBend.GetMask();
#endif

    void CollisionLayer::Reflect(AZ::ReflectContext* context)
    {
        if (AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<Physics::CollisionLayer>()
                ->Version(1)
                ->Field("Index", &Physics::CollisionLayer::m_index)
                ;
        }
    }

    void CollisionGroup::Reflect(AZ::ReflectContext* context)
    {
        if (AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<Physics::CollisionGroup>()
                ->Version(1)
                ->Field("Mask", &Physics::CollisionGroup::m_mask)
                ;
        }
    }
    void CollisionLayers::Reflect(AZ::ReflectContext* context)
    {
        if (AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<Physics::CollisionLayers>()
                ->Version(1)
                ->Field("LayerNames", &Physics::CollisionLayers::m_names)
                ;

            if (auto ec = serializeContext->GetEditContext())
            {
                ec->Class<Physics::CollisionLayers>("Collision Layers", "List of defined collision layers")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ->DataElement(AZ::Edit::UIHandlers::Default, &Physics::CollisionLayers::m_names, "Layers", "Names of each collision layer")
                    ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
                    ;
            }
        }
    }

    void CollisionGroups::Reflect(AZ::ReflectContext* context)
    {
        CollisionGroups::Id::Reflect(context);
        CollisionGroups::Preset::Reflect(context);

        if (AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<Physics::CollisionGroups>()
                ->Version(1)
                ->Field("GroupPresets", &Physics::CollisionGroups::m_groups)
                ;
        }
    }

    void CollisionGroups::Id::Reflect(AZ::ReflectContext* context)
    {
        if (AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<Physics::CollisionGroups::Id>()
                ->Version(1)
                ->Field("GroupId", &Physics::CollisionGroups::Id::m_id)
                ;
        }
    }

    void CollisionGroups::Preset::Reflect(AZ::ReflectContext* context)
    {
        if (AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<Physics::CollisionGroups::Preset>()
                ->Version(1)
                ->Field("Id", &Physics::CollisionGroups::Preset::m_id)
                ->Field("Name", &Physics::CollisionGroups::Preset::m_name)
                ->Field("Group", &Physics::CollisionGroups::Preset::m_group)
                ->Field("ReadOnly", &Physics::CollisionGroups::Preset::m_readOnly)
                ;
        }
    }

    void CollisionConfiguration::Reflect(AZ::ReflectContext* context)
    {
        if (AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<Physics::CollisionConfiguration>()
                ->Version(1)
                ->Field("Layers", &Physics::CollisionConfiguration::m_collisionLayers)
                ->Field("Groups", &Physics::CollisionConfiguration::m_collisionGroups)
                ;
        }
    }

    CollisionLayer::CollisionLayer(AZ::u8 index) 
        : m_index(index) 
    {
        AZ_Assert(m_index < CollisionLayers::s_maxCollisionLayers, "Index is too large. Valid values are 0-%d"
            , CollisionLayers::s_maxCollisionLayers - 1);
    }

    CollisionLayer::CollisionLayer(const AZStd::string& layerName)
    {
        CollisionLayer layer;
        Physics::CollisionRequestBus::BroadcastResult(layer, &Physics::CollisionRequests::GetCollisionLayerByName, layerName);
        m_index = layer.m_index;
    }

    AZ::u8 CollisionLayer::GetIndex() const
    {
        return m_index;
    }

    void CollisionLayer::SetIndex(AZ::u8 index)
    { 
        AZ_Assert(m_index < CollisionLayers::s_maxCollisionLayers, "Index is too large. Valid values are 0-%d"
            , CollisionLayers::s_maxCollisionLayers - 1);
        m_index = index;
    }

    AZ::u64 CollisionLayer::GetMask() const
    { 
        return 1ULL << m_index; 
    }

    CollisionGroup::CollisionGroup(AZ::u64 mask) 
        : m_mask(mask)
    {
    }

    CollisionGroup::CollisionGroup(const AZStd::string& groupName)
    {
        CollisionGroup group;
        Physics::CollisionRequestBus::BroadcastResult(group, &Physics::CollisionRequests::GetCollisionGroupByName, groupName);
        m_mask = group.GetMask();
    }

    void CollisionGroup::SetLayer(CollisionLayer layer, bool set)
    {
        if (set)
        {
            m_mask |= 1ULL << layer.GetIndex();
        }
        else
        { 
            m_mask &= ~(1ULL << layer.GetIndex());
        }
    }

    bool CollisionGroup::IsSet(CollisionLayer layer) const
    {
        return (m_mask & layer.GetMask()) != 0;
    }

    AZ::u64 CollisionGroup::GetMask() const
    {
        return m_mask;
    }

    bool CollisionGroup::operator!=(const CollisionGroup& collisionGroup) const
    {
        return collisionGroup.m_mask != m_mask;
    }

    bool CollisionGroup::operator==(const CollisionGroup& collisionGroup) const
    {
        return collisionGroup.m_mask == m_mask;
    }

    Physics::CollisionLayer CollisionLayers::GetLayer(const AZStd::string& layerName) const
    {
        Physics::CollisionLayer layer = Physics::CollisionLayer::Default;
        TryGetLayer(layerName, layer);
        return layer;
    }

    bool CollisionLayers::TryGetLayer(const AZStd::string& layerName, Physics::CollisionLayer& layer) const
    {
        if (layerName.empty())
        {
            return false;
        }

        for (AZ::u8 i = 0; i < m_names.size(); ++i)
        {
            if (m_names[i] == layerName)
            {
                layer = Physics::CollisionLayer(i);
                return true;
            }
        }
        AZ_Warning("CollisionLayers", false, "Could not find collision layer:%s. Does it exist in the physx configuration window?", layerName.c_str());
        return false;
    }

    const AZStd::string& CollisionLayers::GetName(Physics::CollisionLayer layer) const
    {
        return m_names[layer.GetIndex()];
    }

    const AZStd::array<AZStd::string, CollisionLayers::s_maxCollisionLayers>& CollisionLayers::GetNames() const
    {
        return m_names;
    }

    void CollisionLayers::SetName(Physics::CollisionLayer layer, const AZStd::string& layerName)
    {
        m_names[layer.GetIndex()] = layerName;
    }

    void CollisionLayers::SetName(AZ::u64 layerIndex, const AZStd::string& layerName)
    {
        if (layerIndex >= m_names.size())
        {
            AZ_Warning("PhysX Collision Layers", false, "Trying to set layer name of layer with invalid index: %d", layerIndex);
            return;
        }
        m_names[layerIndex] = layerName;
    }

    CollisionGroups::Id CollisionGroups::CreateGroup(const AZStd::string& name, CollisionGroup group, Id id, bool readOnly)
    {
        Preset preset;
        preset.m_id = id;
        preset.m_name = name;
        preset.m_group = group;
        preset.m_readOnly = readOnly;
        m_groups.push_back(preset);
        return preset.m_id;
    }

    void CollisionGroups::DeleteGroup(Id id)
    {
        if (!id.m_id.IsNull())
        {
            auto last = AZStd::remove_if(m_groups.begin(), m_groups.end(), [id](const Preset& preset)
            {
                return preset.m_id == id;
            });
            m_groups.erase(last);
        }
    }

    CollisionGroup CollisionGroups::FindGroupById(CollisionGroups::Id id) const
    {
        auto found = AZStd::find_if(m_groups.begin(), m_groups.end(), [id](const Preset& preset) 
        {
            return preset.m_id == id;
        });

        if (found != m_groups.end())
        {
            return found->m_group;
        }
        return CollisionGroup::All;
    }

    CollisionGroup CollisionGroups::FindGroupByName(const AZStd::string& groupName) const
    {
        CollisionGroup group = CollisionGroup::All;
        TryFindGroupByName(groupName, group);
        return group;
    }

    bool CollisionGroups::TryFindGroupByName(const AZStd::string& groupName, CollisionGroup& group) const
    {
        auto found = AZStd::find_if(m_groups.begin(), m_groups.end(), [groupName](const Preset& preset)
        {
            return preset.m_name == groupName;
        });

        if (found != m_groups.end())
        {
            group = found->m_group;
            return true;
        }

        AZ_Warning("CollisionGroups", false, "Could not find collision group:%s. Does it exist in the physx configuration window?", groupName.c_str());
        return false;
    }

    CollisionGroups::Id CollisionGroups::FindGroupIdByName(const AZStd::string& groupName) const
    {
        auto found = AZStd::find_if(m_groups.begin(), m_groups.end(), [groupName](const Preset& preset)
        {
            return preset.m_name == groupName;
        });

        if (found != m_groups.end())
        {
            return found->m_id;
        }
        return CollisionGroups::Id();
    }

    AZStd::string CollisionGroups::FindGroupNameById(Id id) const
    {
        auto found = AZStd::find_if(m_groups.begin(), m_groups.end(), [id](const Preset& preset)
        {
            return preset.m_id.m_id == id.m_id;
        });

        if (found != m_groups.end())
        {
            return found->m_name;
        }
        return "";
    }

    void CollisionGroups::SetGroupName(CollisionGroups::Id id, const AZStd::string& groupName)
    {
        auto found = AZStd::find_if(m_groups.begin(), m_groups.end(), [id](const Preset& preset)
        {
            return preset.m_id.m_id == id.m_id;
        });

        if (found != m_groups.end())
        {
            found->m_name = groupName;
        }
    }

    void CollisionGroups::SetLayer(Id id, CollisionLayer layer, bool enabled)
    {
        auto found = AZStd::find_if(m_groups.begin(), m_groups.end(), [id](const Preset& preset)
        {
            return preset.m_id.m_id == id.m_id;
        });

        if (found != m_groups.end())
        {
            found->m_group.SetLayer(layer, enabled);
        }
    }

    const AZStd::vector<CollisionGroups::Preset>& CollisionGroups::GetPresets() const
    {
        return m_groups;
    }

    CollisionGroup operator|(CollisionLayer layer1, CollisionLayer layer2)
    {
        CollisionGroup group = CollisionGroup::None;
        group.SetLayer(layer1, true);
        group.SetLayer(layer2, true);
        return group;
    }

    CollisionGroup operator|(CollisionGroup otherGroup, CollisionLayer layer)
    {
        CollisionGroup group = otherGroup;
        group.SetLayer(layer, true);
        return group;
    }
}

