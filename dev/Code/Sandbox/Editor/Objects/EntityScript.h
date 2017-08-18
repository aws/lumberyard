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

#ifndef CRYINCLUDE_EDITOR_OBJECTS_ENTITYSCRIPT_H
#define CRYINCLUDE_EDITOR_OBJECTS_ENTITYSCRIPT_H
#pragma once


#include <IEntityClass.h>
#include <IScriptSystem.h>

#define PROPERTIES_TABLE "Properties"
#define PROPERTIES2_TABLE "PropertiesInstance"
#define COMMONPARAMS_TABLE "CommonParams"
#define FIRST_ENTITY_CLASS_ID 200

#include "Util/smartptr.h"
#include "Util/EditorUtils.h"

// forward declaration
class CEntityObject;
struct IScriptTable;
struct IScriptObject;

#define EVENT_PREFIX "Event_"

//////////////////////////////////////////////////////////////////////////
enum EEntityScriptFlags
{
    ENTITY_SCRIPT_SHOWBOUNDS      = 0x0001,
    ENTITY_SCRIPT_ABSOLUTERADIUS  = 0x0002,
    ENTITY_SCRIPT_ICONONTOP       = 0x0004,
    ENTITY_SCRIPT_DISPLAY_ARROW   = 0x0008,
    ENTITY_SCRIPT_ISNOTSCALABLE   = 0x0010,
    ENTITY_SCRIPT_ISNOTROTATABLE  = 0x0020,
};

/*!
 *  CEntityScript holds information about Entity lua script.
 */
class SANDBOX_API CEntityScript
    : public CRefCountBase
{
public:
    CEntityScript(IEntityClass* pClass);
    virtual ~CEntityScript();

    //////////////////////////////////////////////////////////////////////////
    void SetClass(IEntityClass* pClass);

    //////////////////////////////////////////////////////////////////////////
    IEntityClass* GetClass() const { return m_pClass; }

    //! Get name of entity script.
    QString GetName() const;
    // Get file of script.
    QString GetFile() const;

    //////////////////////////////////////////////////////////////////////////
    // Flags.
    int GetFlags() const { return m_nFlags; }

    int GetMethodCount() const { return m_methods.size(); }
    const QString& GetMethod(int index) const { return m_methods[index]; }

    //////////////////////////////////////////////////////////////////////////
    int GetEventCount();
    QString GetEvent(int i);

    //////////////////////////////////////////////////////////////////////////
    int GetEmptyLinkCount();
    const QString& GetEmptyLink(int i);

    bool Load();
    void Reload();
    bool IsValid() const { return m_bValid; }

    //! Marks script not valid, must be loaded on next access.
    void Invalidate() { m_bValid = false; }

    //! Get default properties of this script.
    CVarBlock* GetDefaultProperties() { return m_pDefaultProperties; }
    CVarBlock* GetDefaultProperties2() { return m_pDefaultProperties2; }

    //! Takes current values of var block and copies them to entity script table.
    void CopyPropertiesToScriptTable(IEntity* pEntity, CVarBlock* pVarBlock, bool bCallUpdate);
    void CopyProperties2ToScriptTable(IEntity* pEntity, CVarBlock* pVarBlock, bool bCallUpdate);
    void CallOnPropertyChange(IEntity* pEntity);

    // Takes current values from script table and copies it to respective properties var block
    void CopyPropertiesFromScriptTable(IEntity* pEntity, CVarBlock* pVarBlock);
    void CopyProperties2FromScriptTable(IEntity* pEntity, CVarBlock* pVarBlock);

    //! Setup entity target events table
    void SetEventsTable(CEntityObject* pEntity);

    //! Run method.
    void RunMethod(IEntity* pEntity, const QString& method);
    void SendEvent(IEntity* pEntity, const QString& event);

    // Edit methods.
    void GotoMethod(const QString& method);
    void AddMethod(const QString& method);

    //! Get visual object for this entity script.
    const QString& GetVisualObject() { return m_visualObject; }

    // Updates the texture icon with any overrides from the entity.  Should be called
    // after any property change.
    void UpdateTextureIcon(IEntity* pEntity);

    // Retrieve texture icon associated with this entity class.
    int GetTextureIcon() { return m_nTextureIcon; }

    //! Check if entity of this class can be used in editor.
    bool IsUsable() const { return m_usable; }

    // Set class as placeable or not.
    void SetUsable(bool usable) { m_usable = usable; }

    // Get client override for display path in editor entity treeview
    const QString& GetDisplayPath();

private:
    void CopyPropertiesToScriptInternal(IEntity* pEntity, CVarBlock* pVarBlock, bool bCallUpdate, const char* tableKey);
    void CopyPropertiesFromScriptInternal(IEntity* pEntity, CVarBlock* pVarBlock, const char* tableKey);

    bool ParseScript();
    int FindLineNum(const QString& line);

    //! Copy variable to script table
    void VarToScriptTable(IVariable* pVariable, IScriptTable* pScriptTable);

    //! Get variable from script table
    void ScriptTableToVar(IScriptTable* pScriptTable, IVariable* pVariable);

    IEntityClass* m_pClass;

    bool m_bValid;

    bool m_haveEventsTable;

    //! True if entity script have update entity
    bool m_bUpdatePropertiesImplemented;

    QString m_visualObject;
    int m_visibilityMask;
    int m_nTextureIcon;

    bool m_usable;

    //! Array of methods in this script.
    QStringList m_methods;

    //! Array of events supported by this script.
    QStringList m_events;

    // Create empty links.
    QStringList m_emptyLinks;

    int m_nFlags;
    TSmartPtr<CVarBlock> m_pDefaultProperties;
    TSmartPtr<CVarBlock> m_pDefaultProperties2;

    HSCRIPTFUNCTION m_pOnPropertyChangedFunc;
    QString m_userPath;
};

typedef TSmartPtr<CEntityScript> CEntityScriptPtr;

/*!
 *  CEntityScriptRegistry   manages all known CEntityScripts instances.
 */
class CEntityScriptRegistry
    : public IEntityClassRegistryListener
{
public:
    CEntityScriptRegistry();
    ~CEntityScriptRegistry();

    // IEntityClassRegistryListener
    void OnEntityClassRegistryEvent(EEntityClassRegistryEvent event, const IEntityClass* pEntityClass);
    // ~IEntityClassRegistryListener

    CEntityScript* Find(const QString& name);
    void    Insert(CEntityScript* script);

    void LoadScripts();
    void Reload();

    //! Get all scripts as array.
    void    GetScripts(std::vector<CEntityScript*>& scripts);

    static CEntityScriptRegistry* Instance();
    static void Release();

private:

    void SetClassCategory(CEntityScript* script);

    typedef StdMap<QString, CEntityScriptPtr> TScriptMap;

    TScriptMap m_scripts;
    static CEntityScriptRegistry* m_instance;
};
#endif // CRYINCLUDE_EDITOR_OBJECTS_ENTITYSCRIPT_H
