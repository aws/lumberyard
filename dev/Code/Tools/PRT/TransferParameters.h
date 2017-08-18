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

#ifndef CRYINCLUDE_TOOLS_PRT_TRANSFERPARAMETERS_H
#define CRYINCLUDE_TOOLS_PRT_TRANSFERPARAMETERS_H
#pragma once


#include "PRTTypes.h"


namespace NSH
{
	//!< determines the encoding and usage of the coefficients
	typedef enum EBumpVisUsage
	{
		BUMP_VIS_USAGE_LOOKUP,					//!< normals performs SH - lookup
		BUMP_VIS_USAGE_COS_CONVOLUTION	//!< normals performs SH - cosine convolution
	}EBumpVisUsage;

/************************************************************************************************************************************************/

	//!< lookup/convolution constant retriever
	template <EBumpVisUsage TUsage>
	struct SCoeffUsage;

	template <>
	struct SCoeffUsage<BUMP_VIS_USAGE_LOOKUP>
	{
			static const float cv0;
			static const float cv1;
			static const float cv2;
			static const float cv3;
			static const float cv4;
	};

	template <>
	struct SCoeffUsage<BUMP_VIS_USAGE_COS_CONVOLUTION>
	{
		static const float cv0;
		static const float cv1;
		static const float cv2;
		static const float cv3;
		static const float cv4;
	};

/************************************************************************************************************************************************/

	namespace NTransfer
	{
		//!< target output coord system, always object space
		enum EMatrixHeuristic
		{
			MATRIX_HEURISTIC_MAX,				//!< up: Z  right: X  back: Y
			MATRIX_HEURISTIC_SH					//!< up: Z  right: Y  back: -X
		};

		//!< transfer process mode, configurator to provide flexible transfers
		enum ETransferConfigurator
		{
			TRANSFER_CONFIG_VEGETATION,		//!< vegetation mode
			TRANSFER_CONFIG_ICE,					//!< ice mode
			TRANSFER_CONFIG_DEFAULT				//!< standard mode without 360 degree RT
		};

		//!< transfer parameters controlling the transfer computation
		struct STransferParameters
		{
			Vec3  backlightingColour;							//!< optional backlighting colour if backlighting is supported
			float groundPlaneBlockValue;					//!< if a ray is fired toward the lower object, this value tell it how much it is blocked
			float rayTracingBias;									//!< bias for ray tracer
			float	transparencyShadowingFactor;		//!< factor 0..1 shadowing the transparency of materials to fake density(0: complete non transparent, 1: unchanged)
			float minDirectBumpCoeffVisibility;		//!< minimum value for direct bump coefficients
			int32 sampleCountPerVertex;						//!< number of samples to retrieve per vertex, -1 indicates max count
			EMatrixHeuristic matrixTargetCS;			//!< target coordinate system to rotate coeffs in
			uint32 rayCastingThreads;							//!< number of threads processing the ray casting
			ETransferConfigurator configType;			//!< configuration type used for transfer processing
			bool	bumpGranularity;								//!< if true, then direct coefficients are computed as per vertex tangent space scalar coefficients
			bool  supportTransparency;						//!< if true, then support transparency in base texture material(if there is no alpha, use base texture materials)	
			bool  compressToByte;									//!< if true, then coefficients are compressed from floats to 8 bits, this is done in a object global manner with an individual factor for each coefficient index

			STransferParameters();								//!< default and only constructor
			void Log()const;											//!< logs used parameters
			const bool CheckForConsistency()const;//!< checks parameters for consistency
		};
	}
}

#include "TransferParameters.inl"
#endif // CRYINCLUDE_TOOLS_PRT_TRANSFERPARAMETERS_H
