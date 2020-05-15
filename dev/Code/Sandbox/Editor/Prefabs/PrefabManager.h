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

#ifndef CRYINCLUDE_EDITOR_PREFABS_PREFABMANAGER_H
#define CRYINCLUDE_EDITOR_PREFABS_PREFABMANAGER_H
#pragma once

#include "BaseLibraryManager.h"

class CPrefabItem;
class CPrefabObject;
class CPrefabLibrary;

typedef _smart_ptr<CPrefabObject> CPrefabObjectPtr;

class CUndoAddObjectsToPrefab
    : public IUndoObject
{
public:
    CUndoAddObjectsToPrefab(CPrefabObject* prefabObj, TBaseObjects& objects);

protected:
    virtual int GetSize() { return sizeof(*this); }; // Return size of xml state.
    virtual QString GetDescription() { return "Add Objects To Prefab"; };

    virtual void Undo(bool bUndo);
    virtual void Redo();

private:

    struct SObjectsLinks
    {
        GUID m_object;
        GUID m_objectParent;
        std::vector<GUID> m_objectsChilds;
    };

    typedef std::vector<SObjectsLinks> TObjectsLinks;

    CPrefabObjectPtr m_pPrefabObject;
    TObjectsLinks m_addedObjects;
};

/** Manages Prefab libraries and systems.
*/
class CRYEDIT_API CPrefabManager
    : public CBaseLibraryManager
{
public:
    CPrefabManager();
    ~CPrefabManager() = default;

    // Clear all prototypes
    void ClearAll();

    //! Delete item from library and manager.
    void DeleteItem(IDataBaseItem* pItem);

    //! Serialize manager.
    virtual void Serialize(XmlNodeRef& node, bool bLoading);

    //! Make new prefab item from selection.
    CPrefabItem* MakeFromSelection();

    //! Add selected objects to prefab (which selected too)
    void AddSelectionToPrefab();

    //! Open all prefabs in level
    void OpenAllPrefabs();

    //! Close all prefabs in level
    void CloseAllPrefabs();

    void CloneObjectsFromPrefabs(std::vector<CBaseObject*>& childObjects, bool bSelectCloned = true);
    void CloneAllFromPrefabs(std::vector<CPrefabObject*>& pPrefabs, bool bSelectCloned = true);

    void ExtractObjectsFromPrefabs(std::vector<CBaseObject*>& childObjects, bool bSelectExtracted = true);
    void ExtractAllFromPrefabs(std::vector<CPrefabObject*>& pPrefabs, bool bSelectExtracted = true);

    bool ClosePrefabs(std::vector<CPrefabObject*>& prefabObjects, const char* undoDescription);
    bool OpenPrefabs(std::vector<CPrefabObject*>& prefabObjects, const char* undoDescription);

    //! Get all prefab objects in level
    void GetPrefabObjects(std::vector<CPrefabObject*>& outPrefabObjects);

    //! Get prefab instance count used in level
    int GetPrefabInstanceCount(CPrefabItem* pPrefabItem);

    IDataBaseLibrary* LoadLibrary(const QString& filename, bool bReload = false);

    bool ShouldSkipPrefabUpdate() const { return m_skipPrefabUpdate; }
    void SetSkipPrefabUpdate(bool skip) { m_skipPrefabUpdate = skip; }

private:
    void SelectObjectsIgnoringUndo(CSelectionGroup& extractedObjects) const;
    void ExpandGroup(CBaseObject* pObject, CSelectionGroup& selection) const;

protected:
    virtual CBaseLibraryItem* MakeNewItem();
    virtual CBaseLibrary* MakeNewLibrary();
    //! Root node where this library will be saved.
    virtual QString GetRootNodeName();
    //! Path to libraries in this manager.
    virtual QString GetLibsPath();

    QString m_libsPath;

    bool m_skipPrefabUpdate;
};

#endif // CRYINCLUDE_EDITOR_PREFABS_PREFABMANAGER_H
