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


#ifndef EXCLUDE_OCULUS_SDK

#include "OculusAllocator.h"

//////////////////////////////////////////////////////////////////////////
// OculusAllocator


OculusAllocator::OculusAllocator()
{
}


OculusAllocator& OculusAllocator::GetAccess()
{
    static OculusAllocator s_inst;
    return s_inst;
}


OculusAllocator::~OculusAllocator()
{
}


void* OculusAllocator::Alloc(OVR::UPInt size)
{
    ScopedSwitchToGlobalHeap globalHeap;
    return malloc(size);
}


void* OculusAllocator::Realloc(void* p, OVR::UPInt newSize)
{
    ScopedSwitchToGlobalHeap globalHeap;
    return realloc(p, newSize);
}


void OculusAllocator::Free(void* p)
{
    free(p);
}


#endif // #ifndef EXCLUDE_OCULUS_SDK
