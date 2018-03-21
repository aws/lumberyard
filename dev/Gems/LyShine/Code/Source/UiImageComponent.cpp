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
#include "LyShine_precompiled.h"
#include "UiImageComponent.h"

#include <AzCore/Math/Crc.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/Component/ComponentApplicationBus.h>

#include <IRenderer.h>

#include <LyShine/IDraw2d.h>
#include <LyShine/UiSerializeHelpers.h>
#include <LyShine/Bus/UiElementBus.h>
#include <LyShine/Bus/UiCanvasBus.h>
#include <LyShine/Bus/UiTransformBus.h>
#include <LyShine/Bus/UiTransform2dBus.h>
#include <LyShine/Bus/UiLayoutManagerBus.h>
#include <LyShine/Bus/UiLayoutControllerBus.h>
#include <LyShine/Bus/UiEditorChangeNotificationBus.h>
#include <LyShine/ISprite.h>
#include <LyShine/IUiRenderer.h>

#include "UiSerialize.h"
#include "Sprite.h"
#include "UiLayoutHelpers.h"

namespace
{
    //! Given a sprite with a cell index, populates the UV/ST coords array for 9-sliced image types.
    //!
    //! It's assumed that the given right and bottom border values have
    //! already had a "correction scaling" applied. This scaling gets applied
    //! when the left/right and/or top/bottom border sizes are greater than
    //! the image element's width or height (respectively).
    //!
    //! \param sValues Output array of U/S texture coords for 9-sliced image.
    //!
    //! \param tValues Output array of V/T texture coords for 9-sliced image.
    //!
    //! \param sprite Pointer to sprite  object info.
    //!
    //! \param cellIndex Index of a particular cell in sprite-sheet, if applicable.
    //!
    //! \param rightBorder One minus the "right" border info for the cell, with "correction scaling" applied.
    //!
    //! \param bottomBorder Same as rightBorder, but for bottom cell border value.
    void GetSlicedStValuesFromCorrectionalScaleBorders(
        float sValues[4],
        float tValues[4],
        const ISprite* sprite,
        const int cellIndex,
        const float rightBorder,
        const float bottomBorder)
    {
        const float cellMinUCoord = sprite->GetCellUvCoords(cellIndex).TopLeft().GetX();
        const float cellMaxUCoord = sprite->GetCellUvCoords(cellIndex).TopRight().GetX();
        const float cellMinVCoord = sprite->GetCellUvCoords(cellIndex).TopLeft().GetY();
        const float cellMaxVCoord = sprite->GetCellUvCoords(cellIndex).BottomLeft().GetY();

        // Transform border values from cell space to texture space
        const AZ::Vector2 cellUvSize = sprite->GetCellUvSize(cellIndex);
        const ISprite::Borders spriteBorders = sprite->GetCellUvBorders(cellIndex);
        const float leftBorderTextureSpace = spriteBorders.m_left * cellUvSize.GetX();
        const float rightBorderTextureSpace = (1.0f - rightBorder) * cellUvSize.GetX();
        const float topBorderTextureSpace = spriteBorders.m_top * cellUvSize.GetY();
        const float bottomBorderTextureSpace = (1.0f - bottomBorder) * cellUvSize.GetY();

        // The texture coords are just based on the border values
        int uIndex = 0;
        sValues[uIndex++] = cellMinUCoord;
        sValues[uIndex++] = cellMinUCoord + leftBorderTextureSpace;
        sValues[uIndex++] = cellMinUCoord + rightBorderTextureSpace;
        sValues[uIndex++] = cellMaxUCoord;

        int vIndex = 0;
        tValues[vIndex++] = cellMinVCoord;
        tValues[vIndex++] = cellMinVCoord + topBorderTextureSpace;
        tValues[vIndex++] = cellMinVCoord + bottomBorderTextureSpace;
        tValues[vIndex++] = cellMaxVCoord;
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// PUBLIC MEMBER FUNCTIONS
////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////
UiImageComponent::UiImageComponent()
    : m_isColorOverridden(false)
    , m_isAlphaOverridden(false)
{
}

////////////////////////////////////////////////////////////////////////////////////////////////////
UiImageComponent::~UiImageComponent()
{
    SAFE_RELEASE(m_sprite);
    SAFE_RELEASE(m_overrideSprite);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiImageComponent::ResetOverrides()
{
    m_overrideColor = m_color;
    m_overrideAlpha = m_alpha;
    SAFE_RELEASE(m_overrideSprite);

    m_isColorOverridden = false;
    m_isAlphaOverridden = false;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiImageComponent::SetOverrideColor(const AZ::Color& color)
{
    m_overrideColor.Set(color.GetAsVector3());

    m_isColorOverridden = true;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiImageComponent::SetOverrideAlpha(float alpha)
{
    m_overrideAlpha = alpha;

    m_isAlphaOverridden = true;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiImageComponent::SetOverrideSprite(ISprite* sprite, AZ::u32 cellIndex)
{
    SAFE_RELEASE(m_overrideSprite);
    m_overrideSprite = sprite;

    if (m_overrideSprite)
    {
        m_overrideSprite->AddRef();
        m_overrideSpriteCellIndex = cellIndex;
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiImageComponent::Render()
{
    const char* profileMarker = "UI_IMAGE";
    gEnv->pRenderer->PushProfileMarker(profileMarker);

    // get fade value (tracked by UiRenderer)
    float fade = IUiRenderer::Get()->GetAlphaFade();

    ISprite* sprite = (m_overrideSprite) ? m_overrideSprite : m_sprite;
    int cellIndex = (m_overrideSprite) ? m_overrideSpriteCellIndex : m_spriteSheetCellIndex;

    ImageType imageType = m_imageType;

    // if there is no texture we will just use a white texture and want to stretch it
    const bool spriteOrTextureIsNull = sprite == nullptr || sprite->GetTexture() == nullptr;

    // if the borders are zero width then sliced is the same as stretched and stretched is simpler to render
    const bool spriteIsSlicedAndBordersAreZeroWidth = imageType == ImageType::Sliced && sprite && sprite->AreCellBordersZeroWidth(cellIndex);

    if (spriteOrTextureIsNull || spriteIsSlicedAndBordersAreZeroWidth)
    {
        imageType = ImageType::Stretched;
    }

    switch (imageType)
    {
    case ImageType::Stretched:
        RenderStretchedSprite(sprite, cellIndex, fade);
        break;
    case ImageType::Sliced:
        AZ_Assert(sprite, "Should not get here if no sprite path is specified");
        RenderSlicedSprite(sprite, cellIndex, fade);   // will not get here if sprite is null since we change type in that case above
        break;
    case ImageType::Fixed:
        AZ_Assert(sprite, "Should not get here if no sprite path is specified");
        RenderFixedSprite(sprite, cellIndex, fade);
        break;
    case ImageType::Tiled:
        AZ_Assert(sprite, "Should not get here if no sprite path is specified");
        RenderTiledSprite(sprite, fade);
        break;
    case ImageType::StretchedToFit:
        AZ_Assert(sprite, "Should not get here if no sprite path is specified");
        RenderStretchedToFitOrFillSprite(sprite, cellIndex, fade, true);
        break;
    case ImageType::StretchedToFill:
        AZ_Assert(sprite, "Should not get here if no sprite path is specified");
        RenderStretchedToFitOrFillSprite(sprite, cellIndex, fade, false);
        break;
    }

    gEnv->pRenderer->PopProfileMarker(profileMarker);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
AZ::Color UiImageComponent::GetColor()
{
    return AZ::Color::CreateFromVector3AndFloat(m_color.GetAsVector3(), m_alpha);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiImageComponent::SetColor(const AZ::Color& color)
{
    m_color.Set(color.GetAsVector3());
    m_alpha = color.GetA();
    if (!m_isColorOverridden)
    {
        m_overrideColor = m_color;
    }
    if (!m_isAlphaOverridden)
    {
        m_overrideAlpha = m_alpha;
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
ISprite* UiImageComponent::GetSprite()
{
    return m_sprite;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiImageComponent::SetSprite(ISprite* sprite)
{
    if (m_sprite)
    {
        m_sprite->Release();
        m_spritePathname.SetAssetPath("");
    }

    m_sprite = sprite;

    if (m_sprite)
    {
        m_sprite->AddRef();
        m_spritePathname.SetAssetPath(m_sprite->GetPathname().c_str());
    }

    InvalidateLayouts();
}

////////////////////////////////////////////////////////////////////////////////////////////////////
AZStd::string UiImageComponent::GetSpritePathname()
{
    return m_spritePathname.GetAssetPath();
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiImageComponent::SetSpritePathname(AZStd::string spritePath)
{
    m_spritePathname.SetAssetPath(spritePath.c_str());

    if (IsSpriteTypeAsset())
    {
        OnSpritePathnameChange();
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
AZStd::string UiImageComponent::GetRenderTargetName()
{
    return m_renderTargetName;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiImageComponent::SetRenderTargetName(AZStd::string renderTargetName)
{
    m_renderTargetName = renderTargetName;

    if (m_spriteType == UiImageInterface::SpriteType::RenderTarget)
    {
        OnSpriteRenderTargetNameChange();
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
UiImageInterface::SpriteType UiImageComponent::GetSpriteType()
{
    return m_spriteType;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiImageComponent::SetSpriteType(SpriteType spriteType)
{
    m_spriteType = spriteType;

    OnSpriteTypeChange();
}

////////////////////////////////////////////////////////////////////////////////////////////////////
UiImageInterface::ImageType UiImageComponent::GetImageType()
{
    return m_imageType;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiImageComponent::SetImageType(ImageType imageType)
{
    if (imageType == ImageType::Fixed && m_imageType != imageType)
    {
        SnapOffsetsToFixedImage();
    }

    m_imageType = imageType;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiImageComponent::SetSpriteSheetCellIndex(AZ::u32 index)
{
    m_spriteSheetCellIndex = index;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
const AZ::u32 UiImageComponent::GetSpriteSheetCellIndex()
{
    return m_spriteSheetCellIndex;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
const AZ::u32 UiImageComponent::GetSpriteSheetCellCount()
{
    if (m_sprite)
    {
        return m_sprite->GetSpriteSheetCells().size();
    }

    return 0;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
UiImageInterface::FillType UiImageComponent::GetFillType()
{
    return m_fillType;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiImageComponent::SetFillType(UiImageInterface::FillType fillType)
{
    m_fillType = fillType;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
float UiImageComponent::GetFillAmount()
{
    return m_fillAmount;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiImageComponent::SetFillAmount(float fillAmount)
{
    m_fillAmount = AZ::GetClamp(fillAmount, 0.0f, 1.0f);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
float UiImageComponent::GetRadialFillStartAngle()
{
    return m_fillStartAngle;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiImageComponent::SetRadialFillStartAngle(float radialFillStartAngle)
{
    m_fillStartAngle = radialFillStartAngle;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
UiImageInterface::FillCornerOrigin UiImageComponent::GetCornerFillOrigin()
{
    return m_fillCornerOrigin;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiImageComponent::SetCornerFillOrigin(UiImageInterface::FillCornerOrigin cornerOrigin)
{
    m_fillCornerOrigin = cornerOrigin;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
UiImageInterface::FillEdgeOrigin UiImageComponent::GetEdgeFillOrigin()
{
    return m_fillEdgeOrigin;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiImageComponent::SetEdgeFillOrigin(UiImageInterface::FillEdgeOrigin edgeOrigin)
{
    m_fillEdgeOrigin = edgeOrigin;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
bool UiImageComponent::GetFillClockwise()
{
    return m_fillClockwise;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiImageComponent::SetFillClockwise(bool fillClockwise)
{
    m_fillClockwise = fillClockwise;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
bool UiImageComponent::GetFillCenter()
{
    return m_fillCenter;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiImageComponent::SetFillCenter(bool fillCenter)
{
    m_fillCenter = fillCenter;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
AZStd::string UiImageComponent::GetSpriteSheetCellAlias(AZ::u32 index)
{
    return m_sprite ? m_sprite->GetCellAlias(index) : AZStd::string();
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiImageComponent::SetSpriteSheetCellAlias(AZ::u32 index, const AZStd::string& alias)
{
    m_sprite ? m_sprite->SetCellAlias(index, alias) : AZ_UNUSED(0);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
AZ::u32 UiImageComponent::GetSpriteSheetCellIndexFromAlias(const AZStd::string& alias)
{
    return m_sprite ? m_sprite->GetCellIndexFromAlias(alias) : 0;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiImageComponent::PropertyValuesChanged()
{
    if (!m_isColorOverridden)
    {
        m_overrideColor = m_color;
    }
    if (!m_isAlphaOverridden)
    {
        m_overrideAlpha = m_alpha;
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
float UiImageComponent::GetMinWidth()
{
    return 0.0f;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
float UiImageComponent::GetMinHeight()
{
    return 0.0f;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
float UiImageComponent::GetTargetWidth()
{
    float targetWidth = 0.0f;

    ITexture* texture = (m_sprite) ? m_sprite->GetTexture() : nullptr;
    if (texture)
    {
        switch (m_imageType)
        {
        case ImageType::Fixed:
        {
            AZ::Vector2 textureSize = m_sprite->GetSize();
            targetWidth = textureSize.GetX();
        }
        break;

        case ImageType::Sliced:
        {
            AZ::Vector2 textureSize = m_sprite->GetSize();
            targetWidth = (m_sprite->GetBorders().m_left * textureSize.GetX()) + ((1.0f - m_sprite->GetBorders().m_right) * textureSize.GetX());
        }
        break;

        default:
        {
            targetWidth = 0.0f;
        }
        break;
        }
    }

    return targetWidth;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
float UiImageComponent::GetTargetHeight()
{
    float targetHeight = 0.0f;

    ITexture* texture = (m_sprite) ? m_sprite->GetTexture() : nullptr;
    if (texture)
    {
        switch (m_imageType)
        {
        case ImageType::Fixed:
        {
            AZ::Vector2 textureSize = m_sprite->GetSize();
            targetHeight = textureSize.GetY();
        }
        break;

        case ImageType::Sliced:
        {
            AZ::Vector2 textureSize = m_sprite->GetSize();
            targetHeight = (m_sprite->GetBorders().m_top * textureSize.GetY()) + ((1.0f - m_sprite->GetBorders().m_bottom) * textureSize.GetY());
        }
        break;

        case ImageType::StretchedToFit:
        {
            AZ::Vector2 textureSize = m_sprite->GetSize();
            if (textureSize.GetX() > 0.0f)
            {
                // Get element size
                AZ::Vector2 size(0.0f, 0.0f);
                EBUS_EVENT_ID_RESULT(size, GetEntityId(), UiTransformBus, GetCanvasSpaceSizeNoScaleRotate);

                targetHeight = textureSize.GetY() * (size.GetX() / textureSize.GetX());
            }
            else
            {
                targetHeight = 0.0f;
            }
        }
        break;

        default:
        {
            targetHeight = 0.0f;
        }
        break;
        }
    }

    return targetHeight;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
float UiImageComponent::GetExtraWidthRatio()
{
    return 1.0f;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
float UiImageComponent::GetExtraHeightRatio()
{
    return 1.0f;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// PUBLIC STATIC MEMBER FUNCTIONS
////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiImageComponent::Reflect(AZ::ReflectContext* context)
{
    AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);

    if (serializeContext)
    {
        serializeContext->Class<UiImageComponent, AZ::Component>()
            ->Version(7, &VersionConverter)
            ->Field("SpriteType", &UiImageComponent::m_spriteType)
            ->Field("SpritePath", &UiImageComponent::m_spritePathname)
            ->Field("Index", &UiImageComponent::m_spriteSheetCellIndex)
            ->Field("RenderTargetName", &UiImageComponent::m_renderTargetName)
            ->Field("Color", &UiImageComponent::m_color)
            ->Field("Alpha", &UiImageComponent::m_alpha)
            ->Field("ImageType", &UiImageComponent::m_imageType)
            ->Field("FillCenter", &UiImageComponent::m_fillCenter)
            ->Field("BlendMode", &UiImageComponent::m_blendMode)
            ->Field("FillType", &UiImageComponent::m_fillType)
            ->Field("FillAmount", &UiImageComponent::m_fillAmount)
            ->Field("FillStartAngle", &UiImageComponent::m_fillStartAngle)
            ->Field("FillCornerOrigin", &UiImageComponent::m_fillCornerOrigin)
            ->Field("FillEdgeOrigin", &UiImageComponent::m_fillEdgeOrigin)
            ->Field("FillClockwise", &UiImageComponent::m_fillClockwise);

        AZ::EditContext* ec = serializeContext->GetEditContext();
        if (ec)
        {
            auto editInfo = ec->Class<UiImageComponent>("Image", "A visual component to draw a rectangle with an optional sprite/texture");

            editInfo->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                ->Attribute(AZ::Edit::Attributes::Icon, "Editor/Icons/Components/UiImage.png")
                ->Attribute(AZ::Edit::Attributes::ViewportIcon, "Editor/Icons/Components/Viewport/UiImage.png")
                ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC("UI", 0x27ff46b0))
                ->Attribute(AZ::Edit::Attributes::AutoExpand, true);

            editInfo->DataElement(AZ::Edit::UIHandlers::ComboBox, &UiImageComponent::m_spriteType, "SpriteType", "The sprite type.")
                ->EnumAttribute(UiImageInterface::SpriteType::SpriteAsset, "Sprite/Texture asset")
                ->EnumAttribute(UiImageInterface::SpriteType::RenderTarget, "Render target")
                ->Attribute(AZ::Edit::Attributes::ChangeNotify, &UiImageComponent::OnSpriteTypeChange)
                ->Attribute(AZ::Edit::Attributes::ChangeNotify, &UiImageComponent::CheckLayoutFitterAndRefreshEditorTransformProperties)
                ->Attribute(AZ::Edit::Attributes::ChangeNotify, AZ_CRC("RefreshEntireTree", 0xefbc823c));
            editInfo->DataElement("Sprite", &UiImageComponent::m_spritePathname, "Sprite path", "The sprite path. Can be overridden by another component such as an interactable.")
                ->Attribute(AZ::Edit::Attributes::Visibility, &UiImageComponent::IsSpriteTypeAsset)
                ->Attribute(AZ::Edit::Attributes::ChangeNotify, &UiImageComponent::OnEditorSpritePathnameChange)
                ->Attribute(AZ::Edit::Attributes::ChangeNotify, AZ_CRC("RefreshEntireTree", 0xefbc823c))
                ->Attribute(AZ::Edit::Attributes::ChangeNotify, &UiImageComponent::CheckLayoutFitterAndRefreshEditorTransformProperties);
            editInfo->DataElement(AZ::Edit::UIHandlers::ComboBox, &UiImageComponent::m_spriteSheetCellIndex, "Index", "Sprite-sheet index. Defines which cell in a sprite-sheet is displayed.")
                ->Attribute(AZ::Edit::Attributes::Visibility, &UiImageComponent::IsSpriteTypeSpriteSheet)
                ->Attribute(AZ::Edit::Attributes::ChangeNotify, &UiImageComponent::OnIndexChange)
                ->Attribute("EnumValues", &UiImageComponent::PopulateIndexStringList);
            editInfo->DataElement(0, &UiImageComponent::m_renderTargetName, "Render target name", "The name of the render target associated with the sprite.")
                ->Attribute(AZ::Edit::Attributes::Visibility, &UiImageComponent::IsSpriteTypeRenderTarget)
                ->Attribute(AZ::Edit::Attributes::ChangeNotify, &UiImageComponent::OnSpriteRenderTargetNameChange);
            editInfo->DataElement(AZ::Edit::UIHandlers::Color, &UiImageComponent::m_color, "Color", "The color tint for the image. Can be overridden by another component such as an interactable.")
                ->Attribute(AZ::Edit::Attributes::ChangeNotify, &UiImageComponent::OnColorChange);
            editInfo->DataElement(AZ::Edit::UIHandlers::Slider, &UiImageComponent::m_alpha, "Alpha", "The transparency. Can be overridden by another component such as an interactable.")
                ->Attribute(AZ::Edit::Attributes::ChangeNotify, &UiImageComponent::OnColorChange)
                ->Attribute(AZ::Edit::Attributes::Min, 0.0f)
                ->Attribute(AZ::Edit::Attributes::Max, 1.0f);
            editInfo->DataElement(AZ::Edit::UIHandlers::ComboBox, &UiImageComponent::m_imageType, "ImageType", "The image type. Affects how the texture/sprite is mapped to the image rectangle.")
                ->EnumAttribute(UiImageInterface::ImageType::Stretched, "Stretched")
                ->EnumAttribute(UiImageInterface::ImageType::Sliced, "Sliced")
                ->EnumAttribute(UiImageInterface::ImageType::Fixed, "Fixed")
                ->EnumAttribute(UiImageInterface::ImageType::Tiled, "Tiled")
                ->EnumAttribute(UiImageInterface::ImageType::StretchedToFit, "Stretched To Fit")
                ->EnumAttribute(UiImageInterface::ImageType::StretchedToFill, "Stretched To Fill")
                ->Attribute(AZ::Edit::Attributes::ChangeNotify, AZ_CRC("RefreshEntireTree", 0xefbc823c))
                ->Attribute(AZ::Edit::Attributes::ChangeNotify, &UiImageComponent::InvalidateLayouts)
                ->Attribute(AZ::Edit::Attributes::ChangeNotify, &UiImageComponent::CheckLayoutFitterAndRefreshEditorTransformProperties);
            editInfo->DataElement(AZ::Edit::UIHandlers::CheckBox, &UiImageComponent::m_fillCenter, "Fill Center", "Sliced image center is filled.")
                ->Attribute(AZ::Edit::Attributes::Visibility, &UiImageComponent::IsSliced);
            editInfo->DataElement(AZ::Edit::UIHandlers::ComboBox, &UiImageComponent::m_blendMode, "BlendMode", "The blend mode used to draw the image")
                ->EnumAttribute(LyShine::BlendMode::Normal, "Normal")
                ->EnumAttribute(LyShine::BlendMode::Add, "Add")
                ->EnumAttribute(LyShine::BlendMode::Screen, "Screen")
                ->EnumAttribute(LyShine::BlendMode::Darken, "Darken")
                ->EnumAttribute(LyShine::BlendMode::Lighten, "Lighten");
            editInfo->DataElement(AZ::Edit::UIHandlers::ComboBox, &UiImageComponent::m_fillType, "Fill Type", "The fill style used to draw the image.")
                ->EnumAttribute(UiImageComponent::FillType::None, "None")
                ->EnumAttribute(UiImageComponent::FillType::Linear, "Linear")
                ->EnumAttribute(UiImageComponent::FillType::Radial, "Radial")
                ->EnumAttribute(UiImageComponent::FillType::RadialCorner, "RadialCorner")
                ->EnumAttribute(UiImageComponent::FillType::RadialEdge, "RadialEdge")
                ->Attribute(AZ::Edit::Attributes::ChangeNotify, AZ_CRC("RefreshEntireTree", 0xefbc823c));
            editInfo->DataElement(AZ::Edit::UIHandlers::Slider, &UiImageComponent::m_fillAmount, "Fill Amount", "The amount of the image to be filled.")
                ->Attribute(AZ::Edit::Attributes::Visibility, &UiImageComponent::IsFilled)
                ->Attribute(AZ::Edit::Attributes::Min, 0.0f)
                ->Attribute(AZ::Edit::Attributes::Max, 1.0f);
            editInfo->DataElement(AZ::Edit::UIHandlers::Slider, &UiImageComponent::m_fillStartAngle, "Fill Start Angle", "The start angle for the fill in degrees measured clockwise from straight up.")
                ->Attribute(AZ::Edit::Attributes::Visibility, &UiImageComponent::IsRadialFilled)
                ->Attribute(AZ::Edit::Attributes::Step, 0.1f)
                ->Attribute(AZ::Edit::Attributes::Suffix, " degrees")
                ->Attribute(AZ::Edit::Attributes::Min, 0.0f)
                ->Attribute(AZ::Edit::Attributes::Max, 360.0f);
            editInfo->DataElement(AZ::Edit::UIHandlers::ComboBox, &UiImageComponent::m_fillCornerOrigin, "Corner Fill Origin", "The corner from which the image is filled.")
                ->Attribute(AZ::Edit::Attributes::Visibility, &UiImageComponent::IsCornerFilled)
                ->EnumAttribute(UiImageComponent::FillCornerOrigin::TopLeft, "TopLeft")
                ->EnumAttribute(UiImageComponent::FillCornerOrigin::TopRight, "TopRight")
                ->EnumAttribute(UiImageComponent::FillCornerOrigin::BottomRight, "BottomRight")
                ->EnumAttribute(UiImageComponent::FillCornerOrigin::BottomLeft, "BottomLeft");
            editInfo->DataElement(AZ::Edit::UIHandlers::ComboBox, &UiImageComponent::m_fillEdgeOrigin, "Edge Fill Origin", "The edge from which the image is filled.")
                ->Attribute(AZ::Edit::Attributes::Visibility, &UiImageComponent::IsEdgeFilled)
                ->EnumAttribute(UiImageComponent::FillEdgeOrigin::Left, "Left")
                ->EnumAttribute(UiImageComponent::FillEdgeOrigin::Top, "Top")
                ->EnumAttribute(UiImageComponent::FillEdgeOrigin::Right, "Right")
                ->EnumAttribute(UiImageComponent::FillEdgeOrigin::Bottom, "Bottom");
            editInfo->DataElement(AZ::Edit::UIHandlers::CheckBox, &UiImageComponent::m_fillClockwise, "Fill Clockwise", "Image is filled clockwise about the origin.")
                ->Attribute(AZ::Edit::Attributes::Visibility, &UiImageComponent::IsRadialAnyFilled);
        }
    }

    AZ::BehaviorContext* behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context);
    if (behaviorContext)
    {
        behaviorContext->Enum<(int)UiImageInterface::ImageType::Stretched>("eUiImageType_Stretched")
            ->Enum<(int)UiImageInterface::ImageType::Sliced>("eUiImageType_Sliced")
            ->Enum<(int)UiImageInterface::ImageType::Fixed>("eUiImageType_Fixed")
            ->Enum<(int)UiImageInterface::ImageType::Tiled>("eUiImageType_Tiled")
            ->Enum<(int)UiImageInterface::ImageType::StretchedToFit>("eUiImageType_StretchedToFit")
            ->Enum<(int)UiImageInterface::ImageType::StretchedToFill>("eUiImageType_StretchedToFill")
            ->Enum<(int)UiImageInterface::SpriteType::SpriteAsset>("eUiSpriteType_SpriteAsset")
            ->Enum<(int)UiImageInterface::SpriteType::RenderTarget>("eUiSpriteType_RenderTarget")
            ->Enum<(int)UiImageInterface::FillType::None>("eUiFillType_None")
            ->Enum<(int)UiImageInterface::FillType::Linear>("eUiFillType_Linear")
            ->Enum<(int)UiImageInterface::FillType::Radial>("eUiFillType_Radial")
            ->Enum<(int)UiImageInterface::FillType::RadialCorner>("eUiFillType_RadialCorner")
            ->Enum<(int)UiImageInterface::FillType::RadialEdge>("eUiFillType_RadialEdge")
            ->Enum<(int)UiImageInterface::FillCornerOrigin::TopLeft>("eUiFillCornerOrigin_TopLeft")
            ->Enum<(int)UiImageInterface::FillCornerOrigin::TopRight>("eUiFillCornerOrigin_TopRight")
            ->Enum<(int)UiImageInterface::FillCornerOrigin::BottomRight>("eUiFillCornerOrigin_BottomRight")
            ->Enum<(int)UiImageInterface::FillCornerOrigin::BottomLeft>("eUiFillCornerOrigin_BottomLeft")
            ->Enum<(int)UiImageInterface::FillEdgeOrigin::Left>("eUiFillEdgeOrigin_Left")
            ->Enum<(int)UiImageInterface::FillEdgeOrigin::Top>("eUiFillEdgeOrigin_Top")
            ->Enum<(int)UiImageInterface::FillEdgeOrigin::Right>("eUiFillEdgeOrigin_Right")
            ->Enum<(int)UiImageInterface::FillEdgeOrigin::Bottom>("eUiFillEdgeOrigin_Bottom");

        behaviorContext->EBus<UiImageBus>("UiImageBus")
            ->Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::Preview)
            ->Event("GetColor", &UiImageBus::Events::GetColor)
            ->Event("SetColor", &UiImageBus::Events::SetColor)
            ->Event("GetSpritePathname", &UiImageBus::Events::GetSpritePathname)
            ->Event("SetSpritePathname", &UiImageBus::Events::SetSpritePathname)
            ->Event("GetRenderTargetName", &UiImageBus::Events::GetRenderTargetName)
            ->Event("SetRenderTargetName", &UiImageBus::Events::SetRenderTargetName)
            ->Event("GetSpriteType", &UiImageBus::Events::GetSpriteType)
            ->Event("SetSpriteType", &UiImageBus::Events::SetSpriteType)
            ->Event("GetImageType", &UiImageBus::Events::GetImageType)
            ->Event("SetImageType", &UiImageBus::Events::SetImageType)
            ->Event("GetSpriteSheetCellIndex", &UiImageBus::Events::GetSpriteSheetCellIndex)
            ->Event("SetSpriteSheetCellIndex", &UiImageBus::Events::SetSpriteSheetCellIndex)
            ->Event("GetFillType", &UiImageBus::Events::GetFillType)
            ->Event("SetFillType", &UiImageBus::Events::SetFillType)
            ->Event("GetFillAmount", &UiImageBus::Events::GetFillAmount)
            ->Event("SetFillAmount", &UiImageBus::Events::SetFillAmount)
            ->Event("GetRadialFillStartAngle", &UiImageBus::Events::GetRadialFillStartAngle)
            ->Event("SetRadialFillStartAngle", &UiImageBus::Events::SetRadialFillStartAngle)
            ->Event("GetCornerFillOrigin", &UiImageBus::Events::GetCornerFillOrigin)
            ->Event("SetCornerFillOrigin", &UiImageBus::Events::SetCornerFillOrigin)
            ->Event("GetEdgeFillOrigin", &UiImageBus::Events::GetEdgeFillOrigin)
            ->Event("SetEdgeFillOrigin", &UiImageBus::Events::SetEdgeFillOrigin)
            ->Event("GetFillClockwise", &UiImageBus::Events::GetFillClockwise)
            ->Event("SetFillClockwise", &UiImageBus::Events::SetFillClockwise)
            ->Event("GetFillCenter", &UiImageBus::Events::GetFillCenter)
            ->Event("SetFillCenter", &UiImageBus::Events::SetFillCenter)
            ->Event("GetSpriteSheetCellAlias", &UiImageBus::Events::GetSpriteSheetCellAlias)
            ->Event("SetSpriteSheetCellAlias", &UiImageBus::Events::SetSpriteSheetCellAlias)
            ->Event("GetSpriteSheetCellIndexFromAlias", &UiImageBus::Events::GetSpriteSheetCellIndexFromAlias)
            ->VirtualProperty("Color", "GetColor", "SetColor");
        behaviorContext->Class<UiImageComponent>()->RequestBus("UiImageBus");
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
UiImageComponent::AZu32ComboBoxVec UiImageComponent::GetEnumSpriteIndexList(AZ::u32 indexMin, AZ::u32 indexMax, ISprite* sprite, const char* errorMessage)
{
    AZu32ComboBoxVec indexStringComboVec;

    if (sprite && indexMin < indexMax)
    {
        for (AZ::u32 i = indexMin; i <= indexMax; ++i)
        {
            AZStd::string cellAlias = sprite->GetCellAlias(i);
            AZStd::string indexStrBuffer;

            if (cellAlias.empty())
            {
                indexStrBuffer = AZStd::string::format("%d", i);
            }
            else
            {
                indexStrBuffer = AZStd::string::format("%d (%s)", i, cellAlias.c_str());
            }

            indexStringComboVec.push_back(
                AZStd::make_pair(
                    i,
                    indexStrBuffer
                    ));
        }
    }
    else
    {
        // Add an empty element to prevent an AzToolsFramework warning that fires
        // when an empty container is encountered.
        indexStringComboVec.push_back(AZStd::make_pair(0, errorMessage));
    }

    return indexStringComboVec;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// PROTECTED MEMBER FUNCTIONS
////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiImageComponent::Init()
{
    // If this is called from RC.exe for example these pointers will not be set. In that case
    // we only need to be able to load, init and save the component. It will never be
    // activated.
    if (!(gEnv && gEnv->pLyShine))
    {
        return;
    }

    // This supports serialization. If we have a sprite pathname but no sprite is loaded
    // then load the sprite
    if (!m_sprite)
    {
        if (IsSpriteTypeAsset())
        {
            if (!m_spritePathname.GetAssetPath().empty())
            {
                m_sprite = gEnv->pLyShine->LoadSprite(m_spritePathname.GetAssetPath().c_str());
            }
        }
        else if (m_spriteType == UiImageInterface::SpriteType::RenderTarget)
        {
            if (!m_renderTargetName.empty())
            {
                m_sprite = gEnv->pLyShine->CreateSprite(m_renderTargetName.c_str());
            }
        }
        else
        {
            AZ_Assert(false, "unhandled sprite type");
        }
    }

    m_overrideColor = m_color;
    m_overrideAlpha = m_alpha;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiImageComponent::Activate()
{
    UiVisualBus::Handler::BusConnect(m_entity->GetId());
    UiRenderBus::Handler::BusConnect(m_entity->GetId());
    UiImageBus::Handler::BusConnect(m_entity->GetId());
    UiAnimateEntityBus::Handler::BusConnect(m_entity->GetId());
    UiLayoutCellDefaultBus::Handler::BusConnect(m_entity->GetId());
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiImageComponent::Deactivate()
{
    UiVisualBus::Handler::BusDisconnect();
    UiRenderBus::Handler::BusDisconnect();
    UiImageBus::Handler::BusDisconnect();
    UiAnimateEntityBus::Handler::BusDisconnect();
    UiLayoutCellDefaultBus::Handler::BusDisconnect();
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiImageComponent::ResetSpriteSheetCellIndex()
{
    m_spriteSheetCellIndex = 0;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// PRIVATE MEMBER FUNCTIONS
////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiImageComponent::RenderStretchedSprite(ISprite* sprite, int cellIndex, float fade)
{
    UiTransformInterface::RectPoints points;
    EBUS_EVENT_ID(GetEntityId(), UiTransformBus, GetViewportSpacePoints, points);

    ITexture* texture = (sprite) ? sprite->GetTexture() : nullptr;

    if (!texture)
    {
        texture = gEnv->pRenderer->EF_GetTextureByID(gEnv->pRenderer->GetWhiteTextureId());
    }

    texture->SetClamp(true);

    if (sprite)
    {
        const UiTransformInterface::RectPoints& uvCoords = sprite->GetCellUvCoords(cellIndex);
        const AZ::Vector2 uvs[4] =
        {
            uvCoords.TopLeft(),
            uvCoords.TopRight(),
            uvCoords.BottomRight(),
            uvCoords.BottomLeft(),
        };
        if (m_fillType != FillType::None)
        {
            RenderFilledQuad(texture, points.pt, uvs, fade);
        }
        else
        {
            RenderSingleQuad(texture, points.pt, uvs, fade);
        }
    }
    else
    {
        // points are a clockwise quad
        static const AZ::Vector2 uvs[4] = { AZ::Vector2(0, 0), AZ::Vector2(1, 0), AZ::Vector2(1, 1), AZ::Vector2(0, 1) };
        if (m_fillType != FillType::None)
        {
            RenderFilledQuad(texture, points.pt, uvs, fade);
        }
        else
        {
            RenderSingleQuad(texture, points.pt, uvs, fade);
        }
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiImageComponent::RenderSlicedSprite(ISprite* sprite, int cellIndex, float fade)
{
    ITexture* texture = sprite->GetTexture();
    texture->SetClamp(true);

    // get the details of the texture
    AZ::Vector2 textureSize(sprite->GetCellSize(cellIndex));

    // get the untransformed rect for the element plus it's transform matrix
    UiTransformInterface::RectPoints points;
    EBUS_EVENT_ID(GetEntityId(), UiTransformBus, GetCanvasSpacePointsNoScaleRotate, points);
    AZ::Matrix4x4 transform;
    EBUS_EVENT_ID(GetEntityId(), UiTransformBus, GetTransformToViewport, transform);

    // incorporate the fade value into the vertex colors
    AZ::Color color = AZ::Color::CreateFromVector3AndFloat(m_overrideColor.GetAsVector3(), m_overrideAlpha);
    color = color.GammaToLinear(); // the colors are specified in sRGB but we want linear colors in the shader
    color.SetA(color.GetA() * fade);
    uint32 packedColor = (color.GetA8() << 24) | (color.GetR8() << 16) | (color.GetG8() << 8) | color.GetB8();

    ISprite::Borders cellUvBorders(sprite->GetCellUvBorders(cellIndex));
    float leftBorder = cellUvBorders.m_left;
    float rightBorder = 1.0f - cellUvBorders.m_right;
    float rectWidth = points.TopRight().GetX() - points.TopLeft().GetX();
    float leftPlusRightBorderWidth = (leftBorder + rightBorder) * textureSize.GetX();

    if (leftPlusRightBorderWidth > rectWidth)
    {
        // the width of the element rect is less that the right and left borders combined
        // so we need to adjust so that they don't get drawn overlapping. We adjust them
        // proportionally
        float correctionScale = rectWidth / leftPlusRightBorderWidth;
        leftBorder *= correctionScale;
        rightBorder *= correctionScale;
    }

    float topBorder = cellUvBorders.m_top;
    float bottomBorder = 1.0f - cellUvBorders.m_bottom;
    float rectHeight = points.BottomLeft().GetY() - points.TopLeft().GetY();
    float topPlusBottomBorderHeight = (topBorder + bottomBorder) * textureSize.GetY();

    if (topPlusBottomBorderHeight > rectHeight)
    {
        // the height of the element rect is less that the top and bottom borders combined
        // so we need to adjust so that they don't get drawn overlapping. We adjust them
        // proportionally
        float correctionScale = rectHeight / topPlusBottomBorderHeight;
        topBorder *= correctionScale;
        bottomBorder *= correctionScale;
    }

    if (m_fillType == FillType::None)
    {
        // compute the values for the vertex positions, the mid positions are a fixed number of pixels in from the
        // edges. This is based on the border percentage of the texture size
        float xValues[] = {
            points.TopLeft().GetX(),
            points.TopLeft().GetX() + textureSize.GetX() * leftBorder,
            points.BottomRight().GetX() - textureSize.GetX() * rightBorder,
            points.BottomRight().GetX(),
        };
        float yValues[] = {
            points.TopLeft().GetY(),
            points.TopLeft().GetY() + textureSize.GetY() * topBorder,
            points.BottomRight().GetY() - textureSize.GetY() * bottomBorder,
            points.BottomRight().GetY(),
        };

        float sValues[4];
        float tValues[4];
        GetSlicedStValuesFromCorrectionalScaleBorders(sValues, tValues, sprite, cellIndex, rightBorder, bottomBorder);

        // fill out the 16 verts to define the 3x3 set of quads
        const int32 numVertices = 16; // to create a 4x4 grid making 9 quads
        SVF_P3F_C4B_T2F vertices[numVertices];
        int i = 0;
        const float z = 1.0f;   // depth test disabled, if writing Z this will write at far plane
        IDraw2d::Rounding pixelRounding = IsPixelAligned() ? IDraw2d::Rounding::Nearest : IDraw2d::Rounding::None;

        for (int y = 0; y < 4; ++y)
        {
            for (int x = 0; x < 4; ++x)
            {
                AZ::Vector3 point3(xValues[x], yValues[y], z);
                point3 = transform * point3;
                point3 = Draw2dHelper::RoundXY(point3, pixelRounding);
                vertices[i].xyz = Vec3(point3.GetX(), point3.GetY(), point3.GetZ());

                vertices[i].color.dcolor = packedColor;

                vertices[i].st = Vec2(sValues[x], tValues[y]);

                ++i;
            }
        }

        // The vertices are in the order of top row left->right, then next row left->right etc
        //  0  1  2  3
        //  4  5  6  7
        //  8  9 10 11
        // 12 13 14 15
        //
        // The indices are for one triangle strip which looks like this
        //  |\|\|\|     ->
        //  |\|\|\|     <-
        //  |\|\|\|     ->

        if (m_fillCenter)
        {
            const int32 numFilledIndices = 26; // 8 for each row, plus two for degenerates to jump rows
            uint16 filledIndices[numFilledIndices] =
            {
                4,  0,   5,  1,   6,  2,   7,  3,   3,
                7, 11,   6, 10,   5,  9,   4,  8,   8,
                12,  8,  13,  9,  14, 10,  15, 11,
            };

            RenderTriangleStrip(texture, vertices, filledIndices, numVertices, numFilledIndices);
        }
        else
        {
            const int32 numUnfilledIndices = 28; // 8 for each row, plus two for degenerates to jump rows, plus 2 degenerates to skip center
            uint16 unfilledIndices[numUnfilledIndices] =
            {
                4,  0,   5,  1,   6,  2,   7,  3,   3,
                7, 11,   6, 10,  10,  5,  5,  9,   4,  8,   8,
                12,  8,  13,  9,  14, 10,  15, 11,
            };

            RenderTriangleStrip(texture, vertices, unfilledIndices, numVertices, numUnfilledIndices);
        }
    }
    else if (m_fillType == FillType::Linear)
    {
        RenderSlicedLinearFilledSprite(sprite, cellIndex, texture, packedColor, topBorder, leftBorder, bottomBorder, rightBorder);
    }
    else if (m_fillType == FillType::Radial)
    {
        RenderSlicedRadialFilledSprite(sprite, cellIndex, texture, packedColor, topBorder, leftBorder, bottomBorder, rightBorder);
    }
    else if (m_fillType == FillType::RadialCorner || m_fillType == FillType::RadialEdge)
    {
        RenderSlicedRadialCornerOrEdgeFilledSprite(sprite, cellIndex, texture, packedColor, topBorder, leftBorder, bottomBorder, rightBorder);
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiImageComponent::RenderFixedSprite(ISprite* sprite, int cellIndex, float fade)
{
    ITexture* texture = sprite->GetTexture();
    texture->SetClamp(true);

    AZ::Vector2 textureSize(sprite->GetCellSize(cellIndex));

    UiTransformInterface::RectPoints points;
    EBUS_EVENT_ID(GetEntityId(), UiTransformBus, GetCanvasSpacePointsNoScaleRotate, points);

    AZ::Vector2 pivot;
    EBUS_EVENT_ID_RESULT(pivot, GetEntityId(), UiTransformBus, GetPivot);

    // change width and height to match texture
    AZ::Vector2 rectSize = points.GetAxisAlignedSize();
    AZ::Vector2 sizeDiff = textureSize - rectSize;

    AZ::Vector2 topLeftOffset(sizeDiff.GetX() * pivot.GetX(), sizeDiff.GetY() * pivot.GetY());
    AZ::Vector2 bottomRightOffset(sizeDiff.GetX() * (1.0f - pivot.GetX()), sizeDiff.GetY() * (1.0f - pivot.GetY()));

    points.TopLeft() -= topLeftOffset;
    points.BottomRight() += bottomRightOffset;
    points.TopRight() = AZ::Vector2(points.BottomRight().GetX(), points.TopLeft().GetY());
    points.BottomLeft() = AZ::Vector2(points.TopLeft().GetX(), points.BottomRight().GetY());

    // now apply scale and rotation
    EBUS_EVENT_ID(GetEntityId(), UiTransformBus, RotateAndScalePoints, points);

    // now draw the same as Stretched
    const UiTransformInterface::RectPoints& uvCoords = sprite->GetCellUvCoords(cellIndex);
    const AZ::Vector2 uvs[4] =
    {
        uvCoords.TopLeft(),
        uvCoords.TopRight(),
        uvCoords.BottomRight(),
        uvCoords.BottomLeft(),
    };
    if (m_fillType == FillType::None)
    {
        RenderSingleQuad(texture, points.pt, uvs, fade);
    }
    else
    {
        RenderFilledQuad(texture, points.pt, uvs, fade);
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiImageComponent::RenderTiledSprite(ISprite* sprite, float fade)
{
    ITexture* texture = sprite->GetTexture();
    texture->SetClamp(false);

    AZ::Vector2 textureSize = sprite->GetSize();

    UiTransformInterface::RectPoints points;
    EBUS_EVENT_ID(GetEntityId(), UiTransformBus, GetCanvasSpacePointsNoScaleRotate, points);

    // scale UV's so that one texel is one pixel on screen
    AZ::Vector2 rectSize = points.GetAxisAlignedSize();
    AZ::Vector2 uvScale(rectSize.GetX() / textureSize.GetX(), rectSize.GetY() / textureSize.GetY());

    // now apply scale and rotation to points
    EBUS_EVENT_ID(GetEntityId(), UiTransformBus, RotateAndScalePoints, points);

    // now draw the same as Stretched but with UV's adjusted
    const AZ::Vector2 uvs[4] = { AZ::Vector2(0, 0), AZ::Vector2(uvScale.GetX(), 0), AZ::Vector2(uvScale.GetX(), uvScale.GetY()), AZ::Vector2(0, uvScale.GetY()) };
    if (m_fillType == FillType::None)
    {
        RenderSingleQuad(texture, points.pt, uvs, fade);
    }
    else
    {
        RenderFilledQuad(texture, points.pt, uvs, fade);
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiImageComponent::RenderStretchedToFitOrFillSprite(ISprite* sprite, int cellIndex, float fade, bool toFit)
{
    ITexture* texture = sprite->GetTexture();
    texture->SetClamp(true);

    AZ::Vector2 textureSize = sprite->GetCellSize(cellIndex);

    UiTransformInterface::RectPoints points;
    EBUS_EVENT_ID(GetEntityId(), UiTransformBus, GetCanvasSpacePointsNoScaleRotate, points);

    AZ::Vector2 pivot;
    EBUS_EVENT_ID_RESULT(pivot, GetEntityId(), UiTransformBus, GetPivot);

    // scale the texture so it either fits or fills the enclosing rect
    AZ::Vector2 rectSize = points.GetAxisAlignedSize();
    const float scaleFactorX = rectSize.GetX() / textureSize.GetX();
    const float scaleFactorY = rectSize.GetY() / textureSize.GetY();
    const float scaleFactor = toFit ?
        AZ::GetMin(scaleFactorX, scaleFactorY) :
        AZ::GetMax(scaleFactorX, scaleFactorY);

    AZ::Vector2 scaledTextureSize = textureSize * scaleFactor;
    AZ::Vector2 sizeDiff = scaledTextureSize - rectSize;

    AZ::Vector2 topLeftOffset(sizeDiff.GetX() * pivot.GetX(), sizeDiff.GetY() * pivot.GetY());
    AZ::Vector2 bottomRightOffset(sizeDiff.GetX() * (1.0f - pivot.GetX()), sizeDiff.GetY() * (1.0f - pivot.GetY()));

    points.TopLeft() -= topLeftOffset;
    points.BottomRight() += bottomRightOffset;
    points.TopRight() = AZ::Vector2(points.BottomRight().GetX(), points.TopLeft().GetY());
    points.BottomLeft() = AZ::Vector2(points.TopLeft().GetX(), points.BottomRight().GetY());

    // now apply scale and rotation
    EBUS_EVENT_ID(GetEntityId(), UiTransformBus, RotateAndScalePoints, points);

    // now draw the same as Stretched
    const UiTransformInterface::RectPoints& uvCoords = sprite->GetCellUvCoords(cellIndex);
    const AZ::Vector2 uvs[4] =
    {
        uvCoords.TopLeft(),
        uvCoords.TopRight(),
        uvCoords.BottomRight(),
        uvCoords.BottomLeft(),
    };
    if (m_fillType == FillType::None)
    {
        RenderSingleQuad(texture, points.pt, uvs, fade);
    }
    else
    {
        RenderFilledQuad(texture, points.pt, uvs, fade);
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiImageComponent::RenderSingleQuad(ITexture* texture, const AZ::Vector2* positions, const AZ::Vector2* uvs, float fade)
{
    AZ::Color color = AZ::Color::CreateFromVector3AndFloat(m_overrideColor.GetAsVector3(), m_overrideAlpha);
    color = color.GammaToLinear();   // the colors are specified in sRGB but we want linear colors in the shader
    color.SetA(color.GetA() * fade);

    // points are a clockwise quad
    IDraw2d::VertexPosColUV verts[4];
    for (int i = 0; i < 4; ++i)
    {
        verts[i].position = positions[i];
        verts[i].color = color;
        verts[i].uv = uvs[i];
    }

    int blendMode = GetBlendModeStateFlags();
    IDraw2d::Rounding rounding = IsPixelAligned() ? IDraw2d::Rounding::Nearest : IDraw2d::Rounding::None;
    Draw2dHelper::GetDraw2d()->DrawQuad(texture->GetTextureID(), verts, blendMode, rounding, IUiRenderer::Get()->GetBaseState());
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiImageComponent::RenderFilledQuad(ITexture* texture, const AZ::Vector2* positions, const AZ::Vector2* uvs, float fade)
{
    AZ::Color color = AZ::Color::CreateFromVector3AndFloat(m_overrideColor.GetAsVector3(), m_overrideAlpha);
    color = color.GammaToLinear();   // the colors are specified in sRGB but we want linear colors in the shader
    color.SetA(color.GetA() * fade);

    if (m_fillType == FillType::Linear)
    {
        RenderLinearFilledQuad(texture, positions, uvs, fade, color);
    }
    else if (m_fillType == FillType::Radial)
    {
        RenderRadialFilledQuad(texture, positions, uvs, fade, color);
    }
    else if (m_fillType == FillType::RadialCorner)
    {
        RenderRadialCornerFilledQuad(texture, positions, uvs, fade, color);
    }
    else if (m_fillType == FillType::RadialEdge)
    {
        RenderRadialEdgeFilledQuad(texture, positions, uvs, fade, color);
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiImageComponent::RenderLinearFilledQuad(ITexture* texture, const AZ::Vector2* positions, const AZ::Vector2* uvs, float fade, AZ::Color color)
{
    // This fills the vertices (rotating them based on the origin edge) similar to RenderSingleQuad but then edits 2 vertices based on m_fillAmount.
    int vertexOffset = 0;
    if (m_fillEdgeOrigin == FillEdgeOrigin::Left)
    {
        vertexOffset = 0;
    }
    else if (m_fillEdgeOrigin == FillEdgeOrigin::Top)
    {
        vertexOffset = 1;
    }
    else if (m_fillEdgeOrigin == FillEdgeOrigin::Right)
    {
        vertexOffset = 2;
    }
    else
    {
        vertexOffset = 3;
    }

    // points are a clockwise quad
    IDraw2d::VertexPosColUV verts[4];
    for (int i = 0; i < 4; ++i)
    {
        verts[i].position = positions[(i + vertexOffset) % 4];
        verts[i].color = color;
        verts[i].uv = uvs[(i + vertexOffset) % 4];
    }

    verts[1].position = verts[0].position + m_fillAmount * (verts[1].position - verts[0].position);
    verts[2].position = verts[3].position + m_fillAmount * (verts[2].position - verts[3].position);

    verts[1].uv = verts[0].uv + m_fillAmount * (verts[1].uv - verts[0].uv);
    verts[2].uv = verts[3].uv + m_fillAmount * (verts[2].uv - verts[3].uv);

    int blendMode = GetBlendModeStateFlags();
    IDraw2d::Rounding rounding = IsPixelAligned() ? IDraw2d::Rounding::Nearest : IDraw2d::Rounding::None;
    Draw2dHelper::GetDraw2d()->DrawQuad(texture->GetTextureID(), verts, blendMode, rounding, IUiRenderer::Get()->GetBaseState());
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiImageComponent::RenderRadialFilledQuad(ITexture* texture, const AZ::Vector2* positions, const AZ::Vector2* uvs, float fade, AZ::Color color)
{
    // 1. Fill all vertices/indices as if m_fillAmount is 1.0f
    // 2. Calculate which vertex needs to be moved based on the current value of m_fillAmount and set it's new position/UVs accordingly.
    // 3. Submit only the required amount of vertices/indices

    // The vertices are in the following order. If the calculated fillOffset does not lie on the top edge, the indices are rotated to keep the fill algorithm the same.
    // 5 1/6 2
    //    0
    // 4     3

    int firstIndexOffset = 0;
    int secondIndexOffset = 1;
    int currentVertexToFill = 0;
    int windingDirection = 1;

    float fillOffset = (fmod(m_fillStartAngle, 360.0f) / 360.0f) + 1.125f; // Offsets below are calculated from vertex 5, keep the value is above 0
                                                                           // and offset by 1/8 here so it behaves as if calculated from vertex 1.
    if (!m_fillClockwise)
    {
        fillOffset = 1 - (fmod(m_fillStartAngle, 360.0f) / 360.0f) + 1.875f; // Start angle should be clockwise and offsets below are now measured
                                                                             // counter-clockwise so offset further here back into vertex 1 position.
    }

    int startingEdge = static_cast<int>((fillOffset) / 0.25f) % 4;
    if (!m_fillClockwise)
    {
        // Flip vertices and direction so that the fill algorithm is unchanged.
        firstIndexOffset = 1;
        secondIndexOffset = 0;
        currentVertexToFill = 6;
        windingDirection = -1;
        startingEdge *= -1;
    }

    // Fill vertices (rotated based on startingEdge).
    const int numVertices = 7; // The maximum amount of vertices that can be used
    SVF_P3F_C4B_T2F verts[numVertices];
    const float z = 1.0f;
    uint32 packedColor = (color.GetA8() << 24) | (color.GetR8() << 16) | (color.GetG8() << 8) | color.GetB8();
    for (int i = 1; i < 5; ++i)
    {
        verts[currentVertexToFill % 4 + 2].xyz = Vec3(positions[(4 + i + startingEdge) % 4].GetX(), positions[(4 + i + startingEdge) % 4].GetY(), z);
        verts[currentVertexToFill % 4 + 2].color.dcolor = packedColor;
        verts[currentVertexToFill % 4 + 2].st = Vec2(uvs[(4 + i + startingEdge) % 4].GetX(), uvs[(4 + i + startingEdge) % 4].GetY());

        currentVertexToFill += windingDirection;
    }

    const int numIndices = 15;
    uint16 indices[numIndices];
    for (int ix = 0; ix < 5; ++ix)
    {
        indices[ix * 3 + firstIndexOffset] = ix + 1;
        indices[ix * 3 + secondIndexOffset] = ix + 2;
        indices[ix * 3 + 2] = 0;
    }

    float startingEdgeRemainder = fmod(fillOffset, 0.25f);
    float startingEdgePercentage = startingEdgeRemainder * 4;

    // Set start/end vertices
    verts[1].xyz = verts[5].xyz + startingEdgePercentage * (verts[2].xyz - verts[5].xyz);
    verts[1].color.dcolor = packedColor;
    verts[1].st = verts[5].st + startingEdgePercentage * (verts[2].st - verts[5].st);
    verts[6] = verts[1];

    // Set center vertex
    verts[0].xyz = (verts[5].xyz + verts[3].xyz) * 0.5f;
    verts[0].color.dcolor = packedColor;
    verts[0].st = (verts[5].st + verts[3].st) * 0.5f;

    int finalEdge = static_cast<int>((startingEdgeRemainder + m_fillAmount) / 0.25f);
    float finalEdgePercentage = fmod(fillOffset + m_fillAmount, 0.25f) * 4;

    // Calculate which vertex should be moved for the current m_fillAmount value and set it's new position/UV.
    int editedVertexIndex = finalEdge + 2;
    int previousVertexIndex = ((3 + editedVertexIndex - 2) % 4) + 2;
    int nextVertexIndex = ((editedVertexIndex - 2) % 4) + 2;
    verts[editedVertexIndex].xyz = verts[previousVertexIndex].xyz + finalEdgePercentage * (verts[nextVertexIndex].xyz - verts[previousVertexIndex].xyz);
    verts[editedVertexIndex].st = verts[previousVertexIndex].st + finalEdgePercentage * (verts[nextVertexIndex].st - verts[previousVertexIndex].st);

    RenderTriangleList(texture, verts, indices, editedVertexIndex + 1, 3 * (editedVertexIndex - 1));
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiImageComponent::RenderRadialCornerFilledQuad(ITexture* texture, const AZ::Vector2* positions, const AZ::Vector2* uvs, float fade, AZ::Color color)
{
    // This fills the vertices (rotating them based on the origin edge) similar to RenderSingleQuad, then edits a vertex based on m_fillAmount.
    const uint32 numVerts = 4;
    SVF_P3F_C4B_T2F verts[numVerts];
    int vertexOffset = 0;
    if (m_fillCornerOrigin == FillCornerOrigin::TopLeft)
    {
        vertexOffset = 0;
    }
    else if (m_fillCornerOrigin == FillCornerOrigin::TopRight)
    {
        vertexOffset = 1;
    }
    else if (m_fillCornerOrigin == FillCornerOrigin::BottomRight)
    {
        vertexOffset = 2;
    }
    else
    {
        vertexOffset = 3;
    }
    const float z = 1.0f;
    uint32 packedColor = (color.GetA8() << 24) | (color.GetR8() << 16) | (color.GetG8() << 8) | color.GetB8();
    for (int i = 0; i < 4; ++i)
    {
        verts[i].xyz = Vec3(positions[(i + vertexOffset) % 4].GetX(), positions[(i + vertexOffset) % 4].GetY(), z);
        verts[i].color.dcolor = packedColor;
        verts[i].st = Vec2(uvs[(i + vertexOffset) % 4].GetX(), uvs[(i + vertexOffset) % 4].GetY());
    }

    const uint32 numIndices = 4;
    uint16 indicesCW[numIndices] = { 1, 2, 0, 3 };
    uint16 indicesCCW[numIndices] = { 3, 0, 2, 1 };

    uint16* indices = indicesCW;
    if (!m_fillClockwise)
    {
        // Change index order as we're now filling from the end edge back to the start.
        indices = indicesCCW;
    }

    // Calculate which vertex needs to be moved based on m_fillAmount and set its new position and UV.
    int half = static_cast<int>(floor(m_fillAmount + 0.5f));
    float s = (m_fillAmount - (0.5f * half)) * 2;
    int order = m_fillClockwise ? 1 : -1;
    int vertexToEdit = (half * order) + 2;
    verts[vertexToEdit].xyz = verts[vertexToEdit - order].xyz + (verts[vertexToEdit].xyz - verts[vertexToEdit - order].xyz) * s;
    verts[vertexToEdit].st = verts[vertexToEdit - order].st + (verts[vertexToEdit].st - verts[vertexToEdit - order].st) * s;

    int numIndicesToDraw = 3 + half;

    RenderTriangleStrip(texture, verts, indices, numVerts, numIndicesToDraw);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiImageComponent::RenderRadialEdgeFilledQuad(ITexture* texture, const AZ::Vector2* positions, const AZ::Vector2* uvs, float fade, AZ::Color color)
{
    // This fills the vertices (rotating them based on the origin edge) similar to RenderSingleQuad, then edits a vertex based on m_fillAmount.
    const uint32 numVertices = 5; // Need an extra vertex for the origin.
    SVF_P3F_C4B_T2F verts[numVertices];
    int vertexOffset = 0;
    if (m_fillEdgeOrigin == FillEdgeOrigin::Left)
    {
        vertexOffset = 0;
    }
    else if (m_fillEdgeOrigin == FillEdgeOrigin::Top)
    {
        vertexOffset = 1;
    }
    else if (m_fillEdgeOrigin == FillEdgeOrigin::Right)
    {
        vertexOffset = 2;
    }
    else
    {
        vertexOffset = 3;
    }

    float z = 1.0f;
    // Generate the vertex on the edge.
    AZ::Vector2 calculatedPosition = (positions[(0 + vertexOffset) % 4] + positions[(3 + vertexOffset) % 4]) * 0.5f;
    verts[0].xyz = Vec3(calculatedPosition.GetX(), calculatedPosition.GetY(), z);

    uint32 packedColor = (color.GetA8() << 24) | (color.GetR8() << 16) | (color.GetG8() << 8) | color.GetB8();
    verts[0].color.dcolor = packedColor;

    AZ::Vector2 calculatedUV = (uvs[(0 + vertexOffset) % 4] + uvs[(3 + vertexOffset) % 4]) * 0.5f;
    verts[0].st = Vec2(calculatedUV.GetX(), calculatedUV.GetY());

    // Fill other vertices
    for (int i = 1; i < 5; ++i)
    {
        calculatedPosition = positions[(i - 1 + vertexOffset) % 4];
        calculatedUV = uvs[(i - 1 + vertexOffset) % 4];
        verts[i].xyz = Vec3(calculatedPosition.GetX(), calculatedPosition.GetY(), z);
        verts[i].color.dcolor = packedColor;
        verts[i].st = Vec2(calculatedUV.GetX(), calculatedUV.GetY());
    }

    const uint32 numIndices = 9;

    uint16 indicesCW[numIndices] = { 0, 1, 2, 0, 2, 3, 0, 3, 4 };
    uint16 indicesCCW[numIndices] = { 0, 3, 4, 0, 2, 3, 0, 1, 2 };

    uint16* indices = indicesCW;
    int segment = static_cast<int>(fmin(m_fillAmount * 3, 2.0f));
    float s = (m_fillAmount - (0.3333f * segment)) * 3;
    int order = 1;
    int firstVertex = 2;
    if (!m_fillClockwise)
    {
        // Change order as we're now filling from the end back to the start.
        order = -1;
        firstVertex = 3;
        indices = indicesCCW;
    }
    // Calculate which vertex needs to be moved based on m_fillAmount and set its new position and UV.
    int vertexToEdit = (segment * order) + firstVertex;
    verts[vertexToEdit].xyz = verts[vertexToEdit - order].xyz + (verts[vertexToEdit].xyz - verts[vertexToEdit - order].xyz) * s;
    verts[vertexToEdit].st = verts[vertexToEdit - order].st + (verts[vertexToEdit].st - verts[vertexToEdit - order].st) * s;

    int numIndicesToDraw = 3 * (segment + 1);

    RenderTriangleList(texture, verts, indices, numVertices, numIndicesToDraw);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiImageComponent::RenderSlicedLinearFilledSprite(ISprite* sprite, int cellIndex, ITexture* texture, uint32 packedColor, float topBorder, float leftBorder, float bottomBorder, float rightBorder)
{
    // 1. Clamp x and y and corresponding s and t values based on m_fillAmount.
    // 2. Fill vertices in the same way as a standard sliced sprite
    AZ::Vector2 textureSize(sprite->GetCellSize(cellIndex));

    UiTransformInterface::RectPoints points;
    EBUS_EVENT_ID(GetEntityId(), UiTransformBus, GetCanvasSpacePointsNoScaleRotate, points);
    AZ::Matrix4x4 transform;
    EBUS_EVENT_ID(GetEntityId(), UiTransformBus, GetTransformToViewport, transform);

    float xValues[] = {
        points.TopLeft().GetX(),
        points.TopLeft().GetX() + textureSize.GetX() * leftBorder,
        points.BottomRight().GetX() - textureSize.GetX() * rightBorder,
        points.BottomRight().GetX(),
    };
    float yValues[] = {
        points.TopLeft().GetY(),
        points.TopLeft().GetY() + textureSize.GetY() * topBorder,
        points.BottomRight().GetY() - textureSize.GetY() * bottomBorder,
        points.BottomRight().GetY(),
    };

    float sValues[4];
    float tValues[4];
    GetSlicedStValuesFromCorrectionalScaleBorders(sValues, tValues, sprite, cellIndex, rightBorder, bottomBorder);

    const uint32 numVertices = 16;
    SVF_P3F_C4B_T2F vertices[numVertices];

    float* clipPosition = xValues;
    float* clipSTs = sValues;
    int startClip = 0;
    int clipInc = 1;

    if (m_fillEdgeOrigin == FillEdgeOrigin::Left)
    {
        clipPosition = xValues;
        clipSTs = sValues;
        startClip = 0;
        clipInc = 1;
    }
    else if (m_fillEdgeOrigin == FillEdgeOrigin::Top)
    {
        clipPosition = yValues;
        clipSTs = tValues;
        startClip = 0;
        clipInc = 1;
    }
    else if (m_fillEdgeOrigin == FillEdgeOrigin::Right)
    {
        clipPosition = xValues;
        clipSTs = sValues;
        // Start from the end of the array and work back.
        startClip = 3;
        clipInc = -1;
    }
    else
    {
        clipPosition = yValues;
        clipSTs = tValues;
        // Start from the end of the array and work back.
        startClip = 3;
        clipInc = -1;
    }

    // If a segment ends before m_fillAmount then it is fully displayed.
    // If m_fillAmount lies in this segment, change the x/y position and clamp all remaining values to this.
    float totalLength = (clipPosition[startClip + (3 * clipInc)] - clipPosition[startClip]);
    float previousPercentage = 0;
    int previousIndex = startClip;
    int clampIndex = -1; // to clamp all values greater than m_fillAmount in specified direction.
    for (int arrayPos = 1; arrayPos < 4; ++arrayPos)
    {
        int currentIndex = startClip + arrayPos * clipInc;
        float thisPercentage = (clipPosition[currentIndex] - clipPosition[startClip]) / totalLength;

        if (clampIndex != -1) // we've already passed m_fillAmount.
        {
            clipPosition[currentIndex] = clipPosition[clampIndex];
            clipSTs[currentIndex] = clipSTs[clampIndex];
        }
        else if (thisPercentage > m_fillAmount)
        {
            // this index is greater than m_fillAmount but the previous one was not, so calculate our new position and UV.
            float segmentPercentFilled = (m_fillAmount - previousPercentage) / (thisPercentage - previousPercentage);
            clipPosition[currentIndex] = clipPosition[previousIndex] + segmentPercentFilled * (clipPosition[currentIndex] - clipPosition[previousIndex]);
            clipSTs[currentIndex] = clipSTs[previousIndex] + segmentPercentFilled * (clipSTs[currentIndex] - clipSTs[previousIndex]);
            clampIndex = currentIndex; // clamp remaining values to this one to generate degenerate triangles.
        }

        previousPercentage = thisPercentage;
        previousIndex = currentIndex;
    }

    // Fill the vertices with the generated xy and st values.
    int i = 0;
    const float z = 1.0f;   // depth test disabled, if writing Z this will write at far plane
    IDraw2d::Rounding pixelRounding = IsPixelAligned() ? IDraw2d::Rounding::Nearest : IDraw2d::Rounding::None;
    for (int y = 0; y < 4; ++y)
    {
        for (int x = 0; x < 4; ++x)
        {
            AZ::Vector3 point3(xValues[x], yValues[y], z);
            point3 = transform * point3;
            point3 = Draw2dHelper::RoundXY(point3, pixelRounding);

            vertices[i].xyz = Vec3(point3.GetX(), point3.GetY(), point3.GetZ());
            vertices[i].color.dcolor = packedColor;
            vertices[i].st = Vec2(sValues[x], tValues[y]);

            ++i;
        }
    }

    const uint32 numIndices = 54;
    const uint32 numIndicesExcludingCenter = 48;
    uint16 indices[numIndices] = {
        0, 1, 4, 1, 5, 4, 1, 2, 5, 2, 6, 5, 2, 3, 6, 3, 7, 6,
        4, 5, 8, 5, 9, 8, 6, 7, 10, 7, 11, 10,
        8, 9, 12, 9, 13, 12, 9, 10, 13, 10, 14, 13, 10, 11, 14, 11, 15, 14, 5, 6, 9, 6, 10, 9,
    };

    int totalIndices = (m_fillCenter ? numIndices : numIndicesExcludingCenter);

    RenderTriangleList(texture, vertices, indices, numVertices, totalIndices);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiImageComponent::RenderSlicedRadialFilledSprite(ISprite* sprite, int cellIndex, ITexture* texture, uint32 packedColor, float topBorder, float leftBorder, float bottomBorder, float rightBorder)
{
    // 1. Fill vertices in the same layout as the non-filled sliced sprite.
    // 2. Calculate two points of lines from the center to a point based on m_fillAmount and m_fillOrigin.
    // 3. Clip the triangles of the sprite against those lines based on the fill amount.

    AZ::Vector2 textureSize(sprite->GetCellSize(cellIndex));

    UiTransformInterface::RectPoints points;
    EBUS_EVENT_ID(GetEntityId(), UiTransformBus, GetCanvasSpacePointsNoScaleRotate, points);
    AZ::Matrix4x4 transform;
    EBUS_EVENT_ID(GetEntityId(), UiTransformBus, GetTransformToViewport, transform);

    float xValues[] = {
        points.TopLeft().GetX(),
        points.TopLeft().GetX() + textureSize.GetX() * leftBorder,
        points.BottomRight().GetX() - textureSize.GetX() * rightBorder,
        points.BottomRight().GetX(),
    };
    float yValues[] = {
        points.TopLeft().GetY(),
        points.TopLeft().GetY() + textureSize.GetY() * topBorder,
        points.BottomRight().GetY() - textureSize.GetY() * bottomBorder,
        points.BottomRight().GetY(),
    };

    float sValues[4];
    float tValues[4];
    GetSlicedStValuesFromCorrectionalScaleBorders(sValues, tValues, sprite, cellIndex, rightBorder, bottomBorder);

    const uint32 numVerts = 27;
    const uint32 numIndices = 54;
    const uint32 numIndicesExcludingCenter = 48;
    uint16 indices[numIndices] = {
        0, 1, 4, 1, 5, 4, 1, 2, 5, 2, 6, 5, 2, 3, 6, 3, 7, 6,
        4, 5, 8, 5, 9, 8, 6, 7, 10, 7, 11, 10,
        8, 9, 12, 9, 13, 12, 9, 10, 13, 10, 14, 13, 10, 11, 14, 11, 15, 14, 5, 6, 9, 6, 10, 9,
    };

    SVF_P3F_C4B_T2F verts[numVerts];
    SVF_P3F_C4B_T2F renderVerts[numIndices * 4]; // ClipToLine doesn't check for duplicate vertices for speed, so this is the maximum we'll need.
    uint16 renderIndices[numIndices * 4] = { 0 };

    IDraw2d::Rounding pixelRounding = IsPixelAligned() ? IDraw2d::Rounding::Nearest : IDraw2d::Rounding::None;
    float z = 1.0f;
    int i = 0;
    for (int y = 0; y < 4; ++y)
    {
        for (int x = 0; x < 4; ++x)
        {
            AZ::Vector3 point3(xValues[x], yValues[y], z);
            point3 = transform * point3;
            point3 = Draw2dHelper::RoundXY(point3, pixelRounding);

            verts[i].xyz = Vec3(point3.GetX(), point3.GetY(), point3.GetZ());
            verts[i].color.dcolor = packedColor;
            verts[i].st = Vec2(sValues[x], tValues[y]);

            ++i;
        }
    }

    float fillOffset = AZ::DegToRad(m_fillStartAngle);

    Vec3 lineOrigin = (verts[0].xyz + verts[15].xyz) * 0.5f;
    Vec3 rotatingLineEnd = ((verts[0].xyz + verts[3].xyz) * 0.5f) - lineOrigin;
    Vec3 firstHalfFixedLineEnd = (rotatingLineEnd * -1.0f);
    Vec3 secondHalfFixedLineEnd = rotatingLineEnd;
    float startAngle = 0;
    float endAngle = -AZ::Constants::TwoPi;

    if (!m_fillClockwise)
    {
        // Clip from the opposite side of the line and rotate the line in the opposite direction.
        float temp = startAngle;
        startAngle = endAngle;
        endAngle = temp;
        rotatingLineEnd = rotatingLineEnd * -1.0f;
        firstHalfFixedLineEnd = firstHalfFixedLineEnd * -1.0f;
        secondHalfFixedLineEnd = secondHalfFixedLineEnd * -1.0f;
    }
    Matrix33 lineRotationMatrix;

    lineRotationMatrix.SetRotationZ(startAngle - fillOffset);
    firstHalfFixedLineEnd = firstHalfFixedLineEnd * lineRotationMatrix;
    firstHalfFixedLineEnd = lineOrigin + firstHalfFixedLineEnd;
    secondHalfFixedLineEnd = secondHalfFixedLineEnd * lineRotationMatrix;
    secondHalfFixedLineEnd = lineOrigin + secondHalfFixedLineEnd;

    lineRotationMatrix.SetRotationZ(startAngle - fillOffset + (endAngle - startAngle) * m_fillAmount);
    rotatingLineEnd = rotatingLineEnd * lineRotationMatrix;
    rotatingLineEnd = lineOrigin + rotatingLineEnd;

    int numIndicesToRender = 0;
    int vertexOffset = 0;
    int totalIndices = (m_fillCenter ? numIndices : numIndicesExcludingCenter);
    int indicesUsed = 0;
    const int maxTemporaryVerts = 4;
    const int maxTemporaryIndices = 6;
    if (m_fillAmount < 0.5f)
    {
        // Clips against first half line and then rotating line and adds results to render list.
        for (int currentIndex = 0; currentIndex < totalIndices; currentIndex += 3)
        {
            SVF_P3F_C4B_T2F intermediateVerts[maxTemporaryVerts];
            uint16 intermediateIndices[maxTemporaryIndices];
            int intermedateVertexOffset = 0;
            int intermediateIndicesUsed = ClipToLine(verts, &indices[currentIndex], intermediateVerts, intermediateIndices, intermedateVertexOffset, 0, lineOrigin, firstHalfFixedLineEnd);
            for (int currentIntermediateIndex = 0; currentIntermediateIndex < intermediateIndicesUsed; currentIntermediateIndex += 3)
            {
                indicesUsed = ClipToLine(intermediateVerts, &intermediateIndices[currentIntermediateIndex], renderVerts, renderIndices, vertexOffset, numIndicesToRender, lineOrigin, rotatingLineEnd);
                numIndicesToRender += indicesUsed;
            }
        }
    }
    else
    {
        // Clips against first half line and adds results to render list then clips against the second half line and rotating line and also adds those results to render list.
        for (int currentIndex = 0; currentIndex < totalIndices; currentIndex += 3)
        {
            SVF_P3F_C4B_T2F intermediateVerts[maxTemporaryVerts];
            uint16 intermediateIndices[maxTemporaryIndices];
            indicesUsed = ClipToLine(verts, &indices[currentIndex], renderVerts, renderIndices, vertexOffset, numIndicesToRender, lineOrigin, firstHalfFixedLineEnd);
            numIndicesToRender += indicesUsed;

            int intermedateVertexOffset = 0;
            int intermediateIndicesUsed = ClipToLine(verts, &indices[currentIndex], intermediateVerts, intermediateIndices, intermedateVertexOffset, 0, lineOrigin, secondHalfFixedLineEnd);
            for (int currentIntermediateIndex = 0; currentIntermediateIndex < intermediateIndicesUsed; currentIntermediateIndex += 3)
            {
                indicesUsed = ClipToLine(intermediateVerts, &intermediateIndices[currentIntermediateIndex], renderVerts, renderIndices, vertexOffset, numIndicesToRender, lineOrigin, rotatingLineEnd);
                numIndicesToRender += indicesUsed;
            }
        }
    }

    RenderTriangleList(texture, renderVerts, renderIndices, vertexOffset, numIndicesToRender);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiImageComponent::RenderSlicedRadialCornerOrEdgeFilledSprite(ISprite* sprite, int cellIndex, ITexture* texture, uint32 packedColor, float topBorder, float leftBorder, float bottomBorder, float rightBorder)
{
    // 1. Fill vertices in the same layout as the non-filled sliced sprite.
    // 2. Calculate two points of a line from either the corner or center of an edge to a point based on m_fillAmount.
    // 3. Clip the triangles of the sprite against that line.

    AZ::Vector2 textureSize(sprite->GetCellSize(cellIndex));

    UiTransformInterface::RectPoints points;
    EBUS_EVENT_ID(GetEntityId(), UiTransformBus, GetCanvasSpacePointsNoScaleRotate, points);
    AZ::Matrix4x4 transform;
    EBUS_EVENT_ID(GetEntityId(), UiTransformBus, GetTransformToViewport, transform);

    float xValues[] = {
        points.TopLeft().GetX(),
        points.TopLeft().GetX() + textureSize.GetX() * leftBorder,
        points.BottomRight().GetX() - textureSize.GetX() * rightBorder,
        points.BottomRight().GetX(),
    };
    float yValues[] = {
        points.TopLeft().GetY(),
        points.TopLeft().GetY() + textureSize.GetY() * topBorder,
        points.BottomRight().GetY() - textureSize.GetY() * bottomBorder,
        points.BottomRight().GetY(),
    };

    float sValues[4];
    float tValues[4];
    GetSlicedStValuesFromCorrectionalScaleBorders(sValues, tValues, sprite, cellIndex, rightBorder, bottomBorder);

    const uint32 numVerts = 27;
    const uint32 numIndices = 54;
    const uint32 numIndicesExcludingCenter = 48;
    uint16 indices[numIndices] = {
        0, 1, 4, 1, 5, 4, 1, 2, 5, 2, 6, 5, 2, 3, 6, 3, 7, 6,
        4, 5, 8, 5, 9, 8, 6, 7, 10, 7, 11, 10,
        8, 9, 12, 9, 13, 12, 9, 10, 13, 10, 14, 13, 10, 11, 14, 11, 15, 14, 5, 6, 9, 6, 10, 9,
    };

    SVF_P3F_C4B_T2F verts[numVerts];
    SVF_P3F_C4B_T2F renderVerts[numIndices * 2]; // ClipToLine doesn't check for duplicate vertices for speed, so this is the maximum we'll need.
    uint16 renderIndices[numIndices * 2] = { 0 };

    IDraw2d::Rounding pixelRounding = IsPixelAligned() ? IDraw2d::Rounding::Nearest : IDraw2d::Rounding::None;
    float z = 1.0f;
    int i = 0;
    for (int y = 0; y < 4; ++y)
    {
        for (int x = 0; x < 4; x += 1)
        {
            AZ::Vector3 point3(xValues[x], yValues[y], z);
            point3 = transform * point3;
            point3 = Draw2dHelper::RoundXY(point3, pixelRounding);

            verts[i].xyz = Vec3(point3.GetX(), point3.GetY(), point3.GetZ());
            verts[i].color.dcolor = packedColor;
            verts[i].st = Vec2(sValues[x], tValues[y]);

            ++i;
        }
    }

    // Generate the start and direction of the line to clip against based on the fill origin and fill amount.
    int originVertex = 0;
    int targetVertex = 0;

    if ((m_fillType == FillType::RadialCorner && m_fillCornerOrigin == FillCornerOrigin::TopLeft) ||
        (m_fillType == FillType::RadialEdge && m_fillEdgeOrigin == FillEdgeOrigin::Top))
    {
        originVertex = 0;
        targetVertex = 3;
    }
    else if ((m_fillType == FillType::RadialCorner && m_fillCornerOrigin == FillCornerOrigin::TopRight) ||
             (m_fillType == FillType::RadialEdge && m_fillEdgeOrigin == FillEdgeOrigin::Right))
    {
        originVertex = 3;
        targetVertex = 15;
    }
    else if ((m_fillType == FillType::RadialCorner && m_fillCornerOrigin == FillCornerOrigin::BottomRight) ||
             (m_fillType == FillType::RadialEdge && m_fillEdgeOrigin == FillEdgeOrigin::Bottom))
    {
        originVertex = 15;
        targetVertex = 12;
    }
    else if ((m_fillType == FillType::RadialCorner && m_fillCornerOrigin == FillCornerOrigin::BottomLeft) ||
             (m_fillType == FillType::RadialEdge && m_fillEdgeOrigin == FillEdgeOrigin::Left))
    {
        originVertex = 12;
        targetVertex = 0;
    }

    Vec3 lineOrigin(verts[originVertex].xyz);
    if (m_fillType == FillType::RadialEdge)
    {
        lineOrigin = (verts[originVertex].xyz + verts[targetVertex].xyz) * 0.5f;
    }
    lineOrigin.z = 0.0f;

    Vec3 lineEnd = verts[targetVertex].xyz - verts[originVertex].xyz;
    float startAngle = 0;
    float endAngle = m_fillType == FillType::RadialCorner ? -AZ::Constants::HalfPi : -AZ::Constants::Pi;

    if (!m_fillClockwise)
    {
        // Clip from the opposite side of the line and rotate the line in the opposite direction.
        float temp = startAngle;
        startAngle = endAngle;
        endAngle = temp;
        lineEnd = lineEnd * -1.0f;
    }
    Matrix33 lineRotationMatrix;
    lineRotationMatrix.SetRotationZ(startAngle + (endAngle - startAngle) * m_fillAmount);
    lineEnd = lineEnd * lineRotationMatrix;
    lineEnd = lineOrigin + lineEnd;

    int numIndicesToRender = 0;
    int vertexOffset = 0;
    int totalIndices = (m_fillCenter ? numIndices : numIndicesExcludingCenter);
    for (int ix = 0; ix < totalIndices; ix += 3)
    {
        int indicesUsed = ClipToLine(verts, &indices[ix], renderVerts, renderIndices, vertexOffset, numIndicesToRender, lineOrigin, lineEnd);
        numIndicesToRender += indicesUsed;
    }

    RenderTriangleList(texture, renderVerts, renderIndices, vertexOffset, numIndicesToRender);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
int UiImageComponent::ClipToLine(SVF_P3F_C4B_T2F* vertices, uint16* indices, SVF_P3F_C4B_T2F* renderVertices, uint16* renderIndices, int& vertexOffset, int renderIndexOffset, const Vec3& lineOrigin, const Vec3& lineEnd)
{
    Vec3 lineVector = lineEnd - lineOrigin;
    SVF_P3F_C4B_T2F lastVertex = vertices[indices[2]];
    SVF_P3F_C4B_T2F currentVertex;
    int verticesAdded = 0;

    for (int i = 0; i < 3; ++i)
    {
        currentVertex = vertices[indices[i]];
        Vec3 triangleEdgeDirection = currentVertex.xyz - lastVertex.xyz;
        Vec3 currentPointVector = (currentVertex.xyz - lineOrigin);
        Vec3 lastPointVector = (lastVertex.xyz - lineOrigin);
        float currentPointDeterminant = (lineVector.x * currentPointVector.y) - (lineVector.y * currentPointVector.x);
        float lastPointDeterminant = (lineVector.x * lastPointVector.y) - (lineVector.y * lastPointVector.x);
        const float epsilon = 0.001f;

        Vec3 perpendicularLineVector(-lineVector.y, lineVector.x, lineVector.z);
        Vec3 vertexToLine = lineOrigin - lastVertex.xyz;

        if (currentPointDeterminant < epsilon)
        {
            if (lastPointDeterminant > -epsilon && fabs(currentPointDeterminant) > epsilon && fabs(lastPointDeterminant) > epsilon)
            {
                //add calculated intersection
                float intersectionDistance = (vertexToLine.x * perpendicularLineVector.x + vertexToLine.y * perpendicularLineVector.y) / (triangleEdgeDirection.x * perpendicularLineVector.x + triangleEdgeDirection.y * perpendicularLineVector.y);
                SVF_P3F_C4B_T2F intersectPoint;
                intersectPoint.xyz = lastVertex.xyz + triangleEdgeDirection * intersectionDistance;
                intersectPoint.st = lastVertex.st + (currentVertex.st - lastVertex.st) * intersectionDistance;
                intersectPoint.color.dcolor = lastVertex.color.dcolor;

                renderVertices[vertexOffset] = intersectPoint;
                vertexOffset++;
                verticesAdded++;
            }
            //add currentvertex
            renderVertices[vertexOffset] = currentVertex;
            vertexOffset++;
            verticesAdded++;
        }
        else if (lastPointDeterminant < epsilon)
        {
            //add calculated intersection
            float intersectionDistance = (vertexToLine.x * perpendicularLineVector.x + vertexToLine.y * perpendicularLineVector.y) / (triangleEdgeDirection.x * perpendicularLineVector.x + triangleEdgeDirection.y * perpendicularLineVector.y);
            SVF_P3F_C4B_T2F intersectPoint;
            intersectPoint.xyz = lastVertex.xyz + triangleEdgeDirection * intersectionDistance;
            intersectPoint.st = lastVertex.st + (currentVertex.st - lastVertex.st) * intersectionDistance;
            intersectPoint.color.dcolor = lastVertex.color.dcolor;

            renderVertices[vertexOffset] = intersectPoint;
            vertexOffset++;
            verticesAdded++;
        }
        lastVertex = currentVertex;
    }

    int indicesAdded = 0;
    if (verticesAdded == 3)
    {
        renderIndices[renderIndexOffset] = vertexOffset - 3;
        renderIndices[renderIndexOffset + 1] = vertexOffset - 2;
        renderIndices[renderIndexOffset + 2] = vertexOffset - 1;
        indicesAdded = 3;
    }
    else if (verticesAdded == 4)
    {
        renderIndices[renderIndexOffset] = vertexOffset - 4;
        renderIndices[renderIndexOffset + 1] = vertexOffset - 3;
        renderIndices[renderIndexOffset + 2] = vertexOffset - 2;

        renderIndices[renderIndexOffset + 3] = vertexOffset - 4;
        renderIndices[renderIndexOffset + 4] = vertexOffset - 2;
        renderIndices[renderIndexOffset + 5] = vertexOffset - 1;
        indicesAdded = 6;
    }


    return indicesAdded;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiImageComponent::RenderTriangleList(ITexture* texture, SVF_P3F_C4B_T2F* vertices, uint16* indices, int numVertices, int numIndices)
{
    IRenderer* renderer = gEnv->pRenderer;

    RenderSetStates(renderer, texture);

    // This will end up using DrawIndexedPrimitive to render the quad
    renderer->DrawDynVB(vertices, indices, numVertices, numIndices, prtTriangleList);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiImageComponent::RenderTriangleStrip(ITexture* texture, SVF_P3F_C4B_T2F* vertices, uint16* indices, int numVertices, int numIndices)
{
    IRenderer* renderer = gEnv->pRenderer;

    RenderSetStates(renderer, texture);

    // This will end up using DrawIndexedPrimitive to render the quad
    renderer->DrawDynVB(vertices, indices, numVertices, numIndices, prtTriangleStrip);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiImageComponent::RenderSetStates(IRenderer* renderer, ITexture* texture)
{
    renderer->SetTexture(texture->GetTextureID());

    int blendMode = GetBlendModeStateFlags();
    renderer->SetState(blendMode | IUiRenderer::Get()->GetBaseState());
    renderer->SetColorOp(eCO_MODULATE, eCO_MODULATE, DEF_TEXARG0, DEF_TEXARG0);

    renderer->SetState(blendMode | IUiRenderer::Get()->GetBaseState());
    renderer->SetColorOp(eCO_MODULATE, eCO_MODULATE, DEF_TEXARG0, DEF_TEXARG0);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiImageComponent::SnapOffsetsToFixedImage()
{
    // check that the element is using transform2d - if not then can't adjust the offsets
    if (!UiTransform2dBus::FindFirstHandler(GetEntityId()))
    {
        return;
    }

    // if the image has no texture it will not use Fixed rendering so do nothing
    if (!m_sprite || !m_sprite->GetTexture())
    {
        return;
    }

    // get the anchors and offsets from the element's transform component
    UiTransform2dInterface::Anchors anchors;
    UiTransform2dInterface::Offsets offsets;
    EBUS_EVENT_ID_RESULT(anchors, GetEntityId(), UiTransform2dBus, GetAnchors);
    EBUS_EVENT_ID_RESULT(offsets, GetEntityId(), UiTransform2dBus, GetOffsets);

    // Get the size of the element rect before scale/rotate
    UiTransformInterface::RectPoints points;
    EBUS_EVENT_ID(GetEntityId(), UiTransformBus, GetCanvasSpacePointsNoScaleRotate, points);
    AZ::Vector2 rectSize = points.GetAxisAlignedSize();

    // get the texture size
    AZ::Vector2 textureSize = m_sprite->GetSize();

    // calculate difference in the current rect size and the texture size
    AZ::Vector2 sizeDiff = textureSize - rectSize;

    // get the pivot of the element, the fixed image will render the texture aligned with the pivot
    AZ::Vector2 pivot;
    EBUS_EVENT_ID_RESULT(pivot, GetEntityId(), UiTransformBus, GetPivot);

    // if the anchors are together (no stretching) in either dimension,
    // then adjust the offsets in that dimension to fit the texture
    bool offsetsChanged = false;
    if (anchors.m_left == anchors.m_right)
    {
        offsets.m_left -= sizeDiff.GetX() * pivot.GetX();
        offsets.m_right += sizeDiff.GetX() * (1.0f - pivot.GetX());
        offsetsChanged = true;
    }
    if (anchors.m_top == anchors.m_bottom)
    {
        offsets.m_top -= sizeDiff.GetY() * pivot.GetY();
        offsets.m_bottom += sizeDiff.GetY() * (1.0f - pivot.GetY());
        offsetsChanged = true;
    }

    if (offsetsChanged)
    {
        EBUS_EVENT_ID(GetEntityId(), UiTransform2dBus, SetOffsets, offsets);
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
int UiImageComponent::GetBlendModeStateFlags()
{
    int flags = GS_BLSRC_SRCALPHA | GS_BLDST_ONEMINUSSRCALPHA; // the default
    switch (m_blendMode)
    {
    case LyShine::BlendMode::Normal:
        // This is the default mode that does an alpha blend by interpolating based on src alpha
        flags = GS_BLSRC_SRCALPHA | GS_BLDST_ONEMINUSSRCALPHA;
        break;
    case LyShine::BlendMode::Add:
        // This works well, the amount of the src color added is controlled by src alpha
        flags = GS_BLSRC_SRCALPHA | GS_BLDST_ONE;
        break;
    case LyShine::BlendMode::Screen:
        // This is an approximation of the PhotoShop Screen mode but trying to take some account of src alpha
        flags = GS_BLSRC_SRCALPHA | GS_BLDST_ONEMINUSSRCCOL;
        break;
    case LyShine::BlendMode::Darken:
        // This is an approximation of the PhotoShop Darken mode but trying to take some account of src alpha
        flags = GS_BLOP_MIN | GS_BLSRC_ONEMINUSSRCALPHA | GS_BLDST_ONE;
        break;
    case LyShine::BlendMode::Lighten:
        // This is an approximation of the PhotoShop Lighten mode but trying to take some account of src alpha
        flags = GS_BLOP_MAX | GS_BLSRC_SRCALPHA | GS_BLDST_ONE;
        break;
    }

    return flags;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
bool UiImageComponent::IsPixelAligned()
{
    AZ::EntityId canvasEntityId;
    EBUS_EVENT_ID_RESULT(canvasEntityId, GetEntityId(), UiElementBus, GetCanvasEntityId);
    bool isPixelAligned = true;
    EBUS_EVENT_ID_RESULT(isPixelAligned, canvasEntityId, UiCanvasBus, GetIsPixelAligned);
    return isPixelAligned;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
bool UiImageComponent::IsSpriteTypeAsset()
{
    return m_spriteType == UiImageInterface::SpriteType::SpriteAsset;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
bool UiImageComponent::IsSpriteTypeSpriteSheet()
{
    return GetSpriteSheetCellCount() > 1;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
bool UiImageComponent::IsSpriteTypeRenderTarget()
{
    return (m_spriteType == UiImageInterface::SpriteType::RenderTarget);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
bool UiImageComponent::IsFilled()
{
    return (m_fillType != FillType::None);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
bool UiImageComponent::IsLinearFilled()
{
    return (m_fillType == FillType::Linear);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
bool UiImageComponent::IsRadialFilled()
{
    return (m_fillType == FillType::Radial);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
bool UiImageComponent::IsRadialAnyFilled()
{
    return (m_fillType == FillType::Radial) || (m_fillType == FillType::RadialCorner) || (m_fillType == FillType::RadialEdge);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
bool UiImageComponent::IsCornerFilled()
{
    return (m_fillType == FillType::RadialCorner);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
bool UiImageComponent::IsEdgeFilled()
{
    return (m_fillType == FillType::RadialEdge) || (m_fillType == FillType::Linear);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
bool UiImageComponent::IsSliced()
{
    return m_imageType == ImageType::Sliced;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiImageComponent::OnEditorSpritePathnameChange()
{
    OnSpritePathnameChange();
    EBUS_EVENT(UiEditorChangeNotificationBus, OnEditorPropertiesRefreshEntireTree);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiImageComponent::OnSpritePathnameChange()
{
    ISprite* newSprite = nullptr;

    if (!m_spritePathname.GetAssetPath().empty())
    {
        // Load the new texture.
        newSprite = gEnv->pLyShine->LoadSprite(m_spritePathname.GetAssetPath().c_str());
    }

    SAFE_RELEASE(m_sprite);
    m_sprite = newSprite;

    InvalidateLayouts();
    ResetSpriteSheetCellIndex();
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiImageComponent::OnSpriteRenderTargetNameChange()
{
    ISprite* newSprite = nullptr;

    if (!m_renderTargetName.empty())
    {
        newSprite = gEnv->pLyShine->CreateSprite(m_renderTargetName.c_str());
    }

    SAFE_RELEASE(m_sprite);
    m_sprite = newSprite;

    InvalidateLayouts();
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiImageComponent::OnSpriteTypeChange()
{
    if (IsSpriteTypeAsset())
    {
        OnSpritePathnameChange();
    }
    else if (m_spriteType == UiImageInterface::SpriteType::RenderTarget)
    {
        OnSpriteRenderTargetNameChange();
    }
    else
    {
        AZ_Assert(false, "unhandled sprite type");
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiImageComponent::OnColorChange()
{
    m_overrideColor = m_color;
    m_overrideAlpha = m_alpha;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiImageComponent::InvalidateLayouts()
{
    AZ::EntityId canvasEntityId;
    EBUS_EVENT_ID_RESULT(canvasEntityId, GetEntityId(), UiElementBus, GetCanvasEntityId);
    EBUS_EVENT_ID(canvasEntityId, UiLayoutManagerBus, MarkToRecomputeLayoutsAffectedByLayoutCellChange, GetEntityId(), true);
    EBUS_EVENT_ID(canvasEntityId, UiLayoutManagerBus, MarkToRecomputeLayout, GetEntityId());
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiImageComponent::OnIndexChange()
{
    // Index update logic will go here
}

////////////////////////////////////////////////////////////////////////////////////////////////////
UiImageComponent::AZu32ComboBoxVec UiImageComponent::PopulateIndexStringList() const
{
    // There may not be a sprite loaded for this component
    if (m_sprite)
    {
        const AZ::u32 numCells = m_sprite->GetSpriteSheetCells().size();

        if (numCells != 0)
        {
            return GetEnumSpriteIndexList(0, numCells - 1, m_sprite);
        }
    }

    return AZu32ComboBoxVec();
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiImageComponent::CheckLayoutFitterAndRefreshEditorTransformProperties() const
{
    UiLayoutHelpers::CheckFitterAndRefreshEditorTransformProperties(GetEntityId());
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// PRIVATE STATIC MEMBER FUNCTIONS
////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////
bool UiImageComponent::VersionConverter(AZ::SerializeContext& context,
    AZ::SerializeContext::DataElementNode& classElement)
{
    // conversion from version 1:
    // - Need to convert CryString elements to AZStd::string
    // - Need to convert Color to Color and Alpha
    if (classElement.GetVersion() <= 1)
    {
        if (!LyShine::ConvertSubElementFromCryStringToAzString(context, classElement, "SpritePath"))
        {
            return false;
        }

        if (!LyShine::ConvertSubElementFromColorToColorPlusAlpha(context, classElement, "Color", "Alpha"))
        {
            return false;
        }
    }

    // conversion from version 1 or 2 to current:
    // - Need to convert AZStd::string sprites to AzFramework::SimpleAssetReference<LmbrCentral::TextureAsset>
    if (classElement.GetVersion() <= 2)
    {
        if (!LyShine::ConvertSubElementFromAzStringToAssetRef<LmbrCentral::TextureAsset>(context, classElement, "SpritePath"))
        {
            return false;
        }
    }

    // conversion from version 3 to current:
    // - Strip off any leading forward slashes from sprite path
    if (classElement.GetVersion() <= 3)
    {
        if (!LyShine::RemoveLeadingForwardSlashesFromAssetPath(context, classElement, "SpritePath"))
        {
            return false;
        }
    }

    // conversion from version 4 to current:
    // - Need to convert AZ::Vector3 to AZ::Color
    if (classElement.GetVersion() <= 4)
    {
        if (!LyShine::ConvertSubElementFromVector3ToAzColor(context, classElement, "Color"))
        {
            return false;
        }
    }

    return true;
}
