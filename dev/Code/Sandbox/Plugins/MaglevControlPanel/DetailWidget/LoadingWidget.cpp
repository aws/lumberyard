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
#include "stdafx.h"

#include "IEditor.h"
#include "EditorCoreAPI.h"

#include "LoadingWidget.h"

#include <QVariant>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>

#include "HeadingWidget.h"
#include "CloudCanvasLogger.h"

#include <DetailWidget/LoadingWidget.moc>
#include <ResourceManagementView.h>

LoadingWidget::LoadingWidget(ResourceManagementView* view, QWidget* parent)
    : DetailWidget {view, parent}
{

    // root

    setObjectName("Loading");

    auto rootLayout = new QVBoxLayout {};
    rootLayout->setSpacing(0);
    rootLayout->setMargin(0);
    setLayout(rootLayout);

    // heading

    auto loadingLabel = new QLabel(tr("Loading..."));
    loadingLabel->setObjectName("Title");
    loadingLabel->setWordWrap(true);
    loadingLabel->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Ignored);
    rootLayout->addWidget(loadingLabel);

    // message

    auto messageLabel = new QLabel(tr("Please wait while the Cloud Canvas configuration is loaded."));
    messageLabel->setObjectName("Message");
    messageLabel->setWordWrap(true);
    messageLabel->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Ignored);
    rootLayout->addWidget(messageLabel);

    // stretch

    rootLayout->addStretch();
}



