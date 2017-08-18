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

#define UICANVASEDITOR_QTOOLBAR_SEPARATOR_STYLESHEET    "QToolBar::separator { background-color: rgb(90, 90, 90); }"

MainToolbar::MainToolbar(EditorWindow* parent)
    : QToolBar("Main Toolbar", parent)
    , m_newElementToolbarSection(new NewElementToolbarSection(this, true))
    , m_coordinateSystemToolbarSection(new CoordinateSystemToolbarSection(this, true))
    , m_canvasSizeToolbarSection(new ReferenceCanvasSizeToolbarSection(this, false))
    , m_zoomFactorLabel(new QLabel(parent))
{
    setObjectName("MainToolbar");    // needed to save state
    setFloatable(false);

    setStyleSheet(UICANVASEDITOR_QTOOLBAR_SEPARATOR_STYLESHEET);

    // Zoom factor.
    addWidget(m_zoomFactorLabel);

    parent->addToolBar(this);
}

QLabel* MainToolbar::GetZoomFactorLabel()
{
    return m_zoomFactorLabel;
}

#include <MainToolbar.moc>
