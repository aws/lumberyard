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
#include "UiTextComponent.h"

#include <AzCore/Math/Crc.h>
#include <AzCore/Math/MathUtils.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/std/string/conversions.h>
#include <AzCore/std/string/regex.h>
#include <AzFramework/API/ApplicationAPI.h>

#include <LyShine/Bus/UiTransform2dBus.h>
#include <LyShine/Bus/UiElementBus.h>
#include <LyShine/Bus/UiLayoutManagerBus.h>
#include <LyShine/UiSerializeHelpers.h>
#include <LyShine/IUiRenderer.h>

#include <ILocalizationManager.h>

#include "UiSerialize.h"
#include "Draw2d.h"
#include "TextMarkup.h"
#include "UiTextComponentOffsetsSelector.h"
#include "StringUtfUtils.h"
#include "UiLayoutHelpers.h"

namespace
{
    AZStd::string DefaultDisplayedTextFunction(const AZStd::string& originalText)
    {
        // By default, the text component renders the string contents as-is
        return originalText;
    }

    //! Replaces occurrences of multiple strings
    //! Example:
    //! vector<string> find = { "a", "b", "c", "d" };
    //! vector<string> replace = { "e", "f", "g", "h" };
    //! ReplaceStringPairs("abcd dcba", find, replace); // result: "efgh hgfe"
    AZStd::string ReplaceStringPairs(
        const AZStd::string& source,
        const AZStd::vector<AZStd::string>& find,
        const AZStd::vector<AZStd::string>& replace)
    {
        const int numElements = find.size();
        AZStd::string returnText(source);
        AZStd::string::size_type startIndex = 0;
        AZStd::string::size_type foundCharIt;

        do
        {
            foundCharIt = AZStd::string::npos;
            int foundElementIndex = 0;
            for (int i = 0; i < numElements; ++i)
            {
                const AZStd::string::size_type foundPos = returnText.find(find[i], startIndex);

                // Look for earliest occurrence of any of the "find" strings
                if (foundPos < foundCharIt)
                {
                    foundCharIt = foundPos;
                    foundElementIndex = i;
                }
            }

            if (foundCharIt == AZStd::string::npos)
            {
                break;
            }

            // Replace the "find" occurence with its "replace" counterpart
            returnText.replace(foundCharIt, find[foundElementIndex].length(), replace[foundElementIndex]);

            // Start search again after this replaced occurence
            startIndex = foundCharIt + replace[foundElementIndex].length();
        } while (foundCharIt != AZStd::string::npos);

        return returnText;
    }

    const AZStd::vector<AZStd::string>& GetEscapedMarkupStrings()
    {
        static const AZStd::vector<AZStd::string> escapedMarkup = { "&amp;", "&lt;", "&gt;", "&#37;" };
        return escapedMarkup;
    }

    const AZStd::vector<AZStd::string>& GetUnescapedMarkupChars()
    {
        static const AZStd::vector<AZStd::string> unescapedChars = { "&", "<", ">", "%" };
        return unescapedChars;
    }

    AZStd::string EscapeMarkup(const AZStd::string& markupString)
    {
        return ReplaceStringPairs(markupString, GetUnescapedMarkupChars(), GetEscapedMarkupStrings());
    }

    AZStd::string UnescapeMarkup(const AZStd::string& markupString)
    {
        return ReplaceStringPairs(markupString, GetEscapedMarkupStrings(), GetUnescapedMarkupChars());
    }

    bool RemoveV4MarkupFlag(
        AZ::SerializeContext& context,
        AZ::SerializeContext::DataElementNode& classElement)
    {
        int index = classElement.FindElement(AZ_CRC("SupportMarkup", 0x5e81a9c7));
        if (index != -1)
        {
            classElement.RemoveElement(index);
        }

        return true;
    }

    bool ConvertV3FontFileNameIfDefault(
        AZ::SerializeContext& context,
        AZ::SerializeContext::DataElementNode& classElement)
    {
        int index = classElement.FindElement(AZ_CRC("FontFileName", 0x44defd6f));
        if (index != -1)
        {
            AZ::SerializeContext::DataElementNode& fontFileNameNode = classElement.GetSubElement(index);
            index = fontFileNameNode.FindElement(AZ_CRC("BaseClass1", 0xd4925735));

            if (index != -1)
            {
                AZ::SerializeContext::DataElementNode& baseClassNode = fontFileNameNode.GetSubElement(index);
                index = baseClassNode.FindElement(AZ_CRC("AssetPath", 0x2c355179));

                if (index != -1)
                {
                    AZ::SerializeContext::DataElementNode& assetPathNode = baseClassNode.GetSubElement(index);
                    AZStd::string oldData;

                    if (!assetPathNode.GetData(oldData))
                    {
                        AZ_Error("Serialization", false, "Element AssetPath is not a AZStd::string.");
                        return false;
                    }

                    if (oldData == "default")
                    {
                        if (!assetPathNode.SetData(context, AZStd::string("default-ui")))
                        {
                            AZ_Error("Serialization", false, "Unable to set AssetPath data.");
                            return false;
                        }

                        // The effect indicies have flip-flopped between the "default" and "default-ui"
                        // fonts. Handle the conversion here.
                        index = classElement.FindElement(AZ_CRC("EffectIndex", 0x4d3320e3));
                        if (index != -1)
                        {
                            AZ::SerializeContext::DataElementNode& effectIndexNode = classElement.GetSubElement(index);
                            uint32 effectIndex = 0;

                            if (!effectIndexNode.GetData(effectIndex))
                            {
                                AZ_Error("Serialization", false, "Element EffectIndex is not an int.");
                                return false;
                            }

                            uint32 newEffectIndex = effectIndex;

                            // Only handle converting indices 1 and 0 in the rare (?) case that the user added
                            // their own effects to the default font.
                            if (newEffectIndex == 1)
                            {
                                newEffectIndex = 0;
                            }
                            else if (newEffectIndex == 0)
                            {
                                newEffectIndex = 1;
                            }

                            if (!effectIndexNode.SetData(context, newEffectIndex))
                            {
                                AZ_Error("Serialization", false, "Unable to set EffectIndex data.");
                                return false;
                            }
                        }
                    }
                }
            }
        }

        return true;
    }

    void SanitizeUserEnteredNewlineChar(AZStd::string& stringToSanitize)
    {
        // Convert user-entered newline delimiters to proper ones before wrapping
        // the text so they can be correctly accounted for.
        static const AZStd::string NewlineDelimiter("\n");
        static const AZStd::regex UserInputNewlineDelimiter("\\\\n");
        stringToSanitize = AZStd::regex_replace(stringToSanitize, UserInputNewlineDelimiter, NewlineDelimiter);
    }
    //! Builds a list of DrawBatch objects from a XML tag tree.
    //!
    //! A DrawBatch is essentially render "state" for text. This method tries
    //! to determine what the current state is that should be applied based
    //! on the tag tree traversal. Once all of a tag's children are
    //! traversed, and a new DrawBatch was created, the batch is popped off
    //! the batch stack and moved into the DrawBatch output list.
    //!
    //! Example usage:
    //!
    //! TextMarkup::Tag markupRoot;
    //! if (TextMarkup::ParseMarkupBuffer(markupText, markupRoot))
    //! {
    //!   AZStd::stack<UiTextComponent::DrawBatch> batchStack;
    //!   AZStd::stack<FontFamily*> fontFamilyStack;
    //!   fontFamilyStack.push(m_fontFamily.get());
    //!   BuildDrawBatches(drawBatches, batchStack, fontFamilyStack, &markupRoot);
    //! }
    //!
    //! \param output List of DrawBatch objects built based on tag tree contents
    //! \param fontFamilyRefs List of Font Family's that output (strongly) references.
    //! \param batchStack The DrawBatch on "top" of the stack is the state that is currently active.
    //! \param fontFamilyStack The FontFamily on top of the stack is the font family that's currently active.
    //! The font family can change when the font tag is encountered.
    //! \param currentTag Current tag being visited in the parsed tag tree.
    void BuildDrawBatches(
        UiTextComponent::DrawBatchContainer& output,
        UiTextComponent::FontFamilyRefSet& fontFamilyRefs,
        AZStd::stack<UiTextComponent::DrawBatch>& batchStack,
        AZStd::stack<FontFamily*>& fontFamilyStack,
        const TextMarkup::Tag* currentTag)
    {
        TextMarkup::TagType type = currentTag->GetType();

        const bool isRoot = type == TextMarkup::TagType::Root;

        bool newBatchStackPushed = false;

        // Root tag doesn't push any new state
        if (!isRoot)
        {
            if (batchStack.empty())
            {
                batchStack.push(UiTextComponent::DrawBatch());
                newBatchStackPushed = true;

                // For new batches, use the Font Family's "normal" font by default
                batchStack.top().font = fontFamilyStack.top()->normal;
            }

            // Prevent creating a new DrawBatch if the "current" batch has
            // no text associated with it yet.
            else if (!batchStack.top().text.empty())
            {
                // Create a copy of the top
                batchStack.push(batchStack.top());
                newBatchStackPushed = true;

                // We assume that a DrawBatch will eventually get its own
                // text assigned, but in case no character was specified
                // in the markup, we explicitly clear the text here to avoid
                // showing duplicate text.
                batchStack.top().text.clear();
            }
        }

        // We need the previous batch for all cases except the root case
        // (where there is no previous batch). To streamline handling this
        // case, we just create an unused default-constructed DrawBatch
        // for the root case.
        const UiTextComponent::DrawBatch& prevBatch = batchStack.empty() ? UiTextComponent::DrawBatch() : batchStack.top();

        bool newFontFamilyPushed = false;
        switch (type)
        {
        case TextMarkup::TagType::Text:
        {
            batchStack.top().text = (static_cast<const TextMarkup::TextTag*>(currentTag))->text;

            // Replace escaped newlines with actual newlines
            batchStack.top().text = AZStd::regex_replace(batchStack.top().text, AZStd::regex("\\\\n"), "\n");

            break;
        }
        case TextMarkup::TagType::Bold:
        {
            if (prevBatch.font == fontFamilyStack.top()->bold)
            {
                // adjacent bold tags, no need to push a new batch
                break;
            }
            else if (prevBatch.font == fontFamilyStack.top()->italic)
            {
                // We're on a bold tag, but current font applied is
                // italic, so we apply the bold-italic font.
                batchStack.top().font = fontFamilyStack.top()->boldItalic;
            }
            else
            {
                batchStack.top().font = fontFamilyStack.top()->bold;
            }
            break;
        }
        case TextMarkup::TagType::Italic:
        {
            if (prevBatch.font == fontFamilyStack.top()->italic)
            {
                // adjacent italic tags, no need to push a new batch
                break;
            }
            else if (prevBatch.font == fontFamilyStack.top()->bold)
            {
                // We're on an italic tag, but current font applied is
                // bold, so we apply the bold-italic font.
                batchStack.top().font = fontFamilyStack.top()->boldItalic;
            }
            else
            {
                batchStack.top().font = fontFamilyStack.top()->italic;
            }
            break;
        }
        case TextMarkup::TagType::Font:
        {
            const TextMarkup::FontTag* pFontTag = static_cast<const TextMarkup::FontTag*>(currentTag);
            if (!(pFontTag->face.empty()))
            {
                FontFamilyPtr pFontFamily = gEnv->pCryFont->GetFontFamily(pFontTag->face.c_str());

                if (!pFontFamily)
                {
                    pFontFamily = gEnv->pCryFont->LoadFontFamily(pFontTag->face.c_str());
                }

                // Still need to check for pFontFamily validity since
                // Font Family load could have failed.
                if (pFontFamily)
                {
                    // Important to strongly reference the Font Family
                    // here otherwise it will de-ref once we go out of
                    // scope (and possibly unload).
                    fontFamilyRefs.insert(pFontFamily);

                    if (fontFamilyStack.top() != pFontFamily.get())
                    {
                        fontFamilyStack.push(pFontFamily.get());
                        newFontFamilyPushed = true;

                        // Reset font to default face for new font family
                        batchStack.top().font = pFontFamily->normal;
                    }
                }
            }
            const bool newColorNeeded = pFontTag->color != prevBatch.color;
            const bool tagHasValidColor = pFontTag->color != TextMarkup::ColorInvalid;
            if (newColorNeeded && tagHasValidColor)
            {
                batchStack.top().color = pFontTag->color;
            }
            break;
        }
        default:
        {
            break;
        }
        }

        // We only want to push a DrawBatch when it has text to display. We
        // store character data in separate tags. So when a bold tag is
        // traversed, we haven't yet visited its child character data:
        // <b> <!-- Bold tag DrawBatch created, no text yet -->
        //   <ch>Child character data here</ch>
        // </b>
        if (!batchStack.empty() && !batchStack.top().text.empty())
        {
            output.push_back(batchStack.top());
        }

        // Depth-first traversal of children tags.
        auto iter = currentTag->children.begin();
        for (; iter != currentTag->children.end(); ++iter)
        {
            BuildDrawBatches(output, fontFamilyRefs, batchStack, fontFamilyStack, *iter);
        }

        // Children visited, clear newly created DrawBatch state.
        if (newBatchStackPushed)
        {
            batchStack.pop();
        }

        // Clear FontFamily state also.
        if (newFontFamilyPushed)
        {
            fontFamilyStack.pop();
        }
    }

    //! Use the given width and font context to insert newline breaks in the given DrawBatchList.
    //! This code is largely adapted from FFont::WrapText to work with DrawBatch objects.
    void InsertNewlinesToWrapText(
        UiTextComponent::DrawBatchContainer& drawBatches,
        const STextDrawContext& ctx,
        float elementWidth)
    {
        if (drawBatches.empty())
        {
            return;
        }

        // Keep track of the last space char we encountered as ideal
        // locations for inserting newlines for word-wrapping. We also need
        // to track which DrawBatch contained the last-encountered space.
        const char* pLastSpace = NULL;
        UiTextComponent::DrawBatch* lastSpaceBatch = nullptr;
        int lastSpace = -1;
        int lastSpaceIndexInBatch = -1;
        float lastSpaceWidth = 0.0f;

        int curChar = 0;
        float curLineWidth = 0.0f;
        float biggestLineWidth = 0.0f;
        float widthSum = 0.0f;

        // When iterating over batches, we need to know the previous
        // character, which we can only obtain if we keep track of the last
        // batch we iterated on.
        UiTextComponent::DrawBatch* prevBatch = &drawBatches.front();

        // Map draw batches to text indices where spaces should be restored
        // (more details below); this happens after we've inserted newlines
        // for word-wrapping.
        using SpaceIndices = AZStd::vector < int >;
        AZStd::unordered_map<UiTextComponent::DrawBatch*, SpaceIndices> batchSpaceIndices;

        // Iterate over all drawbatches, calculating line length and add newlines
        // when element width is exceeded. Reset line length when a newline is added
        // or a newline is encountered.
        for (UiTextComponent::DrawBatch& drawBatch : drawBatches)
        {
            // If this entry ultimately ends up not having any space char
            // indices associated with it, we will simply skip iterating over
            // it later.
            batchSpaceIndices.insert(&drawBatch);

            int batchCurChar = 0;

            Unicode::CIterator<const char*, false> pChar(drawBatch.text.c_str());
            uint32_t prevCh = 0;
            while (uint32_t ch = *pChar)
            {
                char codepoint[5];
                Unicode::Convert(codepoint, ch);

                float curCharWidth = drawBatch.font->GetTextSize(codepoint, true, ctx).x;

                if (prevCh && ctx.m_kerningEnabled)
                {
                    curCharWidth += drawBatch.font->GetKerning(prevCh, ch, ctx).x;
                }

                if (prevCh)
                {
                    curCharWidth += ctx.m_tracking;
                }

                prevCh = ch;

                // keep track of spaces
                // they are good for splitting the string
                if (ch == ' ')
                {
                    lastSpace = curChar;
                    lastSpaceIndexInBatch = batchCurChar;
                    lastSpaceBatch = &drawBatch;
                    lastSpaceWidth = curLineWidth + curCharWidth;
                    pLastSpace = pChar.GetPosition();
                    assert(*pLastSpace == ' ');
                }

                bool prevCharWasNewline = false;
                const bool isFirstChar = pChar.GetPosition() == drawBatch.text.c_str();
                if (ch && !isFirstChar)
                {
                    const char* pPrevCharStr = pChar.GetPosition() - 1;
                    prevCharWasNewline = pPrevCharStr[0] == '\n';
                }
                else if (isFirstChar)
                {
                    // Since prevBatch is initialized to front of drawBatches,
                    // check to ensure there was a previous batch.
                    const bool prevBatchValid = prevBatch != &drawBatch;

                    if (prevBatchValid && !prevBatch->text.empty())
                    {
                        prevCharWasNewline = prevBatch->text.at(prevBatch->text.length() - 1) == '\n';
                    }
                }

                // if line exceed allowed width, split it
                if (prevCharWasNewline || (curLineWidth + curCharWidth > elementWidth && ch))
                {
                    if (prevCharWasNewline)
                    {
                        // Reset the current line width to account for newline
                        curLineWidth = curCharWidth;
                        widthSum += curLineWidth;
                    }
                    else if ((lastSpace > 0) && ((curChar - lastSpace) < 16) && (curChar - lastSpace >= 0)) // 16 is the default threshold
                    {
                        *(char*)pLastSpace = '\n';  // This is safe inside UTF-8 because space is single-byte codepoint
                        batchSpaceIndices.at(lastSpaceBatch).push_back(lastSpaceIndexInBatch);

                        if (lastSpaceWidth > biggestLineWidth)
                        {
                            biggestLineWidth = lastSpaceWidth;
                        }

                        curLineWidth = curLineWidth - lastSpaceWidth + curCharWidth;
                        widthSum += curLineWidth;
                    }
                    else
                    {
                        const char* pBuf = pChar.GetPosition();
                        AZStd::string::size_type bytesProcessed = pBuf - drawBatch.text.c_str();
                        drawBatch.text.insert(bytesProcessed, "\n"); // Insert the newline, this invalidates the iterator
                        pBuf = drawBatch.text.c_str() + bytesProcessed; // In case reallocation occurs, we ensure we are inside the new buffer
                        assert(*pBuf == '\n');
                        pChar.SetPosition(pBuf); // pChar once again points inside the target string, at the current character
                        assert(*pChar == ch);
                        ++pChar;
                        ++curChar;
                        ++batchCurChar;

                        if (curLineWidth > biggestLineWidth)
                        {
                            biggestLineWidth = curLineWidth;
                        }

                        widthSum += curLineWidth;
                        curLineWidth = curCharWidth;
                    }

                    lastSpaceWidth = 0;
                    lastSpace = 0;
                }
                else
                {
                    curLineWidth += curCharWidth;
                }

                curChar += LyShine::GetMultiByteCharSize(ch);
                batchCurChar += LyShine::GetMultiByteCharSize(ch);
                ++pChar;
            }

            prevBatch = &drawBatch;
        }

        // We insert newline breaks (perform word-wrapping) in-place for
        // formatting purposes, but we restore the original delimiting
        // space characters now. This resolves a lot of issues with indices
        // mismatching between the rendered string content and the original
        // string.
        //
        // This seems unintuitive since (above) we simply (in some cases)
        // replace the space character with newline, so inserting an additional
        // space now would mismatch the original string contents even further.
        // However, since draw batch "lines" are delimited by newline, the
        // newline character will eventually be removed (because it will be
        // implied). So at this part in the pipeline, the strings will not
        // match in content or length, but eventually will.
        for (auto& batchSpaceList : batchSpaceIndices)
        {
            UiTextComponent::DrawBatch* drawBatch = batchSpaceList.first;
            const SpaceIndices& spaceIndices = batchSpaceList.second;

            int insertOffset = 0;
            for (const int spaceIndex : spaceIndices)
            {
                drawBatch->text.insert(spaceIndex + insertOffset, 1, ' ');

                // Each time we insert, our indices our off by one.
                ++insertOffset;
            }
        }
    }

    //! Given a "flat" list of DrawBatches, separate them by newline and place in output.
    void CreateBatchLines(
        UiTextComponent::DrawBatchLines& output,
        UiTextComponent::DrawBatchContainer& drawBatches,
        FontFamily* defaultFontFamily)
    {
        UiTextComponent::DrawBatchLineContainer& lineList = output.batchLines;
        lineList.push_back(UiTextComponent::DrawBatchLine());

        for (UiTextComponent::DrawBatch& drawBatch : drawBatches)
        {
            AZStd::string::size_type newlineIndex = drawBatch.text.find('\n');

            if (newlineIndex == AZStd::string::npos)
            {
                lineList.back().drawBatchList.push_back(drawBatch);
                continue;
            }
            while (newlineIndex != AZStd::string::npos)
            {
                // Found a newline within a single drawbatch, so split
                // into two batches, one for the current line, and one
                // for the following.
                UiTextComponent::DrawBatchContainer& currentLine = lineList.back().drawBatchList;
                lineList.push_back(UiTextComponent::DrawBatchLine());
                UiTextComponent::DrawBatchContainer& newLine = lineList.back().drawBatchList;

                const bool moreCharactersAfterNewline = drawBatch.text.length() - 1 > newlineIndex;

                // Note that we purposely build the string such that the newline
                // character is truncated from the string. If it were included,
                // it would be doubly-accounted for in the renderer.
                UiTextComponent::DrawBatch splitBatch(drawBatch);
                splitBatch.text = drawBatch.text.substr(0, newlineIndex);
                currentLine.push_back(splitBatch);

                // Start a new newline
                if (moreCharactersAfterNewline)
                {
                    const AZStd::string::size_type endOfStringIndex = drawBatch.text.length() - newlineIndex - 1;
                    drawBatch.text = drawBatch.text.substr(newlineIndex + 1, endOfStringIndex);
                    newlineIndex = drawBatch.text.find('\n');

                    if (newlineIndex == AZStd::string::npos)
                    {
                        newLine.push_back(drawBatch);
                    }
                }
                else
                {
                    break;
                }
            }
        }

        // Push an empty DrawBatch if the string happened to end with a
        // newline but no following text (e.g. "Hello\n").
        // :TODO: is this still needed? Can the final DrawBatchLine be removed
        // altogether if it has no content?
        if (lineList.back().drawBatchList.empty())
        {
            lineList.back().drawBatchList.push_back(UiTextComponent::DrawBatch());
            lineList.back().drawBatchList.front().font = defaultFontFamily->normal;
        }
    }

    void AssignLineSizes(
        UiTextComponent::DrawBatchLines& output,
        FontFamily* fontFamily,
        const STextDrawContext& ctx,
        UiTextInterface::DisplayedTextFunction displayedTextFunction,
        bool excludeTrailingSpace = true)
    {
        for (UiTextComponent::DrawBatchLine& drawBatchLine : output.batchLines)
        {
            float width = 0.0f;
            float height = 0.0f;
            for (const UiTextComponent::DrawBatch& drawBatch : drawBatchLine.drawBatchList)
            {
                AZStd::string displayString(displayedTextFunction(drawBatch.text));

                // For now, we only use batch text size for rendering purposes,
                // so we don't account for trailing spaces to avoid alignment
                // and formatting issues. In the future, we may need to
                // calculate batch size by use case (rendering, "true" size,
                // etc.). rather than assume one-size-fits-all.

                // Trim right
                if (excludeTrailingSpace)
                {
                    if (displayString.length() > 0)
                    {
                        AZStd::string::size_type endpos = displayString.find_last_not_of(" \t\n\v\f\r");
                        if ((endpos != string::npos) && (endpos != displayString.length() - 1))
                        {
                            displayString.erase(endpos + 1);
                        }
                    }
                }

                Vec2 batchTextSize(drawBatch.font->GetTextSize(displayString.c_str(), true, ctx));
                width += batchTextSize.x;
                height = AZ::GetMax<float>(height, batchTextSize.y);
            }

            drawBatchLine.lineSize.SetX(width);
            drawBatchLine.lineSize.SetY(height);
        }
    }

    //! Takes a flat list of draw batches (created by the Draw Batch Builder) and groups them
    //! by line, taking the element width into account, and also taking any newline characters
    //! that may already exist within the character data of the DrawBatch objects
    void BatchAwareWrapText(
        UiTextComponent::DrawBatchLines& output,
        UiTextComponent::DrawBatchContainer& drawBatches,
        FontFamily* fontFamily,
        const STextDrawContext& ctx,
        UiTextInterface::DisplayedTextFunction displayedTextFunction,
        float elementWidth)
    {
        InsertNewlinesToWrapText(drawBatches, ctx, elementWidth);
        CreateBatchLines(output, drawBatches, fontFamily);
        AssignLineSizes(output, fontFamily, ctx, displayedTextFunction);
    }
}   // anonymous namespace

////////////////////////////////////////////////////////////////////////////////////////////////////
// PUBLIC MEMBER FUNCTIONS
////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////
UiTextComponent::DrawBatch::DrawBatch()
    : color(TextMarkup::ColorInvalid)
    , font(nullptr)
{
}

////////////////////////////////////////////////////////////////////////////////////////////////////
UiTextComponent::DrawBatchLine::DrawBatchLine()
    : lineSize(0.0f, 0.0f)
{
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiTextComponent::DrawBatchLines::Clear()
{
    batchLines.clear();
    fontFamilyRefs.clear();
}

////////////////////////////////////////////////////////////////////////////////////////////////////
UiTextComponent::UiTextComponent()
    : m_text("My string")
    , m_color(1.0f, 1.0f, 1.0f, 1.0f)
    , m_alpha(1.0f)
    , m_fontSize(32)
    , m_textHAlignment(IDraw2d::HAlign::Center)
    , m_textVAlignment(IDraw2d::VAlign::Center)
    , m_charSpacing(0.0f)
    , m_lineSpacing(0.0f)
    , m_currFontSize(m_fontSize)
    , m_currCharSpacing(m_charSpacing)
    , m_font(nullptr)
    , m_fontFamily(nullptr)
    , m_fontEffectIndex(0)
    , m_displayedTextFunction(DefaultDisplayedTextFunction)
    , m_overrideColor(m_color)
    , m_overrideAlpha(m_alpha)
    , m_overrideFontFamily(nullptr)
    , m_overrideFontEffectIndex(m_fontEffectIndex)
    , m_isColorOverridden(false)
    , m_isAlphaOverridden(false)
    , m_isFontFamilyOverridden(false)
    , m_isFontEffectOverridden(false)
    , m_textSelectionColor(0.0f, 0.0f, 0.0f, 1.0f)
    , m_selectionStart(-1)
    , m_selectionEnd(-1)
    , m_cursorLineNumHint(-1)
    , m_overflowMode(OverflowMode::OverflowText)
    , m_wrapTextSetting(WrapTextSetting::NoWrap)
    , m_clipOffset(0.0f)
    , m_clipOffsetMultiplier(1.0f)
    , m_elementSizeReady(false)
{
    static const AZStd::string DefaultUi("default-ui");
    m_fontFilename.SetAssetPath(DefaultUi.c_str());

    if (gEnv && gEnv->pCryFont) // these will be null in RC.exe
    {
        m_fontFamily = gEnv->pCryFont->GetFontFamily(DefaultUi.c_str());

        if (!m_fontFamily)
        {
            m_fontFamily = gEnv->pCryFont->LoadFontFamily(DefaultUi.c_str());
        }
    }

    if (m_fontFamily)
    {
        m_font = m_fontFamily->normal;
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
UiTextComponent::~UiTextComponent()
{
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiTextComponent::ResetOverrides()
{
    m_overrideColor = m_color;
    m_overrideAlpha = m_alpha;
    m_overrideFontFamily = m_fontFamily;
    m_overrideFontEffectIndex = m_fontEffectIndex;

    m_isColorOverridden = false;
    m_isAlphaOverridden = false;
    m_isFontFamilyOverridden = false;
    m_isFontEffectOverridden = false;

    PrepareDisplayedTextInternal(PrepareTextOption::Localize, true);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiTextComponent::SetOverrideColor(const AZ::Color& color)
{
    m_overrideColor.Set(color.GetAsVector3());

    m_isColorOverridden = true;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiTextComponent::SetOverrideAlpha(float alpha)
{
    m_overrideAlpha = alpha;

    m_isAlphaOverridden = true;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiTextComponent::SetOverrideFont(FontFamilyPtr fontFamily)
{
    m_overrideFontFamily = fontFamily;

    m_isFontFamilyOverridden = true;

    PrepareDisplayedTextInternal(PrepareTextOption::Localize, true);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiTextComponent::SetOverrideFontEffect(unsigned int fontEffectIndex)
{
    m_overrideFontEffectIndex = fontEffectIndex;

    m_isFontEffectOverridden = true;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiTextComponent::Render()
{
    if (!m_overrideFontFamily)
    {
        return;
    }

    // first draw the text selection background box if any
    if (m_selectionStart != -1)
    {
        UiTransformInterface::RectPointsArray rectPoints;
        GetTextBoundingBoxPrivate(m_selectionStart, m_selectionEnd, rectPoints);

        // points are a clockwise quad
        static const AZ::Vector2 uvs[4] = {
            AZ::Vector2(0, 0), AZ::Vector2(1, 0), AZ::Vector2(1, 1), AZ::Vector2(0, 1)
        };

        for (UiTransformInterface::RectPoints& rect : rectPoints)
        {
            IDraw2d::VertexPosColUV verts[4];
            for (int i = 0; i < 4; ++i)
            {
                verts[i].position = rect.pt[i];
                verts[i].color = m_textSelectionColor;
                verts[i].uv = uvs[i];
            }


            // The font rendering rounds down. But for the selection box, no rounding looks better.
            // For the text cursor the positions have already been rounded.
            const IDraw2d::ImageOptions& imageOptions = Draw2dHelper::GetDraw2d()->GetDefaultImageOptions();
            IRenderer* renderer = gEnv->pRenderer;
            Draw2dHelper::GetDraw2d()->DrawQuad(renderer->GetWhiteTextureId(), verts, imageOptions.blendMode, IDraw2d::Rounding::None, IUiRenderer::Get()->GetBaseState());
        }
    }

    // get fade value (tracked by UiRenderer)
    float fade = IUiRenderer::Get()->GetAlphaFade();

    // Get the rect that positions the text prior to scale and rotate. The scale and rotate transform
    // will be applied inside the font draw.
    UiTransformInterface::RectPoints points;
    EBUS_EVENT_ID(GetEntityId(), UiTransformBus, GetCanvasSpacePointsNoScaleRotate, points);

    AZ::Vector2 pos;

    switch (m_textHAlignment)
    {
    case IDraw2d::HAlign::Left:
        pos.SetX(points.TopLeft().GetX());
        break;
    case IDraw2d::HAlign::Center:
    {
        float width = points.TopRight().GetX() - points.TopLeft().GetX();
        pos.SetX(points.TopLeft().GetX() + width * 0.5f);
        break;
    }
    case IDraw2d::HAlign::Right:
        pos.SetX(points.TopRight().GetX());
        break;
    }

    switch (m_textVAlignment)
    {
    case IDraw2d::VAlign::Top:
        pos.SetY(points.TopLeft().GetY());
        break;
    case IDraw2d::VAlign::Center:
    {
        float height = points.BottomLeft().GetY() - points.TopLeft().GetY();
        pos.SetY(points.TopLeft().GetY() + height * 0.5f);
        break;
    }
    case IDraw2d::VAlign::Bottom:
        pos.SetY(points.BottomLeft().GetY());
        break;
    }

    STextDrawContext fontContext(GetTextDrawContextPrototype());
    fontContext.SetOverrideViewProjMatrices(false);

    ColorF color = LyShine::MakeColorF(m_overrideColor.GetAsVector3(), m_overrideAlpha);
    color.srgb2rgb();   // the colors are specified in sRGB but we want linear colors in the shader
    color.a *= fade;
    fontContext.SetColor(color);

    // FFont.cpp uses the alpha value of the color to decide whether to use the color, if the alpha value is zero
    // (in a ColorB format) then the color set via SetColor is ignored and it usually ends up drawing with an alpha of 1.
    // This is not what we want so in this case do not draw at all.
    if (!fontContext.IsColorOverridden())
    {
        return;
    }

    // Tell the font system how to we are aligning the text
    // The font system uses these alignment flags to force text to be in the safe zone
    // depending on overscan etc
    int flags = 0;
    if (m_textHAlignment == IDraw2d::HAlign::Center)
    {
        flags |= eDrawText_Center;
    }
    else if (m_textHAlignment == IDraw2d::HAlign::Right)
    {
        flags |= eDrawText_Right;
    }

    if (m_textVAlignment == IDraw2d::VAlign::Center)
    {
        flags |= eDrawText_CenterV;
    }
    else if (m_textVAlignment == IDraw2d::VAlign::Bottom)
    {
        flags |= eDrawText_Bottom;
    }

    flags |= eDrawText_UseTransform;
    fontContext.SetFlags(flags);

    AZ::Matrix4x4 transform;
    EBUS_EVENT_ID(GetEntityId(), UiTransformBus, GetTransformToViewport, transform);

    float transFloats[16];
    transform.StoreToRowMajorFloat16(transFloats);
    Matrix34 transform34(transFloats[0], transFloats[1], transFloats[2], transFloats[3],
        transFloats[4], transFloats[5], transFloats[6], transFloats[7],
        transFloats[8], transFloats[9], transFloats[10], transFloats[11]);
    fontContext.SetTransform(transform34);

    fontContext.SetBaseState(IUiRenderer::Get()->GetBaseState());

    // For each newline-delimited string, we increment the draw call Y pos
    // by the font size
    float newlinePosYIncrement = 0.0f;

    size_t numLinesOfText = m_drawBatchLines.batchLines.size();

    // For bottom-aligned text, we need to offset the verticle draw position
    // such that the text never displays below the max Y position
    if (m_textVAlignment == IDraw2d::VAlign::Bottom)
    {
        // Subtract one from the number of newline-delimited strings since,
        // by default, there is at least one string that will be correctly
        // aligned (consider single-line case).
        pos.SetY(pos.GetY() - (m_fontSize + m_lineSpacing) * (numLinesOfText - 1.0f));
    }

    // Centered alignment is obtained by offsetting by half the height of the
    // entire text
    else if (m_textVAlignment == IDraw2d::VAlign::Center)
    {
        pos.SetY(pos.GetY() - (m_fontSize + m_lineSpacing) * (numLinesOfText - 1.0f) * 0.5f);
    }

    RenderDrawBatchLines(pos, points, fontContext, newlinePosYIncrement);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiTextComponent::Update(float deltaTime)
{
    AssignSizeReadyFlag();
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiTextComponent::UpdateInEditor()
{
    AssignSizeReadyFlag();
}

////////////////////////////////////////////////////////////////////////////////////////////////////
AZStd::string UiTextComponent::GetText()
{
    return m_text;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiTextComponent::SetText(const AZStd::string& text)
{
    m_text = text;

    // Since display routines use m_locText, we place the text here as well,
    // even though it didn't get translated via localization.
    if (m_elementSizeReady)
    {
        PrepareDisplayedTextInternal(PrepareTextOption::IgnoreLocalization);
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
AZStd::string UiTextComponent::GetTextWithFlags(GetTextFlags flags)
{
    return (flags == UiTextInterface::GetLocalized) ? m_locText : m_text;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiTextComponent::SetTextWithFlags(const AZStd::string& text, SetTextFlags flags)
{
    m_text = text;

    PrepareTextOption prepareOption = PrepareTextOption::IgnoreLocalization;
    if ((flags& UiTextInterface::SetLocalized) == UiTextInterface::SetLocalized)
    {
        prepareOption = PrepareTextOption::Localize;
    }

    if ((flags& UiTextInterface::SetEscapeMarkup) == UiTextInterface::SetEscapeMarkup)
    {
        prepareOption = static_cast<PrepareTextOption>(prepareOption | PrepareTextOption::EscapeMarkup);
    }

    // Since display routines use m_locText, we place the text here as well,
    // even though it didn't get translated via localization.
    if (m_elementSizeReady)
    {
        PrepareDisplayedTextInternal(prepareOption);
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
AZ::Color UiTextComponent::GetColor()
{
    return AZ::Color::CreateFromVector3AndFloat(m_color.GetAsVector3(), m_alpha);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiTextComponent::SetColor(const AZ::Color& color)
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
LyShine::PathnameType UiTextComponent::GetFont()
{
    return m_fontFilename.GetAssetPath().c_str();
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiTextComponent::SetFont(const LyShine::PathnameType& fontPath)
{
    // the input string could be in any form but must be a game path - not a full path.
    // Make it normalized
    AZStd::string newPath = fontPath;
    EBUS_EVENT(AzFramework::ApplicationRequests::Bus, NormalizePath, newPath);

    if (m_fontFilename.GetAssetPath() != newPath)
    {
        ChangeFont(newPath);
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
int UiTextComponent::GetFontEffect()
{
    return m_fontEffectIndex;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiTextComponent::SetFontEffect(int effectIndex)
{
    m_fontEffectIndex = effectIndex;

    m_overrideFontEffectIndex = effectIndex;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
float UiTextComponent::GetFontSize()
{
    return m_fontSize;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiTextComponent::SetFontSize(float fontSize)
{
    m_fontSize = fontSize;
    m_currFontSize = fontSize;

    InvalidateLayout();
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiTextComponent::GetTextAlignment(IDraw2d::HAlign& horizontalAlignment,
    IDraw2d::VAlign& verticalAlignment)
{
    horizontalAlignment = m_textHAlignment;
    verticalAlignment = m_textVAlignment;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiTextComponent::SetTextAlignment(IDraw2d::HAlign horizontalAlignment,
    IDraw2d::VAlign verticalAlignment)
{
    m_textHAlignment = horizontalAlignment;
    m_textVAlignment = verticalAlignment;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
IDraw2d::HAlign UiTextComponent::GetHorizontalTextAlignment()
{
    return m_textHAlignment;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiTextComponent::SetHorizontalTextAlignment(IDraw2d::HAlign alignment)
{
    m_textHAlignment = alignment;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
IDraw2d::VAlign UiTextComponent::GetVerticalTextAlignment()
{
    return m_textVAlignment;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiTextComponent::SetVerticalTextAlignment(IDraw2d::VAlign alignment)
{
    m_textVAlignment = alignment;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
float UiTextComponent::GetCharacterSpacing()
{
    return m_charSpacing;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiTextComponent::SetCharacterSpacing(float characterSpacing)
{
    m_charSpacing = characterSpacing;
    m_currCharSpacing = characterSpacing;

    // Recompute the text since we might have more lines to draw now (for word wrap)
    OnTextWidthPropertyChanged();

    InvalidateLayout();
}

////////////////////////////////////////////////////////////////////////////////////////////////////
float UiTextComponent::GetLineSpacing()
{
    return m_lineSpacing;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiTextComponent::SetLineSpacing(float lineSpacing)
{
    m_lineSpacing = lineSpacing;

    InvalidateLayout();
}

////////////////////////////////////////////////////////////////////////////////////////////////////
int UiTextComponent::GetCharIndexFromPoint(AZ::Vector2 point, bool mustBeInBoundingBox)
{
    // get the input point into untransformed canvas space
    AZ::Vector3 point3(point.GetX(), point.GetY(), 0.0f);
    AZ::Matrix4x4 transform;
    EBUS_EVENT_ID(GetEntityId(), UiTransformBus, GetTransformFromViewport, transform);
    point3 = transform * point3;
    AZ::Vector2 pointInCanvasSpace(point3.GetX(), point3.GetY());

    return GetCharIndexFromCanvasSpacePoint(pointInCanvasSpace, mustBeInBoundingBox);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
int UiTextComponent::GetCharIndexFromCanvasSpacePoint(AZ::Vector2 point, bool mustBeInBoundingBox)
{
    // get the bounding rectangle of the text itself in untransformed canvas space
    UiTransformInterface::RectPoints rect;
    GetTextRect(rect);

    // Since the text rect differs from the clipping rect, we have to adjust
    // the user's input by the clipping offset to match the selection with
    // the contents on-screen.
    point.SetX(point.GetX() + CalculateHorizontalClipOffset());

    // first test if the point is in the bounding box
    // point is in rect if it is within rect or exactly on edge
    bool isInRect = false;
    if (point.GetX() >= rect.TopLeft().GetX() &&
        point.GetX() <= rect.BottomRight().GetX() &&
        point.GetY() >= rect.TopLeft().GetY() &&
        point.GetY() <= rect.BottomRight().GetY())
    {
        isInRect = true;
    }

    if (mustBeInBoundingBox && !isInRect)
    {
        return -1;
    }

    // Get point relative to this element's TopLeft() rect. We use this offset
    // to see how far along we've iterated over the rendered string size and
    // whether or not the index has been found.
    AZ::Vector2 pickOffset = AZ::Vector2(point.GetX() - rect.TopLeft().GetX(),
            point.GetY() - rect.TopLeft().GetY());

    STextDrawContext fontContext(GetTextDrawContextPrototype());

    int indexIter = 0;
    float lastSubstrX = 0;
    float accumulatedHeight = m_fontSize;
    const bool multiLineText = m_drawBatchLines.batchLines.size() > 1;
    uint32_t lineCounter = 0;

    // Iterate over each rendered line of text
    for (const DrawBatchLine& batchLine : m_drawBatchLines.batchLines)
    {
        ++lineCounter;

        // Iterate to the line containing the point
        if (multiLineText && pickOffset.GetY() >= accumulatedHeight)
        {
            // Increment indexIter by number of characters on this line
            for (const DrawBatch& drawBatch : batchLine.drawBatchList)
            {
                AZStd::string displayedText(m_displayedTextFunction(drawBatch.text));
                indexIter += LyShine::GetUtf8StringLength(displayedText);
            }

            accumulatedHeight += m_fontSize;
            continue;
        }

        // In some cases, we may want the cursor to be displayed on the end
        // of a preceding line, and in others, we may want the cursor to be
        // displaying at the beginning of the following line. We resolve this
        // ambiguity by assigning a "hint" to the offsets calculator on where
        // to place the cursor.
        m_cursorLineNumHint = lineCounter;

        // This index allows us to index relative to the current line of text
        // we're iterating on.
        int curLineIndexIter = 0;

        // Iterate across the line
        for (const DrawBatch& drawBatch : batchLine.drawBatchList)
        {
            AZStd::string displayedText(m_displayedTextFunction(drawBatch.text));
            Unicode::CIterator<const char*, false> pChar(displayedText.c_str());
            while (uint32_t ch = *pChar)
            {
                ++pChar;
                curLineIndexIter += LyShine::GetMultiByteCharSize(ch);

                // Iterate across each character of text until the width
                // exceeds the X pick offset.
                AZStd::string subString(displayedText.substr(0, curLineIndexIter));
                Vec2 sizeSoFar = m_font->GetTextSize(subString.c_str(), true, fontContext);
                float charWidth = sizeSoFar.x - lastSubstrX;

                // pickOffset is a screen-position and the text position changes
                // based on its alignment. We add an offset here to account for
                // the location of the text on-screen for different alignments.
                float alignedOffset = 0.0f;
                if (m_textHAlignment == IDraw2d::HAlign::Center)
                {
                    alignedOffset = 0.5f * (rect.GetAxisAlignedSize().GetX() - batchLine.lineSize.GetX());
                }
                else if (m_textHAlignment == IDraw2d::HAlign::Right)
                {
                    alignedOffset = rect.GetAxisAlignedSize().GetX() - batchLine.lineSize.GetX();
                }

                if (pickOffset.GetX() <= alignedOffset + lastSubstrX + (charWidth * 0.5f))
                {
                    return indexIter;
                }

                lastSubstrX = sizeSoFar.x;
                ++indexIter;
            }
        }

        return indexIter;
    }

    // We can reach here if the point is just on the boundary of the rect.
    // In this case, there are no more lines of text to iterate on, so just
    // assume the user is trying to get to the end of the string.
    return indexIter;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
AZ::Vector2 UiTextComponent::GetPointFromCharIndex(int index)
{
    // Left and right offsets for determining the position of the beginning
    // and end of the selection.
    LineOffsets top, middle, bottom;

    GetOffsetsFromSelectionInternal(top, middle, bottom, index, index);

    UiTransformInterface::RectPoints rect;
    GetTextRect(rect);

    // LineOffsets values don't take on-screen position with alignment
    // into account, so we adjust the offset here.
    float alignedOffset = 0.0f;
    if (m_textHAlignment == IDraw2d::HAlign::Center)
    {
        alignedOffset = 0.5f * (rect.GetAxisAlignedSize().GetX() - top.batchLineLength);
    }
    else if (m_textHAlignment == IDraw2d::HAlign::Right)
    {
        alignedOffset = rect.GetAxisAlignedSize().GetX() - top.batchLineLength;
    }

    // Calculate left and right rect positions for start and end selection
    rect.TopLeft().SetX(alignedOffset + rect.TopLeft().GetX() + top.left.GetX());

    // Finally, add the y-offset to position the cursor on the correct line
    // of text.
    rect.TopLeft().SetY(rect.TopLeft().GetY() + top.left.GetY());

    return rect.TopLeft();
}

////////////////////////////////////////////////////////////////////////////////////////////////////
AZ::Color UiTextComponent::GetSelectionColor()
{
    return m_textSelectionColor;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiTextComponent::GetSelectionRange(int& startIndex, int& endIndex)
{
    startIndex = m_selectionStart;
    endIndex = m_selectionEnd;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiTextComponent::SetSelectionRange(int startIndex, int endIndex, const AZ::Color& textSelectionColor)
{
    m_selectionStart = startIndex;
    m_selectionEnd = endIndex;
    m_textSelectionColor = textSelectionColor;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiTextComponent::ClearSelectionRange()
{
    m_selectionStart = m_selectionEnd = -1;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
AZ::Vector2 UiTextComponent::GetTextSize()
{
    // First ensure that the text wrapping is in sync with the element's width.
    // If the element's transform flag is dirty, then the text wrapping does not reflect the current
    // width of the element. Sync it up by checking and handling a change in canvas space size.
    // The notification handler will prepare the text again
    bool canvasSpaceSizeChanged = false;
    EBUS_EVENT_ID_RESULT(canvasSpaceSizeChanged, GetEntityId(), UiTransformBus, HasCanvasSpaceSizeChanged);
    if (canvasSpaceSizeChanged)
    {
        EBUS_EVENT_ID(GetEntityId(), UiTransformBus, NotifyAndResetCanvasSpaceRectChange);
    }

    AZ::Vector2 size = AZ::Vector2(0.0f, 0.0f);

    for (const DrawBatchLine& drawBatchLine : m_drawBatchLines.batchLines)
    {
        size.SetX(AZ::GetMax(drawBatchLine.lineSize.GetX(), size.GetX()));

        size.SetY(size.GetY() + drawBatchLine.lineSize.GetY());
    }

    // Add the extra line spacing to the Y size
    if (m_drawBatchLines.batchLines.size() > 0)
    {
        size.SetY(size.GetY() + (m_drawBatchLines.batchLines.size() - 1) * m_lineSpacing);
    }

    return size;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiTextComponent::GetTextBoundingBox(int startIndex, int endIndex, UiTransformInterface::RectPointsArray& rectPoints)
{
    GetTextBoundingBoxPrivate(startIndex, endIndex, rectPoints);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiTextComponent::GetTextBoundingBoxPrivate(int startIndex, int endIndex, UiTransformInterface::RectPointsArray& rectPoints)
{
    // Multi-line selection can be broken up into three pairs of offsets
    // representing the first (top) and last (bottom) lines, and everything
    // in-between (middle).
    LineOffsets top, middle, bottom;

    GetOffsetsFromSelectionInternal(top, middle, bottom, startIndex, endIndex);

    AZStd::stack<LineOffsets*> lineOffsetsStack;
    lineOffsetsStack.push(&bottom);
    lineOffsetsStack.push(&middle);
    lineOffsetsStack.push(&top);

    // Build rectPoints array depending on how many lines of text are selected
    rectPoints.push_back(UiTransformInterface::RectPoints());

    AZ::Vector2 zeroVector = AZ::Vector2::CreateZero();
    if (middle.left != zeroVector || middle.right != zeroVector)
    {
        rectPoints.push_back(UiTransformInterface::RectPoints());
    }

    if (bottom.left != zeroVector || bottom.right != zeroVector)
    {
        rectPoints.push_back(UiTransformInterface::RectPoints());
    }

    // Build RectPoints geometry based on selected lines of text
    for (UiTransformInterface::RectPoints& rect : rectPoints)
    {
        LineOffsets* lineOffsets = lineOffsetsStack.top();
        lineOffsetsStack.pop();
        AZ::Vector2& leftOffset = lineOffsets->left;
        AZ::Vector2& rightOffset = lineOffsets->right;

        // GetTextSize() returns the max width and height that this text component
        // occupies on-screen.
        AZ::Vector2 textSize(GetTextSize());

        // For a multi-line selection, the width of our selection will span the
        // entire text element width. Otherwise, we need to adjust the text
        // size to only account for the current line width.
        const bool multiLineSection = leftOffset.GetY() < rightOffset.GetY();
        if (!multiLineSection)
        {
            textSize.SetX(lineOffsets->batchLineLength);
        }

        GetTextRect(rect, textSize);

        rect.TopLeft().SetX(rect.TopLeft().GetX() + leftOffset.GetX());
        rect.BottomLeft().SetX(rect.BottomLeft().GetX() + leftOffset.GetX());
        rect.TopRight().SetX(rect.TopLeft().GetX() + rightOffset.GetX());
        rect.BottomRight().SetX(rect.BottomLeft().GetX() + rightOffset.GetX());

        // Finally, add the y-offset to position the cursor on the correct line
        // of text.
        rect.TopLeft().SetY(rect.TopLeft().GetY() + leftOffset.GetY());
        rect.TopRight().SetY(rect.TopRight().GetY() + leftOffset.GetY());
        rightOffset.SetY(rightOffset.GetY() > 0.0f ? rightOffset.GetY() : m_fontSize);
        rect.BottomLeft().SetY(rect.TopLeft().GetY() + rightOffset.GetY());
        rect.BottomRight().SetY(rect.TopRight().GetY() + rightOffset.GetY());

        // Adjust cursor position to account for clipped text
        if (m_overflowMode == OverflowMode::ClipText)
        {
            UiTransformInterface::RectPoints elemRect;
            EBUS_EVENT_ID(GetEntityId(), UiTransformBus, GetCanvasSpacePointsNoScaleRotate, elemRect);
            AZ::Vector2 elemSize = elemRect.GetAxisAlignedSize();
            const float displayedTextWidth = GetTextSize().GetX();

            // When we render clipped text, we offset its draw position in order to
            // clip the text properly and keep the visible text within the bounds of
            // the element. Here, we account for that offset in order to render the
            // cursor position at the correct location.
            const bool textOverflowing = displayedTextWidth > elemSize.GetX();
            if (textOverflowing)
            {
                const float rectOffset = CalculateHorizontalClipOffset();
                rect.TopLeft().SetX(rect.TopLeft().GetX() - rectOffset);
                rect.BottomLeft().SetX(rect.BottomLeft().GetX() - rectOffset);
                rect.TopRight().SetX(rect.TopRight().GetX() - rectOffset);
                rect.BottomRight().SetX(rect.BottomRight().GetX() - rectOffset);

                // For clipped selections we can end up with a rect that is too big
                // for the clipped boundaries. Here, we restrict the selection rect
                // size to match the boundaries of the element's size.
                rect.TopLeft().SetX(AZStd::GetMax<float>(elemRect.TopLeft().GetX(), rect.TopLeft().GetX()));
                rect.BottomLeft().SetX(AZStd::GetMax<float>(elemRect.BottomLeft().GetX(), rect.BottomLeft().GetX()));
                rect.TopRight().SetX(AZStd::GetMin<float>(elemRect.TopRight().GetX(), rect.TopRight().GetX()));
                rect.BottomRight().SetX(AZStd::GetMin<float>(elemRect.BottomRight().GetX(), rect.BottomRight().GetX()));
            }
        }

        // now we have the rect in untransformed canvas space, so transform it to viewport space
        EBUS_EVENT_ID(GetEntityId(), UiTransformBus, RotateAndScalePoints, rect);

        // if the start and end indices are the same we want to draw a cursor
        if (startIndex == endIndex)
        {
            // we want to make the rect one pixel wide in transformed space.
            // Get the transform to viewport for the text entity
            AZ::Matrix4x4 transformToViewport;
            EBUS_EVENT_ID(GetEntityId(), UiTransformBus, GetTransformToViewport, transformToViewport);

            // take a sample vector along X-axis and transform it then normalize it
            AZ::Vector3 offset(100.0f, 0.0f, 0.0f);
            AZ::Vector3 transformedOffset3 = transformToViewport.Multiply3x3(offset);
            transformedOffset3.NormalizeSafe();
            AZ::Vector2 transformedOffset(transformedOffset3.GetX(), transformedOffset3.GetY());

            // to help with scaled and rotated text round the offset to nearest pixels
            transformedOffset = Draw2dHelper::RoundXY(transformedOffset, IDraw2d::Rounding::Nearest);

            // before making it exactly one pixel wide, round the left edge to either the nearest pixel or round down
            // (nearest looks best for fonts smaller than 32 and down looks best for fonts larger than 32).
            // Really a better solution would be to draw a textured quad. In the 32 pt proportional font there is
            // usually exactly 2 pixels between characters so by picking one pixel to draw the line on we either make
            // it closer to one character or the other. If we had a text cursor texture we could draw a 4 pixel wide
            // quad and it would look better. A cursor would also look smoother when rotated than a one pixel line.
            // NOTE: The positions of text characters themselves will always be rounded DOWN to a pixel in the
            // font rendering
            IDraw2d::Rounding round = (m_fontSize < 32) ? IDraw2d::Rounding::Nearest : IDraw2d::Rounding::Down;
            rect.TopLeft() = Draw2dHelper::RoundXY(rect.TopLeft(), round);
            rect.BottomLeft() = Draw2dHelper::RoundXY(rect.BottomLeft(), round);

            // now add the unit vector to the two left hand corners of the transformed rect
            // to get the two right hand corners.
            // it will now be one pixel wide in transformed space
            rect.TopRight() = rect.TopLeft() + AZ::Vector2(transformedOffset);
            rect.BottomRight() = rect.BottomLeft() + AZ::Vector2(transformedOffset);
        }
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiTextComponent::SetDisplayedTextFunction(const DisplayedTextFunction& displayedTextFunction)
{
    if (displayedTextFunction)
    {
        m_displayedTextFunction = displayedTextFunction;
    }
    else
    {
        // For null function objects, we fall back on our default implementation
        m_displayedTextFunction = DefaultDisplayedTextFunction;
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
UiTextInterface::OverflowMode UiTextComponent::GetOverflowMode()
{
    return m_overflowMode;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiTextComponent::SetOverflowMode(OverflowMode overflowMode)
{
    m_overflowMode = overflowMode;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
UiTextInterface::WrapTextSetting UiTextComponent::GetWrapText()
{
    return m_wrapTextSetting;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiTextComponent::SetWrapText(WrapTextSetting wrapSetting)
{
    m_wrapTextSetting = wrapSetting;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiTextComponent::PropertyValuesChanged()
{
    if (!m_isColorOverridden)
    {
        m_overrideColor = m_color;
    }
    if (!m_isAlphaOverridden)
    {
        m_overrideAlpha = m_alpha;
    }

    if (!m_isFontFamilyOverridden)
    {
        m_overrideFontFamily = m_fontFamily;
    }

    if (!m_isFontEffectOverridden)
    {
        m_overrideFontEffectIndex = m_fontEffectIndex;
    }

    // If any of the properties that affect line width changed
    if (m_currFontSize != m_fontSize || m_currCharSpacing != m_charSpacing)
    {
        OnTextWidthPropertyChanged();

        m_currFontSize = m_fontSize;
        m_currCharSpacing = m_charSpacing;
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiTextComponent::OnCanvasSpaceRectChanged(AZ::EntityId entityId, const UiTransformInterface::Rect& oldRect, const UiTransformInterface::Rect& newRect)
{
    // If old rect equals new rect, size changed due to initialization
    bool sizeChanged = (oldRect == newRect) || (!oldRect.GetSize().IsClose(newRect.GetSize(), 0.05f));

    if (sizeChanged)
    {
        // OnCanvasSpaceRectChanged (with a size change) is called on the first canvas update, so text will initially be prepared here.
        // Text will also be prepared on all subsequent calls to OnCanvasSpaceRectChanged with a size change.
        // This will remove the current word-wrap applied to the string by re-initializing the
        // string's contents via the loc system

        // Show xml warnings on the initial call to OnCanvasSpaceRectChanged while suppressing xml warnings on all subsequent calls
        bool suppressXmlWarnings = m_elementSizeReady;

        PrepareDisplayedTextInternal(PrepareTextOption::Localize, suppressXmlWarnings, false);
    }

    // If the text is wrapped, we need to know the element size before calling PrepareDisplayedTextInternal.
    // Since OnCanvasSpaceRectChanged is called on the first canvas update, we know that the component has been activated
    // at this time, and the element size can be retrieved. We can't use InGamePostActivate because it is only available
    // at runtime. We also need word-wrap to occur in the editor, so we use a simple boolean variable instead.
    AssignSizeReadyFlag();
}

////////////////////////////////////////////////////////////////////////////////////////////////////
float UiTextComponent::GetMinWidth()
{
    return 0.0f;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
float UiTextComponent::GetMinHeight()
{
    return 0.0f;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
float UiTextComponent::GetTargetWidth()
{
    // Calculate draw batch lines without relying on element width (ignoring wrapping)
    DrawBatchLines drawBatchLines;
    AZStd::string locText;
    CalculateDrawBatchLines(drawBatchLines, locText, PrepareTextOption::Localize, true, false, false);

    // Calculate the target width based on the draw batch line sizes
    float textWidth = 0.0f;
    for (const DrawBatchLine& drawBatchLine : drawBatchLines.batchLines)
    {
        textWidth = AZ::GetMax(drawBatchLine.lineSize.GetX(), textWidth);
    }

    if (m_wrapTextSetting != UiTextInterface::WrapTextSetting::NoWrap)
    {
        // In order for the wrapping to remain the same after the resize, the
        // text element width would need to match the string width exactly. To accommodate
        // for slight variation in size, add a small value to ensure that the string will fit
        // inside the text element's bounds. The downside to this is there may be extra space
        // at the bottom, but this is unlikely.
        const float epsilon = 0.01f;
        textWidth += epsilon;
    }

    return textWidth;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
float UiTextComponent::GetTargetHeight()
{
    // Since target height is calculated after widths are assigned, it can rely on the element's width
    AZ::Vector2 textSize = GetTextSize();
    return textSize.GetY();
}

////////////////////////////////////////////////////////////////////////////////////////////////////
float UiTextComponent::GetExtraWidthRatio()
{
    return 1.0f;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
float UiTextComponent::GetExtraHeightRatio()
{
    return 1.0f;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiTextComponent::LanguageChanged()
{
    // the font family may have been deleted and reloaded so make sure we update m_fontFamily
    ChangeFont(m_fontFilename.GetAssetPath());
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// PUBLIC STATIC MEMBER FUNCTIONS
////////////////////////////////////////////////////////////////////////////////////////////////////

void UiTextComponent::Reflect(AZ::ReflectContext* context)
{
    AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);

    if (serializeContext)
    {
        serializeContext->Class<UiTextComponent, AZ::Component>()
            ->Version(7, &VersionConverter)
            ->Field("Text", &UiTextComponent::m_text)
            ->Field("Color", &UiTextComponent::m_color)
            ->Field("Alpha", &UiTextComponent::m_alpha)
            ->Field("FontFileName", &UiTextComponent::m_fontFilename)
            ->Field("FontSize", &UiTextComponent::m_fontSize)
            ->Field("EffectIndex", &UiTextComponent::m_fontEffectIndex)
            ->Field("TextHAlignment", &UiTextComponent::m_textHAlignment)
            ->Field("TextVAlignment", &UiTextComponent::m_textVAlignment)
            ->Field("CharacterSpacing", &UiTextComponent::m_charSpacing)
            ->Field("LineSpacing", &UiTextComponent::m_lineSpacing)
            ->Field("OverflowMode", &UiTextComponent::m_overflowMode)
            ->Field("WrapTextSetting", &UiTextComponent::m_wrapTextSetting);

        AZ::EditContext* ec = serializeContext->GetEditContext();
        if (ec)
        {
            auto editInfo = ec->Class<UiTextComponent>("Text", "A visual component that draws a text string");

            editInfo->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                ->Attribute(AZ::Edit::Attributes::Icon, "Editor/Icons/Components/UiText.png")
                ->Attribute(AZ::Edit::Attributes::ViewportIcon, "Editor/Icons/Components/Viewport/UiText.png")
                ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC("UI", 0x27ff46b0))
                ->Attribute(AZ::Edit::Attributes::AutoExpand, true);

            editInfo->DataElement(0, &UiTextComponent::m_text, "Text", "The text string")
                ->Attribute(AZ::Edit::Attributes::ChangeNotify, &UiTextComponent::OnTextChange)
                ->Attribute(AZ::Edit::Attributes::ChangeNotify, &UiTextComponent::CheckLayoutFitterAndRefreshEditorTransformProperties);
            editInfo->DataElement(AZ::Edit::UIHandlers::Color, &UiTextComponent::m_color, "Color", "The color to draw the text string")
                ->Attribute(AZ::Edit::Attributes::ChangeNotify, &UiTextComponent::OnColorChange);
            editInfo->DataElement(AZ::Edit::UIHandlers::Slider, &UiTextComponent::m_alpha, "Alpha", "The transparency of the text string")
                ->Attribute(AZ::Edit::Attributes::ChangeNotify, &UiTextComponent::OnColorChange)
                ->Attribute(AZ::Edit::Attributes::Min, 0.0f)
                ->Attribute(AZ::Edit::Attributes::Max, 1.0f);
            editInfo->DataElement("SimpleAssetRef", &UiTextComponent::m_fontFilename, "Font path", "The pathname to the font")
                ->Attribute(AZ::Edit::Attributes::ChangeNotify, &UiTextComponent::OnFontPathnameChange)
                ->Attribute(AZ::Edit::Attributes::ChangeNotify, &UiTextComponent::CheckLayoutFitterAndRefreshEditorTransformProperties);
            editInfo->DataElement(AZ::Edit::UIHandlers::ComboBox, &UiTextComponent::m_fontEffectIndex, "Font effect", "The font effect (from font file)")
                ->Attribute("EnumValues", &UiTextComponent::PopulateFontEffectList)
                ->Attribute(AZ::Edit::Attributes::ChangeNotify, &UiTextComponent::OnFontEffectChange)
                ->Attribute(AZ::Edit::Attributes::ChangeNotify, &UiTextComponent::CheckLayoutFitterAndRefreshEditorTransformProperties);
            editInfo->DataElement(AZ::Edit::UIHandlers::SpinBox, &UiTextComponent::m_fontSize, "Font size", "The size of the font in points")
                ->Attribute(AZ::Edit::Attributes::ChangeNotify, &UiTextComponent::OnFontSizeChange)
                ->Attribute(AZ::Edit::Attributes::ChangeNotify, &UiTextComponent::CheckLayoutFitterAndRefreshEditorTransformProperties)
                ->Attribute(AZ::Edit::Attributes::Min, 0.0f)
                ->Attribute(AZ::Edit::Attributes::Step, 1.0f);
            editInfo->DataElement(AZ::Edit::UIHandlers::ComboBox, &UiTextComponent::m_textHAlignment, "Horizontal text alignment", "How to align the text within the rect")
                ->EnumAttribute(IDraw2d::HAlign::Left, "Left")
                ->EnumAttribute(IDraw2d::HAlign::Center, "Center")
                ->EnumAttribute(IDraw2d::HAlign::Right, "Right");
            editInfo->DataElement(AZ::Edit::UIHandlers::ComboBox, &UiTextComponent::m_textVAlignment, "Vertical text alignment", "How to align the text within the rect")
                ->EnumAttribute(IDraw2d::VAlign::Top, "Top")
                ->EnumAttribute(IDraw2d::VAlign::Center, "Center")
                ->EnumAttribute(IDraw2d::VAlign::Bottom, "Bottom");
            editInfo->DataElement(0, &UiTextComponent::m_charSpacing, "Character Spacing",
                "The spacing in 1/1000th of ems to add between each two consecutive characters.\n"
                "One em is equal to the currently specified font size.")
                ->Attribute(AZ::Edit::Attributes::ChangeNotify, &UiTextComponent::OnCharSpacingChange)
                ->Attribute(AZ::Edit::Attributes::ChangeNotify, &UiTextComponent::CheckLayoutFitterAndRefreshEditorTransformProperties);
            editInfo->DataElement(0, &UiTextComponent::m_lineSpacing, "Line Spacing", "The amount of pixels to add between each two consecutive lines.")
                ->Attribute(AZ::Edit::Attributes::ChangeNotify, &UiTextComponent::OnLineSpacingChange)
                ->Attribute(AZ::Edit::Attributes::ChangeNotify, &UiTextComponent::CheckLayoutFitterAndRefreshEditorTransformProperties);
            editInfo->DataElement(AZ::Edit::UIHandlers::ComboBox, &UiTextComponent::m_overflowMode, "Overflow mode", "How text should fit within the element")
                ->EnumAttribute(OverflowMode::OverflowText, "Overflow")
                ->EnumAttribute(OverflowMode::ClipText, "Clip text");
            editInfo->DataElement(AZ::Edit::UIHandlers::ComboBox, &UiTextComponent::m_wrapTextSetting, "Wrap text", "Determines whether text is wrapped")
                ->Attribute(AZ::Edit::Attributes::ChangeNotify, &UiTextComponent::OnWrapTextSettingChange)
                ->Attribute(AZ::Edit::Attributes::ChangeNotify, &UiTextComponent::CheckLayoutFitterAndRefreshEditorTransformProperties)
                ->EnumAttribute(WrapTextSetting::NoWrap, "No wrap")
                ->EnumAttribute(WrapTextSetting::Wrap, "Wrap text");
        }
    }

    AZ::BehaviorContext* behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context);
    if (behaviorContext)
    {
        behaviorContext->EBus<UiTextBus>("UiTextBus")
            ->Event("GetText", &UiTextBus::Events::GetText)
            ->Event("SetText", &UiTextBus::Events::SetText)
            ->Event("GetColor", &UiTextBus::Events::GetColor)
            ->Event("SetColor", &UiTextBus::Events::SetColor)
            ->Event("GetFont", &UiTextBus::Events::GetFont)
            ->Event("SetFont", &UiTextBus::Events::SetFont)
            ->Event("GetFontEffect", &UiTextBus::Events::GetFontEffect)
            ->Event("SetFontEffect", &UiTextBus::Events::SetFontEffect)
            ->Event("GetFontSize", &UiTextBus::Events::GetFontSize)
            ->Event("SetFontSize", &UiTextBus::Events::SetFontSize)
            ->Event("GetHorizontalTextAlignment", &UiTextBus::Events::GetHorizontalTextAlignment)
            ->Event("SetHorizontalTextAlignment", &UiTextBus::Events::SetHorizontalTextAlignment)
            ->Event("GetVerticalTextAlignment", &UiTextBus::Events::GetVerticalTextAlignment)
            ->Event("SetVerticalTextAlignment", &UiTextBus::Events::SetVerticalTextAlignment)
            ->Event("GetCharacterSpacing", &UiTextBus::Events::GetCharacterSpacing)
            ->Event("SetCharacterSpacing", &UiTextBus::Events::SetCharacterSpacing)
            ->Event("GetLineSpacing", &UiTextBus::Events::GetLineSpacing)
            ->Event("SetLineSpacing", &UiTextBus::Events::SetLineSpacing)
            ->Event("GetOverflowMode", &UiTextBus::Events::GetOverflowMode)
            ->Event("SetOverflowMode", &UiTextBus::Events::SetOverflowMode)
            ->Event("GetWrapText", &UiTextBus::Events::GetWrapText)
            ->Event("SetWrapText", &UiTextBus::Events::SetWrapText)
            ->VirtualProperty("FontSize", "GetFontSize", "SetFontSize")
            ->VirtualProperty("Color", "GetColor", "SetColor")
            ->VirtualProperty("CharacterSpacing", "GetCharacterSpacing", "SetCharacterSpacing")
            ->VirtualProperty("LineSpacing", "GetLineSpacing", "SetLineSpacing");

        behaviorContext->Class<UiTextComponent>()->RequestBus("UiTextBus");

        behaviorContext->Enum<(int)UiTextInterface::OverflowMode::OverflowText>("eUiTextOverflowMode_OverflowText")
            ->Enum<(int)UiTextInterface::OverflowMode::ClipText>("eUiTextOverflowMode_ClipText")
            ->Enum<(int)UiTextInterface::WrapTextSetting::NoWrap>("eUiTextWrapTextSetting_NoWrap")
            ->Enum<(int)UiTextInterface::WrapTextSetting::Wrap>("eUiTextWrapTextSetting_Wrap");
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// PROTECTED MEMBER FUNCTIONS
////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiTextComponent::Init()
{
    m_overrideColor = m_color;
    m_overrideAlpha = m_alpha;
    m_overrideFontFamily = m_fontFamily;
    m_overrideFontEffectIndex = m_fontEffectIndex;

    // If this is called from RC.exe for example these pointers will not be set. In that case
    // we only need to be able to load, init and save the component. It will never be
    // activated.
    if (!gEnv || !gEnv->pCryFont || !gEnv->pSystem)
    {
        return;
    }

    // if the font is not the one specified by the path (e.g. after loading using serialization)
    if (gEnv->pCryFont->GetFontFamily(m_fontFilename.GetAssetPath().c_str()) != m_fontFamily)
    {
        ChangeFont(m_fontFilename.GetAssetPath());
    }

    // A call to prepare text is required here to build the draw batches for display. When
    // adding a text component to a pre-existing element, the rect may or may not change,
    // so we can't rely on that to prepare the text for display. Text components with fonts
    // other than the default font can also cause the text to be prepared for display, but if
    // the font used is the default font, that won't trigger a prepare either.
    PrepareDisplayedTextInternal(PrepareTextOption::Localize, true);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiTextComponent::Activate()
{
    UiVisualBus::Handler::BusConnect(m_entity->GetId());
    UiRenderBus::Handler::BusConnect(m_entity->GetId());
    UiUpdateBus::Handler::BusConnect(m_entity->GetId());
    UiTextBus::Handler::BusConnect(m_entity->GetId());
    UiAnimateEntityBus::Handler::BusConnect(m_entity->GetId());
    UiTransformChangeNotificationBus::Handler::BusConnect(m_entity->GetId());
    UiLayoutCellDefaultBus::Handler::BusConnect(m_entity->GetId());
    LanguageChangeNotificationBus::Handler::BusConnect();
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiTextComponent::Deactivate()
{
    UiVisualBus::Handler::BusDisconnect();
    UiRenderBus::Handler::BusDisconnect();
    UiUpdateBus::Handler::BusDisconnect();
    UiTextBus::Handler::BusDisconnect();
    UiAnimateEntityBus::Handler::BusDisconnect();
    UiTransformChangeNotificationBus::Handler::BusDisconnect();
    UiLayoutCellDefaultBus::Handler::BusDisconnect();
    LanguageChangeNotificationBus::Handler::BusDisconnect();
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiTextComponent::ChangeFont(const AZStd::string& fontFileName)
{
    AZStd::string fileName = fontFileName;
    if (fileName.empty())
    {
        fileName = "default-ui";
    }

    FontFamilyPtr fontFamily = gEnv->pCryFont->GetFontFamily(fileName.c_str());
    if (!fontFamily)
    {
        fontFamily = gEnv->pCryFont->LoadFontFamily(fileName.c_str());
    }

    if (fontFamily)
    {
        m_fontFamily = fontFamily;
        m_font = m_fontFamily->normal;

        // we know that the input path is a root relative and normalized pathname
        m_fontFilename.SetAssetPath(fileName.c_str());

        // the font has changed so check that the font effect is valid
        unsigned int numEffects = m_font->GetNumEffects();
        if (m_fontEffectIndex >= numEffects)
        {
            m_fontEffectIndex = 0;
        }

        if (!m_isFontFamilyOverridden)
        {
            m_overrideFontFamily = m_fontFamily;

            if (m_overrideFontEffectIndex >= numEffects)
            {
                m_overrideFontEffectIndex = m_fontEffectIndex;
            }
        }

        // When the font changes, we need to rebuild our draw batches, but we
        // need to know the element's size for word-wrapping. ChangeFont may
        // be called prior to activation, so guard against that (batches will
        // be re-built regardless on first render iteration - "post-activate").
        if (m_elementSizeReady)
        {
            PrepareDisplayedTextInternal(PrepareTextOption::Localize, true);
        }
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiTextComponent::GetTextRect(UiTransformInterface::RectPoints& rect)
{
    GetTextRect(rect, GetTextSize());
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiTextComponent::GetTextRect(UiTransformInterface::RectPoints& rect, const AZ::Vector2& textSize)
{
    // get the "no scale rotate" element box
    UiTransformInterface::RectPoints elemRect;
    EBUS_EVENT_ID(GetEntityId(), UiTransformBus, GetCanvasSpacePointsNoScaleRotate, elemRect);
    AZ::Vector2 elemSize = elemRect.GetAxisAlignedSize();

    // given the text alignment work out the box of the actual text
    rect = elemRect;
    switch (m_textHAlignment)
    {
    case IDraw2d::HAlign::Left:
        rect.BottomRight().SetX(rect.TopLeft().GetX() + textSize.GetX());
        rect.TopRight().SetX(rect.BottomRight().GetX());
        break;
    case IDraw2d::HAlign::Center:
    {
        float centerX = (rect.TopLeft().GetX() + rect.TopRight().GetX()) * 0.5f;
        float halfWidth = textSize.GetX() * 0.5f;
        rect.BottomLeft().SetX(centerX - halfWidth);
        rect.TopLeft().SetX(rect.BottomLeft().GetX());
        rect.BottomRight().SetX(centerX + halfWidth);
        rect.TopRight().SetX(rect.BottomRight().GetX());
        break;
    }
    case IDraw2d::HAlign::Right:
        rect.BottomLeft().SetX(rect.TopRight().GetX() - textSize.GetX());
        rect.TopLeft().SetX(rect.BottomLeft().GetX());
        break;
    }
    switch (m_textVAlignment)
    {
    case IDraw2d::VAlign::Top:
        rect.BottomLeft().SetY(rect.TopLeft().GetY() + textSize.GetY());
        rect.BottomRight().SetY(rect.BottomLeft().GetY());
        break;
    case IDraw2d::VAlign::Center:
    {
        float centerY = (rect.TopLeft().GetY() + rect.BottomLeft().GetY()) * 0.5f;
        float halfHeight = textSize.GetY() * 0.5f;
        rect.TopLeft().SetY(centerY - halfHeight);
        rect.TopRight().SetY(rect.TopLeft().GetY());
        rect.BottomLeft().SetY(centerY + halfHeight);
        rect.BottomRight().SetY(rect.BottomLeft().GetY());
        break;
    }
    case IDraw2d::VAlign::Bottom:
        rect.TopLeft().SetY(rect.BottomLeft().GetY() - textSize.GetY());
        rect.TopRight().SetY(rect.TopLeft().GetY());
        break;
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiTextComponent::OnTextChange()
{
    PrepareDisplayedTextInternal(PrepareTextOption::Localize, true);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiTextComponent::OnColorChange()
{
    m_overrideColor = m_color;
    m_overrideAlpha = m_alpha;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiTextComponent::OnFontSizeChange()
{
    InvalidateLayout();

    OnTextWidthPropertyChanged();
}

////////////////////////////////////////////////////////////////////////////////////////////////////
AZ::u32 UiTextComponent::OnFontPathnameChange()
{
    // we should be guaranteed that the asset path in the simple asset ref is root relative and
    // normalized. But just to be safe we make sure is normalized
    AZStd::string fontPath = m_fontFilename.GetAssetPath();
    EBUS_EVENT(AzFramework::ApplicationRequests::Bus, NormalizePath, fontPath);
    m_fontFilename.SetAssetPath(fontPath.c_str());

    // if the font we have loaded has a different pathname to the one we want then change
    // the font (Release the old one and Load or AddRef the new one)
    if (gEnv->pCryFont->GetFontFamily(fontPath.c_str()) != m_fontFamily)
    {
        ChangeFont(m_fontFilename.GetAssetPath());
    }

    return AZ::Edit::PropertyRefreshLevels::AttributesAndValues;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiTextComponent::OnFontEffectChange()
{
    m_overrideFontEffectIndex = m_fontEffectIndex;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiTextComponent::OnWrapTextSettingChange()
{
    PrepareDisplayedTextInternal(PrepareTextOption::Localize, true);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiTextComponent::OnCharSpacingChange()
{
    InvalidateLayout();

    OnTextWidthPropertyChanged();
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiTextComponent::OnLineSpacingChange()
{
    InvalidateLayout();
}

////////////////////////////////////////////////////////////////////////////////////////////////////
UiTextComponent::FontEffectComboBoxVec UiTextComponent::PopulateFontEffectList()
{
    FontEffectComboBoxVec result;
    AZStd::vector<AZ::EntityId> entityIdList;

    // there is always a valid font since we default to "default-ui"
    // so just get the effects from the font and add their names to the result list
    unsigned int numEffects = m_font->GetNumEffects();
    for (int i = 0; i < numEffects; ++i)
    {
        const char* name = m_font->GetEffectName(i);
        result.push_back(AZStd::make_pair(i, name));
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiTextComponent::WrapText()
{
    UiTransformInterface::RectPoints points;
    EBUS_EVENT_ID(GetEntityId(), UiTransformBus, GetCanvasSpacePointsNoScaleRotate, points);
    const float elementWidth = points.GetAxisAlignedSize().GetX();

    STextDrawContext fontContext(GetTextDrawContextPrototype());
    string wrappedText;
    m_font->WrapText(wrappedText, elementWidth, m_locText.c_str(), fontContext);
    m_locText = wrappedText;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
float UiTextComponent::CalculateHorizontalClipOffset()
{
    const bool cursorIsValid = m_selectionStart >= 0;

    if (m_overflowMode == OverflowMode::ClipText && m_wrapTextSetting != WrapTextSetting::Wrap && cursorIsValid)
    {
        UiTransformInterface::RectPoints points;
        EBUS_EVENT_ID(GetEntityId(), UiTransformBus, GetCanvasSpacePointsNoScaleRotate, points);

        STextDrawContext fontContext(GetTextDrawContextPrototype());

        const AZStd::string displayedText(m_displayedTextFunction(m_locText));
        const AZ::Vector2 displayedTextSize(GetTextSize());
        const AZ::Vector2 elemSize(points.GetAxisAlignedSize());
        const bool textOverflowing = displayedTextSize.GetX() > elemSize.GetX();

        if (textOverflowing)
        {
            // Get size of text from beginning of the string to the end of
            // the text selection. This forms the basis of the assumptions
            // for the left and center-justified text cases, specifically for
            // calculating the following boolean variables for each case:
            // - cursorAtFirstChar
            // - cursorClippedRight
            // - cursorClippedLeft
            int bytesToSelectionEnd = LyShine::GetByteLengthOfUtf8Chars(displayedText.c_str(), m_selectionEnd);
            AZStd::string leftString(displayedText.substr(0, bytesToSelectionEnd));
            Vec2 leftSize(m_font->GetTextSize(leftString.c_str(), true, fontContext));

            if (m_textHAlignment == IDraw2d::HAlign::Left)
            {
                // Positive clip offset will scroll text left
                m_clipOffsetMultiplier = 1.0f;

                // Positive clip offsets scroll the text left, and negative
                // scrolls the text right. Zero is the minimum for left-
                // aligned since there is no text to scroll to before the first
                // character in the string.
                const float clipOffsetMin = 0.0f;

                // Width of the clipping area to the left of the visible text
                const float clipOffsetLeft = m_clipOffset;

                // We calculate the clip offset differently based on where
                // the cursor position is currently located.
                const bool cursorAtFirstChar = leftSize.x == 0.0f;
                const bool cursorClippedRight = leftSize.x > elemSize.GetX() + clipOffsetLeft;
                const bool cursorClippedLeft = leftSize.x < clipOffsetLeft;

                if (cursorAtFirstChar)
                {
                    m_clipOffset = clipOffsetMin;
                }
                else if (cursorClippedRight)
                {
                    // Scroll the text left to display characters to the
                    // right of the clipping area. The amount scrolled by is
                    // the clipped and non-clipped widths added together and
                    // subtracted from the string size to the left of the cursor.
                    m_clipOffset += leftSize.x - elemSize.GetX() - clipOffsetLeft;
                }
                else if (cursorClippedLeft)
                {
                    // Cursor is clipped to the left, so scroll the text
                    // right by decreasing the clip offset.
                    m_clipOffset = leftSize.x;
                }
            }

            else if (m_textHAlignment == IDraw2d::HAlign::Center)
            {
                // At zero offset, text is displayed centered. Negative
                // values scroll text to the right, so to display the first
                // char in the string, we would need to scroll by half of the
                // total clipped text.
                const float clipOffsetMin = -0.5f * (displayedTextSize.GetX() - elemSize.GetX());

                // Width of the clipped text to the left of the visible text. Adjusted
                // by the min clipping value when the offset becomes negative.
                const float clipOffsetLeft = m_clipOffset >= 0.0f ? m_clipOffset : m_clipOffset - clipOffsetMin;

                const bool cursorAtFirstChar = leftSize.x == 0.0f;
                const bool cursorClippedRight = leftSize.x > elemSize.GetX() + clipOffsetLeft;
                const bool cursorClippedLeft = leftSize.x < clipOffsetLeft;

                if (cursorAtFirstChar)
                {
                    m_clipOffset = clipOffsetMin;
                    m_clipOffsetMultiplier = 1.0f;
                }
                else if (cursorClippedRight)
                {
                    // Similar to left-aligned text, but we adjust our offset
                    // multiplier to account for half of the width already
                    // being accounted for in centered-alignment logic elsewhere.
                    m_clipOffset += leftSize.x - elemSize.GetX() - clipOffsetLeft;
                    m_clipOffsetMultiplier = 0.5f;
                }
                else if (cursorClippedLeft)
                {
                    const float prevClipOffset = m_clipOffset;
                    m_clipOffset = leftSize.x;

                    // Obtain a multiplier that, when multiplied by the new
                    // offset, returns the current offset value, minus the
                    // difference between the current and new offsets (to
                    // account for the clipped space).
                    const float clipOffsetInverse = 1.0f / m_clipOffset;
                    m_clipOffsetMultiplier = clipOffsetInverse * (prevClipOffset * (m_clipOffsetMultiplier - 1) + leftSize.x);
                }
            }

            // Handle right-alignment
            else
            {
                // Get the size of the text following the text selection. This
                // is in contrast to left and center-aligned text, simply
                // because it's more intuitive when dealing with right-
                // aligned text, for the following conditions:
                // - cursorAtFirstChar
                // - cursorClippedRight
                // - cursorClippedLeft
                AZStd::string rightString(displayedText.substr(bytesToSelectionEnd, displayedText.length() - bytesToSelectionEnd));
                Vec2 rightSize(m_font->GetTextSize(rightString.c_str(), true, fontContext));

                // Negative offset will scroll text to the right
                m_clipOffsetMultiplier = -1.0f;

                // Clip offset 0 means the text is text is furthest to the
                // right (for right-justified text).
                const float clipOffsetMin = 0.0f;

                // The difference between the total string size and element
                // size results in the total width that is clipped. When
                // the offset reaches this max value, the text is scrolled
                // furthest to the right (displaying the left-most character
                // in the string).
                const float clipOffsetMax = -1.0f * (displayedTextSize.GetX() - elemSize.GetX());

                // Amout of clipped text to the right of the non-clipped text
                const float clipOffsetRight = m_clipOffset;

                // Amout of clipped text to the left of the non-clipped text
                const float clipOffsetLeft = clipOffsetRight > 0.0f ? fabs(clipOffsetMax) - clipOffsetRight : 0.0f;

                const bool cursorAtFirstChar = rightSize.x == 0.0f;
                const bool cursorClippedRight = leftSize.x > elemSize.GetX() + clipOffsetLeft;
                const bool cursorClippedLeft = rightSize.x > elemSize.GetX() + clipOffsetRight;

                if (cursorAtFirstChar)
                {
                    m_clipOffset = clipOffsetMin;
                }
                else if (cursorClippedRight)
                {
                    // The way the math is setup, if clip offset is zero, we
                    // would subtract from the offset amount each frame.
                    if (m_clipOffset != 0.0f)
                    {
                        m_clipOffset -= leftSize.x - elemSize.GetX() - clipOffsetLeft;
                    }
                }
                else if (cursorClippedLeft)
                {
                    m_clipOffset += rightSize.x - elemSize.GetX() - clipOffsetRight;
                }
            }
        }
        else
        {
            m_clipOffset = 0.0f;
        }
    }

    return m_clipOffset * m_clipOffsetMultiplier;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiTextComponent::PrepareDisplayedTextInternal(PrepareTextOption prepOption, bool suppressXmlWarnings, bool invalidateParentLayout)
{
    CalculateDrawBatchLines(m_drawBatchLines, m_locText, prepOption, suppressXmlWarnings, true, true);

    if (invalidateParentLayout)
    {
        InvalidateLayout();
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiTextComponent::CalculateDrawBatchLines(
    UiTextComponent::DrawBatchLines& drawBatchLinesOut,
    AZStd::string& locTextOut,
    PrepareTextOption prepOption,
    bool suppressXmlWarnings,
    bool useElementWidth,
    bool excludeTrailingSpace
    )
{
    float elementWidth = 0.0f;
    if (useElementWidth)
    {
        // Getting info from the TransformBus could trigger OnCanvasSpaceRectChanged,
        // which would cause this method to be called again. Call this first before
        // we start building our string content! Otherwise drawbatches etc. will end
        // up in a potentially undefined state.
        UiTransformInterface::RectPoints points;
        EBUS_EVENT_ID(GetEntityId(), UiTransformBus, GetCanvasSpacePointsNoScaleRotate, points);
        elementWidth = points.GetAxisAlignedSize().GetX();
    }

    if ((prepOption& PrepareTextOption::Localize) == PrepareTextOption::Localize)
    {
        string locText;
        gEnv->pSystem->GetLocalizationManager()->LocalizeString(m_text.c_str(), locText);
        locTextOut = locText;
    }
    else
    {
        // No localization, just assign contents directly
        locTextOut = m_text;
    }

    // We add localized text to the font texture at Init() also, but if the
    // font changed recently, then the font texture is empty, and won't be
    // populated until the frame renders. If the glyphs aren't mapped to the
    // font texture, then their sizes will be reported as zero/missing, which
    // causes issues with alignment.
    gEnv->pCryFont->AddCharsToFontTextures(m_fontFamily, locTextOut.c_str());

    drawBatchLinesOut.Clear();
    UiTextComponent::DrawBatchContainer drawBatches;
    TextMarkup::Tag markupRoot;

    AZStd::string markupText(locTextOut);

    if ((prepOption& PrepareTextOption::EscapeMarkup) == PrepareTextOption::EscapeMarkup)
    {
        markupText = ::EscapeMarkup(locTextOut.c_str()).c_str();
    }

    if (TextMarkup::ParseMarkupBuffer(markupText, markupRoot, suppressXmlWarnings))
    {
        AZStd::stack<UiTextComponent::DrawBatch> batchStack;
        AZStd::stack<FontFamily*> fontFamilyStack;
        fontFamilyStack.push(m_overrideFontFamily.get());

        BuildDrawBatches(drawBatches, drawBatchLinesOut.fontFamilyRefs, batchStack, fontFamilyStack, &markupRoot);
    }
    else
    {
        drawBatches.push_back(DrawBatch());
        drawBatches.front().font = m_overrideFontFamily->normal;
        drawBatches.front().text = locTextOut;
    }

    SanitizeUserEnteredNewlineChar(locTextOut);

    STextDrawContext fontContext(GetTextDrawContextPrototype());

    if (useElementWidth && m_wrapTextSetting == WrapTextSetting::Wrap)
    {
        BatchAwareWrapText(drawBatchLinesOut, drawBatches, m_fontFamily.get(), fontContext, m_displayedTextFunction, elementWidth);
    }
    else
    {
        CreateBatchLines(drawBatchLinesOut, drawBatches, m_fontFamily.get());
        AssignLineSizes(drawBatchLinesOut, m_fontFamily.get(), fontContext, m_displayedTextFunction, excludeTrailingSpace);
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiTextComponent::RenderDrawBatchLines(
    const AZ::Vector2& pos,
    const UiTransformInterface::RectPoints& points,
    STextDrawContext& fontContext,
    float newlinePosYIncrement)
{
    const ColorB origColor(fontContext.m_colorOverride);

    for (const DrawBatchLine& drawBatchLine : m_drawBatchLines.batchLines)
    {
        float xDrawPosOffset = 0.0f;

        AZ::Vector2 alignedPosition;
        if (m_textHAlignment == IDraw2d::HAlign::Left && m_textVAlignment == IDraw2d::VAlign::Top)
        {
            alignedPosition = pos;
        }
        else
        {
            // we align based on the size of the default font effect, because we do not want the
            // text to move when the font effect is changed
            unsigned int effectIndex = fontContext.m_fxIdx;
            fontContext.SetEffect(0);

            fontContext.SetEffect(effectIndex);

            alignedPosition = CDraw2d::Align(pos, drawBatchLine.lineSize, m_textHAlignment, m_textVAlignment);
        }

        alignedPosition.SetY(alignedPosition.GetY() + newlinePosYIncrement);

        for (const DrawBatch& drawBatch : drawBatchLine.drawBatchList)
        {
            const AZStd::string displayString(m_displayedTextFunction(drawBatch.text));

            if (m_overflowMode == OverflowMode::ClipText)
            {
                fontContext.EnableClipping(true);
                const AZ::Vector2 elemSize(points.GetAxisAlignedSize());

                // Set the clipping rect to be the same size and position of this
                // element's rect.
                fontContext.SetClippingRect(
                    points.TopLeft().GetX(),
                    points.TopLeft().GetY(),
                    elemSize.GetX(),
                    elemSize.GetY());

                alignedPosition.SetX(alignedPosition.GetX() - CalculateHorizontalClipOffset());
            }

            alignedPosition.SetX(alignedPosition.GetX() + xDrawPosOffset);

            Vec2 textSize(drawBatch.font->GetTextSize(displayString.c_str(), true, fontContext));
            xDrawPosOffset = textSize.x;

            const char* profileMarker = "UI_TEXT";
            gEnv->pRenderer->PushProfileMarker(profileMarker);

            const bool drawBatchHasColorAssigned = drawBatch.color != TextMarkup::ColorInvalid;
            if (drawBatchHasColorAssigned)
            {
                const ColorF drawBatchColor(LyShine::MakeColorF(drawBatch.color, 1.0f));
                fontContext.SetColor(drawBatchColor);
            }
            else
            {
                // Since we're re-using the same font context for all draw
                // batches, we need to restore the original color state for
                // batches that don't have a color assigned.
                fontContext.m_colorOverride = origColor;
            }

            drawBatch.font->DrawString(alignedPosition.GetX(), alignedPosition.GetY(), displayString.c_str(), true, fontContext);

            gEnv->pRenderer->PopProfileMarker(profileMarker);
        }

        newlinePosYIncrement += m_fontSize + m_lineSpacing;
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
STextDrawContext UiTextComponent::GetTextDrawContextPrototype() const
{
    STextDrawContext ctx;
    ctx.SetEffect(m_overrideFontEffectIndex);
    ctx.SetSizeIn800x600(false);
    ctx.SetSize(vector2f(m_fontSize, m_fontSize));
    ctx.m_processSpecialChars = false;
    ctx.m_tracking = (m_charSpacing * m_fontSize) / 1000.0f; // m_charSpacing units are 1/1000th of ems, 1 em is equal to font size
    return ctx;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiTextComponent::OnTextWidthPropertyChanged()
{
    if (m_wrapTextSetting == UiTextInterface::WrapTextSetting::NoWrap)
    {
        // Recompute the line sizes so that they are aligned properly (else the sizes will be aligned
        // according to their original width)
        STextDrawContext fontContext(GetTextDrawContextPrototype());
        AssignLineSizes(m_drawBatchLines, m_fontFamily.get(), fontContext, m_displayedTextFunction, true);
    }
    else
    {
        // Recompute even the lines, because since we have new widths, we might have more lines due
        // to word wrap
        PrepareDisplayedTextInternal(PrepareTextOption::Localize, true);
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// PRIVATE STATIC MEMBER FUNCTIONS
////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////
bool UiTextComponent::VersionConverter(AZ::SerializeContext& context,
    AZ::SerializeContext::DataElementNode& classElement)
{
    // conversion from version 1: Need to convert Color to Color and Alpha
    if (classElement.GetVersion() == 1)
    {
        if (!LyShine::ConvertSubElementFromCryStringToAssetRef<LyShine::FontAsset>(context, classElement, "FontFileName"))
        {
            return false;
        }

        if (!LyShine::ConvertSubElementFromColorToColorPlusAlpha(context, classElement, "Color", "Alpha"))
        {
            return false;
        }
    }

    // conversion from version 1 or 2: Need to convert Text from CryString to AzString
    if (classElement.GetVersion() <= 2)
    {
        // Call internal function to work-around serialization of empty AZ std string
        if (!LyShine::ConvertSubElementFromCryStringToAzString(context, classElement, "Text"))
        {
            return false;
        }
    }

    // Versions prior to v4: Change default font
    if (classElement.GetVersion() <= 3)
    {
        if (!ConvertV3FontFileNameIfDefault(context, classElement))
        {
            return false;
        }
    }

    // V4: remove deprecated "supports markup" flag
    if (classElement.GetVersion() == 4)
    {
        if (!RemoveV4MarkupFlag(context, classElement))
        {
            return false;
        }
    }

    // conversion from version 5 to current: Strip off any leading forward slashes from font path
    if (classElement.GetVersion() <= 5)
    {
        if (!LyShine::RemoveLeadingForwardSlashesFromAssetPath(context, classElement, "FontFileName"))
        {
            return false;
        }
    }

    // conversion from version 6 to current: Need to convert ColorF to AZ::Color
    if (classElement.GetVersion() <= 6)
    {
        if (!LyShine::ConvertSubElementFromVector3ToAzColor(context, classElement, "Color"))
        {
            return false;
        }
    }

    return true;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiTextComponent::GetOffsetsFromSelectionInternal(LineOffsets& top, LineOffsets& middle, LineOffsets& bottom, int selectionStart, int selectionEnd)
{
    const int localLastIndex = AZStd::GetMax<int>(selectionStart, selectionEnd);

    STextDrawContext fontContext(GetTextDrawContextPrototype());

    UiTextComponentOffsetsSelector offsetsSelector(
        m_drawBatchLines,
        fontContext,
        m_displayedTextFunction,
        m_fontSize,
        AZStd::GetMin<int>(selectionStart, selectionEnd),
        localLastIndex,
        GetLineNumberFromCharIndex(localLastIndex),
        m_cursorLineNumHint);

    offsetsSelector.CalculateOffsets(top, middle, bottom);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
int UiTextComponent::GetLineNumberFromCharIndex(const int soughtIndex) const
{
    int lineCounter = 0;
    int indexIter = 0;

    // Iterate across the lines of text until soughtIndex is found,
    // incrementing lineCounter along the way and ultimately returning its
    // value when the index is found.
    for (const DrawBatchLine& batchLine : m_drawBatchLines.batchLines)
    {
        lineCounter++;

        for (const DrawBatch& drawBatch : batchLine.drawBatchList)
        {
            Unicode::CIterator<const char*, false> pChar(drawBatch.text.c_str());
            while (uint32_t ch = *pChar)
            {
                ++pChar;
                if (indexIter == soughtIndex)
                {
                    return lineCounter;
                }

                ++indexIter;
            }
        }
    }

    // Note that it's possible for sought index to be one past the end of
    // the line string, in which case we count the soughtIndex as being on
    // that line anyways.
    return lineCounter;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiTextComponent::InvalidateLayout() const
{
    // Invalidate the parent's layout
    AZ::EntityId canvasEntityId;
    EBUS_EVENT_ID_RESULT(canvasEntityId, GetEntityId(), UiElementBus, GetCanvasEntityId);
    EBUS_EVENT_ID(canvasEntityId, UiLayoutManagerBus, MarkToRecomputeLayoutsAffectedByLayoutCellChange, GetEntityId(), true);

    // Invalidate the element's layout
    EBUS_EVENT_ID(canvasEntityId, UiLayoutManagerBus, MarkToRecomputeLayout, GetEntityId());
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiTextComponent::CheckLayoutFitterAndRefreshEditorTransformProperties() const
{
    UiLayoutHelpers::CheckFitterAndRefreshEditorTransformProperties(GetEntityId());
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiTextComponent::AssignSizeReadyFlag()
{
    m_elementSizeReady = true;
    UiUpdateBus::Handler::BusDisconnect();
}

#include "Tests/internal/test_UiTextComponent.cpp"
