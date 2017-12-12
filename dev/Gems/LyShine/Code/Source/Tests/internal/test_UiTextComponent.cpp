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

////////////////////////////////////////////////////////////////////////////////////////////////////
#if defined(LYSHINE_INTERNAL_UNIT_TEST)

#include <LyShine/Bus/UiCanvasBus.h>
#include <regex>

namespace
{
    using FontList = AZStd::list < IFFont * >;

    void AssertTextNotEmpty(const AZStd::list<UiTextComponent::DrawBatch>& drawBatches)
    {
        for (const UiTextComponent::DrawBatch& drawBatch : drawBatches)
        {
            AZ_Assert(!drawBatch.text.empty(), "Test failed");
        }
    }

    void AssertDrawBatchFontOrder(
        const AZStd::list<UiTextComponent::DrawBatch>& drawBatches,
        const FontList& fontList)
    {
        AZ_Assert(drawBatches.size() == fontList.size(), "Test failed");

        auto drawBatchIt = drawBatches.begin();
        auto fontsIt = fontList.begin();
        for (;
            drawBatchIt != drawBatches.end();
            ++drawBatchIt, ++fontsIt)
        {
            const UiTextComponent::DrawBatch& drawBatch = *drawBatchIt;
            const IFFont* font = *fontsIt;
            AZ_Assert(drawBatch.font == font, "Test failed");
        }
    }

    void AssertDrawBatchSingleColor(
        const AZStd::list<UiTextComponent::DrawBatch>& drawBatches,
        const AZ::Vector3& color)
    {
        auto drawBatchIt = drawBatches.begin();
        for (;
            drawBatchIt != drawBatches.end();
            ++drawBatchIt)
        {
            const UiTextComponent::DrawBatch& drawBatch = *drawBatchIt;
            AZ_Assert(drawBatch.color == color, "Test failed");
        }
    }

    using ColorList = AZStd::list < AZ::Vector3 >;

    void AssertDrawBatchMultiColor(
        const AZStd::list<UiTextComponent::DrawBatch>& drawBatches,
        const ColorList& colorList)
    {
        auto drawBatchIt = drawBatches.begin();
        auto colorIt = colorList.begin();
        for (;
            drawBatchIt != drawBatches.end();
            ++drawBatchIt, ++colorIt)
        {
            const UiTextComponent::DrawBatch& drawBatch = *drawBatchIt;
            const AZ::Vector3& color(*colorIt);
            AZ_Assert(drawBatch.color == color, "Test failed");
        }
    }

    using StringList = AZStd::list < LyShine::StringType >;

    void AssertDrawBatchTextContent(
        const AZStd::list<UiTextComponent::DrawBatch>& drawBatches,
        const StringList& stringList)
    {
        AZ_Assert(drawBatches.size() == stringList.size(), "Test failed");

        auto drawBatchIt = drawBatches.begin();
        auto stringIt = stringList.begin();
        for (;
            drawBatchIt != drawBatches.end();
            ++drawBatchIt, ++stringIt)
        {
            const UiTextComponent::DrawBatch& drawBatch = *drawBatchIt;
            const LyShine::StringType& text = *stringIt;
            AZ_Assert(drawBatch.text == text, "Test failed");
        }
    }

    FontFamilyPtr FontFamilyLoad(const char* fontFamilyFilename)
    {
        FontFamilyPtr fontFamily = gEnv->pCryFont->GetFontFamily(fontFamilyFilename);
        if (!fontFamily)
        {
            fontFamily = gEnv->pCryFont->LoadFontFamily(fontFamilyFilename);
            AZ_Assert(gEnv->pCryFont->GetFontFamily(fontFamilyFilename).get(), "Test failed");
        }

        // We need the font family to load correctly in order to test properly
        AZ_Assert(fontFamily.get(), "Test failed");

        return fontFamily;
    }

    //! \brief Verify fonts that ship with Lumberyard load correctly.
    //! 
    //! This test depends on the LyShineExamples and UiBasics gems being
    //! included in the project.
    //!
    //! There are other fonts that ship in other projects (SamplesProject, 
    //! FeatureTests), but that would call for project-specific unit-tests
    //! which don't belong here.
    void VerifyShippingFonts()
    {
        FontFamilyLoad("ui/fonts/lyshineexamples/notosans/notosans.fontfamily");
        FontFamilyLoad("ui/fonts/lyshineexamples/notoserif/notoserif.fontfamily");
        FontFamilyLoad("fonts/vera.fontfamily");
    }

    void NewlineSanitizeTests()
    {
        {
            AZStd::string inputString("Test\\nHi");
            SanitizeUserEnteredNewlineChar(inputString);

            const AZStd::string expectedOutput("Test\nHi");
            AZ_Assert(expectedOutput == inputString, "Test failed");

            // Sanity check that AZStd::regex and std::regex are functionally equivalent.
            {
                const std::string NewlineDelimiter("\n");
                const std::regex UserInputNewlineDelimiter("\\\\n");
                std::string inputStringCopy("Test\\nHi");
                inputStringCopy = std::regex_replace(inputStringCopy, UserInputNewlineDelimiter, NewlineDelimiter);
                AZ_Assert(inputStringCopy == std::string(inputString.c_str()), "Test failed");
                AZ_Assert(AZStd::string(inputStringCopy.c_str()) == inputString, "Test failed");
            }
        }
        
    }

    void MarkupEscapingTests()
    {
        // One char un-escaping
        {
            const AZStd::string escaped("&amp;");
            const AZStd::string unescaped("&");
            AZ_Assert(UnescapeMarkup(escaped) == unescaped, "Test failed");
            AZ_Assert(EscapeMarkup(unescaped) == escaped, "Test failed");
        }
        {
            const AZStd::string escaped("&lt;");
            const AZStd::string unescaped("<");
            AZ_Assert(UnescapeMarkup(escaped) == unescaped, "Test failed");
            AZ_Assert(EscapeMarkup(unescaped) == escaped, "Test failed");
        }
        {
            const AZStd::string escaped("&gt;");
            const AZStd::string unescaped(">");
            AZ_Assert(UnescapeMarkup(escaped) == unescaped, "Test failed");
            AZ_Assert(EscapeMarkup(unescaped) == escaped, "Test failed");
        }
        {
            const AZStd::string escaped("&#37;");
            const AZStd::string unescaped("%");
            AZ_Assert(UnescapeMarkup(escaped) == unescaped, "Test failed");
            AZ_Assert(EscapeMarkup(unescaped) == escaped, "Test failed");
        }

        // Multi-char unescaping
        {
            const AZStd::string escaped("&amp;&lt;&gt;&#37;");
            const AZStd::string unescaped("&<>%");
            AZ_Assert(UnescapeMarkup(escaped) == unescaped, "Test failed");
            AZ_Assert(EscapeMarkup(unescaped) == escaped, "Test failed");
        }
        {
            const AZStd::string escaped("&lt;&amp;&gt;&#37;");
            const AZStd::string unescaped("<&>%");
            AZ_Assert(UnescapeMarkup(escaped) == unescaped, "Test failed");
            AZ_Assert(EscapeMarkup(unescaped) == escaped, "Test failed");
        }
        {
            const AZStd::string escaped("&gt;&lt;&amp;&#37;");
            const AZStd::string unescaped("><&%");
            AZ_Assert(UnescapeMarkup(escaped) == unescaped, "Test failed");
            AZ_Assert(EscapeMarkup(unescaped) == escaped, "Test failed");
        }
        {
            const AZStd::string escaped("&#37;&gt;&lt;&amp;");
            const AZStd::string unescaped("%><&");
            AZ_Assert(UnescapeMarkup(escaped) == unescaped, "Test failed");
            AZ_Assert(EscapeMarkup(unescaped) == escaped, "Test failed");
        }
        {
            const AZStd::string escaped("&amp; &lt; &gt; &#37;");
            const AZStd::string unescaped("& < > %");
            AZ_Assert(UnescapeMarkup(escaped) == unescaped, "Test failed");
            AZ_Assert(EscapeMarkup(unescaped) == escaped, "Test failed");
        }
        {
            const AZStd::string escaped("&lt; &amp; &gt; &#37;");
            const AZStd::string unescaped("< & > %");
            AZ_Assert(UnescapeMarkup(escaped) == unescaped, "Test failed");
            AZ_Assert(EscapeMarkup(unescaped) == escaped, "Test failed");
        }
        {
            const AZStd::string escaped("&gt; &lt; &amp; &#37;");
            const AZStd::string unescaped("> < & %");
            AZ_Assert(UnescapeMarkup(escaped) == unescaped, "Test failed");
            AZ_Assert(EscapeMarkup(unescaped) == escaped, "Test failed");
        }
        {
            const AZStd::string escaped("&#37; &gt; &lt; &amp;");
            const AZStd::string unescaped("% > < &");
            AZ_Assert(UnescapeMarkup(escaped) == unescaped, "Test failed");
            AZ_Assert(EscapeMarkup(unescaped) == escaped, "Test failed");
        }

        // Unescaping markup that results in escaped markup
        {
            const AZStd::string escaped("&amp;amp;");
            const AZStd::string unescaped("&amp;");
            AZ_Assert(UnescapeMarkup(escaped) == unescaped, "Test failed");
            AZ_Assert(EscapeMarkup(unescaped) == escaped, "Test failed");
        }
        {
            const AZStd::string escaped("&amp;lt;");
            const AZStd::string unescaped("&lt;");
            AZ_Assert(UnescapeMarkup(escaped) == unescaped, "Test failed");
            AZ_Assert(EscapeMarkup(unescaped) == escaped, "Test failed");
        }
        {
            const AZStd::string escaped("&amp;gt;");
            const AZStd::string unescaped("&gt;");
            AZ_Assert(UnescapeMarkup(escaped) == unescaped, "Test failed");
            AZ_Assert(EscapeMarkup(unescaped) == escaped, "Test failed");
        }
        {
            const AZStd::string escaped("&amp;#37;");
            const AZStd::string unescaped("&#37;");
            AZ_Assert(UnescapeMarkup(escaped) == unescaped, "Test failed");
            AZ_Assert(EscapeMarkup(unescaped) == escaped, "Test failed");
        }
    }

    void BuildDrawBatchesTests(FontFamily* fontFamily)
    {
        // Plain string
        {
            const LyShine::StringType markupTestString("this is a test!");

            {
                TextMarkup::Tag markupRoot;
                AZ_Assert(TextMarkup::ParseMarkupBuffer(markupTestString, markupRoot), "Test failed");
                AZStd::list<UiTextComponent::DrawBatch> drawBatches;
                AZStd::stack<UiTextComponent::DrawBatch> batchStack;

                AZStd::stack<FontFamily*> fontFamilyStack;
                fontFamilyStack.push(fontFamily);

                UiTextComponent::FontFamilyRefSet fontFamilyRefs;
                BuildDrawBatches(drawBatches, fontFamilyRefs, batchStack, fontFamilyStack, &markupRoot);
                AZ_Assert(0 == fontFamilyRefs.size(), "Test failed");
                AZ_Assert(1 == drawBatches.size(), "Test failed");
                AssertTextNotEmpty(drawBatches);

                FontList fontList;
                fontList.push_back(fontFamily->normal);
                AssertDrawBatchFontOrder(drawBatches, fontList);

                AssertDrawBatchSingleColor(drawBatches, TextMarkup::ColorInvalid);

            }
        }

        // Plain string: newline
        {
            const LyShine::StringType markupTestString("Regular Bold Italic\n");

            {
                TextMarkup::Tag markupRoot;
                AZ_Assert(TextMarkup::ParseMarkupBuffer(markupTestString, markupRoot), "Test failed");

                AZStd::list<UiTextComponent::DrawBatch> drawBatches;
                AZStd::stack<UiTextComponent::DrawBatch> batchStack;

                AZStd::stack<FontFamily*> fontFamilyStack;
                fontFamilyStack.push(fontFamily);

                UiTextComponent::FontFamilyRefSet fontFamilyRefs;
                BuildDrawBatches(drawBatches, fontFamilyRefs, batchStack, fontFamilyStack, &markupRoot);
                AZ_Assert(0 == fontFamilyRefs.size(), "Test failed");
                AZ_Assert(1 == drawBatches.size(), "Test failed");
                AssertTextNotEmpty(drawBatches);

                FontList fontList;
                fontList.push_back(fontFamily->normal);
                AssertDrawBatchFontOrder(drawBatches, fontList);

                AssertDrawBatchSingleColor(drawBatches, TextMarkup::ColorInvalid);

            }
        }

        // Single bold
        {
            const LyShine::StringType markupTestString("<b>this</b> is a test!");

            {
                TextMarkup::Tag markupRoot;
                AZ_Assert(TextMarkup::ParseMarkupBuffer(markupTestString, markupRoot), "Test failed");
                AZStd::list<UiTextComponent::DrawBatch> drawBatches;
                AZStd::stack<UiTextComponent::DrawBatch> batchStack;

                AZStd::stack<FontFamily*> fontFamilyStack;
                fontFamilyStack.push(fontFamily);

                UiTextComponent::FontFamilyRefSet fontFamilyRefs;
                BuildDrawBatches(drawBatches, fontFamilyRefs, batchStack, fontFamilyStack, &markupRoot);
                AZ_Assert(0 == fontFamilyRefs.size(), "Test failed");
                AZ_Assert(2 == drawBatches.size(), "Test failed");
                AssertTextNotEmpty(drawBatches);

                FontList fontList;
                fontList.push_back(fontFamily->bold);
                fontList.push_back(fontFamily->normal);
                AssertDrawBatchFontOrder(drawBatches, fontList);

                AssertDrawBatchSingleColor(drawBatches, TextMarkup::ColorInvalid);
            }
        }

        // Single italic
        {
            const LyShine::StringType markupTestString("<i>this</i> is a test!");

            {
                TextMarkup::Tag markupRoot;
                AZ_Assert(TextMarkup::ParseMarkupBuffer(markupTestString, markupRoot), "Test failed");
                AZStd::list<UiTextComponent::DrawBatch> drawBatches;
                AZStd::stack<UiTextComponent::DrawBatch> batchStack;

                AZStd::stack<FontFamily*> fontFamilyStack;
                fontFamilyStack.push(fontFamily);

                UiTextComponent::FontFamilyRefSet fontFamilyRefs;
                BuildDrawBatches(drawBatches, fontFamilyRefs, batchStack, fontFamilyStack, &markupRoot);
                AZ_Assert(0 == fontFamilyRefs.size(), "Test failed");
                AZ_Assert(2 == drawBatches.size(), "Test failed");
                AssertTextNotEmpty(drawBatches);

                FontList fontList;
                fontList.push_back(fontFamily->italic);
                fontList.push_back(fontFamily->normal);
                AssertDrawBatchFontOrder(drawBatches, fontList);

                AssertDrawBatchSingleColor(drawBatches, TextMarkup::ColorInvalid);
            }
        }

        // Bold-italic
        {
            const LyShine::StringType markupTestString("<b><i>this</i></b> is a test!");

            {
                TextMarkup::Tag markupRoot;
                AZ_Assert(TextMarkup::ParseMarkupBuffer(markupTestString, markupRoot), "Test failed");
                AZStd::list<UiTextComponent::DrawBatch> drawBatches;
                AZStd::stack<UiTextComponent::DrawBatch> batchStack;

                AZStd::stack<FontFamily*> fontFamilyStack;
                fontFamilyStack.push(fontFamily);

                UiTextComponent::FontFamilyRefSet fontFamilyRefs;
                BuildDrawBatches(drawBatches, fontFamilyRefs, batchStack, fontFamilyStack, &markupRoot);
                AZ_Assert(0 == fontFamilyRefs.size(), "Test failed");
                AZ_Assert(2 == drawBatches.size(), "Test failed");
                AssertTextNotEmpty(drawBatches);

                FontList fontList;
                fontList.push_back(fontFamily->boldItalic);
                fontList.push_back(fontFamily->normal);
                AssertDrawBatchFontOrder(drawBatches, fontList);

                AssertDrawBatchSingleColor(drawBatches, TextMarkup::ColorInvalid);
            }
        }

        // Font tag: font face
        {
            const LyShine::StringType markupTestString("<font face=\"notoserif\">this</font> is a test!");

            {
                TextMarkup::Tag markupRoot;
                AZ_Assert(TextMarkup::ParseMarkupBuffer(markupTestString, markupRoot), "Test failed");
                AZStd::list<UiTextComponent::DrawBatch> drawBatches;
                AZStd::stack<UiTextComponent::DrawBatch> batchStack;

                AZStd::stack<FontFamily*> fontFamilyStack;
                fontFamilyStack.push(fontFamily);

                UiTextComponent::FontFamilyRefSet fontFamilyRefs;
                BuildDrawBatches(drawBatches, fontFamilyRefs, batchStack, fontFamilyStack, &markupRoot);
                AZ_Assert(1 == fontFamilyRefs.size(), "Test failed");
                AZ_Assert(2 == drawBatches.size(), "Test failed");
                AssertTextNotEmpty(drawBatches);

                StringList stringList;
                stringList.push_back("this");
                stringList.push_back(" is a test!");
                AssertDrawBatchTextContent(drawBatches, stringList);

                FontFamilyPtr notoSerifFamily = gEnv->pCryFont->GetFontFamily("notoserif");
                AZ_Assert(notoSerifFamily, "Test failed");

                FontList fontList;
                fontList.push_back(notoSerifFamily->normal);
                fontList.push_back(fontFamily->normal);
                AssertDrawBatchFontOrder(drawBatches, fontList);

                ColorList colorList;
                colorList.push_back(TextMarkup::ColorInvalid);
                colorList.push_back(TextMarkup::ColorInvalid);
                AssertDrawBatchMultiColor(drawBatches, colorList);
            }
        }

        // Font tag: font face (different font)
        {
            const LyShine::StringType markupTestString("<font face=\"notosans\">this</font> is a test!");

            {
                TextMarkup::Tag markupRoot;
                AZ_Assert(TextMarkup::ParseMarkupBuffer(markupTestString, markupRoot), "Test failed");
                AZStd::list<UiTextComponent::DrawBatch> drawBatches;
                AZStd::stack<UiTextComponent::DrawBatch> batchStack;

                AZStd::stack<FontFamily*> fontFamilyStack;
                fontFamilyStack.push(fontFamily);

                UiTextComponent::FontFamilyRefSet fontFamilyRefs;
                BuildDrawBatches(drawBatches, fontFamilyRefs, batchStack, fontFamilyStack, &markupRoot);
                AZ_Assert(1 == fontFamilyRefs.size(), "Test failed", "Test failed");
                AZ_Assert(2 == drawBatches.size(), "Test failed", "Test failed");
                AssertTextNotEmpty(drawBatches);

                StringList stringList;
                stringList.push_back("this");
                stringList.push_back(" is a test!");
                AssertDrawBatchTextContent(drawBatches, stringList);

                FontFamilyPtr notoSansFamily = gEnv->pCryFont->GetFontFamily("notosans");
                AZ_Assert(notoSansFamily, "Test failed");

                FontList fontList;
                fontList.push_back(notoSansFamily->normal);
                fontList.push_back(fontFamily->normal);
                AssertDrawBatchFontOrder(drawBatches, fontList);

                ColorList colorList;
                colorList.push_back(TextMarkup::ColorInvalid);
                colorList.push_back(TextMarkup::ColorInvalid);
                AssertDrawBatchMultiColor(drawBatches, colorList);
            }
        }

        // Font tag: font face (leading space)
        {
            const LyShine::StringType markupTestString("<font face=\"   notosans\">this</font> is a test!");

            {
                TextMarkup::Tag markupRoot;
                AZ_Assert(TextMarkup::ParseMarkupBuffer(markupTestString, markupRoot), "Test failed");
                AZStd::list<UiTextComponent::DrawBatch> drawBatches;
                AZStd::stack<UiTextComponent::DrawBatch> batchStack;

                AZStd::stack<FontFamily*> fontFamilyStack;
                fontFamilyStack.push(fontFamily);

                UiTextComponent::FontFamilyRefSet fontFamilyRefs;
                BuildDrawBatches(drawBatches, fontFamilyRefs, batchStack, fontFamilyStack, &markupRoot);
                AZ_Assert(1 == fontFamilyRefs.size(), "Test failed");
                AZ_Assert(2 == drawBatches.size(), "Test failed");
                AssertTextNotEmpty(drawBatches);

                StringList stringList;
                stringList.push_back("this");
                stringList.push_back(" is a test!");
                AssertDrawBatchTextContent(drawBatches, stringList);

                FontFamilyPtr notoSansFamily = gEnv->pCryFont->GetFontFamily("notosans");
                AZ_Assert(notoSansFamily, "Test failed");

                FontList fontList;
                fontList.push_back(notoSansFamily->normal);
                fontList.push_back(fontFamily->normal);
                AssertDrawBatchFontOrder(drawBatches, fontList);

                ColorList colorList;
                colorList.push_back(TextMarkup::ColorInvalid);
                colorList.push_back(TextMarkup::ColorInvalid);
                AssertDrawBatchMultiColor(drawBatches, colorList);
            }
        }

        // Font tag: font face (trailing space)
        {
            const LyShine::StringType markupTestString("<font face=\"notosans   \">this</font> is a test!");

            {
                TextMarkup::Tag markupRoot;
                AZ_Assert(TextMarkup::ParseMarkupBuffer(markupTestString, markupRoot), "Test failed");
                AZStd::list<UiTextComponent::DrawBatch> drawBatches;
                AZStd::stack<UiTextComponent::DrawBatch> batchStack;

                AZStd::stack<FontFamily*> fontFamilyStack;
                fontFamilyStack.push(fontFamily);

                UiTextComponent::FontFamilyRefSet fontFamilyRefs;
                BuildDrawBatches(drawBatches, fontFamilyRefs, batchStack, fontFamilyStack, &markupRoot);
                AZ_Assert(1 == fontFamilyRefs.size(), "Test failed");
                AZ_Assert(2 == drawBatches.size(), "Test failed");
                AssertTextNotEmpty(drawBatches);

                StringList stringList;
                stringList.push_back("this");
                stringList.push_back(" is a test!");
                AssertDrawBatchTextContent(drawBatches, stringList);

                FontFamilyPtr notoSansFamily = gEnv->pCryFont->GetFontFamily("notosans");
                AZ_Assert(notoSansFamily, "Test failed");

                FontList fontList;
                fontList.push_back(notoSansFamily->normal);
                fontList.push_back(fontFamily->normal);
                AssertDrawBatchFontOrder(drawBatches, fontList);

                ColorList colorList;
                colorList.push_back(TextMarkup::ColorInvalid);
                colorList.push_back(TextMarkup::ColorInvalid);
                AssertDrawBatchMultiColor(drawBatches, colorList);
            }
        }

        // Font tag: font face (leading and trailing space)
        {
            const LyShine::StringType markupTestString("<font face=\"    notosans   \">this</font> is a test!");

            {
                TextMarkup::Tag markupRoot;
                AZ_Assert(TextMarkup::ParseMarkupBuffer(markupTestString, markupRoot), "Test failed");
                AZStd::list<UiTextComponent::DrawBatch> drawBatches;
                AZStd::stack<UiTextComponent::DrawBatch> batchStack;

                AZStd::stack<FontFamily*> fontFamilyStack;
                fontFamilyStack.push(fontFamily);

                UiTextComponent::FontFamilyRefSet fontFamilyRefs;
                BuildDrawBatches(drawBatches, fontFamilyRefs, batchStack, fontFamilyStack, &markupRoot);
                AZ_Assert(1 == fontFamilyRefs.size(), "Test failed");
                AZ_Assert(2 == drawBatches.size(), "Test failed");
                AssertTextNotEmpty(drawBatches);

                StringList stringList;
                stringList.push_back("this");
                stringList.push_back(" is a test!");
                AssertDrawBatchTextContent(drawBatches, stringList);

                FontFamilyPtr notoSansFamily = gEnv->pCryFont->GetFontFamily("notosans");
                AZ_Assert(notoSansFamily, "Test failed");

                FontList fontList;
                fontList.push_back(notoSansFamily->normal);
                fontList.push_back(fontFamily->normal);
                AssertDrawBatchFontOrder(drawBatches, fontList);

                ColorList colorList;
                colorList.push_back(TextMarkup::ColorInvalid);
                colorList.push_back(TextMarkup::ColorInvalid);
                AssertDrawBatchMultiColor(drawBatches, colorList);
            }
        }

        // Font tag: font face ("pass-through" font)
        {
            const LyShine::StringType markupTestString("<font face=\"default-ui\">this</font> is a test!");

            {
                TextMarkup::Tag markupRoot;
                AZ_Assert(TextMarkup::ParseMarkupBuffer(markupTestString, markupRoot), "Test failed");
                AZStd::list<UiTextComponent::DrawBatch> drawBatches;
                AZStd::stack<UiTextComponent::DrawBatch> batchStack;

                AZStd::stack<FontFamily*> fontFamilyStack;
                fontFamilyStack.push(fontFamily);

                UiTextComponent::FontFamilyRefSet fontFamilyRefs;
                BuildDrawBatches(drawBatches, fontFamilyRefs, batchStack, fontFamilyStack, &markupRoot);
                AZ_Assert(1 == fontFamilyRefs.size(), "Test failed");
                AZ_Assert(2 == drawBatches.size(), "Test failed");
                AssertTextNotEmpty(drawBatches);

                StringList stringList;
                stringList.push_back("this");
                stringList.push_back(" is a test!");
                AssertDrawBatchTextContent(drawBatches, stringList);

                FontFamilyPtr defaultUiFamily = gEnv->pCryFont->GetFontFamily("default-ui");
                AZ_Assert(defaultUiFamily, "Test failed");

                FontList fontList;
                fontList.push_back(defaultUiFamily->normal);
                fontList.push_back(fontFamily->normal);
                AssertDrawBatchFontOrder(drawBatches, fontList);

                ColorList colorList;
                colorList.push_back(TextMarkup::ColorInvalid);
                colorList.push_back(TextMarkup::ColorInvalid);
                AssertDrawBatchMultiColor(drawBatches, colorList);
            }
        }

        // Font tag: font face (invalid font)
        {
            const LyShine::StringType markupTestString("<font face=\"invalidFontName\">this</font> is a test!");

            {
                TextMarkup::Tag markupRoot;
                AZ_Assert(TextMarkup::ParseMarkupBuffer(markupTestString, markupRoot), "Test failed");
                AZStd::list<UiTextComponent::DrawBatch> drawBatches;
                AZStd::stack<UiTextComponent::DrawBatch> batchStack;

                AZStd::stack<FontFamily*> fontFamilyStack;
                fontFamilyStack.push(fontFamily);

                UiTextComponent::FontFamilyRefSet fontFamilyRefs;
                BuildDrawBatches(drawBatches, fontFamilyRefs, batchStack, fontFamilyStack, &markupRoot);
                AZ_Assert(0 == fontFamilyRefs.size(), "Test failed");
                AZ_Assert(2 == drawBatches.size(), "Test failed");
                AssertTextNotEmpty(drawBatches);

                StringList stringList;
                stringList.push_back("this");
                stringList.push_back(" is a test!");
                AssertDrawBatchTextContent(drawBatches, stringList);

                FontList fontList;
                fontList.push_back(fontFamily->normal);
                fontList.push_back(fontFamily->normal);
                AssertDrawBatchFontOrder(drawBatches, fontList);

                ColorList colorList;
                colorList.push_back(TextMarkup::ColorInvalid);
                colorList.push_back(TextMarkup::ColorInvalid);
                AssertDrawBatchMultiColor(drawBatches, colorList);
            }
        }

        // Font tag: font face (invalid empty string)
        {
            const LyShine::StringType markupTestString("<font face=\"\">this</font> is a test!");

            {
                TextMarkup::Tag markupRoot;
                AZ_Assert(TextMarkup::ParseMarkupBuffer(markupTestString, markupRoot), "Test failed");
                AZStd::list<UiTextComponent::DrawBatch> drawBatches;
                AZStd::stack<UiTextComponent::DrawBatch> batchStack;

                AZStd::stack<FontFamily*> fontFamilyStack;
                fontFamilyStack.push(fontFamily);

                UiTextComponent::FontFamilyRefSet fontFamilyRefs;
                BuildDrawBatches(drawBatches, fontFamilyRefs, batchStack, fontFamilyStack, &markupRoot);
                AZ_Assert(0 == fontFamilyRefs.size(), "Test failed");
                AZ_Assert(2 == drawBatches.size(), "Test failed");
                AssertTextNotEmpty(drawBatches);

                StringList stringList;
                stringList.push_back("this");
                stringList.push_back(" is a test!");
                AssertDrawBatchTextContent(drawBatches, stringList);

                FontList fontList;
                fontList.push_back(fontFamily->normal);
                fontList.push_back(fontFamily->normal);
                AssertDrawBatchFontOrder(drawBatches, fontList);

                ColorList colorList;
                colorList.push_back(TextMarkup::ColorInvalid);
                colorList.push_back(TextMarkup::ColorInvalid);
                AssertDrawBatchMultiColor(drawBatches, colorList);
            }
        }

        // Font tag: font color (red, lower case)
        {
            const LyShine::StringType markupTestString("<font color=\"#ff0000\">this</font> is a test!");

            {
                TextMarkup::Tag markupRoot;
                AZ_Assert(TextMarkup::ParseMarkupBuffer(markupTestString, markupRoot), "Test failed");
                AZStd::list<UiTextComponent::DrawBatch> drawBatches;
                AZStd::stack<UiTextComponent::DrawBatch> batchStack;

                AZStd::stack<FontFamily*> fontFamilyStack;
                fontFamilyStack.push(fontFamily);

                UiTextComponent::FontFamilyRefSet fontFamilyRefs;
                BuildDrawBatches(drawBatches, fontFamilyRefs, batchStack, fontFamilyStack, &markupRoot);
                AZ_Assert(0 == fontFamilyRefs.size(), "Test failed");
                AZ_Assert(2 == drawBatches.size(), "Test failed");
                AssertTextNotEmpty(drawBatches);

                StringList stringList;
                stringList.push_back("this");
                stringList.push_back(" is a test!");
                AssertDrawBatchTextContent(drawBatches, stringList);

                FontList fontList;
                fontList.push_back(fontFamily->normal);
                fontList.push_back(fontFamily->normal);
                AssertDrawBatchFontOrder(drawBatches, fontList);

                ColorList colorList;
                colorList.push_back(AZ::Vector3(1.0f, 0.0f, 0.0f));
                colorList.push_back(TextMarkup::ColorInvalid);
                AssertDrawBatchMultiColor(drawBatches, colorList);
            }
        }

        // Font tag: font color (red, upper case)
        {
            const LyShine::StringType markupTestString("<font color=\"#FF0000\">this</font> is a test!");

            {
                TextMarkup::Tag markupRoot;
                AZ_Assert(TextMarkup::ParseMarkupBuffer(markupTestString, markupRoot), "Test failed");
                AZStd::list<UiTextComponent::DrawBatch> drawBatches;
                AZStd::stack<UiTextComponent::DrawBatch> batchStack;

                AZStd::stack<FontFamily*> fontFamilyStack;
                fontFamilyStack.push(fontFamily);

                UiTextComponent::FontFamilyRefSet fontFamilyRefs;
                BuildDrawBatches(drawBatches, fontFamilyRefs, batchStack, fontFamilyStack, &markupRoot);
                AZ_Assert(0 == fontFamilyRefs.size(), "Test failed");
                AZ_Assert(2 == drawBatches.size(), "Test failed");
                AssertTextNotEmpty(drawBatches);

                StringList stringList;
                stringList.push_back("this");
                stringList.push_back(" is a test!");
                AssertDrawBatchTextContent(drawBatches, stringList);

                FontList fontList;
                fontList.push_back(fontFamily->normal);
                fontList.push_back(fontFamily->normal);
                AssertDrawBatchFontOrder(drawBatches, fontList);

                ColorList colorList;
                colorList.push_back(AZ::Vector3(1.0f, 0.0f, 0.0f));
                colorList.push_back(TextMarkup::ColorInvalid);
                AssertDrawBatchMultiColor(drawBatches, colorList);
            }
        }

        // Font tag: font color (red, mixed case) 1
        {
            const LyShine::StringType markupTestString("<font color=\"#fF0000\">this</font> is a test!");

            {
                TextMarkup::Tag markupRoot;
                AZ_Assert(TextMarkup::ParseMarkupBuffer(markupTestString, markupRoot), "Test failed");
                AZStd::list<UiTextComponent::DrawBatch> drawBatches;
                AZStd::stack<UiTextComponent::DrawBatch> batchStack;

                AZStd::stack<FontFamily*> fontFamilyStack;
                fontFamilyStack.push(fontFamily);

                UiTextComponent::FontFamilyRefSet fontFamilyRefs;
                BuildDrawBatches(drawBatches, fontFamilyRefs, batchStack, fontFamilyStack, &markupRoot);
                AZ_Assert(0 == fontFamilyRefs.size(), "Test failed");
                AZ_Assert(2 == drawBatches.size(), "Test failed");
                AssertTextNotEmpty(drawBatches);

                StringList stringList;
                stringList.push_back("this");
                stringList.push_back(" is a test!");
                AssertDrawBatchTextContent(drawBatches, stringList);

                FontList fontList;
                fontList.push_back(fontFamily->normal);
                fontList.push_back(fontFamily->normal);
                AssertDrawBatchFontOrder(drawBatches, fontList);

                ColorList colorList;
                colorList.push_back(AZ::Vector3(1.0f, 0.0f, 0.0f));
                colorList.push_back(TextMarkup::ColorInvalid);
                AssertDrawBatchMultiColor(drawBatches, colorList);
            }
        }

        // Font tag: font color (red, mixed case) 2
        {
            const LyShine::StringType markupTestString("<font color=\"#Ff0000\">this</font> is a test!");

            {
                TextMarkup::Tag markupRoot;
                AZ_Assert(TextMarkup::ParseMarkupBuffer(markupTestString, markupRoot), "Test failed");
                AZStd::list<UiTextComponent::DrawBatch> drawBatches;
                AZStd::stack<UiTextComponent::DrawBatch> batchStack;

                AZStd::stack<FontFamily*> fontFamilyStack;
                fontFamilyStack.push(fontFamily);

                UiTextComponent::FontFamilyRefSet fontFamilyRefs;
                BuildDrawBatches(drawBatches, fontFamilyRefs, batchStack, fontFamilyStack, &markupRoot);
                AZ_Assert(0 == fontFamilyRefs.size(), "Test failed");
                AZ_Assert(2 == drawBatches.size(), "Test failed");
                AssertTextNotEmpty(drawBatches);

                StringList stringList;
                stringList.push_back("this");
                stringList.push_back(" is a test!");
                AssertDrawBatchTextContent(drawBatches, stringList);

                FontList fontList;
                fontList.push_back(fontFamily->normal);
                fontList.push_back(fontFamily->normal);
                AssertDrawBatchFontOrder(drawBatches, fontList);

                ColorList colorList;
                colorList.push_back(AZ::Vector3(1.0f, 0.0f, 0.0f));
                colorList.push_back(TextMarkup::ColorInvalid);
                AssertDrawBatchMultiColor(drawBatches, colorList);
            }
        }

        // Font tag: font color (red, upper case, leading space)
        {
            const LyShine::StringType markupTestString("<font color=\"   #FF0000\">this</font> is a test!");

            {
                TextMarkup::Tag markupRoot;
                AZ_Assert(TextMarkup::ParseMarkupBuffer(markupTestString, markupRoot), "Test failed");
                AZStd::list<UiTextComponent::DrawBatch> drawBatches;
                AZStd::stack<UiTextComponent::DrawBatch> batchStack;

                AZStd::stack<FontFamily*> fontFamilyStack;
                fontFamilyStack.push(fontFamily);

                UiTextComponent::FontFamilyRefSet fontFamilyRefs;
                BuildDrawBatches(drawBatches, fontFamilyRefs, batchStack, fontFamilyStack, &markupRoot);
                AZ_Assert(0 == fontFamilyRefs.size(), "Test failed");
                AZ_Assert(2 == drawBatches.size(), "Test failed");
                AssertTextNotEmpty(drawBatches);

                StringList stringList;
                stringList.push_back("this");
                stringList.push_back(" is a test!");
                AssertDrawBatchTextContent(drawBatches, stringList);

                FontList fontList;
                fontList.push_back(fontFamily->normal);
                fontList.push_back(fontFamily->normal);
                AssertDrawBatchFontOrder(drawBatches, fontList);

                ColorList colorList;
                colorList.push_back(AZ::Vector3(1.0f, 0.0f, 0.0f));
                colorList.push_back(TextMarkup::ColorInvalid);
                AssertDrawBatchMultiColor(drawBatches, colorList);
            }
        }

        // Font tag: font color (red, upper case, trailing space)
        {
            const LyShine::StringType markupTestString("<font color=\"#FF0000   \">this</font> is a test!");

            {
                TextMarkup::Tag markupRoot;
                AZ_Assert(TextMarkup::ParseMarkupBuffer(markupTestString, markupRoot), "Test failed");
                AZStd::list<UiTextComponent::DrawBatch> drawBatches;
                AZStd::stack<UiTextComponent::DrawBatch> batchStack;

                AZStd::stack<FontFamily*> fontFamilyStack;
                fontFamilyStack.push(fontFamily);

                UiTextComponent::FontFamilyRefSet fontFamilyRefs;
                BuildDrawBatches(drawBatches, fontFamilyRefs, batchStack, fontFamilyStack, &markupRoot);
                AZ_Assert(0 == fontFamilyRefs.size(), "Test failed");
                AZ_Assert(2 == drawBatches.size(), "Test failed");
                AssertTextNotEmpty(drawBatches);

                StringList stringList;
                stringList.push_back("this");
                stringList.push_back(" is a test!");
                AssertDrawBatchTextContent(drawBatches, stringList);

                FontList fontList;
                fontList.push_back(fontFamily->normal);
                fontList.push_back(fontFamily->normal);
                AssertDrawBatchFontOrder(drawBatches, fontList);

                ColorList colorList;
                colorList.push_back(AZ::Vector3(1.0f, 0.0f, 0.0f));
                colorList.push_back(TextMarkup::ColorInvalid);
                AssertDrawBatchMultiColor(drawBatches, colorList);
            }
        }

        // Font tag: font color (red, upper case, leading and trailing space)
        {
            const LyShine::StringType markupTestString("<font color=\"   #FF0000   \">this</font> is a test!");

            {
                TextMarkup::Tag markupRoot;
                AZ_Assert(TextMarkup::ParseMarkupBuffer(markupTestString, markupRoot), "Test failed");
                AZStd::list<UiTextComponent::DrawBatch> drawBatches;
                AZStd::stack<UiTextComponent::DrawBatch> batchStack;

                AZStd::stack<FontFamily*> fontFamilyStack;
                fontFamilyStack.push(fontFamily);

                UiTextComponent::FontFamilyRefSet fontFamilyRefs;
                BuildDrawBatches(drawBatches, fontFamilyRefs, batchStack, fontFamilyStack, &markupRoot);
                AZ_Assert(0 == fontFamilyRefs.size(), "Test failed");
                AZ_Assert(2 == drawBatches.size(), "Test failed");
                AssertTextNotEmpty(drawBatches);

                StringList stringList;
                stringList.push_back("this");
                stringList.push_back(" is a test!");
                AssertDrawBatchTextContent(drawBatches, stringList);

                FontList fontList;
                fontList.push_back(fontFamily->normal);
                fontList.push_back(fontFamily->normal);
                AssertDrawBatchFontOrder(drawBatches, fontList);

                ColorList colorList;
                colorList.push_back(AZ::Vector3(1.0f, 0.0f, 0.0f));
                colorList.push_back(TextMarkup::ColorInvalid);
                AssertDrawBatchMultiColor(drawBatches, colorList);
            }
        }

        // Font tag: font color (green, upper case)
        {
            const LyShine::StringType markupTestString("<font color=\"#00FF00\">this</font> is a test!");

            {
                TextMarkup::Tag markupRoot;
                AZ_Assert(TextMarkup::ParseMarkupBuffer(markupTestString, markupRoot), "Test failed");
                AZStd::list<UiTextComponent::DrawBatch> drawBatches;
                AZStd::stack<UiTextComponent::DrawBatch> batchStack;

                AZStd::stack<FontFamily*> fontFamilyStack;
                fontFamilyStack.push(fontFamily);

                UiTextComponent::FontFamilyRefSet fontFamilyRefs;
                BuildDrawBatches(drawBatches, fontFamilyRefs, batchStack, fontFamilyStack, &markupRoot);
                AZ_Assert(0 == fontFamilyRefs.size(), "Test failed");
                AZ_Assert(2 == drawBatches.size(), "Test failed");
                AssertTextNotEmpty(drawBatches);

                StringList stringList;
                stringList.push_back("this");
                stringList.push_back(" is a test!");
                AssertDrawBatchTextContent(drawBatches, stringList);

                FontList fontList;
                fontList.push_back(fontFamily->normal);
                fontList.push_back(fontFamily->normal);
                AssertDrawBatchFontOrder(drawBatches, fontList);

                ColorList colorList;
                colorList.push_back(AZ::Vector3(0.0f, 1.0f, 0.0f));
                colorList.push_back(TextMarkup::ColorInvalid);
                AssertDrawBatchMultiColor(drawBatches, colorList);
            }
        }

        // Font tag: font color (blue, upper case)
        {
            const LyShine::StringType markupTestString("<font color=\"#0000FF\">this</font> is a test!");

            {
                TextMarkup::Tag markupRoot;
                AZ_Assert(TextMarkup::ParseMarkupBuffer(markupTestString, markupRoot), "Test failed");
                AZStd::list<UiTextComponent::DrawBatch> drawBatches;
                AZStd::stack<UiTextComponent::DrawBatch> batchStack;

                AZStd::stack<FontFamily*> fontFamilyStack;
                fontFamilyStack.push(fontFamily);

                UiTextComponent::FontFamilyRefSet fontFamilyRefs;
                BuildDrawBatches(drawBatches, fontFamilyRefs, batchStack, fontFamilyStack, &markupRoot);
                AZ_Assert(0 == fontFamilyRefs.size(), "Test failed");
                AZ_Assert(2 == drawBatches.size(), "Test failed");
                AssertTextNotEmpty(drawBatches);

                StringList stringList;
                stringList.push_back("this");
                stringList.push_back(" is a test!");
                AssertDrawBatchTextContent(drawBatches, stringList);

                FontList fontList;
                fontList.push_back(fontFamily->normal);
                fontList.push_back(fontFamily->normal);
                AssertDrawBatchFontOrder(drawBatches, fontList);

                ColorList colorList;
                colorList.push_back(AZ::Vector3(0.0f, 0.0f, 1.0f));
                colorList.push_back(TextMarkup::ColorInvalid);
                AssertDrawBatchMultiColor(drawBatches, colorList);
            }
        }

        // Font tag: font color (invalid hex value)
        {
            const LyShine::StringType markupTestString("<font color=\"#GGGGGG\">this</font> is a test!");

            {
                TextMarkup::Tag markupRoot;
                AZ_Assert(TextMarkup::ParseMarkupBuffer(markupTestString, markupRoot), "Test failed");
                AZStd::list<UiTextComponent::DrawBatch> drawBatches;
                AZStd::stack<UiTextComponent::DrawBatch> batchStack;

                AZStd::stack<FontFamily*> fontFamilyStack;
                fontFamilyStack.push(fontFamily);

                UiTextComponent::FontFamilyRefSet fontFamilyRefs;
                BuildDrawBatches(drawBatches, fontFamilyRefs, batchStack, fontFamilyStack, &markupRoot);
                AZ_Assert(0 == fontFamilyRefs.size(), "Test failed");
                AZ_Assert(2 == drawBatches.size(), "Test failed");
                AssertTextNotEmpty(drawBatches);

                StringList stringList;
                stringList.push_back("this");
                stringList.push_back(" is a test!");
                AssertDrawBatchTextContent(drawBatches, stringList);

                FontList fontList;
                fontList.push_back(fontFamily->normal);
                fontList.push_back(fontFamily->normal);
                AssertDrawBatchFontOrder(drawBatches, fontList);

                ColorList colorList;
                colorList.push_back(AZ::Vector3(0.0f, 0.0f, 0.0f));
                colorList.push_back(TextMarkup::ColorInvalid);
                AssertDrawBatchMultiColor(drawBatches, colorList);
            }
        }

        // Font tag: font color (invalid hex value)
        {
            const LyShine::StringType markupTestString("<font color=\"#FF\">this</font> is a test!");

            {
                TextMarkup::Tag markupRoot;
                AZ_Assert(TextMarkup::ParseMarkupBuffer(markupTestString, markupRoot), "Test failed");
                AZStd::list<UiTextComponent::DrawBatch> drawBatches;
                AZStd::stack<UiTextComponent::DrawBatch> batchStack;

                AZStd::stack<FontFamily*> fontFamilyStack;
                fontFamilyStack.push(fontFamily);

                UiTextComponent::FontFamilyRefSet fontFamilyRefs;
                BuildDrawBatches(drawBatches, fontFamilyRefs, batchStack, fontFamilyStack, &markupRoot);
                AZ_Assert(0 == fontFamilyRefs.size(), "Test failed");
                AZ_Assert(2 == drawBatches.size(), "Test failed");
                AssertTextNotEmpty(drawBatches);

                StringList stringList;
                stringList.push_back("this");
                stringList.push_back(" is a test!");
                AssertDrawBatchTextContent(drawBatches, stringList);

                FontList fontList;
                fontList.push_back(fontFamily->normal);
                fontList.push_back(fontFamily->normal);
                AssertDrawBatchFontOrder(drawBatches, fontList);

                ColorList colorList;
                colorList.push_back(TextMarkup::ColorInvalid);
                colorList.push_back(TextMarkup::ColorInvalid);
                AssertDrawBatchMultiColor(drawBatches, colorList);
            }
        }

        // Font tag: font color (invalid formatting)
        {
            const LyShine::StringType markupTestString("<font color=\"FF0000\">this</font> is a test!");

            {
                TextMarkup::Tag markupRoot;
                AZ_Assert(TextMarkup::ParseMarkupBuffer(markupTestString, markupRoot), "Test failed");
                AZStd::list<UiTextComponent::DrawBatch> drawBatches;
                AZStd::stack<UiTextComponent::DrawBatch> batchStack;

                AZStd::stack<FontFamily*> fontFamilyStack;
                fontFamilyStack.push(fontFamily);

                UiTextComponent::FontFamilyRefSet fontFamilyRefs;
                BuildDrawBatches(drawBatches, fontFamilyRefs, batchStack, fontFamilyStack, &markupRoot);
                AZ_Assert(0 == fontFamilyRefs.size(), "Test failed");
                AZ_Assert(2 == drawBatches.size(), "Test failed");
                AssertTextNotEmpty(drawBatches);

                StringList stringList;
                stringList.push_back("this");
                stringList.push_back(" is a test!");
                AssertDrawBatchTextContent(drawBatches, stringList);

                FontList fontList;
                fontList.push_back(fontFamily->normal);
                fontList.push_back(fontFamily->normal);
                AssertDrawBatchFontOrder(drawBatches, fontList);

                ColorList colorList;
                colorList.push_back(TextMarkup::ColorInvalid);
                colorList.push_back(TextMarkup::ColorInvalid);
                AssertDrawBatchMultiColor(drawBatches, colorList);
            }
        }

        // Font tag: font color (invalid formatting)
        {
            const LyShine::StringType markupTestString("<font color=\"gluten\">this</font> is a test!");

            {
                TextMarkup::Tag markupRoot;
                AZ_Assert(TextMarkup::ParseMarkupBuffer(markupTestString, markupRoot), "Test failed");
                AZStd::list<UiTextComponent::DrawBatch> drawBatches;
                AZStd::stack<UiTextComponent::DrawBatch> batchStack;

                AZStd::stack<FontFamily*> fontFamilyStack;
                fontFamilyStack.push(fontFamily);

                UiTextComponent::FontFamilyRefSet fontFamilyRefs;
                BuildDrawBatches(drawBatches, fontFamilyRefs, batchStack, fontFamilyStack, &markupRoot);
                AZ_Assert(0 == fontFamilyRefs.size(), "Test failed");
                AZ_Assert(2 == drawBatches.size(), "Test failed");
                AssertTextNotEmpty(drawBatches);

                StringList stringList;
                stringList.push_back("this");
                stringList.push_back(" is a test!");
                AssertDrawBatchTextContent(drawBatches, stringList);

                FontList fontList;
                fontList.push_back(fontFamily->normal);
                fontList.push_back(fontFamily->normal);
                AssertDrawBatchFontOrder(drawBatches, fontList);

                ColorList colorList;
                colorList.push_back(TextMarkup::ColorInvalid);
                colorList.push_back(TextMarkup::ColorInvalid);
                AssertDrawBatchMultiColor(drawBatches, colorList);
            }
        }

        // Font tag: font color (invalid value, empty string)
        {
            const LyShine::StringType markupTestString("<font color=\"\">this</font> is a test!");

            {
                TextMarkup::Tag markupRoot;
                AZ_Assert(TextMarkup::ParseMarkupBuffer(markupTestString, markupRoot), "Test failed");
                AZStd::list<UiTextComponent::DrawBatch> drawBatches;
                AZStd::stack<UiTextComponent::DrawBatch> batchStack;

                AZStd::stack<FontFamily*> fontFamilyStack;
                fontFamilyStack.push(fontFamily);

                UiTextComponent::FontFamilyRefSet fontFamilyRefs;
                BuildDrawBatches(drawBatches, fontFamilyRefs, batchStack, fontFamilyStack, &markupRoot);
                AZ_Assert(0 == fontFamilyRefs.size(), "Test failed");
                AZ_Assert(2 == drawBatches.size(), "Test failed");
                AssertTextNotEmpty(drawBatches);

                StringList stringList;
                stringList.push_back("this");
                stringList.push_back(" is a test!");
                AssertDrawBatchTextContent(drawBatches, stringList);

                FontList fontList;
                fontList.push_back(fontFamily->normal);
                fontList.push_back(fontFamily->normal);
                AssertDrawBatchFontOrder(drawBatches, fontList);

                ColorList colorList;
                colorList.push_back(TextMarkup::ColorInvalid);
                colorList.push_back(TextMarkup::ColorInvalid);
                AssertDrawBatchMultiColor(drawBatches, colorList);
            }
        }

        // Font tag: font color (invalid value, empty string, spaces)
        {
            const LyShine::StringType markupTestString("<font color=\"   \">this</font> is a test!");

            {
                TextMarkup::Tag markupRoot;
                AZ_Assert(TextMarkup::ParseMarkupBuffer(markupTestString, markupRoot), "Test failed");
                AZStd::list<UiTextComponent::DrawBatch> drawBatches;
                AZStd::stack<UiTextComponent::DrawBatch> batchStack;

                AZStd::stack<FontFamily*> fontFamilyStack;
                fontFamilyStack.push(fontFamily);

                UiTextComponent::FontFamilyRefSet fontFamilyRefs;
                BuildDrawBatches(drawBatches, fontFamilyRefs, batchStack, fontFamilyStack, &markupRoot);
                AZ_Assert(0 == fontFamilyRefs.size(), "Test failed");
                AZ_Assert(2 == drawBatches.size(), "Test failed");
                AssertTextNotEmpty(drawBatches);

                StringList stringList;
                stringList.push_back("this");
                stringList.push_back(" is a test!");
                AssertDrawBatchTextContent(drawBatches, stringList);

                FontList fontList;
                fontList.push_back(fontFamily->normal);
                fontList.push_back(fontFamily->normal);
                AssertDrawBatchFontOrder(drawBatches, fontList);

                ColorList colorList;
                colorList.push_back(TextMarkup::ColorInvalid);
                colorList.push_back(TextMarkup::ColorInvalid);
                AssertDrawBatchMultiColor(drawBatches, colorList);
            }
        }

        // Font tag: font color (invalid value, leading hash, empty following)
        {
            const LyShine::StringType markupTestString("<font color=\"#\">this</font> is a test!");

            {
                TextMarkup::Tag markupRoot;
                AZ_Assert(TextMarkup::ParseMarkupBuffer(markupTestString, markupRoot), "Test failed");
                AZStd::list<UiTextComponent::DrawBatch> drawBatches;
                AZStd::stack<UiTextComponent::DrawBatch> batchStack;

                AZStd::stack<FontFamily*> fontFamilyStack;
                fontFamilyStack.push(fontFamily);

                UiTextComponent::FontFamilyRefSet fontFamilyRefs;
                BuildDrawBatches(drawBatches, fontFamilyRefs, batchStack, fontFamilyStack, &markupRoot);
                AZ_Assert(0 == fontFamilyRefs.size(), "Test failed");
                AZ_Assert(2 == drawBatches.size(), "Test failed");
                AssertTextNotEmpty(drawBatches);

                StringList stringList;
                stringList.push_back("this");
                stringList.push_back(" is a test!");
                AssertDrawBatchTextContent(drawBatches, stringList);

                FontList fontList;
                fontList.push_back(fontFamily->normal);
                fontList.push_back(fontFamily->normal);
                AssertDrawBatchFontOrder(drawBatches, fontList);

                ColorList colorList;
                colorList.push_back(TextMarkup::ColorInvalid);
                colorList.push_back(TextMarkup::ColorInvalid);
                AssertDrawBatchMultiColor(drawBatches, colorList);
            }
        }

        // Font tag: font color (invalid value, leading spaces with hash)
        {
            const LyShine::StringType markupTestString("<font color=\"  #\">this</font> is a test!");

            {
                TextMarkup::Tag markupRoot;
                AZ_Assert(TextMarkup::ParseMarkupBuffer(markupTestString, markupRoot), "Test failed");
                AZStd::list<UiTextComponent::DrawBatch> drawBatches;
                AZStd::stack<UiTextComponent::DrawBatch> batchStack;

                AZStd::stack<FontFamily*> fontFamilyStack;
                fontFamilyStack.push(fontFamily);

                UiTextComponent::FontFamilyRefSet fontFamilyRefs;
                BuildDrawBatches(drawBatches, fontFamilyRefs, batchStack, fontFamilyStack, &markupRoot);
                AZ_Assert(0 == fontFamilyRefs.size(), "Test failed");
                AZ_Assert(2 == drawBatches.size(), "Test failed");
                AssertTextNotEmpty(drawBatches);

                StringList stringList;
                stringList.push_back("this");
                stringList.push_back(" is a test!");
                AssertDrawBatchTextContent(drawBatches, stringList);

                FontList fontList;
                fontList.push_back(fontFamily->normal);
                fontList.push_back(fontFamily->normal);
                AssertDrawBatchFontOrder(drawBatches, fontList);

                ColorList colorList;
                colorList.push_back(TextMarkup::ColorInvalid);
                colorList.push_back(TextMarkup::ColorInvalid);
                AssertDrawBatchMultiColor(drawBatches, colorList);
            }
        }

        // Font tag: font color (invalid value, trailing spaces with hash)
        {
            const LyShine::StringType markupTestString("<font color=\"#  \">this</font> is a test!");

            {
                TextMarkup::Tag markupRoot;
                AZ_Assert(TextMarkup::ParseMarkupBuffer(markupTestString, markupRoot), "Test failed");
                AZStd::list<UiTextComponent::DrawBatch> drawBatches;
                AZStd::stack<UiTextComponent::DrawBatch> batchStack;

                AZStd::stack<FontFamily*> fontFamilyStack;
                fontFamilyStack.push(fontFamily);

                UiTextComponent::FontFamilyRefSet fontFamilyRefs;
                BuildDrawBatches(drawBatches, fontFamilyRefs, batchStack, fontFamilyStack, &markupRoot);
                AZ_Assert(0 == fontFamilyRefs.size(), "Test failed");
                AZ_Assert(2 == drawBatches.size(), "Test failed");
                AssertTextNotEmpty(drawBatches);

                StringList stringList;
                stringList.push_back("this");
                stringList.push_back(" is a test!");
                AssertDrawBatchTextContent(drawBatches, stringList);

                FontList fontList;
                fontList.push_back(fontFamily->normal);
                fontList.push_back(fontFamily->normal);
                AssertDrawBatchFontOrder(drawBatches, fontList);

                ColorList colorList;
                colorList.push_back(TextMarkup::ColorInvalid);
                colorList.push_back(TextMarkup::ColorInvalid);
                AssertDrawBatchMultiColor(drawBatches, colorList);
            }
        }

        // Font tag: font color (invalid value, leading and trailing spaces with hash)
        {
            const LyShine::StringType markupTestString("<font color=\"  #  \">this</font> is a test!");

            {
                TextMarkup::Tag markupRoot;
                AZ_Assert(TextMarkup::ParseMarkupBuffer(markupTestString, markupRoot), "Test failed");
                AZStd::list<UiTextComponent::DrawBatch> drawBatches;
                AZStd::stack<UiTextComponent::DrawBatch> batchStack;

                AZStd::stack<FontFamily*> fontFamilyStack;
                fontFamilyStack.push(fontFamily);

                UiTextComponent::FontFamilyRefSet fontFamilyRefs;
                BuildDrawBatches(drawBatches, fontFamilyRefs, batchStack, fontFamilyStack, &markupRoot);
                AZ_Assert(0 == fontFamilyRefs.size(), "Test failed");
                AZ_Assert(2 == drawBatches.size(), "Test failed");
                AssertTextNotEmpty(drawBatches);

                StringList stringList;
                stringList.push_back("this");
                stringList.push_back(" is a test!");
                AssertDrawBatchTextContent(drawBatches, stringList);

                FontList fontList;
                fontList.push_back(fontFamily->normal);
                fontList.push_back(fontFamily->normal);
                AssertDrawBatchFontOrder(drawBatches, fontList);

                ColorList colorList;
                colorList.push_back(TextMarkup::ColorInvalid);
                colorList.push_back(TextMarkup::ColorInvalid);
                AssertDrawBatchMultiColor(drawBatches, colorList);
            }
        }

        // Font tag: font face and color
        {
            const LyShine::StringType markupTestString("<font face=\"notoserif\" color=\"#FF0000\">this</font> is a test!");

            {
                TextMarkup::Tag markupRoot;
                AZ_Assert(TextMarkup::ParseMarkupBuffer(markupTestString, markupRoot), "Test failed");
                AZStd::list<UiTextComponent::DrawBatch> drawBatches;
                AZStd::stack<UiTextComponent::DrawBatch> batchStack;

                AZStd::stack<FontFamily*> fontFamilyStack;
                fontFamilyStack.push(fontFamily);

                UiTextComponent::FontFamilyRefSet fontFamilyRefs;
                BuildDrawBatches(drawBatches, fontFamilyRefs, batchStack, fontFamilyStack, &markupRoot);
                AZ_Assert(1 == fontFamilyRefs.size(), "Test failed");
                AZ_Assert(2 == drawBatches.size(), "Test failed");
                AssertTextNotEmpty(drawBatches);

                StringList stringList;
                stringList.push_back("this");
                stringList.push_back(" is a test!");
                AssertDrawBatchTextContent(drawBatches, stringList);

                FontFamilyPtr notoSerifFamily = gEnv->pCryFont->GetFontFamily("notoserif");
                AZ_Assert(notoSerifFamily, "Test failed");

                FontList fontList;
                fontList.push_back(notoSerifFamily->normal);
                fontList.push_back(fontFamily->normal);
                AssertDrawBatchFontOrder(drawBatches, fontList);

                ColorList colorList;
                colorList.push_back(AZ::Vector3(1.0f, 0.0f, 0.0f));
                colorList.push_back(TextMarkup::ColorInvalid);
                AssertDrawBatchMultiColor(drawBatches, colorList);
            }
        }

        // Font tag: font color and face
        {
            const LyShine::StringType markupTestString("<font color=\"#FF0000\" face=\"notoserif\">this</font> is a test!");

            {
                TextMarkup::Tag markupRoot;
                AZ_Assert(TextMarkup::ParseMarkupBuffer(markupTestString, markupRoot), "Test failed");
                AZStd::list<UiTextComponent::DrawBatch> drawBatches;
                AZStd::stack<UiTextComponent::DrawBatch> batchStack;

                AZStd::stack<FontFamily*> fontFamilyStack;
                fontFamilyStack.push(fontFamily);

                UiTextComponent::FontFamilyRefSet fontFamilyRefs;
                BuildDrawBatches(drawBatches, fontFamilyRefs, batchStack, fontFamilyStack, &markupRoot);
                AZ_Assert(1 == fontFamilyRefs.size(), "Test failed");
                AZ_Assert(2 == drawBatches.size(), "Test failed");
                AssertTextNotEmpty(drawBatches);

                StringList stringList;
                stringList.push_back("this");
                stringList.push_back(" is a test!");
                AssertDrawBatchTextContent(drawBatches, stringList);

                FontFamilyPtr notoSerifFamily = gEnv->pCryFont->GetFontFamily("notoserif");
                AZ_Assert(notoSerifFamily, "Test failed");

                FontList fontList;
                fontList.push_back(notoSerifFamily->normal);
                fontList.push_back(fontFamily->normal);
                AssertDrawBatchFontOrder(drawBatches, fontList);

                ColorList colorList;
                colorList.push_back(AZ::Vector3(1.0f, 0.0f, 0.0f));
                colorList.push_back(TextMarkup::ColorInvalid);
                AssertDrawBatchMultiColor(drawBatches, colorList);
            }
        }

        // Font tag: invalid attribute
        {
            const LyShine::StringType markupTestString("<font cllor=\"#FF0000\">this</font> is a test!");

            {
                TextMarkup::Tag markupRoot;
                AZ_Assert(false == TextMarkup::ParseMarkupBuffer(markupTestString, markupRoot), "Test failed");
            }
        }

        // Mixed test: Bold, italic, bold-italic
        {
            const LyShine::StringType markupTestString("Regular <b>Bold</b> <i>Italic\n<b>Bold-Italic</b></i>");

            {
                TextMarkup::Tag markupRoot;
                AZ_Assert(TextMarkup::ParseMarkupBuffer(markupTestString, markupRoot), "Test failed");
                AZStd::list<UiTextComponent::DrawBatch> drawBatches;
                AZStd::stack<UiTextComponent::DrawBatch> batchStack;

                AZStd::stack<FontFamily*> fontFamilyStack;
                fontFamilyStack.push(fontFamily);

                UiTextComponent::FontFamilyRefSet fontFamilyRefs;
                BuildDrawBatches(drawBatches, fontFamilyRefs, batchStack, fontFamilyStack, &markupRoot);
                AZ_Assert(0 == fontFamilyRefs.size(), "Test failed");
                AZ_Assert(5 == drawBatches.size(), "Test failed");
                AssertTextNotEmpty(drawBatches);

                StringList stringList;
                stringList.push_back("Regular ");
                stringList.push_back("Bold");
                stringList.push_back(" ");
                stringList.push_back("Italic\n");
                stringList.push_back("Bold-Italic");
                AssertDrawBatchTextContent(drawBatches, stringList);

                FontList fontList;
                fontList.push_back(fontFamily->normal);
                fontList.push_back(fontFamily->bold);
                fontList.push_back(fontFamily->normal);
                fontList.push_back(fontFamily->italic);
                fontList.push_back(fontFamily->boldItalic);
                AssertDrawBatchFontOrder(drawBatches, fontList);

                AssertDrawBatchSingleColor(drawBatches, TextMarkup::ColorInvalid);
            }
        }

        // Mixed test: Font color, font face, bold
        {
            const LyShine::StringType markupTestString("<font color=\"#00ff00\">Regular <font face=\"notoserif\"><b>Bold</b></font></font>");

            {
                TextMarkup::Tag markupRoot;
                AZ_Assert(TextMarkup::ParseMarkupBuffer(markupTestString, markupRoot), "Test failed");
                AZStd::list<UiTextComponent::DrawBatch> drawBatches;
                AZStd::stack<UiTextComponent::DrawBatch> batchStack;

                AZStd::stack<FontFamily*> fontFamilyStack;
                fontFamilyStack.push(fontFamily);

                UiTextComponent::FontFamilyRefSet fontFamilyRefs;
                BuildDrawBatches(drawBatches, fontFamilyRefs, batchStack, fontFamilyStack, &markupRoot);
                AZ_Assert(1 == fontFamilyRefs.size(), "Test failed");
                AZ_Assert(2 == drawBatches.size(), "Test failed");
                AssertTextNotEmpty(drawBatches);

                StringList stringList;
                stringList.push_back("Regular ");
                stringList.push_back("Bold");
                AssertDrawBatchTextContent(drawBatches, stringList);

                FontFamilyPtr notoSerifFamily = gEnv->pCryFont->GetFontFamily("notoserif");
                AZ_Assert(notoSerifFamily, "Test failed");

                FontList fontList;
                fontList.push_back(fontFamily->normal);
                fontList.push_back(notoSerifFamily->bold);
                AssertDrawBatchFontOrder(drawBatches, fontList);

                ColorList colorList;
                colorList.push_back(AZ::Vector3(0.0f, 1.0f, 0.0f));
                colorList.push_back(AZ::Vector3(0.0f, 1.0f, 0.0f));
                AssertDrawBatchMultiColor(drawBatches, colorList);
            }
        }

        // Mixed test: Multiple font faces, color, bold
        {
            const LyShine::StringType markupTestString("<font color=\"#00ff00\">Regular </font><font face=\"notoserif\"><b>Bold</b></font> <i>Italic<b> Bold-Italic</b></i>\nHere is <font face=\"default-ui\">default-ui</font>");

            {
                TextMarkup::Tag markupRoot;
                AZ_Assert(TextMarkup::ParseMarkupBuffer(markupTestString, markupRoot), "Test failed");
                AZStd::list<UiTextComponent::DrawBatch> drawBatches;
                AZStd::stack<UiTextComponent::DrawBatch> batchStack;

                AZStd::stack<FontFamily*> fontFamilyStack;
                fontFamilyStack.push(fontFamily);

                UiTextComponent::FontFamilyRefSet fontFamilyRefs;
                BuildDrawBatches(drawBatches, fontFamilyRefs, batchStack, fontFamilyStack, &markupRoot);
                AZ_Assert(2 == fontFamilyRefs.size(), "Test failed");
                AZ_Assert(7 == drawBatches.size(), "Test failed");
                AssertTextNotEmpty(drawBatches);

                StringList stringList;
                stringList.push_back("Regular ");
                stringList.push_back("Bold");
                stringList.push_back(" ");
                stringList.push_back("Italic");
                stringList.push_back(" Bold-Italic");
                stringList.push_back("\nHere is ");
                stringList.push_back("default-ui");
                AssertDrawBatchTextContent(drawBatches, stringList);

                FontFamilyPtr notoSerifFamily = gEnv->pCryFont->GetFontFamily("notoserif");
                AZ_Assert(notoSerifFamily, "Test failed");
                FontFamilyPtr defaultUiFamily = gEnv->pCryFont->GetFontFamily("default-ui");
                AZ_Assert(notoSerifFamily, "Test failed");

                FontList fontList;
                fontList.push_back(fontFamily->normal);
                fontList.push_back(notoSerifFamily->bold);
                fontList.push_back(fontFamily->normal);
                fontList.push_back(fontFamily->italic);
                fontList.push_back(fontFamily->boldItalic);
                fontList.push_back(fontFamily->normal);
                fontList.push_back(defaultUiFamily->normal);
                AssertDrawBatchFontOrder(drawBatches, fontList);

                ColorList colorList;
                colorList.push_back(AZ::Vector3(0.0f, 1.0f, 0.0f));
                colorList.push_back(TextMarkup::ColorInvalid);
                colorList.push_back(TextMarkup::ColorInvalid);
                colorList.push_back(TextMarkup::ColorInvalid);
                colorList.push_back(TextMarkup::ColorInvalid);
                colorList.push_back(TextMarkup::ColorInvalid);
                colorList.push_back(TextMarkup::ColorInvalid);
                AssertDrawBatchMultiColor(drawBatches, colorList);
            }
        }
    }

    using SizeList = AZStd::list < size_t >;

    void AssertBatchLineSizes(
        const UiTextComponent::DrawBatchLines& batchLines,
        const SizeList& batchSizes)
    {
        AZ_Assert(batchLines.batchLines.size() == batchSizes.size(), "Test failed");

        auto linesIt = batchLines.batchLines.begin();
        auto sizesIt = batchSizes.begin();
        for (;
            linesIt != batchLines.batchLines.end();
            ++linesIt, ++sizesIt)
        {
            const UiTextComponent::DrawBatchContainer& batchLine = (*linesIt).drawBatchList;
            const size_t batchSize = *sizesIt;
            AZ_Assert(batchLine.size() == batchSize, "Test failed");
        }
    }

    using DrawBatchLines = UiTextComponent::DrawBatchLines;
    using DrawBatchContainer = UiTextComponent::DrawBatchContainer;
    using DrawBatch = UiTextComponent::DrawBatch;

    void WrapTextTests(FontFamily* fontFamily)
    {
        STextDrawContext fontContext;
        fontContext.SetEffect(0);
        fontContext.SetSizeIn800x600(false);
        fontContext.SetSize(vector2f(32.0f, 32.0f));

        {
            const LyShine::StringType testMarkup("Regular Bold Italic\n");
            DrawBatchContainer drawBatches;
            DrawBatch b1;
            b1.font = fontFamily->normal;
            b1.text = testMarkup;
            drawBatches.push_back(b1);

            InsertNewlinesToWrapText(drawBatches, fontContext, 1000.0f);
            AZ_Assert(drawBatches.front().text == testMarkup, "Test failed");
        }

        {
            // "Regular Bold   v          .<i>Italic\n</i>Bold-Italic"

            StringList stringList;
            stringList.push_back("Regular Bold   v          .");
            stringList.push_back("Italic\n");
            stringList.push_back("Bold-Italic");
            StringList::const_iterator citer = stringList.begin();

            DrawBatchContainer drawBatches;
            DrawBatch b1;
            b1.font = fontFamily->normal;
            b1.text = *citer; ++citer;
            drawBatches.push_back(b1);
            DrawBatch b2;
            b2.font = fontFamily->italic;
            b2.text = *citer; ++citer;
            drawBatches.push_back(b2);
            DrawBatch b3;
            b3.font = fontFamily->normal;
            b3.text = *citer; ++citer;
            drawBatches.push_back(b3);

            InsertNewlinesToWrapText(drawBatches, fontContext, 1000.0f);
            AssertDrawBatchTextContent(drawBatches, stringList);
        }
    }

    void BatchLinesTests(FontFamily* fontFamily)
    {
        STextDrawContext fontContext;
        fontContext.SetEffect(0);
        fontContext.SetSizeIn800x600(false);
        fontContext.SetSize(vector2f(32.0f, 32.0f));

        UiTextInterface::DisplayedTextFunction displayedTextFunction(DefaultDisplayedTextFunction);

        {
            DrawBatchLines batchLines;
            DrawBatchContainer drawBatches;
            DrawBatch b1;
            b1.font = fontFamily->normal;
            b1.text = "a";
            drawBatches.push_back(b1);

            CreateBatchLines(batchLines, drawBatches, fontFamily);
            AZ_Assert(1 == batchLines.batchLines.size(), "Test failed");

            SizeList sizeList;
            sizeList.push_back(1);
            AssertBatchLineSizes(batchLines, sizeList);
        }

        {
            DrawBatchLines batchLines;
            DrawBatchContainer drawBatches;
            DrawBatch b1;
            b1.font = fontFamily->normal;
            b1.text = "a\n";
            drawBatches.push_back(b1);

            CreateBatchLines(batchLines, drawBatches, fontFamily);
            AZ_Assert(2 == batchLines.batchLines.size(), "Test failed");

            SizeList sizeList;
            sizeList.push_back(1);
            sizeList.push_back(1);
            AssertBatchLineSizes(batchLines, sizeList);
        }

        {
            DrawBatchLines batchLines;
            DrawBatchContainer drawBatches;
            DrawBatch b1;
            b1.font = fontFamily->normal;
            b1.text = "a\nb";
            drawBatches.push_back(b1);

            CreateBatchLines(batchLines, drawBatches, fontFamily);
            AZ_Assert(2 == batchLines.batchLines.size(), "Test failed");

            SizeList sizeList;
            sizeList.push_back(1);
            sizeList.push_back(1);
            AssertBatchLineSizes(batchLines, sizeList);
        }

        {
            DrawBatchLines batchLines;
            DrawBatchContainer drawBatches;
            DrawBatch b1;
            b1.font = fontFamily->normal;
            b1.text = "a\n\nb";
            drawBatches.push_back(b1);

            CreateBatchLines(batchLines, drawBatches, fontFamily);
            AZ_Assert(3 == batchLines.batchLines.size(), "Test failed");

            SizeList sizeList;
            sizeList.push_back(1);
            sizeList.push_back(1);
            sizeList.push_back(1);
            AssertBatchLineSizes(batchLines, sizeList);
        }

        {
            DrawBatchLines batchLines;
            DrawBatchContainer drawBatches;
            DrawBatch b1;
            b1.font = fontFamily->normal;
            b1.text = "a\n\n\nb";
            drawBatches.push_back(b1);

            CreateBatchLines(batchLines, drawBatches, fontFamily);
            AZ_Assert(4 == batchLines.batchLines.size(), "Test failed");

            SizeList sizeList;
            sizeList.push_back(1);
            sizeList.push_back(1);
            sizeList.push_back(1);
            sizeList.push_back(1);
            AssertBatchLineSizes(batchLines, sizeList);
        }

        {
            DrawBatchLines batchLines;
            DrawBatchContainer drawBatches;
            DrawBatch b1;
            b1.font = fontFamily->normal;
            b1.text = "Regular Bold Italic\n";
            drawBatches.push_back(b1);

            CreateBatchLines(batchLines, drawBatches, fontFamily);
            AZ_Assert(2 == batchLines.batchLines.size(), "Test failed");

            SizeList sizeList;
            sizeList.push_back(1);
            sizeList.push_back(1);
            AssertBatchLineSizes(batchLines, sizeList);
        }

        {
            const LyShine::StringType markupTestString("Regular Bold     <i>Italic</i>Bold-Italic");
            TextMarkup::Tag markupRoot;

            AZ_Assert(TextMarkup::ParseMarkupBuffer(markupTestString, markupRoot), "Test failed");
            AZStd::list<UiTextComponent::DrawBatch> drawBatches;
            AZStd::stack<UiTextComponent::DrawBatch> batchStack;

            AZStd::stack<FontFamily*> fontFamilyStack;
            fontFamilyStack.push(fontFamily);

            UiTextComponent::FontFamilyRefSet fontFamilyRefs;
            BuildDrawBatches(drawBatches, fontFamilyRefs, batchStack, fontFamilyStack, &markupRoot);

            DrawBatchLines batchLines;
            BatchAwareWrapText(batchLines, drawBatches, fontFamily, fontContext, displayedTextFunction, 290.0f);
            AZ_Assert(2 == batchLines.batchLines.size(), "Test failed");

            SizeList sizeList;
            sizeList.push_back(1);
            sizeList.push_back(2);
            AssertBatchLineSizes(batchLines, sizeList);
        }

        {
            const LyShine::StringType markupTestString("Regular <b>Bold</b> <i>Italic\n<b>Bold-Italic</b></i>");
            TextMarkup::Tag markupRoot;

            AZ_Assert(TextMarkup::ParseMarkupBuffer(markupTestString, markupRoot), "Test failed");
            AZStd::list<UiTextComponent::DrawBatch> drawBatches;
            AZStd::stack<UiTextComponent::DrawBatch> batchStack;

            AZStd::stack<FontFamily*> fontFamilyStack;
            fontFamilyStack.push(fontFamily);

            UiTextComponent::FontFamilyRefSet fontFamilyRefs;
            BuildDrawBatches(drawBatches, fontFamilyRefs, batchStack, fontFamilyStack, &markupRoot);
            DrawBatchLines batchLines;

            CreateBatchLines(batchLines, drawBatches, fontFamily);
            AZ_Assert(2 == batchLines.batchLines.size(), "Test failed");

            SizeList sizeList;
            sizeList.push_back(4);
            sizeList.push_back(1);
            AssertBatchLineSizes(batchLines, sizeList);
        }
    }

    void CreateComponent(AZ::Entity* entity, const AZ::Uuid& componentTypeId)
    {
        entity->Deactivate();
        entity->CreateComponent(componentTypeId);
        entity->Activate();
    }

    void TrackingLeadingTests(CLyShine* lyshine)
    {
        // Tracking

        // one space
        {
            AZ::EntityId canvasEntityId = lyshine->CreateCanvas();
            UiCanvasInterface* canvas = UiCanvasBus::FindFirstHandler(canvasEntityId);
            AZ_Assert(canvas, "Test failed");

            AZ::Entity* testElem = canvas->CreateChildElement("Test1");
            AZ_Assert(testElem, "Test failed");
            CreateComponent(testElem, LyShine::UiTransform2dComponentUuid);
            CreateComponent(testElem, LyShine::UiTextComponentUuid);
            AZ::EntityId testElemId = testElem->GetId();

            const AZStd::string testString("Hi");
            EBUS_EVENT_ID(testElemId, UiTextBus, SetText, testString);
            
            float baseWidth;
            EBUS_EVENT_ID_RESULT(baseWidth, testElemId, UiLayoutCellDefaultBus, GetTargetWidth);
            
            EBUS_EVENT_ID(testElemId, UiTextBus, SetCharacterSpacing, 1000.0f);
            float newWidth;
            EBUS_EVENT_ID_RESULT(newWidth, testElemId, UiLayoutCellDefaultBus, GetTargetWidth);

            AZ_Assert(newWidth == baseWidth + 1.0f, "Test failed");

            lyshine->ReleaseCanvas(canvasEntityId, false);
        }
        // no space
        {
            AZ::EntityId canvasEntityId = lyshine->CreateCanvas();
            UiCanvasInterface* canvas = UiCanvasBus::FindFirstHandler(canvasEntityId);
            AZ_Assert(canvas, "Test failed");

            AZ::Entity* testElem = canvas->CreateChildElement("Test2");
            AZ_Assert(testElem, "Test failed");
            CreateComponent(testElem, LyShine::UiTransform2dComponentUuid);
            CreateComponent(testElem, LyShine::UiTextComponentUuid);
            AZ::EntityId testElemId = testElem->GetId();

            const AZStd::string testString("");
            EBUS_EVENT_ID(testElemId, UiTextBus, SetText, testString);

            float baseWidth;
            EBUS_EVENT_ID_RESULT(baseWidth, testElemId, UiLayoutCellDefaultBus, GetTargetWidth);

            EBUS_EVENT_ID(testElemId, UiTextBus, SetCharacterSpacing, 1000.0f);
            float newWidth;
            EBUS_EVENT_ID_RESULT(newWidth, testElemId, UiLayoutCellDefaultBus, GetTargetWidth);

            AZ_Assert(newWidth == baseWidth, "Test failed");

            lyshine->ReleaseCanvas(canvasEntityId, false);
        }
        // bigger spacing
        {
            AZ::EntityId canvasEntityId = lyshine->CreateCanvas();
            UiCanvasInterface* canvas = UiCanvasBus::FindFirstHandler(canvasEntityId);
            AZ_Assert(canvas, "Test failed");

            AZ::Entity* testElem = canvas->CreateChildElement("Test3");
            AZ_Assert(testElem, "Test failed");
            CreateComponent(testElem, LyShine::UiTransform2dComponentUuid);
            CreateComponent(testElem, LyShine::UiTextComponentUuid);
            AZ::EntityId testElemId = testElem->GetId();

            const AZStd::string testString("Hi");
            EBUS_EVENT_ID(testElemId, UiTextBus, SetText, testString);

            float baseWidth;
            EBUS_EVENT_ID_RESULT(baseWidth, testElemId, UiLayoutCellDefaultBus, GetTargetWidth);

            EBUS_EVENT_ID(testElemId, UiTextBus, SetCharacterSpacing, 4500.0f);
            float newWidth;
            EBUS_EVENT_ID_RESULT(newWidth, testElemId, UiLayoutCellDefaultBus, GetTargetWidth);

            AZ_Assert(newWidth == baseWidth + 4.5f, "Test failed");

            lyshine->ReleaseCanvas(canvasEntityId, false);
        }
        // four spaces
        {
            AZ::EntityId canvasEntityId = lyshine->CreateCanvas();
            UiCanvasInterface* canvas = UiCanvasBus::FindFirstHandler(canvasEntityId);
            AZ_Assert(canvas, "Test failed");

            AZ::Entity* testElem = canvas->CreateChildElement("Test4");
            AZ_Assert(testElem, "Test failed");
            CreateComponent(testElem, LyShine::UiTransform2dComponentUuid);
            CreateComponent(testElem, LyShine::UiTextComponentUuid);
            AZ::EntityId testElemId = testElem->GetId();

            const AZStd::string testString("abcde");
            EBUS_EVENT_ID(testElemId, UiTextBus, SetText, testString);

            float baseWidth;
            EBUS_EVENT_ID_RESULT(baseWidth, testElemId, UiLayoutCellDefaultBus, GetTargetWidth);

            EBUS_EVENT_ID(testElemId, UiTextBus, SetCharacterSpacing, 1000.0f);
            float newWidth;
            EBUS_EVENT_ID_RESULT(newWidth, testElemId, UiLayoutCellDefaultBus, GetTargetWidth);

            AZ_Assert(newWidth == baseWidth + 4.0f, "Test failed");

            lyshine->ReleaseCanvas(canvasEntityId, false);
        }
        // four spaces, larger spacing
        {
            AZ::EntityId canvasEntityId = lyshine->CreateCanvas();
            UiCanvasInterface* canvas = UiCanvasBus::FindFirstHandler(canvasEntityId);
            AZ_Assert(canvas, "Test failed");

            AZ::Entity* testElem = canvas->CreateChildElement("Test5");
            AZ_Assert(testElem, "Test failed");
            CreateComponent(testElem, LyShine::UiTransform2dComponentUuid);
            CreateComponent(testElem, LyShine::UiTextComponentUuid);
            AZ::EntityId testElemId = testElem->GetId();

            const AZStd::string testString("abcde");
            EBUS_EVENT_ID(testElemId, UiTextBus, SetText, testString);

            float baseWidth;
            EBUS_EVENT_ID_RESULT(baseWidth, testElemId, UiLayoutCellDefaultBus, GetTargetWidth);

            EBUS_EVENT_ID(testElemId, UiTextBus, SetCharacterSpacing, 3500.0f);
            float newWidth;
            EBUS_EVENT_ID_RESULT(newWidth, testElemId, UiLayoutCellDefaultBus, GetTargetWidth);

            AZ_Assert(newWidth == baseWidth + 14.0f, "Test failed");

            lyshine->ReleaseCanvas(canvasEntityId, false);
        }
        // negative spacing
        {
            AZ::EntityId canvasEntityId = lyshine->CreateCanvas();
            UiCanvasInterface* canvas = UiCanvasBus::FindFirstHandler(canvasEntityId);
            AZ_Assert(canvas, "Test failed");

            AZ::Entity* testElem = canvas->CreateChildElement("Test6");
            AZ_Assert(testElem, "Test failed");
            CreateComponent(testElem, LyShine::UiTransform2dComponentUuid);
            CreateComponent(testElem, LyShine::UiTextComponentUuid);
            AZ::EntityId testElemId = testElem->GetId();

            const AZStd::string testString("Hi");
            EBUS_EVENT_ID(testElemId, UiTextBus, SetText, testString);

            float baseWidth;
            EBUS_EVENT_ID_RESULT(baseWidth, testElemId, UiLayoutCellDefaultBus, GetTargetWidth);

            EBUS_EVENT_ID(testElemId, UiTextBus, SetCharacterSpacing, -1000.0f);
            float newWidth;
            EBUS_EVENT_ID_RESULT(newWidth, testElemId, UiLayoutCellDefaultBus, GetTargetWidth);

            AZ_Assert(newWidth == baseWidth - 1.0f, "Test failed");

            lyshine->ReleaseCanvas(canvasEntityId, false);
        }
        // negative spacing, 4 spaces
        {
            AZ::EntityId canvasEntityId = lyshine->CreateCanvas();
            UiCanvasInterface* canvas = UiCanvasBus::FindFirstHandler(canvasEntityId);
            AZ_Assert(canvas, "Test failed");

            AZ::Entity* testElem = canvas->CreateChildElement("Test7");
            AZ_Assert(testElem, "Test failed");
            CreateComponent(testElem, LyShine::UiTransform2dComponentUuid);
            CreateComponent(testElem, LyShine::UiTextComponentUuid);
            AZ::EntityId testElemId = testElem->GetId();

            const AZStd::string testString("abcde");
            EBUS_EVENT_ID(testElemId, UiTextBus, SetText, testString);

            float baseWidth;
            EBUS_EVENT_ID_RESULT(baseWidth, testElemId, UiLayoutCellDefaultBus, GetTargetWidth);

            EBUS_EVENT_ID(testElemId, UiTextBus, SetCharacterSpacing, -1000.0f);
            float newWidth;
            EBUS_EVENT_ID_RESULT(newWidth, testElemId, UiLayoutCellDefaultBus, GetTargetWidth);

            AZ_Assert(newWidth == baseWidth - 4.0f, "Test failed");

            lyshine->ReleaseCanvas(canvasEntityId, false);
        }
        // smaller font size, one space
        {
            AZ::EntityId canvasEntityId = lyshine->CreateCanvas();
            UiCanvasInterface* canvas = UiCanvasBus::FindFirstHandler(canvasEntityId);
            AZ_Assert(canvas, "Test failed");

            AZ::Entity* testElem = canvas->CreateChildElement("Test8");
            AZ_Assert(testElem, "Test failed");
            CreateComponent(testElem, LyShine::UiTransform2dComponentUuid);
            CreateComponent(testElem, LyShine::UiTextComponentUuid);
            AZ::EntityId testElemId = testElem->GetId();

            const AZStd::string testString("Hi");
            EBUS_EVENT_ID(testElemId, UiTextBus, SetText, testString);

            EBUS_EVENT_ID(testElemId, UiTextBus, SetFontSize, 16.0f);

            float baseWidth;
            EBUS_EVENT_ID_RESULT(baseWidth, testElemId, UiLayoutCellDefaultBus, GetTargetWidth);

            EBUS_EVENT_ID(testElemId, UiTextBus, SetCharacterSpacing, 1000.0f);
            float newWidth;
            EBUS_EVENT_ID_RESULT(newWidth, testElemId, UiLayoutCellDefaultBus, GetTargetWidth);

            AZ_Assert(newWidth == baseWidth + 0.5f, "Test failed");

            lyshine->ReleaseCanvas(canvasEntityId, false);
        }
        // smaller font size, ten spaces
        {
            AZ::EntityId canvasEntityId = lyshine->CreateCanvas();
            UiCanvasInterface* canvas = UiCanvasBus::FindFirstHandler(canvasEntityId);
            AZ_Assert(canvas, "Test failed");

            AZ::Entity* testElem = canvas->CreateChildElement("Test9");
            AZ_Assert(testElem, "Test failed");
            CreateComponent(testElem, LyShine::UiTransform2dComponentUuid);
            CreateComponent(testElem, LyShine::UiTextComponentUuid);
            AZ::EntityId testElemId = testElem->GetId();

            const AZStd::string testString("abcdefghijk");
            EBUS_EVENT_ID(testElemId, UiTextBus, SetText, testString);

            EBUS_EVENT_ID(testElemId, UiTextBus, SetFontSize, 16.0f);

            float baseWidth;
            EBUS_EVENT_ID_RESULT(baseWidth, testElemId, UiLayoutCellDefaultBus, GetTargetWidth);

            EBUS_EVENT_ID(testElemId, UiTextBus, SetCharacterSpacing, 1000.0f);
            float newWidth;
            EBUS_EVENT_ID_RESULT(newWidth, testElemId, UiLayoutCellDefaultBus, GetTargetWidth);

            AZ_Assert(newWidth == baseWidth + 5.0f, "Test failed");

            lyshine->ReleaseCanvas(canvasEntityId, false);
        }
        // smaller font size, ten spaces, larger spacing
        {
            AZ::EntityId canvasEntityId = lyshine->CreateCanvas();
            UiCanvasInterface* canvas = UiCanvasBus::FindFirstHandler(canvasEntityId);
            AZ_Assert(canvas, "Test failed");

            AZ::Entity* testElem = canvas->CreateChildElement("Test10");
            AZ_Assert(testElem, "Test failed");
            CreateComponent(testElem, LyShine::UiTransform2dComponentUuid);
            CreateComponent(testElem, LyShine::UiTextComponentUuid);
            AZ::EntityId testElemId = testElem->GetId();

            const AZStd::string testString("abcdefghijk");
            EBUS_EVENT_ID(testElemId, UiTextBus, SetText, testString);

            EBUS_EVENT_ID(testElemId, UiTextBus, SetFontSize, 16.0f);

            float baseWidth;
            EBUS_EVENT_ID_RESULT(baseWidth, testElemId, UiLayoutCellDefaultBus, GetTargetWidth);

            EBUS_EVENT_ID(testElemId, UiTextBus, SetCharacterSpacing, 3500.0f);
            float newWidth;
            EBUS_EVENT_ID_RESULT(newWidth, testElemId, UiLayoutCellDefaultBus, GetTargetWidth);

            AZ_Assert(newWidth == baseWidth + 17.5f, "Test failed");

            lyshine->ReleaseCanvas(canvasEntityId, false);
        }
        // larger font size, one space
        {
            AZ::EntityId canvasEntityId = lyshine->CreateCanvas();
            UiCanvasInterface* canvas = UiCanvasBus::FindFirstHandler(canvasEntityId);
            AZ_Assert(canvas, "Test failed");

            AZ::Entity* testElem = canvas->CreateChildElement("Test11");
            AZ_Assert(testElem, "Test failed");
            CreateComponent(testElem, LyShine::UiTransform2dComponentUuid);
            CreateComponent(testElem, LyShine::UiTextComponentUuid);
            AZ::EntityId testElemId = testElem->GetId();

            const AZStd::string testString("Hi");
            EBUS_EVENT_ID(testElemId, UiTextBus, SetText, testString);

            EBUS_EVENT_ID(testElemId, UiTextBus, SetFontSize, 64.0f);

            float baseWidth;
            EBUS_EVENT_ID_RESULT(baseWidth, testElemId, UiLayoutCellDefaultBus, GetTargetWidth);

            EBUS_EVENT_ID(testElemId, UiTextBus, SetCharacterSpacing, 1000.0f);
            float newWidth;
            EBUS_EVENT_ID_RESULT(newWidth, testElemId, UiLayoutCellDefaultBus, GetTargetWidth);

            AZ_Assert(newWidth == baseWidth + 2.0f, "Test failed");

            lyshine->ReleaseCanvas(canvasEntityId, false);
        }
        // larger font size, seven spaces
        {
            AZ::EntityId canvasEntityId = lyshine->CreateCanvas();
            UiCanvasInterface* canvas = UiCanvasBus::FindFirstHandler(canvasEntityId);
            AZ_Assert(canvas, "Test failed");

            AZ::Entity* testElem = canvas->CreateChildElement("Test12");
            AZ_Assert(testElem, "Test failed");
            CreateComponent(testElem, LyShine::UiTransform2dComponentUuid);
            CreateComponent(testElem, LyShine::UiTextComponentUuid);
            AZ::EntityId testElemId = testElem->GetId();

            const AZStd::string testString("abcdefgh");
            EBUS_EVENT_ID(testElemId, UiTextBus, SetText, testString);

            EBUS_EVENT_ID(testElemId, UiTextBus, SetFontSize, 64.0f);

            float baseWidth;
            EBUS_EVENT_ID_RESULT(baseWidth, testElemId, UiLayoutCellDefaultBus, GetTargetWidth);

            EBUS_EVENT_ID(testElemId, UiTextBus, SetCharacterSpacing, 1000.0f);
            float newWidth;
            EBUS_EVENT_ID_RESULT(newWidth, testElemId, UiLayoutCellDefaultBus, GetTargetWidth);

            AZ_Assert(newWidth == baseWidth + 14.0f, "Test failed");

            lyshine->ReleaseCanvas(canvasEntityId, false);
        }
        // larger font size, seven spaces, larger spacing
        {
            AZ::EntityId canvasEntityId = lyshine->CreateCanvas();
            UiCanvasInterface* canvas = UiCanvasBus::FindFirstHandler(canvasEntityId);
            AZ_Assert(canvas, "Test failed");

            AZ::Entity* testElem = canvas->CreateChildElement("Test13");
            AZ_Assert(testElem, "Test failed");
            CreateComponent(testElem, LyShine::UiTransform2dComponentUuid);
            CreateComponent(testElem, LyShine::UiTextComponentUuid);
            AZ::EntityId testElemId = testElem->GetId();

            const AZStd::string testString("abcdefgh");
            EBUS_EVENT_ID(testElemId, UiTextBus, SetText, testString);

            EBUS_EVENT_ID(testElemId, UiTextBus, SetFontSize, 64.0f);

            float baseWidth;
            EBUS_EVENT_ID_RESULT(baseWidth, testElemId, UiLayoutCellDefaultBus, GetTargetWidth);

            EBUS_EVENT_ID(testElemId, UiTextBus, SetCharacterSpacing, 5200.0f);
            float newWidth;
            EBUS_EVENT_ID_RESULT(newWidth, testElemId, UiLayoutCellDefaultBus, GetTargetWidth);

            AZ_Assert(newWidth == baseWidth + 72.8f, "Test failed");

            lyshine->ReleaseCanvas(canvasEntityId, false);
        }

        // Leading

        // one space
        {
            AZ::EntityId canvasEntityId = lyshine->CreateCanvas();
            UiCanvasInterface* canvas = UiCanvasBus::FindFirstHandler(canvasEntityId);
            AZ_Assert(canvas, "Test failed");

            AZ::Entity* testElem = canvas->CreateChildElement("Test14");
            AZ_Assert(testElem, "Test failed");
            CreateComponent(testElem, LyShine::UiTransform2dComponentUuid);
            CreateComponent(testElem, LyShine::UiTextComponentUuid);
            AZ::EntityId testElemId = testElem->GetId();

            const AZStd::string testString("Hi\nHello");
            EBUS_EVENT_ID(testElemId, UiTextBus, SetText, testString);

            float baseHeight;
            EBUS_EVENT_ID_RESULT(baseHeight, testElemId, UiLayoutCellDefaultBus, GetTargetHeight);

            EBUS_EVENT_ID(testElemId, UiTextBus, SetLineSpacing, 5.0f);
            float newHeight;
            EBUS_EVENT_ID_RESULT(newHeight, testElemId, UiLayoutCellDefaultBus, GetTargetHeight);

            AZ_Assert(newHeight == baseHeight + 5.0f, "Test failed");

            lyshine->ReleaseCanvas(canvasEntityId, false);
        }
        // four spaces
        {
            AZ::EntityId canvasEntityId = lyshine->CreateCanvas();
            UiCanvasInterface* canvas = UiCanvasBus::FindFirstHandler(canvasEntityId);
            AZ_Assert(canvas, "Test failed");

            AZ::Entity* testElem = canvas->CreateChildElement("Test15");
            AZ_Assert(testElem, "Test failed");
            CreateComponent(testElem, LyShine::UiTransform2dComponentUuid);
            CreateComponent(testElem, LyShine::UiTextComponentUuid);
            AZ::EntityId testElemId = testElem->GetId();

            const AZStd::string testString("1\n2\n3\n4\n5");
            EBUS_EVENT_ID(testElemId, UiTextBus, SetText, testString);

            float baseHeight;
            EBUS_EVENT_ID_RESULT(baseHeight, testElemId, UiLayoutCellDefaultBus, GetTargetHeight);

            EBUS_EVENT_ID(testElemId, UiTextBus, SetLineSpacing, 5.0f);
            float newHeight;
            EBUS_EVENT_ID_RESULT(newHeight, testElemId, UiLayoutCellDefaultBus, GetTargetHeight);

            AZ_Assert(newHeight == baseHeight + 20.0f, "Test failed");

            lyshine->ReleaseCanvas(canvasEntityId, false);
        }
        // seven spaces, larger spacing
        {
            AZ::EntityId canvasEntityId = lyshine->CreateCanvas();
            UiCanvasInterface* canvas = UiCanvasBus::FindFirstHandler(canvasEntityId);
            AZ_Assert(canvas, "Test failed");

            AZ::Entity* testElem = canvas->CreateChildElement("Test16");
            AZ_Assert(testElem, "Test failed");
            CreateComponent(testElem, LyShine::UiTransform2dComponentUuid);
            CreateComponent(testElem, LyShine::UiTextComponentUuid);
            AZ::EntityId testElemId = testElem->GetId();

            const AZStd::string testString("1\n2\n3\n4\n5\n6\n7\n8");
            EBUS_EVENT_ID(testElemId, UiTextBus, SetText, testString);

            float baseHeight;
            EBUS_EVENT_ID_RESULT(baseHeight, testElemId, UiLayoutCellDefaultBus, GetTargetHeight);

            EBUS_EVENT_ID(testElemId, UiTextBus, SetLineSpacing, 8.3f);
            float newHeight;
            EBUS_EVENT_ID_RESULT(newHeight, testElemId, UiLayoutCellDefaultBus, GetTargetHeight);

            AZ_Assert(newHeight == baseHeight + 58.1f, "Test failed");

            lyshine->ReleaseCanvas(canvasEntityId, false);
        }
        // one space, negative spacing
        {
            AZ::EntityId canvasEntityId = lyshine->CreateCanvas();
            UiCanvasInterface* canvas = UiCanvasBus::FindFirstHandler(canvasEntityId);
            AZ_Assert(canvas, "Test failed");

            AZ::Entity* testElem = canvas->CreateChildElement("Test17");
            AZ_Assert(testElem, "Test failed");
            CreateComponent(testElem, LyShine::UiTransform2dComponentUuid);
            CreateComponent(testElem, LyShine::UiTextComponentUuid);
            AZ::EntityId testElemId = testElem->GetId();

            const AZStd::string testString("1\n2");
            EBUS_EVENT_ID(testElemId, UiTextBus, SetText, testString);

            float baseHeight;
            EBUS_EVENT_ID_RESULT(baseHeight, testElemId, UiLayoutCellDefaultBus, GetTargetHeight);

            EBUS_EVENT_ID(testElemId, UiTextBus, SetLineSpacing, -1.0f);
            float newHeight;
            EBUS_EVENT_ID_RESULT(newHeight, testElemId, UiLayoutCellDefaultBus, GetTargetHeight);

            AZ_Assert(newHeight == baseHeight - 1.0f, "Test failed");

            lyshine->ReleaseCanvas(canvasEntityId, false);
        }
        // three spaces, negative spacing, larger spacing
        {
            AZ::EntityId canvasEntityId = lyshine->CreateCanvas();
            UiCanvasInterface* canvas = UiCanvasBus::FindFirstHandler(canvasEntityId);
            AZ_Assert(canvas, "Test failed");

            AZ::Entity* testElem = canvas->CreateChildElement("Test18");
            AZ_Assert(testElem, "Test failed");
            CreateComponent(testElem, LyShine::UiTransform2dComponentUuid);
            CreateComponent(testElem, LyShine::UiTextComponentUuid);
            AZ::EntityId testElemId = testElem->GetId();

            const AZStd::string testString("1\n2\n3\n4");
            EBUS_EVENT_ID(testElemId, UiTextBus, SetText, testString);

            float baseHeight;
            EBUS_EVENT_ID_RESULT(baseHeight, testElemId, UiLayoutCellDefaultBus, GetTargetHeight);

            EBUS_EVENT_ID(testElemId, UiTextBus, SetLineSpacing, -2.2f);
            float newHeight;
            EBUS_EVENT_ID_RESULT(newHeight, testElemId, UiLayoutCellDefaultBus, GetTargetHeight);

            AZ_Assert(newHeight == baseHeight - 6.6f, "Test failed");

            lyshine->ReleaseCanvas(canvasEntityId, false);
        }
        // one space, smaller font
        {
            AZ::EntityId canvasEntityId = lyshine->CreateCanvas();
            UiCanvasInterface* canvas = UiCanvasBus::FindFirstHandler(canvasEntityId);
            AZ_Assert(canvas, "Test failed");

            AZ::Entity* testElem = canvas->CreateChildElement("Test19");
            AZ_Assert(testElem, "Test failed");
            CreateComponent(testElem, LyShine::UiTransform2dComponentUuid);
            CreateComponent(testElem, LyShine::UiTextComponentUuid);
            AZ::EntityId testElemId = testElem->GetId();

            const AZStd::string testString("1\n2");
            EBUS_EVENT_ID(testElemId, UiTextBus, SetText, testString);

            EBUS_EVENT_ID(testElemId, UiTextBus, SetFontSize, 18.0f);

            float baseHeight;
            EBUS_EVENT_ID_RESULT(baseHeight, testElemId, UiLayoutCellDefaultBus, GetTargetHeight);

            EBUS_EVENT_ID(testElemId, UiTextBus, SetLineSpacing, 1.0f);
            float newHeight;
            EBUS_EVENT_ID_RESULT(newHeight, testElemId, UiLayoutCellDefaultBus, GetTargetHeight);

            AZ_Assert(newHeight == baseHeight + 1.0f, "Test failed");

            lyshine->ReleaseCanvas(canvasEntityId, false);
        }
        // four spaces, smaller font
        {
            AZ::EntityId canvasEntityId = lyshine->CreateCanvas();
            UiCanvasInterface* canvas = UiCanvasBus::FindFirstHandler(canvasEntityId);
            AZ_Assert(canvas, "Test failed");

            AZ::Entity* testElem = canvas->CreateChildElement("Test20");
            AZ_Assert(testElem, "Test failed");
            CreateComponent(testElem, LyShine::UiTransform2dComponentUuid);
            CreateComponent(testElem, LyShine::UiTextComponentUuid);
            AZ::EntityId testElemId = testElem->GetId();

            const AZStd::string testString("1\n2\n3\n4\n5");
            EBUS_EVENT_ID(testElemId, UiTextBus, SetText, testString);

            EBUS_EVENT_ID(testElemId, UiTextBus, SetFontSize, 18.0f);

            float baseHeight;
            EBUS_EVENT_ID_RESULT(baseHeight, testElemId, UiLayoutCellDefaultBus, GetTargetHeight);

            EBUS_EVENT_ID(testElemId, UiTextBus, SetLineSpacing, 1.0f);
            float newHeight;
            EBUS_EVENT_ID_RESULT(newHeight, testElemId, UiLayoutCellDefaultBus, GetTargetHeight);

            AZ_Assert(newHeight == baseHeight + 4.0f, "Test failed");

            lyshine->ReleaseCanvas(canvasEntityId, false);
        }
        // one space, larger font
        {
            AZ::EntityId canvasEntityId = lyshine->CreateCanvas();
            UiCanvasInterface* canvas = UiCanvasBus::FindFirstHandler(canvasEntityId);
            AZ_Assert(canvas, "Test failed");

            AZ::Entity* testElem = canvas->CreateChildElement("Test21");
            AZ_Assert(testElem, "Test failed");
            CreateComponent(testElem, LyShine::UiTransform2dComponentUuid);
            CreateComponent(testElem, LyShine::UiTextComponentUuid);
            AZ::EntityId testElemId = testElem->GetId();

            const AZStd::string testString("1\n2");
            EBUS_EVENT_ID(testElemId, UiTextBus, SetText, testString);

            EBUS_EVENT_ID(testElemId, UiTextBus, SetFontSize, 64.0f);

            float baseHeight;
            EBUS_EVENT_ID_RESULT(baseHeight, testElemId, UiLayoutCellDefaultBus, GetTargetHeight);

            EBUS_EVENT_ID(testElemId, UiTextBus, SetLineSpacing, 1.0f);
            float newHeight;
            EBUS_EVENT_ID_RESULT(newHeight, testElemId, UiLayoutCellDefaultBus, GetTargetHeight);

            AZ_Assert(newHeight == baseHeight + 1.0f, "Test failed");

            lyshine->ReleaseCanvas(canvasEntityId, false);
        }
        // four spaces, larger font
        {
            AZ::EntityId canvasEntityId = lyshine->CreateCanvas();
            UiCanvasInterface* canvas = UiCanvasBus::FindFirstHandler(canvasEntityId);
            AZ_Assert(canvas, "Test failed");

            AZ::Entity* testElem = canvas->CreateChildElement("Test22");
            AZ_Assert(testElem, "Test failed");
            CreateComponent(testElem, LyShine::UiTransform2dComponentUuid);
            CreateComponent(testElem, LyShine::UiTextComponentUuid);
            AZ::EntityId testElemId = testElem->GetId();

            const AZStd::string testString("1\n2\n3\n4\n5");
            EBUS_EVENT_ID(testElemId, UiTextBus, SetText, testString);

            EBUS_EVENT_ID(testElemId, UiTextBus, SetFontSize, 64.0f);

            float baseHeight;
            EBUS_EVENT_ID_RESULT(baseHeight, testElemId, UiLayoutCellDefaultBus, GetTargetHeight);

            EBUS_EVENT_ID(testElemId, UiTextBus, SetLineSpacing, 1.0f);
            float newHeight;
            EBUS_EVENT_ID_RESULT(newHeight, testElemId, UiLayoutCellDefaultBus, GetTargetHeight);

            AZ_Assert(newHeight == baseHeight + 4.0f, "Test failed");

            lyshine->ReleaseCanvas(canvasEntityId, false);
        }
    }

    void ComponentGetSetTextTests(CLyShine* lyshine)
    {
        {
            AZ::EntityId canvasEntityId = lyshine->CreateCanvas();
            UiCanvasInterface* canvas = UiCanvasBus::FindFirstHandler(canvasEntityId);
            AZ_Assert(canvas, "Test failed");

            AZ::Entity* testElem = canvas->CreateChildElement("Test1");
            AZ_Assert(testElem, "Test failed");
            CreateComponent(testElem, LyShine::UiTransform2dComponentUuid);
            CreateComponent(testElem, LyShine::UiTextComponentUuid);
            AZ::EntityId testElemId = testElem->GetId();

            const AZStd::string testString("Hi");
            EBUS_EVENT_ID(testElemId, UiTextBus, SetText, testString);
            AZStd::string resultString;
            EBUS_EVENT_ID_RESULT(resultString, testElemId, UiTextBus, GetText);
            AZ_Assert(testString == resultString, "Test failed");
            resultString.clear();
            EBUS_EVENT_ID_RESULT(resultString, testElemId, UiTextBus, GetTextWithFlags, UiTextInterface::GetTextFlags::GetAsIs);
            AZ_Assert(testString == resultString, "Test failed");

            lyshine->ReleaseCanvas(canvasEntityId, false);
        }

        {
            AZ::EntityId canvasEntityId = lyshine->CreateCanvas();
            UiCanvasInterface* canvas = UiCanvasBus::FindFirstHandler(canvasEntityId);
            AZ_Assert(canvas, "Test failed");

            AZ::Entity* testElem = canvas->CreateChildElement("Test1");
            AZ_Assert(testElem, "Test failed");
            CreateComponent(testElem, LyShine::UiTransform2dComponentUuid);
            CreateComponent(testElem, LyShine::UiTextComponentUuid);
            AZ::EntityId testElemId = testElem->GetId();

            const AZStd::string testString("Hi");
            EBUS_EVENT_ID(testElemId, UiTextBus, SetTextWithFlags, testString, UiTextInterface::SetAsIs);
            AZStd::string resultString;
            EBUS_EVENT_ID_RESULT(resultString, testElemId, UiTextBus, GetText);
            AZ_Assert(testString == resultString, "Test failed");
            resultString.clear();
            EBUS_EVENT_ID_RESULT(resultString, testElemId, UiTextBus, GetTextWithFlags, UiTextInterface::GetTextFlags::GetAsIs);
            AZ_Assert(testString == resultString, "Test failed");

            lyshine->ReleaseCanvas(canvasEntityId, false);
        }

        {
            AZ::EntityId canvasEntityId = lyshine->CreateCanvas();
            UiCanvasInterface* canvas = UiCanvasBus::FindFirstHandler(canvasEntityId);
            AZ_Assert(canvas, "Test failed");

            AZ::Entity* testElem = canvas->CreateChildElement("Test1");
            AZ_Assert(testElem, "Test failed");
            CreateComponent(testElem, LyShine::UiTransform2dComponentUuid);
            CreateComponent(testElem, LyShine::UiTextComponentUuid);
            AZ::EntityId testElemId = testElem->GetId();

            const AZStd::string testString("&<>%");
            EBUS_EVENT_ID(testElemId, UiTextBus, SetTextWithFlags, testString, UiTextInterface::SetAsIs);
            AZStd::string resultString;
            EBUS_EVENT_ID_RESULT(resultString, testElemId, UiTextBus, GetText);
            AZ_Assert(testString == resultString, "Test failed");
            resultString.clear();
            EBUS_EVENT_ID_RESULT(resultString, testElemId, UiTextBus, GetTextWithFlags, UiTextInterface::GetTextFlags::GetAsIs);
            AZ_Assert(testString == resultString, "Test failed");

            lyshine->ReleaseCanvas(canvasEntityId, false);
        }

        {
            AZ::EntityId canvasEntityId = lyshine->CreateCanvas();
            UiCanvasInterface* canvas = UiCanvasBus::FindFirstHandler(canvasEntityId);
            AZ_Assert(canvas, "Test failed");

            AZ::Entity* testElem = canvas->CreateChildElement("Test1");
            AZ_Assert(testElem, "Test failed");
            CreateComponent(testElem, LyShine::UiTransform2dComponentUuid);
            CreateComponent(testElem, LyShine::UiTextComponentUuid);
            AZ::EntityId testElemId = testElem->GetId();

            const AZStd::string testString("&amp;&lt;&gt;&#37;");
            EBUS_EVENT_ID(testElemId, UiTextBus, SetTextWithFlags, testString, UiTextInterface::SetAsIs);
            AZStd::string resultString;
            EBUS_EVENT_ID_RESULT(resultString, testElemId, UiTextBus, GetText);
            AZ_Assert(testString == resultString, "Test failed");
            resultString.clear();
            EBUS_EVENT_ID_RESULT(resultString, testElemId, UiTextBus, GetTextWithFlags, UiTextInterface::GetTextFlags::GetAsIs);
            AZ_Assert(testString == resultString, "Test failed");

            lyshine->ReleaseCanvas(canvasEntityId, false);
        }

        {
            AZ::EntityId canvasEntityId = lyshine->CreateCanvas();
            UiCanvasInterface* canvas = UiCanvasBus::FindFirstHandler(canvasEntityId);
            AZ_Assert(canvas, "Test failed");

            AZ::Entity* testElem = canvas->CreateChildElement("Test1");
            AZ_Assert(testElem, "Test failed");
            CreateComponent(testElem, LyShine::UiTransform2dComponentUuid);
            CreateComponent(testElem, LyShine::UiTextComponentUuid);
            AZ::EntityId testElemId = testElem->GetId();

            const AZStd::string testString("&<>%");
            EBUS_EVENT_ID(testElemId, UiTextBus, SetTextWithFlags, testString, UiTextInterface::SetEscapeMarkup);
            AZStd::string resultString;
            EBUS_EVENT_ID_RESULT(resultString, testElemId, UiTextBus, GetText);
            AZ_Assert(testString == resultString, "Test failed");
            resultString.clear();
            EBUS_EVENT_ID_RESULT(resultString, testElemId, UiTextBus, GetTextWithFlags, UiTextInterface::GetTextFlags::GetAsIs);
            AZ_Assert(testString == resultString, "Test failed");

            lyshine->ReleaseCanvas(canvasEntityId, false);
        }
    }

    void ComponentGetSetTextTestsLoc(CLyShine* lyshine)
    {
        if (AZStd::string("korean") == GetISystem()->GetLocalizationManager()->GetLanguage())
        {
            static const LyShine::StringType koreanHello("\xEC\x95\x88\xEB\x85\x95\xED\x95\x98\xEC\x84\xB8\xEC\x9A\x94");

            {
                AZ::EntityId canvasEntityId = lyshine->CreateCanvas();
                UiCanvasInterface* canvas = UiCanvasBus::FindFirstHandler(canvasEntityId);
                AZ_Assert(canvas, "Test failed");

                AZ::Entity* testElem = canvas->CreateChildElement("Test1");
                AZ_Assert(testElem, "Test failed");
                CreateComponent(testElem, LyShine::UiTransform2dComponentUuid);
                CreateComponent(testElem, LyShine::UiTextComponentUuid);
                AZ::EntityId testElemId = testElem->GetId();

                AZStd::string testString("@ui_Hello");
                EBUS_EVENT_ID(testElemId, UiTextBus, SetTextWithFlags, testString, UiTextInterface::SetLocalized);
                AZStd::string resultString;
                EBUS_EVENT_ID_RESULT(resultString, testElemId, UiTextBus, GetText);
                AZ_Assert(testString == resultString, "Test failed");
                resultString.clear();
                EBUS_EVENT_ID_RESULT(resultString, testElemId, UiTextBus, GetTextWithFlags, UiTextInterface::GetTextFlags::GetAsIs);
                AZ_Assert(testString == resultString, "Test failed");
                resultString.clear();

                testString = koreanHello;
                EBUS_EVENT_ID_RESULT(resultString, testElemId, UiTextBus, GetTextWithFlags, UiTextInterface::GetLocalized);
                AZ_Assert(testString == resultString, "Test failed");
                resultString.clear();

                lyshine->ReleaseCanvas(canvasEntityId, false);
            }

            {
                AZ::EntityId canvasEntityId = lyshine->CreateCanvas();
                UiCanvasInterface* canvas = UiCanvasBus::FindFirstHandler(canvasEntityId);
                AZ_Assert(canvas, "Test failed");

                AZ::Entity* testElem = canvas->CreateChildElement("Test1");
                AZ_Assert(testElem, "Test failed");
                CreateComponent(testElem, LyShine::UiTransform2dComponentUuid);
                CreateComponent(testElem, LyShine::UiTextComponentUuid);
                AZ::EntityId testElemId = testElem->GetId();

                AZStd::string testString("&<>% @ui_Hello");
                UiTextInterface::SetTextFlags setTextFlags = static_cast<UiTextInterface::SetTextFlags>(UiTextInterface::SetEscapeMarkup | UiTextInterface::SetLocalized);
                EBUS_EVENT_ID(testElemId, UiTextBus, SetTextWithFlags, testString, setTextFlags);
                AZStd::string resultString;
                EBUS_EVENT_ID_RESULT(resultString, testElemId, UiTextBus, GetText);
                AZ_Assert(testString == resultString, "Test failed");
                resultString.clear();
                EBUS_EVENT_ID_RESULT(resultString, testElemId, UiTextBus, GetTextWithFlags, UiTextInterface::GetTextFlags::GetAsIs);
                AZ_Assert(testString == resultString, "Test failed");
                resultString.clear();

                testString = LyShine::StringType("&<>% ") + koreanHello;
                EBUS_EVENT_ID_RESULT(resultString, testElemId, UiTextBus, GetTextWithFlags, UiTextInterface::GetLocalized);
                AZ_Assert(testString == resultString, "Test failed");
                resultString.clear();

                lyshine->ReleaseCanvas(canvasEntityId, false);
            }
        }
    }
}

void UiTextComponent::UnitTest(CLyShine* lyshine)
{
    VerifyShippingFonts();

    // These fonts are required for subsequent unit-tests to work.
    FontFamilyPtr notoSans = FontFamilyLoad("ui/fonts/lyshineexamples/notosans/notosans.fontfamily");
    FontFamilyPtr notoSerif = FontFamilyLoad("ui/fonts/lyshineexamples/notoserif/notoserif.fontfamily");

    NewlineSanitizeTests();
    MarkupEscapingTests();
    BuildDrawBatchesTests(notoSans.get());
    WrapTextTests(notoSans.get());
    BatchLinesTests(notoSans.get());
    TrackingLeadingTests(lyshine);
    ComponentGetSetTextTests(lyshine);
}

void UiTextComponent::UnitTestLocalization(CLyShine* lyshine)
{
    ILocalizationManager* pLocMan = GetISystem()->GetLocalizationManager();

    string localizationXml("libs/localization/localization.xml");

    bool initLocSuccess = false;

    if (pLocMan)
    {
        if (pLocMan->InitLocalizationData(localizationXml.c_str()))
        {
            if (pLocMan->LoadLocalizationDataByTag("init"))
            {
                initLocSuccess = true;
            }
        }
    }

    ComponentGetSetTextTestsLoc(lyshine);
}

#endif