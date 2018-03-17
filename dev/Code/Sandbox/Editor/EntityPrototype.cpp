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

#include "StdAfx.h"
#include "EntityPrototype.h"
#include "Objects/EntityScript.h"
#include "Objects/EntityObject.h"

#include <IEntitySystem.h>

//////////////////////////////////////////////////////////////////////////
// CEntityPrototype implementation.
//////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////
CEntityPrototype::CEntityPrototype()
{
    m_pArchetype = 0;
}

//////////////////////////////////////////////////////////////////////////
CEntityPrototype::~CEntityPrototype()
{
}

//////////////////////////////////////////////////////////////////////////
void CEntityPrototype::SetEntityClassName(const QString& className)
{
    IEntityPrototype::SetEntityClassName(className);

    // clear old properties.
    m_properties = 0;

    // Find this script in registry.
    m_script = CEntityScriptRegistry::Instance()->Find(m_className);
    if (m_script)
    {
        // Assign properties.
        if (!m_script->IsValid())
        {
            if (!m_script->Load())
            {
                Error("Failed to initialize Entity Script %s from %s", m_className.toUtf8().data(), m_script->GetFile().toUtf8().data());
                m_script = 0;
            }
        }
    }

    if (m_script && !m_script->GetClass())
    {
        Error("No Script Class %s", m_className.toUtf8().data());
        m_script = 0;
    }

    if (m_script)
    {
        CVarBlock* scriptProps = m_script->GetDefaultProperties();
        if (scriptProps)
        {
            m_properties = scriptProps->Clone(true);
        }

        // Create a game entity archetype.
        m_pArchetype = gEnv->pEntitySystem->LoadEntityArchetype(GetFullName().toUtf8().data());
        if (!m_pArchetype)
        {
            m_pArchetype = gEnv->pEntitySystem->CreateEntityArchetype(m_script->GetClass(), GetFullName().toUtf8().data());
        }
    }

    _smart_ptr<CBaseObject> pBaseObj = new CEntityObject;
    if (pBaseObj)
    {
        pBaseObj->InitVariables();
        m_pObjectVarBlock = pBaseObj->GetVarBlock()->Clone(true);
    }

    if (m_properties)
    {
        CVariableArray* mv_AdditionalProperties = new CVariableArray();
        CVariable<QString>* mv_prototypeMaterial = new CVariable<QString>();

        // Sets the user interface parameters for the additional properties.
        mv_AdditionalProperties->SetName("AdditionalArchetypeProperties");
        mv_AdditionalProperties->SetHumanName("Additional Archetype Properties");

        // Here we setup the prototype material AND add it to the additional
        // properties.
        mv_prototypeMaterial->SetDataType(IVariable::DT_MATERIAL);
        mv_prototypeMaterial->SetHumanName("Prototype Material");
        mv_prototypeMaterial->SetName("PrototypeMaterial");
        mv_AdditionalProperties->AddVariable(mv_prototypeMaterial);

        // Here we add the array of variables we added.
        m_properties->AddVariable(mv_AdditionalProperties);
    }
};

//////////////////////////////////////////////////////////////////////////
void CEntityPrototype::Reload()
{
    if (m_script)
    {
        CVarBlockPtr oldProperties = m_properties;
        m_properties = 0;
        m_script->Reload();
        CVarBlock* scriptProps = m_script->GetDefaultProperties();
        if (scriptProps)
        {
            m_properties = scriptProps->Clone(true);
        }

        if (m_properties != NULL && oldProperties != NULL)
        {
            m_properties->CopyValuesByName(oldProperties);
        }
    }
    Update();
}

//////////////////////////////////////////////////////////////////////////
void CEntityPrototype::Serialize(SerializeContext& ctx)
{
    CBaseLibraryItem::Serialize(ctx);
    XmlNodeRef node = ctx.node;
    if (ctx.bLoading)
    {
        // Loading
        QString className;
        node->getAttr("Class", className);
        node->getAttr("Description", m_description);

        SetEntityClassName(className);

        XmlNodeRef objVarsNode = node->findChild("ObjectVars");
        XmlNodeRef propsNode = GetISystem()->CreateXmlNode("Properties");

        if (m_properties)
        {
            // Serialize properties.
            XmlNodeRef props = node->findChild("Properties");
            if (props)
            {
                m_properties->Serialize(props, ctx.bLoading);

                if (m_pArchetype)
                {
                    // Reload archetype in Engine.
                    m_properties->Serialize(propsNode, false);
                }
            }
        }
        if (m_pObjectVarBlock)
        {
            // Serialize object vars.
            if (objVarsNode)
            {
                m_pObjectVarBlock->Serialize(objVarsNode, ctx.bLoading);
            }
        }

        if (m_pArchetype)
        {
            m_pArchetype->LoadFromXML(propsNode, objVarsNode);
        }
    }
    else
    {
        // Saving.
        node->setAttr("Class", m_className.toUtf8().data());
        node->setAttr("Description", m_description.toUtf8().data());
        if (m_properties)
        {
            // Serialize properties.
            XmlNodeRef props = node->newChild("Properties");
            m_properties->Serialize(props, ctx.bLoading);
        }
        if (m_pObjectVarBlock)
        {
            XmlNodeRef objVarsNode = node->newChild("ObjectVars");
            m_pObjectVarBlock->Serialize(objVarsNode, ctx.bLoading);
        }
    }
}


//////////////////////////////////////////////////////////////////////////
void CEntityPrototype::Update()
{
    if (m_pArchetype)
    {
        // Reload archetype in Engine.
        XmlNodeRef propsNode = GetISystem()->CreateXmlNode("Properties");
        XmlNodeRef objectVarsNode = GetISystem()->CreateXmlNode("ObjectVars");

        if (m_properties)
        {
            m_properties->Serialize(propsNode, false);
        }

        if (m_pObjectVarBlock)
        {
            m_pObjectVarBlock->Serialize(objectVarsNode, false);
        }

        m_pArchetype->LoadFromXML(propsNode, objectVarsNode);
    }

    for (std::list<UpdateCallback>::iterator pCallback = m_updateListeners.begin(); pCallback != m_updateListeners.end(); ++pCallback)
    {
        // Call callback.
        (*pCallback)();
    }

    // Reloads all entities of with this prototype.
    {
        CBaseObjectsArray   cstBaseObject;
        GetIEditor()->GetObjectManager()->GetObjects(cstBaseObject, NULL);

        for (CBaseObjectsArray::iterator it = cstBaseObject.begin(); it != cstBaseObject.end(); ++it)
        {
            CBaseObject* obj = *it;
            if (qobject_cast<CEntityObject*>(obj))
            {
                CEntityObject*  pEntity((CEntityObject*)obj);
                if (pEntity->GetPrototype() == this)
                {
                    pEntity->Reload();
                }
            }
        }
    }
}
