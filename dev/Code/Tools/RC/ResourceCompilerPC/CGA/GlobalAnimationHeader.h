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

#ifndef CRYINCLUDE_TOOLS_RC_RESOURCECOMPILERPC_CGA_GLOBALANIMATIONHEADER_H
#define CRYINCLUDE_TOOLS_RC_RESOURCECOMPILERPC_CGA_GLOBALANIMATIONHEADER_H
#pragma once


#include "CryCharAnimationParams.h"
#include "Controller.h"

struct GlobalAnimationHeader
{
    GlobalAnimationHeader() { m_nFlags = 0;   };
    virtual ~GlobalAnimationHeader()    {   };

    ILINE uint32 IsAssetLoaded() const {return m_nFlags & CA_ASSET_LOADED; }
    ILINE void OnAssetLoaded() {m_nFlags |= CA_ASSET_LOADED; }

    ILINE uint32 IsAimpose() const {return m_nFlags & CA_AIMPOSE; }
    ILINE void OnAimpose() {m_nFlags |= CA_AIMPOSE; }

    ILINE uint32 IsAimposeUnloaded() const {return m_nFlags & CA_AIMPOSE_UNLOADED; }
    ILINE void OnAimposeUnloaded() {m_nFlags |= CA_AIMPOSE_UNLOADED; }

    ILINE void ClearAssetLoaded() {m_nFlags &= ~CA_ASSET_LOADED; }

    ILINE uint32 IsAssetCreated() const {return m_nFlags & CA_ASSET_CREATED; }
    ILINE void OnAssetCreated() {m_nFlags |= CA_ASSET_CREATED; }

    ILINE uint32 IsAssetAdditive() const { return m_nFlags & CA_ASSET_ADDITIVE; }
    ILINE void OnAssetAdditive() { m_nFlags |= CA_ASSET_ADDITIVE; }

    ILINE uint32 IsAssetCycle() const {return m_nFlags & CA_ASSET_CYCLE; }
    ILINE void OnAssetCycle() {m_nFlags |= CA_ASSET_CYCLE; }

    ILINE uint32 IsAssetLMG() const {return m_nFlags & CA_ASSET_LMG; }
    ILINE void OnAssetLMG() {m_nFlags |= CA_ASSET_LMG; }
    ILINE uint32 IsAssetLMGValid() const { return m_nFlags & CA_ASSET_LMG_VALID; }
    ILINE void OnAssetLMGValid() { m_nFlags |= CA_ASSET_LMG_VALID; }
    ILINE void InvalidateAssetLMG() { m_nFlags &= (CA_ASSET_LMG_VALID ^ -1); }

    ILINE uint32 IsAssetRequested() const { return m_nFlags & CA_ASSET_REQUESTED; }
    ILINE void OnAssetRequested() { m_nFlags |= CA_ASSET_REQUESTED; }
    ILINE void ClearAssetRequested() {m_nFlags &= ~CA_ASSET_REQUESTED; }

    ILINE uint32 IsAssetOnDemand() const { return m_nFlags & CA_ASSET_ONDEMAND; }
    ILINE void OnAssetOnDemand() { m_nFlags |= CA_ASSET_ONDEMAND; }
    ILINE void ClearAssetOnDemand() { m_nFlags &= ~CA_ASSET_ONDEMAND; }

    ILINE uint32 IsAssetNotFound() const { return m_nFlags & CA_ASSET_NOT_FOUND; }
    ILINE void OnAssetNotFound() { m_nFlags |= CA_ASSET_NOT_FOUND; }
    ILINE void ClearAssetNotFound() {m_nFlags &= ~CA_ASSET_NOT_FOUND; }

    uint32 m_nFlags;
};



#endif // CRYINCLUDE_TOOLS_RC_RESOURCECOMPILERPC_CGA_GLOBALANIMATIONHEADER_H
