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
#pragma once

#include <AzCore/Component/ComponentBus.h>
#include <AzCore/Math/Color.h>

class ISprite;

////////////////////////////////////////////////////////////////////////////////////////////////////
class UiImageInterface
    : public AZ::ComponentBus
{
public: // types

    enum class ImageType
    {
        Stretched,      //!< the texture is stretched to fit the rect without maintaining aspect ratio
        Sliced,         //!< the texture is sliced such that center stretches and the edges do not
        Fixed,          //!< the texture is not stretched at all
        Tiled,          //!< the texture is tiled (repeated)
        StretchedToFit, //!< the texture is scaled to fit the rect while maintaining aspect ratio
        StretchedToFill //!< the texture is scaled to fill the rect while maintaining aspect ratio
    };

    enum class SpriteType
    {
        SpriteAsset,
        RenderTarget
    };

public: // member functions

    virtual ~UiImageInterface() {}

    virtual AZ::Color GetColor() = 0;
    virtual void SetColor(const AZ::Color& color) = 0;

    virtual ISprite* GetSprite() = 0;
    virtual void SetSprite(ISprite* sprite) = 0;

    virtual AZStd::string GetSpritePathname() = 0;
    virtual void SetSpritePathname(AZStd::string spritePath) = 0;

    virtual AZStd::string GetRenderTargetName() = 0;
    virtual void SetRenderTargetName(AZStd::string renderTargetName) = 0;

    virtual SpriteType GetSpriteType() = 0;
    virtual void SetSpriteType(SpriteType spriteType) = 0;

    virtual ImageType GetImageType() = 0;
    virtual void SetImageType(ImageType imageType) = 0;

public: // static member data

    //! Only one component on a entity can implement the events
    static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;
};

typedef AZ::EBus<UiImageInterface> UiImageBus;

