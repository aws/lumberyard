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

#include "ActionWidget.h"

#include <QVariant>
#include <QVBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QDesktopServices>
#include <QUrl>
#include <QCursor>

#include "ButtonBarWidget.h"
#include "HeadingWidget.h"

#include <DetailWidget/ActionWidget.moc>

ActionWidget::ActionWidget(QWidget* parent)
    : QFrame(parent)
{
    CreateUI();

    // default content

    SetLearnMoreMessageText(tr(
            "Cloud Canvas enables you to use AWS resources in your Lumberyard project. For "
            "more information on getting started with Cloud Canvas, check out our Getting "
            "Started Guide, documentation, or tutorials."
            ));

    AddCloudCanvasDocumentationLink();
    AddCloudCanvasTutorialsLink();
}

void ActionWidget::CreateUI()
{
    setObjectName("Action");

    // root

    auto rootLayout = new QVBoxLayout {};
    rootLayout->setSpacing(0);
    rootLayout->setMargin(0);
    setLayout(rootLayout);

    // heading

    m_headingWidget = new HeadingWidget {this};
    m_headingWidget->HideRefresh();
    rootLayout->addWidget(m_headingWidget);

    // button bar

    m_buttonBarWidget = new ButtonBarWidget {};
    rootLayout->addWidget(m_buttonBarWidget);

    // button bar - action button

    m_actionButton = new QPushButton("(SetActioButtonText)");
    m_actionButton->setObjectName("ActionButton");
    m_actionButton->setProperty("class", "Primary");
    connect(m_actionButton, &QPushButton::clicked, this, &ActionWidget::ActionClicked);
    m_buttonBarWidget->AddButton(m_actionButton);

    // learn more

    auto learnMoreWidget = new QWidget {};
    learnMoreWidget->setObjectName("LearnMore");
    rootLayout->addWidget(learnMoreWidget);

    m_learnMoreLayout = new QVBoxLayout {};
    m_learnMoreLayout->setMargin(0);
    m_learnMoreLayout->setSpacing(0);
    learnMoreWidget->setLayout(m_learnMoreLayout);

    // learn more title

    auto learnMore = new QLabel("Learn more");
    learnMore->setObjectName("Title");
    learnMore->setWordWrap(true);
    m_learnMoreLayout->addWidget(learnMore);

    // learn more text

    m_learnMoreMessageLabel = new QLabel {
        "(SetLearnMoreMessageText)"
    };
    m_learnMoreMessageLabel->setObjectName("Message");
    m_learnMoreMessageLabel->setWordWrap(true);
    m_learnMoreLayout->addWidget(m_learnMoreMessageLabel);

    // stretch

    m_learnMoreLayout->addStretch();
}

void ActionWidget::SetTitleText(const QString& text)
{
    m_headingWidget->SetTitleText(text);
}

void ActionWidget::SetMessageText(const QString& text)
{
    m_headingWidget->SetMessageText(text);
}

void ActionWidget::SetActionText(const QString& text)
{
    m_actionButton->setText(text);
}

void ActionWidget::SetActionToolTip(const QString& text)
{
    m_actionButton->setToolTip(text);
}

void ActionWidget::AddButton(QPushButton* button)
{
    // insert before stretch
    m_buttonBarWidget->AddButton(button);
}

void ActionWidget::SetLearnMoreMessageText(const QString& text)
{
    m_learnMoreMessageLabel->setText(text);
}

void ActionWidget::AddLearnMoreLink(const QString& text, const QString& url)
{
    auto link = new QPushButton(text);
    link->setObjectName("LinkButton");
    link->setCursor(QCursor {Qt::PointingHandCursor});
    link->setSizePolicy(QSizePolicy::Policy::Fixed, QSizePolicy::Policy::Fixed);
    connect(link, &QPushButton::clicked, this, [this, url]() { OnLinkActivated(url); });
    m_learnMoreLayout->insertWidget(m_learnMoreLayout->count() - 1, link);
}

void ActionWidget::AddCloudCanvasDocumentationLink()
{
    AddLearnMoreLink("Cloud Canvas documentation", "http://docs.aws.amazon.com/lumberyard/latest/developerguide/cloud-canvas-intro.html");
}

void ActionWidget::AddCloudCanvasTutorialsLink()
{
    AddLearnMoreLink("Cloud Canvas tutorial", "http://docs.aws.amazon.com/lumberyard/latest/developerguide/cloud-canvas-tutorial.html");
}

void ActionWidget::AddCloudFormationDocumentationLink()
{
    AddLearnMoreLink("AWS CloudFormation documentation", "https://aws.amazon.com/cloudformation");
}

void ActionWidget::OnLinkActivated(const QString& link)
{
    QDesktopServices::openUrl(link);
}

QPushButton* ActionWidget::GetActionButton()
{
    return m_actionButton;
}
