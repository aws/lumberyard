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
#ifndef CRYCINCLUDE_CRYCOMMON_IPREFABMANAGER_H
#define CRYCINCLUDE_CRYCOMMON_IPREFABMANAGER_H
#pragma once

class IPrefabManager
{
public:
    virtual ~IPrefabManager() {}

    /*!
     * Loads a prefab library if it has not yet been loaded.
     * @param filename The name of the library file.
     * @return True if loaded successfully or was already loaded.
     */
    virtual bool LoadPrefabLibrary(const string& filename) = 0;

    /*!
     * Creates an instance of a prefab.
     * @param libraryName The name of the library the prefab exists in.
     * @param prefabName The name of the prefab in the library to spawn.
     * @param id The id of the entity that is being spawned.
     * @param seed The seed  to use for random spawning (use seed of 0 to not use random spawning).
     * @param maxSpawn The maximum number of this prefab that should be spawned.
     */
    virtual void SpawnPrefab(const string& libraryName, const string& prefabName, EntityId id, uint32 seed, uint32 maxSpawn) = 0;

    /*!
     * Updates the transforms of all child entities of a given prefab instance.
     * @param id The ID of the entity to move.
     */
    virtual void MovePrefab(EntityId id) = 0;

    /*!
     * Deletes an instance of a prefab.
     * @param id The ID of the entity to remove.
     */
    virtual void DeletePrefab(EntityId id) = 0;

    /*!
     * Toggles the hiding of entities of this prefab and all child prefabs, and a hides associated render nodes.
     * @param id The ID of the entity to hide.
     * @param hide Whether the prefab should be hidden
     */
    virtual void HidePrefab(EntityId id, bool hide) = 0;

    /*!
     * Removes all runtime prefabs from the scene, and can unload currently loaded prefab libraries.
     * @param deleteLibs Whether loaded prefab libraries should be unloaded (defaults to true).
     */
    virtual void Clear(bool deleteLibs = true) = 0;
};

#endif
