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

#ifndef CRYINCLUDE_CRYACTION_GAMEVOLUMES_GAMEVOLUMESMANAGER_H
#define CRYINCLUDE_CRYACTION_GAMEVOLUMES_GAMEVOLUMESMANAGER_H
#pragma once

#include <IGameVolumes.h>

class CGameVolumesManager
    : public IGameVolumes
    , IGameVolumesEdit
{
private:

    typedef std::vector<Vec3> Vertices;
    struct EntityVolume
    {
        EntityVolume()
            : entityId(0)
            , height(1.0f)
        {
        }

        bool operator == (const EntityId& id) const
        {
            return entityId == id;
        }

        EntityId    entityId;
        f32             height;
        Vertices    vertices;
    };

    typedef std::vector<EntityVolume>   TEntityVolumes;
    typedef std::vector<IEntityClass*> TVolumeClasses;

public:
    CGameVolumesManager();
    virtual ~CGameVolumesManager();

    // IGameVolumes
    virtual IGameVolumesEdit* GetEditorInterface();
    virtual bool GetVolumeInfoForEntity(EntityId entityId, VolumeInfo* pOutInfo) const;
    virtual void Load(const char* fileName);
    virtual void Reset();
    // ~IGameVolumes


    // IGameVolumesEdit
    virtual void SetVolume(EntityId entityId, const IGameVolumes::VolumeInfo& volumeInfo);
    virtual void DestroyVolume(EntityId entityId);

    virtual void RegisterEntityClass(const char* className);
    virtual size_t GetVolumeClassesCount() const;
    virtual const char* GetVolumeClass(size_t index) const;

    virtual void Export(const char* fileName) const;
    // ~IGameVolumesEdit

private:

    TEntityVolumes m_volumesData;       // Level memory
    TVolumeClasses m_classes;               // Global memory, initialized at start-up

    const static uint32 GAME_VOLUMES_FILE_VERSION = 1;
};

#endif // CRYINCLUDE_CRYACTION_GAMEVOLUMES_GAMEVOLUMESMANAGER_H
