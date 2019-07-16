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
#include <QMetaType>

namespace GraphCanvas
{
    namespace Styling
    {
        namespace Elements
        {
            const char* const Graph = "graph";
            const char* const Node = "node";
            const char* const Slot = "slot";
            const char* const DataSlot = "dataSlot";
            const char* const ExecutionSlot = "executionSlot";
            const char* const PropertySlot = "propertySlot";
            const char* const Group = "group";
            const char* const Connection = "connection";
            const char* const Comment = "comment";
            const char* const BookmarkAnchor = "BookmarkAnchor";

            // General Node Styles
            const char* const GeneralSlotLayout = "generalSlotLayout";

            // Wrapper Node Styles
            namespace WrapperNode
            {
                const char* const Node = "wrapperNode";
                const char* const NodeLayout = "nodeLayout";

                const char* const ActionLabel = "actionLabel";
                const char* const ActionLabelEmptyNodes = "actionLabelEmpty";
            }

            namespace BlockComment
            {
                const char* const BlockComment = "blockComment";
                const char* const Title = "titleBox";
                const char* const BlockArea = "blockArea";
            }

            // Pseudo-elements
            const char* const Title = "title";
            const char* const MainTitle = ".maintitle";
            const char* const SubTitle = ".subtitle";
            
            const char* const ConnectionPin = "connectionPin";
            const char* const ExecutionConnectionPin = "executionConnectionPin";
            const char* const DataConnectionPin = "dataConnectionPin";

            const char* const CommentText = ".commentText";

            // Custom Widgets
            const char* const CheckBox = "checkBox";

        } // namespace Elements

        enum class Element : AZ::u32
        {
            Graph,
            Node,
            Slot,
            Group,
            Compound,
            Connection,
            Comment,

            Title,
            Decoration
        };

        namespace States
        {
            const char* const Hovered = ":hovered";
            const char* const Selected = ":selected";
            const char* const Disabled = ":disabled";
            const char* const Collapsed = ":collapsed";
            const char* const Highlighted = ":highlighted";
            const char* const ValidDrop = ":validDrop";
            const char* const InvalidDrop = ":invalidDrop";
            const char* const Dragging = ":dragging";
            const char* const Editing = ":editing";
            const char* const Deletion = ":deletion";
            const char* const Pressed = ":pressed";
            const char* const Preview = ":preview";
            const char* const InspectionTransparent = ":inspection_transparent";
        } // namespace States

        // Style of curve for connection lines
        enum class Curves : AZ::u32
        {
            Straight,
            Curved 
        };

        enum class PaletteStyle : AZ::u32
        {
            Solid,
            CandyStripe,
            PatternFill
        };

        namespace Attributes
        {

            const char* const Selectors = "selectors";

            const char* const BackgroundColor = "background-color";
            const char* const BackgroundImage = "background-image";

            const char* const GridMajorWidth = "grid-major-width";
            const char* const GridMajorStyle = "grid-major-style";
            const char* const GridMajorColor = "grid-major-color";

            const char* const GridMinorWidth = "grid-minor-width";
            const char* const GridMinorStyle = "grid-minor-style";
            const char* const GridMinorColor = "grid-minor-color";

            const char* const FontFamily = "font-family";
            const char* const FontSize = "font-size";
            const char* const FontWeight = "font-weight";
            const char* const FontStyle = "font-style";
            const char* const FontVariant = "font-variant";
            const char* const Color = "color";

            const char* const BorderWidth = "border-width";
            const char* const BorderStyle = "border-style";
            const char* const BorderColor = "border-color";
            const char* const BorderRadius = "border-radius";

            const char* const LineWidth = "line-width";
            const char* const LineStyle = "line-style";
            const char* const LineColor = "line-color";
            const char* const LineCurve = "line-curve";
            const char* const LineSelectionPadding = "line-selection-padding";

            const char* const CapStyle = "cap-style";

            const char* const Margin = "margin";
            const char* const Padding = "padding";

            const char* const Width = "width";
            const char* const Height = "height";

            const char* const MinWidth = "min-width";
            const char* const MaxWidth = "max-width";
            const char* const MinHeight = "min-height";
            const char* const MaxHeight = "max-height";
            const char* const TextAlignment = "text-alignment";
            const char* const TextVerticalAlignment = "text-vertical-alignment";

            const char* const Spacing = "spacing";

            // Custom Attributes
            const char* const ConnectionJut = "connection-jut";
            const char* const ConnectionDragMaximumDistance = "connection-drag-max-distance";
            const char* const ConnectionDragPercent = "connection-drag-percentage";
            const char* const ConnectionDragMoveBuffer = "connection-drag-move-buffer";
            const char* const ConnectionDefaultMarquee = "connection-default-marquee";

            const char* const PaletteStyle = "palette-style";

            // Candy Stripe Attribute
            const char* const MaximumStripeSize = "max-stripe-size";
            const char* const MinimumStripes = "min-stripes";
            const char* const StripeAngle = "stripe-angle";
            const char* const StripeColor = "stripe-color";
            const char* const StripeOffset = "stripe-offset";

            // Pattern Fill
            const char* const PatternTemplate = "pattern";
            const char* const PatternPalettes = "palettes";
            const char* const OddOffsetPercent = "odd-offset-percent";
            const char* const EvenOffsetPercent = "even-offset-percent";
            const char* const MinimumRepetitions = "min-repetitions";

            const char* const ZValue = "z-value";
            const char* const Opacity = "opacity";

        } // namespace Attributes

        enum class Attribute : AZ::u32
        {
            BackgroundColor = 0x80000001,
            BackgroundImage,

            GridMinorWidth,
            GridMinorStyle,
            GridMinorColor,

            GridMajorWidth,
            GridMajorStyle,
            GridMajorColor,

            FontFamily,
            FontSize,
            FontWeight,
            FontStyle,
            FontVariant,
            Color,

            BorderWidth,
            BorderStyle,
            BorderColor,
            BorderRadius,

            LineWidth,
            LineStyle,
            LineColor,
            LineCurve,
            LineSelectionPadding,

            CapStyle,

            Margin,
            Padding,

            Width,
            Height,

            MinWidth,
            MaxWidth,
            MinHeight,
            MaxHeight,

            Spacing,
            TextAlignment,
            TextVerticalAlignment,

            // Custom Attributes
            ConnectionJut,

            ConnectionDragMaximumDistance,
            ConnectionDragPercent,
            ConnectionDragMoveBuffer,
            ConnectionDefaultMarquee,

            PaletteStyle,

            // Candy Stripe Attributes
            MaximumStripeSize,
            MinimumStripes,
            StripeAngle,
            StripeColor,
            StripeOffset,

            // Pattern Fill
            PatternTemplate,
            PatternPalettes,
            OddOffsetPercent,
            EvenOffsetPercent,
            MinimumRepetitions,

            ZValue,

            Opacity,

            // The selectors are an array "attribute" in the same JSON scope as the rest
            Selectors = 0x80fffffe,

            Invalid = 0x80ffffff
        };

    } // namespace Styling
} // namespace GraphCanvas