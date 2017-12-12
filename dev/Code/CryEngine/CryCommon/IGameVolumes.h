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

#ifndef CRYINCLUDE_CRYCOMMON_IGAMEVOLUMES_H
#define CRYINCLUDE_CRYCOMMON_IGAMEVOLUMES_H
#pragma once


struct IGameVolumesEdit;

struct IGameVolumes
{
    struct VolumeInfo
    {
        VolumeInfo()
            : pVertices(NULL)
            , verticesCount(0)
        {
        }

        const Vec3*     pVertices;
        uint32              verticesCount;
        f32                     volumeHeight;
    };

    // <interfuscator:shuffle>
    virtual ~IGameVolumes() {};

    virtual IGameVolumesEdit* GetEditorInterface() = 0;

    virtual bool GetVolumeInfoForEntity(EntityId entityId, VolumeInfo* pOutInfo) const = 0;
    virtual void Load(const char* fileName) = 0;
    virtual void Reset() = 0;
    // </interfuscator:shuffle>
};

struct IGameVolumesEdit
{
    // <interfuscator:shuffle>
    virtual ~IGameVolumesEdit() {};

    virtual void SetVolume(EntityId entityId, const IGameVolumes::VolumeInfo& volumeInfo) = 0;
    virtual void DestroyVolume(EntityId entityId) = 0;

    virtual void RegisterEntityClass(const char* className) = 0;
    virtual size_t GetVolumeClassesCount() const = 0;
    virtual const char* GetVolumeClass(size_t index) const = 0;

    virtual void Export(const char* fileName) const = 0;
    // </interfuscator:shuffle>
};

#endif // CRYINCLUDE_CRYCOMMON_IGAMEVOLUMES_H
