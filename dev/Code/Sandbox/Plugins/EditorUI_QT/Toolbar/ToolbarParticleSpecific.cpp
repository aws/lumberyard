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
#include "EditorUI_QT_Precompiled.h"
#include "ToolbarParticleSpecific.h"

#include <QtWidgets/QToolBar>
#include <QtCore/QCoreApplication>

CToolbarParticleSpecific::CToolbarParticleSpecific(QMainWindow* mainWindow)
    : IToolbar(mainWindow, TO_CSTR(CToolbarParticleSpecific))
{
    setWindowTitle("Particle Specific");
}

CToolbarParticleSpecific::~CToolbarParticleSpecific()
{
}

void CToolbarParticleSpecific::init()
{
    createAction("actionParticleSpecialReset",      "Reset",        "Reset item to default",                "particleSpecialReset.png");
    createAction("actionParticleSpecialActivate",   "Activate",     "Enable/Disable item and all children", "particleSpecialActivate.png");
}