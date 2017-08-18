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

#ifndef CRYINCLUDE_TOOLS_SPHERICALHARMONICS_PRT_RASTERTABLE_H
#define CRYINCLUDE_TOOLS_SPHERICALHARMONICS_PRT_RASTERTABLE_H
#pragma once

#if defined(OFFLINE_COMPUTATION)



#include "SimpleTriangleRasterizer.h"                   // CSimpleTriangleRasterizer

//#define SUPPORT_TGA

#if defined(SUPPORT_TGA)
/*for PIX_SaveTGA32 from TGA.CPP*/
#include <stdio.h>
#include <stdlib.h>
#include <memory.h>         // memset
#include <assert.h>         // assert
#define BUFSIZE (16384 * 16)
#define SET16(c, w) (*((unsigned short*)(&(c)))) = (w)  /* low order byte first   */

static int write_block(unsigned char* ptr, int len, FILE* handle)
{
    return((int)fwrite(ptr, 1, len, handle));
}

// buffer is in unsigned long:00rrggbb or UBYTE:bbggrr00 format
static bool PIX_SaveTGA32(const char* filename, unsigned char* inpix2, int w, int h, bool AlphaInclude, bool inbCompress)
{
    unsigned char* fbuf;
    int line, x;

    if (AlphaInclude && inbCompress)
    {
        return(false);      // currently not supported
    }
    FILE* handle = fopen(filename, "wb");

    if (handle == 0)
    {
        return(false);
    }

    if ((fbuf = (unsigned char*)malloc(BUFSIZE + 6144)) == NULL)
    {
        fclose(handle);
        return(false);
    }

    fbuf[0] = 0;                                  // idsize
    fbuf[1] = 0;                                  // maptype  (palette or not)
    fbuf[2] = inbCompress ? 10 : 2;       // ftype
    fbuf[3] = 0;                                  // ?
    fbuf[4] = 0;                                  // ?
    SET16(fbuf[5], 0);                       // maplen (palette)
    fbuf[7] = 0;                                  // mapentsize (palette)
    fbuf[8] = 0;                                  // ?
    fbuf[9] = 0;                                  // ?
    fbuf[10] = 0;                                 // ?
    fbuf[11] = 0;                                 // ?
    SET16(fbuf[12], w);                  // width
    SET16(fbuf[14], h);                  // height

    if (AlphaInclude)
    {
        fbuf[16] = 0x20;                  // ?????
        fbuf[17] = 0x28;                  // flag 0x20 means not flipped
    }
    else
    {
        fbuf[16] = 0x18;                  // ?????
        fbuf[17] = 0x20;                  // flag 0x20 means not flipped
    }

    if (write_block(fbuf, 18, handle) != 18)
    {
        fclose(handle);
        return(false);
    }

    for (line = 0; line < h; ++line)
    {
        if (AlphaInclude)
        {
            unsigned char* gbuf = fbuf;
            for (x = 0; x < w; ++x)
            {
                *gbuf++ = *inpix2++;              // could be optimized
                *gbuf++ = *inpix2++;
                *gbuf++ = *inpix2++;
                *gbuf++ = *inpix2++;
            }

            if (write_block(fbuf, 4 * w, handle) != 4 * w)
            {
                fclose(handle);
                return(false);
            }
        }
        else
        {
            if (!inbCompress)
            {
                unsigned char* gbuf = fbuf;
                for (x = 0; x < w; ++x)
                {
                    *gbuf++ = *inpix2++;
                    *gbuf++ = *inpix2++;
                    *gbuf++ = *inpix2++;
                    inpix2++;
                }

                if (write_block(fbuf, 3 * w, handle) != 3 * w)
                {
                    fclose(handle);
                    return(false);
                }
            }
            else    // compressed
            {
                unsigned long RawCount = 0;
                unsigned char* RawPtr = inpix2;

                for (x = 0; x < w; )
                {
                    if (RawCount == 0)
                    {
                        RawPtr = inpix2;
                    }

                    unsigned long r = *inpix2++, g = *inpix2++, b = *inpix2++;
                    inpix2++;
                    unsigned long dat = (b << 16) | (g << 8) | (r);
                    x++;

                    unsigned long CompCount = 1;

                    while (x < w && CompCount + RawCount < 128)
                    {
                        unsigned long r = *inpix2++, g = *inpix2++, b = *inpix2++;
                        inpix2++;
                        unsigned long here = (b << 16) | (g << 8) | (r);
                        x++;

                        if (here != dat)
                        {
                            inpix2 -= 4;
                            x--;
                            break;
                        }

                        CompCount++;
                    }

                    if (RawCount > 127)                                            // uncompressed
                    {
                        putc(RawCount - 1, handle);

                        for (unsigned long i = 0; i < RawCount; i++)
                        {
                            putc(*RawPtr++, handle);
                            putc(*RawPtr++, handle);
                            putc(*RawPtr++, handle);
                            RawPtr++;
                        }

                        RawCount = 0;
                    }

                    if (CompCount > 2)
                    {
                        if (RawCount)                                                // uncompressed
                        {
                            putc(RawCount - 1, handle);

                            for (unsigned long i = 0; i < RawCount; i++)
                            {
                                putc(*RawPtr++, handle);
                                putc(*RawPtr++, handle);
                                putc(*RawPtr++, handle);
                                RawPtr++;
                            }

                            RawCount = 0;
                        }

                        putc(0x80 | (CompCount - 1), handle);        // compressed

                        putc(r, handle);
                        putc(g, handle);
                        putc(b, handle);
                    }
                    else
                    {
                        RawCount += CompCount;
                    }
                }

                if (RawCount)
                {
                    putc(RawCount - 1, handle);                    // uncompressed

                    for (unsigned long i = 0; i < RawCount; i++)
                    {
                        putc(*RawPtr++, handle);
                        putc(*RawPtr++, handle);
                        putc(*RawPtr++, handle);
                        RawPtr++;
                    }
                }
            }
        }
    }
    fclose(handle);
    return(true);
}

#endif


// This class is to speed up raycasting queries.
// each element in a 2D raster has a pointer (or zero) to a few elements in a second memory(elemets: void *) (0 teminated)
// The raster can be used to query elements on a 3D line - if you project the 3D World down to that raster from one side.
// To get the maximum speedup you should do this from 3 sides.
template <class T>
class CRasterTable
{
public:
    //! constructor
    CRasterTable();
    CRasterTable(const CRasterTable<T>& crCopyFrom);

    //! destructor
    virtual ~CRasterTable();

    //! alloc raster, after that Phase 1 has started
    //! /param indwXSize 1..
    //! /param indwYSize 1..
    //! /return true=success, false otherwise
    bool Init(const uint32 indwXSize, const uint32 indwYSize);

    //!
    //! free data
    void DeInit();

    //! Has to be called in Phase 1, after Phase 2 has started
    //! /return true=success, false otherwise
    bool PreProcess();

    //! call this per triangle after Init and before PreProcess
    //! after PreProcess call it again for every triangle
    //! /param infX[3] x coordiantes of the 3 vertices in raster units
    //! /param infX[3] y coordiantes of the 3 vertices in raster units
    //! /param inElement element you want to store
    void PutInTriangle(float infX[3], float infY[3], T & inElement);

    //! returns pointer to zero terminated array of pointers
    //! /param iniX 0..m_dwXSize-1
    //! /param iniY 0..m_dwYSize-1
    //! /return
    const T* GetElementsAt(int iniX, int iniY) const;

    //!
    //! /return memory consumption in bytes O(1)
    uint32 CalcMemoryConsumption();

    //!
    //! /param inPathFileName filename with path and extension
    void Debug(const char* inPathFileName) const;

    //!
    //! /param outPitchInBytes
    //! /return
    uint32* GetDebugData(uint32& outPitchInBytes) const;

    //!
    //! /return width in raster elements
    uint32 GetWidth();

    //!
    //! /return height in raster elements
    uint32 GetHeight();

    /************************************************************************************************************************************************/

    union
    {
        uint32*                       m_pDataCounter;           //!< used in Phase 1 [m_dwXSize*m_dwYSize]
        T**                                m_pDataPtr;                  //!< used in Phase 2 [m_dwXSize*m_dwYSize]
    };

private:
    T*                                     m_pExtraMemoryPool;  //!< for the pointers to store (zero terminated)  (sorted in ascending order) [m_ExtraMemorySize]
    uint32                              m_ExtraMemorySize;  //!< extra memroy pool size in StoredElementPtr elements

    uint32                              m_dwXSize;                  //!< width of the buffer 1.. if allocated, otherwise 0
    uint32                              m_dwYSize;                  //!< height of the buffer 1.. if allocated, otherwise 0

    /************************************************************************************************************************************************/

    //! callback function
    class CPixelIncrement
        : public CSimpleTriangleRasterizer::IRasterizeSink
    {
    public:

        //! constructor
        CPixelIncrement(uint32* inpBuffer, uint32 indwWidth, uint32 indwHeight)
        {
            m_pBuffer = inpBuffer;
            m_dwWidth = indwWidth;
            m_dwHeight = indwHeight;
        }

        //!
        virtual void Line(const float, const float,
            const int iniLeft, const int iniRight, const int iniY)
        {
            assert(iniLeft >= 0);
            assert(iniY >= 0);
            assert(iniRight <= (int)m_dwWidth);
            assert(iniY < (int)m_dwHeight);

            uint32* pMem = &m_pBuffer[iniLeft + iniY * m_dwWidth];

            for (int x = iniLeft; x < iniRight; x++)
            {
                *pMem = *pMem + 1;
                pMem++;
            }
        }

        uint32*        m_pBuffer;               //!<
        uint32          m_dwWidth;              //!< x= [0,m_dwWidth[
        uint32          m_dwHeight;             //!< y= [0,m_dwHeight[
    };

    /************************************************************************************************************************************************/

    //! callback function
    template <class U>
    class CPixelAddArrayElement
        : public CSimpleTriangleRasterizer::IRasterizeSink
    {
    public:

        //! constructor
        CPixelAddArrayElement(U** inpBuffer, uint32 indwWidth, U* inElementPtr)
        {
            m_pBuffer = inpBuffer;
            m_dwWidth = indwWidth;
            m_ElementPtr = inElementPtr;
        }

        //!
        inline void ReturnPixel(U*& rPtr)
        {
            --rPtr;
            *rPtr = *m_ElementPtr;
        }

        //!
        virtual void Line(const float, const float, const int iniLeft, const int iniRight, const int iniY)
        {
            U** pMem = &m_pBuffer[iniLeft + iniY * m_dwWidth];

            for (int x = iniLeft; x < iniRight; x++)
            {
                ReturnPixel(*pMem++);
            }
        }

        U*             m_ElementPtr;            //!< element to store
        U**            m_pBuffer;               //!< pointer to the array buffer
        uint32      m_dwWidth;              //!< width of the buffer
    };
};

// constructor
template <class T>
CRasterTable<T>::CRasterTable()
{
    assert(sizeof(uint32) == sizeof(uint32*));       // only for 32 Bit compiler

    m_pDataPtr = 0;
    m_pExtraMemoryPool = 0;
    m_ExtraMemorySize = 0;
}

template <class T>
CRasterTable<T>::CRasterTable(const CRasterTable<T>& crCopyFrom)
    : m_dwXSize(crCopyFrom.m_dwXSize)
    , m_dwYSize(crCopyFrom.m_dwYSize)
    , m_ExtraMemorySize(crCopyFrom.m_ExtraMemorySize)
{
    static CSHAllocator<T> sAllocatorT;
    static CSHAllocator<uint32> sAllocatorUint32;

    m_pExtraMemoryPool  = (T*)(sAllocatorT.new_mem_array(sizeof(T) * m_ExtraMemorySize));
    m_pDataCounter          = (uint32*)(sAllocatorUint32.new_mem_array(sizeof(uint32) * m_dwXSize * m_dwYSize));
    memcpy(m_pExtraMemoryPool, crCopyFrom.m_pExtraMemoryPool, m_ExtraMemorySize * sizeof(T));
    memcpy(m_pDataCounter, crCopyFrom.m_pDataCounter, m_dwXSize * m_dwYSize * sizeof(uint32));
}

// destructor
template <class T>
CRasterTable<T>::~CRasterTable()
{
    DeInit();
}

// free data
template <class T>
void CRasterTable<T>::DeInit()
{
    static CSHAllocator<T> sAllocatorT;
    static CSHAllocator<uint32> sAllocatorUint32;

    sAllocatorT.delete_mem_array(m_pExtraMemoryPool, sizeof(T) * m_ExtraMemorySize);
    m_pExtraMemoryPool = 0;
    sAllocatorUint32.delete_mem_array(m_pDataCounter, sizeof(uint32) * m_dwXSize * m_dwYSize);
    m_pDataCounter = 0;

    m_ExtraMemorySize = 0;
    m_dwXSize = 0;
    m_dwYSize = 0;
}

// alloc raster, after that Phase 1 has started
template <class T>
bool CRasterTable<T>::Init(const uint32 indwXSize, const uint32 indwYSize)
{
    assert(!m_pDataCounter);
    assert(!m_pExtraMemoryPool);
    DeInit();
    assert(indwXSize);
    assert(indwYSize);

    m_dwXSize = indwXSize;
    m_dwYSize = indwYSize;

    assert(sizeof(uint32) == sizeof(T*));
    static CSHAllocator<uint32> sAllocatorUint32;
    m_pDataCounter          = (uint32*)(sAllocatorUint32.new_mem_array(sizeof(uint32) * m_dwXSize * m_dwYSize));

    if (m_pDataCounter)
    {
        memset(m_pDataCounter, 0, m_dwXSize * m_dwYSize * sizeof(uint32));
    }

    return(m_pDataPtr != 0);
}

// Has to be called in Phase1, after that Phase 2 has started
template <class T>
bool CRasterTable<T>::PreProcess()
{
    assert(m_pDataCounter);
    assert(!m_pExtraMemoryPool);
    assert(!m_ExtraMemorySize);

    uint32 dwSum = 0;         // extra memory pool size in StoredElementPtr elements

    {
        uint32* ptr = m_pDataCounter;

        for (uint32 i = 0; i < m_dwXSize * m_dwYSize; i++)
        {
            uint32 dwNumEl = *ptr++;

            if (dwNumEl != 0)
            {
                dwSum += dwNumEl + 1;       // Elements + NullTermination
            }
        }
    }

    if (dwSum == 0)
    {
        return(true);                   // no information to store - no problem
    }
    static CSHAllocator<T> sAllocatorT;
    m_pExtraMemoryPool  = (T*)(sAllocatorT.new_mem_array(sizeof(T) * dwSum));

    m_ExtraMemorySize = dwSum;

    if (!m_pExtraMemoryPool)
    {
        return(false);
    }

    memset(m_pExtraMemoryPool, 0, dwSum * sizeof(T));

    // build the access structure for compressing the elements in a vector (pointer is in the beginning on the last element)
    // after filing in the triangles once again the pointer is on the first element
    {
        uint32* ptr1 = m_pDataCounter;
        T** ptr2 = m_pDataPtr;
        T* dest = m_pExtraMemoryPool;

        for (uint32 i = 0; i < m_dwXSize * m_dwYSize; i++)
        {
            uint32 dwNumEl = *ptr1++;

            if (dwNumEl != 0)
            {
                dest += dwNumEl;
                *ptr2++ = dest;
                dest++;                         // Elements + NullTermination
            }
            else
            {
                *ptr2++ = 0;
            }
        }
    }

    return(true);
}

// call this per triangle after Init and before PreProcess
// after PreProcess call it again for every triangle
template <class T>
void CRasterTable<T>::PutInTriangle(float infX[3], float infY[3], T& inElement)
{
    float fU[3], fV[3];

    for (int i = 0; i < 3; i++)
    {
        fU[i] = infX[i];
        fV[i] = infY[i];
    }

    CSimpleTriangleRasterizer Rasterizer(m_dwXSize, m_dwYSize);

    if (m_pExtraMemoryPool == 0)           // Phase 1
    {
        CPixelIncrement pix(m_pDataCounter, m_dwXSize, m_dwYSize);
        Rasterizer.CallbackFillConservative(fU, fV, &pix);
    }
    else                                                    // Phase 2
    {
        CPixelAddArrayElement<T> pix(m_pDataPtr, m_dwXSize, &inElement);
        Rasterizer.CallbackFillConservative(fU, fV, &pix);
    }
}

// returns pointer to zero terminated array of pointers
template <class T>
const T* CRasterTable<T>::GetElementsAt(int iniX, int iniY) const
{
    assert(iniX >= 0);
    assert(iniX < (int)m_dwXSize);
    assert(iniY >= 0);
    assert(iniY < (int)m_dwYSize);

    const T* pRet = m_pDataPtr[iniY * m_dwXSize + iniX];

    if (pRet)
    {
        assert(*pRet);          // no pointer in the raster should point to empty list
    }
    return(pRet);
}

// calculates memory consumption in bytes O(1)
template <class T>
uint32 CRasterTable<T>::CalcMemoryConsumption(void)
{
    return(m_dwXSize * m_dwYSize * sizeof(m_pDataCounter) + m_ExtraMemorySize * sizeof(T));
}

template <class T>
#if defined(SUPPORT_TGA)
void CRasterTable<T>::Debug(const char* inPathFileName) const
#else
void CRasterTable<T>::Debug(const char*) const
#endif
{
#if defined(SUPPORT_TGA)
    for (uint32 i = 0; i < m_dwXSize * m_dwYSize; i++)
    {
        m_pDataCounter[i] = m_pDataCounter[i] * 10 + 0x0800;
    }

    PIX_SaveTGA32(inPathFileName, (unsigned char*)m_pDataCounter, m_dwXSize, m_dwYSize, false, false);

    for (uint32 i = 0; i < m_dwXSize * m_dwYSize; i++)
    {
        m_pDataCounter[i] = (m_pDataCounter[i] - 0x0800) / 10;
    }
#endif
}

template <class T>
uint32* CRasterTable<T>::GetDebugData(uint32& outPitchInBytes) const
{
    outPitchInBytes = m_dwXSize * sizeof(uint32);
    return(m_pDataCounter);
}

template <class T>
uint32 CRasterTable<T>::GetWidth()
{
    return(m_dwXSize);
}

template <class T>
uint32 CRasterTable<T>::GetHeight()
{
    return(m_dwYSize);
}

#endif
#endif // CRYINCLUDE_TOOLS_SPHERICALHARMONICS_PRT_RASTERTABLE_H
