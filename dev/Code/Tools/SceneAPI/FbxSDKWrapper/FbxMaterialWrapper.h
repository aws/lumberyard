#pragma once

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

#include <AzCore/Math/Vector3.h>

#include <memory>
#include <cassert>
#include <string>
#include "fbxsdk.h"

namespace AZ
{
    namespace FbxSDKWrapper
    {
        class FbxMaterialWrapper
        {
        public:
            enum class MaterialMapType
            {
                Diffuse,
                Specular,
                Bump
            };

            FbxMaterialWrapper(FbxSurfaceMaterial* fbxMaterial);
            virtual ~FbxMaterialWrapper();

            virtual std::string GetName() const;
            virtual std::string GetTextureFileName(const char* textureType) const;
            virtual std::string GetTextureFileName(const std::string& textureType) const;
            virtual std::string GetTextureFileName(MaterialMapType textureType) const;

            virtual AZ::Vector3 GetDiffuseColor() const;
            virtual AZ::Vector3 GetSpecularColor() const;
            virtual AZ::Vector3 GetEmissiveColor() const;
            virtual float GetOpacity() const;
            virtual float GetShininess() const;

        protected:
            FbxSurfaceMaterial* m_fbxMaterial;
        };
    }
}