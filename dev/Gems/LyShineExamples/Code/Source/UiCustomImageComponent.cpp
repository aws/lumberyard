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
#include "LyShineExamples_precompiled.h"
#include "UiCustomImageComponent.h"

#include <LyShineExamples/UiCustomImageBus.h>

#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/RTTI/BehaviorContext.h>

#include <IRenderer.h>

#include <LyShine/IDraw2d.h>
#include <LyShine/ISprite.h>
#include <LyShine/IUiRenderer.h>
#include <LyShine/Bus/UiElementBus.h>
#include <LyShine/Bus/UiCanvasBus.h>
#include <LyShine/Bus/UiTransformBus.h>

namespace LyShineExamples
{
    ////////////////////////////////////////////////////////////////////////////////////////////////////
    // PUBLIC MEMBER FUNCTIONS
    ////////////////////////////////////////////////////////////////////////////////////////////////////

    ////////////////////////////////////////////////////////////////////////////////////////////////////
    UiCustomImageComponent::UiCustomImageComponent()
        : m_color(1.f, 1.f, 1.f, 1.f)
        , m_alpha(1.f)
        , m_sprite(nullptr)
        , m_uvs(0, 0, 1, 1)
        , m_clamp(true)
        , m_overrideColor(m_color)
        , m_overrideAlpha(m_alpha)
        , m_overrideSprite(nullptr)
    {
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////
    UiCustomImageComponent::~UiCustomImageComponent()
    {
        SAFE_RELEASE(m_sprite);
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////
    void UiCustomImageComponent::ResetOverrides()
    {
        m_overrideColor = m_color;
        m_overrideAlpha = m_alpha;
        m_overrideSprite = nullptr;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////
    void UiCustomImageComponent::SetOverrideColor(const AZ::Color& color)
    {
        m_overrideColor.Set(color.GetAsVector3());
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////
    void UiCustomImageComponent::SetOverrideAlpha(float alpha)
    {
        m_overrideAlpha = alpha;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////
    void UiCustomImageComponent::SetOverrideSprite(ISprite* sprite, AZ::u32 /* cellIndex */)
    {
        m_overrideSprite = sprite;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////
    void UiCustomImageComponent::Render()
    {
        ISprite* sprite = (m_overrideSprite) ? m_overrideSprite : m_sprite;
        ITexture* texture = (sprite) ? sprite->GetTexture() : nullptr;

        if (!texture)
        {
            // if there is no texture we will just use a white texture
            texture = gEnv->pRenderer->EF_GetTextureByID(gEnv->pRenderer->GetWhiteTextureId());
        }

        RenderSprite(texture);
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////
    AZ::Color UiCustomImageComponent::GetColor()
    {
        return AZ::Color::CreateFromVector3AndFloat(m_color.GetAsVector3(), m_alpha);
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////
    void UiCustomImageComponent::SetColor(const AZ::Color& color)
    {
        m_color.Set(color.GetAsVector3());
        m_alpha = color.GetA();
        m_overrideColor = m_color;
        m_overrideAlpha = m_alpha;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////
    ISprite* UiCustomImageComponent::GetSprite()
    {
        return m_sprite;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////
    void UiCustomImageComponent::SetSprite(ISprite* sprite)
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
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////
    AZStd::string UiCustomImageComponent::GetSpritePathname()
    {
        return m_spritePathname.GetAssetPath();
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////
    void UiCustomImageComponent::SetSpritePathname(AZStd::string spritePath)
    {
        m_spritePathname.SetAssetPath(spritePath.c_str());
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////
    UiCustomImageInterface::UVRect UiCustomImageComponent::GetUVs()
    {
        return m_uvs;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////
    void UiCustomImageComponent::SetUVs(UiCustomImageInterface::UVRect uvs)
    {
        m_uvs = uvs;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////
    bool UiCustomImageComponent::GetClamp()
    {
        return m_clamp;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////
    void UiCustomImageComponent::SetClamp(bool clamp)
    {
        m_clamp = clamp;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////
    // PUBLIC STATIC MEMBER FUNCTIONS
    ////////////////////////////////////////////////////////////////////////////////////////////////////

    ////////////////////////////////////////////////////////////////////////////////////////////////////
    void UiCustomImageComponent::Reflect(AZ::ReflectContext* context)
    {
        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
        AZ::BehaviorContext* behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context);

        // Serialize this component
        if (serializeContext)
        {
            serializeContext->Class<UiCustomImageComponent, AZ::Component>()
                ->Field("SpritePath", &UiCustomImageComponent::m_spritePathname)
                ->Field("Color", &UiCustomImageComponent::m_color)
                ->Field("Alpha", &UiCustomImageComponent::m_alpha)
                ->Field("UVCoords", &UiCustomImageComponent::m_uvs)
                ->Field("Clamp", &UiCustomImageComponent::m_clamp);

            AZ::EditContext* ec = serializeContext->GetEditContext();
            if (ec)
            {
                auto editInfo = ec->Class<UiCustomImageComponent>("Custom Image", "A visual component to draw a rectangle with an optional sprite/texture");

                editInfo->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::Icon, "Editor/Icons/Components/UiImage.png")
                    ->Attribute(AZ::Edit::Attributes::ViewportIcon, "Editor/Icons/Components/Viewport/UiImage.png")
                    ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC("UI", 0x27ff46b0))
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true);

                editInfo->DataElement("Sprite", &UiCustomImageComponent::m_spritePathname, "Sprite path", "The sprite path. Can be overridden by another component such as an interactable.")
                    ->Attribute(AZ::Edit::Attributes::ChangeNotify, &UiCustomImageComponent::OnSpritePathnameChange);

                editInfo->DataElement(AZ::Edit::UIHandlers::Color, &UiCustomImageComponent::m_color, "Color", "The color tint for the image. Can be overridden by another component such as an interactable.")
                    ->Attribute(AZ::Edit::Attributes::ChangeNotify, &UiCustomImageComponent::OnColorChange);

                editInfo->DataElement(AZ::Edit::UIHandlers::Slider, &UiCustomImageComponent::m_alpha, "Alpha", "The transparency. Can be overridden by another component such as an interactable.")
                    ->Attribute(AZ::Edit::Attributes::ChangeNotify, &UiCustomImageComponent::OnColorChange)
                    ->Attribute(AZ::Edit::Attributes::Min, 0.0f)
                    ->Attribute(AZ::Edit::Attributes::Max, 1.0f);

                editInfo->DataElement(0, &UiCustomImageComponent::m_uvs, "UV Rect", "The UV coordinates of the rectangle for rendering the texture.")
                    ->Attribute(AZ::Edit::Attributes::ChangeNotify, AZ_CRC("RefreshValues", 0x28e720d4))
                    ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::Show); // needed because sub-elements are hidden

                editInfo->DataElement(AZ::Edit::UIHandlers::CheckBox, &UiCustomImageComponent::m_clamp, "Clamp", "Whether the image should be clamped or not.")
                    ->Attribute(AZ::Edit::Attributes::ChangeNotify, AZ_CRC("RefreshValues", 0x28e720d4));
            }
        }

        if (behaviorContext)
        {
            behaviorContext->EBus<UiCustomImageBus>("UiCustomImageBus")
                ->Event("GetColor", &UiCustomImageBus::Events::GetColor)
                ->Event("SetColor", &UiCustomImageBus::Events::SetColor)
                ->Event("GetSpritePathname", &UiCustomImageBus::Events::GetSpritePathname)
                ->Event("SetSpritePathname", &UiCustomImageBus::Events::SetSpritePathname)
                ->Event("GetUVs", &UiCustomImageBus::Events::GetUVs)
                ->Event("SetUVs", &UiCustomImageBus::Events::SetUVs)
                ->Event("GetClamp", &UiCustomImageBus::Events::GetClamp)
                ->Event("SetClamp", &UiCustomImageBus::Events::SetClamp);
        }
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////
    // PROTECTED MEMBER FUNCTIONS
    ////////////////////////////////////////////////////////////////////////////////////////////////////

    ////////////////////////////////////////////////////////////////////////////////////////////////////
    void UiCustomImageComponent::Init()
    {
        // If this is called from RC.exe for example these pointers will not be set. In that case
        // we only need to be able to load, init and save the component. It will never be
        // activated.
        if (!(gEnv && gEnv->pLyShine))
        {
            return;
        }

        // Load our sprite from the path at the beginning of the game
        if (!m_sprite)
        {
            if (!m_spritePathname.GetAssetPath().empty())
            {
                m_sprite = gEnv->pLyShine->LoadSprite(m_spritePathname.GetAssetPath().c_str());
            }
        }

        m_overrideColor = m_color;
        m_overrideAlpha = m_alpha;
    }


    ////////////////////////////////////////////////////////////////////////////////////////////////////
    void UiCustomImageComponent::Activate()
    {
        UiVisualBus::Handler::BusConnect(m_entity->GetId());
        UiRenderBus::Handler::BusConnect(m_entity->GetId());
        UiCustomImageBus::Handler::BusConnect(m_entity->GetId());
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////
    void UiCustomImageComponent::Deactivate()
    {
        UiVisualBus::Handler::BusDisconnect();
        UiRenderBus::Handler::BusDisconnect();
        UiCustomImageBus::Handler::BusDisconnect();
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////
    // PRIVATE MEMBER FUNCTIONS
    ////////////////////////////////////////////////////////////////////////////////////////////////////

    ////////////////////////////////////////////////////////////////////////////////////////////////////
    void UiCustomImageComponent::RenderSprite(ITexture* texture)
    {
        UiTransformInterface::RectPoints points;
        EBUS_EVENT_ID(GetEntityId(), UiTransformBus, GetViewportSpacePoints, points);

        texture->SetClamp(m_clamp);

        // points are a clockwise quad
        const AZ::Vector2 uvs[4] = {
            AZ::Vector2(m_uvs.m_left, m_uvs.m_top), AZ::Vector2(m_uvs.m_right, m_uvs.m_top)
            , AZ::Vector2(m_uvs.m_right, m_uvs.m_bottom), AZ::Vector2(m_uvs.m_left, m_uvs.m_bottom)
        };
        RenderSingleQuad(texture, points.pt, uvs);
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////
    void UiCustomImageComponent::RenderSingleQuad(ITexture* texture, const AZ::Vector2* positions, const AZ::Vector2* uvs)
    {
        AZ::Color color = AZ::Color::CreateFromVector3AndFloat(m_overrideColor.GetAsVector3(), m_overrideAlpha);
        color = color.GammaToLinear();   // the colors are specified in sRGB but we want linear colors in the shader

        // points are a clockwise quad
        IDraw2d::VertexPosColUV verts[4];
        for (int i = 0; i < 4; ++i)
        {
            verts[i].position = positions[i];
            verts[i].color = color;
            verts[i].uv = uvs[i];
        }

        IDraw2d::Rounding rounding = IsPixelAligned() ? IDraw2d::Rounding::Nearest : IDraw2d::Rounding::None;
        Draw2dHelper::GetDraw2d()->DrawQuad(texture->GetTextureID(), verts, GS_BLSRC_SRCALPHA | GS_BLDST_ONEMINUSSRCALPHA, rounding, IUiRenderer::Get()->GetBaseState());
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////
    bool UiCustomImageComponent::IsPixelAligned()
    {
        AZ::EntityId canvasEntityId;
        EBUS_EVENT_ID_RESULT(canvasEntityId, GetEntityId(), UiElementBus, GetCanvasEntityId);
        bool isPixelAligned = true;
        EBUS_EVENT_ID_RESULT(isPixelAligned, canvasEntityId, UiCanvasBus, GetIsPixelAligned);
        return isPixelAligned;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////
    void UiCustomImageComponent::OnSpritePathnameChange()
    {
        ISprite* newSprite = nullptr;

        if (!m_spritePathname.GetAssetPath().empty())
        {
            // Load the new texture.
            newSprite = gEnv->pLyShine->LoadSprite(m_spritePathname.GetAssetPath().c_str());
        }

        SAFE_RELEASE(m_sprite);
        m_sprite = newSprite;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////
    void UiCustomImageComponent::OnColorChange()
    {
        m_overrideColor = m_color;
        m_overrideAlpha = m_alpha;
    }
}
