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

// StdAfx.h : include file for standard system include files,
//  or project specific include files that are used frequently, but
//      are changed infrequently
//

#if !defined(AFX_STDAFX_H__4AA14050_1B79_4A11_9D24_4E209BF87E2C__INCLUDED_)
#define AFX_STDAFX_H__4AA14050_1B79_4A11_9D24_4E209BF87E2C__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include <vector>
#include <map>
#include <algorithm>
#include <float.h>

//#define MEMSTREAM_DEBUG 1
#define MEMSTREAM_DEBUG_TAG (0xcafebabe)

#if defined(MEMSTREAM_DEBUG)
#define MEMSTREAM_DEBUG_ASSERT(x) if (!(x)) { __debugbreak(); }
#else
#define MEMSTREAM_DEBUG_ASSERT(x)
#endif

#ifndef PHYSICS_EXPORTS
#define PHYSICS_EXPORTS
#endif

#if defined(_CPU_ARM)
//FIXME: There's a threading issue in CryPhysics with ARM's weak memory ordering.
#   define MAX_PHYS_THREADS 1
#else
#   if defined(DEDICATED_SERVER)
#       define MAX_PHYS_THREADS 1
#   else
#       define MAX_PHYS_THREADS 4
#   endif
#endif

// Entity profiling only possible in non-release builds
#if !defined(_RELEASE)
# define ENTITY_PROFILER_ENABLED
#endif

#pragma warning (disable : 4554 4305 4244 4996)
#pragma warning (disable : 6326) //Potential comparison of a constant with another constant

// C6326: potential comparison of a constant with another constant
#define CONSTANT_COMPARISON_OK PREFAST_SUPPRESS_WARNING(6326)
// C6384: dividing sizeof a pointer by another value
#define SIZEOF_ARRAY_OK              PREFAST_SUPPRESS_WARNING(6384)
// C6246: Local declaration of <variable> hides declaration of same name in outer scope.
#define LOCAL_NAME_OVERRIDE_OK PREFAST_SUPPRESS_WARNING(6246)
// C6201: buffer overrun for <variable>, which is possibly stack allocated: index <name> is out of valid index range <min> to <max>
#if defined(__clang__)
#define INDEX_NOT_OUT_OF_RANGE _Pragma("clang diagnostic ignored \"-Warray-bounds\"")
#else
#define INDEX_NOT_OUT_OF_RANGE PREFAST_SUPPRESS_WARNING(6201)
#endif
// C6385: invalid data: accessing <buffer name>, the readable size is <size1> bytes, but <size2> bytes may be read
#define NO_BUFFER_OVERRUN            PREFAST_SUPPRESS_WARNING(6385 6386)
// C6255: _alloca indicates failure by raising a stack overflow exception. Consider using _malloca instead
#define STACK_ALLOC_OK               PREFAST_SUPPRESS_WARNING(6255)

#include <platform.h>


#include "Cry_Math.h"
#include "Cry_XOptimise.h"

#define NO_CRY_STREAM

#ifndef NO_CRY_STREAM
#include "Stream.h"
#else
#include "ISystem.h"
#include "ILog.h"

class CStream
{
public:
    bool WriteBits(unsigned char* pBits, uint32 nSize) { return true; }
    bool ReadBits(unsigned char* pBits, uint32 nSize) { return true; }
    bool Write(bool b) { return true; }
    bool Write(char c) { return true; }
    bool Write(unsigned char uc) { return true; }
    bool Write(float f) { return true; }
    bool Write(unsigned short us) { return true; }
    bool Write(short s) { return true; }
    bool Write(int i) { return true; }
    bool Write(unsigned int ui) { return true; }
    bool Write(const Vec3& v) { return true; }
    bool Write(const Ang3& v) { return true; }
    bool Read(bool& b) { return true; }
    bool Read(char& c) { return true; }
    bool Read(unsigned char& uc) { return true; }
    bool Read(unsigned short& us) { return true; }
    bool Read(short& s) { return true; }
    bool Read(int& i) { return true; }
    bool Read(unsigned int& ui) { return true; }
    bool Read(float& f) { return true; }
    bool Read(Vec3& v) { return true; }
    bool Read(Ang3& v) { return true; }
    bool WriteNumberInBits(int n, size_t nSize) { return true; }
    bool WriteNumberInBits(unsigned int n, size_t nSize) { return true; }
    bool ReadNumberInBits(int& n, size_t nSize) { return true; }
    bool ReadNumberInBits(unsigned int& n, size_t nSize) { return true; }
    bool Seek(size_t dwPos = 0) { return true; }
    size_t GetReadPos() { return 0; }
    unsigned char* GetPtr() const { return 0; };
    size_t GetSize() const { return 0; }
    bool SetSize(size_t indwBitSize) { return true; }
};
#endif

#ifdef WIN64
#undef min
#undef max
#endif


#define ENTGRID_2LEVEL

// TODO: reference additional headers your program requires here
#include "CrySizer.h"
#include "primitives.h"
#include "utils.h"
#include "physinterface.h"


int IsPhysThread();
void MarkAsPhysThread();
void MarkAsPhysWorkerThread(int*);
int get_iCaller();
#if MAX_PHYS_THREADS <= 1
int get_iCaller_int();
#else // MAX_PHYS_THREADS>1
#define get_iCaller_int get_iCaller
#endif


#ifndef MAIN_THREAD_NONWORKER
#define FIRST_WORKER_THREAD 1
#else
#define FIRST_WORKER_THREAD 0
#endif

#endif // !defined(AFX_STDAFX_H__4AA14050_1B79_4A11_9D24_4E209BF87E2C__INCLUDED_)
