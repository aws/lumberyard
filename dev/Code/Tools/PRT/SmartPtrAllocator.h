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

// Description : smart pointer using the own allocator, parts are copied and changed from LOKI
#ifndef CRYINCLUDE_TOOLS_PRT_SMARTPTRALLOCATOR_H
#define CRYINCLUDE_TOOLS_PRT_SMARTPTRALLOCATOR_H
#pragma once

#include "SHAllocator.h"
#include <functional>
#include <assert.h>

namespace NSH
{
	template<int> struct CompileTimeError;
	template<> struct CompileTimeError<true> {};

	//! class template SSelect
	//! Selects one of two types based upon a boolean constant
	//! Invocation: SSelect<flag, T, U>::Result
	//! where:
	//! flag is a compile-time boolean constant
	//! T and U are types
	//! Result evaluates to T if flag is true, and to U otherwise.
	template <bool flag, typename T, typename U>
	struct SSelect
	{
		typedef T Result;
	};

	template <typename T, typename U>
	struct SSelect<false, T, U>
	{
		typedef U Result;
	};

	//! default new and delete operator encapsulation to meet interface required by CRefCounted
	class CDefaultAllocator
	{
	public:
		void* new_mem(size_t Mem) 
		{
			return ::operator new(Mem);
		}

		void delete_mem(void *pMem, size_t Mem) 
		{
			::operator delete(pMem);
		}
	};

	//! class template CDefaultSPStorage
	//! Implementation of the StoragePolicy used by CSmartPtr
	template <class T>
	class CDefaultSPStorage
	{
	public:
		typedef T* StoredType;    //! the type of the pointee_ object
		typedef T* PointerType;   //! type returned by operator->
		typedef T& ReferenceType; //! type returned by operator*
		CDefaultSPStorage() : pointee_(Default()) 
		{}

		//! The storage policy doesn't initialize the stored pointer 
		//!    which will be initialized by the OwnershipPolicy's Clone fn
		CDefaultSPStorage(const CDefaultSPStorage&)
		{}

		template <class U>
		CDefaultSPStorage(const CDefaultSPStorage<U>&) 
		{}
		
		CDefaultSPStorage(const StoredType& p) : pointee_(p) {}
		
		PointerType operator->() const { return pointee_; }
		
		ReferenceType operator*() const { return *pointee_; }
		
		void Swap(CDefaultSPStorage& rhs)
		{ std::swap(pointee_, rhs.pointee_); }

		//! Accessors
		friend inline PointerType GetImpl(const CDefaultSPStorage& sp)
		{ return sp.pointee_; }
		  
		friend inline const StoredType& GetImplRef(const CDefaultSPStorage& sp)
		{ return sp.pointee_; }

		friend inline StoredType& GetImplRef(CDefaultSPStorage& sp)
		{ return sp.pointee_; }

	protected:
		//! Destroys the data stored
		//! (Destruction might be taken over by the OwnershipPolicy)
		void Destroy()
		{ delete pointee_; }
		
		//! Default value to initialize the pointer
		static StoredType Default()
		{ return 0; }

	private:
		StoredType pointee_;	//! Data
	};

	//! class template CRefCounted
	//! Implementation of the OwnershipPolicy used by CSmartPtr
	//! Provides a classic external reference counting implementation
//	template <class P, class A = CDefaultAllocator>//uses normal allocator
//	template <class P, class A = CDefaultAllocator >//uses own allocator
//	class CRefCounted;

	template <class P, class A>
	class CRefCounted
	{
	public:
		CRefCounted() 
		{
			static A sAllocator;
			pCount_ = static_cast<unsigned int*>(sAllocator.new_mem(sizeof(unsigned int)));
			assert(pCount_);
			*pCount_ = 1;
		}
		
		CRefCounted(const CRefCounted& rhs) 
		: pCount_(rhs.pCount_)
		{}
		
		template <typename P1, class A1>
		CRefCounted(const CRefCounted<P1, A1>& rhs) 
		: pCount_(reinterpret_cast<const CRefCounted&>(rhs).pCount_)
		{}
		
		P Clone(const P& val)
		{
			++*pCount_;
			return val;
		}
		
		bool Release(const P&)
		{
			if (!--*pCount_)
			{	
				static A sAllocator;
				sAllocator.delete_mem(pCount_, sizeof(unsigned int));
				return true;
			}
			return false;
		}
		
		void Swap(CRefCounted& rhs)
		{ std::swap(pCount_, rhs.pCount_); }

		enum { destructiveCopy = false };

	private:
		unsigned int* pCount_;			//! Data
	};

	//! class template CCOMRefCounted
	//! Implementation of the OwnershipPolicy used by CSmartPtr
	//! Adapts COM intrusive reference counting to OwnershipPolicy-specific syntax
//	template <class P, class A = CDefaultAllocator>
//	class CCOMRefCounted;

	template <class P, class A>
	class CCOMRefCounted
	{
	public:
		CCOMRefCounted()
		{}
		
		template <class U, class AA>
		CCOMRefCounted(const CCOMRefCounted<U,AA>&)
		{}
		
		static P Clone(const P& val)
		{
			val->AddRef();
			return val;
		}
		
		static bool Release(const P& val)
		{ val->Release(); return false; }
		
		enum { destructiveCopy = false };
		
		static void Swap(CCOMRefCounted&)
		{}
	};

	//! class template SDeepCopy
	//! Implementation of the OwnershipPolicy used by CSmartPtr
	//! Implements deep copy semantics, assumes existence of a Clone() member function of the pointee type
	template <class P>
	struct SDeepCopy
	{
		SDeepCopy()
		{}
		
		template <class P1>
		SDeepCopy(const SDeepCopy<P1>&)
		{}
		
		static P Clone(const P& val)
		{ return val->Clone(); }
		
		static bool Release(const P& val)
		{ return true; }
		
		static void Swap(SDeepCopy&)
		{}
		
		enum { destructiveCopy = false };
	};

	//! class template CRefLinked
	//! Implementation of the OwnershipPolicy used by CSmartPtr
	//! Implements reference linking
	namespace NPrivate
	{
		class CRefLinkedBase
		{
		public:
			CRefLinkedBase() 
			{ prev_ = next_ = this; }
		  
			CRefLinkedBase(const CRefLinkedBase& rhs) 
			{
				prev_ = &rhs;
				next_ = rhs.next_;
				prev_->next_ = this;
				next_->prev_ = this;
			}
		  
			bool Release()
			{
				if (next_ == this)
				{   
					assert(prev_ == this);
					return true;
				}
				prev_->next_ = next_;
				next_->prev_ = prev_;
				return false;
			}
		  
			void Swap(CRefLinkedBase& rhs)
			{
				if (next_ == this)
				{
					assert(prev_ == this);
					if (rhs.next_ == &rhs)
					{
						assert(rhs.prev_ == &rhs);
						// both lists are empty, nothing 2 do
						return;
					}
					prev_ = rhs.prev_;
					next_ = rhs.next_;
					prev_->next_ = next_->prev_ = this;
					rhs.next_ = rhs.prev_ = &rhs;
					return;
				}
				if (rhs.next_ == &rhs)
				{
					rhs.Swap(*this);
					return;
				}
				std::swap(prev_, rhs.prev_);
				std::swap(next_, rhs.next_);
				std::swap(prev_->next_, rhs.prev_->next_);
				std::swap(next_->prev_, rhs.next_->prev_);
			}
		      
			enum { destructiveCopy = false };

		private:
				mutable const CRefLinkedBase* prev_;
				mutable const CRefLinkedBase* next_;
		};
	}

	template <class P>
	class CRefLinked : public NPrivate::CRefLinkedBase
	{
	public:
		CRefLinked()
		{}
		
		template <class P1>
		CRefLinked(const CRefLinked<P1>& rhs) 
		: NPrivate::CRefLinkedBase(rhs)
		{}

		static P Clone(const P& val)
		{ return val; }

		bool Release(const P&)
		{ return NPrivate::CRefLinkedBase::Release(); }
	};

	//! class template CDestructiveCopy
	//! Implementation of the OwnershipPolicy used by CSmartPtr
	//! Implements destructive copy semantics (a la std::auto_ptr)
	template <class P>
	class CDestructiveCopy
	{
	public:
		CDestructiveCopy()
		{}
		
		template <class P1>
		CDestructiveCopy(const CDestructiveCopy<P1>&)
		{}
		
		template <class P1>
		static P Clone(P1& val)
		{
			P result(val);
			val = P1();
			return result;
		}
		
		static bool Release(const P&)
		{ return true; }
		
		static void Swap(CDestructiveCopy&)
		{}
		
		enum { destructiveCopy = true };
	};

	//! class template SAllowConversion
	//! Implementation of the ConversionPolicy used by CSmartPtr
	//! Allows implicit conversion from CSmartPtr to the pointee type
	struct SAllowConversion
	{
		enum { allow = true };
		void Swap(SAllowConversion&)
		{}
	};

	//! class template SDisallowConversion
	//! Implementation of the ConversionPolicy used by CSmartPtr
	//! Does not allow implicit conversion from CSmartPtr to the pointee type
	//! You can initialize a SDisallowConversion with an SAllowConversion
	struct SDisallowConversion
	{
		SDisallowConversion()
		{}
	  
		SDisallowConversion(const SAllowConversion&)
		{}
		  
		enum { allow = false };

		void Swap(SDisallowConversion&)
		{}
	};

	//! class template SNoCheck
	//! Implementation of the CheckingPolicy used by CSmartPtr
	//! Well, it's clear what it does :o)
	template <class P>
	struct SNoCheck
	{
		SNoCheck()
		{}
		
		template <class P1>
		SNoCheck(const SNoCheck<P1>&)
		{}
		
		static void OnDefault(const P&)
		{}

		static void OnInit(const P&)
		{}

		static void OnDereference(const P&)
		{}

		static void Swap(SNoCheck&)
		{}
	};

	//! class template SAssertCheck
	//! Implementation of the CheckingPolicy used by CSmartPtr
	//! Checks the pointer before dereference
	template <class P>
	struct SAssertCheck
	{
		SAssertCheck()
		{}
		
		template <class P1>
		SAssertCheck(const SAssertCheck<P1>&)
		{}
		
		template <class P1>
		SAssertCheck(const SNoCheck<P1>&)
		{}
		
		static void OnDefault(const P&)
		{}

		static void OnInit(const P&)
		{}

		static void OnDereference(P val)
		{ assert(val); }

		static void Swap(SAssertCheck&)
		{}
	};

	//! class template SAssertCheckStrict
	//! Implementation of the CheckingPolicy used by CSmartPtr
	//! Checks the pointer against zero upon initialization and before dereference
	//! You can initialize an SAssertCheckStrict with an SAssertCheck 
	template <class P>
	struct SAssertCheckStrict
	{
		SAssertCheckStrict()
		{}
		
		template <class U>
		SAssertCheckStrict(const SAssertCheckStrict<U>&)
		{}
		
		template <class U>
		SAssertCheckStrict(const SAssertCheck<U>&)
		{}
		
		template <class P1>
		SAssertCheckStrict(const SNoCheck<P1>&)
		{}
		
		static void OnDefault(P val)
		{ assert(val); }
		
		static void OnInit(P val)
		{ assert(val); }
		
		static void OnDereference(P val)
		{ assert(val); }
		
		static void Swap(SAssertCheckStrict&)
		{}
	};

	/*
	//! class SNullPointerException
	//! Used by some implementations of the CheckingPolicy used by CSmartPtr
	struct SNullPointerException : public std::runtime_error
	{
		SNullPointerException() : std::runtime_error("")
		{ }
	};
	*/
		  
	template<typename T>
	struct SFalseType { enum { Value = false }; };
	
	//! class template SRejectNullStatic
	//! Implementation of the CheckingPolicy used by CSmartPtr
	//! Checks the pointer upon initialization and before dereference
	template <class P>
	struct SRejectNullStatic
	{
		SRejectNullStatic()
		{}
		
		template <class P1>
		SRejectNullStatic(const SRejectNullStatic<P1>&)
		{}
		
		template <class P1>
		SRejectNullStatic(const SNoCheck<P1>&)
		{}
		
		template <class P1>
		SRejectNullStatic(const SAssertCheck<P1>&)
		{}
		
		template <class P1>
		SRejectNullStatic(const SAssertCheckStrict<P1>&)
		{}
		
		static void OnDefault(const P&)
		{
				CompileTimeError< SFalseType<P>::Value > ERROR_This_Policy_Does_Not_Allow_Default_Initialization;
		}
		
		static void OnInit(const P& val)
		{}
		
		static void OnDereference(const P& val)
		{}
		
		static void Swap(SRejectNullStatic&)
		{}
	};

	//! class template SRejectNull
	//! Implementation of the CheckingPolicy used by CSmartPtr
	//! Checks the pointer before dereference
	template <class P>
	struct SRejectNull
	{
		SRejectNull()
		{}
		
		template <class P1>
		SRejectNull(const SRejectNull<P1>&)
		{}
		
		static void OnInit(P val)
		{}

		static void OnDefault(P val)
		{ OnInit(val); }
		
		void OnDereference(P val)
		{ OnInit(val); }
		
		void Swap(SRejectNull&)
		{}        
	};

	//! class template SRejectNullStrict
	//! Implementation of the CheckingPolicy used by CSmartPtr
	//! Checks the pointer upon initialization and before dereference
	template <class P>
	struct SRejectNullStrict
	{
		SRejectNullStrict()
		{}
		
		template <class P1>
		SRejectNullStrict(const SRejectNullStrict<P1>&)
		{}
		
		template <class P1>
		SRejectNullStrict(const SRejectNull<P1>&)
		{}
		
		static void OnInit(P val)
		{}

		void OnDereference(P val)
		{ OnInit(val); }
		
		void Swap(SRejectNullStrict&)
		{}        
	};

	//! class template CByRef
	//! Transports a reference as a value
	//! Serves to implement the Colvin/Gibbons trick for CSmartPtr
	template <class T>
	class CByRef
	{
	public:
		CByRef(T& v) : value_(v) {}
		operator T&() { return value_; }
	private:
		T& value_;
	};

	//! class template CSmartPtr (declaration)
	//! The reason for all the fuss above
	template
	<
		typename T,
		class Allocator = CDefaultAllocator,
		template <class, class> class OwnershipPolicy = CRefCounted,
		class ConversionPolicy = SDisallowConversion,
		template <class> class CheckingPolicy = SAssertCheck,
		template <class> class StoragePolicy = CDefaultSPStorage
	>
	class CSmartPtr;

	//! class template CSmartPtr (definition)
	template
	<
		typename T,
		class Allocator,
		template <class, class> class OwnershipPolicy,
		class ConversionPolicy,
		template <class> class CheckingPolicy,
		template <class> class StoragePolicy
	>
	class CSmartPtr
		: public StoragePolicy<T>
		, public OwnershipPolicy<typename StoragePolicy<T>::PointerType, Allocator>
		, public CheckingPolicy<typename StoragePolicy<T>::StoredType>
		, public ConversionPolicy
	{
		typedef StoragePolicy<T> SP;
		typedef OwnershipPolicy<typename StoragePolicy<T>::PointerType, Allocator> OP;
		typedef CheckingPolicy<typename StoragePolicy<T>::StoredType> KP;
		typedef ConversionPolicy CP;
		
	public:
		typedef typename SP::PointerType PointerType;
		typedef typename SP::StoredType StoredType;
		typedef typename SP::ReferenceType ReferenceType;
		
		typedef typename SSelect<OP::destructiveCopy, CSmartPtr, const CSmartPtr>::Result CopyArg;

		CSmartPtr()
		{ 
			KP::OnDefault(GetImpl(*this)); 
		}

		CSmartPtr(const StoredType& p) : SP(p)
		{ 
			KP::OnInit(GetImpl(*this)); 
		}
		
		CSmartPtr(CopyArg& rhs) : SP(rhs), OP(rhs), KP(rhs), CP(rhs)
		{ 
			GetImplRef(*this) = OP::Clone(GetImplRef(rhs)); 
		}

		template
		<
			typename T1,
			class A,
			template <class,class> class OP1,
			class CP1,
			template <class> class KP1,
			template <class> class SP1
		>
		CSmartPtr(const CSmartPtr<T1, A, OP1, CP1, KP1, SP1>& rhs) : SP(rhs), OP(rhs), KP(rhs), CP(rhs)
		{ 
			GetImplRef(*this) = OP::Clone(GetImplRef(rhs)); 
		}

		template
		<
			typename T1,
			class A,
			template <class,class> class OP1,
			class CP1,
			template <class> class KP1,
			template <class> class SP1
		>
		CSmartPtr(CSmartPtr<T1, A, OP1, CP1, KP1, SP1>& rhs)	: SP(rhs), OP(rhs), KP(rhs), CP(rhs)
		{ 
			GetImplRef(*this) = OP::Clone(GetImplRef(rhs)); 
		}

		CSmartPtr(CByRef<CSmartPtr> rhs)	: SP(rhs), OP(rhs), KP(rhs), CP(rhs)
		{}
		  
		operator CByRef<CSmartPtr>()
		{ 
			return CByRef<CSmartPtr>(*this); 
		}

		CSmartPtr& operator=(CopyArg& rhs)
		{
   		CSmartPtr temp(rhs);
   		temp.Swap(*this);
   		return *this;
		}

		template
		<
			typename T1,
			class A,
			template <class,class> class OP1,
			class CP1,
			template <class> class KP1,
			template <class> class SP1
		>
		CSmartPtr& operator=(const CSmartPtr<T1, A, OP1, CP1, KP1, SP1>& rhs)
		{
   		CSmartPtr temp(rhs);
   		temp.Swap(*this);
   		return *this;
		}
		
		template
		<
			typename T1,
			class A,
			template <class,class> class OP1,
			class CP1,
			template <class> class KP1,
			template <class> class SP1
		>
		CSmartPtr& operator=(CSmartPtr<T1, A, OP1, CP1, KP1, SP1>& rhs)
		{
   		CSmartPtr temp(rhs);
   		temp.Swap(*this);
   		return *this;
		}
		
		void Swap(CSmartPtr& rhs)
		{
   		OP::Swap(rhs);
   		CP::Swap(rhs);
   		KP::Swap(rhs);
   		SP::Swap(rhs);
		}
		
		~CSmartPtr()
		{
   		if (OP::Release(GetImpl(*static_cast<SP*>(this))))
   		{
 				SP::Destroy();
   		}
		}
		
		friend inline void Release(CSmartPtr& sp, typename SP::StoredType& p)
		{
   		p = GetImplRef(sp);
   		GetImplRef(sp) = SP::Default();
		}
		
		friend inline void Reset(CSmartPtr& sp, typename SP::StoredType p)
		{ 
			CSmartPtr(p).Swap(sp); 
		}

		PointerType operator->()
		{
			KP::OnDereference(GetImplRef(*this));
			return SP::operator->();
		}

		PointerType operator->() const
		{
			KP::OnDereference(GetImplRef(*this));
			return SP::operator->();
		}

		ReferenceType operator*()
		{
			KP::OnDereference(GetImplRef(*this));
			return SP::operator*();
		}

		ReferenceType operator*() const
		{
			KP::OnDereference(GetImplRef(*this));
			return SP::operator*();
		}

		bool operator!() const // Enables "if (!sp) ..."
		{ return GetImpl(*this) == 0; }
		
		inline friend bool operator==(const CSmartPtr& lhs,
				const T* rhs)
		{ return GetImpl(lhs) == rhs; }
		
		inline friend bool operator==(const T* lhs,
				const CSmartPtr& rhs)
		{ return rhs == lhs; }
		
		inline friend bool operator!=(const CSmartPtr& lhs,
				const T* rhs)
		{ return !(lhs == rhs); }
		
		inline friend bool operator!=(const T* lhs,
				const CSmartPtr& rhs)
		{ return rhs != lhs; }

		//! Ambiguity buster
		template
		<
			typename T1,
			class A,
			template <class,class> class OP1,
			class CP1,
			template <class> class KP1,
			template <class> class SP1
		>
		bool operator==(const CSmartPtr<T1, A, OP1, CP1, KP1, SP1>& rhs) const
		{ 
			return *this == GetImpl(rhs); 
		}

		template
		<
			typename T1,
			class A,
			template <class,class> class OP1,
			class CP1,
			template <class> class KP1,
			template <class> class SP1
		>
		bool operator!=(const CSmartPtr<T1, A, OP1, CP1, KP1, SP1>& rhs) const
		{ 
			return !(*this == rhs); 
		}

		//! Ambiguity buster
		template
		<
			typename T1,
			class A,
			template <class,class> class OP1,
			class CP1,
			template <class> class KP1,
			template <class> class SP1
		>
		bool operator<(const CSmartPtr<T1, A, OP1, CP1, KP1, SP1>& rhs) const
		{ 
			return *this < GetImpl(rhs); 
		}

	private:
		//! Helper for enabling 'if (sp)'
		struct Tester
		{
			Tester() {}
		private:
			void operator delete(void*);
		};
		  
	public:
		//! enable 'if (sp)'
		operator Tester*() const
		{
			if (!*this) return 0;
			static Tester t;
			return &t;
		}

	private:
		//! Helper for disallowing automatic conversion
		struct Insipid
		{
			Insipid(PointerType) {}
		};
		  
		typedef typename SSelect<CP::allow, PointerType, Insipid>::Result AutomaticConversionResult;

	public:        
			operator AutomaticConversionResult() const
			{ 
				return GetImpl(*this); 
			}
	};

	//! free comparison operators for class template CSmartPtr
	//! operator== for lhs = CSmartPtr, rhs = raw pointer
	template
	<
		typename T,
		class A,
		template <class,class> class OP,
		class CP,
		template <class> class KP,
		template <class> class SP,
		typename U
	>
	inline bool operator==(const CSmartPtr<T, A, OP, CP, KP, SP>& lhs, const U* rhs)
	{ 
		return GetImpl(lhs) == rhs; 
	}

	//! operator== for lhs = raw pointer, rhs = CSmartPtr
	template
	<
		typename T,
		class A,
		template <class,class> class OP,
		class CP,
		template <class> class KP,
		template <class> class SP,
		typename U
	>
	inline bool operator==(const U* lhs, const CSmartPtr<T, A, OP, CP, KP, SP>& rhs)
	{ 
		return rhs == lhs; 
	}

	//! operator!= for lhs = CSmartPtr, rhs = raw pointer
	template
	<
		typename T,
		class A,
		template <class,class> class OP,
		class CP,
		template <class> class KP,
		template <class> class SP,
		typename U
	>
	inline bool operator!=(const CSmartPtr<T, A, OP, CP, KP, SP>& lhs, const U* rhs)
	{ 
		return !(lhs == rhs); 
	}

	//! operator!= for lhs = raw pointer, rhs = CSmartPtr
	template
	<
		typename T,
		class A,
		template <class,class> class OP,
		class CP,
		template <class> class KP,
		template <class> class SP,
		typename U
	>
	inline bool operator!=(const U* lhs, const CSmartPtr<T, A, OP, CP, KP, SP>& rhs)
	{ 
		return rhs != lhs; 
	}

	//! operator< for lhs = CSmartPtr, rhs = raw pointer -- NOT DEFINED
	template
	<
		typename T,
		class A,
		template <class,class> class OP,
		class CP,
		template <class> class KP,
		template <class> class SP,
		typename U
	>
	inline bool operator<(const CSmartPtr<T, A, OP, CP, KP, SP>& lhs,	const U* rhs);
		  
	//! operator< for lhs = raw pointer, rhs = CSmartPtr -- NOT DEFINED
	template
	<
		typename T,
		class A,
		template <class,class> class OP,
		class CP,
		template <class> class KP,
		template <class> class SP,
		typename U
	>
	inline bool operator<(const U* lhs,	const CSmartPtr<T, A, OP, CP, KP, SP>& rhs);
		  
	//! operator> for lhs = CSmartPtr, rhs = raw pointer -- NOT DEFINED
	template
	<
		typename T,
		class A,
		template <class,class> class OP,
		class CP,
		template <class> class KP,
		template <class> class SP,
		typename U
	>
	inline bool operator>(const CSmartPtr<T, A, OP, CP, KP, SP>& lhs,	const U* rhs)
	{ 
		return rhs < lhs; 
	}
		  
	//! operator> for lhs = raw pointer, rhs = CSmartPtr
	template
	<
		typename T,
		class A,
		template <class,class> class OP,
		class CP,
		template <class> class KP,
		template <class> class SP,
		typename U
	>
	inline bool operator>(const U* lhs, const CSmartPtr<T, A, OP, CP, KP, SP>& rhs)
	{ 
		return rhs < lhs; 
	}

	//! operator<= for lhs = CSmartPtr, rhs = raw pointer
	template
	<
		typename T,
		class A,
		template <class,class> class OP,
		class CP,
		template <class> class KP,
		template <class> class SP,
		typename U
	>
	inline bool operator<=(const CSmartPtr<T, A, OP, CP, KP, SP>& lhs, const U* rhs)
	{ 
		return !(rhs < lhs); 
	}
		  
	//! operator<= for lhs = raw pointer, rhs = CSmartPtr
	template
	<
		typename T,
		class A,
		template <class,class> class OP,
		class CP,
		template <class> class KP,
		template <class> class SP,
		typename U
	>
	inline bool operator<=(const U* lhs, const CSmartPtr<T, A, OP, CP, KP, SP>& rhs)
	{ 
		return !(rhs < lhs); 
	}

	//! operator>= for lhs = CSmartPtr, rhs = raw pointer
	template
	<
		typename T,
		class A,
		template <class,class> class OP,
		class CP,
		template <class> class KP,
		template <class> class SP,
		typename U
	>
	inline bool operator>=(const CSmartPtr<T, A, OP, CP, KP, SP>& lhs, const U* rhs)
	{ 
		return !(lhs < rhs); 
	}
		  
	//! operator>= for lhs = raw pointer, rhs = CSmartPtr
	template
	<
		typename T,
		class A,
		template <class,class> class OP,
		class CP,
		template <class> class KP,
		template <class> class SP,
		typename U
	>
	inline bool operator>=(const U* lhs, const CSmartPtr<T, A, OP, CP, KP, SP>& rhs)
	{ 
		return !(lhs < rhs); 
	}
}

#endif // CRYINCLUDE_TOOLS_PRT_SMARTPTRALLOCATOR_H
