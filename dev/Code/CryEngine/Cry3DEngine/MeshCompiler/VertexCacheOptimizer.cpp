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
#include "VertexCacheOptimizer.h"


#if defined(AZ_RESTRICTED_PLATFORM)
#undef AZ_RESTRICTED_SECTION
#define VERTEXCACHEOPTIMIZER_CPP_SECTION_1 1
#define VERTEXCACHEOPTIMIZER_CPP_SECTION_2 2
#endif

#if defined(AZ_RESTRICTED_PLATFORM)
#define AZ_RESTRICTED_SECTION VERTEXCACHEOPTIMIZER_CPP_SECTION_1
    #if defined(AZ_PLATFORM_XENIA)
        #include "Xenia/VertexCacheOptimizer_cpp_xenia.inl"
    #elif defined(AZ_PLATFORM_PROVO)
        #include "Provo/VertexCacheOptimizer_cpp_provo.inl"
    #endif
#endif

namespace vcache
{
    bool VertexCacheOptimizer::ReorderIndicesInPlace(void* pIndices, uint32 numIndices, int indexSize)
    {
#if defined(AZ_RESTRICTED_PLATFORM)
#define AZ_RESTRICTED_SECTION VERTEXCACHEOPTIMIZER_CPP_SECTION_2
    #if defined(AZ_PLATFORM_XENIA)
        #include "Xenia/VertexCacheOptimizer_cpp_xenia.inl"
    #elif defined(AZ_PLATFORM_PROVO)
        #include "Provo/VertexCacheOptimizer_cpp_provo.inl"
    #endif
#endif
#if defined(AZ_RESTRICTED_SECTION_IMPLEMENTED)
#undef AZ_RESTRICTED_SECTION_IMPLEMENTED
        #else
        return true;
        #endif
    }
}
