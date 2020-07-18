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
#pragma once

#include <AzQtComponents/AzQtComponentsAPI.h>
#include <QFrame>
#include <QString>

class QVBoxLayout;
class QPushButton;
class QVBoxLayout;
class QIcon;

namespace AzQtComponents
{
    /**
     * Card Notification
     */
    class AZ_QT_COMPONENTS_API CardNotification
        : public QFrame
    {
        Q_OBJECT
    public:
        CardNotification(QWidget* parent, const QString& title, const QIcon& icon, const QSize = {24, 24});

        void addFeature(QWidget* feature);
        QPushButton* addButtonFeature(const QString& buttonText);

    private:
        QVBoxLayout* m_featureLayout = nullptr;
    };
} // namespace AzQtComponents
