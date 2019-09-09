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

#include <AzCore/Memory/Memory.h>

#if defined(AZ_GLOBAL_NEW_AND_DELETE_DEFINED)
#pragma error("NewAndDelete.inl has been included multiple times in a single module")
#endif

#define AZ_GLOBAL_NEW_AND_DELETE_DEFINED
void* operator new(std::size_t size, const AZ::Internal::AllocatorDummy*) { return AZ::OperatorNew(size); }
void* operator new[](std::size_t size, const AZ::Internal::AllocatorDummy*) { return AZ::OperatorNewArray(size); }
void* operator new(std::size_t size, const char* fileName, int lineNum, const char* name, const AZ::Internal::AllocatorDummy*) { return AZ::OperatorNew(size, fileName, lineNum, name); }
void* operator new[](std::size_t size, const char* fileName, int lineNum, const char* name, const AZ::Internal::AllocatorDummy*) { return AZ::OperatorNewArray(size, fileName, lineNum, name); }
void* operator new(std::size_t size) { return AZ::OperatorNew(size); }
void* operator new[](std::size_t size) { return AZ::OperatorNewArray(size); }
void* operator new(size_t size, const std::nothrow_t& nothrow) throw() { return AZ::OperatorNew(size); }
void* operator new[](size_t size, const std::nothrow_t& nothrow) throw() { return AZ::OperatorNewArray(size); }
void operator delete(void* ptr) AZ_TRAIT_OS_DELETE_THROW { AZ::OperatorDelete(ptr); }
void operator delete[](void* ptr) AZ_TRAIT_OS_DELETE_THROW { AZ::OperatorDeleteArray(ptr); }

