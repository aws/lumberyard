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

// Description : Interface for the Check Point System

#ifndef CRYINCLUDE_CRYACTION_ICHECKPOINTSYSTEM_H
#define CRYINCLUDE_CRYACTION_ICHECKPOINTSYSTEM_H
#pragma once

typedef CryFixedStringT<32> FixedCheckpointString;

struct ICheckpointSystem;

//container for checkpoint metadata
struct SCheckpointData
{
    int                     m_versionNumber;
    EntityId                m_checkPointId;
    FixedCheckpointString   m_levelName;
    FixedCheckpointString   m_saveTime;
};

//listener to onSave/onLoad events and interface for external save/load
struct ICheckpointListener
{
    virtual ~ICheckpointListener(){}
    // triggered at the end of saving
    virtual void OnSave(SCheckpointData* pCheckpoint, ICheckpointSystem* pSystem) = 0;
    // triggered at the end of loading
    virtual void OnLoad(SCheckpointData* pCheckpoint, ICheckpointSystem* pSystem) = 0;
};

//listener for game-specific processing of saving/loading
struct ICheckpointGameHandler
{
    virtual ~ICheckpointGameHandler() {}

    // Writing game-specific data
    virtual void OnWriteData(XmlNodeRef parentNode) {}

    // Reading game-specific data
    virtual void OnReadData(XmlNodeRef parentNode) {}

    // Engine reset control
    virtual void OnPreResetEngine() {}
    virtual void OnPostResetEngine() {}

    // Restart
    virtual void OnRestartGameplay() {}
};

//System interface
struct ICheckpointSystem
{
    virtual ~ICheckpointSystem() {}

    //load last (during this session) saved checkpoint
    virtual bool LoadLastCheckpoint() = 0;

    //load a game checkpoint from the specified filename
    virtual bool LoadGame(const char* fileName) = 0;

    //save a game checkpoint with the specified filename
    virtual bool SaveGame(EntityId checkpointId, const char* fileName) = 0;

    //set game handler
    virtual void SetGameHandler(ICheckpointGameHandler* pHandler) = 0;

    //clear game handler
    virtual void ClearGameHandler() = 0;

    //add checkpoint system listener
    virtual void RegisterListener(ICheckpointListener* pListener) = 0;

    //remove checkpoint system listener
    virtual void RemoveListener(ICheckpointListener* pListener) = 0;

    //save/load worldMatrix for an Entity
    virtual void SerializeWorldTM(IEntity* pEntity, XmlNodeRef data, bool writing) = 0;

    // save a single, external entity position/orientation/status during "OnSave"
    // the entity is loaded automatically with the checkpoint
    virtual bool SaveExternalEntity(EntityId id) = 0;
};

#endif // CRYINCLUDE_CRYACTION_ICHECKPOINTSYSTEM_H
