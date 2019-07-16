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

#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/std/containers/bitset.h>

namespace Physics
{
    /// This class represents which layer a collider exists on.
    /// A collider can only exist on a single layer, defined by the index.
    /// There is a maximum of 64 layers.
    class CollisionLayer
    {
    public:

        AZ_CLASS_ALLOCATOR(CollisionLayer, AZ::SystemAllocator, 0);
        AZ_TYPE_INFO(CollisionLayer, "{5AA459C8-2D92-46D2-9154-ED49EE4FE70E}");
        static void Reflect(AZ::ReflectContext* context);

        static const CollisionLayer Default; ///< Default collision layer
        static const CollisionLayer TouchBend; ///< Touch Bendable Vegetation collision layer

        CollisionLayer(AZ::u8 index = Default.GetIndex());
        CollisionLayer(const AZStd::string& layerName);
        virtual ~CollisionLayer() = default;

        AZ::u8 GetIndex() const;
        void SetIndex(AZ::u8 index);
        AZ::u64 GetMask() const;

    private:
        AZ::u8 m_index;
    };

    /// This class represents the layers a collider should collide with.
    /// For two colliders to collide, each layer must be present
    /// in the other colliders group.
    class CollisionGroup
    {
    public:
        AZ_CLASS_ALLOCATOR(CollisionGroup, AZ::SystemAllocator, 0);
        AZ_TYPE_INFO(CollisionGroup, "{E6DA080B-7ED1-4135-A78C-A6A5E495A43E}");
        static void Reflect(AZ::ReflectContext* context);

        static const CollisionGroup None; ///< Collide with nothing
        static const CollisionGroup All; ///< Collide with everything
        static const CollisionGroup All_NoTouchBend; ///< Collide with everything, except Touch Bendable Vegetation.

        CollisionGroup(AZ::u64 mask = All.GetMask());
        CollisionGroup(const AZStd::string& groupName);
        virtual ~CollisionGroup() = default;

        void SetLayer(const CollisionLayer& layer, bool set);
        bool IsSet(const CollisionLayer& layer) const;
        AZ::u64 GetMask() const;

    private:
        AZ::u64 m_mask;
    };

    /// Collision layers defined for the project.
    class CollisionLayers
    {
    public:
        static const AZ::u8 s_maxCollisionLayers = 64;
        AZ_CLASS_ALLOCATOR(CollisionLayers, AZ::SystemAllocator, 0);
        AZ_TYPE_INFO(CollisionLayers, "{68E7CB59-29BC-4825-AE99-182D6421EE65}");
        static void Reflect(AZ::ReflectContext* context);

        CollisionLayers() = default;
        virtual ~CollisionLayers() = default;

        Physics::CollisionLayer GetLayer(const AZStd::string& name);
        AZStd::string GetName(const Physics::CollisionLayer& layer) const;
        const AZStd::array<AZStd::string, s_maxCollisionLayers>& GetNames() const;
        void SetName(Physics::CollisionLayer layer, const AZStd::string& layerName);
        void SetName(AZ::u64 layerIndex, const AZStd::string& layerName);

    private:
        AZStd::array<AZStd::string, s_maxCollisionLayers> m_names;
    };

    /// Overloads for collision layers and groups. This lets you write code like this:
    /// CollisionGroup group = CollisionLayer(0) | CollisionLayer(1) | CollisionLayer(2).
    CollisionGroup operator|(const CollisionLayer& layer1, const CollisionLayer& layer2);
    CollisionGroup operator|(const CollisionGroup& group, const CollisionLayer& layer);

    /// Collision groups can be defined and edited in the PhysXConfiguration window. 
    /// The idea is that collision groups are authored there, and then assigned to components via the
    /// edit context by reflecting Physics::CollisionGroups::Id, or alternatively can be retrieved by
    /// name from the PhysXConfiguration.
    class CollisionGroups
    {
    public:
        AZ_CLASS_ALLOCATOR(CollisionGroups, AZ::SystemAllocator, 0);
        AZ_TYPE_INFO(CollisionGroups, "{309B0B28-F51F-48E2-972E-DA7618ED7249}");
        static void Reflect(AZ::ReflectContext* context);

        /// Id of a collision group. Mainly used by the editor for assign a collision
        /// group to an editor component.
        class Id
        {
        public:
            AZ_CLASS_ALLOCATOR(Id, AZ::SystemAllocator, 0);
            AZ_TYPE_INFO(Id, "{DFED4FE5-2292-4F07-A318-41C68DAEFE9C}");
            static void Reflect(AZ::ReflectContext* context);

            bool operator==(const Id& other) const { return m_id == other.m_id; }
            bool operator!=(const Id& other) const { return m_id != other.m_id; }
            bool operator<(const Id& other) const { return m_id < other.m_id; }
            static Id Create() { Id id; id.m_id = AZ::Uuid::Create(); return id; }
            AZ::Uuid m_id = AZ::Uuid::CreateNull();
        };

        /// A collision group defined with a name and an id which
        /// can be edited in the editor.
        struct Preset
        {
            AZ_CLASS_ALLOCATOR(Preset, AZ::SystemAllocator, 0);
            AZ_TYPE_INFO(Preset, "{032D1485-60A4-45B6-8D74-5B38C929B066}");
            static void Reflect(AZ::ReflectContext* context);

            Id m_id;
            AZStd::string m_name;
            CollisionGroup m_group;
            bool m_readOnly;
        };

        CollisionGroups() = default;
        virtual ~CollisionGroups() = default;

        Id CreateGroup(const AZStd::string& name, const CollisionGroup& group, Id id = Id::Create(), bool readOnly = false);
        void DeleteGroup(Id id);
        void SetGroupName(Id id, const AZStd::string& groupName);
        void SetLayer(Id id, CollisionLayer layer, bool enabled);
        CollisionGroup FindGroupById(Id id) const;
        CollisionGroup FindGroupByName(const AZStd::string& groupName) const;
        Id FindGroupIdByName(const AZStd::string& groupName) const;
        AZStd::string FindGroupNameById(Id id) const;
        const AZStd::vector<Preset>& GetPresets()const;

    private:
        AZStd::vector<Preset> m_groups;
    };
}
