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

#include <LyShine/Bus/UiVisualBus.h>
#include <LyShine/Bus/UiRenderBus.h>
#include <LyShine/Bus/UiImageBus.h>
#include <LyShine/Bus/UiAnimateEntityBus.h>
#include <LyShine/Bus/UiLayoutCellDefaultBus.h>
#include <LyShine/UiComponentTypes.h>

#include <AzCore/Component/Component.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Math/Vector3.h>

#include <LmbrCentral/Rendering/MaterialAsset.h>

class ITexture;
class ISprite;

////////////////////////////////////////////////////////////////////////////////////////////////////
class UiImageComponent
    : public AZ::Component
    , public UiVisualBus::Handler
    , public UiRenderBus::Handler
    , public UiImageBus::Handler
    , public UiAnimateEntityBus::Handler
    , public UiLayoutCellDefaultBus::Handler
{
public: // member functions

    AZ_COMPONENT(UiImageComponent, LyShine::UiImageComponentUuid, AZ::Component);

    UiImageComponent();
    ~UiImageComponent() override;

    // UiVisualInterface
    void ResetOverrides() override;
    void SetOverrideColor(const AZ::Color& color) override;
    void SetOverrideAlpha(float alpha) override;
    void SetOverrideSprite(ISprite* sprite) override;
    // ~UiVisualInterface

    // UiRenderInterface
    void Render() override;
    // ~UiRenderInterface

    // UiImageInterface
    AZ::Color GetColor() override;
    void SetColor(const AZ::Color& color) override;
    ISprite* GetSprite() override;
    void SetSprite(ISprite* sprite) override;
    AZStd::string GetSpritePathname() override;
    void SetSpritePathname(AZStd::string spritePath) override;
    AZStd::string GetRenderTargetName() override;
    void SetRenderTargetName(AZStd::string renderTargetName) override;
    SpriteType GetSpriteType() override;
    void SetSpriteType(SpriteType spriteType) override;
    ImageType GetImageType() override;
    void SetImageType(ImageType imageType) override;
    // ~UiImageInterface

    // UiAnimateEntityInterface
    void PropertyValuesChanged() override;
    // ~UiAnimateEntityInterface

    // UiLayoutCellDefaultInterface
    float GetMinWidth() override;
    float GetMinHeight() override;
    float GetTargetWidth() override;
    float GetTargetHeight() override;
    float GetExtraWidthRatio() override;
    float GetExtraHeightRatio() override;
    // ~UiLayoutCellDefaultInterface

public:  // static member functions

    static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
    {
        provided.push_back(AZ_CRC("UiVisualService", 0xa864fdf8));
    }

    static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
    {
        incompatible.push_back(AZ_CRC("UiVisualService", 0xa864fdf8));
    }

    static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required)
    {
        required.push_back(AZ_CRC("UiElementService", 0x3dca7ad4));
        required.push_back(AZ_CRC("UiTransformService", 0x3a838e34));
    }

    static void Reflect(AZ::ReflectContext* context);

protected: // member functions

    // AZ::Component
    void Init() override;
    void Activate() override;
    void Deactivate() override;
    // ~AZ::Component

private: // member functions

    void RenderStretchedSprite(ITexture* texture, float fade);
    void RenderSlicedSprite(ISprite* sprite, float fade);
    void RenderFixedSprite(ISprite* sprite, float fade);
    void RenderTiledSprite(ISprite* sprite, float fade);
    void RenderStretchedToFitOrFillSprite(ISprite* sprite, float fade, bool toFit);

    void RenderSingleQuad(ITexture* texture, const AZ::Vector2* positions, const AZ::Vector2* uvs, float fade);

    void SnapOffsetsToFixedImage();
    int GetBlendModeStateFlags();

    bool IsPixelAligned();

    bool IsSpriteTypeAsset();
    bool IsSpriteTypeRenderTarget();

    AZ_DISABLE_COPY_MOVE(UiImageComponent);

    void OnSpritePathnameChange();
    void OnSpriteRenderTargetNameChange();
    void OnSpriteTypeChange();

    // ChangeNotify callback for color change
    void OnColorChange();

    //! Invalidate this element and its parent's layouts. Called when a property that 
    //! is used to calculate default layout cell values has changed
    void InvalidateLayouts();

    //! Refresh the transform properties in the editor's properties pane
    void CheckLayoutFitterAndRefreshEditorTransformProperties() const;

private: // static member functions

    static bool VersionConverter(AZ::SerializeContext& context,
        AZ::SerializeContext::DataElementNode& classElement);

private: // data

    AzFramework::SimpleAssetReference<LmbrCentral::TextureAsset> m_spritePathname;
    AZStd::string m_renderTargetName;
    SpriteType m_spriteType;
    AZ::Color m_color;
    float m_alpha;
    ImageType m_imageType;
    LyShine::BlendMode m_blendMode;

    ISprite* m_sprite;

    ISprite* m_overrideSprite;
    AZ::Color m_overrideColor;
    float m_overrideAlpha;

    bool m_isColorOverridden;
    bool m_isAlphaOverridden;
};
