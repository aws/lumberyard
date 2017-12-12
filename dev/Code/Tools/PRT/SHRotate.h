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

#ifndef CRYINCLUDE_TOOLS_PRT_SHROTATE_H
#define CRYINCLUDE_TOOLS_PRT_SHROTATE_H
#pragma once

#include "PRTTypes.h"
#if defined(USE_D3DX)
	#include<d3dx9math.h>
	#pragma comment(lib,"d3dx9.lib")
	#pragma comment(lib,"d3d9.lib ")
#endif


namespace NSH
{
	//!< rotates coefficients without a particular matrix, fastest method using DirectX
	template<class TCoeffType>
	void RotateCoeffs(const Matrix33_tpl<float>& crRotMat, const SCoeffList_tpl<TCoeffType>& rSource, SCoeffList_tpl<TCoeffType>& rDest);

	//!< a SH rotation matrix
	//!< compress matrix because it is block diagonal sparse 
	template<class F, bool Compress = false>
	class CRotateMatrix_tpl
	{
	public:

	static const uint8 GetCompressedElemCount(const uint8 cBands)
	{
		switch(cBands)
		{
			case 0:
				return 0;
			case 1:
				return 0;
			case 2:
				return (3*3);
			case 3:
				return (3*3+5*5);
			case 4:
				return (3*3+5*5+7*7);
			case 5:
				return (3*3+5*5+7*7+9*9);
			default:
			{
				//implement other bands
				assert(false);//larger than gs_cSupportedMaxBands
				int i = cBands;
				int x = 1;
				uint32 res = 1;
				while(--i)
				{
					x = x+2;
					res += x*x;
				}
				return (uint8)(res-1);
			}
		}
	}

		//!< constructor taking sh descriptor
		CRotateMatrix_tpl(const SDescriptor& rSHDesc);

		//!< constructor taking number of coefficients
		CRotateMatrix_tpl(const uint8 cCoefficients);

		//!< constructor taking dimension info and sh descriptor
		CRotateMatrix_tpl(const SDescriptor& rSHDesc, const uint8 cDimRow, const uint8 cDimColumn);

		//!< resets matrix to 0
		void Reset();

		~CRotateMatrix_tpl()
		{
			static CSHAllocator<F> sAllocator;
			sAllocator.delete_mem_array(m_pData, sizeof(F) * m_DimRow * m_DimColumn);
			m_pData = NULL;
		}

		//!< turns sh matrix into a rotation matrix according to crAngles
		void SetSHRotation(const Ang3_tpl<F>& crAngles);

		//!< turns sh matrix into a rotation matrix according to a 3x3 rotation matrix
		void SetSHRotation(const Matrix33_tpl<F>& crRotMat);

		//!< SH rotation  matrices are block diagonal sparse because bands to not interact due to orthogonality
		//!< a certain band index determines the lookup into the respective block responsible for rotating that band and shift it by (a,b)
		inline F& operator () (const int32 cL, const int32 cA, const int32 cB)
		{
			return Value(cL, cA, cB);		
		}
		inline const F operator () (const int32 cL, const int32 cA, const int32 cB) const
		{
			return Value(cL, cA, cB);
		}
		
		//!< matrix row/column access operator
		inline const F operator ()(const size_t cRowIndex, const size_t cColumnIndex) const;
		inline F& operator ()(const size_t cRowIndex, const size_t cColumnIndex); 

		//!< transforms a coefficient set by looping through each band
		//!< optimize this, still block diagonal sparse, lots of redundant calls
		template<class CoeffType>
		void Transform(const SCoeffList_tpl<CoeffType>& rSource, SCoeffList_tpl<CoeffType>& rDest, const int8 cBands = -1) const;
	
		//!< debug output
		void DebugOutput()const;

		//!< retrieves the compressed info
		const bool IsCompressed(){return Compress;}

		//!< copy operator
		template<bool SecondCompressed>
		const CRotateMatrix_tpl<F, Compress>& operator=(const CRotateMatrix_tpl<F, SecondCompressed>& rM);

		//!< copy constructor
		template<bool SecondCompressed>
		CRotateMatrix_tpl(const CRotateMatrix_tpl<F, SecondCompressed>& rM);

#if !defined(_DEBUG)
		//!< bad function, but for the sake of optimization, only available in release mode to avoid any explicit use in all places
		const F* Data()const{return m_pData;}
#endif

	protected:
		F	*m_pData;										//!< matrix elements
		uint8 m_DimRow;								//!< row dimension
		uint8 m_DimColumn;						//!< column dimension
		uint8 m_Bands;								//!< number of bands
		uint8 m_Coefficients;					//!< number of corresponding coefficients

		static F m_sDummy;							//!< dummy returned in Value if an element is accessed which does not exist in a block diagonal sparse matrix

		template<bool SecondCompressed>
		void ReAllocateAndPartCopy(const size_t cSize, const CRotateMatrix_tpl<F, SecondCompressed>& rM);		//!< helper function for assignment operator and copy constructor

		const F Value(const int32 cL, const int32 cA, const int32 cB) const;
		F& Value(const int32 cL, const int32 cA, const int32 cB);

		template<class F1, bool Compress1>
		friend class CRotateMatrix_tpl;
	};

	typedef CRotateMatrix_tpl<float,false> TRotateMatrix;

/************************************************************************************************************************************************/

#pragma warning (disable : 4512) 
	//!< covers some helper function for the SH rotation matrix setup according to Ivanic
	//!< Implementation Reference:
	//!< Rotation Matrices for Real Spherical Harmonics. Direct Determination by Recursion
	//!< Joseph Ivanic and Klaus Ruedenberg
	//!< J. Phys. Chem. 1996, 100, 6342-5347
	namespace NRecursionHelper
	{
		//Kronecker Delta
		template<class F>
			inline const F Delta(const int m, const int n)
		{
			return (F)(m == n ? 1 : 0);
		}

		template<class F>
			void UVW(const int l, const int m, const int n, F& u, F& v, F& w);

		template<class F, bool Compress>
			const F P(const int i, const int l, const int a, const int b, const CRotateMatrix_tpl<F,Compress>& crSHRotMat);

		template<class F, bool Compress>
			const F V(const int l, const int m, const int n, const CRotateMatrix_tpl<F,Compress>& crSHRotMat);

		template<class F, bool Compress>
			const F W(const int l, const int m, const int n, const CRotateMatrix_tpl<F,Compress>& crSHRotMat);

		template<class F, bool Compress>
			const F M(const int l, const int m, const int n, const CRotateMatrix_tpl<F,Compress>& crSHRotMat);

		//gathering helper functions for optimized version for 3 bands
		namespace NBand3Optimization
		{
			template<class F, bool Compress, int i, int a, int b>
				const F P3(const CRotateMatrix_tpl<F,Compress>& crSHRotMat);

			template<class F, bool Compress, int n>
				const F M3m2(const CRotateMatrix_tpl<F,Compress>& crSHRotMat);

			template<class F, bool Compress, int n>
				const F M3m1(const CRotateMatrix_tpl<F,Compress>& crSHRotMat);

			template<class F, bool Compress, int n>
				const F M3mMinus1(const CRotateMatrix_tpl<F,Compress>& crSHRotMat);

			template<class F, bool Compress, int n>
				const F M3mMinus2(const CRotateMatrix_tpl<F,Compress>& crSHRotMat);

			template<class F, bool Compress, int n>
				const F M3m0(const CRotateMatrix_tpl<F,Compress>& crSHRotMat);
		}
	}//namespace NRecursionHelper
}

#include "SHRotate.inl"	//include implementation

#endif // CRYINCLUDE_TOOLS_PRT_SHROTATE_H
