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

#include "StdAfx.h"
#include <PRT/SHAllocator.h>

#if defined(USE_MEM_ALLOCATOR)

static void* SHMalloc(size_t Size)
{
    return malloc(Size);
}

static void SHFreeSize(void* pPtr, size_t Size)
{
    return free(pPtr);
}

void LoadAllocatorModule(FNC_SHMalloc& pfnMalloc, FNC_SHFreeSize& pfnFree)
{
    pfnMalloc = &SHMalloc;
    pfnFree = &SHFreeSize;
}

CSHAllocator<unsigned char> gsByteAllocator;

#endif
