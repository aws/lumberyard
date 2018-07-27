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

#ifndef CRYINCLUDE_CRYANIMATION_GLOBALANIMATIONHEADER_H
#define CRYINCLUDE_CRYANIMATION_GLOBALANIMATIONHEADER_H
#pragma once


struct GlobalAnimationHeader
{
    GlobalAnimationHeader()
    {
        m_nFlags = 0;
        m_FilePathCRC32         = 0;
    };
    virtual ~GlobalAnimationHeader() = default;

    ILINE uint32 IsAssetLoaded() const;

    ILINE uint32 IsAimpose() const {return m_nFlags & CA_AIMPOSE; }
    ILINE void OnAimpose() {m_nFlags |= CA_AIMPOSE; }

    ILINE uint32 IsAimposeUnloaded() const {return m_nFlags & CA_AIMPOSE_UNLOADED; }
    ILINE void OnAimposeUnloaded() {m_nFlags |= CA_AIMPOSE_UNLOADED; }


    ILINE uint32 IsAssetCreated() const {return m_nFlags & CA_ASSET_CREATED; }
    ILINE void OnAssetCreated() {m_nFlags |= CA_ASSET_CREATED; }
    ILINE void InvalidateAssetCreated() { m_nFlags &= (CA_ASSET_CREATED ^ -1); }

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

    ILINE uint32 IsAssetOnDemand() const;

    ILINE uint32 IsAssetNotFound() const { return m_nFlags & CA_ASSET_NOT_FOUND; }
    ILINE void OnAssetNotFound() { m_nFlags |= CA_ASSET_NOT_FOUND; }
    ILINE void ClearAssetNotFound() {m_nFlags &= ~CA_ASSET_NOT_FOUND; }

    ILINE uint32 IsAssetTCB() const { return m_nFlags & CA_ASSET_TCB; }

    ILINE uint32 IsAssetInternalType() const {return m_nFlags & CA_ASSET_INTERNALTYPE; }
    ILINE void OnAssetInternalType() {m_nFlags |= CA_ASSET_INTERNALTYPE; }

    ILINE void SetFlags(uint32 nFlags) {    m_nFlags = nFlags; };
    ILINE uint32 GetFlags() const { return m_nFlags; };

    ILINE void SetFilePath(const string& name) {    m_FilePath = name; };
    ILINE const char* GetFilePath() const { return m_FilePath.c_str(); };

    ILINE void SetFilePathCRC32(const string& name) {   m_FilePathCRC32 = CCrc32::ComputeLowercase(name.c_str()); };
    ILINE void SetFilePathCRC32(uint32 nCRC32) { m_FilePathCRC32 = nCRC32; }
    ILINE int GetFilePathCRC32() const { return m_FilePathCRC32; }


    virtual void AddNewDynamicControllers(IAnimationSet* animationSet){}

protected:
    uint32  m_nFlags;
    string  m_FilePath;             //low-case path-name - unique per animation asset
    uint32  m_FilePathCRC32;    //hash value for searching animation-paths (lower-case)
};



#endif // CRYINCLUDE_CRYANIMATION_GLOBALANIMATIONHEADER_H
