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

#include <AzQtComponents/Components/Widgets/Text.h>
#include <AzQtComponents/Components/Style.h>

#include <QLabel>
#include <QString>

namespace AzQtComponents
{
    static QString g_headlineClass = QStringLiteral("Headline");
    static QString g_titleClass = QStringLiteral("Title");
    static QString g_subtitleClass = QStringLiteral("Subtitle");
    static QString g_paragraphClass = QStringLiteral("Paragraph");

    static QString g_primaryClass = QStringLiteral("primaryText");
    static QString g_secondaryClass = QStringLiteral("secondaryText");
    static QString g_highlightedClass = QStringLiteral("highlightedText");
    static QString g_blackClass = QStringLiteral("blackText");

    void Text::addHeadlineStyle(QLabel* text)
    {
        Style::addClass(text, g_headlineClass);
    }

    void Text::addTitleStyle(QLabel* text)
    {
        Style::addClass(text, g_titleClass);
    }

    void Text::addSubtitleStyle(QLabel* text)
    {
        Style::addClass(text, g_subtitleClass);
    }

    void Text::addParagraphStyle(QLabel* text)
    {
        Style::addClass(text, g_paragraphClass);
    }

    void Text::addPrimaryStyle(QLabel* text)
    {
        Style::addClass(text, g_primaryClass);
    }

    void Text::addSecondaryStyle(QLabel* text)
    {
        Style::addClass(text, g_secondaryClass);
    }

    void Text::addHighlightedStyle(QLabel* text)
    {
        Style::addClass(text, g_highlightedClass);
    }

    void Text::addBlackStyle(QLabel* text)
    {
        Style::addClass(text, g_blackClass);
    }
} // namespace AzQtComponents
