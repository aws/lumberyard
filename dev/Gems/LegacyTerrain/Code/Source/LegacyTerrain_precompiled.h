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

//CryEngine/CryCommon
#include <platform.h> // Many CryCommon files require that this is included first.
#include <ISystem.h>
#include <I3DEngine.h>
#include <IRenderAuxGeom.h>


#define UPDATE_PTR_AND_SIZE(_pData, _nDataSize, _SIZE_PLUS) \
    {                                                       \
        _pData += (_SIZE_PLUS);                             \
        _nDataSize -= (_SIZE_PLUS);                         \
        AZ_Assert(_nDataSize >= 0, "What!");                         \
    }


inline void FixAlignment(byte*& pPtr, int& nDataSize)
{
    while ((UINT_PTR)pPtr & 3)
    {
        *pPtr = 222;
        pPtr++;
        nDataSize--;
    }
}


template <class T>
void AddToPtr(byte*& pPtr, int& nDataSize, const T* pArray, int nElemNum, EEndian eEndian, bool bFixAlignment = false)
{
    assert(!((INT_PTR)pPtr & 3));
    memcpy(pPtr, pArray, nElemNum * sizeof(T));
    SwapEndian((T*)pPtr, nElemNum, eEndian);
    pPtr += nElemNum * sizeof(T);
    nDataSize -= nElemNum * sizeof(T);
    assert(nDataSize >= 0);

    if (bFixAlignment)
    {
        FixAlignment(pPtr, nDataSize);
    }
    else
    {
        assert(!((INT_PTR)pPtr & 3));
    }
}

template <class T>
void AddToPtr(byte*& pPtr, const T* pArray, int nElemNum, EEndian eEndian, bool bFixAlignment = false)
{
    assert(!((INT_PTR)pPtr & 3));
    memcpy(pPtr, pArray, nElemNum * sizeof(T));
    SwapEndian((T*)pPtr, nElemNum, eEndian);
    pPtr += nElemNum * sizeof(T);

    if (bFixAlignment)
    {
        int dummy = 0;
        FixAlignment(pPtr, dummy);
    }
    else
    {
        assert(!((INT_PTR)pPtr & 3));
    }
}

