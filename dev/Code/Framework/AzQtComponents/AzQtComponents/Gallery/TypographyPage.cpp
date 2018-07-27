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

#include "TypographyPage.h"
#include <Gallery/ui_TypographyPage.h>

#include <QLabel>
#include <AzQtComponents/Components/Style.h>
#include <AzQtComponents/Components/Widgets/Text.h>

static QLabel* label(QGridLayout* colorGrid, int row, int column)
{
    return qobject_cast<QLabel*>(colorGrid->itemAtPosition(row, column)->widget());
}

TypographyPage::TypographyPage(QWidget* parent)
: QWidget(parent)
, ui(new Ui::TypographyPage)
{
    ui->setupUi(this);

    AzQtComponents::Text::addHeadlineStyle(ui->m_headline);
    AzQtComponents::Text::addTitleStyle(ui->m_title);
    AzQtComponents::Text::addSubtitleStyle(ui->m_subTitle);
    AzQtComponents::Text::addParagraphStyle(ui->m_paragraph);

    AzQtComponents::Text::addPrimaryStyle(ui->m_primary);
    AzQtComponents::Text::addSecondaryStyle(ui->m_secondary);
    AzQtComponents::Text::addHighlightedStyle(ui->m_hightlighted);
    AzQtComponents::Text::addBlackStyle(ui->m_black);

    QGridLayout* colorGrid = ui->colorGrid;

    typedef void (*TextColorStyleCallback)(QLabel* QLabel);
    TextColorStyleCallback colorClassCallbacks[] = { &AzQtComponents::Text::addBlackStyle, &AzQtComponents::Text::addPrimaryStyle, &AzQtComponents::Text::addSecondaryStyle, &AzQtComponents::Text::addHighlightedStyle };
    for (int column = 0; column < 4; column++)
    {
        using namespace AzQtComponents;

        Text::addHeadlineStyle(label(colorGrid, 1, column));
        Text::addTitleStyle(label(colorGrid, 2, column));
        Text::addSubtitleStyle(label(colorGrid, 3, column));
        Text::addParagraphStyle(label(colorGrid, 5, column));

        for (int row = 1; row < 6; row++)
        {
            colorClassCallbacks[column](label(colorGrid, row, column));
        }
    }

    QString exampleText = R"(

QLabel docs: <a href="http://doc.qt.io/qt-5/qlabel.html">http://doc.qt.io/qt-5/qlabel.html</a><br/>

<pre>
#include &lt;QLabel&gt;
#include &lt;AzQtComponents/Components/Widgets/Text.h&gt;

QLabel* label;

// Assuming you've created a QLabel already (either in code or via .ui file):

// To apply various styles, use one (and only one) of the following:
AzQtComponents::Text::addHeadlineStyle(label);
AzQtComponents::Text::addTitleStyle(label);
AzQtComponents::Text::addSubtitleStyle(label);
AzQtComponents::Text::addParagraphStyle(label);

// To set various colors, use one (and only one) of the following:
AzQtComponents::Text::addPrimaryStyle(label);
AzQtComponents::Text::addSecondaryStyle(label);
AzQtComponents::Text::addHighlightedStyle(label);
AzQtComponents::Text::addBlackStyle(label);

// One text style and one text color can be applied at the same time on the same label.

/*
  Note that QLabel text display has two modes: Qt::PlainText and Qt::RichText.
  By default, it auto-selects the mode based on the text. If the text has HTML in it
  then it'll use Qt::RichText.
  This matters because \n means newline in PlainText mode, but <br> means newline in 
  RichText mode.
*/

</pre>

)";

    ui->exampleText->setHtml(exampleText);
}

TypographyPage::~TypographyPage()
{
}

#include <Gallery/TypographyPage.moc>