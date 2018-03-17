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

#ifndef CRYINCLUDE_CRYCOMMON_IENTITYCLASS_H
#define CRYINCLUDE_CRYCOMMON_IENTITYCLASS_H
#pragma once

#include "BoostHelpers.h"
#include "IComponent.h"

struct IEntity;
struct SEntitySpawnParams;
struct IEntityScript;
struct IScriptTable;
class XmlNodeRef;

struct SEditorClassInfo
{
    SEditorClassInfo()
        : sIcon("")
        , sHelper("")
        , sCategory("")
    {
    }

    const char* sIcon;
    const char* sHelper;
    const char* sCategory;
};

enum EEntityClassFlags
{
    ECLF_INVISIBLE              = BIT(0), // If set this class will not be visible in editor,and entity of this class cannot be placed manually in editor.
    ECLF_DEFAULT                = BIT(1), // If this is default entity class.
    ECLF_BBOX_SELECTION         = BIT(2), // If set entity of this class can be selected by bounding box in the editor 3D view.
    ECLF_DO_NOT_SPAWN_AS_STATIC = BIT(3), // If set the entity of this class stored as part of the level won't be assigned a static id on creation
    ECLF_MODIFY_EXISTING                = BIT(4)  // If set modify an existing class with the same name.
};

struct IEntityClassRegistryListener;

// Description:
//    Custom interface that can be used to enumerate/set an entity class' property.
//    Used for comunication with the editor, mostly.
// See Also:
//    IEntityClassRegistry::SEntityClassDesc
struct IEntityPropertyHandler
{
    virtual ~IEntityPropertyHandler(){}
    //////////////////////////////////////////////////////////////////////////
    // Property info for entity class.
    //////////////////////////////////////////////////////////////////////////
    virtual void GetMemoryUsage(ICrySizer* pSizer) const { /*REPLACE LATER*/}

    enum EPropertyType
    {
        Bool,
        Int,
        Float,
        Vector,
        String,
        Entity,
        FolderBegin,
        FolderEnd
    };

    enum EPropertyFlags
    {
        ePropertyFlag_UIEnum = BIT(0),
        ePropertyFlag_Unsorted = BIT(1),
    };

    struct SPropertyInfo
    {
        const char* name;           // Name of the property.
        EPropertyType type;         // Type of the property value.
        const char* editType;       // Type of edit control to use.
        const char* description;    // Description of the property.
        uint32 flags;               // Property flags.

        struct SLimits              // Limits
        {
            float min;
            float max;
        } limits;
    };

    // Description:
    //    Refresh the class' properties.
    virtual void RefreshProperties() = 0;

    // Description:
    //    Load properties into the entity.
    virtual void LoadEntityXMLProperties(IEntity* entity, const XmlNodeRef& xml) = 0;

    // Description:
    //    Load archetype properties.
    virtual void LoadArchetypeXMLProperties(const char* archetypeName, const XmlNodeRef& xml) = 0;

    // Description:
    //    Init entity with archetype properties.
    virtual void InitArchetypeEntity(IEntity* entity, const char* archetypeName, const SEntitySpawnParams& spawnParams) = 0;

    // Description:
    //    Returns number of properties for this entity.
    // See Also:
    //    GetPropertyInfo
    // Returns:
    //    Return Number of properties.
    virtual int GetPropertyCount() const = 0;

    // Description:
    //    Retrieve information about properties of the entity.
    // See Also:
    //    SPropertyInfo, GetPropertyCount
    // Arguments:
    //    nIndex - Index of the property to retrieve, must be in 0 to GetPropertyCount()-1 range.
    // Returns:
    //    Specified event description in SPropertyInfo structure.
    virtual bool GetPropertyInfo(int index, SPropertyInfo& info) const = 0;

    // Description:
    //    Set a property in a entity of this class.
    // Arguments:
    //    index - Index of the property to set, must be in 0 to GetPropertyCount()-1 range.
    virtual void SetProperty(IEntity* entity, int index, const char* value) = 0;

    // Description:
    //    Get a property in a entity of this class.
    // Arguments:
    //    index - Index of the property to get, must be in 0 to GetPropertyCount()-1 range.
    virtual const char* GetProperty(IEntity* entity, int index) const = 0;

    // Description:
    //    Get a property in a entity of this class.
    // Arguments:
    //    index - Index of the property to get, must be in 0 to GetPropertyCount()-1 range.
    virtual const char* GetDefaultProperty(int index) const = 0;

    // Description:
    //    Get script flags of this class.
    virtual uint32 GetScriptFlags() const = 0;

    // Description:
    //    Inform the implementation that properties have changed. Called at the end of a change block.
    virtual void PropertiesChanged(IEntity* entity) = 0;
};

// Description:
//    Custom interface that can be used to reload an entity's script.
//    Used by the editor, only.
// See Also:
//    IEntityClassRegistry::SEntityClassDesc
struct IEntityScriptFileHandler
{
    virtual ~IEntityScriptFileHandler(){}
    // Description:
    //    Reloads the specified entity class' script.
    virtual void ReloadScriptFile() = 0;

    // Description:
    //    Returns the class' script file name, if any.
    virtual const char* GetScriptFile() const = 0;

    virtual void GetMemoryUsage(ICrySizer* pSizer) const { /*REPLACE LATER*/}
};


struct IEntityEventHandler
{
    virtual ~IEntityEventHandler(){}
    virtual void GetMemoryUsage(ICrySizer* pSizer) const { /*REPLACE LATER*/}

    //////////////////////////////////////////////////////////////////////////
    // Events info for this entity class.
    //////////////////////////////////////////////////////////////////////////
    enum EventValueType
    {
        Bool,
        Int,
        Float,
        Vector,
        Entity,
        String,
    };

    enum EventType
    {
        Input,
        Output,
    };

    struct SEventInfo
    {
        const char* name;           // Name of event.
        EventType type;             // Input or Output event.
        EventValueType valueType;   // Type of event value.
        const char* description;    // Description of the event.
    };

    // Description:
    //    Refresh the class' event info.
    virtual void RefreshEvents() = 0;

    // Description:
    //    Load event info into the entity.
    virtual void LoadEntityXMLEvents(IEntity* entity, const XmlNodeRef& xml) = 0;

    // Description:
    //    Returns number of events for this entity.
    // See Also:
    //    GetEventInfo
    // Returns:
    //    Return Number of events.
    virtual int GetEventCount() const = 0;

    // Description:
    //    Retrieve information about events of the entity.
    // See Also:
    //    SEventInfo, GetEventCount
    // Arguments:
    //    nIndex - Index of the event to retrieve, must be in 0 to GetEventCount()-1 range.
    // Returns:
    //    Specified event description in SEventInfo structure.
    virtual bool GetEventInfo(int index, SEventInfo& info) const = 0;

    // Description:
    //    Send the specified event to the entity.
    // Arguments:
    //    entity - The entity to send the event to.
    //    eventName - Name of the event to send.
    virtual void SendEvent(IEntity* entity, const char* eventName) = 0;
};

namespace Serialization
{
    class IArchive;
}

struct IEntityAttribute;

DECLARE_SMART_POINTERS(IEntityAttribute)

//////////////////////////////////////////////////////////////////////////
// Description:
//    Derive from this interface to expose custom entity properties in
//    the editor using the serialization framework.
//////////////////////////////////////////////////////////////////////////
struct IEntityAttribute
{
    virtual ~IEntityAttribute() {}

    virtual const char* GetName() const = 0;
    virtual const char* GetLabel() const = 0;
    virtual void Serialize(Serialization::IArchive& archive) = 0;
    virtual IEntityAttributePtr Clone() const =  0;
};

typedef DynArray<IEntityAttributePtr> TEntityAttributeArray;

//////////////////////////////////////////////////////////////////////////
// Description:
//    Entity class defines what is this entity, what script it uses,
//    what user component will be spawned with the entity,etc...
//    IEntityClass unique identify type of the entity, Multiple entities
//    share the same entity class.
//    Two entities can be compared if they are of the same type by
//    just comparing their IEntityClass pointers.
//////////////////////////////////////////////////////////////////////////
struct IEntityClass
{
    //////////////////////////////////////////////////////////////////////////
    // ComponentUserCreateFunc is a function pointer type,
    // by calling this function EntitySystem can create user defined ComponentUser class for an entity in SpawnEntity.
    // Ex:
    // IComponentPtr CreateComponentUser( IEntity *pEntity,SEntitySpawnParams &params )
    // {
    //   return new CComponentUser( pEntity,params );
    // }
    typedef IComponentPtr (* ComponentUserCreateFunc)(IEntity* pEntity, SEntitySpawnParams& params, void* pUserData);

    //////////////////////////////////////////////////////////////////////////
    // Events info for this entity class.
    //////////////////////////////////////////////////////////////////////////
    enum EventValueType
    {
        EVT_INT,
        EVT_FLOAT,
        EVT_BOOL,
        EVT_VECTOR,
        EVT_ENTITY,
        EVT_STRING
    };

    struct SEventInfo
    {
        const char* name;           // Name of event.
        EventValueType type;  // Type of event value.
        bool bOutput;                   // Input or Output event.
    };

    // <interfuscator:shuffle>
    virtual ~IEntityClass(){}
    // Description:
    //    Destroy IEntityClass object, do not call directly, only EntityRegisty can destroy entity classes.
    virtual void Release() = 0;

    // Description:
    //    Returns the name of the entity class, Class name must be unique among all the entity classes.
    //    If this entity also uses a script, this is the name of the Lua table representing the entity behavior.
    virtual const char* GetName() const = 0;

    // Description:
    //    Returns entity class flags.
    // See Also:
    //    EEntityClassFlags
    virtual uint32 GetFlags() const = 0;

    // Description:
    //    Set entity class flags.
    // See Also:
    //    EEntityClassFlags
    virtual void SetFlags(uint32 nFlags) = 0;

    // Description:
    //    Returns the Lua script file name.
    // Returns:
    //    Lua Script filename, return empty string if entity does not use script.
    virtual const char* GetScriptFile() const = 0;

    // Description:
    //    Returns the IEntityScript interface assigned for this entity class.
    // Returns:
    //    IEntityScript interface if this entity have script, or NULL if no script defined for this entity class.
    virtual IEntityScript* GetIEntityScript() const = 0;

    // Description:
    //    Returns the IScriptTable interface assigned for this entity class.
    // Returns:
    //    IScriptTable interface if this entity have script, or NULL if no script defined for this entity class.
    virtual IScriptTable* GetScriptTable() const = 0;

    virtual IEntityPropertyHandler* GetPropertyHandler() const = 0;
    virtual IEntityEventHandler* GetEventHandler() const = 0;
    virtual IEntityScriptFileHandler* GetScriptFileHandler() const = 0;

    virtual const SEditorClassInfo& GetEditorClassInfo() const = 0;
    virtual void SetEditorClassInfo(const SEditorClassInfo& editorClassInfo) = 0;

    // Description:
    //    Loads the script.
    //    It is safe to call LoadScript multiple times, only first time the script will be loaded, if bForceReload is not specified.
    virtual bool LoadScript(bool bForceReload) = 0;

    // Description:
    //    Returns pointer to the user defined function to create ComponentUser.
    // Returns:
    //    Return ComponentUserCreateFunc function pointer.
    virtual IEntityClass::ComponentUserCreateFunc GetComponentUserCreateFunc() const = 0;

    // Description:
    //    Returns pointer to the user defined data to be passed when creating ComponentUser.
    // Returns:
    //    Return pointer to custom user component data.
    virtual void* GetComponentUserData() const = 0;

    // Description:
    //    Returns number of input and output events defiend in the entity script.
    // See Also:
    //    GetEventInfo
    // Returns:
    //    Return Number of events.
    virtual int GetEventCount() = 0;

    // Description:
    //    Retrieve information about input/output event of the entity.
    // See Also:
    //    SEventInfo, GetEventCount
    // Arguments:
    //    nIndex - Index of the event to retrieve, must be in 0 to GetEventCount()-1 range.
    // Returns:
    //    Specified event description in SEventInfo structure.
    virtual IEntityClass::SEventInfo GetEventInfo(int nIndex) = 0;

    // Description:
    //    Find event by name.
    // See Also:
    //    SEventInfo, GetEventInfo
    // Arguments:
    //    sEvent - Name of the event.
    //    event - Output parameter for event.
    // Returns:
    //    True if event found and event parameter is initialized.
    virtual bool FindEventInfo(const char* sEvent, SEventInfo& event) = 0;

    // Description:
    //    Get attributes associated with this entity class.
    // See Also:
    //    IEntityAttribute
    // Returns:
    //    Array of entity attributes.
    virtual TEntityAttributeArray& GetClassAttributes() = 0;
    virtual const TEntityAttributeArray& GetClassAttributes() const = 0;

    // Description:
    //    Get attributes associated with entities of this class.
    // See Also:
    //    IEntityAttribute
    // Returns:
    //    Array of entity attributes.
    virtual TEntityAttributeArray& GetEntityAttributes() = 0;
    virtual const TEntityAttributeArray& GetEntityAttributes() const = 0;

    virtual void GetMemoryUsage(ICrySizer* pSizer) const = 0;
    // </interfuscator:shuffle>
};

//////////////////////////////////////////////////////////////////////////
// Description:
//    This interface is the repository of the the various entity classes, it allows
//    creation and modification of entities types.
//    There`s only one IEntityClassRegistry interface can exist per EntitySystem.
//    Every entity class that can be spawned must be registered in this interface.
// See Also:
//    IEntitySystem::GetClassRegistry
//////////////////////////////////////////////////////////////////////////
struct IEntityClassRegistry
{
    struct SEntityClassDesc
    {
        SEntityClassDesc()
            : flags(0)
            , sName("")
            , sScriptFile("")
            , pScriptTable(NULL)
            , editorClassInfo()
            , pComponentUserCreateFunc(NULL)
            , pComponentUserData(NULL)
            , pPropertyHandler(NULL)
            , pEventHandler(NULL)
            , pScriptFileHandler(NULL)
        {
        };

        int           flags;
        const char*   sName;
        const char*   sScriptFile;
        IScriptTable* pScriptTable;

        SEditorClassInfo editorClassInfo;

        IEntityClass::ComponentUserCreateFunc pComponentUserCreateFunc;
        void*         pComponentUserData;

        IEntityPropertyHandler* pPropertyHandler;
        IEntityEventHandler* pEventHandler;
        IEntityScriptFileHandler* pScriptFileHandler;

        TEntityAttributeArray   classAttributes;
        TEntityAttributeArray   entityAttributes;
    };

    virtual ~IEntityClassRegistry(){}
    // Description:
    //    Register a new entity class.
    // Returns:
    //    true if successfully registered.
    virtual bool RegisterEntityClass(IEntityClass* pClass) = 0;

    // Description:
    //    Unregister an entity class.
    // Returns:
    //    true if successfully unregistered.
    virtual bool UnregisterEntityClass(IEntityClass* pClass) = 0;

    // Description:
    //    Retrieves pointer to the IEntityClass interface by entity class name.
    // Returns:
    //    Pointer to the IEntityClass interface, or NULL if class not found.
    virtual IEntityClass* FindClass(const char* sClassName) const = 0;

    // Description:
    //    Retrieves pointer to the IEntityClass interface for a default entity class.
    // Returns:
    //    Pointer to the IEntityClass interface, It can never return NULL.
    virtual IEntityClass* GetDefaultClass() const = 0;

    // Description:
    //    Load all entity class description xml files with extension ".ent" from specified directory.
    // Arguments:
    //    sPath - Path where to search for .ent files.
    virtual void LoadClasses(const char* sPath, bool bOnlyNewClasses = false) = 0;

    // Description:
    //    Register standard entity class, if class id not specified (is zero), generate a new class id.
    // Returns:
    //    Pointer to the new created and registered IEntityClass interface, or NULL if failed.
    virtual IEntityClass* RegisterStdClass(const SEntityClassDesc& entityClassDesc) = 0;

    // Description:
    //      Register a listener.
    virtual void RegisterListener(IEntityClassRegistryListener* pListener) = 0;

    // Description:
    //      Unregister a listener.
    virtual void UnregisterListener(IEntityClassRegistryListener* pListener) = 0;

    //////////////////////////////////////////////////////////////////////////
    // Registry iterator.
    //////////////////////////////////////////////////////////////////////////

    // Description:
    //    Move the entity class iterator to the begin of the registry.
    //    To iterate over all entity classes, ex:
    //    ...
    //    IEntityClass *pClass = NULL;
    //    for (pEntityRegistry->IteratorMoveFirst(); pClass = pEntityRegistry->IteratorNext();;)
    //    {
    //       pClass
    //    ...
    //    }
    virtual void IteratorMoveFirst() = 0;

    // Description:
    //    Get the next entity class in the registry.
    // Returns:
    //    Return a pointer to the next IEntityClass interface, or NULL if is the end
    virtual IEntityClass* IteratorNext() = 0;

    // Description:
    //    Return the number of entity classes in the registry.
    // Returns:
    //    Return a pointer to the next IEntityClass interface, or NULL if is the end
    virtual int GetClassCount() const = 0;
    // </interfuscator:shuffle>
};

enum EEntityClassRegistryEvent
{
    ECRE_CLASS_REGISTERED = 0,  // Sent when new entity class is registered.
    ECRE_CLASS_MODIFIED,                // Sent when new entity class is modified (see ECLF_MODIFY_EXISTING).
    ECRE_CLASS_UNREGISTERED         // Sent when new entity class is unregistered.
};

//////////////////////////////////////////////////////////////////////////
// Description:
//    Use this interface to monitor changes within the entity class registry.
//////////////////////////////////////////////////////////////////////////
struct IEntityClassRegistryListener
{
    friend class CEntityClassRegistry;

public:

    inline IEntityClassRegistryListener()
        : m_pRegistry(NULL)
    {}

    virtual ~IEntityClassRegistryListener()
    {
        if (m_pRegistry)
        {
            m_pRegistry->UnregisterListener(this);
        }
    }

    virtual void OnEntityClassRegistryEvent(EEntityClassRegistryEvent event, const IEntityClass* pEntityClass) = 0;

private:

    IEntityClassRegistry*   m_pRegistry;
};

#endif // CRYINCLUDE_CRYCOMMON_IENTITYCLASS_H
