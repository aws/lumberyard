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

#ifndef CRYINCLUDE_TOOLS_PRT_ITRANSFERCONFIGURATOR_H
#define CRYINCLUDE_TOOLS_PRT_ITRANSFERCONFIGURATOR_H
#pragma once

#if defined(OFFLINE_COMPUTATION)


#include "PRTTypes.h"
#include "TransferParameters.h"
#include <PRT/ISHMaterial.h>
#include <PRT/SHFrameworkBasis.h>

namespace NSH
{
	namespace NTransfer
	{
		struct ITransferConfigurator;
	}
	typedef NSH::CSmartPtr<NSH::NTransfer::ITransferConfigurator, CSHAllocator<> > ITransferConfiguratorPtr;

	namespace NTransfer
	{
		//!< struct needed to process ray tracing results for one sample
		//!< used for ProcessRayCastingResult in direct pass
		struct SRayResultData
		{
			//input data
			const bool cHasTransparentMats;			//!< determines whether there are any transparent materials or not(in that case ray casting needs to be handled in another way)
			const TCartesianCoord& crRayDir;		//!< ray direction
			const bool cIsOnUpperHemisphere;		//!< indicates where sample lies with respect to vertex
			const TRayResultVec& crRayResultVec;//!< results from ray casting, all hits sorted in distance ascending order
			//in/output data
			TRGBCoeffType& rIncidentIntensity;	//!< incident intensity for coefficients(determined by materials and ray path)
			//output data
			bool& rContinueLoop;								//!< indicates whether to continue sample loop or not, basically tells whether there is something to store into the coeff list or not
			bool& rApplyTransparency;						//!< indicates whether to apply transparency for visibility or not, basically is the ray entirely blocked or not
			SRayResult* pRayRes;								//!< ray result to store for interreflections, can only be one
			SRayResultData
			(
				const bool cHasTransparentMatsParam,
				const TCartesianCoord& crRayDirParam,
				const bool cIsOnUpperHemisphereParam,
				const TRayResultVec& crRayResultVecParam,
				TRGBCoeffType& rIncidentIntensityParam,
				bool& rContinueLoopParam,
				bool& rApplyTransparencyParam
			) : cHasTransparentMats(cHasTransparentMatsParam), crRayDir(crRayDirParam), cIsOnUpperHemisphere(cIsOnUpperHemisphereParam),
						rIncidentIntensity(rIncidentIntensityParam), rContinueLoop(rContinueLoopParam), rApplyTransparency(rApplyTransparencyParam),
						crRayResultVec(crRayResultVecParam), pRayRes(NULL)
			{}
		};

/************************************************************************************************************************************************/

		//!< struct needed to transform processed ray tracing results for one sample into coefficient specific data
		//!< used for TransformRayCastingResult in direct pass
		struct SRayProcessedData
		{
			//input data
			const NSH::NMaterial::ISHMaterial& crMat;							//!< reference to current material (coefficient vertex belonging to)
			const CSample_tpl<SCoeffList_tpl<TScalarCoeff> >& crSample;		//!< reference to current sample
			const STransferParameters& crParameters;							//!< reference to transfer parameters
			const TVec& crPos;																		//!< pos of current vertex coefficients belonging to
			const TVec& crNormal;																	//!< normal of current vertex coefficients belonging to			
			const Vec2& crTexCoord;																//!< tex coord of current vertex coefficients belonging to
			const bool cApplyTransparency;												//!< indicates whether to apply transparency or not
			const bool cIsOnUpperHemisphere;											//!< indicates location of sample in terms of hemisphere with respect to vertex
			//output data
			TRGBCoeffType& rIncidentIntensity;										//!< incident intensity at vertex
			SCoeffList_tpl<TScalarCoeff>& rCoeffListDirect;				//!< reference to direct coefficients
			float& rVis;																					//!< reference to overall visibility for statistic purposes
			
			SRayProcessedData
			(
				const NSH::NMaterial::ISHMaterial& crMatParam,
				const CSample_tpl<SCoeffList_tpl<TScalarCoeff> >& crSampleParam,
				const STransferParameters& crParametersParam,
				const TVec& crPosParam,
				const TVec& crNormalParam,
				const Vec2& crTexCoordParam,
				const bool cApplyTransparencyParam,
				const bool cIsOnUpperHemisphereParam,
				const bool cApplyInterreflectionParam,
				TRGBCoeffType& rIncidentIntensityParam,
				SCoeffList_tpl<TScalarCoeff>& rCoeffListDirectParam,
				float& rVisParam
			) : crMat(crMatParam), crSample(crSampleParam), rIncidentIntensity(rIncidentIntensityParam),
					crParameters(crParametersParam), crPos(crPosParam), crNormal(crNormalParam), crTexCoord(crTexCoordParam), 
					cApplyTransparency(cApplyTransparencyParam), cIsOnUpperHemisphere(cIsOnUpperHemisphereParam),
					rCoeffListDirect(rCoeffListDirectParam), rVis(rVisParam)
			{}
		};

/************************************************************************************************************************************************/

		//!< interface for transfer configurators
		struct ITransferConfigurator
		{
			virtual const bool ProcessOnlyUpperHemisphere(const NMaterial::EMaterialType)const = 0;	//!< retrieves whether to process only upper hemisphere
			virtual const bool UseCoefficientLookupMode()const = 0;		//!< retrieves whether to prepare coefficients for lookup or convolution
			//!< processes ray results, default implementation provided, the exitant ray intensity and some other flow controlling bools are set
			virtual void ProcessRayCastingResult(SRayResultData& rRayResultData)const;			
			//!< transforms results from ProcessRayCastingResult into the coefficient specific data
			virtual void TransformRayCastingResult(SRayProcessedData& rRayProcessedData)const;			
			//following functions abstract the adding of values to the different per vertex coefficients
			//!< adds a scalar value into the coefficients
			virtual void AddDirectScalarCoefficientValue
			(
				SCoeffList_tpl<TScalarCoeff>& rCoeffListDirect, const TScalarCoeff::TComponentType& crValue, const CSample_tpl<SCoeffList_tpl<TScalarCoeff> >& crSample
			)const;			
			//!< virtual destructor, crashes for some reason otherwise even with no members involved
			virtual ~ITransferConfigurator(){}
			//!< need cloned configurators for multithreading
			virtual ITransferConfiguratorPtr Clone()const = 0;
		};
	}
}

#endif

#endif // CRYINCLUDE_TOOLS_PRT_ITRANSFERCONFIGURATOR_H
