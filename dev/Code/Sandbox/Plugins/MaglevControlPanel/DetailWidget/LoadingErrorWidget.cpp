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

#include <stdafx.h>

#include "LoadingErrorWidget.h"

#include <QVariant>
#include <QVBoxLayout>
#include <QLabel>
#include <QPushButton>

#include "HeadingWidget.h"
#include "IAWSResourceManager.h"
#include "ResourceManagementView.h"

#include <DetailWidget/LoadingErrorWidget.moc>

#include "IEditor.h"

LoadingErrorWidget::LoadingErrorWidget(ResourceManagementView* view, QWidget* parent)
    : DetailWidget{view, parent}
{
    // root

    setObjectName("LoadingError");

    auto rootLayout = new QVBoxLayout{};
    rootLayout->setSpacing(0);
    rootLayout->setMargin(0);
    setLayout(rootLayout);

    // heading

    auto loadingLabel = new QLabel(tr("Error Loading Configuration"));
    loadingLabel->setObjectName("Title");
    loadingLabel->setWordWrap(true);
    loadingLabel->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Ignored);
    rootLayout->addWidget(loadingLabel);

    // message

    auto messageLabel = new QLabel(tr("There was an error loading the Cloud Canvas configuration:"));
    messageLabel->setObjectName("Message");
    messageLabel->setWordWrap(true);
    messageLabel->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Ignored);
    rootLayout->addWidget(messageLabel);

    // error(s)

    auto errorLabel = new QLabel {view->GetResourceManager()->GetErrorString()};
    errorLabel->setObjectName("ErrorText");
    errorLabel->setWordWrap(true);
    errorLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);
    errorLabel->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Ignored);
    rootLayout->addWidget(errorLabel);

    // put a little space between the message and the buttons
    rootLayout->addSpacing(20);

    // add layout for refresh and cancel buttons
    auto buttonLayout = new QHBoxLayout{};
    rootLayout->addLayout(buttonLayout);

    // refresh button
    auto retryButton = new QPushButton(tr("Retry"));
    retryButton->setToolTip("Retry the failed commands.");
    retryButton->setMaximumWidth(100);
    connectUntilHidden(retryButton, &QPushButton::clicked, this, &LoadingErrorWidget::OnRetryClicked);

    buttonLayout->addWidget(retryButton);

    buttonLayout->addSpacing(50);

    // cancel button

    auto cancelButton = new QPushButton(tr("Cancel"));
    cancelButton->setToolTip("Closes the Cloud Canvas Resource Manager window.");
    cancelButton->setMaximumWidth(100);
    connectUntilHidden(cancelButton, &QPushButton::clicked, this, &LoadingErrorWidget::OnCancelClicked);
    buttonLayout->addWidget(cancelButton);

    // button stretch

    buttonLayout->addStretch();

    // stretch

    rootLayout->addStretch();
}

void LoadingErrorWidget::OnRetryClicked()
{
    m_view->GetResourceManager()->RetryLoading();
}

void LoadingErrorWidget::OnCancelClicked()
{
    GetIEditor()->ExecuteCommand("general.close_pane 'Cloud Canvas Resource Manager'");
}

