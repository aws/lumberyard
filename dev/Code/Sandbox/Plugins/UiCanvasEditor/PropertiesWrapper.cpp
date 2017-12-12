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

#include "EditorCommon.h"

PropertiesWrapper::PropertiesWrapper(HierarchyWidget* hierarchy,
    EditorWindow* parent)
    : QWidget(parent)
    , m_properties(new PropertiesWidget(parent, this))
{
    AZ_Assert(parent, "Parent EditorWindow is null");

    QVBoxLayout* outerLayout = new QVBoxLayout(this);

    QVBoxLayout* innerLayout = new QVBoxLayout();
    {
        innerLayout->setContentsMargins(4, 4, 4, 4);
        innerLayout->setSpacing(2);

        QLabel* elementName = new QLabel(this);
        elementName->setObjectName(QStringLiteral("m_elementName"));
        elementName->setText("Canvas");

        innerLayout->addWidget(elementName);

        m_properties->SetSelectedEntityDisplayNameWidget(elementName);

        innerLayout->addWidget(new ComponentButton(hierarchy, this));
    }
    outerLayout->addLayout(innerLayout);

    outerLayout->addWidget(m_properties);

    outerLayout->setContentsMargins(0, 0, 0, 0);
    outerLayout->setSpacing(0);
}

PropertiesWidget* PropertiesWrapper::GetProperties()
{
    return m_properties;
}

#include <PropertiesWrapper.moc>
