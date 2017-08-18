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

// Description : material implementation with alpha base texture acess as material
//               reflection and transparency scale

#ifndef CRYINCLUDE_TOOLS_SPHERICALHARMONICS_PRT_ALPHATEXTUREMATERIAL_H
#define CRYINCLUDE_TOOLS_SPHERICALHARMONICS_PRT_ALPHATEXTUREMATERIAL_H
#pragma once
#if defined(OFFLINE_COMPUTATION)


#include <PRT/ISHMaterial.h>


class CSimpleIndexedMesh;

namespace NSH
{
    namespace NMaterial
    {
        //!< sh-related material using the base texture as interreflection material value, without subsurface transfer, diffuse model based on incident angle (cos)
        class CAlphaTextureSHMaterial
            : public ISHMaterial
        {
        public:
            INSTALL_CLASS_NEW(CAlphaTextureSHMaterial)
            //!< constructor taking the related mesh, 32 bit loaded base texture image data
            CAlphaTextureSHMaterial
            (
                CSimpleIndexedMesh* pMesh,
                SAlphaImageValue* pImageData,
                const uint32 cImageWidth,
                const uint32 cImageHeight,
                const float cTransparencyShadowingFactor = 1.f
            );
            virtual ~CAlphaTextureSHMaterial();
            //!< sets diffuse material components
            virtual void SetDiffuseIntensity(const float cRedIntensity, const float cGreenIntensity, const float cBlueIntensity, const float cAlphaIntensity = 0.f);
            //!< returns diffuse intensity at a given angle, coloured incident intensity and surface orientation
            virtual const TRGBCoeffD DiffuseIntensity(const TRGBCoeffD& crIncidentIntensity, const uint32 cTriangleIndex, const TVec& rBaryCoord, const TCartesianCoord& rIncidentDir, const bool cApplyCosTerm = true, const bool cApplyExitanceTerm = false, const bool cAbsCosTerm = false, const bool cUseTransparency = false) const;
            virtual const TRGBCoeffD DiffuseIntensity(const TVec& crVertexPos, const TVec& crVertexNormal, const Vec2& rUVCoords, const TRGBCoeffD& crIncidentIntensity, const TCartesianCoord& rIncidentDir, const bool cApplyCosTerm = true, const bool cApplyExitanceTerm = false, const bool cAbsCosTerm = false, const bool cUseTransparency = false) const;
            //!< indicates transparency support
            virtual const bool HasTransparencyTransfer() const{return true; }
            virtual const EMaterialType Type() const{return MATERIAL_TYPE_ALPHATEXTURE; }

        private:
            CSimpleIndexedMesh* m_pMesh;        //!< indexed triangle mesh where surface data get retrieves from, indices are not shared among triangles
            SAlphaImageValue* m_pImageData; //!< associated float RGB bit raw image data
            const uint32 m_ImageWidth;          //!< image width
            const uint32 m_ImageHeight;         //!< image height
            float m_RedIntensity;                       //!< diffuse reflectance material property for red
            float m_GreenIntensity;                 //!< diffuse reflectance material property for green
            float m_BlueIntensity;                  //!< diffuse reflectance material property for blue
            float m_TransparencyShadowingFactor; //!< factor 0..1 shadowing the transparency of materials to fake density(0: complete non transparent, 1: unchanged)

            void RetrieveTexelValue(const Vec2& rUVCoords, NSH::NMaterial::SAlphaImageValue& rTexelValue) const; //!< retrieves the filtered texel value
            //!< helper function computing the outgoing intensity when cos angle positive
            const TRGBCoeffD ComputeDiffuseIntensity(const TRGBCoeffD& crIncidentIntensity, const Vec2& rUVCoords, const bool cApplyCosTerm, const bool cApplyExitanceTerm, const bool cUseTransparency, const double cCosAngle) const;
        };
    }
}

#endif
#endif // CRYINCLUDE_TOOLS_SPHERICALHARMONICS_PRT_ALPHATEXTUREMATERIAL_H
