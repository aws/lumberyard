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

#ifndef CRYINCLUDE_EDITOR_PREFABS_PREFABITEM_H
#define CRYINCLUDE_EDITOR_PREFABS_PREFABITEM_H
#pragma once

#include "BaseLibraryItem.h"
#include <I3DEngine.h>

class CPrefabObject;

// Helpers for object ID mapping for better code readability
struct SObjectIdMapping
{
    GUID from;
    GUID to;
    SObjectIdMapping()
        : from(GUID_NULL)
        , to(GUID_NULL) {}
    SObjectIdMapping(GUID src, GUID dst)
        : from(src)
        , to(dst) {}
};

typedef std::vector<SObjectIdMapping> TObjectIdMapping;

/*! CPrefabItem contain definition of particle system spawning parameters.
 *
 */
class CRYEDIT_API CPrefabItem
    : public CBaseLibraryItem
{
    friend class CPrefabObject;
public:
    CPrefabItem();
    ~CPrefabItem();

    virtual EDataBaseItemType GetType() const { return EDB_TYPE_PREFAB; };

    void Serialize(SerializeContext& ctx);

    //! Make prefab from selection of objects.
    void MakeFromSelection(CSelectionGroup& selection);

    //! Called when something changed in a prefab the pPrefabObject is the changed object
    void UpdateFromPrefabObject(CPrefabObject* pPrefabObject, const SObjectChangedContext& context);

    //! Called after particle parameters where updated.
    void Update();
    //! Returns xml node containing prefab objects.
    XmlNodeRef GetObjectsNode() { return m_objectsNode; };
    QString GetPrefabObjectClassName() { return m_PrefabClassName; };
    void SetPrefabClassName(QString prefabClassNameString);

    void UpdateObjects();

private:
    //! Function to serialize changes to the main prefab lib (this changes only the internal XML representation m_objectsNode)
    void ModifyLibraryPrefab(CSelectionGroup& objectsInPrefabAsFlatSelection, CPrefabObject* pPrefabObject, const SObjectChangedContext& context, const TObjectIdMapping& guidMapping);
    //! Function to update a instanced prefabs in the level
    void ModifyInstancedPrefab(CSelectionGroup& objectsInPrefabAsFlatSelection, CPrefabObject* pPrefabObject, const SObjectChangedContext& context, const TObjectIdMapping& guidMapping);
    //! Registers prefab event flowgraph nodes from all prefab instances
    void RegisterPrefabEventFlowNodes(CBaseObject* const  pEntityObj);
    //! Searches and finds the XmlNode with a specified Id in m_objectsNode (the XML representation of this prefab in the prefab library)
    XmlNodeRef FindObjectInThisPrefabItemByGuid(REFGUID guid, bool fowardSearch);
    //! Searches for a object inside the TBaseObjects container by a prefabId
    CBaseObjectPtr FindObjectByPrefabGuid(const TBaseObjects& objects, REFGUID guidInPrefab);
    //! Extracts all object mapping from a selection (prefabID -> uniqueID)
    void ExtractObjectsPrefabIDtoGuidMapping(CSelectionGroup& objects, TObjectIdMapping& mapping);
    //! Remaps id to another id, or it returns the id unchanged if no mapping was found
    GUID ResolveID(const TObjectIdMapping& prefabIdToGuidMapping, GUID id, bool prefabIdToGuidDirection);
    //! Goes through objectNode and all its children (except Flowgraph) and remaps the ids in the direction specified by the prefabIdToGuidDirection parameter
    void RemapIDsInNodeAndChildren(XmlNodeRef objectNode, const TObjectIdMapping& mapping, bool prefabIdToGuidDirection);
    //! Function changes the ids XML attributes of objectNode in the direction specified by the prefabIdToGuidDirection parameter
    void RemapIDsInNode(XmlNodeRef objectNode, const TObjectIdMapping& mapping, bool prefabIdToGuidDirection);
    //! Removes all XML child nodes in m_objectsNode, which has a Parent with the specified GUID
    void RemoveAllChildsOf(GUID guid);

private:
    XmlNodeRef m_objectsNode;
    QString m_PrefabClassName;
};

#endif // CRYINCLUDE_EDITOR_PREFABS_PREFABITEM_H


