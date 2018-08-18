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

#ifndef CRYINCLUDE_TOOLS_PRT_PRTTYPES_H
#define CRYINCLUDE_TOOLS_PRT_PRTTYPES_H
#pragma once


#pragma warning (disable : 4290)
#pragma warning (disable : 4239)
#pragma warning (disable : 4505)
#pragma warning (disable : 4512)

#undef min
#undef max

#include "SHAllocator.h"
#include <vector>
#include <map>
#include <set>
#include "SmartPtrAllocator.h"
#include "SHMath.h"
#include "Legendre.h"
#include "ISHLog.h"	//to be able to log everywhere

extern NSH::ISHLog& GetSHLog();

class CSimpleIndexedMesh;
class CSimpleIndexedObjMesh;

namespace NSH
{
	namespace NMaterial
	{
		struct ISHMaterial;
	}
}

//!< namespace containing all SH specific types and functions
namespace NSH
{
	//typedef std::basic_string<char, std::char_traits<char>, CSHAllocator<char> > string;	//!< NSH::string typedef to use internal allocator

	static const uint8	gs_cDefaultCoeffCount						= 9;				//!< default number of coefficients
	static const uint8	gs_cDefaultBandCount						= 3;				//!< default number of bands to use
	static const uint8	gs_cSupportedMaxBands						= 5;				//!< currently supported max band number
	static const uint32	gs_MaxSupportedCoefficientSize	= 25;				//!< maximal coefficients supported by matrix (bands*bands), determines which dimension to build for matrix

	typedef uint16			TSampleHandle;					//!< sample handle type (suffice for at least 40000 samples)
	//since max is a somewhere declared macro, i cannot use numeric_limits
	const uint32				g_cMaxHandleTypeValue = ((2 << (sizeof(TSampleHandle) * 8 - 1)) - 1);	//!< maximum value a handle type can get


	//!< descriptor for a spherical harmonics function, main values are the number of used bands and number of coefficients, but some other properties might be added
	typedef struct SDescriptor
	{
		uint8	Bands;					//!< bands used
		uint8	Coefficients;		//!< resulting number of coefficients
		//!< constructor just set it to some valid corresponding values
		SDescriptor(const uint8 cNumberOfBands = gs_cDefaultBandCount);
		const SDescriptor& operator=(const SDescriptor& rCopyFrom);
	}SDescriptor;

	/************************************************************************************************************************************************/

	//simple compatible vector class for scalar built in types to make it work with different STLs, keep name similar to stl
	//be aware that it does not behave completely the same (no push_back, no reserve functionality)
	//has overhead of 4 bytes over array rather than 8 as std::vector
	template<class T>
	class prtvector
	{
	public:
		typedef prtvector<T> TPRTVec;
		INSTALL_CLASS_NEW(TPRTVec)

	private:
		T *m_pElements;			//the elements
		size_t m_ElemCount;	//number of currently allocated elements

	public:
		typedef T* iterator;
		typedef const T* const_iterator;

		prtvector();
		prtvector(const uint32 cInitialElemCount);
		prtvector(const prtvector<T>& crCopyFrom);
		~prtvector();
		prtvector<T>& operator=(const prtvector<T>& crCopyFrom);

		void resize(const size_t cNewElemCount);
		void reserve(const size_t cNewElemCount);
		T& operator[](const size_t cIndex);
		const T& operator[](const size_t cIndex) const;
		const_iterator begin() const;
		iterator begin();
		iterator end();
		const_iterator end() const;
		const size_t size() const;
		const bool empty() const;
	};

	/************************************************************************************************************************************************/

	//simple double linked list class implementing push_back, iterator and remove
	//required to work with mixed STLs
	template<class T>
	class prtlist
	{
	public:
		typedef prtlist<T> TPRTList;
		INSTALL_CLASS_NEW(TPRTList)

	private:
		template<class TE>
		struct SListElem
		{
			typedef SListElem<TE> TPRTListElem;
			INSTALL_CLASS_NEW(TPRTListElem)

			TE elem;
			SListElem<TE> *pPrevious;
			SListElem<TE> *pNext;

			SListElem();
			SListElem(const SListElem<TE>& crCopyFrom);
			SListElem(const TE& crElem);
			SListElem(SListElem<TE> *cpPrevious, SListElem<TE> *cpNext);
			TE& operator*();
			const TE& operator*() const;
			TE& operator->();
			const TE& operator->() const;
			SListElem<TE>& operator++();
			SListElem<TE> operator++(int);
			SListElem<TE>& operator =(const SListElem<TE>& crElem);

			static SListElem<TE> CreateDummy();

			const bool operator !=(const SListElem<TE>& crE) const;
		};

		SListElem<T> *m_pFirst;
		SListElem<T> *m_pLastElem;
		SListElem<T> m_LastDummy;

	public:

		typedef SListElem<T> iterator;
		typedef const SListElem<T> const_iterator;

		prtlist();
//		const_iterator begin() const;
		iterator begin();
//		const_iterator end() const;
		iterator end();
		void remove(const T& crElem);
		void push_back(const T& crElem);
	};

	/************************************************************************************************************************************************/

	//!< scalar coefficient type
	template<class TFloatType>
	struct SScalarCoeff_tpl
	{
		typedef SScalarCoeff_tpl<TFloatType> TScalarTypeTempl;
		INSTALL_CLASS_NEW(TScalarTypeTempl)

		SScalarCoeff_tpl();
		SScalarCoeff_tpl(const TFloatType cX);
		const SScalarCoeff_tpl<TFloatType>& operator=(const TFloatType cX);
		operator TFloatType&();
		operator const TFloatType&()const;
		const TFloatType& operator[](const size_t cIndex)const;
		TFloatType& operator[](const size_t cIndex);
		template <class TCopyFloatType>
		SScalarCoeff_tpl<TFloatType>& operator=(const SScalarCoeff_tpl<TCopyFloatType>& crCopyFrom);
		static const uint8 Components(){return 1;}	//!< needed for serialization
		typedef TFloatType TComponentType;

		TFloatType m_Value;	//!< the encapsulated value
	};

	typedef struct SScalarCoeff_tpl<float> TScalarCoeff;

/************************************************************************************************************************************************/

	//!< rgb coefficient type
	template<class TFloatType>
	struct SRGBCoeff_tpl : public Vec3_tpl<TFloatType>
	{
		typedef SRGBCoeff_tpl<TFloatType> TRGBTypeTempl;
		INSTALL_CLASS_NEW(TRGBTypeTempl)

		SRGBCoeff_tpl();
		SRGBCoeff_tpl(const TFloatType cX, const TFloatType cY, const TFloatType cZ);
		SRGBCoeff_tpl(const Vec3_tpl<TFloatType>& rCopyFrom);

		const SRGBCoeff_tpl& operator *=(const SRGBCoeff_tpl& rSource);

		const SRGBCoeff_tpl& operator *=(const TFloatType cScalar);

		template <class TOtherFloatType>
		const SRGBCoeff_tpl& operator =(const Vec3_tpl<TOtherFloatType>& rCopyFrom);

		template<class TOtherFloatType>
		operator SRGBCoeff_tpl<TOtherFloatType>()const;

		static const uint8 Components(){return 3;}	//!< needed for serialization
		typedef TFloatType TComponentType;
	};

	typedef struct SRGBCoeff_tpl<double> TRGBCoeffD;
	typedef struct SRGBCoeff_tpl<float> TRGBCoeffF;

	template<class TFloatType>
	const SRGBCoeff_tpl<TFloatType> operator *(const SRGBCoeff_tpl<TFloatType>& r0, const SRGBCoeff_tpl<TFloatType>& r1);

	template<class TFloatType>
	const SRGBCoeff_tpl<TFloatType> operator +(const SRGBCoeff_tpl<TFloatType>& r0, const SRGBCoeff_tpl<TFloatType>& r1);

	template<class TFloatType>
	const SRGBCoeff_tpl<TFloatType> operator *(const SRGBCoeff_tpl<TFloatType>& r0, const TFloatType cScalar);

/************************************************************************************************************************************************/

	//!< polar coordinate set
	//!< theta is in the range [0...g_cPi] (altitude)
	//!< phi is in the range [0...2 g_cPi] (azimuth)
	template<class TFloatType>
	struct SPolarCoord_tpl
	{
		TFloatType theta;		//!< theta angle
		TFloatType phi;			//!< phi angle
		SPolarCoord_tpl();
		SPolarCoord_tpl(const TFloatType cTheta, const TFloatType cPhi);
		template<class TOtherFloatType>
		operator SPolarCoord_tpl<TOtherFloatType>()const;
	};

	typedef SPolarCoord_tpl<double> TPolarCoord;

/************************************************************************************************************************************************/

	//!< cartesian coordinate set
	template<class TFloatType>
	struct SCartesianCoord_tpl : public Vec3_tpl<TFloatType>
	{
		SCartesianCoord_tpl();
		SCartesianCoord_tpl(const TFloatType cX, const TFloatType cY, const TFloatType cZ);
		template<class F>
		SCartesianCoord_tpl(const SCartesianCoord_tpl<F>& rcCopyFrom);
		const SCartesianCoord_tpl<TFloatType> operator-()const;
		template<class F>
		const SCartesianCoord_tpl<TFloatType>& operator =(const Vec3_tpl<F>& rcAssignFrom);
		template<class TOtherFloatType>
		operator SCartesianCoord_tpl<TOtherFloatType>()const;
	};

	typedef SCartesianCoord_tpl<double> TCartesianCoord;

	//!< converts a polar coordinate into a cartesian
	//!< 0,0,1 means origin with z pointing upwards
	//!< NOTE: output is in SH coordinate system, engine and max negates x and y
	template<class TFloatType>
	void ConvertToCartesian(SCartesianCoord_tpl<TFloatType>& rCartesian, const SPolarCoord_tpl<TFloatType>& crPolar);

	//!< converts a unit sphere cartesian coordinate into a polar
	//!< theta is mapped into the range [0...g_cPi] (altitude)
	//!< phi is into the range [0...2 g_cPi] (azimuth)
	//!< NOTE: input is suppose to be in SH coordinate system, engine and max negates x and y
	template<class TFloatType>
	void ConvertUnitToPolar(SPolarCoord_tpl<TFloatType>& rPolar, const SCartesianCoord_tpl<TFloatType>& crCartesian);

	//!< converts a common sphere cartesian coordinate into a polar
	//!< theta is mapped into the range [0...g_cPi] (altitude)
	//!< phi is into the range [0...2 g_cPi] (azimuth)
	//!< NOTE: input is suppose to be in SH coordinate system, engine and max negates x and y
	template<class TFloatType>
	void ConvertToPolar(SPolarCoord_tpl<TFloatType>& rPolar, const SCartesianCoord_tpl<TFloatType>& crCartesian);

/************************************************************************************************************************************************/

	//!< calculates the rotation matrix around (0,0,0) to rotate from rFrom to (0,0,1)
	template<class TFloatType>
	void CalcRotMatrixToSphereOrigin(Matrix33_tpl<TFloatType>& rRotMatrix, const TCartesianCoord& rFrom);

	template<class TFloatType>
	void CalcRotMatrixToSphereOrigin(Matrix33_tpl<TFloatType>& rRotMatrix, const TPolarCoord& rFrom);

	//!< calculates the rotation matrix around (0,0,0) to rotate from (0,0,1) to rTo
	template<class TFloatType>
	void CalcRotMatrixFromSphereOrigin(Matrix33_tpl<TFloatType>& rRotMatrix, const TCartesianCoord& rTo);

	template<class TFloatType>
	void CalcRotMatrixFromSphereOrigin(Matrix33_tpl<TFloatType>& rRotMatrix, const TPolarCoord& rTo);

	/************************************************************************************************************************************************/

	typedef enum ECoeffChoice {ALL_COEFFS, NO_COEFF_Y2m2, NO_COEFF_Y22} ECoeffChoice;	//!< enum describing coeff choice used for SH-lookup

	/************************************************************************************************************************************************/

	template<typename CoeffTypeList>
	class CSample_tpl;

	//!< list of (scalar/RGB) coefficients
	template< typename CoeffType >
	struct SCoeffList_tpl
	{
		typedef CoeffType TComponentType;
		typedef typename CoeffType::TComponentType TComponentFloatType;

		explicit SCoeffList_tpl(const SDescriptor& crDescriptor);
		explicit SCoeffList_tpl(const uint32 cCoefficients = gs_cDefaultCoeffCount);
		template< typename CoeffTypeFrom >
		SCoeffList_tpl(const SCoeffList_tpl<CoeffTypeFrom>& rCopyFrom);
		template< typename CoeffTypeFrom >
		SCoeffList_tpl& operator=(const SCoeffList_tpl<CoeffTypeFrom>& rCopyFrom);
		void ResetToZero();
		void ReSize(const SDescriptor& crDescriptor);
		CoeffType& operator () (const int32 cL, const int32 cM);	//!< access operators for a particular band index
		const CoeffType& operator () (const int32 cL, const int32 cM) const;
		const SCoeffList_tpl<CoeffType>& operator +=(const SCoeffList_tpl<CoeffType>& rToAdd);
		const SCoeffList_tpl<CoeffType>& operator *=(const double cScalar);
		const CoeffType* const GetCoeffs() const;
		const CoeffType PerformLookup(const CSample_tpl<SCoeffList_tpl<TScalarCoeff> >& crSample, const ECoeffChoice cCoeffChoice = ALL_COEFFS) const;
		const size_t size()const;		//!< access operator to make it act like a vector
		CoeffType& operator [](const size_t cIndex);
		const CoeffType operator [](const size_t cIndex)const;

//		std::vector<CoeffType, CSHAllocator<CoeffType> > m_Coeffs;			//!< coefficients set corresponding to the bands
		prtvector<CoeffType> m_Coeffs;			//!< coefficients set corresponding to the bands
		~SCoeffList_tpl();
	};

	template<typename CoeffType>
	const SCoeffList_tpl<CoeffType> operator +(const SCoeffList_tpl<CoeffType>& r0, const SCoeffList_tpl<CoeffType>& r1);

	template<typename CoeffType>
	const SCoeffList_tpl<CoeffType> operator *(const SCoeffList_tpl<CoeffType>& r0, const SCoeffList_tpl<CoeffType>& r1);

	template<typename CoeffType>
	const SCoeffList_tpl<CoeffType> operator *(const SCoeffList_tpl<CoeffType>& r0, const double cScalar);

/************************************************************************************************************************************************/

	//!< sample on sphere containing all required data for one coefficient set and the set itself
	template<typename CoeffTypeList>
	class CSample_tpl
	{
	public:
		typedef CoeffTypeList TComponentType;

		//!< constructor for one sample on a sphere
		CSample_tpl(const SDescriptor& crDescriptor, const TPolarCoord& crPolarCoord, const TSampleHandle cHandle = 0);
		CSample_tpl();
		CSample_tpl(const CSample_tpl<CoeffTypeList>& crCopyFrom);	//!< copy ctor, sets handle to 0 (be careful using it)
		void Validate()const;
		const uint8 NumberOfCoeffs()const;									//!< retrieves the number of coefficients
		const TCartesianCoord& GetCartesianPos()const;			//!< retrieves the cartesian position on sphere
		const TPolarCoord& GetPolarPos()const;							//!< retrieves the polar position on sphere
		const CoeffTypeList& Coeffs()const;									//!< retrieves the coefficients list
		CoeffTypeList& Coeffs();														//!< retrieves the coefficients list
		void SetHandle(const TSampleHandle cHandle);				//!< sets a new Handle
		const TSampleHandle GetHandle()const;								//!< sets a new Handle

	protected:
#if defined(_DEBUG)
		bool m_Constructed;
#endif
		TCartesianCoord	m_CartCoord;		//!< cartesian coordinate
		TPolarCoord			m_PolarCoord;		//!< corresponding polar coordinate
		CoeffTypeList		m_Coeffs;				//!< coefficients for this sample, depending on descriptor
		TSampleHandle		m_Handle;				//!< for indexing and identifying the samples
	};

	typedef CSample_tpl<NSH::SCoeffList_tpl<TScalarCoeff> > TSample;//!< scalar sample type

/************************************************************************************************************************************************/

	//!< return value for ray caster telling which triangle is the closest one intersecting ray
	struct SRayResult
	{
		const CSimpleIndexedMesh* pMesh;		//!< mesh triangle belongs to
		int32 faceID;												//!< triangle face id within pMesh
		Vec3 baryCoord;											//!< barycentric coordinate of hit
		SRayResult() : pMesh(NULL), faceID(-1), baryCoord(0.f,0.f,0.f){}
	};

/************************************************************************************************************************************************/

	typedef std::pair<uint32, uint32> TUint32Pair;
	typedef std::pair<float, float> TFloatPair;
	typedef std::vector<CSimpleIndexedMesh*, CSHAllocator<CSimpleIndexedMesh*> > TGeomVec;
	typedef std::vector<CSample_tpl<SCoeffList_tpl<TScalarCoeff> >, CSHAllocator<CSample_tpl<SCoeffList_tpl<TScalarCoeff> > > > TSampleVec;		//!< sample vector for primitive SH samples
	typedef struct SRGBCoeff_tpl<TScalarCoeff::TComponentType> TRGBCoeffType;//typedef it to the same as the scalar coefficients are
	typedef std::set<uint32, std::less<uint32>, CSHAllocator<uint32> > TVertexIndexSet; //used for recording vertex indices
	typedef std::vector<uint32, CSHAllocator<uint32> > TUint32Vec;
	typedef std::vector<Vec3, CSHAllocator<Vec3> > TVec3Vec;
	typedef std::vector<SCoeffList_tpl<TRGBCoeffType>, CSHAllocator<SCoeffList_tpl<TRGBCoeffType> > > TRGBCoeffVec;
	typedef std::vector<TRGBCoeffVec, CSHAllocator<TRGBCoeffVec> > TRGBCoeffVecVec;
	typedef prtvector<SCoeffList_tpl<TScalarCoeff> > TScalarCoeffVec;
	typedef prtvector<TScalarCoeffVec> TScalarCoeffVecVec;
	typedef prtvector<TFloatPair> TFloatPairVec;//used outside
	typedef prtvector<TFloatPairVec> TFloatPairVecVec;
	typedef std::vector<bool, CSHAllocator<bool> > TBoolVec;
	typedef std::vector<TVec, CSHAllocator<TVec> > TVecDVec;
	typedef std::vector<float*, CSHAllocator<float*> > TFloatPtrVec;
	typedef std::vector<SRayResult, CSHAllocator<SRayResult> > TRayResultVec;


	//!< describes the compression for each material(set of decompression values and so on)
	struct SCompressionInfo
	{
		TFloatPairVec compressionValue;						//!< compression scale values and center offset for direct coeffs, individual for each mesh and material
		bool isCompressed;												//!< indicates the compression
		SCompressionInfo() : isCompressed(false), compressionValue((const uint32)0){}
	};

	typedef prtvector<NSH::SCompressionInfo> TCompressionInfo;
}


#include "PRTTypes.inl"

#endif // CRYINCLUDE_TOOLS_PRT_PRTTYPES_H
