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

#include <QFrame>

class QWidget;
class QLabel;
class QVBoxLayout;

namespace AzToolsFramework
{
    class ComponentEditorNotification
        : public QFrame
    {
		//Q_OBJECT;

    public:
        ComponentEditorNotification(QWidget* parent, const QString& title, const QIcon& icon);
        void AddFeature(QWidget* feature);

    protected:

		QFrame* m_headerFrame = nullptr;
		QLabel* m_iconLabel = nullptr;
        QLabel* m_titleLabel = nullptr;
        QVBoxLayout* m_featureLayout = nullptr;
    };
}