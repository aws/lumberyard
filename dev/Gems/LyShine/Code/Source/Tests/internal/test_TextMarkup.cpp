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

#if defined(LYSHINE_INTERNAL_UNIT_TEST)
void TextMarkup::UnitTest()
{
    const LyShine::StringType rawCharData("this is a test!");

    ///////////////////////////////////////////////////////////////
    // Expected inputs: general
    {
        LyShine::StringType source = "this <i>is</i> a <b>test</b>!";
        LyShine::StringType expectedMarkup = "<root><ch value=\"this \" /><i><ch value=\"is\" /></i><ch value=\" a \" /><b><ch value=\"test\" /></b><ch value=\"!\" /></root>";

        LyShine::StringType markupTarget;
        InsertMarkup(source, markupTarget);
        AZ_Assert(expectedMarkup == markupTarget, "Test failed");

        LyShine::StringType target;
        TextMarkup::CopyCharData(source, target);
        AZ_Assert(rawCharData == target, "Test failed");
    }

    {
        LyShine::StringType source = "<b>this</b> <i>is</i> a <b>test</b>!";
        LyShine::StringType expectedMarkup = "<root><b><ch value=\"this\" /></b><ch value=\" \" /><i><ch value=\"is\" /></i><ch value=\" a \" /><b><ch value=\"test\" /></b><ch value=\"!\" /></root>";

        LyShine::StringType markupTarget;
        InsertMarkup(source, markupTarget);
        AZ_Assert(expectedMarkup == markupTarget, "Test failed");

        LyShine::StringType target;
        TextMarkup::CopyCharData(source, target);
        AZ_Assert(rawCharData == target, "Test failed");
    }

    ///////////////////////////////////////////////////////////////
    // Expected inputs: <font> tag
    {

        LyShine::StringType source = "<font face=\"times\">this</font> <i>is</i> a <b>test</b>!";
        LyShine::StringType expectedMarkup = "<root><font face=\"times\"><ch value=\"this\" /></font><ch value=\" \" /><i><ch value=\"is\" /></i><ch value=\" a \" /><b><ch value=\"test\" /></b><ch value=\"!\" /></root>";

        LyShine::StringType markupTarget;
        InsertMarkup(source, markupTarget);
        AZ_Assert(expectedMarkup == markupTarget, "Test failed");

        LyShine::StringType target;
        TextMarkup::CopyCharData(source, target);
        AZ_Assert(rawCharData == target, "Test failed");
    }

    {
        LyShine::StringType source = "<font color=\"#FF00FF\">this</font> <i>is</i> a <b>test</b>!";
        LyShine::StringType expectedMarkup = "<root><font color=\"#FF00FF\"><ch value=\"this\" /></font><ch value=\" \" /><i><ch value=\"is\" /></i><ch value=\" a \" /><b><ch value=\"test\" /></b><ch value=\"!\" /></root>";

        LyShine::StringType markupTarget;
        InsertMarkup(source, markupTarget);
        AZ_Assert(expectedMarkup == markupTarget, "Test failed");

        LyShine::StringType target;
        TextMarkup::CopyCharData(source, target);
        AZ_Assert(rawCharData == target, "Test failed");
    }

    {
        LyShine::StringType source = "<font color=\"#FF00FF\" face=\"times\">this</font> <i>is</i> a <b>test</b>!";
        LyShine::StringType expectedMarkup = "<root><font color=\"#FF00FF\" face=\"times\"><ch value=\"this\" /></font><ch value=\" \" /><i><ch value=\"is\" /></i><ch value=\" a \" /><b><ch value=\"test\" /></b><ch value=\"!\" /></root>";

        LyShine::StringType markupTarget;
        InsertMarkup(source, markupTarget);
        AZ_Assert(expectedMarkup == markupTarget, "Test failed");

        LyShine::StringType target;
        TextMarkup::CopyCharData(source, target);
        AZ_Assert(rawCharData == target, "Test failed");
    }

    {
        LyShine::StringType source = "<font face=\"times\" color=\"#FF00FF\" >this</font> <i>is</i> a <b>test</b>!";
        LyShine::StringType expectedMarkup = "<root><font face=\"times\" color=\"#FF00FF\" ><ch value=\"this\" /></font><ch value=\" \" /><i><ch value=\"is\" /></i><ch value=\" a \" /><b><ch value=\"test\" /></b><ch value=\"!\" /></root>";

        LyShine::StringType markupTarget;
        InsertMarkup(source, markupTarget);
        AZ_Assert(expectedMarkup == markupTarget, "Test failed");

        LyShine::StringType target;
        TextMarkup::CopyCharData(source, target);
        AZ_Assert(rawCharData == target, "Test failed");
    }

    {
        LyShine::StringType source = "<font face=\"times\">this <i>is</i> a <b>test</b>!</font>";
        LyShine::StringType expectedMarkup = "<root><font face=\"times\"><ch value=\"this \" /><i><ch value=\"is\" /></i><ch value=\" a \" /><b><ch value=\"test\" /></b><ch value=\"!\" /></font></root>";

        LyShine::StringType markupTarget;
        InsertMarkup(source, markupTarget);
        AZ_Assert(expectedMarkup == markupTarget, "Test failed");

        LyShine::StringType target;
        TextMarkup::CopyCharData(source, target);
        AZ_Assert(rawCharData == target, "Test failed");
    }

    {
        LyShine::StringType source = "<font color=\"#FF00FF\">this <i>is</i> a <b>test</b>!</font>";
        LyShine::StringType expectedMarkup = "<root><font color=\"#FF00FF\"><ch value=\"this \" /><i><ch value=\"is\" /></i><ch value=\" a \" /><b><ch value=\"test\" /></b><ch value=\"!\" /></font></root>";

        LyShine::StringType markupTarget;
        InsertMarkup(source, markupTarget);
        AZ_Assert(expectedMarkup == markupTarget, "Test failed");

        LyShine::StringType target;
        TextMarkup::CopyCharData(source, target);
        AZ_Assert(rawCharData == target, "Test failed");
    }

    {
        LyShine::StringType source = "<font color=\"#FF00FF\" face=\"times\">this <i>is</i> a <b>test</b>!</font>";
        LyShine::StringType expectedMarkup = "<root><font color=\"#FF00FF\" face=\"times\"><ch value=\"this \" /><i><ch value=\"is\" /></i><ch value=\" a \" /><b><ch value=\"test\" /></b><ch value=\"!\" /></font></root>";

        LyShine::StringType markupTarget;
        InsertMarkup(source, markupTarget);
        AZ_Assert(expectedMarkup == markupTarget, "Test failed");

        LyShine::StringType target;
        TextMarkup::CopyCharData(source, target);
        AZ_Assert(rawCharData == target, "Test failed");
    }

    {
        LyShine::StringType source = "<font face=\"times\" color=\"#FF00FF\" >this <i>is</i> a <b>test</b>!</font>";
        LyShine::StringType expectedMarkup = "<root><font face=\"times\" color=\"#FF00FF\" ><ch value=\"this \" /><i><ch value=\"is\" /></i><ch value=\" a \" /><b><ch value=\"test\" /></b><ch value=\"!\" /></font></root>";

        LyShine::StringType markupTarget;
        InsertMarkup(source, markupTarget);
        AZ_Assert(expectedMarkup == markupTarget, "Test failed");

        LyShine::StringType target;
        TextMarkup::CopyCharData(source, target);
        AZ_Assert(rawCharData == target, "Test failed");
    }

    ///////////////////////////////////////////////////////////////
    // Font tags with no attributes: expect failure
    {
        LyShine::StringType source = "<font>this</font> <i>is</i> a <b>test</b>!";
        LyShine::StringType expectedMarkup = "<root><font><ch value=\"this\" /></font><ch value=\" \" /><i><ch value=\"is\" /></i><ch value=\" a \" /><b><ch value=\"test\" /></b><ch value=\"!\" /></root>";

        LyShine::StringType markupTarget;
        InsertMarkup(source, markupTarget);
        AZ_Assert(expectedMarkup == markupTarget, "Test failed");

        LyShine::StringType target;
        TextMarkup::CopyCharData(source, target);
        AZ_Assert(source == target, "Test failed");
    }

    {
        LyShine::StringType source = "this <i><font>is</font></i> a <b>test</b>!";
        LyShine::StringType expectedMarkup = "<root><ch value=\"this \" /><i><font><ch value=\"is\" /></font></i><ch value=\" a \" /><b><ch value=\"test\" /></b><ch value=\"!\" /></root>";

        LyShine::StringType markupTarget;
        InsertMarkup(source, markupTarget);
        AZ_Assert(expectedMarkup == markupTarget, "Test failed");

        LyShine::StringType target;
        TextMarkup::CopyCharData(source, target);
        AZ_Assert(source == target, "Test failed");
    }

    {
        LyShine::StringType source = "this <i>is</i> a <b><font>test</font></b>!";
        LyShine::StringType expectedMarkup = "<root><ch value=\"this \" /><i><ch value=\"is\" /></i><ch value=\" a \" /><b><font><ch value=\"test\" /></font></b><ch value=\"!\" /></root>";

        LyShine::StringType markupTarget;
        InsertMarkup(source, markupTarget);
        AZ_Assert(expectedMarkup == markupTarget, "Test failed");

        LyShine::StringType target;
        TextMarkup::CopyCharData(source, target);
        AZ_Assert(source == target, "Test failed");
    }

    ///////////////////////////////////////////////////////////////
    // Font tags with unrecognized attributes: expect failure
    {
        LyShine::StringType source = "<font bad=\"1\">this</font> <i>is</i> a <b>test</b>!";
        LyShine::StringType expectedMarkup = "<root><font bad=\"1\"><ch value=\"this\" /></font><ch value=\" \" /><i><ch value=\"is\" /></i><ch value=\" a \" /><b><ch value=\"test\" /></b><ch value=\"!\" /></root>";

        LyShine::StringType markupTarget;
        InsertMarkup(source, markupTarget);
        AZ_Assert(expectedMarkup == markupTarget, "Test failed");

        LyShine::StringType target;
        TextMarkup::CopyCharData(source, target);
        AZ_Assert(source == target, "Test failed");
    }

    {
        LyShine::StringType source = "this <i><font bad=\"1\">is</font></i> a <b>test</b>!";
        LyShine::StringType expectedMarkup = "<root><ch value=\"this \" /><i><font bad=\"1\"><ch value=\"is\" /></font></i><ch value=\" a \" /><b><ch value=\"test\" /></b><ch value=\"!\" /></root>";

        LyShine::StringType markupTarget;
        InsertMarkup(source, markupTarget);
        AZ_Assert(expectedMarkup == markupTarget, "Test failed");

        LyShine::StringType target;
        TextMarkup::CopyCharData(source, target);
        AZ_Assert(source == target, "Test failed");
    }

    {
        LyShine::StringType source = "this <i>is</i> a <b><font bad=\"1\">test</font></b>!";
        LyShine::StringType expectedMarkup = "<root><ch value=\"this \" /><i><ch value=\"is\" /></i><ch value=\" a \" /><b><font bad=\"1\"><ch value=\"test\" /></font></b><ch value=\"!\" /></root>";

        LyShine::StringType markupTarget;
        InsertMarkup(source, markupTarget);
        AZ_Assert(expectedMarkup == markupTarget, "Test failed");

        LyShine::StringType target;
        TextMarkup::CopyCharData(source, target);
        AZ_Assert(source == target, "Test failed");
    }

    ///////////////////////////////////////////////////////////////
    // Spacing tests
    {
        const LyShine::StringType rawCharData("this  is a test!");
        LyShine::StringType source = "this  <i>is</i> a <b>test</b>!";
        LyShine::StringType expectedMarkup = "<root><ch value=\"this  \" /><i><ch value=\"is\" /></i><ch value=\" a \" /><b><ch value=\"test\" /></b><ch value=\"!\" /></root>";

        LyShine::StringType markupTarget;
        InsertMarkup(source, markupTarget);
        AZ_Assert(expectedMarkup == markupTarget, "Test failed");

        LyShine::StringType target;
        TextMarkup::CopyCharData(source, target);
        AZ_Assert(rawCharData == target, "Test failed");
    }

    {
        const LyShine::StringType rawCharData(" thisis a test!");
        LyShine::StringType source = " this<i>is</i> a <b>test</b>!";
        LyShine::StringType expectedMarkup = "<root><ch value=\" this\" /><i><ch value=\"is\" /></i><ch value=\" a \" /><b><ch value=\"test\" /></b><ch value=\"!\" /></root>";

        LyShine::StringType markupTarget;
        InsertMarkup(source, markupTarget);
        AZ_Assert(expectedMarkup == markupTarget, "Test failed");

        LyShine::StringType target;
        TextMarkup::CopyCharData(source, target);
        AZ_Assert(rawCharData == target, "Test failed");
    }

    {
        const LyShine::StringType rawCharData(" this is a test!");
        LyShine::StringType source = " this <i>is</i> a <b>test</b>!";
        LyShine::StringType expectedMarkup = "<root><ch value=\" this \" /><i><ch value=\"is\" /></i><ch value=\" a \" /><b><ch value=\"test\" /></b><ch value=\"!\" /></root>";

        LyShine::StringType markupTarget;
        InsertMarkup(source, markupTarget);
        AZ_Assert(expectedMarkup == markupTarget, "Test failed");

        LyShine::StringType target;
        TextMarkup::CopyCharData(source, target);
        AZ_Assert(rawCharData == target, "Test failed");
    }

    {
        LyShine::StringType source = "<b>this</b> <i>is</i> a <b>test</b>!";
        LyShine::StringType expectedMarkup = "<root><b><ch value=\"this\" /></b><ch value=\" \" /><i><ch value=\"is\" /></i><ch value=\" a \" /><b><ch value=\"test\" /></b><ch value=\"!\" /></root>";

        LyShine::StringType markupTarget;
        InsertMarkup(source, markupTarget);
        AZ_Assert(expectedMarkup == markupTarget, "Test failed");

        LyShine::StringType target;
        TextMarkup::CopyCharData(source, target);
        AZ_Assert(rawCharData == target, "Test failed");
    }

    {
        const LyShine::StringType rawCharData("this  is a test!");
        LyShine::StringType source = "<b>this</b>  <i>is</i> a <b>test</b>!";
        LyShine::StringType expectedMarkup = "<root><b><ch value=\"this\" /></b><ch value=\"  \" /><i><ch value=\"is\" /></i><ch value=\" a \" /><b><ch value=\"test\" /></b><ch value=\"!\" /></root>";

        LyShine::StringType markupTarget;
        InsertMarkup(source, markupTarget);
        AZ_Assert(expectedMarkup == markupTarget, "Test failed");

        LyShine::StringType target;
        TextMarkup::CopyCharData(source, target);
        AZ_Assert(rawCharData == target, "Test failed");
    }

    {
        const LyShine::StringType rawCharData("this                         is a test!");
        LyShine::StringType source = "<b>this</b>                         <i>is</i> a <b>test</b>!";
        LyShine::StringType expectedMarkup = "<root><b><ch value=\"this\" /></b><ch value=\"                         \" /><i><ch value=\"is\" /></i><ch value=\" a \" /><b><ch value=\"test\" /></b><ch value=\"!\" /></root>";

        LyShine::StringType markupTarget;
        InsertMarkup(source, markupTarget);
        AZ_Assert(expectedMarkup == markupTarget, "Test failed");

        LyShine::StringType target;
        TextMarkup::CopyCharData(source, target);
        AZ_Assert(rawCharData == target, "Test failed");
    }

    {
        LyShine::StringType source = "<b>this</b><i></i> <i>is</i> a <b>test</b>!";
        LyShine::StringType expectedMarkup = "<root><b><ch value=\"this\" /></b><i></i><ch value=\" \" /><i><ch value=\"is\" /></i><ch value=\" a \" /><b><ch value=\"test\" /></b><ch value=\"!\" /></root>";

        LyShine::StringType markupTarget;
        InsertMarkup(source, markupTarget);
        AZ_Assert(expectedMarkup == markupTarget, "Test failed");

        LyShine::StringType target;
        TextMarkup::CopyCharData(source, target);
        AZ_Assert(rawCharData == target, "Test failed");
    }

    {
        LyShine::StringType source = "<b>this</b> <i></i><i>is</i> a <b>test</b>!";
        LyShine::StringType expectedMarkup = "<root><b><ch value=\"this\" /></b><ch value=\" \" /><i></i><i><ch value=\"is\" /></i><ch value=\" a \" /><b><ch value=\"test\" /></b><ch value=\"!\" /></root>";

        LyShine::StringType markupTarget;
        InsertMarkup(source, markupTarget);
        AZ_Assert(expectedMarkup == markupTarget, "Test failed");

        LyShine::StringType target;
        TextMarkup::CopyCharData(source, target);
        AZ_Assert(rawCharData == target, "Test failed");
    }

    {
        LyShine::StringType source = "<b>this</b><i></i> <b></b><i>is</i> a <b>test</b>!";
        LyShine::StringType expectedMarkup = "<root><b><ch value=\"this\" /></b><i></i><ch value=\" \" /><b></b><i><ch value=\"is\" /></i><ch value=\" a \" /><b><ch value=\"test\" /></b><ch value=\"!\" /></root>";

        LyShine::StringType markupTarget;
        InsertMarkup(source, markupTarget);
        AZ_Assert(expectedMarkup == markupTarget, "Test failed");

        LyShine::StringType target;
        TextMarkup::CopyCharData(source, target);
        AZ_Assert(rawCharData == target, "Test failed");
    }

    ///////////////////////////////////////////////////////////////
    // Spacing tests: character escaping
    {
        const LyShine::StringType rawCharData("&  1");
        LyShine::StringType source = "&amp;  1";
        LyShine::StringType expectedMarkup = "<root><ch value=\"&amp;  1\" /></root>";

        LyShine::StringType markupTarget;
        InsertMarkup(source, markupTarget);
        AZ_Assert(expectedMarkup == markupTarget, "Test failed");

        LyShine::StringType target;
        TextMarkup::CopyCharData(source, target);
        AZ_Assert(rawCharData == target, "Test failed");
    }

    {
        const LyShine::StringType rawCharData("& 1");
        LyShine::StringType source = "&amp; 1";
        LyShine::StringType expectedMarkup = "<root><ch value=\"&amp; 1\" /></root>";

        LyShine::StringType markupTarget;
        InsertMarkup(source, markupTarget);
        AZ_Assert(expectedMarkup == markupTarget, "Test failed");

        LyShine::StringType target;
        TextMarkup::CopyCharData(source, target);
        AZ_Assert(rawCharData == target, "Test failed");
    }

    {
        const LyShine::StringType rawCharData("& ");
        LyShine::StringType source = "&amp; ";
        LyShine::StringType expectedMarkup = "<root><ch value=\"&amp; \" /></root>";

        LyShine::StringType markupTarget;
        InsertMarkup(source, markupTarget);
        AZ_Assert(expectedMarkup == markupTarget, "Test failed");

        LyShine::StringType target;
        TextMarkup::CopyCharData(source, target);
        AZ_Assert(rawCharData == target, "Test failed");
    }

    {
        const LyShine::StringType rawCharData(" &");
        LyShine::StringType source = " &amp;";
        LyShine::StringType expectedMarkup = "<root><ch value=\" &amp;\" /></root>";

        LyShine::StringType markupTarget;
        InsertMarkup(source, markupTarget);
        AZ_Assert(expectedMarkup == markupTarget, "Test failed");

        LyShine::StringType target;
        TextMarkup::CopyCharData(source, target);
        AZ_Assert(rawCharData == target, "Test failed");
    }

    {
        const LyShine::StringType rawCharData(" & ");
        LyShine::StringType source = " &amp; ";
        LyShine::StringType expectedMarkup = "<root><ch value=\" &amp; \" /></root>";

        LyShine::StringType markupTarget;
        InsertMarkup(source, markupTarget);
        AZ_Assert(expectedMarkup == markupTarget, "Test failed");

        LyShine::StringType target;
        TextMarkup::CopyCharData(source, target);
        AZ_Assert(rawCharData == target, "Test failed");
    }

    {
        const LyShine::StringType rawCharData("&1 ");
        LyShine::StringType source = "&amp;1 ";
        LyShine::StringType expectedMarkup = "<root><ch value=\"&amp;1 \" /></root>";

        LyShine::StringType markupTarget;
        InsertMarkup(source, markupTarget);
        AZ_Assert(expectedMarkup == markupTarget, "Test failed");

        LyShine::StringType target;
        TextMarkup::CopyCharData(source, target);
        AZ_Assert(rawCharData == target, "Test failed");
    }

    {
        const LyShine::StringType rawCharData("& 1");
        LyShine::StringType source = "&amp; 1";
        LyShine::StringType expectedMarkup = "<root><ch value=\"&amp; 1\" /></root>";

        LyShine::StringType markupTarget;
        InsertMarkup(source, markupTarget);
        AZ_Assert(expectedMarkup == markupTarget, "Test failed");

        LyShine::StringType target;
        TextMarkup::CopyCharData(source, target);
        AZ_Assert(rawCharData == target, "Test failed");
    }

    {
        const LyShine::StringType rawCharData("1&");
        LyShine::StringType source = "1&amp;";
        LyShine::StringType expectedMarkup = "<root><ch value=\"1&amp;\" /></root>";

        LyShine::StringType markupTarget;
        InsertMarkup(source, markupTarget);
        AZ_Assert(expectedMarkup == markupTarget, "Test failed");

        LyShine::StringType target;
        TextMarkup::CopyCharData(source, target);
        AZ_Assert(rawCharData == target, "Test failed");
    }

    {
        const LyShine::StringType rawCharData("1& ");
        LyShine::StringType source = "1&amp; ";
        LyShine::StringType expectedMarkup = "<root><ch value=\"1&amp; \" /></root>";

        LyShine::StringType markupTarget;
        InsertMarkup(source, markupTarget);
        AZ_Assert(expectedMarkup == markupTarget, "Test failed");

        LyShine::StringType target;
        TextMarkup::CopyCharData(source, target);
        AZ_Assert(rawCharData == target, "Test failed");
    }

    {
        const LyShine::StringType rawCharData("1 &");
        LyShine::StringType source = "1 &amp;";
        LyShine::StringType expectedMarkup = "<root><ch value=\"1 &amp;\" /></root>";

        LyShine::StringType markupTarget;
        InsertMarkup(source, markupTarget);
        AZ_Assert(expectedMarkup == markupTarget, "Test failed");

        LyShine::StringType target;
        TextMarkup::CopyCharData(source, target);
        AZ_Assert(rawCharData == target, "Test failed");
    }

    {
        const LyShine::StringType rawCharData(" 1&");
        LyShine::StringType source = " 1&amp;";
        LyShine::StringType expectedMarkup = "<root><ch value=\" 1&amp;\" /></root>";

        LyShine::StringType markupTarget;
        InsertMarkup(source, markupTarget);
        AZ_Assert(expectedMarkup == markupTarget, "Test failed");

        LyShine::StringType target;
        TextMarkup::CopyCharData(source, target);
        AZ_Assert(rawCharData == target, "Test failed");
    }

    {
        const LyShine::StringType rawCharData("&1 &");
        LyShine::StringType source = "&amp;1 &amp;";
        LyShine::StringType expectedMarkup = "<root><ch value=\"&amp;1 &amp;\" /></root>";

        LyShine::StringType markupTarget;
        InsertMarkup(source, markupTarget);
        AZ_Assert(expectedMarkup == markupTarget, "Test failed");

        LyShine::StringType target;
        TextMarkup::CopyCharData(source, target);
        AZ_Assert(rawCharData == target, "Test failed");
    }

    {
        const LyShine::StringType rawCharData("& 1&");
        LyShine::StringType source = "&amp; 1&amp;";
        LyShine::StringType expectedMarkup = "<root><ch value=\"&amp; 1&amp;\" /></root>";

        LyShine::StringType markupTarget;
        InsertMarkup(source, markupTarget);
        AZ_Assert(expectedMarkup == markupTarget, "Test failed");

        LyShine::StringType target;
        TextMarkup::CopyCharData(source, target);
        AZ_Assert(rawCharData == target, "Test failed");
    }

    {
        const LyShine::StringType rawCharData("& 1& ");
        LyShine::StringType source = "&amp; 1&amp; ";
        LyShine::StringType expectedMarkup = "<root><ch value=\"&amp; 1&amp; \" /></root>";

        LyShine::StringType markupTarget;
        InsertMarkup(source, markupTarget);
        AZ_Assert(expectedMarkup == markupTarget, "Test failed");

        LyShine::StringType target;
        TextMarkup::CopyCharData(source, target);
        AZ_Assert(rawCharData == target, "Test failed");
    }

    {
        const LyShine::StringType rawCharData("&1 & ");
        LyShine::StringType source = "&amp;1 &amp; ";
        LyShine::StringType expectedMarkup = "<root><ch value=\"&amp;1 &amp; \" /></root>";

        LyShine::StringType markupTarget;
        InsertMarkup(source, markupTarget);
        AZ_Assert(expectedMarkup == markupTarget, "Test failed");

        LyShine::StringType target;
        TextMarkup::CopyCharData(source, target);
        AZ_Assert(rawCharData == target, "Test failed");
    }

    {
        const LyShine::StringType rawCharData(" &1 & ");
        LyShine::StringType source = " &amp;1 &amp; ";
        LyShine::StringType expectedMarkup = "<root><ch value=\" &amp;1 &amp; \" /></root>";

        LyShine::StringType markupTarget;
        InsertMarkup(source, markupTarget);
        AZ_Assert(expectedMarkup == markupTarget, "Test failed");

        LyShine::StringType target;
        TextMarkup::CopyCharData(source, target);
        AZ_Assert(rawCharData == target, "Test failed");
    }

    {
        const LyShine::StringType rawCharData("& 1 &");
        LyShine::StringType source = "&amp; 1 &amp;";
        LyShine::StringType expectedMarkup = "<root><ch value=\"&amp; 1 &amp;\" /></root>";

        LyShine::StringType markupTarget;
        InsertMarkup(source, markupTarget);
        AZ_Assert(expectedMarkup == markupTarget, "Test failed");

        LyShine::StringType target;
        TextMarkup::CopyCharData(source, target);
        AZ_Assert(rawCharData == target, "Test failed");
    }

    {
        const LyShine::StringType rawCharData("& &");
        LyShine::StringType source = "&amp; &amp;";
        LyShine::StringType expectedMarkup = "<root><ch value=\"&amp; &amp;\" /></root>";

        LyShine::StringType markupTarget;
        InsertMarkup(source, markupTarget);
        AZ_Assert(expectedMarkup == markupTarget, "Test failed");

        LyShine::StringType target;
        TextMarkup::CopyCharData(source, target);
        AZ_Assert(rawCharData == target, "Test failed");
    }

    ///////////////////////////////////////////////////////////////
    // Pass-thru tests
    {
        LyShine::StringType source = " this is a test!";
        LyShine::StringType expectedMarkup = "<root><ch value=\" this is a test!\" /></root>";

        LyShine::StringType markupTarget;
        InsertMarkup(source, markupTarget);
        AZ_Assert(expectedMarkup == markupTarget, "Test failed");

        LyShine::StringType target;
        TextMarkup::CopyCharData(source, target);
        AZ_Assert(source == target, "Test failed");
    }

    {
        LyShine::StringType source = "this is a test! ";
        LyShine::StringType expectedMarkup = "<root><ch value=\"this is a test! \" /></root>";

        LyShine::StringType markupTarget;
        InsertMarkup(source, markupTarget);
        AZ_Assert(expectedMarkup == markupTarget, "Test failed");

        LyShine::StringType target;
        TextMarkup::CopyCharData(source, target);
        AZ_Assert(source == target, "Test failed");
    }

    {
        LyShine::StringType source;
        LyShine::StringType expectedMarkup = "<root></root>";;
        LyShine::StringType markupTarget;
        InsertMarkup(source, markupTarget);
        AZ_Assert(expectedMarkup == markupTarget, "Test failed");

        LyShine::StringType target;
        TextMarkup::CopyCharData(rawCharData, target);
        AZ_Assert(rawCharData == target, "Test failed");
    }

    ///////////////////////////////////////////////////////////////
    // Bad xml tests
    {
        LyShine::StringType source = "<this <i>is</i> a <b>test</b>!";
        LyShine::StringType expectedMarkup = "<root><this <i>is</i> a <b>test</b>!</root>";

        LyShine::StringType target;
        TextMarkup::CopyCharData(source, target);
        AZ_Assert(source == target, "Test failed");
    }

    {
        LyShine::StringType source = "<<this <i>is</i> a <b>test</b>!";
        LyShine::StringType target;
        TextMarkup::CopyCharData(source, target);
        AZ_Assert(source == target, "Test failed");
    }

    {
        LyShine::StringType source = "this<badtag></badtag> <i>is</i> a <b>test</b>!";
        LyShine::StringType target;
        TextMarkup::CopyCharData(source, target);
        AZ_Assert(source == target, "Test failed");
    }

    {
        LyShine::StringType source = "<       this <i>is</i> a <b>test</b>!";
        LyShine::StringType target;
        TextMarkup::CopyCharData(source, target);
        AZ_Assert(source == target, "Test failed");
    }

    {
        LyShine::StringType source = "<>this <i>is</i> a <b>test</b>!";
        LyShine::StringType target;
        TextMarkup::CopyCharData(source, target);
        AZ_Assert(source == target, "Test failed");
    }

    {
        LyShine::StringType source = "<font face=\"times\">this</font></b> <i>is</i> a <b>test</b>!";
        LyShine::StringType target;
        TextMarkup::CopyCharData(source, target);
        AZ_Assert(source == target, "Test failed");
    }
}
#endif