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

#ifndef CRYINCLUDE_TOOLS_RC_RESOURCECOMPILERPC_CGF_ANIMSAVER_H
#define CRYINCLUDE_TOOLS_RC_RESOURCECOMPILERPC_CGF_ANIMSAVER_H
#pragma once

#include "CryPath.h"
#include <CryTypeInfo.h>
#include "../CryEngine/Cry3DEngine/CGF/ChunkFile.h"
#include "CGFContent.h"
#include "../CGA/Controller.h"
#include "../CGA/ControllerPQ.h"
#include "../CGA/ControllerPQLog.h"
#include "LoaderCAF.h"


class CSaverAnim
{
public:
    CSaverAnim(const char* filename, CChunkFile& chunkFile);
    virtual void Save(const CContentCGF* pCGF, const CInternalSkinningInfo* pSkinningInfo) = 0;

    static int SaveTCB3Track(CChunkFile* pChunkFile, const CInternalSkinningInfo* pSkinningInfo, int trackIndex);
    static int SaveTCBQTrack(CChunkFile* pChunkFile, const CInternalSkinningInfo* pSkinningInfo, int trackIndex);
    static int SaveTiming(CChunkFile* pChunkFile, const CInternalSkinningInfo* pSkinningInfo);

protected:
    int SaveExportFlags(const CContentCGF* pCGF);
    int SaveTiming(const CInternalSkinningInfo* pSkinningInfo);

    string m_filename;
    CChunkFile* m_pChunkFile;
};

#endif // CRYINCLUDE_TOOLS_RC_RESOURCECOMPILERPC_CGF_ANIMSAVER_H
