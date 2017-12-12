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

#include "QBackgroundLambda.h"

#include <QBackgroundLambda.moc>

void RunBackgroundLambda(QBackgroundLambdaBase* backgroundTask)
{
    if (!backgroundTask)
    {
        return;
    }

    QThread* thread = new QThread;
    backgroundTask->moveToThread(thread);
    QWidget::connect(thread, SIGNAL(started()), backgroundTask, SLOT(Run()));
    QWidget::connect(backgroundTask, SIGNAL(finished()), thread, SLOT(quit()));
    QWidget::connect(backgroundTask, SIGNAL(finished()), backgroundTask, SLOT(deleteLater()));
    QWidget::connect(thread, SIGNAL(finished()), thread, SLOT(deleteLater()));
    thread->start();
}
