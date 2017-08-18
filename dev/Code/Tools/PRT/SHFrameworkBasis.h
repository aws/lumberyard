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

#ifndef CRYINCLUDE_TOOLS_PRT_SHFRAMEWORKBASIS_H
#define CRYINCLUDE_TOOLS_PRT_SHFRAMEWORKBASIS_H
#pragma once


#include "PRTTypes.h"

#if defined(OFFLINE_COMPUTATION)
	class CSimpleIndexedObjMesh;
#endif

namespace NSH
{
	namespace NTransfer
	{
		struct STransferParameters;
		struct ITransferConfigurator;
	}

	namespace NProjection
	{
		struct IProjectFunction;
	}

	namespace NSampleGen
	{
		struct ISampleGenerator;
	}

	namespace NFramework
	{
		static const uint32 gs_cDefaultSampleCount = 20000;	//!< default number of samples generated inside any sample generator

#if defined(OFFLINE_COMPUTATION)
		//!< describes the type of SH coefficients stored (basically under what purpose to render)
		typedef enum ESHRenderType
		{
			SH_RENDER_TYPE_NO_ACTIVITY = 0,								//!< no SH rendering
			SH_RENDER_TYPE_NO_BUMP_ACTIVITY = 1,					//!< vertex based activity
			SH_RENDER_TYPE_DIRECT_BUMP_ACTIVITY = 2,			//!< bump activity for direct illumination
			SH_RENDER_TYPE_INDIRECT_BUMP_ACTIVITY = 3		//!< full bump activity for direct and indirect illumination 
		} ESHRenderType;
#endif

		/************************************************************************************************************************************************/

		//!< framework parameters
		struct SFrameworkParameter
		{
			uint32 sampleCount;								//!< sample count for generator
			int32  minSampleCountToRetrieve;	//!< min sample count to retrieve(maybe just guessed)
			uint8 supportedBands;							//!< number of supported band
			//!< default constructor setting some values to some useful data
			SFrameworkParameter():sampleCount(gs_cDefaultSampleCount), supportedBands(gs_cSupportedMaxBands), minSampleCountToRetrieve(-1){}
		};

#if defined(OFFLINE_COMPUTATION)
		/************************************************************************************************************************************************/

		//!< policy how to store coefficients, currently only obj-export supported
		typedef enum ECoefficientStorePolicy
		{
			COEFFICIENT_STORE_POLICY_OBJ
		}ECoefficientStorePolicy;

		//! stores a coefficient set into one file(any type: rgb, float, double...)
		//! stores in the same order as vertices
		//! \param crCoeffsToStore coefficients to store
		//! \param cpFileName file to store into
		//! \param ConvertToFloat if true it converts each coefficient component to float
		template<class CoeffType>
			void StoreCoefficientSetAsStream(const std::vector<SCoeffList_tpl<CoeffType>, CSHAllocator<SCoeffList_tpl<CoeffType> > >& crCoeffsToStore, const char* cpFileName, bool ConvertToFloat = true);

		/************************************************************************************************************************************************/

		//!< current header of sh-coefficient set storage
		struct SSHHeader
		{
			uint32 ContainedCoefficientSets;
			uint8 BytesPerCoefficientComponent;
			uint8 ComponentsPerCoefficient;
			uint8 CoefficientsPerCoefficientSet;
			uint8 padding;
			SSHHeader() : ContainedCoefficientSets(0), BytesPerCoefficientComponent(0), ComponentsPerCoefficient(0), CoefficientsPerCoefficientSet(0), padding(0){}
		};

		/************************************************************************************************************************************************/

		//!< compressed coefficients, passed and filled by framework functions for each mesh
		struct SMeshCompressedCoefficients
		{
			//!< scalar byte coefficients for each vertex (for direct coeffs)
			uint8 *pDirectCoeffs;	
			uint32 size;	//!< corresponding size of pDirectCoeffs
			SMeshCompressedCoefficients() : pDirectCoeffs(NULL), size(0)
			{}

			~SMeshCompressedCoefficients()
			{
				if(pDirectCoeffs)
					delete [] pDirectCoeffs;
			}
		};
#endif

		/************************************************************************************************************************************************/

		//!< singleton framework containing/controlling sample generator and ray cast engine, restricted usage to framework functions
		class CSHFrameworkManager
		{
		public:
			//!< singleton instance stuff
			static CSHFrameworkManager* Instance(const SFrameworkParameter&, const bool cUseCurrent = false);
			
			const NSampleGen::ISampleGenerator& GetSampleGen() const;		//!< retrieve sample gen

		private:

			SDescriptor m_Desc;													//!< sh-descriptor
			bool m_InstanceActive;											//!< controls activity of sample generator
			NSampleGen::ISampleGenerator *m_pSampleGen;	//!< best available sample generator

			//!< singleton stuff
			CSHFrameworkManager() : m_Desc(3), m_InstanceActive(false), m_pSampleGen(0){}
			~CSHFrameworkManager();
			CSHFrameworkManager(const CSHFrameworkManager&);
			CSHFrameworkManager& operator= (const CSHFrameworkManager&);

#if defined(OFFLINE_COMPUTATION)
			// friend functions where this manager is visible to
			friend const bool ProjectSphericalEnvironmentMap(SCoeffList_tpl<TRGBCoeffD>&, const char*, const uint8);
			friend void ProjectFunction(NSH::SCoeffList_tpl<NSH::TRGBCoeffF>&, const NSH::NProjection::IProjectFunction&, const NSH::NTransfer::STransferParameters&, const bool);
			friend const bool ProjectSphericalEnvironmentMap(const char*, const uint8);
			friend const bool ComputeSingleMeshTransferCompressed(CSimpleIndexedMesh&, const NTransfer::STransferParameters&, SMeshCompressedCoefficients&);
#endif
		}; 
	}
}


#endif // CRYINCLUDE_TOOLS_PRT_SHFRAMEWORKBASIS_H
