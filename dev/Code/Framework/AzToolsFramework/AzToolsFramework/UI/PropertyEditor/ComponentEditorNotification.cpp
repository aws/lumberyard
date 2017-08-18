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
#include "StdAfx.h"
#include "ComponentEditorNotification.hxx"

#include <QtWidgets/QHBoxLayout>
#include <QStyleOption>
#include <QPainter>
#include <QStylePainter>

namespace AzToolsFramework
{
    ComponentEditorNotification::ComponentEditorNotification(QWidget* parent, const QString& title, const QIcon& icon)
        : QFrame(parent)
    {
		//this should work based off the class name when properly configured n the stylesheet
		setObjectName("ComponentEditorNotification");
        setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Maximum);

        m_headerFrame = aznew QFrame(this);
        m_headerFrame->setObjectName("HeaderFrame");
        m_headerFrame->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Maximum);

		// icon widget
        m_iconLabel = aznew QLabel(m_headerFrame);
		m_iconLabel->setObjectName("Icon");
		m_iconLabel->setPixmap(icon.pixmap(icon.availableSizes().front()));

        // title widget
        m_titleLabel = aznew QLabel(title, m_headerFrame);
        m_titleLabel->setObjectName("Title");
        m_titleLabel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
        m_titleLabel->setWordWrap(true);

        auto headerLayout = aznew QHBoxLayout(m_headerFrame);
        headerLayout->setSizeConstraint(QLayout::SetMinimumSize);
        headerLayout->addWidget(m_iconLabel);
        headerLayout->addWidget(m_titleLabel);
        m_headerFrame->setLayout(headerLayout);

        m_featureLayout = aznew QVBoxLayout(this);
        m_featureLayout->setSizeConstraint(QLayout::SetMinimumSize);
        m_featureLayout->addWidget(m_headerFrame);
        setLayout(m_featureLayout);
	}

    void ComponentEditorNotification::AddFeature(QWidget* feature)
    {
        feature->setParent(this);
        m_featureLayout->addWidget(feature);
    }
}

#include <UI/PropertyEditor/ComponentEditorNotification.moc>
