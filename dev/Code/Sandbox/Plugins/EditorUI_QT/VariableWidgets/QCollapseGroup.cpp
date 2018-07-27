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

#include "QCollapseGroup.h"
#include <QMenu>
#include <QMouseEvent>


QCollapseGroup::QCollapseGroup(const QString& title, QWidget* parent)
    : QWidget(parent)
    , m_layout(this)
    , m_groupBox(title, this)
{
    m_layout.setMargin(0);
    m_layout.setSizeConstraint(QLayout::SetNoConstraint);
    setAttribute(Qt::WA_TranslucentBackground, true);

    m_groupBox.setCheckable(true);

    m_layout.addWidget(&m_groupBox);
    setLayout(&m_layout);
}
