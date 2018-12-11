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

// Description : Implements TypeInfo for ParticleParams.
//               Include only once per executable.

#ifndef CRYINCLUDE_CRYCOMMON_PARTICLEPARAMS_TYPEINFO_H
#define CRYINCLUDE_CRYCOMMON_PARTICLEPARAMS_TYPEINFO_H
#pragma once


#ifdef ENABLE_TYPE_INFO_NAMES
    #if !ENABLE_TYPE_INFO_NAMES
        #error ENABLE_TYPE_INFO_NAMES previously defined to 0
    #endif
#else
    #define ENABLE_TYPE_INFO_NAMES  1
#endif

#include "TypeInfo_impl.h"
#include "IShader_info.h"
#include "I3DEngine_info.h"
#include "ITerrain_info.h"
#include "Common_TypeInfo2.h"
#include "ParticleParams_info.h"
#include "Name_TypeInfo.h"
#include "CryTypeInfo.h"

///////////////////////////////////////////////////////////////////////
// Implementation of TCurve<> functions.

// Helper class for serialization.

template<class S>
struct SplineElem
{
    UnitFloat8  t;
    S           v;
    SAngle      d;
    SAngle      s;
    int         flags;

    STRUCT_INFO
};

// Manually define type info.
STRUCT_INFO_T_BEGIN(SplineElem, class, S)
VAR_INFO(t)
VAR_INFO(v)
VAR_INFO(d)
VAR_INFO(s)
VAR_INFO(flags)
STRUCT_INFO_T_END(SplineElem, class, S)


template<class S>
string TCurve<S>::ToString(FToString flags) const
{
    string str;
    for (int i = 0; i < num_keys(); i++)
    {
        if (i > 0)
        {
            str += ";";
        }
        key_type k = key(i);
        SplineElem<TMod> elem;
        elem.t = k.time;
        elem.v = k.value;
        elem.flags = k.flags;

        // We only serialize tangent data for point-curves (as oposed to Color or Vec3 curves).
        // Point-curves (such as OptSpline<float>), use a Point struct that saves tangent data
        // as a floating-point value. Checking either in/out tangents for a floating-point type is the 
        // indicator that we are handling a point-curve.
        if(aztypeid_cmp(aztypeid(T), aztypeid(float)))
        {
            // Given compiler template behavior, we must forcefully cast to get float values.
            elem.d = SAngle(*reinterpret_cast<float*>(&k.dd));
            elem.s = SAngle(*reinterpret_cast<float*>(&k.ds));
        }
        str += ::TypeInfo(&elem).ToString(&elem, flags);
    }
    return str;
}

template<class S>
bool TCurve<S>::FromString(cstr str_in, FFromString flags)
{
    CryStackStringT<char, 256> strTemp;

    source_spline source;

    cstr str = str_in;
    while (*str)
    {
        // Extract element string.
        while (*str == ' ')
        {
            str++;
        }
        cstr strElem = str;
        if (cstr strEnd = strchr(str, ';'))
        {
            strTemp.assign(str, strEnd);
            strElem = strTemp;
            str = strEnd + 1;
        }
        else
        {
            str = "";
        }

        // Parse element.
        SplineElem<TMod> elem = { 0, TMod(0), 0 };
        if (!::TypeInfo(&elem).FromString(&elem, strElem, FFromString().SkipEmpty(1)))
        {
            return false;
        }

        spline::SplineKey<T> key;
        key.time = elem.t;
        key.value = elem.v;
        key.flags = elem.flags;
        key.dd = T(elem.d);
        key.ds = T(elem.s);

        source.insert_key(key);
    }
    ;

    super_type::from_source(source);

    return true;
}

///////////////////////////////////////////////////////////////////////
template<class S>
struct TCurve<S>::CCustomInfo
    : CTypeInfo
{
    typedef spline::FinalizingSpline< typename TCurve<S>::source_spline, TCurve<S> > TIndirectSpline;

    CCustomInfo()
        : CTypeInfo("TCurve<>", sizeof(TThis), alignof(TThis))
    {
    }
    virtual string ToString(const void* data, FToString flags = 0, const void* def_data = 0) const
    {
        if (!HasString(*(const TThis*)data, flags, def_data))
        {
            return string();
        }
        return ((const TThis*)data)->ToString(flags);
    }
    virtual bool FromString(void* data, cstr str, FFromString flags = 0) const
    {
        return ((TThis*)data)->FromString(str, flags);
    }
    virtual bool ToValue(const void* data, void* value, const CTypeInfo& typeVal) const
    {
        if (&typeVal == this)
        {
            return *(TThis*)value = *(const TThis*)data, true;
        }
        if (typeVal.IsType<ISplineInterpolator*>())
        {
            TCurve<S>* pSource = (TCurve<S>*)data;
            TIndirectSpline*& pDest = *(TIndirectSpline**)value;
            if (!pDest)
            {
                pDest = new TIndirectSpline;
            }
            pDest->SetFinal(pSource);
            return true;
        }
        return false;
    }
    virtual bool FromValue(void* data, const void* value, const CTypeInfo& typeVal) const
    {
        if (&typeVal == this)
        {
            return *(TThis*)data = *(const TThis*)value, true;
        }
        return false;
    }
    virtual bool ValueEqual(const void* data, const void* def_data = 0) const
    {
        if (!def_data)
        {
            return ((const TThis*)data)->empty();
        }
        return *(const TThis*)data == *(const TThis*)def_data;
    }
    virtual void GetMemoryUsage(ICrySizer* pSizer, const void* data) const
    {
        ((TThis*)data)->GetMemoryUsage(pSizer);
    }
};

///////////////////////////////////////////////////////////////////////
// Implementation of CSurfaceTypeIndex::TypeInfo

const CTypeInfo& CSurfaceTypeIndex::TypeInfo() const
{
    struct SurfaceEnums
        : LegacyDynArray<CEnumInfo<uint16>::SElem>
    {
        // Enumerate game surface types.
        SurfaceEnums()
        {
            CEnumInfo<uint16>::SElem elem = { 0, "" };

            // Empty elem for 0 value.
            push_back(elem);

            // Trigger surface types loading.
            gEnv->p3DEngine->GetMaterialManager()->GetDefaultLayersMaterial();

            // Get surface types, duplicate names, store in enum array.
            ISurfaceTypeEnumerator* pSurfaceTypeEnum = gEnv->p3DEngine->GetMaterialManager()->GetSurfaceTypeManager()->GetEnumerator();
            for (ISurfaceType* pSurfaceType = pSurfaceTypeEnum->GetFirst(); pSurfaceType; pSurfaceType = pSurfaceTypeEnum->GetNext())
            {
                elem.Value = pSurfaceType->GetId();
                elem.Name = pSurfaceType->GetName();
                if (size_t nLen = strlen(elem.Name))
                {
                    elem.Name = (char*)memcpy(new char[nLen + 1], elem.Name, nLen + 1);
                }
                push_back(elem);
            }
            pSurfaceTypeEnum->Release();
        }

        ~SurfaceEnums()
        {
            // Free names.
            for (int i = size() - 1; i >= 0; i--)
            {
                if (*at(i).Name)
                {
                    delete [] at(i).Name;
                }
            }
        }
    };

    struct CCustomInfo
        : SurfaceEnums
        , CEnumInfo<uint16>
    {
        CCustomInfo()
            : CEnumInfo<uint16>("CSurfaceTypeIndex", *this)
        {}
    };
    static CCustomInfo Info;
    return Info;
}

#if defined(TEST_TYPEINFO) && defined(_DEBUG)

struct STypeInfoTest
{
    STypeInfoTest()
    {
        TestTypes<UnitFloat8>(1.f);
        TestTypes<UnitFloat8>(0.5f);
        TestTypes<UnitFloat8>(37.f / 255);
        TestTypes<UnitFloat8>(80.f / 240);
        TestTypes<UnitFloat8>(1.001f);
        TestTypes<UnitFloat8>(-78.f);

        TestTypes<SFloat16>(0.f);
        TestTypes<SFloat16>(1.f);
        TestTypes<SFloat16>(0.999f);
        TestTypes<SFloat16>(0.9999f);
        TestTypes<SFloat16>(-123.4f);
        TestTypes<SFloat16>(-123.5f);
        TestTypes<SFloat16>(-123.6f);
        TestTypes<SFloat16>(-123.456f);
        TestTypes<SFloat16>(-0.00012345f);
        TestTypes<SFloat16>(-1e-8f);
        TestTypes<SFloat16>(1e27f);

        TestTypes<UFloat16>(1.f);
        TestTypes<UFloat16>(9.87654321f);
        TestTypes<UFloat16>(0.00012345f);
        TestTypes<UFloat16>(45678.9012f);
        TestTypes<UFloat16>(1e16f);

        TestTypes<Vec3U>(Vec3(2, 3, 4));
        TestTypes<Color3B>(Color3F(1, 0.5, 0.25));
    }
};
static STypeInfoTest _ParticleTypeInfoTest;

#endif

#endif // CRYINCLUDE_CRYCOMMON_PARTICLEPARAMS_TYPEINFO_H
