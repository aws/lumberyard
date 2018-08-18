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

#ifndef CRYINCLUDE_CRYANIMATION_ATTACHMENTBASE_H
#define CRYINCLUDE_CRYANIMATION_ATTACHMENTBASE_H
#pragma once

#include "Skeleton.h"
class CAttachmentManager;

struct SAttachmentBase
{
    SAttachmentBase()
    {
        m_AttFlags  = 0;
        m_nSocketCRC32 = 0;
        m_nRefCounter = 0;
        m_pIAttachmentObject =   0;
    };

    //shared members
    uint32 m_AttFlags;
    string m_strSocketName;
    uint32 m_nSocketCRC32;
    int32    m_nRefCounter;
    IAttachmentObject* m_pIAttachmentObject;
    CAttachmentManager* m_pAttachmentManager;
};


#endif // CRYINCLUDE_CRYANIMATION_ATTACHMENTBASE_H
