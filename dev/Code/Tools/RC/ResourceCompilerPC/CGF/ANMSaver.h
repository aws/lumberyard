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

#ifndef CRYINCLUDE_TOOLS_RC_RESOURCECOMPILERPC_CGF_ANMSAVER_H
#define CRYINCLUDE_TOOLS_RC_RESOURCECOMPILERPC_CGF_ANMSAVER_H
#pragma once

#include "AnimSaver.h"

class CSaverANM
    : public CSaverAnim
{
public:
    CSaverANM(const char* filename, CChunkFile& chunkFile)
        : CSaverAnim(filename, chunkFile) {}
    virtual void Save(const CContentCGF* pCGF, const CInternalSkinningInfo* pSkinningInfo) override;

private:
    int SaveNode(const CNodeCGF* pNode, int pos_cont_id, int rot_cont_id, int scl_cont_id);
    int SaveTCB3Track(const CInternalSkinningInfo* pSkinningInfo, int trackIndex);
    int SaveTCBQTrack(const CInternalSkinningInfo* pSkinningInfo, int trackIndex);
};

#endif // CRYINCLUDE_TOOLS_RC_RESOURCECOMPILERPC_CGF_ANMSAVER_H
