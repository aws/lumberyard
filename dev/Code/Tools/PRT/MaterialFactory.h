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

#ifndef CRYINCLUDE_TOOLS_PRT_MATERIALFACTORY_H
#define CRYINCLUDE_TOOLS_PRT_MATERIALFACTORY_H
#pragma once

#if defined(OFFLINE_COMPUTATION)


#include "ISHMaterial.h"

class CSimpleIndexedMesh;

namespace NSH
{
	namespace NTransfer
	{
		struct STransferParameters;
		struct ITransferConfigurator;
	}

	namespace NMaterial
	{
		struct SSharedMaterialInitData
		{
			CSimpleIndexedMesh *pMesh;					//!< mesh associated to material
			union
			{
				TRGBCoeffF** pRGBImageData;
				NSH::NMaterial::SAlphaImageValue** pARGBImageData;
			};																	//!< optional image data associated to material
			uint32 imageWidth;									//!< optional image data width associated to material
			uint32 imageHeight;									//!< optional image data height associated to material
			Vec3	 backlightingColour;					//!< optional backlighting colour associated to material
			float transparencyShadowingFactor;	//!< factor 0..1 shadowing the transparency of materials to fake density(0: complete non transparent, 1: unchanged)
			SAlphaImageValue diffuseIntensity;	//!< additional diffuse intensity for materials
			SSharedMaterialInitData() 
				: pRGBImageData(NULL), pMesh(NULL), imageWidth(0), imageHeight(0), backlightingColour(0,0,0), transparencyShadowingFactor(1.f), diffuseIntensity(1.f,1.f,1.f,0.f){}
		};

		//!< singleton to abstract image factory
		class CMaterialFactory
		{
		public:
			//!< singleton stuff
			static CMaterialFactory* Instance();

			const NSH::CSmartPtr<NSH::NMaterial::ISHMaterial, CSHAllocator<> > GetMaterial(const EMaterialType cMaterialType, const SSharedMaterialInitData& rInitData);
			const char* GetObjExportShaderName
			(
				const NSH::NTransfer::STransferParameters& crParameters, 
				const EMaterialType cMaterialType
			);

		private:
			//!< singleton stuff
			CMaterialFactory(){}
			CMaterialFactory(const CMaterialFactory&);
			CMaterialFactory& operator= (const CMaterialFactory&);
		};
	}
}

#endif

#endif // CRYINCLUDE_TOOLS_PRT_MATERIALFACTORY_H
