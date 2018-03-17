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

#ifndef CRYINCLUDE_TOOLS_PRT_SHALLOCATOR_H
#define CRYINCLUDE_TOOLS_PRT_SHALLOCATOR_H
#pragma once

// Detect potential mismatches of this macro
#if defined(_MSC_VER) && _MSC_VER >= 1600
#if defined(USE_MEM_ALLOCATOR)
#pragma detect_mismatch("SH_USE_MEM_ALLOCATOR", "yes")
#else
#pragma detect_mismatch("SH_USE_MEM_ALLOCATOR", "no")
#endif
#endif

#ifdef _WIN32
	#include <windows.h>
#endif
#include <limits>

#if   !defined(LINUX) && !defined(APPLE)
#ifdef __cplusplus
	#include <new.h> 
#endif
#endif

#undef min
#undef max

#ifdef __APPLE__
typedef long long int int64;
#endif

//class specific new handlers
#define INSTALL_CLASS_NEW(T) \
	static void* operator new(size_t size)\
	{\
		static CSHAllocator<T> sAllocator;\
		return sAllocator.new_mem(size);\
	}\
	static void* operator new(size_t, void *p)\
	{\
		return p;\
	}\
	static void* operator new(size_t size, const std::nothrow_t&)\
	{\
	  static CSHAllocator<T> sAllocator;\
		return sAllocator.new_mem(size);\
	}\
	static void operator delete(void *pMem, size_t size)\
	{\
		static CSHAllocator<T> sAllocator;\
		sAllocator.delete_mem(pMem, size);\
	}\
	static void *operator new[](size_t size)\
	{\
		static CSHAllocator<T> sAllocator;\
		return sAllocator.new_mem_array(size);\
	}\
	static void *operator new[](size_t size, const std::nothrow_t&)\
	{\
	  static CSHAllocator<T> sAllocator;\
		return sAllocator.new_mem_array(size);\
	}\
	static void operator delete[](void *pMem, size_t size)\
	{\
		static CSHAllocator<T> sAllocator;\
		sAllocator.delete_mem_array(pMem, size);\
	}\

#if defined(_WIN32) || defined(_WIN64)
	//#define USE_MEM_ALLOCATOR
#endif

#if defined(SH_ALLOCATOR_EXPORT)
	#define SH_ALLOCATOR_API __declspec(dllexport)
#else
	#define SH_ALLOCATOR_API __declspec(dllimport)
#endif

#if defined(USE_MEM_ALLOCATOR)

typedef void *(*FNC_SHMalloc)(size_t Size);
typedef void (*FNC_SHFreeSize)(void *pPtr, size_t Size);

void LoadAllocatorModule(FNC_SHMalloc&, FNC_SHFreeSize&);

extern "C"
{
	void* SHModuleMalloc(size_t Size) ;
	void  SHModuleFreeSize(void *pPtr,size_t Size);
}

extern "C" 
{
#if defined(SH_ALLOCATOR_EXPORT)
	SH_ALLOCATOR_API void *SHMalloc(size_t Size);
	SH_ALLOCATOR_API void SHFreeSize(void *pPtr,size_t Size);
#endif
}

#endif

template <class T = unsigned int>
class CSHAllocator;

template <class T>
class CSHAllocator
{
public:
	// type definitions
	typedef T        value_type;
	typedef T*       pointer;
	typedef const T* const_pointer;
	typedef T&       reference;
	typedef const T& const_reference;
	typedef size_t    size_type;
#ifdef  _WIN64
	typedef __int64 difference_type;
#elif defined(LINUX) || defined(APPLE)
	typedef int64 difference_type;
#else
	typedef _W64 int difference_type;
#endif
	CSHAllocator();
	
	~CSHAllocator();

	// rebind allocator to type U
	template <class U>
	struct rebind 
	{
		typedef CSHAllocator<U> other;
	};

	// return address of values
	pointer address (reference rValue) const 
	{
		return &rValue;
	}

	const_pointer address(const_reference crValue) const 
	{
		return &crValue;
	}

	CSHAllocator(const CSHAllocator& crSrc)  
	{
#if defined(USE_MEM_ALLOCATOR)
		m_pSHAllocFnc			= crSrc.m_pSHAllocFnc;
		m_pSHFreeSizeFnc	= crSrc.m_pSHFreeSizeFnc;
#endif
	}

	const CSHAllocator& operator=(const CSHAllocator& crSrc)  
	{
#if defined(USE_MEM_ALLOCATOR)
		m_pSHAllocFnc			= crSrc.m_pSHAllocFnc;
		m_pSHFreeSizeFnc	= crSrc.m_pSHFreeSizeFnc;
#endif
		return *this;
	}

	template <class U >
	CSHAllocator(const CSHAllocator<U>& crSrc)  
	{
#if defined(USE_MEM_ALLOCATOR)
		m_pSHAllocFnc			= crSrc.m_pSHAllocFnc;
		m_pSHFreeSizeFnc	= crSrc.m_pSHFreeSizeFnc;
#endif
	}
	
	// allocate but don't initialize num elements of type T
	pointer allocate(size_type Num, const void* = 0) 
	{
		// print message and allocate memory with global new
#if defined(USE_MEM_ALLOCATOR)
		pointer pRet = (pointer)m_pSHAllocFnc(Num * sizeof(T));
#else
		pointer pRet = (pointer)malloc(Num * sizeof(T));
#endif
		return pRet;
	}

	// initialize elements of allocated storage p with value value
	void construct(pointer pPtr, const T& crValue) 
	{
		// initialize memory with placement new
		::new(reinterpret_cast<void*>(pPtr) ) T(crValue);
	}

	// destroy elements of initialized storage p
	void destroy(pointer pPtr) 
	{
		// destroy objects by calling their destructor
		pPtr->~T();
	}

	// deallocate storage p of deleted elements
	void deallocate(pointer pPtr, size_type Num) 
	{
		if(pPtr == NULL)
			return;
#if defined(USE_MEM_ALLOCATOR)
		m_pSHFreeSizeFnc(pPtr, Num * sizeof(T));
#else
		free(pPtr);
#endif
	}

	size_type max_size () const
	{
		return std::numeric_limits<size_t>::max() / sizeof(T);
	}

	// templatized new operator 
	void* new_mem(size_type Mem) 
	{
/*		// print message and allocate memory with global new
		if(Mem != sizeof(value_type))
			return ::operator new(Mem);
*/
#if defined(USE_MEM_ALLOCATOR)
		void* pRet = (pointer)m_pSHAllocFnc(Mem);
#else
		void* pRet = (pointer)malloc(Mem);
#endif
		return pRet;
	}

	// templatized delete operator 
	void delete_mem(void *pMem, size_type Mem) 
	{
/*		// print message and allocate memory with global new
		if(Mem != sizeof(value_type))
		{
			::operator delete(pMem);
			return;
		}
*/
#if defined(USE_MEM_ALLOCATOR)
		m_pSHFreeSizeFnc(pMem, Mem);
#else
		free(pMem);
#endif
	}
	// templatized new array operator 
	void* new_mem_array(size_type Mem) 
	{
#if defined(USE_MEM_ALLOCATOR)
		void* pRet = (pointer)m_pSHAllocFnc(Mem);
#else
		void* pRet = (pointer)malloc(Mem);
#endif
		return pRet;
	}

	// templatized delete array operator 
	void delete_mem_array(void *pMem, size_type Mem) 
	{
#if defined(USE_MEM_ALLOCATOR)
		m_pSHFreeSizeFnc(pMem, Mem);
#else
		free(pMem);
#endif
	}

protected:
#if defined(USE_MEM_ALLOCATOR)
	FNC_SHMalloc m_pSHAllocFnc;
	FNC_SHFreeSize m_pSHFreeSizeFnc;
#endif

	template<class U>
	friend class CSHAllocator;
};

// return that all specializations of this allocator are interchangeable
template <class T1, class T2>
bool operator ==(const CSHAllocator<T1>&, const CSHAllocator<T2>&)  
{
	return true;
}

template <class T1, class T2>
bool operator !=(const CSHAllocator<T1>&, const CSHAllocator<T2>&)  
{
	return false;
}

template <class T>
inline CSHAllocator<T>::~CSHAllocator()
{
}

template <class T>
CSHAllocator<T>::CSHAllocator() 
{
#if defined(USE_MEM_ALLOCATOR)
	LoadAllocatorModule(m_pSHAllocFnc, m_pSHFreeSizeFnc);
#endif 
}

#endif // CRYINCLUDE_TOOLS_PRT_SHALLOCATOR_H
