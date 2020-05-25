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

#ifndef CRYINCLUDE_EDITOR_ENTITYPROTOTYPE_H
#define CRYINCLUDE_EDITOR_ENTITYPROTOTYPE_H
#pragma once

#include "Include/IEntityPrototype.h"
#include "Objects/EntityScript.h"

//////////////////////////////////////////////////////////////////////////
AZ_PUSH_DISABLE_DLL_EXPORT_BASECLASS_WARNING
/** Prototype of entity, contain specified entity properties.
*/
class CRYEDIT_API CEntityPrototype
    : public IEntityPrototype
{
AZ_POP_DISABLE_DLL_EXPORT_BASECLASS_WARNING
public:
    typedef Functor0 UpdateCallback;

    CEntityPrototype();
    ~CEntityPrototype();

    virtual EDataBaseItemType GetType() const override { return EDB_TYPE_ENTITY_ARCHETYPE; };

    //! Reload entity class.
    virtual void Reload() override;

    virtual void SetEntityClassName(const QString& className) override;

    //////////////////////////////////////////////////////////////////////////
    //! Serialize prototype to xml.
    virtual void Serialize(SerializeContext& ctx) override;

    virtual CEntityScript* GetScript() override { return m_script; }

    //////////////////////////////////////////////////////////////////////////
    // Update callback.
    //////////////////////////////////////////////////////////////////////////

    //! Called after prototype is updated.
    void Update() override;

private:
    // Pointer to entity script.
    TSmartPtr<CEntityScript> m_script;
};

TYPEDEF_AUTOPTR(CEntityPrototype);

#endif // CRYINCLUDE_EDITOR_ENTITYPROTOTYPE_H
