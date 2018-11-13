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

// Description : Generic parameter class.


#ifndef CRYINCLUDE_CRYCOMMON_GENERICPARAM_H
#define CRYINCLUDE_CRYCOMMON_GENERICPARAM_H
#pragma once


template <class STORAGE>
class CGenericParam
    : private STORAGE
{
public:

    struct ETypeId
    {
        enum
        {
            Nil = 0,
            Unknown,
            String,
            Int32,
            UInt32,
            Int64,
            UInt64,
            Float,
            Double,
            Vec2,
            Vec3,
            Quat,
            Count
        };
    };

    using STORAGE::Data;

    CGenericParam()
        : m_size(0)
        , m_typeId(ETypeId::Nil)
        , m_filter(false)
    {
    }

    inline CGenericParam(const char* rhs, bool filter = true)
        : m_filter(filter)
    {
        Assign(rhs);
    }

    inline CGenericParam(const string& rhs, bool filter = true)
        : m_filter(filter)
    {
        Assign(rhs);
    }

    inline CGenericParam(const int32& rhs, bool filter = true)
        : m_filter(filter)
    {
        Assign(rhs);
    }

    inline CGenericParam(const uint32& rhs, bool filter = true)
        : m_filter(filter)
    {
        Assign(rhs);
    }

    inline CGenericParam(const int64& rhs, bool filter = true)
        : m_filter(filter)
    {
        Assign(rhs);
    }

    inline CGenericParam(const uint64& rhs, bool filter = true)
        : m_filter(filter)
    {
        Assign(rhs);
    }

    inline CGenericParam(const float& rhs, bool filter = true)
        : m_filter(filter)
    {
        Assign(rhs);
    }

    inline CGenericParam(const double& rhs, bool filter = true)
        : m_filter(filter)
    {
        Assign(rhs);
    }

    inline CGenericParam(const Vec2& rhs, bool filter = true)
        : m_filter(filter)
    {
        Assign(rhs);
    }

    inline CGenericParam(const Vec3& rhs, bool filter = true)
        : m_filter(filter)
    {
        Assign(rhs);
    }

    inline CGenericParam(const Quat& rhs, bool filter = true)
        : m_filter(filter)
    {
        Assign(rhs);
    }

    inline CGenericParam(const CGenericParam& rhs)
        : m_filter(rhs.m_filter)
    {
        Assign(rhs);
    }

    inline uint16 Size() const
    {
        return m_size;
    }

    inline uint8 TypeId() const
    {
        return m_typeId;
    }

    inline bool Filter() const
    {
        return !!m_filter;
    }

    inline string GetString() const
    {
        CRY_ASSERT(m_typeId == ETypeId::String);

        return string(static_cast<const char*>(Data()));
    }

    inline int32 GetInt32() const
    {
        CRY_ASSERT(m_typeId == ETypeId::Int32);

        return *static_cast<const int32*>(Data());
    }

    inline uint32 GetUInt32() const
    {
        CRY_ASSERT(m_typeId == ETypeId::UInt32);

        return *static_cast<const uint32*>(Data());
    }

    inline int64 GetInt64() const
    {
        CRY_ASSERT(m_typeId == ETypeId::Int64);

        return *static_cast<const int64*>(Data());
    }

    inline uint64 GetUInt64() const
    {
        CRY_ASSERT(m_typeId == ETypeId::UInt64);

        return *static_cast<const uint64*>(Data());
    }

    inline float GetFloat() const
    {
        CRY_ASSERT(m_typeId == ETypeId::Float);

        return *static_cast<const float*>(Data());
    }

    inline double GetDouble() const
    {
        CRY_ASSERT(m_typeId == ETypeId::Double);

        return *static_cast<const double*>(Data());
    }

    inline Vec2 GetVec2() const
    {
        CRY_ASSERT(m_typeId == ETypeId::Vec2);

        return *static_cast<const Vec2*>(Data());
    }

    inline Vec3 GetVec3() const
    {
        CRY_ASSERT(m_typeId == ETypeId::Vec3);

        return *static_cast<const Vec3*>(Data());
    }

    inline Quat GetQuat() const
    {
        CRY_ASSERT(m_typeId == ETypeId::Quat);

        return *static_cast<const Quat*>(Data());
    }

    inline string ToString() const
    {
        string  output;

        switch (m_typeId)
        {
        case ETypeId::String:
        {
            output.Format("%s", static_cast<const char*>(Data()));

            break;
        }

        case ETypeId::Int32:
        {
            output.Format("%d", *static_cast<const int32*>(Data()));

            break;
        }

        case ETypeId::UInt32:
        {
            output.Format("%u", *static_cast<const uint32*>(Data()));

            break;
        }

        case ETypeId::Int64:
        {
            output.Format("%ld", *static_cast<const int64*>(Data()));

            break;
        }

        case ETypeId::UInt64:
        {
            output.Format("%lu", *static_cast<const uint64*>(Data()));

            break;
        }

        case ETypeId::Float:
        {
            output.Format("%f", *static_cast<const float*>(Data()));

            break;
        }

        case ETypeId::Double:
        {
            output.Format("%Lf", *static_cast<const double*>(Data()));

            break;
        }

        case ETypeId::Vec2:
        {
            const Vec2* pVec2 = static_cast<const Vec2*>(Data());

            output.Format("%f, %f", pVec2->x, pVec2->y);

            break;
        }

        case ETypeId::Vec3:
        {
            const Vec3* pVec3 = static_cast<const Vec3*>(Data());

            output.Format("%f, %f, %f", pVec3->x, pVec3->y, pVec3->z);

            break;
        }

        case ETypeId::Quat:
        {
            const Quat* pQuat = static_cast<const Quat*>(Data());

            output.Format("%f, %f, %f, %f", pQuat->v.x, pQuat->v.y, pQuat->v.z, pQuat->w);

            break;
        }
        }

        return output;
    }

    inline void operator = (const char* rhs)
    {
        Assign(rhs);

        m_filter = true;
    }

    inline void operator = (const string& rhs)
    {
        Assign(rhs);

        m_filter = true;
    }

    inline void operator = (const int32& rhs)
    {
        Assign(rhs);

        m_filter = true;
    }

    inline void operator = (const uint32& rhs)
    {
        Assign(rhs);

        m_filter = true;
    }

    inline void operator = (const int64& rhs)
    {
        Assign(rhs);

        m_filter = true;
    }

    inline void operator = (const uint64& rhs)
    {
        Assign(rhs);

        m_filter = true;
    }

    inline void operator = (const float& rhs)
    {
        Assign(rhs);

        m_filter = true;
    }

    inline void operator = (const double& rhs)
    {
        Assign(rhs);

        m_filter = true;
    }

    inline void operator = (const Vec2& rhs)
    {
        Assign(rhs);

        m_filter = true;
    }

    inline void operator = (const Vec3& rhs)
    {
        Assign(rhs);

        m_filter = true;
    }

    inline void operator = (const Quat& rhs)
    {
        Assign(rhs);

        m_filter = true;
    }

    inline void operator = (const CGenericParam& rhs)
    {
        Assign(rhs);

        m_filter = rhs.m_filter;
    }

    inline bool operator == (const CGenericParam& rhs) const
    {
        const void* pData = Data(), * pRHSData = rhs.Data();

        return (m_typeId == rhs.m_typeId) && (m_size == rhs.m_size) && ((pData == pRHSData) || (memcmp(pData, pRHSData, m_size) == 0));
    }

    inline bool operator != (const CGenericParam& rhs) const
    {
        const void* pData = Data(), * pRHSData = rhs.Data();

        return (m_typeId != rhs.m_typeId) || (m_size != rhs.m_size) || ((pData != pRHSData) && (memcmp(pData, pRHSData, m_size) != 0));
    }

private:

    inline void Assign(const char* rhs)
    {
        uint16  size = rhs ? strlen(rhs) + 1 : 0;

        STORAGE::Store(rhs, size);

        m_size      = size;
        m_typeId    = ETypeId::String;
    }

    inline void Assign(const string& rhs)
    {
        uint16  size = rhs.length();

        STORAGE::Store(rhs.c_str(), size);

        m_size      = size;
        m_typeId    = ETypeId::String;
    }

    inline void Assign(const int32& rhs)
    {
        STORAGE::Store(&rhs, sizeof(rhs));

        m_size      = sizeof(rhs);
        m_typeId    = ETypeId::Int32;
    }

    inline void Assign(const uint32& rhs)
    {
        STORAGE::Store(&rhs, sizeof(rhs));

        m_size      = sizeof(rhs);
        m_typeId    = ETypeId::UInt32;
    }

    inline void Assign(const int64& rhs)
    {
        STORAGE::Store(&rhs, sizeof(rhs));

        m_size      = sizeof(rhs);
        m_typeId    = ETypeId::Int64;
    }

    inline void Assign(const uint64& rhs)
    {
        STORAGE::Store(&rhs, sizeof(rhs));

        m_size      = sizeof(rhs);
        m_typeId    = ETypeId::UInt32;
    }

    inline void Assign(const float& rhs)
    {
        STORAGE::Store(&rhs, sizeof(rhs));

        m_size      = sizeof(rhs);
        m_typeId    = ETypeId::Float;
    }

    inline void Assign(const double& rhs)
    {
        STORAGE::Store(&rhs, sizeof(rhs));

        m_size      = sizeof(rhs);
        m_typeId    = ETypeId::Double;
    }

    inline void Assign(const Vec2& rhs)
    {
        STORAGE::Store(&rhs, sizeof(rhs));

        m_size      = sizeof(rhs);
        m_typeId    = ETypeId::Vec2;
    }

    inline void Assign(const Vec3& rhs)
    {
        STORAGE::Store(&rhs, sizeof(rhs));

        m_size      = sizeof(rhs);
        m_typeId    = ETypeId::Vec3;
    }

    inline void Assign(const Quat& rhs)
    {
        STORAGE::Store(&rhs, sizeof(rhs));

        m_size      = sizeof(rhs);
        m_typeId    = ETypeId::Quat;
    }

    inline void Assign(const CGenericParam& rhs)
    {
        uint16  size = rhs.m_size;

        STORAGE::Store(rhs.Data(), size);

        m_size      = size;
        m_typeId    = rhs.m_typeId;
    }

    uint16  m_size;

    uint8       m_typeId, m_filter;
};

namespace GenericParamUtils
{
    class CValueStorage
    {
    protected:

        inline CValueStorage()
            : m_pData(NULL) {}

        inline ~CValueStorage()
        {
            if (m_pData)
            {
                CryModuleFree(m_pData);
            }
        }

        inline void Store(const void* pData, size_t size)
        {
            m_pData = memcpy(CryModuleMalloc(size), pData, size);
        }

        inline const void* Data() const
        {
            return m_pData;
        }

    private:

        void* m_pData;
    };

    class CPtrStorage
    {
    protected:

        inline CPtrStorage()
            : m_pData(NULL) {}

        inline void Store(const void* pData, size_t size)
        {
            m_pData = pData;
        }

        inline const void* Data() const
        {
            return m_pData;
        }

    private:

        const void* m_pData;
    };
}

#endif // CRYINCLUDE_CRYCOMMON_GENERICPARAM_H
