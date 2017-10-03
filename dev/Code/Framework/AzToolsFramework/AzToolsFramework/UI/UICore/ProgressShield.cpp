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

#include <AzCore/Component/TickBus.h>

#include "ProgressShield.hxx"
#include <UI/UICore/ui_ProgressShield.h>

namespace AzToolsFramework
{
    ProgressShield::ProgressShield(QWidget* pParent)
        : QWidget(pParent)
    {
        AZ_Assert(pParent && parentWidget(), "There must be a parent.");
        if (pParent && parentWidget())
        {
            uiConstructor = azcreate(Ui::progressShield, ());
            uiConstructor->setupUi(this);
            setAttribute(Qt::WA_StyledBackground, true);
            int width = parentWidget()->geometry().width();
            int height = parentWidget()->geometry().height();
            this->setGeometry(0, 0, width, height);
            pParent->installEventFilter(this);
            setProgress(0, 0, tr("Loading..."));
            UpdateGeometry();
        }
    }

    ProgressShield::~ProgressShield()
    {
        parentWidget()->removeEventFilter(this);
        azdestroy(uiConstructor);
    }

    void ProgressShield::LegacyShowAndWait(QWidget* parent, QString label, AZStd::function<bool(int& current, int& max)> completeCallback)
    {
        ProgressShield shield(parent);

        int current = 0, max = 0;
        shield.show();

        while (!completeCallback(current, max))
        {
            shield.setProgress(current, max, label);

            // re: 16 msec - pump QueuedEvents at a min of ~60 FPS

            QCoreApplication::processEvents(QEventLoop::ExcludeUserInputEvents |QEventLoop::WaitForMoreEvents, 16);
            AZ::TickBus::ExecuteQueuedEvents();
        }
    }

    void ProgressShield::UpdateGeometry()
    {
        int width = this->parentWidget()->geometry().width();
        int height =  this->parentWidget()->geometry().height();
        this->setGeometry(0, 0, width, height);
    }


    bool ProgressShield::eventFilter(QObject* obj, QEvent* event)
    {
        if (event->type() == QEvent::Resize)
        {
            UpdateGeometry();
        }

        // hand it to the owner
        return QObject::eventFilter(obj, event);
    }

    void ProgressShield::setProgress(int current, int max, const QString& label)
    {
        uiConstructor->progressLabel->setText(label);
        uiConstructor->progressLabel->show();
        uiConstructor->progressBar->setMaximum(max);
        uiConstructor->progressBar->setValue(current);
    }
}
#include <UI/UICore/ProgressShield.moc>