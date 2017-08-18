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


// sh framework declaration
// file to include for external usage, acts as user interface

#ifndef CRYINCLUDE_TOOLS_PRT_SHFRAMEWORK_H
#define CRYINCLUDE_TOOLS_PRT_SHFRAMEWORK_H
#pragma once


#include "SHFrameworkBasis.h"
#include "TransferParameters.h"
#include "SHRotate.h"
#if defined(OFFLINE_COMPUTATION)
	#include "SimpleIndexedMesh.h"
	#include "ISHMaterial.h"
	#include "MaterialFactory.h"
#endif
#include "SHCompressor.h"
//#include "Quadtree/Quadtree.h"
#include "QuantizedSamplesHolder.h"

namespace NSH
{
	namespace NFramework
	{
#if defined(OFFLINE_COMPUTATION)//use these functions only in case of resource compiler or so

		/****************************************************MAIN FRAMEWORK FUNCTIONS*********************************************************************/

		//! computes and stores the interreflected transfer for a single meshes (in the compressed byte format, 8 byte per component)
		//! works in world space, if object space then mesh matrix should be the identity one
		//! \param rMesh mesh to work on
		//! \param crParameters parameters to control the computation
		//! \return true = transfer coefficients successfully computed
		const bool ComputeSingleMeshTransferCompressed(CSimpleIndexedMesh& rMesh, const NTransfer::STransferParameters& crParameters, SMeshCompressedCoefficients& rCompressedCoeffs);
#endif//OFFLINE_COMPUTATION	

		//! returns a sh compressor instance for the specific coefficient type to compress
		//! can be used for scalar as for rgb types
		//! \param rCompressor
		//! \param crProp
		//! \param iterBegin
		//! \param cIterEnd
		//! \param cBands
		template<typename TCoeffType, typename TIteratorSink>
		const CSHCompressor<TCoeffType, TIteratorSink> GetSHCompressor
		(
			const typename CSHCompressor<TCoeffType, TIteratorSink>::SCompressionProp& crProp, 
			TIteratorSink iterBegin, 
			const TIteratorSink cIterEnd, 
			const uint32 cBands = 3
		);

		//! quantizes the samples from the sample generator into azimuth steps and fills a vector
		//! returns total number of retrieved samples
		//! \param rSampleHolder sample quantizer holder containing vector of samples, will contain cAzimuthSteps sub-vecs
		//! \param cOnlyUpperHemisphere determines whether to retrieve all samples or just from the upper hemisphere
		const uint32 QuantizeSamplesAzimuth
		(
			CQuantizedSamplesHolder& rSampleHolder,
			const bool cOnlyUpperHemisphere = false
		);

		/************************************************************************************************************************************************/

	}
}
	
#include "SHFramework.inl"
#endif // CRYINCLUDE_TOOLS_PRT_SHFRAMEWORK_H
