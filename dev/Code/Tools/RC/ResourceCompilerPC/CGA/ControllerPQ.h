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

// Description : Quat\pos controller implementation.

#ifndef CRYINCLUDE_TOOLS_RC_RESOURCECOMPILERPC_CGA_CONTROLLERPQ_H
#define CRYINCLUDE_TOOLS_RC_RESOURCECOMPILERPC_CGA_CONTROLLERPQ_H
#pragma once


#include "CGFContent.h"
#include "QuatQuantization.h"

#include "wavelets/Compression.h"

typedef Quat TRotation;
typedef Vec3 TPosition;
typedef Vec3 TScale;


enum EKeyTimesFormat
{
    eF32,
    eUINT16,
    eByte,
    eF32StartStop,
    eUINT16StartStop,
    eByteStartStop,
    eBitset
};


class IPositionController;
class IRotationController;
class IKeyTimesInformation;
class ITrackPositionStorage;
class ITrackRotationStorage;

namespace ControllerHelper
{
    ITrackPositionStorage* GetPositionControllerPtr(ECompressionFormat format);
    ITrackRotationStorage* GetRotationControllerPtr(ECompressionFormat format);
    IKeyTimesInformation* GetKeyTimesControllerPtr(uint32);

    uint32 GetPositionsFormatSizeOf(uint32 format);
    uint32 GetRotationFormatSizeOf(uint32 format);
    uint32 GetKeyTimesFormatSizeOf(uint32 format);
    
    static const int byteArraySize = 256;
    extern const uint8 m_byteLowBit[byteArraySize];
    extern const uint8 m_byteHighBit[byteArraySize];
}

struct F32Encoder
{
    static EKeyTimesFormat GetFormat() { return eF32; };
};

struct UINT16Encoder
{
    static EKeyTimesFormat GetFormat() { return eUINT16; };
};

struct ByteEncoder
{
    static EKeyTimesFormat GetFormat() { return eByte; };
};

struct F32StartStopEncoder
{
    static EKeyTimesFormat GetFormat() { return eF32StartStop; };
};

struct UINT16StartStopEncoder
{
    static EKeyTimesFormat GetFormat() { return eUINT16StartStop; };
};

struct ByteStartStopEncoder
{
    static EKeyTimesFormat GetFormat() { return eByteStartStop; };
};

struct BitsetEncoder
{
    static EKeyTimesFormat GetFormat() { return eBitset; };
};

class IKeyTimesInformation
    : public _reference_target_t
{
public:
    virtual ~IKeyTimesInformation() {}
    //  virtual TKeyTime GetKeyValue(uint32 key) const = 0;
    // return key value in f32 format
    virtual f32 GetKeyValueFloat(uint32 key) const  = 0;

    virtual void  SetKeyValueFloat(uint32 key, float time)  = 0;
    // return key number for normalized_time
    virtual uint32  GetKey(f32 normalized_time, f32& difference_time) = 0;

    virtual uint32 GetNumKeys() const = 0;

    virtual uint32 GetDataCount() const = 0;

    virtual void AddKeyTime(f32 val) = 0;

    virtual void ReserveKeyTime(uint32 count) = 0;

    virtual void ResizeKeyTime(uint32 count) = 0;

    virtual uint32 GetFormat() = 0;

    virtual char* GetData() = 0;

    virtual uint32 GetDataRawSize() = 0;

    virtual void Clear() = 0;

    virtual void SwapBytes() = 0;
};



template <class TKeyTime, class _Encoder>
class CKeyTimesInformation
    : public IKeyTimesInformation
{
public:
    CKeyTimesInformation()
        : m_lastTime(-FLT_MAX) {}

    // Return decoded keytime value from array. Useful for compression
    //  inline f32 DecodeKeyValue(TKeyTime key) const { return key; } // insert decompression code here. Example (f32)m_arrKeys[key]/60 - keytimes stores in short int format
    //Return encoded value for comparison in getKey
    //  inline TKeyTime EncodeKeyValue(f32 val) const { return val; }; // insert compression code here. Example (TKeyTime)(val * 60) - keytimes stores in short int format

    // return key value
    TKeyTime GetKeyValue(uint32 key) const { return m_arrKeys[key]; };

    // return key value in f32 format
    f32 GetKeyValueFloat(uint32 key) const { return /*_Encoder::DecodeKeyValue*/ (m_arrKeys[key]); }

    void SetKeyValueFloat(uint32 key, float f) {m_arrKeys[key] = (TKeyTime)f; }

    // return key number for normalized_time
    uint32  GetKey(f32 realtime, f32& difference_time)
    {
        if (realtime == m_lastTime)
        {
            difference_time =  m_LastDifferenceTime;
            return m_lastKey;
        }

        m_lastTime = realtime;


        uint32 numKey = m_arrKeys.size();

        TKeyTime keytime_start = m_arrKeys[0];
        TKeyTime keytime_end = m_arrKeys[numKey - 1];

        f32 test_end = keytime_end;
        if (realtime < keytime_start)
        {
            test_end += realtime;
        }

        if (realtime < keytime_start)
        {
            m_lastKey = 0;
            return 0;
        }

        if (realtime >= keytime_end)
        {
            m_lastKey = numKey;
            return numKey;
        }


        int nPos  = numKey >> 1;
        int nStep = numKey >> 2;

        // use binary search
        //TODO: Need check efficiency of []operator. Maybe wise use pointer
        while (nStep)
        {
            if (realtime < m_arrKeys[nPos])
            {
                nPos = nPos - nStep;
            }
            else
            if (realtime > m_arrKeys[nPos])
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
        while (realtime > m_arrKeys[nPos])
        {
            nPos++;
        }

        while (realtime < m_arrKeys[nPos - 1])
        {
            nPos--;
        }

        m_lastKey = nPos;

        // possible error if encoder uses nonlinear methods!!!
        m_LastDifferenceTime = difference_time = /*_Encoder::DecodeKeyValue*/ (f32)(realtime - (f32)m_arrKeys[nPos - 1]) / ((f32)m_arrKeys[nPos] - (f32)m_arrKeys[nPos - 1]);

        return nPos;
    }

    uint32 GetNumKeys() const {return m_arrKeys.size(); };

    virtual uint32 GetDataCount() const {return m_arrKeys.size(); };

    virtual void Clear()
    {
        m_arrKeys.clear();
    }

    void AddKeyTime(f32 val)
    {
        m_arrKeys.push_back(/*_Encoder::EncodeKeyValue*/ (TKeyTime)(val));
    }

    void ReserveKeyTime(uint32 count)
    {
        m_arrKeys.reserve(count);
    }

    void ResizeKeyTime(uint32 count)
    {
        m_arrKeys.resize(count);
    }

    uint32 GetFormat()
    {
        return _Encoder::GetFormat();
    }

    char* GetData()
    {
        return (char*) m_arrKeys.begin();
    }

    uint32 GetDataRawSize()
    {
        return (uint32)((char*)m_arrKeys.end() - (char*)m_arrKeys.begin());
    }

    virtual void SwapBytes()
    {
        SwapEndianness(&m_arrKeys[0], m_arrKeys.size());
    }

private:
    DynArray<TKeyTime> m_arrKeys;
    f32 m_lastTime;
    f32 m_LastDifferenceTime;
    uint32 m_lastKey;
};

typedef _smart_ptr<IKeyTimesInformation> KeyTimesInformationPtr;

typedef CKeyTimesInformation<f32, F32Encoder> F32KeyTimesInformation;
typedef CKeyTimesInformation<uint16, UINT16Encoder> UINT16KeyTimesInformation;
typedef CKeyTimesInformation<uint8, ByteEncoder> ByteKeyTimesInformation;





class ITrackPositionStorage
    : public _reference_target_t
{
public:
    virtual void AddValue(Vec3& val) = 0;
    virtual void SetValueForKey(uint32 key, Vec3& val) = 0;
    virtual char* GetData() = 0;
    virtual uint32 GetDataRawSize() const = 0;
    virtual void Resize(uint32 count) = 0;
    virtual void Reserve(uint32 count) = 0;
    virtual uint32 GetFormat() const = 0;
    virtual void GetValue(uint32 key, Vec3& val) const = 0;
    virtual uint32 GetNumKeys() const = 0;
    virtual uint32 GetDataCount() const = 0;
    virtual void SwapBytes() = 0;
};

typedef _smart_ptr<ITrackPositionStorage> TrackPositionStoragePtr;


class ITrackRotationStorage
    : public _reference_target_t
{
public:
    virtual void AddValue(Quat& val) = 0;
    virtual void SetValueForKey(uint32 key, Quat& val) = 0;
    virtual char* GetData() = 0;
    virtual uint32 GetDataRawSize() const = 0;
    virtual void Resize(uint32 count) = 0;
    virtual void Reserve(uint32 count) = 0;
    virtual uint32 GetFormat() const = 0;
    virtual void GetValue(uint32 key, Quat& val) const = 0;
    virtual uint32 GetNumKeys() const = 0;
    virtual uint32 GetDataCount() const = 0;
    virtual void SwapBytes() = 0;
};

typedef _smart_ptr<ITrackPositionStorage> TrackPositionStoragePtr;
typedef _smart_ptr<ITrackRotationStorage> TrackRotationStoragePtr;

// specialization
template<class _Type, class _Storage, class _Base>
class CTrackDataStorage
    : public _Base
{
public:
    virtual void Resize(uint32 count)
    {
        m_arrKeys.resize(count);
    }

    virtual void Reserve(uint32 count)
    {
        m_arrKeys.reserve(count);
    }

    virtual void AddValue(_Type& val)
    {
        _Storage v;
        v.ToInternalType(val);
        m_arrKeys.push_back(v);
    }


    char* GetData()
    {
        return (char*) m_arrKeys.begin();
    }

    virtual uint32 GetNumKeys() const
    {
        return m_arrKeys.size();
    }

    virtual uint32 GetDataCount() const
    {
        return m_arrKeys.size();
    }


    uint32 GetDataRawSize() const
    {
        return (uint32)((char*)m_arrKeys.end() - (char*)m_arrKeys.begin());
    }

    void GetValue(uint32 key, _Type& val) const
    {
        val =  m_arrKeys[key].ToExternalType();
    }

    virtual void SetValueForKey(uint32 key, _Type& val)
    {
        uint32 numKeys = m_arrKeys.size();

        if (key >= numKeys)
        {
            return;
        }
        _Storage v;
        v.ToInternalType(val);

        m_arrKeys[key] = v;
    }

    virtual uint32 GetFormat() const
    {
        return _Storage::GetFormat();
    }

    virtual void SwapBytes()
    {
        for (uint32 i = 0; i < m_arrKeys.size(); ++i)
        {
            m_arrKeys[i].SwapBytes();
        }
    }

    DynArray<_Storage> m_arrKeys;
};


typedef CTrackDataStorage<Vec3, NoCompressVec3, ITrackPositionStorage> NoCompressPosition;
typedef _smart_ptr<NoCompressPosition> NoCompressPositionPtr;

typedef CTrackDataStorage<Quat, NoCompressQuat, ITrackRotationStorage> NoCompressRotation;
typedef _smart_ptr<NoCompressRotation> NoCompressRotationPtr;

typedef CTrackDataStorage<Quat, SmallTree48BitQuat, ITrackRotationStorage> SmallTree48BitQuatRotation;
typedef _smart_ptr<SmallTree48BitQuatRotation> SmallTree48BitQuatRotationPtr;

typedef CTrackDataStorage<Quat, SmallTree64BitQuat, ITrackRotationStorage> SmallTree64BitQuatRotation;
typedef _smart_ptr<SmallTree64BitQuatRotation> SmallTree64BitQuatRotationPtr;

typedef CTrackDataStorage<Quat, SmallTree64BitExtQuat, ITrackRotationStorage> SmallTree64BitExtQuatRotation;
typedef _smart_ptr<SmallTree64BitExtQuatRotation> SmallTree64BitExtQuatRotationPtr;


class ITrackInformation
    : public _reference_target_t
{
public:
    uint32 GetNumKeys() const
    {
        return m_pKeyTimes->GetNumKeys();
    }

    f32 GetTimeFromKey(uint32 key) const
    {
        return m_pKeyTimes->GetKeyValueFloat(key);
    }

    void SetKeyTimesInformation(KeyTimesInformationPtr& ptr)
    {
        m_pKeyTimes = ptr;
    }

    KeyTimesInformationPtr& GetKeyTimesInformation()
    {
        return m_pKeyTimes;
    }
protected:
    KeyTimesInformationPtr m_pKeyTimes;
};
//common interface for positions
class IPositionController
    : public ITrackInformation
{
public:
    virtual ~IPositionController() {};

    virtual void DecodeKey(f32 normalized_time, Vec3& quat) = 0;
    virtual size_t SizeOfThis() const
    {
        return sizeof(this) + m_pData->GetDataRawSize();
    }
    virtual void GetValueFromKey(uint32 key, Vec3& val) const
    {
        uint32 numKeys = GetNumKeys();

        if (key >= numKeys)
        {
            key = numKeys - 1;
        }

        m_pData->GetValue(key, val);
    }

    virtual void SetPositionStorage(TrackPositionStoragePtr& ptr)
    {
        m_pData = ptr;
    }

    virtual TrackPositionStoragePtr& GetPositionStorage()
    {
        return m_pData;
    }

    virtual uint32 GetFormat() const
    {
        return m_pData->GetFormat();
    }

    virtual uint32 GetDataCount() const
    {
        return m_pData->GetNumKeys();
    }

    virtual void SwapBytes()
    {
        m_pData->SwapBytes();
    }

protected:
    TrackPositionStoragePtr m_pData;
};


// Common interface for rotation information
class IRotationController
    : public ITrackInformation
{
public:

    virtual ~IRotationController() {};

    virtual void DecodeKey(f32 normalized_time, Quat& quat) = 0;
    virtual size_t SizeOfThis() const
    {
        return sizeof(this) + m_pData->GetDataRawSize();
    }

    virtual void GetValueFromKey(uint32 key, Quat& val) const
    {
        uint32 numKeys = GetNumKeys();

        if (key >= numKeys)
        {
            key = numKeys - 1;
        }

        m_pData->GetValue(key, val);
    }

    virtual void SetRotationStorage(TrackRotationStoragePtr& ptr)
    {
        m_pData = ptr;
    }
    virtual TrackRotationStoragePtr& GetRotationStorage()
    {
        return m_pData;
    }

    virtual uint32 GetFormat() const
    {
        return m_pData->GetFormat();
    }

    virtual uint32 GetDataCount() const
    {
        return m_pData->GetNumKeys();
    }

    virtual void SwapBytes()
    {
        m_pData->SwapBytes();
    }


protected:
    TrackRotationStoragePtr m_pData;
};

typedef _smart_ptr<IRotationController> RotationControllerPtr;
typedef _smart_ptr<IPositionController> PositionControllerPtr;

struct Vec3Lerp
{
    static inline void Blend(Vec3& res, const Vec3& val1, const Vec3& val2, f32 t)
    {
        res.SetLerp(val1, val2, t);
    }
};

struct QuatLerp
{
    static inline void Blend(Quat& res, const Quat& val1, const Quat& val2, f32 t)
    {
        res.SetNlerp(val1, val2, t);
    }
};


template<class _Type, class _Interpolator, class _Base>
class CAdvancedTrackInformation
    : public _Base
{
public:
    CAdvancedTrackInformation()
    {
    }


    virtual void DecodeKey(f32 normalized_time, _Type& pos)
    {
        //DEFINE_PROFILER_SECTION("ControllerPQ::GetValue");

        f32 t;
        uint32 key = this->m_pKeyTimes->GetKey(normalized_time, t);

        if (key == 0)
        {
            this->GetValueFromKey(0, pos);
        }
        else
        if (key >= this->GetNumKeys())
        {
            this->GetValueFromKey(key - 1, pos);
        }
        else
        {
            _Type p1, p2;
            this->GetValueFromKey(key - 1, p1);
            this->GetValueFromKey(key, p2);

            _Interpolator::Blend(pos, p1, p2, t);
        }
    }
};

typedef CAdvancedTrackInformation<Quat, QuatLerp, IRotationController> RotationTrackInformation;
typedef _smart_ptr<RotationTrackInformation> RotationTrackInformationPtr;

typedef CAdvancedTrackInformation<Vec3, Vec3Lerp, IPositionController> PositionTrackInformation;
typedef _smart_ptr<PositionTrackInformation> PositionTrackInformationPtr;

// My implementation of controllers
class CController
    : public IController
{
public:
    //Creation interface

    virtual const CController* GetCController() const { return this; };

    uint32 numKeys() const
    {
        // now its hack, because num keys might be different
        if (m_pRotationController)
        {
            return m_pRotationController->GetNumKeys();
        }

        if (m_pPositionController)
        {
            return m_pPositionController->GetNumKeys();
        }

        return 0;
        //return m_arrKeys.size();
    }

    uint32 GetID () const {return m_nControllerId; }

    Status4 GetOPS (f32 normalized_time, Quat& quat, Vec3& pos, Diag33& scale)
    {
        Status4 res;
        res.o = GetO(normalized_time, quat) ? 1 : 0;
        res.p = GetP(normalized_time, pos) ? 1 : 0;
        res.s = GetS(normalized_time, scale) ? 1 : 0;

        return res;
    }

    Status4 GetOP (f32 normalized_time, Quat& quat, Vec3& pos)
    {
        Status4 res;
        res.o = GetO(normalized_time, quat) ? 1 : 0;
        res.p = GetP(normalized_time, pos) ? 1 : 0;

        return res;
    }

    uint32 GetO (f32 normalized_time, Quat& quat)
    {
        if (m_pRotationController)
        {
            m_pRotationController->DecodeKey(normalized_time, quat);
            return STATUS_O;
        }

        return 0;
    }

    uint32 GetP (f32 normalized_time, Vec3& pos)
    {
        if (m_pPositionController)
        {
            m_pPositionController->DecodeKey(normalized_time, pos);
            return STATUS_P;
        }

        return 0;
    }

    uint32 GetS (f32 normalized_time, Diag33& pos)
    {
        if (m_pScaleController)
        {
            Vec3 scale;
            m_pScaleController->DecodeKey(normalized_time, scale);

            pos.x = scale.x;
            pos.y = scale.y;
            pos.z = scale.z;

            return STATUS_S;
        }
        return 0;
    }


    int32 GetO_numKey()
    {
        if (m_pRotationController)
        {
            return m_pRotationController->GetNumKeys();
        }
        return -1;
    }
    int32 GetP_numKey()
    {
        if (m_pPositionController)
        {
            return m_pPositionController->GetNumKeys();
        }
        return -1;
    }

    // returns the start time


    size_t SizeOfThis () const
    {
        size_t res(sizeof(CController));

        if (m_pPositionController)
        {
            res += m_pPositionController->SizeOfThis();
        }

        if (m_pScaleController)
        {
            res += m_pScaleController->SizeOfThis();
        }

        if (m_pRotationController)
        {
            res += m_pRotationController->SizeOfThis();
        }

        return res;
    }



    //FIXME:!!!
    CInfo GetControllerInfoRoot()
    {
        CInfo info;

        info.numKeys = numKeys();
        if (m_pRotationController)
        {
            m_pRotationController->GetValueFromKey(info.numKeys - 1, info.quat);
            info.etime   = (int)(m_pRotationController->GetTimeFromKey(info.numKeys - 1) * TICKS_CONVERT);
        }

        if (m_pPositionController)
        {
            m_pPositionController->GetValueFromKey(m_pPositionController->GetNumKeys() - 1, info.pos);
            info.etime   = (int)(m_pPositionController->GetTimeFromKey(info.numKeys - 1) * TICKS_CONVERT);
        }



        return info;
    }

    CInfo GetControllerInfo() const
    {
        CInfo info;
        info.etime = 0;
        info.stime = 0;
        info.numKeys = 0;

        if (m_pRotationController)
        {
            info.numKeys = m_pRotationController->GetNumKeys();
            m_pRotationController->GetValueFromKey(info.numKeys - 1, info.quat);
            info.etime   = (int)m_pRotationController->GetTimeFromKey(info.numKeys - 1);
            info.stime   = (int)m_pRotationController->GetTimeFromKey(0);
        }

        if (m_pPositionController)
        {
            info.numKeys = m_pPositionController->GetNumKeys();
            m_pPositionController->GetValueFromKey(info.numKeys - 1, info.pos);
            info.etime   = (int)m_pPositionController->GetTimeFromKey(info.numKeys - 1);
            info.stime   = (int)m_pPositionController->GetTimeFromKey(0);
        }


        info.realkeys = (info.etime - info.stime) /*/0xa0*/ + 1;

        return info;
    }


    void SetRotationController(RotationControllerPtr& ptr)
    {
        m_pRotationController = ptr;
    }

    void SetPositionController(PositionControllerPtr& ptr)
    {
        m_pPositionController = ptr;
    }

    void SetScaleController(PositionControllerPtr& ptr)
    {
        m_pScaleController = ptr;
    }

    const RotationControllerPtr& GetRotationController() const
    {
        return m_pRotationController;
    }

    RotationControllerPtr& GetRotationController()
    {
        return m_pRotationController;
    }

    const PositionControllerPtr& GetPositionController() const
    {
        return m_pPositionController;
    }

    PositionControllerPtr& GetPositionController()
    {
        return m_pPositionController;
    }

    PositionControllerPtr& GetScaleController()
    {
        return m_pScaleController;
    }

    //virtual EControllerInfo GetControllerType()
    //{
    //  return eCController;
    //}

    uint32 m_nControllerId;
    uint32 m_nPositionKeyTimesTrackId;
    uint32 m_nRotationKeyTimesTrackId;
    uint32 m_nPositionTrackId;
    uint32 m_nRotationTrackId;

    bool m_bShared;
    //members
protected:
    RotationControllerPtr m_pRotationController;
    PositionControllerPtr m_pPositionController;
    PositionControllerPtr m_pScaleController;
};

TYPEDEF_AUTOPTR(CController);


template <class TKeyTime, class _Encoder>
class CKeyTimesInformationStartStop
    : public IKeyTimesInformation
{
public:
    CKeyTimesInformationStartStop() {m_arrKeys[0] = (TKeyTime)0; m_arrKeys[1] = (TKeyTime)0; };

    // return key value in f32 format
    f32 GetKeyValueFloat(uint32 key) const { return /*_Encoder::DecodeKeyValue*/ (float)(m_arrKeys[0] + key); }

    // return key number for normalized_time
    uint32  GetKey(f32 realtime, f32& difference_time)
    {
        if (realtime < m_arrKeys[0])
        {
            return 0;
        }

        if (realtime > m_arrKeys[1])
        {
            return (uint32)(m_arrKeys[1] - m_arrKeys[0]);
        }

        uint32 nKey = (uint32)realtime;
        difference_time = realtime - (float)(nKey);

        return nKey;
    }

    uint32 GetNumKeys() const {return (uint32)(m_arrKeys[1] - m_arrKeys[0] + 1); };
    virtual uint32 GetDataCount() const {return 2; };

    virtual void Clear()
    {
        //m_arrKeys.clear();
    }

    void AddKeyTime(f32 val)
    {
        //m_arrKeys.push_back(/*_Encoder::EncodeKeyValue*/(TKeyTime)(val));
        if (m_arrKeys[0] > val)
        {
            m_arrKeys[0] = (TKeyTime)val;
        }

        if (m_arrKeys[1] < val)
        {
            m_arrKeys[1] = (TKeyTime)val;
        }
    };

    void ReserveKeyTime(uint32 count)
    {
        //m_arrKeys.reserve(count);
    };

    void ResizeKeyTime(uint32 count)
    {
        //      m_arrKeys.resize(count);
    };

    uint32 GetFormat()
    {
        return _Encoder::GetFormat();
    }

    char* GetData()
    {
        return (char*)(&m_arrKeys[0]);
        //return (char*) m_arrKeys.begin();
    }

    uint32 GetDataRawSize()
    {
        return sizeof(TKeyTime) * 2;
        //;/return (uint32)((char*)m_arrKeys.end() - (char*)m_arrKeys.begin());
    }

    //  SwapBytes
private:
    //DynArray<TKeyTime> m_arrKeys;
    TKeyTime m_arrKeys[2]; // start pos, stop pos.
};

typedef CKeyTimesInformationStartStop<f32, F32StartStopEncoder> F32SSKeyTimesInformation;
typedef CKeyTimesInformationStartStop<uint16, UINT16StartStopEncoder> UINT16SSKeyTimesInformation;
typedef CKeyTimesInformationStartStop<uint8, ByteStartStopEncoder> ByteSSKeyTimesInformation;

typedef CKeyTimesInformationStartStop<f32, F32StartStopEncoder> F32SSKeyTimesInformationInplace;
typedef CKeyTimesInformationStartStop<uint16, UINT16StartStopEncoder> UINT16SSKeyTimesInformationInplace;
typedef CKeyTimesInformationStartStop<uint8, ByteStartStopEncoder> ByteSSKeyTimesInformationInplace;




#ifdef DO_ASM

inline uint16 GetFirstLowBit(uint16 _word)
{
    uint16 res = 16;

    __asm
    {
        mov ax, res
        mov bx, _word
        bsf ax, bx
        mov res, ax
    }

    return res;
}

inline uint16 GetFirstHighBit(uint16 _word)
{
    uint16 res = 16;

    __asm
    {
        mov ax, res
        mov bx, _word
        bsr ax, bx
        mov res, ax
    }
    return res;
}


#else

#ifdef WIN32
#include <intrin.h>
#pragma intrinsic(_BitScanForward)
#pragma intrinsic(_BitScanReverse)

inline uint16 GetFirstLowBit(uint16 word)
{
    unsigned long lword(word);
    unsigned long index;
    if (_BitScanForward(&index, lword))
    {
        return (uint16)(index);
    }
    
    return 16;
}

inline uint16 GetFirstHighBit(uint16 word)
{
    unsigned long lword(word);
    unsigned long index;
    if (_BitScanReverse(&index, lword))
    {
        return (uint16)(index);
    }
    
    return 16;
}

#else


inline uint16 GetFirstLowBit(uint16 word)
{
    uint16 firstBitIndex = (ControllerHelper::m_byteLowBit[word & 0xFF]);
    if (firstBitIndex == 8)
    {
        firstBitIndex = (ControllerHelper::m_byteLowBit[(word >> 8) & 0xFF]) + 8;
    }
    
    return firstBitIndex;
}

inline uint16 GetFirstHighBit(uint16 word)
{
    uint16 firstBitIndex = ControllerHelper::m_byteHighBit[(word >> 8) & 0xFF] + 8;
    if (firstBitIndex == 24)
    {
        firstBitIndex = (ControllerHelper::m_byteHighBit[word & 0xFF]);
    }
    
    return firstBitIndex;
}

#endif
#endif


///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///template <class TKeyTime, class _Encoder>
class CKeyTimesInformationBitSet
    : public IKeyTimesInformation
{
private:
    static uint8 m_bTable[256];


public:
    CKeyTimesInformationBitSet()
        : m_lastTime(-1)                        /* : m_lastTime(-1), m_arrKeys(0), m_Start(0),  m_Size(0)*/
    {
    };

    virtual void  SetKeyValueFloat(uint32 key, float time) {}
    //// return key value
    //TKeyTime GetKeyValue(uint32 key) const { return m_arrKeys[key]; };

    // return key value in f32 format
    f32 GetKeyValueFloat(uint32 key) const
    {
        if (key == 0)
        {
            // first one

            return (float)GetHeader()->m_Start;
        }
        else
        if (key == GetNumKeys() - 1)
        {
            // last one
            return (float)GetHeader()->m_End;
        }
        // worse situation
        int c(0);

        int keys = GetNumKeys();
        int count(0);
        for (int i = 0; i < keys; ++i)
        {
            uint16 bits = GetKeyData(i);

            for (int j = 0; j < 16; ++j)
            {
                if ((bits >> j) & 1)
                {
                    ++count;
                    if ((count - 1) == key)
                    {
                        return (float)(i * 16 + j) + GetHeader()->m_Start;
                    }
                }
            }
        }

        return 0;
    }

    // return key number for normalized_time
    uint32  GetKey(f32 realtime, f32& difference_time)
    {
        if (realtime == m_lastTime)
        {
            difference_time =  m_LastDifferenceTime;
            return m_lastKey;
        }
        m_lastTime = realtime;
        uint32 numKey = m_arrKeys.size();

        f32 keytime_start = (float)GetHeader()->m_Start;
        f32 keytime_end = (float)GetHeader()->m_End;
        f32 test_end = keytime_end;

        if (realtime < keytime_start)
        {
            test_end += realtime;
        }

        if (realtime < keytime_start)
        {
            m_lastKey = 0;
            return 0;
        }

        if (realtime >= keytime_end)
        {
            m_lastKey = numKey;
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
        m_LastDifferenceTime = difference_time = (f32)(internalTime - (f32)nearestLeft) / ((f32)nearestRight - (f32)nearestLeft);

        // count nPos
        uint32 nPos(0);
        for (uint16 i = 0; i < rigthPiece; ++i)
        {
            uint16 data = GetKeyData(i);
            nPos += m_bTable[data & 255] + m_bTable[data >> 8];
        }

        data = GetKeyData(rigthPiece);
        data = ((data <<  (15 - wBit)) & 0xFFFF) >> (15 - wBit);
        nPos += m_bTable[data & 255] + m_bTable[data >> 8];

        m_lastKey = nPos - 1;

        return m_lastKey;
    }

    uint32 GetNumKeys() const {return GetHeader()->m_Size; };
    virtual uint32 GetDataCount() const {return m_arrKeys.size(); };


    void AddKeyTime(f32 val)
    {
        //  m_arrKeys.push_back(/*_Encoder::EncodeKeyValue*/(TKeyTime)(val));
    };

    void ReserveKeyTime(uint32 count)
    {
        m_arrKeys.reserve(count);
    };

    void ResizeKeyTime(uint32 count)
    {
        m_arrKeys.resize(count);
    };

    uint32 GetFormat()
    {
        return eBitset;
    }

    char* GetData()
    {
        return (char*)m_arrKeys.begin();
    }

    uint32 GetDataRawSize()
    {
        return (uint32)((char*)m_arrKeys.end() - (char*)m_arrKeys.begin());
    }

    virtual void Clear()
    {
    }

    virtual void SwapBytes()
    {
        SwapEndianness(&m_arrKeys[0], m_arrKeys.size());
    }

protected:

    struct Header
    {
        uint16      m_Start;
        uint16    m_End;
        uint16      m_Size;
    };

    inline Header* GetHeader() const
    {
        return (Header*)(&m_arrKeys[0]);
    };

    inline uint16 GetKeyData(int i) const
    {
        return m_arrKeys[3 + i];
    };

    DynArray<uint16> m_arrKeys;

    f32 m_lastTime;
    f32 m_LastDifferenceTime;
    uint32 m_lastKey;
};


#endif // CRYINCLUDE_TOOLS_RC_RESOURCECOMPILERPC_CGA_CONTROLLERPQ_H

