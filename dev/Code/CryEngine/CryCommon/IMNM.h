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
// Original file Copyright Crytek GMBH or its affiliates, used under license.

#ifndef __IMNM_H__
#define __IMNM_H__

#pragma once

namespace MNM
{
    namespace Constants
    {
        enum Edges
        {
            InvalidEdgeIndex = ~0u
        };

        enum TileIdConstants
        {
            InvalidTileID = 0,
        };
        enum TriangleIDConstants
        {
            InvalidTriangleID = 0,
        };

        enum EStaticIsland
        {
            eStaticIsland_InvalidIslandID = 0,
            eStaticIsland_FirstValidIslandID = 1,
        };
        enum EGlobalIsland
        {
            eGlobalIsland_InvalidIslandID = 0,
        };

        enum EOffMeshLink
        {
            eOffMeshLinks_InvalidOffMeshLinkID = 0,
        };
    }

    // ------------------------------------------------------------------------------------------
    // Basic types used in the MNM namespace

    typedef uint32 TileID;
    typedef uint32 TriangleID;
    typedef uint32 OffMeshLinkID;

    // StaticIslandIDs identify triangles that are statically connected inside a mesh
    // and that are reachable without the use of any off mesh links.
    typedef uint32 StaticIslandID;

    // GlobalIslandIDs define IDs able to code and connect islands between meshes.
    struct GlobalIslandID
    {
        GlobalIslandID(const uint64 defaultValue = MNM::Constants::eGlobalIsland_InvalidIslandID)
            : id(defaultValue)
        {
            STATIC_ASSERT(sizeof(StaticIslandID) <= 4, "The maximum supported size for StaticIslandIDs is 4 bytes.");
        }

        GlobalIslandID(const uint32 navigationMeshID, const MNM::StaticIslandID islandID)
        {
            id = ((uint64)navigationMeshID << 32) | islandID;
        }

        GlobalIslandID operator=(const GlobalIslandID other)
        {
            id = other.id;
            return *this;
        }

        inline bool operator==(const GlobalIslandID& rhs) const
        {
            return id == rhs.id;
        }

        inline bool operator<(const GlobalIslandID& rhs) const
        {
            return id < rhs.id;
        }

        MNM::StaticIslandID GetStaticIslandID() const
        {
            return (MNM::StaticIslandID) (id & ((uint64)1 << 32) - 1);
        }

        uint32 GetNavigationMeshIDAsUint32() const
        {
            return static_cast<uint32>(id >> 32);
        }

        uint64 id;
    };
    // ------------------------------------------------------------------------------------------

    struct OffMeshLink
        : public _i_reference_target_t
    {
    public:

        enum LinkType
        {
            eLinkType_Invalid = -1,
            eLinkType_SmartObject,
            eLinkType_Custom,
        };

        virtual ~OffMeshLink() {};

    protected:

        OffMeshLink(LinkType _linkType, EntityId _entityId)
            : m_linkType(_linkType)
            , m_entityId(_entityId)
        {
        }

        //Do not implement
        OffMeshLink();

    public:

        ILINE LinkType GetLinkType() const { return m_linkType; }
        ILINE void SetLinkID(OffMeshLinkID linkID) { m_linkID = linkID; }
        ILINE OffMeshLinkID GetLinkId() const { return m_linkID; }
        ILINE EntityId GetEntityIdForOffMeshLink() const { return m_entityId; }

        template<class LinkDataClass>
        LinkDataClass* CastTo()
        {
            return (m_linkType == LinkDataClass::GetType()) ? static_cast<LinkDataClass*>(this) : NULL;
        }

        template<class LinkDataClass>
        const LinkDataClass* CastTo() const
        {
            return (m_linkType == LinkDataClass::GetType()) ? static_cast<const LinkDataClass*>(this) : NULL;
        }

        virtual bool CanUse(IEntity* pRequester, float* costMultiplier) const = 0;

        virtual OffMeshLink* Clone() const = 0;
        virtual Vec3 GetStartPosition() const = 0;
        virtual Vec3 GetEndPosition() const = 0;

        // Used when links are auto-sized
        virtual void SetStartPosition(const Vec3& pos) = 0;
        virtual void SetEndPosition(const Vec3& pos) = 0;

        virtual bool IsEnabled() const { return true; }

    protected:
        LinkType m_linkType;
        EntityId m_entityId;
        OffMeshLinkID m_linkID;
    };

    DECLARE_SMART_POINTERS(OffMeshLink);
}

#endif // __IMNM_H__