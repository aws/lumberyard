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

#pragma warning (disable : 4127) 


#include "CompressedLookupTable.h"

static const NSH::CCompressedLookupTable_tpl<NSH::gs_cSupportedMaxBands> scLookupTable;
template<class F, bool Compress>
F NSH::CRotateMatrix_tpl<F, Compress>::m_sDummy;

#if defined(USE_D3DX)
	//template specialization for scalar type, uses directx for the sake of max speed
	template <>
	inline void NSH::RotateCoeffs(const Matrix33_tpl<float>& crRotMat, const NSH::SCoeffList_tpl<NSH::TScalarCoeff>& rSource, NSH::SCoeffList_tpl<NSH::TScalarCoeff>& rDest)
	{
		const uint8 cBands = (uint8)sqrtf((float)rSource.size());
		assert(1 < cBands && cBands <= gs_cSupportedMaxBands);

		D3DXMATRIX matD3D;
		matD3D._11 = (float)crRotMat(0,0);
		matD3D._12 = (float)crRotMat(1,0);
		matD3D._13 = (float)crRotMat(2,0);
		matD3D._21 = (float)crRotMat(0,1);
		matD3D._22 = (float)crRotMat(1,1);
		matD3D._23 = (float)crRotMat(2,1);
		matD3D._31 = (float)crRotMat(0,2);
		matD3D._32 = (float)crRotMat(1,2);
		matD3D._33 = (float)crRotMat(2,2);

		float inCoeffs[gs_cSupportedMaxBands * gs_cSupportedMaxBands];
		float outCoeffs[gs_cSupportedMaxBands * gs_cSupportedMaxBands];
		for(int8 i=0; i<cBands*cBands;++i)
			inCoeffs[i] = (float)rSource[i];
		D3DXSHRotate(outCoeffs, cBands, &matD3D, inCoeffs);
		for(int8 i=0; i<cBands*cBands;++i)
			rDest[i] = (NSH::TScalarCoeff::TComponentType)outCoeffs[i];
	}

	//template specialization for rgb type, do not use directx
	template <>
	inline void NSH::RotateCoeffs(const Matrix33_tpl<float>& crRotMat, const NSH::SCoeffList_tpl<NSH::SRGBCoeff_tpl<TScalarCoeff::TComponentType> >& rSource, NSH::SCoeffList_tpl<NSH::SRGBCoeff_tpl<NSH::TScalarCoeff::TComponentType> >& rDest)
	{
		if(rSource.size() == 9)
		{
			//special since 3 bands are the most common case
			static NSH::CRotateMatrix_tpl<float, true> matRotSH(9);	//avoid reallocation
			matRotSH.SetSHRotation(crRotMat);
			matRotSH.Transform(rSource, rDest);
		}
		else
		{
			NSH::CRotateMatrix_tpl<float, true> matRotSH((uint8)rSource.size());	
			matRotSH.SetSHRotation(crRotMat);
			matRotSH.Transform(rSource, rDest);
		}
	}
#else
	//template specialization for rgb type, do not use directx
	template<class TCoeffType>
	inline void NSH::RotateCoeffs(const Matrix33_tpl<float>& crRotMat, const NSH::SCoeffList_tpl<TCoeffType>& rSource, NSH::SCoeffList_tpl<TCoeffType>& rDest)
	{
		if(rSource.size() == 9)
		{
			//special since 3 bands are the most common case
			static NSH::CRotateMatrix_tpl<float, true> matRotSH(9);	//avoid reallocation
			matRotSH.SetSHRotation(crRotMat);
			matRotSH.Transform(rSource, rDest);
		}
		else
		{
			NSH::CRotateMatrix_tpl<float, true> matRotSH((const uint8)rSource.size());	
			matRotSH.SetSHRotation(crRotMat);
			matRotSH.Transform(rSource, rDest);
		}
	}
#endif

//!< constructor taking sh descriptor
template<class F, bool Compress>
inline NSH::CRotateMatrix_tpl<F, Compress>::CRotateMatrix_tpl(const SDescriptor& rSHDesc) : m_DimRow(rSHDesc.Bands * rSHDesc.Bands), m_DimColumn(rSHDesc.Bands * rSHDesc.Bands), m_Bands(rSHDesc.Bands), m_Coefficients(rSHDesc.Coefficients)
{
	assert(1 < rSHDesc.Bands && rSHDesc.Bands < gs_cSupportedMaxBands);
	static CSHAllocator<F> sAllocator;
	if(Compress)
	{
		m_pData = (F*)(sAllocator.new_mem_array(sizeof(F) * GetCompressedElemCount(m_Bands)));
		assert(m_pData);
	}
	else
	{
		m_pData = (F*)(sAllocator.new_mem_array(sizeof(F) * m_DimRow * m_DimColumn));
		assert(m_pData);
	}
	assert(m_pData);
	Reset();
}

//!< constructor taking dimension info and sh descriptor
template<class F, bool Compress>
inline NSH::CRotateMatrix_tpl<F, Compress>::CRotateMatrix_tpl(const SDescriptor& rSHDesc, const uint8 cDimRow, const uint8 cDimColumn) : m_DimRow(cDimRow), m_DimColumn(cDimColumn), m_Bands(rSHDesc.Bands), m_Coefficients(rSHDesc.Coefficients)
{
	assert(1 < rSHDesc.Bands && rSHDesc.Bands < gs_cSupportedMaxBands);
	assert(cDimRow < gs_cSupportedMaxBands * gs_cSupportedMaxBands);
	assert(cDimColumn < gs_cSupportedMaxBands * gs_cSupportedMaxBands);
	static CSHAllocator<F> sAllocator;
	if(Compress)
	{
		m_pData = (F*)(sAllocator.new_mem_array(sizeof(F) * GetCompressedElemCount(m_Bands)));
		assert(m_pData);
	}
	else
	{
		m_pData = (F*)(sAllocator.new_mem_array(sizeof(F) * m_DimRow * m_DimColumn));
		assert(m_pData);
	}
	Reset();
}

template<class F, bool Compress>
inline NSH::CRotateMatrix_tpl<F, Compress>::CRotateMatrix_tpl(const uint8 cCoefficients) : m_DimRow(cCoefficients), m_DimColumn(cCoefficients), m_Coefficients(cCoefficients)
{
	const uint8 bands = (uint8)sqrtf((float)cCoefficients);
	assert(bands * bands == cCoefficients);
	assert(1 < bands && bands < gs_cSupportedMaxBands);
	m_Bands = bands;
	static CSHAllocator<F> sAllocator;
	if(Compress)
	{
		m_pData = (F*)(sAllocator.new_mem_array(sizeof(F) * GetCompressedElemCount(m_Bands)));
		assert(m_pData);
	}
	else
	{
		m_pData = (F*)(sAllocator.new_mem_array(sizeof(F) * m_DimRow * m_DimColumn));
		assert(m_pData);
	}
	Reset();
}

//!< resets matrix to 0
template<class F, bool Compress>
inline void NSH::CRotateMatrix_tpl<F, Compress>::Reset()
{
	memset(m_pData, 0, sizeof(F) * (Compress? GetCompressedElemCount(m_Bands) : m_DimRow * m_DimColumn));
	// set already known 1x1 rotation matrix for band zero
	if(!Compress)
		m_pData[0] = 1.;
}

//!< matrix row/column access operator
template<class F, bool Compress>
inline const F NSH::CRotateMatrix_tpl<F, Compress>::operator ()(const size_t cRowIndex, const size_t cColumnIndex) const
{
	assert(cRowIndex < m_DimRow && cColumnIndex < m_DimColumn);
	if(Compress)
	{
		const int32 index = scLookupTable(cRowIndex, cColumnIndex);
		assert(index < GetCompressedElemCount(m_Bands));
		return (index != -1)?m_pData[index]:(F)0.;
	}
	else
		return m_pData[cRowIndex * m_DimColumn + cColumnIndex];
}

template<class F, bool Compress>
inline F& NSH::CRotateMatrix_tpl<F, Compress>::operator ()(const size_t cRowIndex, const size_t cColumnIndex) 
{
	assert(cRowIndex < m_DimRow && cColumnIndex < m_DimColumn);
	if(Compress)
	{
		const int32 index = scLookupTable(cRowIndex, cColumnIndex);
		assert(index != -1 && index < GetCompressedElemCount(m_Bands));//wrong access for compressed lookup
		return m_pData[index];
	}
	return m_pData[cRowIndex * m_DimColumn + cColumnIndex];
}

template<class F, bool Compress>
inline const F NSH::CRotateMatrix_tpl<F, Compress>::Value(const int32 cL, const int32 cA, const int32 cB) const
{
	const int32 cCentre = (cL + 1) * cL ;
	const int32 cOffset = m_Coefficients * (cCentre + cA) + cCentre + cB;
	assert(cOffset < m_DimRow * m_DimColumn);
	if(Compress)
	{
		const int32 index = scLookupTable.GetMapping(cOffset, m_DimColumn);
		assert(index < GetCompressedElemCount(m_Bands));
		return (index != -1)?m_pData[index]:(F)0.;
	}
	else
		return m_pData[cOffset];
}

template<class F, bool Compress>
inline F& NSH::CRotateMatrix_tpl<F, Compress>::Value(const int32 cL, const int32 cA, const int32 cB)
{
	const int32 cCentre = (cL + 1) * cL ;
	const int32 cOffset = m_Coefficients * (cCentre + cA) + cCentre + cB;
	assert(cOffset < m_DimRow * m_DimColumn);
	if(Compress)
	{
		const int32 index = scLookupTable.GetMapping(cOffset, m_DimColumn);
		assert(index < GetCompressedElemCount(m_Bands));
		return (index != -1)?m_pData[index] : m_sDummy;
	}
	else
	{
		return m_pData[cOffset];
	}
}


template<class F, bool Compress>
void NSH::CRotateMatrix_tpl<F, Compress>::SetSHRotation(const Ang3_tpl<F>& crAngles)
{
	if(m_Bands >= 2)
	{
		Matrix33_tpl<F> matRot;	
		matRot.SetRotationXYZ(crAngles);
		SetSHRotation(matRot);
		return;
	}
}

template<class F, bool Compress>
void NSH::CRotateMatrix_tpl<F, Compress>::SetSHRotation(const Matrix33_tpl<F>& crRotMat)
{
	//looks pretty much complicated, but this only is due to handling of compressed and uncompressed matrices and for the sake of optimization for 3 bands
	if(Compress)
	{
		// the first band is rotated by a permutation of the original matrix
		m_pData[0] = crRotMat(1,1);
		m_pData[1] = -crRotMat(1,2);
		m_pData[2] = crRotMat(1,0);
		m_pData[3] = -crRotMat(2,1);
		m_pData[4] = crRotMat(2,2);
		m_pData[5] = -crRotMat(2,0);
		m_pData[6] = crRotMat(0,1);
		m_pData[7] = -crRotMat(0,2);
		m_pData[8] = crRotMat(0,0);
		if(m_Bands >= 3)
		{
			m_pData[9] = NRecursionHelper::NBand3Optimization::M3mMinus2<F,Compress,-2>(*this);
			m_pData[10] = NRecursionHelper::NBand3Optimization::M3mMinus2<F,Compress,-1>(*this);
			m_pData[11] = NRecursionHelper::NBand3Optimization::M3mMinus2<F,Compress,0>(*this);
			m_pData[12] = NRecursionHelper::NBand3Optimization::M3mMinus2<F,Compress,1>(*this);
			m_pData[13] = NRecursionHelper::NBand3Optimization::M3mMinus2<F,Compress,2>(*this);
			m_pData[14] = NRecursionHelper::NBand3Optimization::M3mMinus1<F,Compress,-2>(*this);
			m_pData[15] = NRecursionHelper::NBand3Optimization::M3mMinus1<F,Compress,-1>(*this);
			m_pData[16] = NRecursionHelper::NBand3Optimization::M3mMinus1<F,Compress,0>(*this);
			m_pData[17] = NRecursionHelper::NBand3Optimization::M3mMinus1<F,Compress,1>(*this);
			m_pData[18] = NRecursionHelper::NBand3Optimization::M3mMinus1<F,Compress,2>(*this);
			m_pData[19] = NRecursionHelper::NBand3Optimization::M3m0<F,Compress,-2>(*this);
			m_pData[20] = NRecursionHelper::NBand3Optimization::M3m0<F,Compress,-1>(*this);
			m_pData[21] = NRecursionHelper::NBand3Optimization::M3m0<F,Compress,0>(*this);
			m_pData[22] = NRecursionHelper::NBand3Optimization::M3m0<F,Compress,1>(*this);
			m_pData[23] = NRecursionHelper::NBand3Optimization::M3m0<F,Compress,2>(*this);
			m_pData[24] = NRecursionHelper::NBand3Optimization::M3m1<F,Compress,-2>(*this);
			m_pData[25] = NRecursionHelper::NBand3Optimization::M3m1<F,Compress,-1>(*this);
			m_pData[26] = NRecursionHelper::NBand3Optimization::M3m1<F,Compress,0>(*this);
			m_pData[27] = NRecursionHelper::NBand3Optimization::M3m1<F,Compress,1>(*this);
			m_pData[28] = NRecursionHelper::NBand3Optimization::M3m1<F,Compress,2>(*this);
			m_pData[29] = NRecursionHelper::NBand3Optimization::M3m2<F,Compress,-2>(*this);
			m_pData[30] = NRecursionHelper::NBand3Optimization::M3m2<F,Compress,-1>(*this);
			m_pData[31] = NRecursionHelper::NBand3Optimization::M3m2<F,Compress,0>(*this);
			m_pData[32] = NRecursionHelper::NBand3Optimization::M3m2<F,Compress,1>(*this);
			m_pData[33] = NRecursionHelper::NBand3Optimization::M3m2<F,Compress,2>(*this);
			unsigned int offset = 34;
			for (int band = 3; band < m_Bands; band++)
			{
				for (int m = -band; m <= band; m++)
					for (int n = -band; n <= band; n++)
						m_pData[offset++] = NRecursionHelper::M<F,Compress>(band, m, n, *this);
			}
		}
		return;
	};
	//optimized version for 3 bands
	if(m_Bands == 3)
	{
		// the first band is rotated by a permutation of the original matrix
		m_pData[10] = crRotMat(1,1);
		m_pData[11] = -crRotMat(1,2);
		m_pData[12] = crRotMat(1,0);
		m_pData[19] = -crRotMat(2,1);
		m_pData[20] = crRotMat(2,2);
		m_pData[21] = -crRotMat(2,0);
		m_pData[28] = crRotMat(0,1);
		m_pData[29] = -crRotMat(0,2);
		m_pData[30] = crRotMat(0,0);
		m_pData[40] = NRecursionHelper::NBand3Optimization::M3mMinus2<F,Compress,-2>(*this);
		m_pData[41] = NRecursionHelper::NBand3Optimization::M3mMinus2<F,Compress,-1>(*this);
		m_pData[42] = NRecursionHelper::NBand3Optimization::M3mMinus2<F,Compress,0>(*this);
		m_pData[43] = NRecursionHelper::NBand3Optimization::M3mMinus2<F,Compress,1>(*this);
		m_pData[44] = NRecursionHelper::NBand3Optimization::M3mMinus2<F,Compress,2>(*this);
		m_pData[49] = NRecursionHelper::NBand3Optimization::M3mMinus1<F,Compress,-2>(*this);
		m_pData[50] = NRecursionHelper::NBand3Optimization::M3mMinus1<F,Compress,-1>(*this);
		m_pData[51] = NRecursionHelper::NBand3Optimization::M3mMinus1<F,Compress,0>(*this);
		m_pData[52] = NRecursionHelper::NBand3Optimization::M3mMinus1<F,Compress,1>(*this);
		m_pData[53] = NRecursionHelper::NBand3Optimization::M3mMinus1<F,Compress,2>(*this);
		m_pData[58] = NRecursionHelper::NBand3Optimization::M3m0<F,Compress,-2>(*this);
		m_pData[59] = NRecursionHelper::NBand3Optimization::M3m0<F,Compress,-1>(*this);
		m_pData[60] = NRecursionHelper::NBand3Optimization::M3m0<F,Compress,0>(*this);
		m_pData[61] = NRecursionHelper::NBand3Optimization::M3m0<F,Compress,1>(*this);
		m_pData[62] = NRecursionHelper::NBand3Optimization::M3m0<F,Compress,2>(*this);
		m_pData[67] = NRecursionHelper::NBand3Optimization::M3m1<F,Compress,-2>(*this);
		m_pData[68] = NRecursionHelper::NBand3Optimization::M3m1<F,Compress,-1>(*this);
		m_pData[69] = NRecursionHelper::NBand3Optimization::M3m1<F,Compress,0>(*this);
		m_pData[70] = NRecursionHelper::NBand3Optimization::M3m1<F,Compress,1>(*this);
		m_pData[71] = NRecursionHelper::NBand3Optimization::M3m1<F,Compress,2>(*this);
		m_pData[76] = NRecursionHelper::NBand3Optimization::M3m2<F,Compress,-2>(*this);
		m_pData[77] = NRecursionHelper::NBand3Optimization::M3m2<F,Compress,-1>(*this);
		m_pData[78] = NRecursionHelper::NBand3Optimization::M3m2<F,Compress,0>(*this);
		m_pData[79] = NRecursionHelper::NBand3Optimization::M3m2<F,Compress,1>(*this);
		m_pData[80] = NRecursionHelper::NBand3Optimization::M3m2<F,Compress,2>(*this);
	}
	else
	{
		// the first band is rotated by a permutation of the original matrix
		uint32 offset = m_Coefficients;
		m_pData[1+offset] = crRotMat(1,1);
		m_pData[2+offset] = -crRotMat(1,2);
		m_pData[3+offset] = crRotMat(1,0);
		offset += m_Coefficients;
		m_pData[1+offset] = -crRotMat(2,1);
		m_pData[2+offset] = crRotMat(2,2);
		m_pData[3+offset] = -crRotMat(2,0);
		offset += m_Coefficients;
		m_pData[1+offset] = crRotMat(0,1);
		m_pData[2+offset] = -crRotMat(0,2);
		m_pData[3+offset] = crRotMat(0,0);
		//now use optimized version for bands 2
		if(m_Bands > 2)
		{
			unsigned int offset = 4 + 4 * m_Coefficients;
			m_pData[offset++] = NRecursionHelper::NBand3Optimization::M3mMinus2<F,Compress,-2>(*this);
			m_pData[offset++] = NRecursionHelper::NBand3Optimization::M3mMinus2<F,Compress,-1>(*this);
			m_pData[offset++] = NRecursionHelper::NBand3Optimization::M3mMinus2<F,Compress,0>(*this);
			m_pData[offset++] = NRecursionHelper::NBand3Optimization::M3mMinus2<F,Compress,1>(*this);
			m_pData[offset] = NRecursionHelper::NBand3Optimization::M3mMinus2<F,Compress,2>(*this);
			offset += m_Coefficients - 4;
			m_pData[offset++] = NRecursionHelper::NBand3Optimization::M3mMinus1<F,Compress,-2>(*this);
			m_pData[offset++] = NRecursionHelper::NBand3Optimization::M3mMinus1<F,Compress,-1>(*this);
			m_pData[offset++] = NRecursionHelper::NBand3Optimization::M3mMinus1<F,Compress,0>(*this);
			m_pData[offset++] = NRecursionHelper::NBand3Optimization::M3mMinus1<F,Compress,1>(*this);
			m_pData[offset] = NRecursionHelper::NBand3Optimization::M3mMinus1<F,Compress,2>(*this);
			offset += m_Coefficients - 4;
			m_pData[offset++] = NRecursionHelper::NBand3Optimization::M3m0<F,Compress,-2>(*this);
			m_pData[offset++] = NRecursionHelper::NBand3Optimization::M3m0<F,Compress,-1>(*this);
			m_pData[offset++] = NRecursionHelper::NBand3Optimization::M3m0<F,Compress,0>(*this);
			m_pData[offset++] = NRecursionHelper::NBand3Optimization::M3m0<F,Compress,1>(*this);
			m_pData[offset] = NRecursionHelper::NBand3Optimization::M3m0<F,Compress,2>(*this);
			offset += m_Coefficients - 4;
			m_pData[offset++] = NRecursionHelper::NBand3Optimization::M3m1<F,Compress,-2>(*this);
			m_pData[offset++] = NRecursionHelper::NBand3Optimization::M3m1<F,Compress,-1>(*this);
			m_pData[offset++] = NRecursionHelper::NBand3Optimization::M3m1<F,Compress,0>(*this);
			m_pData[offset++] = NRecursionHelper::NBand3Optimization::M3m1<F,Compress,1>(*this);
			m_pData[offset] = NRecursionHelper::NBand3Optimization::M3m1<F,Compress,2>(*this);
			offset += m_Coefficients - 4;
			m_pData[offset++] = NRecursionHelper::NBand3Optimization::M3m2<F,Compress,-2>(*this);
			m_pData[offset++] = NRecursionHelper::NBand3Optimization::M3m2<F,Compress,-1>(*this);
			m_pData[offset++] = NRecursionHelper::NBand3Optimization::M3m2<F,Compress,0>(*this);
			m_pData[offset++] = NRecursionHelper::NBand3Optimization::M3m2<F,Compress,1>(*this);
			m_pData[offset] = NRecursionHelper::NBand3Optimization::M3m2<F,Compress,2>(*this);

			// calculate each block of the rotation matrix for each band
			for (int band = 3; band < m_Bands; band++)
			{
				for (int m = -band; m <= band; m++)
					for (int n = -band; n <= band; n++)
						Value(band, m, n) = NRecursionHelper::M<F,Compress>(band, m, n, *this);
			}
		}
	}
}

template<class F, bool Compress>
template<class CoeffType>
void NSH::CRotateMatrix_tpl<F,Compress>::Transform(const SCoeffList_tpl<CoeffType>& rSource, SCoeffList_tpl<CoeffType>& rDest, const int8 cBands) const
{
	const int32 cMaxBand = (cBands != -1)? cBands : m_Bands;
	assert(1 < cMaxBand && cMaxBand <= m_Bands);
	rDest[0] = rSource[0];
	if(Compress)
	{
		//fast method only using the non zero elements
		for (uint8 i = 1; i < cMaxBand; ++i)
		{
			const uint8 cCompressedElemCount = GetCompressedElemCount(i);
			for (int32 m = 0; m < (2 * i + 1); ++m)
			{
				CoeffType& rgb = rDest[i * i + m];
				rgb = rSource[0 + (i * i)] * m_pData[cCompressedElemCount + (2*i+1) * m + 0];
				for (int32 n = 1; n < (2 * i + 1); ++n)
				{
					rgb += rSource[n + (i * i)] * m_pData[cCompressedElemCount + (2*i+1) * m + n];
				}
			}
		}
	}
	else
	{
		if(m_Bands == 3 && cMaxBand == 3)//optimization for 3 bands
		{
			const CoeffType& rSource1 = rSource[1];
			const CoeffType& rSource2 = rSource[2];
			const CoeffType& rSource3 = rSource[3];
			const CoeffType& rSource4 = rSource[4];
			rDest[1] = rSource1 * Value(1, -1, -1) + rSource2 * Value(1, -1, 0) + rSource3 * Value(1, -1, 1);
			rDest[2] = rSource1 * Value(1, 0, -1) + rSource2 * Value(1, 0, 0) + rSource3 * Value(1, 0, 1);
			rDest[3] = rSource1 * Value(1, 1, -1) + rSource2 * Value(1, 1, 0) + rSource3 * Value(1, 1, 1);
			for (int32 mo = -2; mo <= 2; mo++)
			{
				CoeffType& rgb = rDest(2, mo);
				rgb = rSource4 * Value(2, mo, -2);
				for (int32 mi = -1; mi <= 2; mi++)
					rgb += rSource(2, mi) * Value(2, mo, mi);
			}
			return;
		}
	
		const CoeffType& rSource1 = rSource[1];
		const CoeffType& rSource2 = rSource[2];
		const CoeffType& rSource3 = rSource[3];
		rDest[1] = rSource1 * Value(1, -1, -1) + rSource2 * Value(1, -1, 0) + rSource3 * Value(1, -1, 1);
		rDest[2] = rSource1 * Value(1, 0, -1) + rSource2 * Value(1, 0, 0) + rSource3 * Value(1, 0, 1);
		rDest[3] = rSource1 * Value(1, 1, -1) + rSource2 * Value(1, 1, 0) + rSource3 * Value(1, 1, 1);
		uint32 offset = 4;
		for (int32 l = 2; l < cMaxBand; l++) 
		{
			for (int32 mo = -l; mo <= l; mo++)
			{
				CoeffType& rgb = rDest[offset++];
				rgb = rSource(l, -l) * Value(l, mo, -l);
				for (int32 mi = -l+1; mi <= l; mi++)
					rgb += rSource(l, mi) * Value(l, mo, mi);
			}
		}
	}
}
 
template<class F>
void NSH::NRecursionHelper::UVW(const int l, const int m, const int n, F& u, F& v, F& w)
{
	// Pre-calculate simple reusable terms
	const F cDelta = Delta<F>(m, 0);
	const int cAbsM = abs(m);
	const F denom = (abs(n) == l)? (F)((2 * l) * (2 * l - 1)) : (F)((l + n) * (l - n));
	u = Sqrt((l + m) * (l - m) / denom);
	v = 0.5f * Sqrt((1 + cDelta) * (l + cAbsM - 1) * (l + cAbsM) / denom) * (1 - 2 * cDelta);
	w = -0.5f * Sqrt((l - cAbsM - 1) * (l - cAbsM) / denom) * (1 - cDelta);
}

template<class F, bool Compress>
const F NSH::NRecursionHelper::P(const int i, const int l, const int a, const int b, const NSH::CRotateMatrix_tpl<F,Compress>& crSHRotMat)
{
	const F ri1	= crSHRotMat(1, i, 1);
	const F rim1 = crSHRotMat(1, i, -1);
	const F ri0	= crSHRotMat(1, i, 0);

	if (b == -l)
		return (ri1 * crSHRotMat(l - 1, a, -l + 1) + rim1 * crSHRotMat(l - 1, a, l - 1));

	else if (b == l)
		return (ri1 * crSHRotMat(l - 1, a, l - 1) - rim1 * crSHRotMat(l - 1, a, -l + 1));

	else // |b|<l
		return (ri0 * crSHRotMat(l - 1, a, b));
}

template<class F, bool Compress>
const F NSH::NRecursionHelper::V(const int l, const int m, const int n, const NSH::CRotateMatrix_tpl<F,Compress>& crSHRotMat)
{
	if (m == 0)
	{
		const F p0 = P<F,Compress>(1, l, 1, n, crSHRotMat);
		const F p1 = P<F,Compress>(-1, l, -1, n, crSHRotMat);
		return (p0 + p1);
	}

	else if (m > 0)
	{
		const F d = Delta<F>(m, 1);
		const F p0 = P<F,Compress>(1, l, m - 1, n, crSHRotMat);
		const F p1 = P<F,Compress>(-1, l, -m + 1, n, crSHRotMat);
		return (p0 * Sqrt(1 + d) - p1 * (1 - d));
	}

	else // m < 0
	{
		const F d = Delta<F>(m, -1);
		const F p0 = P<F,Compress>(1, l, m + 1, n, crSHRotMat);
		const F p1 = P<F,Compress>(-1, l, -m - 1, n, crSHRotMat);
		return (p0 * (1 - d) + p1 * Sqrt(1 + d));
	}
}

template<class F, bool Compress>
const F NSH::NRecursionHelper::W(const int l, const int m, const int n, const NSH::CRotateMatrix_tpl<F,Compress>& crSHRotMat)
{
	if (m == 0)
	{
		// never gets called as kd=0
		assert(false);
		return (0);
	}

	else if (m > 0)
	{
		const F p0 = P<F,Compress>(1, l, m + 1, n, crSHRotMat);
		const F p1 = P<F,Compress>(-1, l, -m - 1, n, crSHRotMat);
		return (p0 + p1);
	}

	else // m < 0
	{
		const F p0 = P<F,Compress>(1, l, m - 1, n, crSHRotMat);
		const F p1 = P<F,Compress>(-1, l, -m + 1, n, crSHRotMat);
		return (p0 - p1);
	}
}

/******************* START band 3 optimization **********************************************************************************************************/

template<class F, bool Compress, int i, int a, int b>
const F NSH::NRecursionHelper::NBand3Optimization::P3(const CRotateMatrix_tpl<F,Compress>& crSHRotMat)
{
#if defined(_DEBUG)//use normal style with asserts for debug mode
	const F ri1	= crSHRotMat(1, i, 1);
	const F rim1 = crSHRotMat(1, i, -1);
	const F ri0	= crSHRotMat(1, i, 0);

	if (b == -2)
		return (ri1 * crSHRotMat(1, a, -1) + rim1 * crSHRotMat(1, a, 1));
	else if (b == 2)
		return (ri1 * crSHRotMat(1, a, 1) - rim1 * crSHRotMat(1, a, -1));
	else 
		return (ri0 * crSHRotMat(1, a, b));
#else
	const F* cpMatrixData = crSHRotMat.Data();//evil but for the sake of optimization
	if(Compress)
	{
		const F ri1		= cpMatrixData[scLookupTable.GetMapping(9 * (2 + i) + 3, 9)];
		const F rim1	= cpMatrixData[scLookupTable.GetMapping(9 * (2 + i) + 1, 9)];
		const F ri0		= cpMatrixData[scLookupTable.GetMapping(9 * (2 + i) + 2, 9)];
		if (b == -2)
			return (ri1 * cpMatrixData[scLookupTable.GetMapping(9 * (2 + a) + 1, 9)] + rim1 * cpMatrixData[scLookupTable.GetMapping(9 * (2 + a) + 3, 9)]);
		else if (b == 2)
			return (ri1 * cpMatrixData[scLookupTable.GetMapping(9 * (2 + a) + 3, 9)] - rim1 * cpMatrixData[scLookupTable.GetMapping(9 * (2 + a) + 1, 9)]);
		else 
			return (ri0 * cpMatrixData[scLookupTable.GetMapping(9 * (2 + a) + 2 + b, 9)]);
	}
	else
	{
		const F ri1		= cpMatrixData[9 * (2 + i) + 3];
		const F rim1	= cpMatrixData[9 * (2 + i) + 1];
		const F ri0		= cpMatrixData[9 * (2 + i) + 2];
		if (b == -2)
			return (ri1 * cpMatrixData[9 * (2 + a) + 1] + rim1 * cpMatrixData[9 * (2 + a) + 3]);
		else if (b == 2)
			return (ri1 * cpMatrixData[9 * (2 + a) + 3] - rim1 * cpMatrixData[9 * (2 + a) + 1]);
		else 
			return (ri0 * cpMatrixData[9 * (2 + a) + 2 + b]);
	}
#endif
}

template<class F, bool Compress, int n>
const F NSH::NRecursionHelper::NBand3Optimization::M3m1(const CRotateMatrix_tpl<F,Compress>& crSHRotMat)
{
	const F denom = (abs(n) == 2)? (F)((2 * 2) * (2 * 2 - 1)) : (F)((2 + n) * (2 - n));
	// Scale by their functions
	const F u = Sqrt((F)3.0 / denom) * P3<F,Compress, 0, 1, n>(crSHRotMat);
	const F p0 = P3<F,Compress, 1, 0, n>(crSHRotMat);
	return (u +  ((F)0.5 * Sqrt((F)6.0 / denom) * (p0 * Sqrt((F)2.0))));
}

template<class F, bool Compress, int n>
const F NSH::NRecursionHelper::NBand3Optimization::M3mMinus1(const CRotateMatrix_tpl<F,Compress>& crSHRotMat)
{
	const F denom = (abs(n) == 2)? (F)((2 * 2) * (2 * 2 - 1)) : (F)((2 + n) * (2 - n));
	// Scale by their functions
	const F u = Sqrt((F)3.0 / denom) * P3<F,Compress, 0, -1, n>(crSHRotMat);
	const F p1 = P3<F,Compress, -1, 0, n>(crSHRotMat);
	return (u + ((F)0.5 * Sqrt((F)6.0 / denom) * (p1 * Sqrt((F)2.0f))));
}


template<class F, bool Compress, int n>
const F NSH::NRecursionHelper::NBand3Optimization::M3mMinus2(const CRotateMatrix_tpl<F,Compress>& crSHRotMat)
{
	const F denom = (abs(n) == 2)? (F)((2 * 2) * (2 * 2 - 1)) : (F)((2 + n) * (2 - n));
	const F p0 = P3<F,Compress, 1, -1, n>(crSHRotMat);
	const F p1 = P3<F,Compress, -1, 1, n>(crSHRotMat);
	return ((F)0.5 * Sqrt((F)12.0 / denom) * (p0 + p1));
}

template<class F, bool Compress, int n>
const F NSH::NRecursionHelper::NBand3Optimization::M3m2(const CRotateMatrix_tpl<F,Compress>& crSHRotMat)
{
	const F denom = (abs(n) == 2)? (F)((2 * 2) * (2 * 2 - 1)) : (F)((2 + n) * (2 - n));
	// Scale by their functions
	const F p0 = P3<F,Compress, 1, 1, n>(crSHRotMat);
	const F p1 = P3<F,Compress, -1, -1, n>(crSHRotMat);
	return ((F)0.5 * Sqrt((F)12.0 / denom) * (p0 - p1));
}

template<class F, bool Compress, int n>
const F NSH::NRecursionHelper::NBand3Optimization::M3m0(const NSH::CRotateMatrix_tpl<F,Compress>& crSHRotMat)
{
	const F denom = (abs(n) == 2)? (F)((2 * 2) * (2 * 2 - 1)) : (F)((2 + n) * (2 - n));
	// Scale by their functions
	const F u = Sqrt((F)4.0 / denom) * P3<F,Compress, 0, 0, n>(crSHRotMat);
	const F p0 = P3<F,Compress, 1, 1, n>(crSHRotMat);
	const F p1 = P3<F,Compress, -1, -1, n>(crSHRotMat);
	return (u + ((F)-1.0 * Sqrt((F)1.0 / denom) * (p0 + p1)));
}

/******************* END band 3 optimization **********************************************************************************************************/

template<class F, bool Compress>
inline const F NSH::NRecursionHelper::M(const int l, const int m, const int n, const NSH::CRotateMatrix_tpl<F,Compress>& crSHRotMat)
{
	// First get the scalars
	F u, v, w;
	UVW(l, m, n, u, v, w);
	// Scale by their functions
	if(u)
		u *= P<F,Compress>(0, l, m, n, crSHRotMat);
	if(v)
		v *= V(l, m, n, crSHRotMat);
	if(w)
		w *= W(l, m, n, crSHRotMat);

	return (u + v + w);
}

template<class F, bool Compress>
void NSH::CRotateMatrix_tpl<F, Compress>::DebugOutput()const
{
	for(int i=0; i<m_DimRow; ++i)	
	{
		for(int j=0; j<m_DimColumn; ++j)
		{
			if(Compress)
			{
				const int32 index = scLookupTable.GetMapping(i*m_DimColumn+j, m_DimColumn);
				printf(" %.4f ",(index!=-1)?(float)m_pData[index]:0.);
			}
			else
				printf(" %.4f ",(float)(m_pData[i*m_DimColumn+j]));
		}
		printf("\n");
	}
	printf("\n");
}

template<class F, bool Compress>
template<bool SecondCompressed>
inline void NSH::CRotateMatrix_tpl<F, Compress>::ReAllocateAndPartCopy(const size_t cSize, const NSH::CRotateMatrix_tpl<F, SecondCompressed>& rM)
{
	//both uncompressed, copy values
	static CSHAllocator<F> sAllocator;
	if(m_pData)
	{ 
		sAllocator.delete_mem_array(m_pData, sizeof(F) * m_DimRow * m_DimColumn);
		m_pData = NULL;
	}
	m_DimRow				= rM.m_DimRow;
	m_DimColumn			=	rM.m_DimColumn;
	m_Bands					= rM.m_Bands;
	m_Coefficients	= rM.m_Coefficients;
	m_pData = (F*)(sAllocator.new_mem_array(sizeof(F) * cSize));
	assert(m_pData);
	assert(rM.m_pData);
}

template<class F, bool Compress>
template<bool SecondCompressed>
const NSH::CRotateMatrix_tpl<F, Compress>& NSH::CRotateMatrix_tpl<F, Compress>::operator=(const NSH::CRotateMatrix_tpl<F, SecondCompressed>& rM)
{
	//check self assignment 
	if(m_pData == rM.m_pData)
		return *this;
	if(Compress && SecondCompressed)
	{
		//both compressed, copy values
		const size_t elem = GetCompressedElemCount(m_Bands);
		ReAllocateAndPartCopy(elem, rM);
		//copy data
		for(size_t i=0; i<elem; ++i)
			m_pData[i] = rM.m_pData[i];
	}
	else
	if(Compress && !SecondCompressed)
	{
		//compressed/uncompressed
		const size_t elem = GetCompressedElemCount(m_Bands);
		ReAllocateAndPartCopy(elem, rM);
		//copy data
		for(int i=0; i<m_DimRow * m_DimColumn; ++i)
		{
			const int32 index = scLookupTable.GetMapping(i, m_DimColumn);		//retrieve index from second matrix, if != -1, add value
			if(index != -1)
				m_pData[index] = rM.m_pData[i];
		}
	}
	else
	if(!Compress && SecondCompressed)
	{
		//uncompressed/compressed
		const size_t elem = m_DimRow * m_DimColumn;
		ReAllocateAndPartCopy(elem, rM);
		//copy data
		for(int i=0; i<m_DimRow * m_DimColumn; ++i)
		{
			const int32 index = scLookupTable.GetMapping(i, m_DimColumn);		//retrieve index from second matrix, if != -1, add value
			if(index != -1)
				m_pData[i] = rM.m_pData[index];
			else
				m_pData[i] = 0;
		}
	}
	else
	if(!Compress && !SecondCompressed)
	{
		//both uncompressed, copy values
		const size_t elem = m_DimRow * m_DimColumn;
		ReAllocateAndPartCopy(elem, rM);
		//copy data
		for(size_t i=0; i<elem; ++i)
			m_pData[i] = rM.m_pData[i];
	}
	return *this;
}

template<class F, bool Compress>
template<bool SecondCompressed>
NSH::CRotateMatrix_tpl<F, Compress>::CRotateMatrix_tpl(const NSH::CRotateMatrix_tpl<F, SecondCompressed>& rM)	
	: m_DimRow(rM.m_DimRow), m_DimColumn(rM.m_DimColumn), m_Bands(rM.m_Bands), m_Coefficients(rM.m_Coefficients)
{
	static CSHAllocator<F> sAllocator;
	if(Compress && SecondCompressed)
	{
		//both compressed, copy values
		const size_t elem = GetCompressedElemCount(m_Bands);
		m_pData = (F*)(sAllocator.new_mem_array(sizeof(F) * elem));
		assert(m_pData);
		assert(rM.m_pData);
		//copy data
		for(size_t i=0; i<elem; ++i)
			m_pData[i] = rM.m_pData[i];
	}
	else
	if(Compress && !SecondCompressed)
	{
		//compressed/uncompressed
		const size_t elem = GetCompressedElemCount(m_Bands);
		m_pData = (F*)(sAllocator.new_mem_array(sizeof(F) * elem));
		assert(m_pData);
		assert(rM.m_pData);
		//copy data
		for(int i=0; i<m_DimRow * m_DimColumn; ++i)
		{
			const int32 index = scLookupTable.GetMapping(i, m_DimColumn);		//retrieve index from second matrix, if != -1, add value
			if(index != -1)
				m_pData[index] = rM.m_pData[i];
		}
	}
	else
	if(!Compress && SecondCompressed)
	{
		//uncompressed/compressed
		const size_t elem = m_DimRow * m_DimColumn;
		m_pData = (F*)(sAllocator.new_mem_array(sizeof(F) * elem));
		assert(m_pData);
		assert(rM.m_pData);
		//copy data
		for(int i=0; i<m_DimRow * m_DimColumn; ++i)
		{
			const int32 index = scLookupTable.GetMapping(i, m_DimColumn);		//retrieve index from second matrix, if != -1, add value
			if(index != -1)
				m_pData[i] = rM.m_pData[index];
			else
				m_pData[i] = 0;
		}
	}
	else
	if(!Compress && !SecondCompressed)
	{
		//both uncompressed, copy values
		const size_t elem = m_DimRow * m_DimColumn;
		m_pData = (F*)(sAllocator.new_mem_array(sizeof(F) * elem));
		assert(m_pData);
		assert(rM.m_pData);
		//copy data
		for(size_t i=0; i<elem; ++i)
			m_pData[i] = rM.m_pData[i];
	}
}

#pragma warning (default : 4127) 