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

// Description : stream manager declaration which converts coefficient vectors to streams 
//               for export, compresses if requested too

#ifndef CRYINCLUDE_TOOLS_PRT_MESHCOEFFICIENTSTREAMS_H
#define CRYINCLUDE_TOOLS_PRT_MESHCOEFFICIENTSTREAMS_H
#pragma once

#if defined(OFFLINE_COMPUTATION)


#include "PRTTypes.h"
#include "TransferParameters.h"
#include "ITransferConfigurator.h"

#pragma warning (disable : 4512) 

namespace NSH
{
	//!< stream manager, creates streams for export with possible compression
	//!< template arguments for all coefficient types
	//!< individual instance per mesh
	template<class DirectCoeffType>
	class CMeshCoefficientStreams
	{
		typedef prtvector<NSH::SCoeffList_tpl<DirectCoeffType> > TDirectCoeffVec;
		#define TCoeffList prtvector<NSH::SCoeffList_tpl<CoeffType> >

	public:
		//!< constructor taking  the references for the stream creation, stream creation happens on demand
		CMeshCoefficientStreams 
		(
			const NSH::SDescriptor& crDescriptor,								
			const NTransfer::STransferParameters& crParameters, 
			CSimpleIndexedMesh& rMesh, 
			TDirectCoeffVec&		rCoeffsToStore, 
			const NSH::NTransfer::ITransferConfigurator& crTransferConfigurator
		);

		~CMeshCoefficientStreams();

		const TFloatPtrVec& GetStream();	//!< retrieves the direct coeff stream

		void RetrieveCompressedCoeffs(NSH::NFramework::SMeshCompressedCoefficients& rCompressedData);	//!< fill the compressed data struct
		static void FillCompressedCoeffs
		(
			NSH::NFramework::SMeshCompressedCoefficients& rCompressedData,
			const CSimpleIndexedMesh& crMesh,
			const NSH::TFloatPtrVec& crDirectCoeffStream
		);		//!< fill the compressed data struct

	private:
		NSH::SDescriptor m_Descriptor;										//!< transfer sh descriptor	
		NTransfer::STransferParameters m_Parameters;			//!< transfer parameters controlling the transfer and compression
		TDirectCoeffVec&		m_rDirectCoeffsToStore;				//!< direct coefficients to streamconvert
		TFloatPtrVec m_CoeffStreams;											//!< coefficient stream
		CSimpleIndexedMesh& m_rMesh;											//!< mesh belonging too
		const NSH::NTransfer::ITransferConfigurator& m_crTransferConfigurator;	//!< transfer configurator to work with
		//!< compression infos for each material, empty if no compression is requested, compression info index corresponds to the respective m_ObjMaterials index
		TCompressionInfo	m_MaterialCompressionInfo;
		bool m_StreamCreated;															//!< indicates whether the streams were created yet or not(to make it on demand)

		void PrepareCoefficientsForExport();	//!< prepares coefficients for export, changes the coefficients doing so
		//!< compresses the prepared coefficients, changes the coefficient floats to the byte number (0.3 becomes f.i. 240.0)
		template<class CoeffType>
			void Compress(TCoeffList& rCoeffsToCompress);	
		//!< create a single stream, still floats but the meaning is different if it is compressed
		template<class CoeffType>
			void CreateSingleStream(TCoeffList& rCoeffsToStreamConvert);
		void CreateStreams();									//!< creates the streams controlled by data stored in rMesh and m_Parameters
	};
}

#include "MeshCoefficientStreams.inl"
#pragma warning (default : 4512)

#endif
#endif // CRYINCLUDE_TOOLS_PRT_MESHCOEFFICIENTSTREAMS_H
