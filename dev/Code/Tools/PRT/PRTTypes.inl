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

#if defined(USE_MEM_ALLOCATOR)
	extern CSHAllocator<unsigned char> gsByteAllocator;
#endif

namespace NSH
{
	/************************************************************************************************************************************************/

#pragma warning (disable : 4291)
	//simple compatible vector class to make it work with different STLs, keep name similar to stl
	//be aware that it does not behave completely the same
	template<class T>
	inline prtvector<T>::prtvector() : m_pElements(NULL), m_ElemCount(0)
	{}

	template<class T>
	inline prtvector<T>::prtvector(const uint32 cInitialElemCount) : m_pElements(NULL), m_ElemCount(cInitialElemCount)
	{
		if(cInitialElemCount == 0)
			return;
#if defined(USE_MEM_ALLOCATOR)
		m_pElements = (T*)gsByteAllocator.new_mem_array(m_ElemCount * sizeof(T));
#else
		m_pElements = (T*)malloc(m_ElemCount * sizeof(T));
#endif
		for(uint32 i=0; i<m_ElemCount; ++i)
			new (&m_pElements[i]) T();//invoke default ctor
	}

	template<class T>
	inline prtvector<T>::~prtvector()
	{
		if(m_ElemCount)
		{
			for(uint32 i=0; i<m_ElemCount; ++i)
				m_pElements[i].~T();
#if defined(USE_MEM_ALLOCATOR)
			gsByteAllocator.delete_mem_array(m_pElements, m_ElemCount * sizeof(T));
#else
			free(m_pElements);
#endif
		}
#if defined(_DEBUG)
		m_pElements = NULL;
		m_ElemCount = 0;
#endif
	}

	template<class T>
	inline prtvector<T>::prtvector(const prtvector<T>& crCopyFrom)
	{
		m_pElements = NULL;
		m_ElemCount = crCopyFrom.m_ElemCount;
		if(m_ElemCount == 0)
			return;
#if defined(USE_MEM_ALLOCATOR)
		m_pElements = (T*)gsByteAllocator.new_mem_array(m_ElemCount * sizeof(T));
#else
		m_pElements = (T*)malloc(m_ElemCount * sizeof(T));
#endif
		assert(m_pElements);
		for(uint32 i=0; i<m_ElemCount; ++i)
			new (&m_pElements[i]) T(crCopyFrom.m_pElements[i]);//invoke copy ctor
	}

	template<class T>
	inline prtvector<T>& prtvector<T>::operator=(const prtvector<T>& crAssignFrom)
	{
		if(m_pElements)
		{
			assert(m_ElemCount > 0);
			for(uint32 i=0; i<m_ElemCount; ++i)
				m_pElements[i].~T();
#if defined(USE_MEM_ALLOCATOR)
			gsByteAllocator.delete_mem_array(m_pElements, m_ElemCount * sizeof(T));
#else
			free(m_pElements);
#endif
		}
		m_pElements = NULL;
		m_ElemCount = crAssignFrom.m_ElemCount;
		if(m_ElemCount == 0)
			return *this;
#if defined(USE_MEM_ALLOCATOR)
		m_pElements = (T*)gsByteAllocator.new_mem_array(m_ElemCount * sizeof(T));
#else
		m_pElements = (T*)malloc(m_ElemCount * sizeof(T));
#endif
		assert(m_pElements);
		for(uint32 i=0; i<m_ElemCount; ++i)
			m_pElements[i] = crAssignFrom.m_pElements[i];
		return *this;
	}

	template<class T>
	inline void prtvector<T>::resize(const size_t cNewElemCount)
	{
		if(cNewElemCount > m_ElemCount)
		{
			if(m_ElemCount > 0)
			{
				for(uint32 i=0; i<m_ElemCount; ++i)
					m_pElements[i].~T();
#if defined(USE_MEM_ALLOCATOR)
				gsByteAllocator.delete_mem_array(m_pElements, m_ElemCount * sizeof(T));			
			}
			m_pElements = (T*)gsByteAllocator.new_mem_array(cNewElemCount * sizeof(T));
#else
				free(m_pElements);
			}
			m_pElements = (T*)malloc(cNewElemCount * sizeof(T));
#endif
		}
		m_ElemCount = cNewElemCount;
		for(uint32 i=0; i<m_ElemCount; ++i)
			new (&m_pElements[i]) T();//invoke default ctor
	}

	template<class T>
	inline void prtvector<T>::reserve(const size_t cNewElemCount)
	{}

	template<class T>
	inline const bool prtvector<T>::empty() const
	{
		return (m_ElemCount == 0);
	}

	template<class T>
	inline T& prtvector<T>::operator[](const size_t cIndex)
	{
		assert(cIndex < m_ElemCount);
		return m_pElements[cIndex];
	}

	template<class T>
	inline const T& prtvector<T>::operator[](const size_t cIndex) const
	{
		assert(cIndex < m_ElemCount);
		return m_pElements[cIndex];
	}

	template<class T>
	inline typename prtvector<T>::const_iterator prtvector<T>::begin() const
	{
		assert(m_ElemCount > 0);
		return m_pElements;
	}

	template<class T>
	inline typename prtvector<T>::iterator prtvector<T>::begin()
	{
		assert(m_ElemCount > 0);
		return m_pElements;
	}

	template<class T>
	inline typename prtvector<T>::iterator prtvector<T>::end()
	{
		assert(m_ElemCount > 0);
		return (m_pElements + m_ElemCount);
	}

	template<class T>
	inline typename prtvector<T>::const_iterator prtvector<T>::end() const
	{
		assert(m_ElemCount > 0);
		return (m_pElements + m_ElemCount);
	}

	template<class T>
	inline const size_t prtvector<T>::size() const
	{
		return m_ElemCount;
	}

	/************************************************************************************************************************************************/

	template<class T>
	template<class TE>
	inline
#if defined(_MSC_VER)
	typename
#endif
	prtlist<T>::SListElem<TE> prtlist<T>::SListElem<TE>::CreateDummy()
	{
		return SListElem<TE>((SListElem<TE>*)(UINT_PTR)0xFFFFFFFF, NULL);
	}

	template<class T>
	template<class TE>
	inline prtlist<T>::SListElem<TE>::SListElem(const SListElem<TE>& crCopyFrom) 
		: pNext(crCopyFrom.pNext), pPrevious(crCopyFrom.pPrevious), elem(crCopyFrom.elem)
	{}

	template<class T>
	template<class TE>
	inline prtlist<T>::SListElem<TE>::SListElem(SListElem<TE> *cpPrevious, SListElem<TE> *cpNext) 
		: pNext(cpNext), pPrevious(cpPrevious)
	{}
	
	template<class T>
	template<class TE>
	inline prtlist<T>::SListElem<TE>::SListElem(const TE& crElem) 
		: pNext(NULL), pPrevious(NULL), elem(crElem)
	{}

	template<class T>
	template<class TE>
	inline prtlist<T>::SListElem<TE>::SListElem()
	{
		pPrevious = pNext = NULL;
	}

	template<class T>
	template<class TE>
	inline TE& prtlist<T>::SListElem<TE>::operator*()
	{
		return elem;
	}

	template<class T>
	template<class TE>
	inline const TE& prtlist<T>::SListElem<TE>::operator*() const
	{
		return elem;
	}

	template<class T>
	template<class TE>
	inline TE& prtlist<T>::SListElem<TE>::operator->()
	{
		return elem;
	}

	template<class T>
	template<class TE>
	inline const TE& prtlist<T>::SListElem<TE>::operator->() const
	{
		return elem;
	}

	template<class T>
	template<class TE>
	inline
#if defined(_MSC_VER)
	typename
#endif
	prtlist<T>::SListElem<TE>& prtlist<T>::SListElem<TE>::operator=(const
#if defined(_MSC_VER)
	    typename
#endif
	    prtlist<T>::SListElem<TE>& crElem)
	{	
		pNext = crElem.pNext;
		pPrevious = crElem.pPrevious;
		elem = crElem.elem;
		return *this;
	}

	template<class T>
	template<class TE>
	inline
#if defined(_MSC_VER)
	typename
#endif
	prtlist<T>::SListElem<TE>& prtlist<T>::SListElem<TE>::operator++()
	{	
		SListElem<TE> *pNextTmp = pNext;
		if(!pNextTmp)
			operator=(CreateDummy());
		else
		{
			pNext = pNextTmp->pNext;
			pPrevious = pNextTmp->pPrevious;
			elem = pNextTmp->elem;
		}	
		return *this;
	}

	template<class T>
	template<class TE>
	inline
#if defined(_MSC_VER)
	typename
#endif
	prtlist<T>::SListElem<TE> prtlist<T>::SListElem<TE>::operator++(int)
	{
		SListElem<TE> old(*this);
		SListElem<TE> *pNextTmp = pNext;
		if(!pNextTmp)
			operator=(this->m_LastDummy);
		else
		{
			pNext = pNextTmp->pNext;
			pPrevious = pNextTmp->pPrevious;
			elem = pNextTmp->elem;
		}	
		return old;
	}

	template<class T>
	template<class TE>
	inline
#if defined(_MSC_VER)
	typename
#endif
	const bool prtlist<T>::SListElem<TE>::operator !=(const
#if defined(_MSC_VER)
	    typename
#endif
	    prtlist<T>::SListElem<TE>& crE) const
	{
		return (pPrevious != crE.pPrevious) || (pNext != crE.pNext);
	}

	template<class T>
	inline prtlist<T>::prtlist() : m_pFirst(NULL), m_pLastElem(NULL), m_LastDummy(SListElem<T>::CreateDummy())
	{}

/*	template<class T>
	inline typename prtlist<T>::const_iterator prtlist<T>::begin() const
	{
		return (const_iterator)*m_pFirst;
	}
*/
	template<class T>
	inline typename prtlist<T>::iterator prtlist<T>::begin()
	{
		return (iterator)*m_pFirst;
	}

/*	template<class T>
	inline typename prtlist<T>::const_iterator prtlist<T>::end() const
	{
		return (const_iterator)m_LastDummy;
	}
*/
	template<class T>
	inline typename prtlist<T>::iterator prtlist<T>::end()
	{
		return (iterator)m_LastDummy;
	}

	template<class T>
	inline void prtlist<T>::remove(const T& crElem)
	{
		assert(m_pFirst);
		SListElem<T> *iter = m_pFirst;
		while(iter)
		{
			if(iter->elem == crElem)
			{
				//found element
				if(iter->pPrevious)
					iter->pPrevious->pNext = iter->pNext;
				if(iter->pNext)
					iter->pNext->pPrevious = iter->pPrevious;
				if(m_pFirst == iter)
					m_pFirst = NULL;
				if(m_pLastElem == iter)
				{
					if(m_pLastElem == m_pFirst)
						m_pLastElem = NULL;
					else
						m_pLastElem = iter->pPrevious;
				}
#if defined(USE_MEM_ALLOCATOR)
				gsByteAllocator.delete_mem(iter, sizeof(SListElem<T>));
#else
				free(iter);
#endif
				break;
			}
			iter = iter->pNext;
		}
	}

	template<class T>
	inline void prtlist<T>::push_back(const T& crElem)
	{
#if defined(USE_MEM_ALLOCATOR)
			SListElem<T> *pNewElem = (SListElem<T>*)gsByteAllocator.new_mem(sizeof(SListElem<T>));
#else
			SListElem<T> *pNewElem = (SListElem<T>*)malloc(sizeof(SListElem<T>));
#endif
		assert(pNewElem);
		new (pNewElem) SListElem<T>(crElem);
		if(!m_pFirst)
			m_pFirst = pNewElem;
		else
			pNewElem->pPrevious = m_pLastElem;
		pNewElem->pNext = &m_LastDummy;
		m_pLastElem = pNewElem;
	}
#pragma warning (default : 4291) 

	/************************************************************************************************************************************************/

	inline const SDescriptor& SDescriptor::operator=(const SDescriptor& rCopyFrom)
	{
		Bands					= rCopyFrom.Bands;
		Coefficients	= rCopyFrom.Coefficients;
		return *this;
	}

	inline SDescriptor::SDescriptor(const uint8 cNumberOfBands) : Bands(cNumberOfBands), Coefficients(cNumberOfBands*cNumberOfBands){}

	/************************************************************************************************************************************************/

	//!< scalar coefficient type
	template<class TFloatType>
	inline SScalarCoeff_tpl<TFloatType>::SScalarCoeff_tpl() : m_Value(0.){}

	template<class TFloatType>
	inline SScalarCoeff_tpl<TFloatType>::SScalarCoeff_tpl(const TFloatType cX) : m_Value(cX){}

	template<class TFloatType>
	inline const SScalarCoeff_tpl<TFloatType>& SScalarCoeff_tpl<TFloatType>::operator=(const TFloatType cX){m_Value = cX;return *this;}

	template<class TFloatType>
	inline SScalarCoeff_tpl<TFloatType>::operator TFloatType&(){return m_Value;}

	template<class TFloatType>
	inline SScalarCoeff_tpl<TFloatType>::operator const TFloatType&()const{return m_Value;}

	template<class TFloatType>
	inline const TFloatType& SScalarCoeff_tpl<TFloatType>::operator[](const size_t cIndex)const{assert(cIndex == 0);return m_Value;}

	template<class TFloatType>
	inline TFloatType& SScalarCoeff_tpl<TFloatType>::operator[](const size_t cIndex){assert(cIndex == 0);return m_Value;}

	template<class TFloatType>
	template <class TCopyFloatType>
	inline SScalarCoeff_tpl<TFloatType>& SScalarCoeff_tpl<TFloatType>::operator=(const SScalarCoeff_tpl<TCopyFloatType>& crCopyFrom)
	{
		m_Value = (TFloatType)crCopyFrom.m_Value;
		return *this;
	}

	/************************************************************************************************************************************************/

	template<class TFloatType>
	inline SRGBCoeff_tpl<TFloatType>::SRGBCoeff_tpl() : ::Vec3_tpl<TFloatType>(0.,0.,0.){}

	template<class TFloatType>
	inline SRGBCoeff_tpl<TFloatType>::SRGBCoeff_tpl(const TFloatType cX, const TFloatType cY, const TFloatType cZ) : ::Vec3_tpl<TFloatType>(cX, cY, cZ){}

	template<class TFloatType>
	inline const SRGBCoeff_tpl<TFloatType>& SRGBCoeff_tpl<TFloatType>::operator *=(const SRGBCoeff_tpl& rSource)
	{
		//multiply componentwise
		this->x *= rSource.x;
		this->y *= rSource.y;
		this->z *= rSource.z;
		return *this;
	}

	template<class TFloatType>
	inline const SRGBCoeff_tpl<TFloatType>& SRGBCoeff_tpl<TFloatType>::operator *=(const TFloatType cScalar)
	{
		//multiply componentwise
		this->x *= cScalar;
		this->y *= cScalar;
		this->z *= cScalar;
		return *this;
	}

	template<class TFloatType>
	template <class TOtherFloatType>
	inline const SRGBCoeff_tpl<TFloatType>& SRGBCoeff_tpl<TFloatType>::operator =(const Vec3_tpl<TOtherFloatType>& rCopyFrom)
	{
		this->x = (TFloatType)rCopyFrom.x;
		this->y = (TFloatType)rCopyFrom.y;
		this->z = (TFloatType)rCopyFrom.z;
		return *this;
	}

	template<class TFloatType>
	inline SRGBCoeff_tpl<TFloatType>::SRGBCoeff_tpl(const Vec3_tpl<TFloatType>& rCopyFrom) : ::Vec3_tpl<TFloatType>(rCopyFrom){}

	template<class TFloatType>
	template<class TOtherFloatType>
	inline SRGBCoeff_tpl<TFloatType>::operator SRGBCoeff_tpl<TOtherFloatType>()const
	{
		return SRGBCoeff_tpl<TOtherFloatType>((TOtherFloatType)this->x, (TOtherFloatType)this->y, (TOtherFloatType)this->z);
	}

	template<class TFloatType>
	inline const SRGBCoeff_tpl<TFloatType> operator *(const SRGBCoeff_tpl<TFloatType>& r0, const SRGBCoeff_tpl<TFloatType>& r1)
	{
		//multiply componentwise
		return SRGBCoeff_tpl<TFloatType>(r0.x * r1.x, r0.y * r1.y, r0.z * r1.z);
	}

	template<class TFloatType>
	inline const SRGBCoeff_tpl<TFloatType> operator +(const SRGBCoeff_tpl<TFloatType>& r0, const SRGBCoeff_tpl<TFloatType>& r1)
	{
		//multiply componentwise
		return SRGBCoeff_tpl<TFloatType>(r0.x + r1.x, r0.y + r1.y, r0.z + r1.z);
	}

	template<class TFloatType>
	inline const SRGBCoeff_tpl<TFloatType> operator *(const SRGBCoeff_tpl<TFloatType>& r0, const TFloatType cScalar)
	{
		return SRGBCoeff_tpl<TFloatType>(r0.x * cScalar, r0.y * cScalar, r0.z * cScalar);
	}

	/************************************************************************************************************************************************/

	template<class TFloatType>
	inline SPolarCoord_tpl<TFloatType>::SPolarCoord_tpl() : theta(0), phi(0){}

	template<class TFloatType>
	inline SPolarCoord_tpl<TFloatType>::SPolarCoord_tpl(const TFloatType cTheta, const TFloatType cPhi) : theta(cTheta), phi(cPhi){}

	template<class TFloatType>
	template<class TOtherFloatType>
	inline SPolarCoord_tpl<TFloatType>::operator SPolarCoord_tpl<TOtherFloatType>()const
	{
		return SPolarCoord_tpl<TOtherFloatType>((TOtherFloatType)theta, (TOtherFloatType)phi);
	}

	/************************************************************************************************************************************************/

	//!< cartesian coordinate set
	template<class TFloatType>
	inline SCartesianCoord_tpl<TFloatType>::SCartesianCoord_tpl() : ::Vec3_tpl<TFloatType>(0,0,0){}

	template<class TFloatType>
	inline SCartesianCoord_tpl<TFloatType>::SCartesianCoord_tpl(const TFloatType cX, const TFloatType cY, const TFloatType cZ) : ::Vec3_tpl<TFloatType>(cX, cY, cZ){}
	
	template<class TFloatType>
	template<class F>
	inline SCartesianCoord_tpl<TFloatType>::SCartesianCoord_tpl(const SCartesianCoord_tpl<F>& rcCopyFrom) : ::Vec3_tpl<TFloatType>((TFloatType)rcCopyFrom.x, (TFloatType)rcCopyFrom.y, (TFloatType)rcCopyFrom.z){}

	template<class TFloatType>
	inline const SCartesianCoord_tpl<TFloatType> SCartesianCoord_tpl<TFloatType>::operator-()const{return SCartesianCoord_tpl<TFloatType>(-this->x, -this->y, -this->z);}

	template<class TFloatType>
	template<class F>
	inline const SCartesianCoord_tpl<TFloatType>& SCartesianCoord_tpl<TFloatType>::operator =(const Vec3_tpl<F>& rcAssignFrom)
	{
		this->x = rcAssignFrom.x;					this->y = rcAssignFrom.y;					this->z = rcAssignFrom.z;
		return *this;
	}

	template<class TFloatType>
	template<class TOtherFloatType>
	inline SCartesianCoord_tpl<TFloatType>::operator SCartesianCoord_tpl<TOtherFloatType>()const
	{
		return SCartesianCoord_tpl<TOtherFloatType>
			(
				(TOtherFloatType)Vec3_tpl<TFloatType>::x, 
				(TOtherFloatType)Vec3_tpl<TFloatType>::y, 
				(TOtherFloatType)Vec3_tpl<TFloatType>::z
			);
	}

	template<class TFloatType>
	inline void ConvertToCartesian(SCartesianCoord_tpl<TFloatType>& rCartesian, const SPolarCoord_tpl<TFloatType>& crPolar)
	{
		rCartesian.x = Sin(crPolar.theta) * Cos(crPolar.phi);
		rCartesian.y = Sin(crPolar.theta) * Sin(crPolar.phi);
		rCartesian.z = Cos(crPolar.theta);
	}

	template<class TFloatType>
	inline void ConvertUnitToPolar(SPolarCoord_tpl<TFloatType>& rPolar, const SCartesianCoord_tpl<TFloatType>& crCartesian)
	{
		const double cS = Sqrt(crCartesian.x * crCartesian.x + crCartesian.y * crCartesian.y);
		rPolar.theta	= (TFloatType)ACos(crCartesian.z);
		rPolar.phi = (TFloatType)((cS < 0.0000001)?0.:(crCartesian.x>=0.)?ASin(crCartesian.y/cS):(g_cPi-ASin(crCartesian.y/cS)));
		if(rPolar.phi < 0)
			rPolar.phi += (TFloatType)(2.0 * g_cPi);
		assert(rPolar.phi >= 0 && rPolar.phi <= 2 * g_cPi && rPolar.theta >= 0.  && rPolar.theta <= g_cPi);
/*		const double cS = Sqrt(crCartesian.x * crCartesian.x + crCartesian.y * crCartesian.y);
		rPolar.theta	= (TFloatType)ACos(crCartesian.z);
		const TFloatType cASin = (TFloatType)((abs(crCartesian.x) >= 0.005)?ASin(crCartesian.y/cS) : (crCartesian.y>0.f)? g_cPi*0.5 : -g_cPi*0.5);
		rPolar.phi		= (TFloatType)((cS < 0.00001)?0.:(crCartesian.x>=0.)?cASin:(g_cPi-cASin));
		if(rPolar.phi < 0)
			rPolar.phi += (TFloatType)(2.0 * g_cPi);
*/
		assert(rPolar.phi >= 0 && rPolar.phi <= 2 * g_cPi && rPolar.theta >= 0.  && rPolar.theta <= g_cPi);
	}

	template<class TFloatType>
	inline void ConvertToPolar(SPolarCoord_tpl<TFloatType>& rPolar, const SCartesianCoord_tpl<TFloatType>& crCartesian)
	{
		const double cR = Sqrt(crCartesian.x * crCartesian.x + crCartesian.y * crCartesian.y + crCartesian.z * crCartesian.z);
		const double cS = Sqrt(crCartesian.x * crCartesian.x + crCartesian.y * crCartesian.y);
		rPolar.theta = ACos(crCartesian.z/cR);
		rPolar.phi = (TFloatType)((cS < 0.0000001)?0.:(crCartesian.x>=0.)?ASin(crCartesian.y/cS):(g_cPi-ASin(crCartesian.y/cS)));
		if(rPolar.phi < 0)
			rPolar.phi += (TFloatType)(2.0 * g_cPi);
		assert(rPolar.phi >= 0 && rPolar.phi <= 2 * g_cPi && rPolar.theta >= 0.  && rPolar.theta <= g_cPi);
	}

	/************************************************************************************************************************************************/

	template<class TFloatType>
	inline void CalcRotMatrixToSphereOrigin(Matrix33_tpl<TFloatType>& rRotMatrix, const TCartesianCoord& rFrom)
	{
		//retrieve polar coordinates and concatenate the two rotations
		TPolarCoord polarCoord;
		ConvertUnitToPolar(polarCoord, rFrom);
		rRotMatrix.SetRotationZ((float)-polarCoord.phi);
		rRotMatrix = Matrix33_tpl<TFloatType>::CreateRotationY((float)-polarCoord.theta) * rRotMatrix;

		//TODO: check ivos method for creating a rotation matrix toward sphere origin

		//const Vec3 yaxis = GetOrthogonal(rFrom).GetNormalized();
		//const Vec3 xaxis = yaxis % rFrom;
		//rRotMatrix.M00 = xaxis.x;      rRotMatrix.M01 = xaxis.y;		rRotMatrix.M02 = xaxis.z;
		//rRotMatrix.M10 = yaxis.x;      rRotMatrix.M11 = yaxis.y;		rRotMatrix.M12 = yaxis.z;
		//rRotMatrix.M20 = rFrom.x;      rRotMatrix.M21 = rFrom.y;    rRotMatrix.M22 = rFrom.z;
	}

	template<class TFloatType>
	inline void CalcRotMatrixToSphereOrigin(Matrix33_tpl<TFloatType>& rRotMatrix, const TPolarCoord& rFrom)
	{
		rRotMatrix.SetRotationZ((float)-rFrom.phi);
		rRotMatrix = Matrix33_tpl<TFloatType>::CreateRotationY((float)-rFrom.theta) * rRotMatrix;
	}

	template<class TFloatType>
	inline void CalcRotMatrixFromSphereOrigin(Matrix33_tpl<TFloatType>& rRotMatrix, const TCartesianCoord& rTo)
	{
		//retrieve polar coordinates and concatenate the two rotations
		TPolarCoord polarCoord;
		ConvertUnitToPolar(polarCoord, rTo);
		rRotMatrix.SetRotationZ((float)polarCoord.phi);
		rRotMatrix = Matrix33_tpl<TFloatType>::CreateRotationY((float)polarCoord.theta) * rRotMatrix;
	}

	template<class TFloatType>
	inline void CalcRotMatrixFromSphereOrigin(Matrix33_tpl<TFloatType>& rRotMatrix, const TPolarCoord& rTo)
	{
		//retrieve polar coordinates and concatenate the two rotations
		rRotMatrix.SetRotationZ((TFloatType)rTo.phi);
		rRotMatrix = Matrix33_tpl<TFloatType>::CreateRotationY((TFloatType)rTo.theta) * rRotMatrix;
	}

	/************************************************************************************************************************************************/

	template< typename CoeffType >
	inline SCoeffList_tpl<CoeffType>::~SCoeffList_tpl()
	{}

	template< typename CoeffType >
	inline SCoeffList_tpl<CoeffType>::SCoeffList_tpl(const SDescriptor& crDescriptor) : m_Coeffs(crDescriptor.Coefficients){}

	template< typename CoeffType >
	inline SCoeffList_tpl<CoeffType>::SCoeffList_tpl(const uint32 cCoefficients) : m_Coeffs(cCoefficients){}

	template< typename CoeffType >
	template< typename CoeffTypeFrom >
	inline SCoeffList_tpl<CoeffType>::SCoeffList_tpl(const SCoeffList_tpl<CoeffTypeFrom>& rCopyFrom)
	{
		if(m_Coeffs.size() != rCopyFrom.size())
			m_Coeffs.resize(rCopyFrom.size());
		//copy values
		for(size_t i=0; i<rCopyFrom.size(); ++i)
			m_Coeffs[i] = (CoeffType)rCopyFrom.m_Coeffs[i];
	}

	template< typename CoeffType >
	template< typename CoeffTypeFrom >
	inline SCoeffList_tpl<CoeffType>& SCoeffList_tpl<CoeffType>::operator=(const SCoeffList_tpl<CoeffTypeFrom>& rCopyFrom)
	{
		if(m_Coeffs.size() != rCopyFrom.size())
			m_Coeffs.resize(rCopyFrom.size());
		//copy values
		for(size_t i=0; i<rCopyFrom.size(); ++i)
			m_Coeffs[i] = (CoeffType)rCopyFrom.m_Coeffs[i];
		return *this;
	}

	template< typename CoeffType >
	inline void SCoeffList_tpl<CoeffType>::ResetToZero()
	{
		for(int i=0; i<(int)m_Coeffs.size(); ++i)
			for(int j=0; j<(int)CoeffType::Components(); ++j)
				(m_Coeffs[i])[j] = 0.f;
	}

	template< typename CoeffType >
	inline void SCoeffList_tpl<CoeffType>::ReSize(const SDescriptor& crDescriptor)
	{
		m_Coeffs.resize(crDescriptor.Coefficients);
	}

	template< typename CoeffType >
	inline CoeffType& SCoeffList_tpl<CoeffType>::operator()(const int32 cL, const int32 cM)
	{
		return (m_Coeffs[cL * (cL + 1) + cM]);
	}

	template< typename CoeffType >
	inline const CoeffType& SCoeffList_tpl<CoeffType>::operator()(const int32 cL, const int32 cM) const
	{
		return (m_Coeffs[cL * (cL + 1) + cM]);
	}

	template< typename CoeffType >
	inline const SCoeffList_tpl<CoeffType>& SCoeffList_tpl<CoeffType>::operator +=(const SCoeffList_tpl<CoeffType>& rToAdd)
	{
		assert(rToAdd.size() == m_Coeffs.size());
		const int numberToCopy = (int)std::min((size_t)rToAdd.size(), (size_t)m_Coeffs.size());
		for(int i=0; i<numberToCopy; ++i)
			m_Coeffs[i] += rToAdd.m_Coeffs[i];
		return *this;
	}

	template< typename CoeffType >
	inline const SCoeffList_tpl<CoeffType>& SCoeffList_tpl<CoeffType>::operator *=(const double cScalar)
	{
		for(size_t i=0; i<m_Coeffs.size(); ++i)
			m_Coeffs[i] *= (TComponentFloatType)cScalar;
		return *this;
	}

	template< typename CoeffType >
	const CoeffType* const SCoeffList_tpl<CoeffType>::GetCoeffs() const
	{
		return (m_Coeffs.size() > 0)?(CoeffType*)&m_Coeffs[0] : NULL;
	}

	template<typename CoeffType>
	const CoeffType SCoeffList_tpl<CoeffType>::PerformLookup(const CSample_tpl<SCoeffList_tpl<TScalarCoeff> >& crSample, const ECoeffChoice cCoeffChoice) const
	{
		//now set up vectors for dot4
		assert(crSample.NumberOfCoeffs() > 0 && size() > 0);
		const SCoeffList_tpl<TScalarCoeff>& rSampleCoeffList = crSample.Coeffs();
		TComponentType output = m_Coeffs[0] * (typename TComponentType::TComponentType)rSampleCoeffList[0];
		if(crSample.NumberOfCoeffs() > 8)//special case only for >=3 bands
		{
			for(int i=1; i<=3; ++i)
				output += m_Coeffs[i] * (typename TComponentType::TComponentType)rSampleCoeffList[i];
			if(cCoeffChoice != NO_COEFF_Y2m2)//dont use coeff Y2-2
				output += m_Coeffs[4] * (typename TComponentType::TComponentType)rSampleCoeffList[4];
			for(int i=5; i<=7; ++i)
				output += m_Coeffs[i] * (typename TComponentType::TComponentType)rSampleCoeffList[i];
			if(cCoeffChoice != NO_COEFF_Y22)//dont use coeff Y22
				output +=  m_Coeffs[8] * (typename TComponentType::TComponentType)rSampleCoeffList[8];
			for(int i=9; i<crSample.NumberOfCoeffs(); ++i)//not used for 3 bands though
				output += m_Coeffs[i] * (typename TComponentType::TComponentType)rSampleCoeffList[i];
		}
		else//no special case for 2 bands
		for(int i=1; i<crSample.NumberOfCoeffs(); ++i)
			output += m_Coeffs[i] * (typename TComponentType::TComponentType)rSampleCoeffList[i];
		return output;
	}

	template< typename CoeffType >
	inline const size_t SCoeffList_tpl<CoeffType>::size()const{return m_Coeffs.size();}

	template< typename CoeffType >
	inline CoeffType& SCoeffList_tpl<CoeffType>::operator[](const size_t cIndex){assert(cIndex < m_Coeffs.size()); return m_Coeffs[cIndex];}

	template< typename CoeffType >
	inline const CoeffType SCoeffList_tpl<CoeffType>::operator[](const size_t cIndex)const {assert(cIndex < m_Coeffs.size()); return m_Coeffs[cIndex];}

	template<typename CoeffType>
	inline const SCoeffList_tpl<CoeffType> operator +(const SCoeffList_tpl<CoeffType>& r0, const SCoeffList_tpl<CoeffType>& r1)
	{
		assert(r0.size() == r1.size());
		SCoeffList_tpl<CoeffType> sum(r0);
		sum += r1;
		return sum;
	}

	template<typename CoeffType>
	inline const SCoeffList_tpl<CoeffType> operator *(const SCoeffList_tpl<CoeffType>& r0, const SCoeffList_tpl<CoeffType>& r1)
	{
		assert(r0.size() == r1.size());
		SCoeffList_tpl<CoeffType> sum(r0);
		sum *= r1;
		return sum;
	}

	template<typename CoeffType>
	inline const SCoeffList_tpl<CoeffType> operator *(const SCoeffList_tpl<CoeffType>& r0, const double cScalar)
	{
		SCoeffList_tpl<CoeffType> sum(r0);
		sum *= cScalar;
		return sum;
	}

	/************************************************************************************************************************************************/	
	
	template<typename CoeffTypeList>
	inline CSample_tpl<CoeffTypeList>::CSample_tpl(const SDescriptor& crDescriptor, const TPolarCoord& crPolarCoord, const TSampleHandle cHandle)
		: m_PolarCoord(crPolarCoord), m_Coeffs(crDescriptor), m_Handle(cHandle)
	{
		for (int l = 0, c = 0; l < crDescriptor.Bands; l++)
			for (int m = -l; m <= l; m++, c++)
				m_Coeffs[c] = (TScalarCoeff::TComponentType)NLegendre::Y(l, m, crPolarCoord.theta, crPolarCoord.phi, NLegendre::SLEGENDRE_CALC_OPTIMIZED() );
		ConvertToCartesian(m_CartCoord, crPolarCoord);
#if defined(_DEBUG)
		m_Constructed = true;
#endif
	}

	template<typename CoeffTypeList>
	inline CSample_tpl<CoeffTypeList>::CSample_tpl(const CSample_tpl<CoeffTypeList>& crCopyFrom) 
		: m_CartCoord(crCopyFrom.m_CartCoord), m_PolarCoord(crCopyFrom.m_PolarCoord), m_Coeffs(crCopyFrom.m_Coeffs), m_Handle(0)
	{
#if defined(_DEBUG)
		m_Constructed = crCopyFrom.m_Constructed;
#endif
	}

	template<typename CoeffTypeList>
	inline CSample_tpl<CoeffTypeList>::CSample_tpl() : m_PolarCoord(TPolarCoord(0,0)), m_Coeffs(SDescriptor())
	{
		//not constructed, do explicitely
		ConvertToCartesian(m_CartCoord, m_PolarCoord);
#if defined(_DEBUG)
		m_Constructed = false;
#endif
	}

	template<typename CoeffTypeList>
	inline void CSample_tpl<CoeffTypeList>::Validate()const
	{
#if defined(PRT_COMPILE)
		assert(m_Constructed == true);
#endif
	}

	template<typename CoeffTypeList>
	inline const uint8 CSample_tpl<CoeffTypeList>::NumberOfCoeffs()const{Validate(); return (uint8)m_Coeffs.size();}

	template<typename CoeffTypeList>
	inline const TCartesianCoord& CSample_tpl<CoeffTypeList>::GetCartesianPos()const{Validate(); return m_CartCoord;}

	template<typename CoeffTypeList>
	inline const TPolarCoord& CSample_tpl<CoeffTypeList>::GetPolarPos()const{Validate(); return m_PolarCoord;}

	template<typename CoeffTypeList>
	inline const CoeffTypeList& CSample_tpl<CoeffTypeList>::Coeffs()const{Validate(); return m_Coeffs;}

	template<typename CoeffTypeList>
	inline CoeffTypeList& CSample_tpl<CoeffTypeList>::Coeffs(){Validate(); return m_Coeffs;}

	template<typename CoeffTypeList>
	inline void CSample_tpl<CoeffTypeList>::SetHandle(const TSampleHandle cHandle){Validate(); m_Handle = cHandle;}

	template<typename CoeffTypeList>
	inline const TSampleHandle CSample_tpl<CoeffTypeList>::GetHandle()const{Validate(); return m_Handle;}

}//NSH
