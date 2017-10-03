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

// include the required headers
#include "SearchButton.h"
#include <MCore/Source/LogManager.h>
#include <MCore/Source/UnicodeString.h>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QLineEdit>


namespace MysticQt
{
    SearchButton::SearchButton(QWidget* parent, const QIcon& searchClearButton)
        : QWidget(parent)
    {
        MCORE_UNUSED(searchClearButton);
        mLayout = new QHBoxLayout(this);
        mLayout->setMargin(0);
        mLayout->setSpacing(0);

        mSearchEdit = new QLineEdit(this);
        QPushButton* closeButton = new QPushButton("", this);
        //closeButton->setIcon(searchClearButton);

        closeButton->setObjectName("SearchClearButton");
        mSearchEdit->setObjectName("SearchEdit");

        closeButton->setMaximumHeight(20);

        //mSearchEdit->setStyleSheet("border-top-right-radius: 0px; border-right: 0px; border-bottom-right-radius: 0px;");
        //closeButton->setStyleSheet("border-top-left-radius: 0px; border-bottom-left-radius: 0px; border-left: 0px; background-color: rgb(144, 152, 160);");

        mLayout->addWidget(mSearchEdit);
        mLayout->addWidget(closeButton);

        connect(mSearchEdit, SIGNAL(textChanged(const QString&)), this, SLOT(TextChanged(const QString&)));
        connect(closeButton, SIGNAL(pressed()), this, SLOT(ClearSearch()));

        setMaximumWidth(250);

        ClearSearch();
    }


    SearchButton::~SearchButton()
    {
    }


    void SearchButton::TextChanged(const QString& text)
    {
        MCORE_UNUSED(text);
        //LOG("%s", text.toAscii().data());
    }


    void SearchButton::ClearSearch()
    {
        mSearchEdit->setText("");
    }


    void SearchButton::focusInEvent(QFocusEvent* event)
    {
        MCORE_UNUSED(event);
        //mSearchEdit->setText("");
    }
} // namespace MysticQt

#include <MysticQt/Source/SearchButton.moc>