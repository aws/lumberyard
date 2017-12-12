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

#ifndef CRYINCLUDE_TOOLS_PRT_SHCOMPRESSOR_H
#define CRYINCLUDE_TOOLS_PRT_SHCOMPRESSOR_H
#pragma once


#include "PRTTypes.h"
#include "TransferParameters.h"

namespace NSH
{
	//!< class for handling SH compression outside meshes
	//!<		accumulates all min max values through a functor, calculates the matrix (retrievable too) and then it provides
	//!<		functionality for compressing the coefficients with that matrix plus swizzling/coeff dropping
	//!<		TIteratorSink must return (reference to ) next coeff list (with type TCoeffType) with * and implement ++ to get next one
	template<typename TCoeffType, typename TIteratorSink>
	class CSHCompressor
	{
	public:
		//!< configurator for SH compression
		struct SCompressionProp
		{
			EBumpVisUsage coeffUsage;			//!< usage of coefficients: convolution or lookup
			bool byteCompression;					//!< if true, it compresses to byte
			bool dropCoefficient;					//!< if true, it drops a coefficient
			bool preSwizzle;							//!< if true, it pre-swizzles and multiplies for fast SH convolution/lookup in shaders 

			SCompressionProp
			(
				const EBumpVisUsage cCoeffUsage			= BUMP_VIS_USAGE_COS_CONVOLUTION, 
				const bool cByteCompression					= true, 
				const bool cDropCoefficient					= true,
				const bool cPreSwizzle							= true
			);
		};

		//!< description of compressed coefficients
		struct SCompressedCoeffListDesc
		{
			uint8 bytesPerCoeff;		//!< bytes per coefficients
			uint8 coeffsPerList;		//!< coefficients per coefficient list
			bool preSwizzled;				//!< indicates whether it has been pre-swizzled or not
			SCompressedCoeffListDesc();	//!< default ctor
		};

	public:	
		typedef std::vector<uint8> TComprCoeffList;	//!< compressed coeff list type, coeff0 (possibly RGB) followed by coeff1 ...
		typedef std::vector<TComprCoeffList>::const_iterator TComprIter;	//!< iterator for compressed elements

		//!< constructor taking compression settings plus begin and end iterator
		CSHCompressor(const SCompressionProp& crProp, TIteratorSink iterBegin, const TIteratorSink cIterEnd, const uint32 cBands = 3); 
		const SCompressedCoeffListDesc GetCompressedCoeffDesc() const;//!< retrieves the description for each compressed coeff set 
		TComprIter begin() const;			//!< retrieves begin iterator to compressed elements (stored in the same order as they were retrieved)	
		const TComprIter end() const;	//!< retrieves end iterator to compressed elements (stored in the same order as they were retrieved)	
		//!< retrieves compression matrix, for each coefficient (same for RGB) a float pair: first = scale, second = offset
		const TFloatPairVec& GetDecompressionMatrix() const;	
	
	private:
		std::vector<TComprCoeffList>	m_CompressBuffer;			//!< buffer containing all compressed elements
		SCompressedCoeffListDesc			m_ComprCoeffListDesc;	//!< description of compressed elements
		TFloatPairVec									m_CompressionMatrix;	//!< float pair vec for each coefficient compression
		std::vector<uint8>						m_MappingVec;					//!< vec maps coefficient indices according to stream type
		size_t												m_CoeffCount;					//!< count of coeffs to compress	
		bool													m_DoByteCompression;	//!< true if to do byte compression	

		void GenerateCompressionMatrix(const SCompressionProp& crProp, TIteratorSink iterBegin, const TIteratorSink cIterEnd);//!< generates compression matrix
		void CompressCoeffs(const SCompressionProp& crProp, TIteratorSink iterBegin, const TIteratorSink cIterEnd);//!< compresses all coefficients
		void GenerateCoeffMapping(const SCompressionProp& crProp);	//!< generates the coefficient mapping according to coeffMode SCompressionProp
		//!< pre-swizzles a coefficient-list for fast lookup/convolution in vertex shader, only for 8 coeffs
		const SCoeffList_tpl<TCoeffType> PreSwizzleDirect(const SCompressionProp& crProp, const SCoeffList_tpl<TCoeffType>& crCoeff);
		void InvertCompressionMatrix();//!< inverts the compression matrix into a decompression matrix
	};

}

#include "SHCompressor.inl"

#endif // CRYINCLUDE_TOOLS_PRT_SHCOMPRESSOR_H
