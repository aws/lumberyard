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
#include "StdAfx.h"

#include "../NetworkGridmateMarshaling.h"
#include "../NetworkGridmateDebug.h"

#include "EntityReplicaSpawnParams.h"

#include "../Compatibility/GridMateRMI.h"

#include <GridMate/Serialize/CompressionMarshal.h>

namespace GridMate
{
    static const unsigned int kMaxExtFlags = 8; // maximum number of extended flags supported
    static const unsigned int kMaxStrLen = 255; // maximum length of strings(name, className, archetype) in spawn parameters

    class CryNameMarshaler
    {
    public:
        void Marshal(WriteBuffer& wb, const string& name)
        {
            AZ_Assert(name.size() <= kMaxStrLen, "String is too long");
            wb.Write(static_cast<AZ::u8>(name.size()));
            wb.WriteRaw(name.data(), name.size());
        }

        void Unmarshal(string& name, ReadBuffer& rb)
        {
            AZ::u8 len = 0;
            rb.Read(len);
            char str[kMaxStrLen];
            rb.ReadRaw(str, len);
            name.assign(str, str + len);
        }
    };
    //-----------------------------------------------------------------------------
    //-----------------------------------------------------------------------------
    EntitySpawnParamsStorage::EntitySpawnParamsStorage()
        : m_channelId(kInvalidChannelId)
        , m_orientation(IDENTITY)
        , m_paramsFlags(0)
    {
    }

    EntitySpawnParamsStorage::EntitySpawnParamsStorage(const EntitySpawnParamsStorage& another)
        : m_id(another.m_id)
        , m_guid(another.m_guid)
        , m_entityName(another.m_entityName)
        , m_className(another.m_className)
        , m_archetypeName(another.m_archetypeName)
        , m_flags(another.m_flags)
        , m_flagsExtended(another.m_flagsExtended)
        , m_channelId(another.m_channelId)
        , m_position(another.m_position)
        , m_orientation(another.m_orientation)
        , m_scale(another.m_scale)
        , m_paramsFlags(another.m_paramsFlags)
    {

    }


    //-----------------------------------------------------------------------------
    EntitySpawnParamsStorage::EntitySpawnParamsStorage(const SEntitySpawnParams& sourceParams,
        ChannelId channelId,
        uint32 paramsFlags /*= kParamsFlag_None*/)
    {
        AZ_Assert(sourceParams.nFlagsExtended <= ((1 << kMaxExtFlags) - 1), "Too many extended flags, must be < %d", kMaxExtFlags);
        AZ_Assert(sourceParams.sName && strlen(sourceParams.sName) <= kMaxStrLen, "Name is too long, must be < %d", kMaxStrLen);
        AZ_Assert(!sourceParams.pClass || strlen(sourceParams.pClass->GetName()) <= kMaxStrLen, "Class name is too long, must be < %d", kMaxStrLen);
        AZ_Assert(!sourceParams.pArchetype || strlen(sourceParams.pArchetype->GetName()) <= kMaxStrLen, "Archetype name is too long, must be < %d", kMaxStrLen);

        m_id = sourceParams.id;
        m_guid = sourceParams.guid;
        m_entityName = sourceParams.sName;
        m_className = sourceParams.pClass ? sourceParams.pClass->GetName() : "";
        m_archetypeName = sourceParams.pArchetype ? sourceParams.pArchetype->GetName() : "";
        m_flags = sourceParams.nFlags;
        m_flagsExtended = sourceParams.nFlagsExtended;
        m_position = sourceParams.vPosition;
        m_orientation = sourceParams.qRotation;
        m_scale = sourceParams.vScale;

        m_channelId = channelId;
        m_paramsFlags = paramsFlags;

        if (sourceParams.bCreatedThroughPool)
        {
            m_paramsFlags |= kParamsFlag_CreatedThroughPool;
        }
    }

    //-----------------------------------------------------------------------------
    SEntitySpawnParams EntitySpawnParamsStorage::ToEntitySpawnParams() const
    {
        SEntitySpawnParams spawnParams;
        spawnParams.id = m_id;
        spawnParams.guid = m_guid;
        spawnParams.sName = m_entityName.c_str();

        IEntitySystem* entitySystem = GetEntitySystem();
        IEntityClassRegistry* classRegistry = entitySystem ? entitySystem->GetClassRegistry() : nullptr;

        spawnParams.pClass = classRegistry ? classRegistry->FindClass(m_className.c_str()) : nullptr;

        GM_ASSERT_TRACE(spawnParams.pClass, "Could not locate entity class \"%s\".", m_className.c_str());

        if (!m_archetypeName.empty() && m_archetypeName[ 0 ])
        {
            spawnParams.pArchetype = entitySystem->LoadEntityArchetype(m_archetypeName.c_str());
            GM_ASSERT_TRACE(spawnParams.pClass, "Could not locate entity archetype \"%s\".", m_archetypeName.c_str());
        }

        spawnParams.nFlags = m_flags;
        spawnParams.nFlagsExtended = m_flags;
        spawnParams.bCreatedThroughPool = IsCreatedThroughPool();

        spawnParams.vPosition = m_position;
        spawnParams.qRotation = m_orientation;
        spawnParams.vScale = m_scale;

        return spawnParams;
    }

    //-----------------------------------------------------------------------------
    //-----------------------------------------------------------------------------
    void EntitySpawnParamsStorage::Marshaler::Marshal(GridMate::WriteBuffer& wb, const EntitySpawnParamsStorage& s)
    {
        static_assert(kParamsFlag_Max <= BIT(4) + 1, "Using 4 bits for params flags");
        static_assert(kMarshalFlag_Max <= BIT(4) + 1, "Using 4 bits for marshal flags");

        auto flagsMarker = wb.InsertMarker<AZ::u8>();

        AZ::u8 marshalFlags = 0;

        wb.Write(s.m_id);

        if (s.m_guid)
        {
            wb.Write(s.m_guid);
            marshalFlags |= kMarshalFlag_HasGuid;
        }

        wb.Write(s.m_flags, VlqU32Marshaler());
        wb.Write(s.m_flagsExtended);
        wb.Write(s.m_channelId);

        wb.Write(s.m_position);
        if (s.m_orientation.IsUnit())
        {
            AZ::Quaternion ori(s.m_orientation.v[0], s.m_orientation.v[1], s.m_orientation.v[2], s.m_orientation.w);
            wb.Write(ori, QuatCompNormMarshaler());
            marshalFlags |= kMarshalFlag_OrientationNorm;
        }
        else
        {
            wb.Write(s.m_orientation);
        }

        if (!s.m_scale.IsEquivalent(Vec3(1.f, 1.f, 1.f)))
        {
            wb.Write(s.m_scale);
            marshalFlags |= kMarshalFlag_HasScale;
        }

        wb.Write(s.m_entityName, CryNameMarshaler());
        wb.Write(s.m_className, CryNameMarshaler());
        if (!s.m_archetypeName.empty())
        {
            wb.Write(s.m_archetypeName, CryNameMarshaler());
            marshalFlags |= kMarshalFlag_HasArchetype;
        }

        flagsMarker = (marshalFlags << 4) | (s.m_paramsFlags & 0xF); // hi 4 bits - marshal flags, low 4 bits - params flags
    }

    //-----------------------------------------------------------------------------
    void EntitySpawnParamsStorage::Marshaler::Unmarshal(EntitySpawnParamsStorage& s, GridMate::ReadBuffer& rb)
    {
        AZ::u8 flags = 0;

        rb.Read(flags);
        AZ::u8 marshalFlags = flags >> 4; // hi 4 bits - marshal flags, low 4 bits - params flags
        s.m_paramsFlags = flags & 0xF;

        rb.Read(s.m_id);

        s.m_guid = 0;
        if (marshalFlags & kMarshalFlag_HasGuid)
        {
            rb.Read(s.m_guid);
        }

        rb.Read(s.m_flags, VlqU32Marshaler());
        rb.Read(s.m_flagsExtended);
        rb.Read(s.m_channelId);

        rb.Read(s.m_position);
        if (marshalFlags & kMarshalFlag_OrientationNorm)
        {
            AZ::Quaternion ori;
            rb.Read(ori, QuatCompNormMarshaler());
            s.m_orientation = Quat(ori.GetW(), ori.GetX(), ori.GetY(), ori.GetZ());
        }
        else
        {
            rb.Read(s.m_orientation);
        }

        s.m_scale = Vec3(1.f, 1.f, 1.f);
        if (marshalFlags & kMarshalFlag_HasScale)
        {
            rb.Read(s.m_scale);
        }

        rb.Read(s.m_entityName, CryNameMarshaler());
        rb.Read(s.m_className, CryNameMarshaler());

        s.m_archetypeName.clear();
        if (marshalFlags & kMarshalFlag_HasArchetype)
        {
            rb.Read(s.m_archetypeName, CryNameMarshaler());
        }
    }

    //-----------------------------------------------------------------------------
    void EntityExtraSpawnInfo::Marshaler::Marshal(GridMate::WriteBuffer& wb, const EntityExtraSpawnInfo::Ptr& v)
    {
        if (v)
        {
            EntityExtraSpawnInfo::DataBuffer::Marshaler().Marshal(wb, v->m_buffer);
        }
        else
        {
            EntityExtraSpawnInfo::DataBuffer::Marshaler().Marshal(wb, EntityExtraSpawnInfo::DataBuffer());
        }
    }

    //-----------------------------------------------------------------------------
    void EntityExtraSpawnInfo::Marshaler::Unmarshal(EntityExtraSpawnInfo::Ptr& v, GridMate::ReadBuffer& rb)
    {
        v = new EntityExtraSpawnInfo();
        EntityExtraSpawnInfo::DataBuffer::Marshaler().Unmarshal(v->m_buffer, rb);
    }
} // namespace GridMate
