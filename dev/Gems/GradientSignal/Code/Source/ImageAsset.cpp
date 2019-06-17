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
#include "GradientSignal_precompiled.h"

#include <GradientSignal/ImageAsset.h>

#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Math/Vector3.h>
#include <AzCore/RTTI/ReflectContext.h>
#include <AzCore/Debug/Profiler.h>

namespace GradientSignal
{
    void ImageAsset::Reflect(AZ::ReflectContext* context)
    {
        AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context);
        if (serialize)
        {
            serialize->Class<ImageAsset, AZ::Data::AssetData>()
                ->Version(0)
                ->Field("Width", &ImageAsset::m_imageWidth)
                ->Field("Height", &ImageAsset::m_imageHeight)
                ->Field("Format", &ImageAsset::m_imageFormat)
                ->Field("Data", &ImageAsset::m_imageData)
                ;

            AZ::EditContext* edit = serialize->GetEditContext();
            if (edit)
            {
                edit->Class<ImageAsset>(
                    "Image Asset", "")
                    ->DataElement(0, &ImageAsset::m_imageWidth, "Width", "Image width.")
                    ->DataElement(0, &ImageAsset::m_imageHeight, "Height", "Image height.")
                    ->DataElement(0, &ImageAsset::m_imageFormat, "Format", "Image format.")
                    ->DataElement(0, &ImageAsset::m_imageData, "Data", "Image color data.")
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ->Attribute(AZ::Edit::Attributes::ContainerCanBeModified, true)
                    ;
            }
        }
    }

    float GetValueFromImageAsset(const AZ::Data::Asset<ImageAsset>& imageAsset, const AZ::Vector3& uvw, float tilingX, float tilingY, float defaultValue)
    {
        AZ_PROFILE_FUNCTION(AZ::Debug::ProfileCategory::Entity);

        if (imageAsset.IsReady())
        {
            const auto& image = imageAsset.Get();
            if (image->m_imageWidth > 0 &&
                image->m_imageHeight > 0 &&
                image->m_imageData.size() == image->m_imageWidth * image->m_imageHeight)
            {
                // When "rasterizing" from uvs, a range of 0-1 has slightly different meanings depending on the sampler state.
                // For repeating states (Unbounded/None, Repeat), a uv value of 1 should wrap around back to our 0th pixel.
                // For clamping states (Clamp to Zero, Clamp to Edge), a uv value of 1 should point to the last pixel.

                // We assume here that the code handling sampler states has handled this for us in the clamping cases
                // by reducing our uv by a small delta value such that anything that wants the last pixel has a value
                // just slightly less than 1.

                // Keeping that in mind, we scale our uv from 0-1 to 0-image size inclusive.  So a 4-pixel image will scale
                // uv values of 0-1 to 0-4, not 0-3 as you might expect.  This is because we want the following range mappings:
                // [0 - 1/4)   = pixel 0
                // [1/4 - 1/2) = pixel 1
                // [1/2 - 3/4) = pixel 2
                // [3/4 - 1)   = pixel 3
                // [1 - 1 1/4) = pixel 0
                // ...

                // Also, based on our tiling settings, we extend the size of our image virtually by a factor of tilingX and tilingY.  
                // A 16x16 pixel image and tilingX = tilingY = 1  maps the uv range of 0-1 to 0-16 pixels.  
                // A 16x16 pixel image and tilingX = tilingY = 1.5 maps the uv range of 0-1 to 0-24 pixels.

                const AZ::Vector3 tiledDimensions((image->m_imageWidth  * tilingX),
                                                  (image->m_imageHeight * tilingY),
                                                  0.0f);

                // Convert from uv space back to pixel space
                AZ::Vector3 pixelLookup = (uvw * tiledDimensions);

                // UVs outside the 0-1 range are treated as infinitely tiling, so that we behave the same as the 
                // other gradient generators.  As mentioned above, if clamping is desired, we expect it to be applied
                // outside of this function.
                size_t x = static_cast<size_t>(pixelLookup.GetX()) % image->m_imageWidth;
                size_t y = static_cast<size_t>(pixelLookup.GetY()) % image->m_imageHeight;

                // Flip the y because images are stored in reverse of our world axes
                size_t index = ((image->m_imageHeight - 1) - y) * image->m_imageWidth + x;

                return image->m_imageData[index] / 255.0f;
            }
        }
        return defaultValue;
    }
}