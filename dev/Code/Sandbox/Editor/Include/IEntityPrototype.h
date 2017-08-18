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
#ifndef CRYINCLUDE_EDITOR_CORE_IENTITYPROTOTYPE_H
#define CRYINCLUDE_EDITOR_CORE_IENTITYPROTOTYPE_H
#pragma once

#include "BaseLibraryItem.h"

class CEntityPrototypeLibrary;
class CEntityScript;
struct IEntityArchetype;
class CVarBlock;

//////////////////////////////////////////////////////////////////////////
/** Prototype of entity, contain specified entity properties.
*/
class IEntityPrototype
    : public CBaseLibraryItem
{
public:
    typedef Functor0 UpdateCallback;

    virtual ~IEntityPrototype(){}

    virtual EDataBaseItemType GetType() const = 0;

    //! Reload entity class.
    virtual void Reload() = 0;

    //! Called after prototype is updated.
    virtual void Update() = 0;

    //! Serialize prototype to xml.
    virtual void Serialize(SerializeContext& ctx) = 0;

    //! Get entity script of this prototype.
    virtual CEntityScript* GetScript() = 0;

    //! Set prototype description.
    void SetDescription(const QString& description) { m_description = description; };

    //! Get prototype description.
    const QString& GetDescription() const { return m_description; };

    //! Set class name of entity.
    virtual void SetEntityClassName(const QString& className) { m_className = className; }

    //! Get class name of entity.
    const QString& GetEntityClassName() const { return m_className; }

    IEntityArchetype* GetIEntityArchetype() const { return m_pArchetype; }

    //! Return properties of entity.
    CVarBlock* GetProperties() { return m_properties; }

    CVarBlock* GetObjectVarBlock() { return m_pObjectVarBlock; };

    void AddUpdateListener(UpdateCallback cb)
    {
        m_updateListeners.push_back(cb);
    }

    void RemoveUpdateListener(UpdateCallback cb)
    {
        std::list<UpdateCallback>::iterator it = std::find(m_updateListeners.begin(), m_updateListeners.end(), cb);
        if (it != m_updateListeners.end())
        {
            m_updateListeners.erase(it);
        }
    }

protected:
    IEntityArchetype* m_pArchetype;

    //! Name of entity class name.
    QString m_className;
    //! Description of this prototype.
    QString m_description;
    //! Entity properties.
    CVarBlockPtr m_properties;

    CVarBlockPtr m_pObjectVarBlock;

    //! List of update callbacks.
    std::list<UpdateCallback> m_updateListeners;
};

#endif // CRYINCLUDE_EDITOR_CORE_IENTITYPROTOTYPE_H
