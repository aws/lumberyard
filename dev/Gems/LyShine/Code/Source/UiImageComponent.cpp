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
#include "StdAfx.h"
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
#include <LyShine/ISprite.h>
#include <LyShine/IUiRenderer.h>
#include <UiFaderComponent.h>

#include "UiSerialize.h"
#include "Sprite.h"
#include "UiLayoutHelpers.h"

////////////////////////////////////////////////////////////////////////////////////////////////////
// PUBLIC MEMBER FUNCTIONS
////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////
UiImageComponent::UiImageComponent()
    : m_spritePathname()
    , m_spriteType(SpriteType::SpriteAsset)
    , m_sprite(nullptr)
    , m_color(1.0f, 1.0f, 1.0f, 1.0f)
    , m_alpha(1.0f)
    , m_imageType(ImageType::Stretched)
    , m_blendMode(LyShine::BlendMode::Normal)
    , m_overrideColor(m_color)
    , m_overrideAlpha(m_alpha)
    , m_overrideSprite(nullptr)
    , m_isColorOverridden(false)
    , m_isAlphaOverridden(false)
{
}

////////////////////////////////////////////////////////////////////////////////////////////////////
UiImageComponent::~UiImageComponent()
{
    SAFE_RELEASE(m_sprite);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiImageComponent::ResetOverrides()
{
    m_overrideColor = m_color;
    m_overrideAlpha = m_alpha;
    m_overrideSprite = nullptr;

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
void UiImageComponent::SetOverrideSprite(ISprite* sprite)
{
    m_overrideSprite = sprite;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiImageComponent::Render()
{
    const char* profileMarker = "UI_IMAGE";
    gEnv->pRenderer->PushProfileMarker(profileMarker);

    // compute fade (short-term solution has dependency on CUiFadeComponent)
    float fade = UiFaderComponent::ComputeElementFadeValue(GetEntity());

    ISprite* sprite = (m_overrideSprite) ? m_overrideSprite : m_sprite;

    ITexture* texture = (sprite) ? sprite->GetTexture() : nullptr;

    ImageType imageType = m_imageType;

    if (!texture)
    {
        // if there is no texture we will just use a white texture and want to stretch it
        texture = gEnv->pRenderer->EF_GetTextureByID(gEnv->pRenderer->GetWhiteTextureId());
        imageType = ImageType::Stretched;
    }

    // If the borders are zero width then sliced is the same as stretched and stretched is simpler to render
    if (imageType == ImageType::Sliced && sprite->AreBordersZeroWidth())
    {
        imageType = ImageType::Stretched;
    }

    switch (imageType)
    {
    case ImageType::Stretched:
        RenderStretchedSprite(texture, fade);
        break;
    case ImageType::Sliced:
        AZ_Assert(sprite, "Should not get here if no sprite path is specified");
        RenderSlicedSprite(sprite, fade);   // will not get here if sprite is null since we change type in that case above
        break;
    case ImageType::Fixed:
        AZ_Assert(sprite, "Should not get here if no sprite path is specified");
        RenderFixedSprite(sprite, fade);
        break;
    case ImageType::Tiled:
        AZ_Assert(sprite, "Should not get here if no sprite path is specified");
        RenderTiledSprite(sprite, fade);
        break;
    case ImageType::StretchedToFit:
        AZ_Assert(sprite, "Should not get here if no sprite path is specified");
        RenderStretchedToFitOrFillSprite(sprite, fade, true);
        break;
    case ImageType::StretchedToFill:
        AZ_Assert(sprite, "Should not get here if no sprite path is specified");
        RenderStretchedToFitOrFillSprite(sprite, fade, false);
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

    if (m_spriteType == UiImageInterface::SpriteType::SpriteAsset)
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
            ->Version(5, &VersionConverter)
            ->Field("SpriteType", &UiImageComponent::m_spriteType)
            ->Field("SpritePath", &UiImageComponent::m_spritePathname)
            ->Field("RenderTargetName", &UiImageComponent::m_renderTargetName)
            ->Field("Color", &UiImageComponent::m_color)
            ->Field("Alpha", &UiImageComponent::m_alpha)
            ->Field("ImageType", &UiImageComponent::m_imageType)
            ->Field("BlendMode", &UiImageComponent::m_blendMode);

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
                ->Attribute(AZ::Edit::Attributes::ChangeNotify, &UiImageComponent::OnSpritePathnameChange)
                ->Attribute(AZ::Edit::Attributes::ChangeNotify, &UiImageComponent::CheckLayoutFitterAndRefreshEditorTransformProperties);
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
                ->Attribute(AZ::Edit::Attributes::ChangeNotify, &UiImageComponent::InvalidateLayouts)
                ->Attribute(AZ::Edit::Attributes::ChangeNotify, &UiImageComponent::CheckLayoutFitterAndRefreshEditorTransformProperties);
            editInfo->DataElement(AZ::Edit::UIHandlers::ComboBox, &UiImageComponent::m_blendMode, "BlendMode", "The blend mode used to draw the image")
                ->EnumAttribute(LyShine::BlendMode::Normal, "Normal")
                ->EnumAttribute(LyShine::BlendMode::Add, "Add")
                ->EnumAttribute(LyShine::BlendMode::Screen, "Screen")
                ->EnumAttribute(LyShine::BlendMode::Darken, "Darken")
                ->EnumAttribute(LyShine::BlendMode::Lighten, "Lighten");
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
            ->Enum<(int)UiImageInterface::SpriteType::RenderTarget>("eUiSpriteType_RenderTarget");

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
            ->Event("SetImageType", &UiImageBus::Events::SetImageType);
    }
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
        if (m_spriteType == UiImageInterface::SpriteType::SpriteAsset)
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
// PRIVATE MEMBER FUNCTIONS
////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiImageComponent::RenderStretchedSprite(ITexture* texture, float fade)
{
    UiTransformInterface::RectPoints points;
    EBUS_EVENT_ID(GetEntityId(), UiTransformBus, GetViewportSpacePoints, points);

    texture->SetClamp(true);

    // points are a clockwise quad
    static const AZ::Vector2 uvs[4] = { AZ::Vector2(0, 0), AZ::Vector2(1, 0), AZ::Vector2(1, 1), AZ::Vector2(0, 1) };
    RenderSingleQuad(texture, points.pt, uvs, fade);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiImageComponent::RenderSlicedSprite(ISprite* sprite, float fade)
{
    ITexture* texture = sprite->GetTexture();
    texture->SetClamp(true);

    // get the details of the texture
    AZ::Vector2 textureSize = sprite->GetSize();

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

    float leftBorder = sprite->GetBorders().m_left;
    float rightBorder = 1.0f - sprite->GetBorders().m_right;
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

    float topBorder = sprite->GetBorders().m_top;
    float bottomBorder = 1.0f - sprite->GetBorders().m_bottom;
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

    // The texture coords are just based on the border values
    float sValues[] = { 0, leftBorder, 1.0f - rightBorder, 1 };
    float tValues[] = { 0, topBorder, 1.0f - bottomBorder, 1 };

    // fill out the 16 verts to define the 3x3 set of quads
    const int32 NUM_VERTS = 16; // to create a 4x4 grid making 9 quads
    SVF_P3F_C4B_T2F vertices[NUM_VERTS];
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
    // The indicies are for one triangle strip which looks like this
    //  |\|\|\|     ->
    //  |\|\|\|     <-
    //  |\|\|\|     ->
    const int32 NUM_INDICES = 26; // 8 for each row, plus two for degenerates to jump rows
    uint16 indicies[NUM_INDICES] =
    {
        4,  0,   5,  1,   6,  2,   7,  3,   3,
        7, 11,   6, 10,   5,  9,   4,  8,   8,
        12,  8,  13,  9,  14, 10,  15, 11,
    };

    IRenderer* renderer = gEnv->pRenderer;
    renderer->SetTexture(texture->GetTextureID());

    int blendMode = GetBlendModeStateFlags();
    renderer->SetState(blendMode | IUiRenderer::Get()->GetBaseState());
    renderer->SetColorOp(eCO_MODULATE, eCO_MODULATE, DEF_TEXARG0, DEF_TEXARG0);

    // This will end up using DrawIndexedPrimitive to render the quad
    renderer->DrawDynVB(vertices, indicies, NUM_VERTS, NUM_INDICES, prtTriangleStrip);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiImageComponent::RenderFixedSprite(ISprite* sprite, float fade)
{
    ITexture* texture = sprite->GetTexture();
    texture->SetClamp(true);

    AZ::Vector2 textureSize = sprite->GetSize();

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
    static const AZ::Vector2 uvs[4] = { AZ::Vector2(0, 0), AZ::Vector2(1, 0), AZ::Vector2(1, 1), AZ::Vector2(0, 1) };
    RenderSingleQuad(texture, points.pt, uvs, fade);
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
    RenderSingleQuad(texture, points.pt, uvs, fade);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiImageComponent::RenderStretchedToFitOrFillSprite(ISprite* sprite, float fade, bool toFit)
{
    ITexture* texture = sprite->GetTexture();
    texture->SetClamp(true);

    AZ::Vector2 textureSize = sprite->GetSize();

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
    static const AZ::Vector2 uvs[4] = { AZ::Vector2(0, 0), AZ::Vector2(1, 0), AZ::Vector2(1, 1), AZ::Vector2(0, 1) };
    RenderSingleQuad(texture, points.pt, uvs, fade);
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
    return (m_spriteType == UiImageInterface::SpriteType::SpriteAsset);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
bool UiImageComponent::IsSpriteTypeRenderTarget()
{
    return (m_spriteType == UiImageInterface::SpriteType::RenderTarget);
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
    if (m_spriteType == UiImageInterface::SpriteType::SpriteAsset)
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
