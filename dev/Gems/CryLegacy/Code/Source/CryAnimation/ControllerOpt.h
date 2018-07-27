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

// Description : Optimized Quat\pos controller implementation.

#ifndef CRYINCLUDE_CRYANIMATION_CONTROLLEROPT_H
#define CRYINCLUDE_CRYANIMATION_CONTROLLEROPT_H
#pragma once

#include "CGFContent.h"
#include "QuatQuantization.h"
#include "ControllerPQ.h"


class IControllerOpt
    : public IController
{
public:
    virtual void SetRotationKeyTimes(uint32 num, char* pData) = 0;
    virtual void SetPositionKeyTimes(uint32 num, char* pData) = 0;
    virtual void SetRotationKeys(uint32 num, char* pData) = 0;
    virtual void SetPositionKeys(uint32 num, char* pData) = 0;

    virtual uint32 GetRotationFormat() const = 0;
    virtual uint32 GetRotationKeyTimesFormat() const = 0;
    virtual uint32 GetPositionFormat() const = 0;
    virtual uint32 GetPositionKeyTimesFormat() const = 0;

    virtual const char* GetRotationKeyData() const = 0;
    virtual const char* GetRotationKeyTimesData() const = 0;
    virtual const char* GetPositionKeyData() const = 0;
    virtual const char* GetPositionKeyTimesData() const = 0;
};

//TYPEDEF_AUTOPTR(IControllerOpt);
typedef IControllerOpt*  IControllerOpt_AutoPtr;

template <class _PosController, class _RotController>
class CControllerOpt
    : public IControllerOpt
    , _PosController
    , _RotController
{
public:
    //Creation interface

    CControllerOpt(){}

    ~CControllerOpt(){}

    uint32 numKeys() const
    {
        // now its hack, because num keys might be different
        return max(this->GetRotationNumCount(), this->GetPositionNumCount());
    }

    JointState GetOPS(f32 key, Quat& quat, Vec3& pos, Diag33& scale) const
    {
        return GetO(key, quat) | GetP(key, pos) | GetS(key, scale);
    }

    JointState GetOP(f32 key, Quat& quat, Vec3& pos) const
    {
        return GetO(key, quat) | GetP(key, pos);
    }

    JointState GetO(f32 key, Quat& quat) const
    {
        return this->GetRotationValue(key, quat);
    }

    JointState GetP(f32 key, Vec3& pos) const
    {
        return this->GetPositionValue(key, pos);
    }

    JointState GetS(f32 key, Diag33& pos) const
    {
        return 0;
    }

    // returns the start time
    size_t SizeOfController() const
    {
        size_t res(sizeof(*this));
        res += this->GetRotationsSize();
        res += this->GetPositionsSize();
        return res;
    }

    virtual void GetMemoryUsage(ICrySizer* pSizer) const
    {
        //static_cast<_PosController*>(this)->_PosController::GetMemoryUsage(pSizer);
        //static_cast<_RotController*>(this)->_RotController::GetMemoryUsage(pSizer);
    }

    size_t ApproximateSizeOfThis() const
    {
        size_t res(sizeof(*this));
        res += this->GetRotationsSize();
        res += this->GetPositionsSize();
        return res;
    }

    size_t GetRotationKeysNum() const
    {
        return this->GetRotationNumCount();
    }

    size_t GetPositionKeysNum() const
    {
        return this->GetPositionNumCount();
    }

    void SetRotationKeyTimes(uint32 num, char* pData)
    {
        this->SetRotKeyTimes/*_RotController::SetKeyTimes*/ (num, pData);
    }

    void SetPositionKeyTimes(uint32 num, char* pData)
    {
        this->SetPosKeyTimes/*_PosController::SetKeyTimes*/ (num, pData);
    }

    void SetRotationKeys(uint32 num, char* pData)
    {
        this->SetRotationData(num, pData);
    }

    void SetPositionKeys(uint32 num, char* pData)
    {
        this->SetPositionData(num, pData);
    }

    uint32 GetRotationFormat() const
    {
        return this->GetRotationType();
    }

    uint32 GetRotationKeyTimesFormat() const
    {
        return this->GetRotationKeyTimeFormat();
    }

    uint32 GetPositionFormat() const
    {
        return this->GetPositionType();
    }

    uint32 GetPositionKeyTimesFormat() const
    {
        return this->GetPositionKeyTimeFormat();
    }

    const char* GetRotationKeyData() const
    {
        return this->GetRotationData();
    }

    const char* GetRotationKeyTimesData() const
    {
        return this->GetRotKeyTimes();
    }

    const char* GetPositionKeyData() const
    {
        return this->GetPositionData();
    }

    const char* GetPositionKeyTimesData() const
    {
        return this->GetPosKeyTimes();
    }

    IControllerOpt* CreateController()
    {
        return (IControllerOpt*)new CControllerOpt<_PosController, _RotController>();
    }
};


// forward declarations
struct ControllerData;
static uint32   GetKeySelector(f32 normalized_time, f32& difference_time, const ControllerData& rConData);


struct ControllerData
{
    ControllerData() {}

    ControllerData(int iType, int iKeyType)
        : m_nDataOffs(0)
        , m_nKeysOffs(0)
        , m_iCount(0)
        , m_eTimeFormat(static_cast<unsigned char>(iKeyType))
        , m_eCompressionType(static_cast<unsigned char>(iType))
    {
    }


    void Init(int iType, int iKeyType)
    {
        m_nDataOffs = 0;
        m_nKeysOffs = 0;
        m_iCount = 0;
        m_eTimeFormat = static_cast<unsigned char>(iKeyType);
        m_eCompressionType = static_cast<unsigned char>(iType);
    }

    // call function to select template implementation of GetKeyData
    uint32 GetKey(f32 normalizedTime, f32& differenceTime) const
    {
        return GetKeySelector(normalizedTime, differenceTime, *this);
    }

    template<typename Type>
    uint32  GetKeyByteData(f32 normalized_time, f32& difference_time, const void* p_data) const
    {
        const Type* data = reinterpret_cast<const Type*>(p_data);

        f32 realtimef = normalized_time;
        Type realtime = (Type)realtimef;

        uint32 numKey = GetNumCount();

        Type keytime_start = data[0];
        Type keytime_end = data[numKey - 1];

        if (realtime < keytime_start)
        {
            return 0;
        }

        if (realtime >= keytime_end)
        {
            return numKey;
        }

        //-------------
        int nPos  = numKey >> 1;
        int nStep = numKey >> 2;

        // use binary search
        //TODO: Need check efficiency of []operator. Maybe wise use pointer
        while (nStep)
        {
            if (realtime < data[nPos])
            {
                nPos = nPos - nStep;
            }
            else
            if (realtime > data[nPos])
            {
                nPos = nPos + nStep;
            }
            else
            {
                break;
            }

            nStep = nStep >> 1;
        }

        // fine-tuning needed since time is not linear
        while (realtime >= data[nPos])
        {
            nPos++;
        }

        while (realtime < data[nPos - 1])
        {
            nPos--;
        }

        // possible error if encoder uses nonlinear methods!!!
        if (data[nPos] == data[nPos - 1])
        {
            difference_time = 0.0f;
        }
        else
        {
            f32 prevtime = (f32)data[nPos - 1];
            f32 time = (f32)data[nPos];
            difference_time = (realtimef - prevtime) / (time - prevtime);
        }

        CRY_ASSERT(difference_time >= 0.0f && difference_time <= 1.0f);
        return nPos;
    }


    uint32  GetKeyBitData(f32 normalized_time, f32& difference_time) const
    {
        f32 realtime = normalized_time;

        uint32 numKey = (uint32)GetHeader()->m_Size;//m_arrKeys.size();

        f32 keytime_start = (float)GetHeader()->m_Start;
        f32 keytime_end = (float)GetHeader()->m_End;
        f32 test_end = keytime_end;

        if (realtime < keytime_start)
        {
            test_end += realtime;
        }

        if (realtime < keytime_start)
        {
            difference_time = 0;
            return 0;
        }

        if (realtime >= keytime_end)
        {
            difference_time = 0;
            return numKey;
        }

        f32 internalTime = realtime - keytime_start;
        uint16 uTime = (uint16)internalTime;
        uint16 piece = (uTime / sizeof(uint16)) >> 3;
        uint16 bit = /*15 - */ (uTime % 16);
        uint16 data = GetKeyData(piece);
        uint16 left = data >> bit;

        //left
        left = data & (0xFFFF >> (15 - bit));
        uint16 leftPiece(piece);
        uint16 nearestLeft = 0;
        uint16 wBit;

        while ((wBit = GetFirstHighBit(left)) == 16)
        {
            --leftPiece;
            left = GetKeyData(leftPiece);
        }
        nearestLeft = leftPiece * 16 + wBit;

        //right
        uint16 right = ((data >> (bit + 1)) & 0xFFFF) << (bit + 1);
        uint16 rigthPiece(piece);
        uint16 nearestRight = 0;

        while ((wBit = GetFirstLowBit(right)) == 16)
        {
            ++rigthPiece;
            right = GetKeyData(rigthPiece);
        }

        nearestRight = ((rigthPiece  * sizeof(uint16)) << 3) + wBit;
        difference_time = (f32)(internalTime - (f32)nearestLeft) / ((f32)nearestRight - (f32)nearestLeft);

        // count nPos
        uint32 nPos(0);
        for (uint16 i = 0; i < rigthPiece; ++i)
        {
            uint16 data2 = GetKeyData(i);
            nPos += ControllerHelper::m_byteTable[data2 & 255] + ControllerHelper::m_byteTable[data2 >> 8];
        }

        data = GetKeyData(rigthPiece);
        data = ((data <<  (15 - wBit)) & 0xFFFF) >> (15 - wBit);
        nPos += ControllerHelper::m_byteTable[data & 255] + ControllerHelper::m_byteTable[data >> 8];

        return nPos - 1;
    }

    // util functions for bitset encoding
    struct Header
    {
        uint16      m_Start;
        uint16    m_End;
        uint16      m_Size;
    };

    inline const Header* GetHeader() const
    {
        return (const Header*)GetData();
    };

    ILINE char* GetData() { return reinterpret_cast<char*>(this) + m_nDataOffs; }
    ILINE char* GetKeys() { return reinterpret_cast<char*>(this) + m_nKeysOffs; }
    ILINE const char* GetData() const { return reinterpret_cast<const char*>(this) + m_nDataOffs; }
    ILINE const char* GetKeys() const { return reinterpret_cast<const char*>(this) + m_nKeysOffs; }

    inline uint16 GetKeyData(int i) const
    {
        const uint16* pData = reinterpret_cast<const uint16*>(GetData());
        return pData[3 + i];
    };

    size_t GetKeysNum() const   { return GetNumCount(); }
    size_t GetNumCount() const  { return static_cast<size_t>(getTimeFormat() == eBitset ? GetHeader()->m_Size : m_iCount); }

    EKeyTimesFormat getTimeFormat() const { return static_cast<EKeyTimesFormat>(m_eTimeFormat); }
    ECompressionInformation getCompressionType() const { return static_cast<ECompressionInformation>(m_eCompressionType); }

    void GetMemoryUsage(ICrySizer* pSizer) const
    {
        pSizer->AddObject(this, sizeof(*this));
    }

    // Data will be within same allocation as controllers, so offsets ought to be less than 2gb away
    int32 m_nDataOffs;
    int32 m_nKeysOffs;

    uint16 m_iCount;

    // using unsigned chars to store enums to save storage space
    unsigned char m_eTimeFormat;
    unsigned char m_eCompressionType;
};


// If the size of this structure changes, RC_GetSizeOfControllerOptNonVirtual (see dev/Code/Tools/RC/ResourceCompilerPC/CGA/TrackStorage.cpp) also needs
// to be updated.

class CControllerOptNonVirtual
    : public IControllerOpt
{
public:

    // ============ new interface ===========//
public:

    CControllerOptNonVirtual() {};

    CControllerOptNonVirtual(int iRotType, int iRotKeyType, int iPosType, int iPosKeyType)
        : m_position(iPosType, iPosKeyType)
        , m_rotation(iRotType, iRotKeyType)
    {
    }

    void Init(int iRotType, int iRotKeyType, int iPosType, int iPosKeyType)
    {
        m_position.Init(iPosType, iPosKeyType);
        m_rotation.Init(iRotType, iRotKeyType);
    }

    // Deliberately nerf'd - instance must be immutable, as it can be moved by defrag
    void AddRef() {}
    void Release() {}

    ~CControllerOptNonVirtual(){}

    // returns the orientation,position and scaling of the controller at the given time
    JointState GetOPS(f32 normalizedTime, Quat& quat, Vec3& pos, Diag33& scale) const
    {
        return
            GetO(normalizedTime, quat) |
            GetP(normalizedTime, pos) |
            GetS(normalizedTime, scale);
    }

    JointState GetOP(f32 normalizedTime, Quat& quat, Vec3& pos) const;
    JointState GetO(f32 normalizedTime, Quat& quat) const;

    JointState GetP(f32 normalizedTime, Vec3& pos) const
    {
        if (eNoFormat == m_position.getTimeFormat())
        {
            return 0;
        }
        pos = this->CControllerOptNonVirtual::GetPosValue(normalizedTime);
        return eJS_Position;
    }

    JointState GetS(f32 normalizedTime, Diag33& pos) const { return 0; }

    Vec3 GetPosValue(f32 normalizedTime) const
    {
        //DEFINE_PROFILER_SECTION("ControllerPQ::GetValue");
        Vec3 pos;

        f32 t;
        uint32 key = m_position.GetKey(normalizedTime, t);

        IF (key == 0, true)
        {
            CryPrefetch(m_position.GetKeys());
            GetPosValueFromKey(0, pos);
        }
        else
        {
            IF (key < m_position.GetNumCount(), true)
            {
                // assume that the 48bit(6byte) encodings are used(can be wrong but should be right the most time)
                const char* pKeys = m_position.GetKeys();
                CryPrefetch(&pKeys[(key - 1) * 6]);
                CryPrefetch(&pKeys[key * 6]);

                DEFINE_ALIGNED_DATA(Vec3, p1, 16);
                DEFINE_ALIGNED_DATA(Vec3, p2, 16);

                GetPosValueFromKey(key - 1, p1);
                GetPosValueFromKey(key, p2);

                pos.SetLerp(p1, p2, t);
            }
            else
            {
                GetPosValueFromKey(m_position.GetNumCount() - 1, pos);
            }
        }

        return pos;
    }


    Quat GetRotValue(f32 normalizedTime) const
    {
        //DEFINE_PROFILER_SECTION("ControllerPQ::GetValue");
        Quat pos;

        f32 t;
        uint32 key = m_rotation.GetKey(normalizedTime, t);

        IF (key == 0, true)
        {
            CryPrefetch(m_rotation.GetKeys());

            DEFINE_ALIGNED_DATA (Quat, p1, 16);
            GetRotValueFromKey(0, p1);
            pos = p1;
        }
        else
        {
            IF (key < m_rotation.GetNumCount(), true)
            {
                // assume that the 48bit(6byte) encodings are used(can be wrong but should be right the most time)
                const char* pKeys = m_rotation.GetKeys();
                CryPrefetch(&pKeys[(key - 1) * 6]);
                CryPrefetch(&pKeys[key * 6]);

                //  Quat p1, p2;
                DEFINE_ALIGNED_DATA (Quat, p1, 16);
                DEFINE_ALIGNED_DATA (Quat, p2, 16);

                GetRotValueFromKey(key - 1, p1);
                GetRotValueFromKey(key, p2);

                pos.SetNlerp(p1, p2, t);
            }
            else
            {
                DEFINE_ALIGNED_DATA(Quat, p1, 16);
                GetRotValueFromKey(m_rotation.GetNumCount() - 1, p1);
                pos = p1;
            }
        }

        return pos;
    }

    template<typename CompressionType, typename ValueType>
    void load_value(uint32 key, const char* data, ValueType& val) const
    {
        const CompressionType* p = reinterpret_cast<const CompressionType*>(data);
        p[key].ToExternalType(val);
    }


    void GetRotValueFromKey(uint32 key, Quat& val) const
    {
        ECompressionInformation format = m_rotation.getCompressionType();
        const char* pKeys = m_rotation.GetKeys();

        // branches ordered by probability
        IF (format == eSmallTree48BitQuat, true)
        {
            load_value<SmallTree48BitQuat>(key, pKeys, val);
        }
        else
        {
            IF (format == eSmallTree64BitExtQuat, true)
            {
                load_value<SmallTree64BitExtQuat>(key, pKeys, val);
            }
            else
            {
                IF (format == eSmallTree64BitQuat, true)
                {
                    load_value<SmallTree64BitQuat>(key, pKeys, val);
                }
                else
                {
                    IF (format == eNoCompressQuat, true)
                    {
                        load_value<NoCompressQuat>(key, pKeys, val);
                    }
                    else
                    {
                        CryFatalError("Unknown Rotation Compression format %i\n", format);
                    }
                }
            }
        }
    }

    void GetPosValueFromKey(uint32 key, Vec3& val) const
    {
        // branches ordered by probability
        IF (m_position.getCompressionType() == eNoCompressVec3, 1)
        {
            load_value<NoCompressVec3>(key, m_position.GetKeys(), val);
        }
        else
        {
            val = ZERO;
            //CryFatalError("Unknown Position Compression format %i", m_position.getCompressionType());
        }
    }


    void SetRotationKeyTimes(uint32 num, char* pData)
    {
        if (eNoFormat == m_rotation.getTimeFormat())
        {
            return;
        }

        m_rotation.m_iCount = static_cast<uint16>(num);
        m_rotation.m_nDataOffs = static_cast<int32>(pData - reinterpret_cast<char*>(&m_rotation));
    }

    void SetPositionKeyTimes(uint32 num, char* pData)
    {
        if (eNoFormat == m_position.getTimeFormat())
        {
            return;
        }

        m_position.m_iCount = static_cast<uint16>(num);
        m_position.m_nDataOffs = static_cast<int32>(pData - reinterpret_cast<char*>(&m_position));
    }

    void SetRotationKeys(uint32 num, char* pData)
    {
        if (eNoFormat == m_rotation.getTimeFormat())
        {
            return;
        }

        m_rotation.m_nKeysOffs = static_cast<int32>(pData - reinterpret_cast<char*>(&m_rotation));
    }

    void SetPositionKeys(uint32 num, char* pData)
    {
        if (eNoFormat == m_position.getTimeFormat())
        {
            return;
        }

        m_position.m_nKeysOffs = static_cast<int32>(pData - reinterpret_cast<char*>(&m_position));
    }

    uint32 GetRotationFormat() const { return m_rotation.getCompressionType(); }
    uint32 GetRotationKeyTimesFormat() const { return m_rotation.getTimeFormat(); }

    uint32 GetPositionFormat() const { return m_position.getCompressionType(); }
    uint32 GetPositionKeyTimesFormat() const { return m_position.getTimeFormat(); }

    const char* GetRotationKeyData() const { return m_rotation.GetKeys(); }
    const char* GetRotationKeyTimesData() const { return m_rotation.GetData(); }

    const char* GetPositionKeyData() const { return m_position.GetKeys(); }
    const char* GetPositionKeyTimesData() const { return m_position.GetData(); }

    size_t SizeOfController() const
    {
        return sizeof(*this);
    }
    size_t ApproximateSizeOfThis() const
    {
        int sizeOfRotKey = ControllerHelper::GetRotationFormatSizeOf(m_rotation.getCompressionType());
        int sizeOfPosKey = ControllerHelper::GetPositionsFormatSizeOf(m_position.getCompressionType());

        return sizeof(*this) + sizeOfRotKey * m_rotation.GetNumCount() + sizeOfPosKey * m_position.GetNumCount();
    }


    virtual size_t GetRotationKeysNum() const { return m_rotation.GetNumCount(); }
    virtual size_t GetPositionKeysNum() const { return m_position.GetNumCount(); }

    virtual void GetMemoryUsage(ICrySizer* pSizer) const{}

private:
    ControllerData m_rotation;
    ControllerData m_position;
};

TYPEDEF_AUTOPTR(CControllerOptNonVirtual);

static uint32   GetKeySelector(f32 normalized_time, f32& difference_time, const ControllerData& rConData)
{
    const void* data = rConData.GetData();

    EKeyTimesFormat format = rConData.getTimeFormat();

    // branches ordered by probability
    IF (format == eByte, true)
    {
        return rConData.GetKeyByteData<uint8>(normalized_time, difference_time, data);
    }
    else
    {
        IF (format == eUINT16, 1)
        {
            return rConData.GetKeyByteData<uint16>(normalized_time, difference_time, data);
        }
        else
        {
            IF (format == eF32, 1)
            {
                return rConData.GetKeyByteData<f32>(normalized_time, difference_time, data);
            }
            else
            {
                return rConData.GetKeyBitData(normalized_time, difference_time);
            }
        }
    }
}

#endif // CRYINCLUDE_CRYANIMATION_CONTROLLEROPT_H

