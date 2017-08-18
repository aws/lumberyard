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
// Original file Copyright Crytek GMBH or its affiliates, used under license.

#include "StdAfx.h"
#include "MannDebugOptionsDialog.h"
#include "MannequinBase.h"
#include "MannequinModelViewport.h"
#include <Controls/ReflectedPropertyControl/ReflectedPropertiesPanel.h>

#include <QVBoxLayout>

CMannequinDebugOptionsDialog::CMannequinDebugOptionsDialog(CMannequinModelViewport* pModelViewport, QWidget* pParent)
    : QDialog(pParent)
    , m_pModelViewport(pModelViewport)
    , m_propPanel(new ReflectedPropertiesPanel(this))
{
    m_propPanel->Setup();
    setWindowTitle(tr("Debug Options"));
    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->addWidget(m_propPanel);
    layout->setMargin(0);
    m_propPanel->AddVars(m_pModelViewport->GetVarObject()->GetVarBlock());
    setWindowFlags(windowFlags() & ~(Qt::WindowContextHelpButtonHint | Qt::WindowMaximizeButtonHint));
    setFixedSize(500, 600);
}
