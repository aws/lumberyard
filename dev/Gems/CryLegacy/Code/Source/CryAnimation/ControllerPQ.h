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


#ifndef CRYINCLUDE_CRYANIMATION_CONTROLLERPQ_H
#define CRYINCLUDE_CRYANIMATION_CONTROLLERPQ_H
#pragma once

#include "CGFContent.h"
#include "QuatQuantization.h"
#include "ControllerDefragHeap.h"


typedef Quat TRotation;
typedef Vec3 TPosition;
typedef Vec3 TScale;




class IPositionController;
class IRotationController;
class IKeyTimesInformation;
class ITrackPositionStorage;
class ITrackRotationStorage;
class IControllerOpt;

namespace ControllerHelper
{
    uint32 GetPositionsFormatSizeOf(uint32 format);
    uint32 GetRotationFormatSizeOf(uint32 format);
    uint32 GetKeyTimesFormatSizeOf(uint32 format);

    ITrackPositionStorage* GetPositionControllerPtr(uint32 format, void* pData = NULL, size_t numElements = 0);
    ITrackRotationStorage* GetRotationControllerPtr(uint32 format, void* pData = NULL, size_t numElements = 0);
    IKeyTimesInformation* GetKeyTimesControllerPtr(uint32 format, void* pData = NULL, size_t numElements = 0);

    // Create array of controllers
    ITrackPositionStorage* GetPositionControllerPtrArray(uint32 format, uint32 size);
    ITrackRotationStorage* GetRotationControllerPtrArray(uint32 format, uint32 size);
    IKeyTimesInformation* GetKeyTimesControllerPtrArray(uint32 format, uint32 size);

    ITrackRotationStorage* GetRotationPtrFromArray(void* ptr, uint32 format, uint32 num);
    IKeyTimesInformation* GetKeyTimesPtrFromArray(void* ptr, uint32 format, uint32 num);
    ITrackPositionStorage* GetPositionPtrFromArray(void* ptr, uint32 format, uint32 num);
    // delete array of controllers
    void DeletePositionControllerPtrArray(ITrackPositionStorage*);
    void DeleteRotationControllerPtrArray(ITrackRotationStorage*);
    void DeleteTimesControllerPtrArray(IKeyTimesInformation*);

    extern const uint8 m_byteTable[256];
    extern const uint8 m_byteLowBit[256];
    extern const uint8 m_byteHighBit[256];
}

extern CControllerDefragHeap g_controllerHeap;

// don't add values which incrase the maximum enum value over 255 because this will break IControllerOptNonVirtual!!!!
enum EKeyTimesFormat
{
    eF32,
    eUINT16,
    eByte,
    eF32StartStop,
    eUINT16StartStop,
    eByteStartStop,
    eBitset,
    eNoFormat = 255

        //eF32Bitset,
        //eUINT16Bitset,
        //eByteBitset,
};

class IControllerRelocatableChain
{
public:
    virtual void Relocate(char* pDst, char* pSrc) = 0;

public:
    IControllerRelocatableChain()
        : m_pNext(NULL)
    {
    }

    void LinkTo(IControllerRelocatableChain* pNext) { m_pNext = pNext; }
    IControllerRelocatableChain* GetNext() const { return m_pNext; }

protected:
    virtual ~IControllerRelocatableChain() {}

private:
    IControllerRelocatableChain* m_pNext;
};

struct F32Encoder
{
    typedef f32 TKeyTypeTraits;
    typedef uint32 TKeyTypeCountTraits;

    static EKeyTimesFormat GetFormat() { return eF32; };
};

struct UINT16Encoder
{
    typedef uint16 TKeyTypeTraits;
    typedef uint16 TKeyTypeCountTraits;

    static EKeyTimesFormat GetFormat() { return eUINT16; };
};

struct ByteEncoder
{
    typedef uint8 TKeyTypeTraits;
    typedef uint16 TKeyTypeCountTraits;

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
    typedef uint16 TKeyTypeTraits;
    typedef uint16 TKeyTypeCountTraits;

    static EKeyTimesFormat GetFormat() { return eBitset; };
};

class IKeyTimesInformation
    : public _reference_target_t
{
public:
    virtual ~IKeyTimesInformation() {; };
    //  virtual TKeyTime GetKeyValue(uint32 key) const = 0;
    // return key value in f32 format
    virtual f32 GetKeyValueFloat(uint32 key) const  = 0;
    // return key number for normalized_time
    virtual uint32  GetKey(f32 normalized_time, f32& difference_time) = 0;

    virtual uint32 GetNumKeys() const = 0;

    virtual uint32 AssignKeyTime(const char* pData, uint32 numElements) = 0;

    virtual uint32 GetFormat() = 0;

    virtual char* GetData() = 0;

    virtual uint32 GetDataRawSize() = 0;

    virtual IControllerRelocatableChain* GetRelocateChain() = 0;

    //virtual void AddRef() = 0;

    //virtual void Release() = 0;

    virtual void GetMemoryUsage(ICrySizer* pSizer) const = 0;
};

template <class TKeyTime, class _Encoder>
class CKeyTimesInformation
    : public IKeyTimesInformation
    , public IControllerRelocatableChain
{
public:
    typedef TKeyTime KeyTimeType;

public:
    CKeyTimesInformation()
        : m_pKeys(NULL)
        , m_numKeys(0)
        , m_lastTime(-1)
    {
    }

    CKeyTimesInformation(TKeyTime* pKeys, uint32 numKeys)
        : m_pKeys(pKeys)
        , m_numKeys(numKeys | (numKeys > 0 ? kKeysConstantMask : 0))
        , m_lastTime(-1)
    {
    }

    ~CKeyTimesInformation()
    {
        if (!IsConstant() && m_hHdl.IsValid())
        {
            g_controllerHeap.Free(m_hHdl);
        }
    }

    // return key value
    TKeyTime GetKeyValue(uint32 key) const { return m_pKeys[key]; };

    // return key value in f32 format
    f32 GetKeyValueFloat(uint32 key) const { return (m_pKeys[key]); }

    // return key number for normalized_time
    uint32 GetKey(f32 normalized_time, f32& difference_time)
    {
        TKeyTime* pKeys = m_pKeys;
        uint32 numKey = GetNumKeys_Int();

        f32 realtime = normalized_time;

        /*
        if (realtime == m_lastTime)
        {
            difference_time =  m_LastDifferenceTime;
            return m_lastKey;
        }*/

        m_lastTime = realtime;


        TKeyTime keytime_start = pKeys[0];
        TKeyTime keytime_end = pKeys[numKey - 1];

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
        while (nStep)
        {
            if (realtime < pKeys[nPos])
            {
                nPos = nPos - nStep;
            }
            else
            if (realtime > pKeys[nPos])
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
        while (realtime > pKeys[nPos])
        {
            nPos++;
        }

        while (realtime < pKeys[nPos - 1])
        {
            nPos--;
        }

        m_lastKey = nPos;

        // possible error if encoder uses nonlinear methods!!!
        m_LastDifferenceTime = difference_time = /*_Encoder::DecodeKeyValue*/ (f32)(realtime - (f32)pKeys[nPos - 1]) / ((f32)pKeys[nPos] - (f32)pKeys[nPos - 1]);

        return nPos;
    }

    uint32 GetNumKeys() const {return GetNumKeys_Int(); };

    uint32 AssignKeyTime(const char* pData, uint32 numElements)
    {
#ifndef _RELEASE
        if (IsConstant())
        {
            __debugbreak();
            return 0;
        }
#endif

        if (m_hHdl.IsValid())
        {
            g_controllerHeap.Free(m_hHdl);
        }

        uint32 sz = numElements * sizeof(TKeyTime);

        this->LinkTo(NULL);
        CControllerDefragHdl hdl = g_controllerHeap.AllocPinned(sz, this);
        m_pKeys = (TKeyTime*)g_controllerHeap.WeakPin(hdl);
        m_hHdl = hdl;
        m_numKeys = numElements;

        memcpy(m_pKeys, pData, sz);
        g_controllerHeap.Unpin(hdl);

        return sz;
    }

    uint32 GetFormat()
    {
        return _Encoder::GetFormat();
    }

    char* GetData()
    {
        return (char*) m_pKeys;
    }

    uint32 GetDataRawSize()
    {
        return GetNumKeys_Int() * sizeof(TKeyTime);
    }

    IControllerRelocatableChain* GetRelocateChain()
    {
        return this;
    }

    void Relocate(char* pDst, char* pSrc)
    {
        ptrdiff_t offs = reinterpret_cast<char*>(m_pKeys) - pSrc;
        m_pKeys = reinterpret_cast<TKeyTime*>(pDst + offs);
    }

    virtual void GetMemoryUsage(ICrySizer* pSizer) const
    {
        pSizer->AddObject(this, sizeof(*this));
    }

private:
    enum
    {
        kKeysSizeMask = 0x7fffffff,
        kKeysConstantMask = 0x80000000,
    };

private:
    // Prevent copy construction
    CKeyTimesInformation(const CKeyTimesInformation<TKeyTime, _Encoder>&);
    CKeyTimesInformation<TKeyTime, _Encoder>& operator = (const CKeyTimesInformation<TKeyTime, _Encoder>&);

private:
    ILINE uint32 IsConstant() const { return m_numKeys & kKeysConstantMask; }
    ILINE uint32 GetNumKeys_Int() const { return m_numKeys & kKeysSizeMask; }

private:
    TKeyTime* m_pKeys;
    CControllerDefragHdl m_hHdl;
    uint32 m_numKeys;
    f32 m_lastTime;
    f32 m_LastDifferenceTime;
    uint32 m_lastKey;
};

template <class TKeyTime, class _Encoder>
class CKeyTimesDeleteable
    : public CKeyTimesInformation<TKeyTime, _Encoder>
{
public:
    CKeyTimesDeleteable()
        : m_nRefCounter(0) {}

    virtual void AddRef()
    {
        ++m_nRefCounter;
    }

    virtual void Release()
    {
        if (--m_nRefCounter <= 0)
        {
            delete this;
        }
    }
protected:
    uint32 m_nRefCounter;
};

template <class TKeyTime, class _Encoder>
class CKeyTimesDeleteableInplace
    : public CKeyTimesInformation<TKeyTime, _Encoder>
{
public:
    virtual void Release()
    {
        //if (--m_nRefCounter <= 0)
        //  this->~CKeyTimesDeleteableInplace();
    }

    virtual void AddRef()
    {
    }
};


//typedef _smart_ptr<IKeyTimesInformation>/* IKeyTimesInformation * */KeyTimesInformationPtr;

//typedef CKeyTimesDeleteable<f32, F32Encoder> F32KeyTimesInformation;
//typedef CKeyTimesDeleteable<uint16, UINT16Encoder> UINT16KeyTimesInformation;
//typedef CKeyTimesDeleteable<uint8, ByteEncoder> ByteKeyTimesInformation;
//
//typedef CKeyTimesDeleteableInplace<f32, F32Encoder> F32KeyTimesInformationInplace;
//typedef CKeyTimesDeleteableInplace<uint16, UINT16Encoder> UINT16KeyTimesInformationInplace;
//typedef CKeyTimesDeleteableInplace<uint8, ByteEncoder> ByteKeyTimesInformationInplace;


typedef CKeyTimesInformation<f32, F32Encoder> F32KeyTimesInformation;
typedef CKeyTimesInformation<uint16, UINT16Encoder> UINT16KeyTimesInformation;
typedef CKeyTimesInformation<uint8, ByteEncoder> ByteKeyTimesInformation;

typedef CKeyTimesInformation<f32, F32Encoder> F32KeyTimesInformationInplace;
typedef CKeyTimesInformation<uint16, UINT16Encoder> UINT16KeyTimesInformationInplace;
typedef CKeyTimesInformation<uint8, ByteEncoder> ByteKeyTimesInformationInplace;


template <class TKeyTime, class _Encoder>
class CKeyTimesInformationStartStop
    : public IKeyTimesInformation
{
public:
    CKeyTimesInformationStartStop() {m_arrKeys[0] = (TKeyTime)0; m_arrKeys[1] = (TKeyTime)0; };
    CKeyTimesInformationStartStop(const TKeyTime* pKeys, uint32 numKeys)
    {
        CRY_ASSERT(numKeys >= 2);
        m_arrKeys[0] = pKeys[0];
        m_arrKeys[1] = pKeys[1];
    }

    // return key value in f32 format
    f32 GetKeyValueFloat(uint32 key) const
    {
        return static_cast<f32>(m_arrKeys[0] + key);
    }

    // return key number for normalized_time
    uint32  GetKey(f32 normalized_time, f32& difference_time)
    {
        f32 realtime = normalized_time;

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

    uint32 GetNumKeys() const {return (uint32)(m_arrKeys[1] - m_arrKeys[0]); };

    uint32 AssignKeyTime(const char* pData, uint32 numElements)
    {
        memcpy(m_arrKeys, pData, sizeof(TKeyTime) * 2);
        return sizeof(TKeyTime) * 2;
    }

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

    IControllerRelocatableChain* GetRelocateChain()
    {
        return NULL;
    }

    void GetMemoryUsage(ICrySizer* pSizer) const
    {
        pSizer->AddObject(this, sizeof(*this));
    }
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



inline uint8 GetFirstLowBitInternal(uint8 word)
{
    uint8 c(0);

    if (word & 1)
    {
        return 0;
    }

    uint8 b;
    do
    {
        word = word >> 1;
        b = word & 1;
        ++c;
    } while (!b && c < 8);

    return c;
}

inline uint8 GetFirstHighBitInternal(uint8 word)
{
    uint8 c(0);

    if (word & 0x80)
    {
        return 7;
    }

    uint8 b;
    do
    {
        word = word << 1;
        b = word & 0x80;
        ++c;
    } while (!b && c < 8);

    if (c == 8)
    {
        return c;
    }
    else
    {
        return 7 - c;
    }
}


//#ifdef NEED_ENDIAN_SWAP
//inline uint16 GetFirstLowBit(uint16 word)
//{
//
//  uint16 res = (ControllerHelper::m_byteLowBit[word && 8]);
//  if ( res == 8) {
//      res = (ControllerHelper::m_byteLowBit[(word >> 8) && 0xFF] + 8);
//  }
//
//  return res;
//}
//
//inline uint16 GetFirstHighBit(uint16 word)
//{
//  uint16 res = (ControllerHelper::m_byteHighBit[word >> 8] + 8) % 16;
//  if ( res == 16) {
//      res = ControllerHelper::m_byteHighBit[word && 0xFF];
//  }
//
//  return res;
//}
//#else

//#endif


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


inline uint16 GetFirstLowBitTest(uint16 word)
{
    uint16 c(0);

    if (word & 1)
    {
        return 0;
    }

    uint16 b;
    do
    {
        word = word >> 1;
        b = word & 1;
        ++c;
    } while (!b && c < 16);

    return c;
}

inline uint16 GetFirstHighBitTest(uint16 word)
{
    uint16 c(0);

    if (word & 0x8000)
    {
        return 15;
    }

    uint16 b;
    do
    {
        word = word << 1;
        b = word & 0x8000;
        ++c;
    } while (!b && c < 16);

    if (c == 16)
    {
        return c;
    }
    else
    {
        return 15 - c;
    }
}


inline uint16 GetFirstLowBit(uint16 word)
{
    uint8 rr = word & 0xFF;
    uint16 res = ControllerHelper::m_byteLowBit[rr];
    if (res == 8)
    {
        res = (ControllerHelper::m_byteLowBit[(word >> 8) & 0xFF]) + 8;
    }

    //if (res != GetFirstLowBitTest(word))
    //  int a = 0;
    return res;
}

inline uint16 GetFirstHighBit(uint16 word)
{
    uint16 res = ControllerHelper::m_byteHighBit[(word >> 8) & 0xFF] + 8;
    if (res == 24)
    {
        res = (ControllerHelper::m_byteHighBit[word & 0xFF]);
    }
    //if (res != GetFirstHighBitTest(word))
    //  int a = 0;

    return res;
}

#endif
#endif


///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///template <class TKeyTime, class _Encoder>


class CKeyTimesInformationBitSet
    : public IKeyTimesInformation
{
public:
    CKeyTimesInformationBitSet()
        : m_pKeys(NULL)
        , m_numKeys(0)
        , m_lastTime(-1)
    {
    }

    CKeyTimesInformationBitSet(uint16* pKeys, uint32 numKeys)
        : m_pKeys(pKeys)
        , m_numKeys((numKeys > 0) ? (numKeys | kKeysConstantMask) : 0)
        , m_lastTime(-1)
    {
    }

    ~CKeyTimesInformationBitSet()
    {
        if (m_pKeys && !IsConstant())
        {
            CryModuleFree(m_pKeys);
        }
    }

    // return key value in f32 format
    f32 GetKeyValueFloat(uint32 key) const
    {
        if (key == 0)
        {
            // first one

            return (float)GetHeader()->m_Start;
        }
        else
        if (key >= GetNumKeys() - 1)
        {
            // last one
            return (float)GetHeader()->m_End;
        }
        // worse situation
        int keys = GetNumKeys();
        int count(0);
        for (int i = 0; i < keys; ++i)
        {
            uint16 bits = GetKeyData(i);

            for (int j = 0; i < 16; ++i)
            {
                if ((bits >> j) & 1)
                {
                    ++count;
                    if ((count - 1) == key)
                    {
                        return (float)(i * 16 + j);
                    }
                }
            }
        }

        return 0;
    }

    // return key number for normalized_time
    uint32  GetKey(f32 normalized_time, f32& difference_time)
    {
        f32 realtime = normalized_time;

        if (realtime == m_lastTime)
        {
            difference_time =  m_LastDifferenceTime;
            return m_lastKey;
        }
        m_lastTime = realtime;
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
            m_lastKey = 0;
            return 0;
        }

        if (realtime >= keytime_end)
        {
            difference_time = 0;
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
            uint16 data2 = GetKeyData(i);
            nPos += ControllerHelper::m_byteTable[data2 & 255] + ControllerHelper::m_byteTable[data2 >> 8];
        }

        data = GetKeyData(rigthPiece);
        data = ((data <<  (15 - wBit)) & 0xFFFF) >> (15 - wBit);
        nPos += ControllerHelper::m_byteTable[data & 255] + ControllerHelper::m_byteTable[data >> 8];

        m_lastKey = nPos - 1;

        return m_lastKey;
    }

    uint32 GetNumKeys() const {return GetHeader()->m_Size; };

    uint32 AssignKeyTime(const char* pData, uint32 numElements)
    {
#ifndef _RELEASE
        if (IsConstant())
        {
            __debugbreak();
            return 0;
        }
#endif

        if (m_pKeys)
        {
            CryModuleFree(m_pKeys);
        }

        uint32 sz = sizeof(uint16) * numElements;

        m_pKeys = (uint16*)CryModuleMalloc(sz);
        m_numKeys = numElements;

        memcpy(&m_pKeys[0], pData, sz);

        return sz;
    }

    uint32 GetFormat()
    {
        return eBitset;
    }

    char* GetData()
    {
        return (char*)m_pKeys;
    }

    uint32 GetDataRawSize()
    {
        return (m_numKeys & kKeysSizeMask) * sizeof(uint16);
    }

    IControllerRelocatableChain* GetRelocateChain()
    {
        return NULL;
    }

    void GetMemoryUsage(ICrySizer* pSizer) const
    {
        pSizer->AddObject(this, sizeof(*this));
        pSizer->AddObject(m_pKeys, GetNumKeys_Int() * sizeof(uint16));
    }

private:
    enum
    {
        kKeysSizeMask = 0x7fffffff,
        kKeysConstantMask = 0x80000000,
    };

    struct Header
    {
        uint16      m_Start;
        uint16    m_End;
        uint16      m_Size;
    };

private:
    // Prevent copy construction
    CKeyTimesInformationBitSet(const CKeyTimesInformationBitSet&);
    CKeyTimesInformationBitSet& operator = (const CKeyTimesInformationBitSet&);

private:
    inline Header* GetHeader() const
    {
        return (Header*)m_pKeys;
    };

    inline uint16 GetKeyData(int i) const
    {
        return m_pKeys[3 + i];
    };

    bool IsConstant() const
    {
        return (m_numKeys & kKeysConstantMask) != 0;
    }

    uint32 GetNumKeys_Int() const
    {
        return m_numKeys & kKeysSizeMask;
    }

private:
    uint16* m_pKeys;
    uint32 m_numKeys;

    f32 m_lastTime;
    f32 m_LastDifferenceTime;
    uint32 m_lastKey;
};


//CKeyTimesInformationBitSet
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

/*
template<class _Type, class _Interpolator, class _Base, class _Storage >
*/

class ITrackPositionStorage
    : public IControllerRelocatableChain
{
public:
    virtual ~ITrackPositionStorage() {};
    virtual char* GetData() = 0;
    virtual uint32 AssignData(const char* pData, uint32 numElements) = 0;
    virtual uint32 GetDataRawSize() = 0;
    virtual uint32 GetFormat() = 0;
    virtual void GetValue(uint32 key, Vec3& val) = 0;
    virtual size_t SizeOfPosController() = 0;
    virtual void GetMemoryUsage(ICrySizer* pSizer) const = 0;
};

typedef /*_smart_ptr<ITrackPositionStorage>*/ ITrackPositionStorage* TrackPositionStoragePtr;


class ITrackRotationStorage
    : public IControllerRelocatableChain
{
public:
    virtual ~ITrackRotationStorage() {};
    virtual char* GetData() = 0;
    virtual uint32 AssignData(const char* pData, uint32 numElements) = 0;
    virtual uint32 GetDataRawSize() = 0;
    virtual uint32 GetFormat() = 0;
    virtual void GetValue(uint32 key, Quat& val) = 0;
    virtual size_t SizeOfRotController() = 0;

    void GetMemoryUsage(ICrySizer* pSizer) const { /*nothing*/}
};

// specialization
template<class _Type, class _Storage, class _Base>
class CTrackDataStorageInt
    : public _Base
{
public:
    CTrackDataStorageInt()
        : m_numKeys(0)
        , m_pKeys(NULL)
    {
    }

    CTrackDataStorageInt(_Storage* pData, uint32 numElements)
        : m_numKeys(numElements | (numElements > 0 ? kKeysConstantMask : 0))
        , m_pKeys(pData)
    {
    }

    ~CTrackDataStorageInt()
    {
        if (!IsConstant() && m_hHdl.IsValid())
        {
            g_controllerHeap.Free(m_hHdl);
        }
    }

    virtual uint32 AssignData(const char* pData, uint32 numElements)
    {
#ifndef _RELEASE
        if (IsConstant())
        {
            __debugbreak();
            return 0;
        }
#endif

        if (m_hHdl.IsValid())
        {
            g_controllerHeap.Free(m_hHdl);
        }

        uint32 sz = numElements * sizeof(_Storage);
        this->LinkTo(NULL);
        CControllerDefragHdl hdl = g_controllerHeap.AllocPinned(sz, this);
        m_hHdl = hdl;
        m_pKeys = (_Storage*)g_controllerHeap.WeakPin(hdl);
        m_numKeys = numElements;
        memcpy(m_pKeys, pData, sz);
        g_controllerHeap.Unpin(hdl);

        return sz;
    }

    char* GetData()
    {
        return (char*) m_pKeys;
    }

    uint32 GetDataRawSize()
    {
        return GetNumKeys() * sizeof(_Storage);
    }

    void Relocate(char* pDst, char* pSrc)
    {
        ptrdiff_t offs = reinterpret_cast<char*>(m_pKeys) - pSrc;
        m_pKeys = reinterpret_cast<_Storage*>(pDst + offs);
    }

    void GetValue(uint32 key, _Type& val)
    {
        _Storage l = m_pKeys[key];
        l.ToExternalType(val);
    }

    virtual uint32 GetFormat()
    {
        return _Storage::GetFormat();
    }

    virtual size_t SizeOfRotController()
    {
        size_t keys = GetNumKeys();
        size_t s0 = sizeof(*this);
        size_t s1 = keys * sizeof(_Storage);
        return s0 + s1;
    }
    virtual size_t SizeOfPosController()
    {
        size_t keys = GetNumKeys();
        size_t s0 = sizeof(*this);
        size_t s1 = keys * sizeof(_Storage);
        return s0 + s1;
    }

private:
    enum
    {
        kKeysSizeMask = 0x7fffffff,
        kKeysConstantMask = 0x80000000,
    };

private:
    CTrackDataStorageInt(const CTrackDataStorageInt<_Type, _Storage, _Base>&);
    CTrackDataStorageInt<_Type, _Storage, _Base>& operator = (const CTrackDataStorageInt<_Type, _Storage, _Base>&);

private:
    ILINE uint32 IsConstant() const { return m_numKeys & kKeysConstantMask; }
    ILINE uint32 GetNumKeys() const { return m_numKeys & kKeysSizeMask; }

private:
    _Storage* m_pKeys;
    uint32 m_numKeys;
    CControllerDefragHdl m_hHdl;
};

template<class _Type, class _Storage, class _Base>
class CTrackDataStorage
    : public CTrackDataStorageInt<_Type, _Storage, _Base>
{
public:
    typedef _Storage StorageType;

public:
    CTrackDataStorage() {}
    CTrackDataStorage(_Storage* pData, uint32 numElements)
        : CTrackDataStorageInt<_Type, _Storage, _Base>(pData, numElements)
    {
    }

    virtual void AddRef() { }
    virtual void Release() { delete this; }

    virtual void GetMemoryUsage(ICrySizer* pSizer) const
    {
        pSizer->AddObject(this, sizeof(*this));
    }
};

template<class _Type, class _Storage, class _Base>
class CTrackDataStorageInplace
    : public CTrackDataStorageInt<_Type, _Storage, _Base>
{
public:
    virtual void Release()
    {
        //if (--m_nRefCounter <= 0)
        //  this->~CTrackDataStorageInplace();
    }
    virtual void AddRef()
    {
    }
    virtual void GetMemoryUsage(ICrySizer* pSizer) const
    {
        pSizer->AddObject(this, sizeof(*this));
    }
};

// normal
//typedef CTrackDataStorage<Vec3, NoCompressVec3, ITrackPositionStorage> NoCompressPosition;
//typedef _smart_ptr<NoCompressPosition> NoCompressPositionPtr;
//
//typedef CTrackDataStorage<Quat, NoCompressQuat, ITrackRotationStorage> NoCompressRotation;
//typedef _smart_ptr<NoCompressRotation> NoCompressRotationPtr;
//
//typedef CTrackDataStorage<Quat, SmallTree48BitQuat, ITrackRotationStorage> SmallTree48BitQuatRotation;
//typedef _smart_ptr<SmallTree48BitQuatRotation> SmallTree48BitQuatRotationPtr;
//
//typedef CTrackDataStorage<Quat, SmallTree64BitQuat, ITrackRotationStorage> SmallTree64BitQuatRotation;
//typedef _smart_ptr<SmallTree64BitQuatRotation> SmallTree64BitQuatRotationPtr;
//
//typedef CTrackDataStorage<Quat, SmallTree64BitExtQuat, ITrackRotationStorage> SmallTree64BitExtQuatRotation;
//typedef _smart_ptr<SmallTree64BitExtQuatRotation> SmallTree64BitExtQuatRotationPtr;
//
//// Inplace copies
//typedef CTrackDataStorageInplace<Vec3, NoCompressVec3, ITrackPositionStorage> NoCompressPositionInplace;
//typedef _smart_ptr<NoCompressPositionInplace> NoCompressPositionInplacePtr;
//
//typedef CTrackDataStorageInplace<Quat, NoCompressQuat, ITrackRotationStorage> NoCompressRotationInplace;
//typedef _smart_ptr<NoCompressRotationInplace> NoCompressRotationInplacePtr;
//
//typedef CTrackDataStorageInplace<Quat, SmallTree48BitQuat, ITrackRotationStorage> SmallTree48BitQuatRotationInplace;
//typedef _smart_ptr<SmallTree48BitQuatRotationInplace> SmallTree48BitQuatRotationInplacePtr;
//
//typedef CTrackDataStorageInplace<Quat, SmallTree64BitQuat, ITrackRotationStorage> SmallTree64BitQuatRotationInplace;
//typedef _smart_ptr<SmallTree64BitQuatRotationInplace> SmallTree64BitQuatRotationInplacePtr;
//
//typedef CTrackDataStorageInplace<Quat, SmallTree64BitExtQuat, ITrackRotationStorage> SmallTree64BitExtQuatRotationInplace;
//typedef _smart_ptr<SmallTree64BitExtQuatRotationInplace> SmallTree64BitExtQuatRotationInplacePtr;

typedef CTrackDataStorage<Vec3, NoCompressVec3, ITrackPositionStorage> NoCompressPosition;
typedef NoCompressPosition NoCompressPositionPtr;

typedef CTrackDataStorage<Quat, NoCompressQuat, ITrackRotationStorage> NoCompressRotation;
typedef NoCompressRotation* NoCompressRotationPtr;

typedef CTrackDataStorage<Quat, SmallTree48BitQuat, ITrackRotationStorage> SmallTree48BitQuatRotation;
typedef SmallTree48BitQuatRotation* SmallTree48BitQuatRotationPtr;

typedef CTrackDataStorage<Quat, SmallTree64BitQuat, ITrackRotationStorage> SmallTree64BitQuatRotation;
typedef SmallTree64BitQuatRotation* SmallTree64BitQuatRotationPtr;

typedef CTrackDataStorage<Quat, SmallTree64BitExtQuat, ITrackRotationStorage> SmallTree64BitExtQuatRotation;
typedef SmallTree64BitExtQuatRotation* SmallTree64BitExtQuatRotationPtr;

// Inplace copies
typedef CTrackDataStorageInplace<Vec3, NoCompressVec3, ITrackPositionStorage> NoCompressPositionInplace;
typedef NoCompressPositionInplace* NoCompressPositionInplacePtr;

typedef CTrackDataStorageInplace<Quat, NoCompressQuat, ITrackRotationStorage> NoCompressRotationInplace;
typedef NoCompressRotationInplace* NoCompressRotationInplacePtr;

typedef CTrackDataStorageInplace<Quat, SmallTree48BitQuat, ITrackRotationStorage> SmallTree48BitQuatRotationInplace;
typedef SmallTree48BitQuatRotationInplace* SmallTree48BitQuatRotationInplacePtr;

typedef CTrackDataStorageInplace<Quat, SmallTree64BitQuat, ITrackRotationStorage> SmallTree64BitQuatRotationInplace;
typedef SmallTree64BitQuatRotationInplace* SmallTree64BitQuatRotationInplacePtr;

typedef CTrackDataStorageInplace<Quat, SmallTree64BitExtQuat, ITrackRotationStorage> SmallTree64BitExtQuatRotationInplace;
typedef SmallTree64BitExtQuatRotationInplace* SmallTree64BitExtQuatRotationInplacePtr;

class ITrackInformation  //: public _reference_target_t
{
public:

    ITrackInformation()
        : m_pKeyTimes(0) {};
    virtual ~ITrackInformation()
    {
        //if (m_pKeyTimes)
        //  delete m_pKeyTimes;
    }

    uint32 GetNumKeys()
    {
        return m_pKeyTimes->GetNumKeys();
    }

    f32 GetTimeFromKey(uint32 key)
    {
        return m_pKeyTimes->GetKeyValueFloat(key);
    }

    void SetKeyTimesInformation(_smart_ptr<IKeyTimesInformation>& ptr)
    {
        m_pKeyTimes = ptr;
    }

    _smart_ptr<IKeyTimesInformation>& GetKeyTimesInformation()
    {
        return m_pKeyTimes;
    }

    virtual char* GetData() = 0;


protected:

    _smart_ptr<IKeyTimesInformation> m_pKeyTimes;
};




//common interface for positions
class IPositionController
    : public ITrackInformation
{
public:
    virtual ~IPositionController()
    {
        delete m_pData;
    };
    IPositionController()
        : m_pData(0) {};

    virtual void GetValue (f32 normalized_time, Vec3& quat) = 0;
    virtual size_t SizeOfPosController()
    {
        uint32 numTimes   = m_pKeyTimes->GetNumKeys();
        uint32 numFormat  = m_pKeyTimes->GetFormat();
        uint32 numRawSize = m_pKeyTimes->GetDataRawSize();
        size_t s0 = sizeof(IPositionController);
        size_t s1 = m_pData->SizeOfPosController();
        return s0 + s1 + numRawSize;
    }
    virtual void GetValueFromKey(uint32 key, Vec3& val);

    virtual void SetPositionStorage(TrackPositionStoragePtr& ptr)
    {
        m_pData = ptr;
    }

    virtual TrackPositionStoragePtr& GetPositionStorage()
    {
        return m_pData;
    }

    virtual uint32 GetFormat()
    {
        return m_pData->GetFormat();
    }

    char* GetData()
    {
        return m_pData->GetData();
    }

    virtual void GetMemoryUsage(ICrySizer* pSizer) const { /* finish this controller madness*/}
protected:
    TrackPositionStoragePtr m_pData;
};


// Common interface for rotation information
class IRotationController
    : public ITrackInformation
{
public:

    IRotationController()
        : m_pData(0) {};


    virtual ~IRotationController()
    {
        delete m_pData;
    };

    virtual void GetValue (f32 normalized_time, Quat& quat) = 0;
    virtual size_t SizeOfRotController()
    {
        uint32 numRawSize = m_pKeyTimes->GetDataRawSize();
        return sizeof(IRotationController) + m_pData->SizeOfRotController() + numRawSize;
    }

    virtual void GetValueFromKey(uint32 key, Quat& val);

    virtual void SetRotationStorage(ITrackRotationStorage*& ptr)
    {
        m_pData = ptr;
    }
    virtual ITrackRotationStorage*& GetRotationStorage()
    {
        return m_pData;
    }

    virtual uint32 GetFormat()
    {
        return m_pData->GetFormat();
    }

    char* GetData()
    {
        return m_pData->GetData();
    }

    virtual void GetMemoryUsage(ICrySizer* pSizer) const
    {
        pSizer->AddObject(m_pData);
    }
protected:
    ITrackRotationStorage* m_pData;
};


typedef IRotationController* RotationControllerPtr;
typedef IPositionController* PositionControllerPtr;

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

    ~CAdvancedTrackInformation()
    {
    }

    virtual void GetValue (f32 normalized_time, _Type& pos)
    {
        //DEFINE_PROFILER_SECTION("ControllerPQ::GetValue");
        f32 t;
        uint32 key = this->m_pKeyTimes->GetKey(normalized_time, t);

        if (key == 0)
        {
            DEFINE_ALIGNED_DATA (_Type, p1, 16);
            _Base::GetValueFromKey(0, p1);
            pos = p1;
        }
        else
        {
            uint32 numKeys = this->GetNumKeys();
            if (key >= numKeys)
            {
                DEFINE_ALIGNED_DATA (_Type, p1, 16);
                _Base::GetValueFromKey(key - 1, p1);
                pos = p1;
            }
            else
            {
                //_Type p1, p2;
                DEFINE_ALIGNED_DATA (_Type, p1, 16);
                DEFINE_ALIGNED_DATA (_Type, p2, 16);
                _Base::GetValueFromKey(key - 1, p1);
                _Base::GetValueFromKey(key, p2);

                _Interpolator::Blend(pos, p1, p2, t);
            }
        }
    }
};

template<class _Type, class _Interpolator, class _Base>
class CAdvancedTrackInformationInplace
    : public CAdvancedTrackInformation<_Type, _Interpolator, _Base>
{
    virtual void Release()
    {
        //if (--m_nRefCounter <= 0)
        //  delete this;
    }
    virtual void AddRef()
    {
    }
};

typedef CAdvancedTrackInformation<Quat, QuatLerp, IRotationController> RotationTrackInformation;
typedef RotationTrackInformation* RotationTrackInformationPtr;

typedef CAdvancedTrackInformation<Vec3, Vec3Lerp, IPositionController> PositionTrackInformation;
typedef PositionTrackInformation* PositionTrackInformationPtr;

typedef CAdvancedTrackInformationInplace<Quat, QuatLerp, IRotationController> RotationTrackInformationInplace;
typedef RotationTrackInformationInplace* RotationTrackInformationInplacePtr;

typedef CAdvancedTrackInformationInplace<Vec3, Vec3Lerp, IPositionController> PositionTrackInformationInplace;
typedef PositionTrackInformationInplace* PositionTrackInformationInplacePtr;

class CController
    : public IController
{
public:
    CController()
        : m_pRotationController(0)
        , m_pPositionController(0) {}

    ~CController()
    {
        if (m_pRotationController)
        {
            delete m_pRotationController;
        }
        if (m_pPositionController)
        {
            delete m_pPositionController;
        }
    }

    uint32 numKeysPQ() const
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
    }

    JointState GetOPS(f32 normalized_time, Quat& quat, Vec3& pos, Diag33& scale) const
    {
        return
            GetO(normalized_time, quat) |
            GetP(normalized_time, pos) |
            GetS(normalized_time, scale);
    }

    JointState GetOP(f32 normalized_time, Quat& quat, Vec3& pos) const;
    JointState GetO(f32 normalized_time, Quat& quat) const;
    JointState GetP(f32 normalized_time, Vec3& pos) const;
    JointState GetS(f32 normalized_time, Diag33& pos) const { return 0; }

    int32 GetO_numKey() const
    {
        if (m_pRotationController)
        {
            return m_pRotationController->GetNumKeys();
        }
        return -1;
    }
    int32 GetP_numKey() const
    {
        if (m_pPositionController)
        {
            return m_pPositionController->GetNumKeys();
        }
        return -1;
    }

    size_t SizeOfController () const
    {
        size_t res = sizeof(*this);
        if (m_pPositionController)
        {
            res += m_pPositionController->SizeOfPosController();
        }
        if (m_pRotationController)
        {
            res += m_pRotationController->SizeOfRotController();
        }
        return res;
    }

    size_t ApproximateSizeOfThis () const
    {
        return SizeOfController();
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
        //      m_pScaleController = ptr;
    }

    RotationControllerPtr& GetRotationController()
    {
        return m_pRotationController;
    }

    PositionControllerPtr& GetPositionController()
    {
        return m_pPositionController;
    }

    size_t GetRotationKeysNum() const
    {
        if (m_pRotationController)
        {
            return m_pRotationController->GetNumKeys();
        }
        return 0;
    }

    size_t GetPositionKeysNum() const
    {
        if (m_pPositionController)
        {
            return m_pPositionController->GetNumKeys();
        }
        return 0;
    }

    virtual void GetMemoryUsage(ICrySizer* pSizer) const
    {
        pSizer->AddObject(m_pRotationController);
        pSizer->AddObject(m_pPositionController);
    }

protected:
    RotationControllerPtr m_pRotationController;
    PositionControllerPtr m_pPositionController;
};


TYPEDEF_AUTOPTR(CController);

class CControllerInplace
    : public CController
{
public:
    virtual void AddRef()
    {
    }

    virtual void Release()
    {
    }
};

TYPEDEF_AUTOPTR (CControllerInplace);
#endif // CRYINCLUDE_CRYANIMATION_CONTROLLERPQ_H
