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
struct SVF_P3F_C4B_T2F;

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
    void SetOverrideSprite(ISprite* sprite, AZ::u32 cellIndex = 0) override;
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
    void SetSpriteSheetCellIndex(AZ::u32 index) override;
    const AZ::u32 GetSpriteSheetCellIndex() override;
    const AZ::u32 GetSpriteSheetCellCount() override;
    FillType GetFillType() override;
    void SetFillType(FillType fillType) override;
    float GetFillAmount() override;
    void SetFillAmount(float fillAmount) override;
    float GetRadialFillStartAngle() override;
    void SetRadialFillStartAngle(float radialFillStartAngle) override;
    FillCornerOrigin GetCornerFillOrigin() override;
    void SetCornerFillOrigin(FillCornerOrigin cornerOrigin) override;
    FillEdgeOrigin GetEdgeFillOrigin() override;
    void SetEdgeFillOrigin(FillEdgeOrigin edgeOrigin) override;
    bool GetFillClockwise() override;
    void SetFillClockwise(bool fillClockwise) override;
    bool GetFillCenter() override;
    void SetFillCenter(bool fillCenter) override;
    AZStd::string GetSpriteSheetCellAlias(AZ::u32 index) override;
    void SetSpriteSheetCellAlias(AZ::u32 index, const AZStd::string& alias) override;
    AZ::u32 GetSpriteSheetCellIndexFromAlias(const AZStd::string& alias) override;
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
        provided.push_back(AZ_CRC("UiImageService"));
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

    using AZu32ComboBoxVec = AZStd::vector<AZStd::pair<AZ::u32, AZStd::string> >;

    //! Returns a string enumeration list for the given min/max value ranges
    static AZu32ComboBoxVec GetEnumSpriteIndexList(AZ::u32 indexMin, AZ::u32 indexMax, ISprite* sprite = nullptr, const char* errorMessage = "");

protected: // member functions

    // AZ::Component
    void Init() override;
    void Activate() override;
    void Deactivate() override;
    // ~AZ::Component

    //! Resets the current sprite-sheet cell index based on whether we're showing a sprite or a sprite-sheet.
    //!
    //! This is necessary since the render routines reference the sprite-sheet cell index regardless of
    //! whether a sprite-sheet is being displayed or not. It's possible to have a sprite-sheet asset loaded
    //! but the image component sprite type be a basic sprite. In that case, indexing the sprite-sheet is
    //! still technically possible, so we assign a special index (-1) to indicate not to index a particular
    //! cell, but rather the whole image.
    void ResetSpriteSheetCellIndex();

private: // member functions

    void RenderStretchedSprite(ISprite* sprite, int cellIndex, float fade);
    void RenderSlicedSprite(ISprite* sprite, int cellIndex, float fade);
    void RenderFixedSprite(ISprite* sprite, int cellIndex, float fade);
    void RenderTiledSprite(ISprite* sprite, float fade);
    void RenderStretchedToFitOrFillSprite(ISprite* sprite, int cellIndex, float fade, bool toFit);

    void RenderSingleQuad(ITexture* texture, const AZ::Vector2* positions, const AZ::Vector2* uvs, float fade);

    void RenderFilledQuad(ITexture* texture, const AZ::Vector2* positions, const AZ::Vector2* uvs, float fade);
    void RenderLinearFilledQuad(ITexture* texture, const AZ::Vector2* positions, const AZ::Vector2* uvs, float fade, AZ::Color color);
    void RenderRadialFilledQuad(ITexture* texture, const AZ::Vector2* positions, const AZ::Vector2* uvs, float fade, AZ::Color color);
    void RenderRadialCornerFilledQuad(ITexture* texture, const AZ::Vector2* positions, const AZ::Vector2* uvs, float fade, AZ::Color color);
    void RenderRadialEdgeFilledQuad(ITexture* texture, const AZ::Vector2* positions, const AZ::Vector2* uvs, float fade, AZ::Color color);
    void RenderSlicedLinearFilledSprite(ISprite* sprite, int cellIndex, ITexture* texture, uint32 packedColor, float topBorder, float leftBorder, float bottomBorder, float rightBorder);
    void RenderSlicedRadialFilledSprite(ISprite* sprite, int cellIndex, ITexture* texture, uint32 packedColor, float topBorder, float leftBorder, float bottomBorder, float rightBorder);
    void RenderSlicedRadialCornerOrEdgeFilledSprite(ISprite* sprite, int cellIndex, ITexture* texture, uint32 packedColor, float topBorder, float leftBorder, float bottomBorder, float rightBorder);
    int ClipToLine(SVF_P3F_C4B_T2F* vertices, uint16* indices, SVF_P3F_C4B_T2F* newVertex, uint16* renderIndices, int& vertexOffset, int idxOffset, const Vec3& lineOrigin, const Vec3& lineEnd);

    void RenderTriangleList(ITexture* texture, SVF_P3F_C4B_T2F* vertices, uint16* indices, int numVertices, int numIndices);
    void RenderTriangleStrip(ITexture* texture, SVF_P3F_C4B_T2F* vertices, uint16* indices, int numVertices, int numIndices);
    void RenderSetStates(IRenderer* renderer, ITexture* texture);

    void SnapOffsetsToFixedImage();
    int GetBlendModeStateFlags();

    bool IsPixelAligned();

    bool IsSpriteTypeAsset();
    bool IsSpriteTypeSpriteSheet();
    bool IsSpriteTypeRenderTarget();

    bool IsFilled();
    bool IsLinearFilled();
    bool IsRadialFilled();
    bool IsRadialAnyFilled();
    bool IsCornerFilled();
    bool IsEdgeFilled();

    bool IsSliced();

    AZ_DISABLE_COPY_MOVE(UiImageComponent);

    void OnEditorSpritePathnameChange();
    void OnSpritePathnameChange();
    void OnSpriteRenderTargetNameChange();
    void OnSpriteTypeChange();

    //! ChangeNotify callback for color change
    void OnColorChange();

    //! Invalidate this element and its parent's layouts. Called when a property that
    //! is used to calculate default layout cell values has changed
    void InvalidateLayouts();

    //! Refresh the transform properties in the editor's properties pane
    void CheckLayoutFitterAndRefreshEditorTransformProperties() const;

    //! ChangeNotify callback for when the index string value selection changes.
    void OnIndexChange();

    //! Returns a string representation of the indices used to index sprite-sheet types.
    UiImageComponent::AZu32ComboBoxVec PopulateIndexStringList() const;

private: // static member functions

    static bool VersionConverter(AZ::SerializeContext& context,
        AZ::SerializeContext::DataElementNode& classElement);

private: // data

    AzFramework::SimpleAssetReference<LmbrCentral::TextureAsset> m_spritePathname;
    AZStd::string m_renderTargetName;
    SpriteType m_spriteType             = SpriteType::SpriteAsset;
    AZ::Color m_color                   = AZ::Color(1.0f, 1.0f, 1.0f, 1.0f);
    float m_alpha                       = 1.0f;
    ImageType m_imageType               = ImageType::Stretched;
    LyShine::BlendMode m_blendMode      = LyShine::BlendMode::Normal;

    ISprite* m_sprite                   = nullptr;

    ISprite* m_overrideSprite           = nullptr;
    AZ::u32 m_overrideSpriteCellIndex   = 0;
    AZ::Color m_overrideColor           = m_color;
    float m_overrideAlpha               = m_alpha;

    AZ::u32 m_spriteSheetCellIndex      = 0;    //!< Current index for sprite-sheet (if this sprite is a sprite-sheet type).

    FillType m_fillType                 = FillType::None;
    float m_fillAmount                  = 1.0f;
    float m_fillStartAngle              = 0.0f; // Start angle for fill measured in degrees clockwise.
    FillCornerOrigin m_fillCornerOrigin = FillCornerOrigin::TopLeft;
    FillEdgeOrigin m_fillEdgeOrigin     = FillEdgeOrigin::Left;
    bool m_fillClockwise                = true;
    bool m_fillCenter                   = true;
    bool m_isColorOverridden;
    bool m_isAlphaOverridden;
};
