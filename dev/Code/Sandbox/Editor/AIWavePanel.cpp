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
#include "AIWavePanel.h"
#include "Objects/AIWave.h"

#include <QPushButton>
#include <QGroupBox>


//////////////////////////////////////////////////////////////////////////
CAIWavePanel::CAIWavePanel(QWidget* pParent /* = nullptr */)
    : CEntityPanel(pParent)
{
    m_prototypeButton->disconnect(this);
    connect(m_prototypeButton, &QPushButton::clicked, this, &CAIWavePanel::OnSelectAssignedAIs);
}

//////////////////////////////////////////////////////////////////////////
void CAIWavePanel::OnSelectAssignedAIs()
{
    GetIEditor()->GetObjectManager()->SelectAssignedEntities();
}

void CAIWavePanel::UpdateAssignedAIsPanel()
{
    size_t nAssignedAIs = GetIEditor()->GetObjectManager()->NumberOfAssignedEntities();

    m_frame->setTitle(QString(tr("Assigned AIs: %1")).arg(nAssignedAIs));

    m_prototypeButton->setEnabled(nAssignedAIs);
}

#include <AIWavePanel.moc>