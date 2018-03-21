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

#include <string>
#include <LyShine/IDraw2d.h>
#include <LyShine/UiBase.h>
#include <LyShine/UiComponentTypes.h>
#include <LyShine/Bus/UiVisualBus.h>
#include <LyShine/Bus/UiRenderBus.h>
#include <LyShine/Bus/UiTextBus.h>
#include <LyShine/Bus/UiLayoutCellDefaultBus.h>
#include <LyShine/Bus/UiUpdateBus.h>

#include <AzCore/Component/Component.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <LyShine/Bus/UiAnimateEntityBus.h>

#include <LyShine/UiAssetTypes.h>

#if defined(_DEBUG)
// Only needed for internal unit-testing
#include <LyShine.h>
#endif

#include <IFont.h>
#include <ILocalizationManager.h>

// Forward declaractions
namespace TextMarkup
{
    struct Tag;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
class UiTextComponent
    : public AZ::Component
    , public UiVisualBus::Handler
    , public UiRenderBus::Handler
    , public UiUpdateBus::Handler
    , public UiTextBus::Handler
    , public UiAnimateEntityBus::Handler
    , public UiTransformChangeNotificationBus::Handler
    , public UiLayoutCellDefaultBus::Handler
    , public LanguageChangeNotificationBus::Handler
{
public: //types

    using FontEffectComboBoxVec = AZStd::vector < AZStd::pair<unsigned int, AZStd::string> >;

    //! Atomic unit of font "state" for drawing text in the renderer.
    //! A single line of text can be divided amongst multiple draw batches,
    //! allowing that line of text to be rendered with different font
    //! stylings, which is used to support FontFamily rendering.
    struct DrawBatch
    {
        DrawBatch();

        AZ::Vector3 color;
        AZStd::string text;
        IFFont* font;
    };

    using DrawBatchContainer = AZStd::list < DrawBatch >;

    //! A single line of text that can be composed of multiple DrawBatch objects
    struct DrawBatchLine
    {
        DrawBatchLine();

        DrawBatchContainer drawBatchList;   //!< DrawBatches that the line is composed of
        AZ::Vector2 lineSize;                      //!< Pixel size of entire line of text
    };

    using DrawBatchLineContainer = AZStd::list < DrawBatchLine >;
    using FontFamilyRefSet = AZStd::set < FontFamilyPtr >;

    //! A collection of batch lines used for multi-line rendering of DrawBatch objects.
    //! A single line of text contains a list of batches, and multi-line rendering requires
    //! a list of multiple lines of draw batches.
    //!
    //! Since different Font Familys can be referenced batch-to-batch, we hold a strong
    //! reference (shared_ptr) for each Font Family that's referenced. Once this struct
    //! goes out of scope, or is cleared, the references are freed.
    struct DrawBatchLines
    {
        //! Clears the batch lines list and releases any Font Family references.
        void Clear();

        DrawBatchLineContainer batchLines;       //!< List of batch lines for drawing, each implicitly separted by a newline.
        FontFamilyRefSet fontFamilyRefs;    //!< Set of strongly referenced Font Family objects used by draw batches.
    };

    //! Simple container for left/right AZ::Vector2 offsets.
    struct LineOffsets
    {
        LineOffsets()
            : left(AZ::Vector2::CreateZero())
            , right(AZ::Vector2::CreateZero())
            , batchLineLength(0.0f) {}

        AZ::Vector2 left;
        AZ::Vector2 right;
        float batchLineLength;
    };

public: // member functions

    AZ_COMPONENT(UiTextComponent, LyShine::UiTextComponentUuid, AZ::Component);

    UiTextComponent();
    ~UiTextComponent() override;

    // UiVisualInterface
    void ResetOverrides() override;
    void SetOverrideColor(const AZ::Color& color) override;
    void SetOverrideAlpha(float alpha) override;
    void SetOverrideFont(FontFamilyPtr fontFamily) override;
    void SetOverrideFontEffect(unsigned int fontEffectIndex) override;
    // ~UiVisualInterface

    // UiRenderInterface
    void Render() override;
    // ~UiRenderInterface

    // UiUpdateInterface
    void Update(float deltaTime) override;
    void UpdateInEditor() override;
    // ~UiUpdateInterface

    // UiTextInterface
    AZStd::string GetText() override;
    void SetText(const AZStd::string& text) override;
    AZStd::string GetTextWithFlags(GetTextFlags flags = GetAsIs) override;
    void SetTextWithFlags(const AZStd::string& text, SetTextFlags flags = SetAsIs) override;
    AZ::Color GetColor() override;
    void SetColor(const AZ::Color& color) override;
    LyShine::PathnameType GetFont() override;
    void SetFont(const LyShine::PathnameType& fontPath) override;
    int GetFontEffect() override;
    void SetFontEffect(int effectIndex) override;
    float GetFontSize() override;
    void SetFontSize(float size) override;
    void GetTextAlignment(IDraw2d::HAlign& horizontalAlignment,
        IDraw2d::VAlign& verticalAlignment) override;
    void SetTextAlignment(IDraw2d::HAlign horizontalAlignment,
        IDraw2d::VAlign verticalAlignment) override;
    IDraw2d::HAlign GetHorizontalTextAlignment() override;
    void SetHorizontalTextAlignment(IDraw2d::HAlign alignment) override;
    IDraw2d::VAlign GetVerticalTextAlignment() override;
    void SetVerticalTextAlignment(IDraw2d::VAlign alignment) override;
    float GetCharacterSpacing() override;
    //! Expects 1/1000th ems, where 1 em = font size. This will also affect text size, which can lead to
    //! formatting changes (with word-wrap enabled for instance).
    void SetCharacterSpacing(float characterSpacing) override;
    float GetLineSpacing() override;
    //! Expects pixels.
    void SetLineSpacing(float lineSpacing) override;
    int GetCharIndexFromPoint(AZ::Vector2 point, bool mustBeInBoundingBox) override;
    int GetCharIndexFromCanvasSpacePoint(AZ::Vector2 point, bool mustBeInBoundingBox) override;
    AZ::Vector2 GetPointFromCharIndex(int index) override;
    AZ::Color GetSelectionColor() override;
    void GetSelectionRange(int& startIndex, int& endIndex) override;
    void SetSelectionRange(int startIndex, int endIndex, const AZ::Color& selectionColor) override;
    void ClearSelectionRange() override;
    AZ::Vector2 GetTextSize() override;
    void GetTextBoundingBox(int startIndex, int endIndex, UiTransformInterface::RectPointsArray& rectPoints) override;
    DisplayedTextFunction GetDisplayedTextFunction() const override { return m_displayedTextFunction; }
    void SetDisplayedTextFunction(const DisplayedTextFunction& displayedTextFunction) override;
    OverflowMode GetOverflowMode() override;
    void SetOverflowMode(OverflowMode overflowMode) override;
    WrapTextSetting GetWrapText() override;
    void SetWrapText(WrapTextSetting wrapSetting) override;
    void ResetCursorLineHint() override { m_cursorLineNumHint = -1; }
    // ~UiTextInterface

    // UiAnimateEntityInterface
    void PropertyValuesChanged() override;
    // ~UiAnimateEntityInterface

    // UiTransformChangeNotificationBus
    void OnCanvasSpaceRectChanged(AZ::EntityId entityId, const UiTransformInterface::Rect& oldRect, const UiTransformInterface::Rect& newRect) override;
    // ~UiTransformChangeNotificationBus

    // UiLayoutCellDefaultInterface
    float GetMinWidth() override;
    float GetMinHeight() override;
    float GetTargetWidth() override;
    float GetTargetHeight() override;
    float GetExtraWidthRatio() override;
    float GetExtraHeightRatio() override;
    // ~UiLayoutCellDefaultInterface

    // LanguageChangeNotification
    void LanguageChanged() override;
    // ~LanguageChangeNotification

#if defined(LYSHINE_INTERNAL_UNIT_TEST)
    static void UnitTest(CLyShine* lyshine);
    static void UnitTestLocalization(CLyShine* lyshine);
#endif

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

protected: // types

    //! Internal use only. Used for preparing text for rendering.
    enum PrepareTextOption
    {
        IgnoreLocalization  = 0,
        Localize            = 1 << 0,
        EscapeMarkup        = 1 << 1,
    };

protected: // member functions

    // AZ::Component
    void Init() override;
    void Activate() override;
    void Deactivate() override;
    // ~AZ::Component

    //! Called when we know the font needs to be changed
    void ChangeFont(const AZStd::string& fontFileName);

    //! Implementation of getting bounding box for the given displayed text
    void GetTextBoundingBoxPrivate(int startIndex, int endIndex, UiTransformInterface::RectPointsArray& rectPoints);

    //! Get the bounding rectangle of the text, in untransformed canvas space
    void GetTextRect(UiTransformInterface::RectPoints& rect);

    //! Similar to GetTextRect, but allows getting a rect for only a portion of text (via textSize).
    //!
    //! This method is particularly useful for multi-line text, where text selection can
    //! vary line-by-line, or across multiple lines of text, in which case you only want
    //! rects for a portion of the displayed text, rather than all of it (which GetTextRect
    //! does).
    void GetTextRect(UiTransformInterface::RectPoints& rect, const AZ::Vector2& textSize);

    //! ChangeNotify callback for text string change
    void OnTextChange();

    //! ChangeNotify callback for color change
    void OnColorChange();

    //! ChangeNotify callback for font size change
    void OnFontSizeChange();

    //! ChangeNotify callback for font pathname change
    AZ::u32 OnFontPathnameChange();

    //! ChangeNotify callback for font effect change
    void OnFontEffectChange();

    //! ChangeNotify callback for text wrap setting change
    void OnWrapTextSettingChange();

    //! ChangeNotify callback for char spacing change
    void OnCharSpacingChange();

    //! ChangeNotify callback for line spacing change
    void OnLineSpacingChange();

    //! Populate the list for the font effect combo box in the properties pane
    FontEffectComboBoxVec PopulateFontEffectList();

    //! Performs text wrapping
    //!
    //! Text wrap works by calling CFFont::WrapText with the element's
    //! current width. The WrapText function will insert appropriately
    //! positioned newline breaks to achieve the text wrapping behavior.
    void WrapText();

    //! Returns the amount of pixels the displayed text is adjusted for clipping.
    //!
    //! Returns zero if text is not large enough to be clipped or clipping
    //! isn't enabled.
    //!
    //! \note This does not simply return m_clipoffset. This method calculates
    //! and assigns new values to m_clipOffset and m_clipOffsetMultiplier and
    //! returns their product.
    float CalculateHorizontalClipOffset();

    //! Prepares text for display (word-wrap, localization, etc.).
    void PrepareDisplayedTextInternal(PrepareTextOption prepOption, bool suppressXmlWarnings = false, bool invalidateParentLayout = true);

    //! Calculates draw batch lines
    void CalculateDrawBatchLines(
        UiTextComponent::DrawBatchLines& drawBatchLinesOut,
        AZStd::string& locTextOut,
        PrepareTextOption prepOption,
        bool suppressXmlWarnings,
        bool useElementWidth,
        bool excludeTrailingSpace
        );

    //! Dispatches DrawBatch lines for rendering
    void RenderDrawBatchLines(const AZ::Vector2& pos, const UiTransformInterface::RectPoints& points, STextDrawContext& fontContext, float newlinePosYIncrement);

    //! Returns a prototypical STextDrawContext to be used when interacting with IFont routines..
    STextDrawContext GetTextDrawContextPrototype() const;

    //! Recomputes draw batch lines as appropriate depending on current options when text width properties are modified
    void OnTextWidthPropertyChanged();

private: // static member functions

    static bool VersionConverter(AZ::SerializeContext& context,
        AZ::SerializeContext::DataElementNode& classElement);

    AZ_DISABLE_COPY_MOVE(UiTextComponent);

    //! Calculates the left and right offsets for cursor placement and text selection bounds.
    void GetOffsetsFromSelectionInternal(LineOffsets& top, LineOffsets& middle, LineOffsets& bottom, int selectionStart, int selectionEnd);

private: // member functions

    //! Given an index into the displayed string, returns the line number that the character is displayed on.
    int GetLineNumberFromCharIndex(const int soughtIndex) const;

    //! Invalidates the parent and this element's layout
    void InvalidateLayout() const;

    //! Refresh the transform properties in the editor's properties pane
    void CheckLayoutFitterAndRefreshEditorTransformProperties() const;

    //! Sets "size ready" flag to true and disconnects from update bus.
    //! Update bus is only used to set the flag to true on first update (when
    //! its presumed that element size info is available by first update).
    void AssignSizeReadyFlag();

private: // data

    AZStd::string m_text;
    AZStd::string m_locText;                        //!< Language-specific localized text (if applicable), keyed by m_text. May contain word-wrap formatting (if enabled).

    DrawBatchLines m_drawBatchLines;                //!< Lists of DrawBatches across multiple lines for rendering text.

    AZ::Color m_color;
    float m_alpha;
    float m_fontSize;
    IDraw2d::HAlign m_textHAlignment;
    IDraw2d::VAlign m_textVAlignment;
    float m_charSpacing;
    float m_lineSpacing;

    float m_currFontSize;                           //!< Needed for PropertyValuesChanged method, used for UI animation
    float m_currCharSpacing;                        //!< Needed for PropertyValuesChanged method, used for UI animation

    AzFramework::SimpleAssetReference<LyShine::FontAsset> m_fontFilename;
    IFFont* m_font;
    FontFamilyPtr m_fontFamily;
    unsigned int m_fontEffectIndex;
    DisplayedTextFunction m_displayedTextFunction;  //!< Function object that returns a string to be used for rendering/display.

    AZ::Color m_overrideColor;
    float m_overrideAlpha;
    FontFamilyPtr m_overrideFontFamily;
    unsigned int m_overrideFontEffectIndex;

    bool m_isColorOverridden;
    bool m_isAlphaOverridden;
    bool m_isFontFamilyOverridden;
    bool m_isFontEffectOverridden;

    AZ::Color m_textSelectionColor;                 //!< color for a selection box drawn as background for a range of text

    int m_selectionStart;                           //!< UTF8 character/element index in the displayed string. This index
                                                    //! marks the beggining of a text selection, such as when this component
                                                    //! is associated with a text input component. If the displayed string
                                                    //! contains UTF8 multi-byte characters, then this index will not
                                                    //!< match 1:1 with an index into the raw string buffer.

    int m_selectionEnd;                             //!< UTF8 character/element index in the displayed string. This index
                                                    //! marks the end of a text selection, such as when this component
                                                    //! is associated with a text input component. If the displayed string
                                                    //! contains UTF8 multi-byte characters, then this index will not
                                                    //!< match 1:1 with an index into the raw string buffer.

    int m_cursorLineNumHint;
    OverflowMode m_overflowMode;                    //!< How text should "fit" within the element
    WrapTextSetting m_wrapTextSetting;              //!< Drives text-wrap setting
    float m_clipOffset;                             //!< Amount of pixels to adjust text draw call to account for clipping rect
    float m_clipOffsetMultiplier;                   //!< Used to adjust clip offset based on horizontal alignment settings

    bool m_elementSizeReady;                        //!< Provides similar functionality to InGamePostActivate, but works at run-time and in the editor.
                                                    //!< Text content can't be properly wrapped until the element's size is available. Also, scripts 
                                                    //!< can make calls on the text component before other components (that text component depends on)
                                                    //!< are ready, like canvas and transform components, which would result in warnings/errors.
};
