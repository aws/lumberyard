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
#include "MannequinChangeMonitor.h"
#include "Helper/MannequinFileChangeWriter.h"

CMannequinChangeMonitor::CMannequinChangeMonitor()
    : m_pFileChangeWriter(new CMannequinFileChangeWriter(true))
{
    GetIEditor()->GetFileMonitor()->RegisterListener(this, "animations/mannequin", "adb");
}

CMannequinChangeMonitor::~CMannequinChangeMonitor()
{
    SAFE_DELETE(m_pFileChangeWriter);
    GetIEditor()->GetFileMonitor()->UnregisterListener(this);
}

void CMannequinChangeMonitor::OnFileChange(const char* sFilename, EChangeType eType)
{
    CryLog("CMannequinChangeMonitor - %s has changed", sFilename);

    if (CMannequinFileChangeWriter::UpdateActiveWriter() == false)
    {
        m_pFileChangeWriter->ShowFileManager();
    }
}
