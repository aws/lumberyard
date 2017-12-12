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

/*
	SH compression implementation
*/

namespace NSH
{
	template<typename TCoeffType, typename TIteratorSink>
	inline CSHCompressor<TCoeffType, TIteratorSink>::SCompressedCoeffListDesc::SCompressedCoeffListDesc() 
		: bytesPerCoeff(0), coeffsPerList(0), preSwizzled(false)
	{}

	template<typename TCoeffType, typename TIteratorSink>
	inline CSHCompressor<TCoeffType, TIteratorSink>::SCompressionProp::SCompressionProp
	(
		const EBumpVisUsage cCoeffUsage, 
		const bool cByteCompression, 
		const bool cDropCoefficient,
		const bool cPreSwizzle
	) : coeffUsage(cCoeffUsage), byteCompression(cByteCompression), dropCoefficient(cDropCoefficient), preSwizzle(cPreSwizzle)
	{}

	template<typename TCoeffType, typename TIteratorSink>
	CSHCompressor<TCoeffType, TIteratorSink>::CSHCompressor(const SCompressionProp& crProp, TIteratorSink iterBegin, const TIteratorSink cIterEnd, const uint32 cBands)
		: m_CoeffCount(0), m_DoByteCompression(crProp.byteCompression)
	{
		assert(cBands > 0);
		m_ComprCoeffListDesc.bytesPerCoeff = TCoeffType::Components() * (crProp.byteCompression?1:4);
		m_ComprCoeffListDesc.coeffsPerList = cBands * cBands - (crProp.dropCoefficient?1:0);
		GenerateCoeffMapping(crProp);
		if((TCoeffType::Components() == 1) && crProp.preSwizzle)//indirect coefficients need another coeff to drop, convolution formula must be derived if needed
			m_ComprCoeffListDesc.preSwizzled = true;
		GenerateCompressionMatrix(crProp, iterBegin, cIterEnd);
		CompressCoeffs(crProp, iterBegin, cIterEnd);
		InvertCompressionMatrix();
	}

	template<typename TCoeffType, typename TIteratorSink>
	inline const typename CSHCompressor<TCoeffType, TIteratorSink>::SCompressedCoeffListDesc CSHCompressor<TCoeffType, TIteratorSink>::GetCompressedCoeffDesc() const
	{
		return m_ComprCoeffListDesc;
	}

	template<typename TCoeffType, typename TIteratorSink>
	inline typename CSHCompressor<TCoeffType, TIteratorSink>::TComprIter CSHCompressor<TCoeffType, TIteratorSink>::begin() const
	{
		return m_CompressBuffer.begin();
	}

	template<typename TCoeffType, typename TIteratorSink>
	inline const typename CSHCompressor<TCoeffType, TIteratorSink>::TComprIter CSHCompressor<TCoeffType, TIteratorSink>::end() const
	{
		return m_CompressBuffer.end();
	}

	template<typename TCoeffType, typename TIteratorSink>
	inline const TFloatPairVec& CSHCompressor<TCoeffType, TIteratorSink>::GetDecompressionMatrix() const	
	{
		return m_CompressionMatrix;
	}

	template<typename TCoeffType, typename TIteratorSink>
	void CSHCompressor<TCoeffType, TIteratorSink>::GenerateCompressionMatrix
		(const SCompressionProp& crProp, TIteratorSink iterBegin, const TIteratorSink cIterEnd)
	{
		m_CompressionMatrix.resize(m_ComprCoeffListDesc.coeffsPerList);
		if(crProp.byteCompression)
		{
			TFloatPairVec minMaxVec(m_CompressionMatrix.size());
			const TFloatPairVec::const_iterator cEnd = minMaxVec.end();
			for(TFloatPairVec::iterator pairIter = minMaxVec.begin(); pairIter != cEnd; ++pairIter)
			{
				//initialize max/min accumulators
				pairIter->first	 = std::numeric_limits<float>::max();
				pairIter->second = std::numeric_limits<float>::min();
			}
			//now iterate all coefficients and record min/max values
			for(TIteratorSink iter = iterBegin; iter != cIterEnd; ++iter)
			{
				++m_CoeffCount;
				const SCoeffList_tpl<TCoeffType> cCoeffList = m_ComprCoeffListDesc.preSwizzled? PreSwizzleDirect(crProp, *iter) : *iter;	
				for(int c=0; c<m_ComprCoeffListDesc.coeffsPerList; ++c)
				{	//iterate all coefficients
					const TCoeffType& crCoeff = cCoeffList[m_MappingVec[c]];//get coefficient mapping
					TFloatPair& rMinMax = minMaxVec[c];
					for(int i=0; i<TCoeffType::Components(); ++i)
					{
						//iterate all components, compressed equally to not have multiple matrices (1 per channel)
						const float cCoeffValue = (float)(crCoeff[i]);
						if(rMinMax.first > cCoeffValue)
							rMinMax.first = cCoeffValue;
						if(rMinMax.second < cCoeffValue)
							rMinMax.second = cCoeffValue;
					}
				}
			}
			//create compression matrix from min/max values
			for(int c=0; c<m_ComprCoeffListDesc.coeffsPerList; ++c)
			{
				const TFloatPair& crMinMax	= minMaxVec[c];
				TFloatPair& rComprElem			= m_CompressionMatrix[c];
				rComprElem.second = -crMinMax.first; //set compression offset
				if(fabs(crMinMax.second - crMinMax.first) < 0.00001f)
					rComprElem.first = 1.f;
				else
					rComprElem.first = 255.f / (crMinMax.second - crMinMax.first); //set compression scale to fit into 0..255
			}
		}
		else
		{
			const TFloatPairVec::const_iterator cEnd = m_CompressionMatrix.end();
			for(TFloatPairVec::iterator pairIter = m_CompressionMatrix.begin(); pairIter != cEnd; ++pairIter)
			{
				//initialize max/min accumulators
				pairIter->first	 = 1.f;
				pairIter->second = 0.f;
			}
		}
	}

	template<typename TCoeffType, typename TIteratorSink>
	void CSHCompressor<TCoeffType, TIteratorSink>::CompressCoeffs(const SCompressionProp& crProp, TIteratorSink iterBegin, const TIteratorSink cIterEnd)
	{
		//first allocate buffer vec
		assert((iterBegin == cIterEnd) || m_CoeffCount > 0);
		if(m_CoeffCount == 0)
			return;
		m_CompressBuffer.resize(m_CoeffCount);
		std::vector<TComprCoeffList>::iterator comprBufferIter = m_CompressBuffer.begin();
		const size_t cBufElemSize = m_ComprCoeffListDesc.bytesPerCoeff * m_ComprCoeffListDesc.coeffsPerList;
		//now iterate all coefficients and record min/max values
		for(TIteratorSink iter = iterBegin; iter != cIterEnd; ++iter)
		{
			TComprCoeffList& rComprElem = *comprBufferIter++;//current compressed coefficient destination 
			rComprElem.resize(cBufElemSize);	//allocate buffer size
			size_t bufIndex = 0;	//current write index into rComprElem
			const SCoeffList_tpl<TCoeffType> cCoeffList = m_ComprCoeffListDesc.preSwizzled? PreSwizzleDirect(crProp, *iter) : *iter;	
			for(int c=0; c<m_ComprCoeffListDesc.coeffsPerList; ++c)	//iterate all coefficients
			{	
				const TCoeffType& crCoeff = cCoeffList[m_MappingVec[c]];//get coefficient mapping
				TFloatPair& rMinMax = m_CompressionMatrix[c];//get compression matrix entry for this coeff
				for(int i=0; i<TCoeffType::Components(); ++i)//iterate all components, compressed equally 
				{
					float coeffValue = (float)(crCoeff[i]);	//cache component value to compress
					if(m_DoByteCompression)
					{
						coeffValue += rMinMax.second;	//add offset, now in range 0...
						coeffValue *= rMinMax.first;	  //scale into 0..255.0
						assert(coeffValue > -0.1f && coeffValue < 255.1f);
						//clamp values, it is just about some inaccuracies
						coeffValue = std::max(coeffValue, (typename TCoeffType::TComponentType)0.0);
						coeffValue = std::min(coeffValue, (typename TCoeffType::TComponentType)255.0);
						rComprElem[bufIndex++] = (uint8)((coeffValue - floor(coeffValue) < 0.5f)?floor(coeffValue) : ceil(coeffValue));
					}
					else
					{
						*reinterpret_cast<float*>(&rComprElem[bufIndex]) = coeffValue;
						bufIndex += 4;
					}
				}
			}
			assert(bufIndex == cBufElemSize);
		}
	}

	template<typename TCoeffType, typename TIteratorSink>
	void CSHCompressor<TCoeffType, TIteratorSink>::InvertCompressionMatrix()
	{
		//turn compression matrix into decompression matrix
		assert(m_CompressionMatrix.size() == m_ComprCoeffListDesc.coeffsPerList);
		for(int i=0; i<m_ComprCoeffListDesc.coeffsPerList; ++i)
		{
			assert(m_CompressionMatrix[i].first > 0.00001f);
			m_CompressionMatrix[i].first = 1.f / m_CompressionMatrix[i].first;
			m_CompressionMatrix[i].second = -m_CompressionMatrix[i].second;
		}
	}

	template<typename TCoeffType, typename TIteratorSink>
	void CSHCompressor<TCoeffType, TIteratorSink>::GenerateCoeffMapping(const SCompressionProp& crProp)
	{
		m_MappingVec.resize(m_ComprCoeffListDesc.coeffsPerList);
		std::vector<uint8>::iterator iter = m_MappingVec.begin();
		for(int i=0; i<std::min((uint8)4,m_ComprCoeffListDesc.coeffsPerList); ++i)	//first 4 coeffs always constant
			*iter++	= i;
		int curDestIndex = 4;		//current mapping index
		if(crProp.dropCoefficient)
			curDestIndex++;//drop Y2-2
		const std::vector<uint8>::const_iterator cEnd = m_MappingVec.end();
		for(; iter != cEnd; ++iter)	//last coeffs stay the same
			*iter	= curDestIndex++;
	}

	template<typename TCoeffType, typename TIteratorSink>
	const SCoeffList_tpl<TCoeffType> CSHCompressor<TCoeffType, TIteratorSink>::PreSwizzleDirect(const SCompressionProp& crProp, const SCoeffList_tpl<TCoeffType>& crCoeff)
	{
		SCoeffList_tpl<TCoeffType> coeff = crCoeff;	//cache current coefficient list to swizzle
		assert(crProp.preSwizzle && (TCoeffType::Components() == 1));
		//pre-swizzle first 8 coefficients, leave 9. coeff as it is
		//only valid for direct coeff stream
		const float cv0 = (crProp.coeffUsage == BUMP_VIS_USAGE_LOOKUP)? SCoeffUsage<BUMP_VIS_USAGE_LOOKUP>::cv0 : SCoeffUsage<BUMP_VIS_USAGE_COS_CONVOLUTION>::cv0;  
		const float cv1 = (crProp.coeffUsage == BUMP_VIS_USAGE_LOOKUP)? SCoeffUsage<BUMP_VIS_USAGE_LOOKUP>::cv1 : SCoeffUsage<BUMP_VIS_USAGE_COS_CONVOLUTION>::cv1;  
		const float cv2 = (crProp.coeffUsage == BUMP_VIS_USAGE_LOOKUP)? SCoeffUsage<BUMP_VIS_USAGE_LOOKUP>::cv2 : SCoeffUsage<BUMP_VIS_USAGE_COS_CONVOLUTION>::cv2;  
		const float cv3 = (crProp.coeffUsage == BUMP_VIS_USAGE_LOOKUP)? SCoeffUsage<BUMP_VIS_USAGE_LOOKUP>::cv3 : SCoeffUsage<BUMP_VIS_USAGE_COS_CONVOLUTION>::cv3;  
		const float cv4 = (crProp.coeffUsage == BUMP_VIS_USAGE_LOOKUP)? SCoeffUsage<BUMP_VIS_USAGE_LOOKUP>::cv4 : SCoeffUsage<BUMP_VIS_USAGE_COS_CONVOLUTION>::cv4;  
		//first 8 coefficients stay as they are, Y2-2 is put to the end unswizzled if not to drop
		const TCoeffType cSwizzledCoeffs[8] = 
			{coeff[3] * cv1, coeff[1] * cv1, coeff[2]* cv1, coeff[0] * cv0 - coeff[6] * cv3, 
			coeff[8] * cv4, coeff[5] * cv2, coeff[6]* 3 * cv3, coeff[7] * cv2};
		if(crProp.dropCoefficient)
		{
			for(int c0=0; c0<4; ++c0)
				coeff[c0] = cSwizzledCoeffs[c0];
			coeff[4] = 0;//unset Y2-2, mapping takes care of the correct mapping for the 9 coeffs with respect to the dropped one
			for(int c1=4; c1<8; ++c1)
				coeff[c1+1] = cSwizzledCoeffs[c1];
		}
		else
		{
			coeff[8] = coeff[4];//put Y2-2 back to the end
			for(int c0=0; c0<8; ++c0)
				coeff[c0] = cSwizzledCoeffs[c0];
			coeff[8] *= (crProp.coeffUsage == BUMP_VIS_USAGE_LOOKUP)?SCoeffUsage<BUMP_VIS_USAGE_LOOKUP>::cv2 : (SCoeffUsage<BUMP_VIS_USAGE_COS_CONVOLUTION>::cv4 * 2.f);
		}
		return coeff;
	}
}




