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

//  Description : Applies collision class filtering to the physics and the
//                editor objects


#include "StdAfx.h"
#include "CollisionFilteringProperties.h"
#include "ObjectPhysicsManager.h"
#include "IGamePhysicsSettings.h"
#include "IGameFramework.h"

/* static */
bool CCollisionFilteringProperties::m_bNamesInitialised = false;
QString CCollisionFilteringProperties::m_collision_class_names[k_maxCollisionClasses];


CCollisionFilteringProperties::CCollisionFilteringProperties()
{
    if (m_bNamesInitialised == false)
    {
        //=====================================================
        // Get the names of the collision classes from the game
        //=====================================================
        IGamePhysicsSettings* physicsSettings = gEnv->pGame && gEnv->pGame->GetIGameFramework() ? gEnv->pGame->GetIGameFramework()->GetIGamePhysicsSettings() : 0;
        for (int i = 0; i < k_maxCollisionClasses; i++)
        {
            const char* name = physicsSettings ? physicsSettings->GetCollisionClassName(i) : "";
            m_collision_class_names[i] = name;
        }
        m_bNamesInitialised = true;
    }
    m_collisionClass = SCollisionClass(0, 0);
}

CCollisionFilteringProperties::~CCollisionFilteringProperties()
{
}

static void AddVariable(CVariableBase& varArray, CVariableBase& var, const QString& varName, IVariable::OnSetCallback func)
{
    var.AddRef();
    var.SetName(varName);
    var.AddOnSetCallback(func);
    varArray.AddVariable(&var);
}

void CCollisionFilteringProperties::AddToObject(CVarObject* object, IVariable::OnSetCallback func)
{
    static QString sVarName_collision_classes = "CollisionFiltering";
    static QString sVarName_collision_classes_type = "Type";
    static QString sVarName_collision_classes_ignore = "Ignore";

    m_callbackFunc = func;

    // Add the root collision class table
    object->AddVariable(mv_collision_classes, sVarName_collision_classes, functor(*this, &CCollisionFilteringProperties::OnVarChange));

    // Add two tables, Type and Ignore
    ::AddVariable(mv_collision_classes, mv_collision_classes_type, sVarName_collision_classes_type, functor(*this, &CCollisionFilteringProperties::OnVarChange));
    ::AddVariable(mv_collision_classes, mv_collision_classes_ignore, sVarName_collision_classes_ignore, functor(*this, &CCollisionFilteringProperties::OnVarChange));

    // Add all the non-empty collision classes
    for (int i = 0; i < k_maxCollisionClasses; i++)
    {
        if (m_collision_class_names[i] != "")
        {
            ::AddVariable(mv_collision_classes_type, mv_collision_classes_type_flag[i], m_collision_class_names[i], functor(*this, &CCollisionFilteringProperties::OnVarChange));
            ::AddVariable(mv_collision_classes_ignore, mv_collision_classes_ignore_flag[i], m_collision_class_names[i], functor(*this, &CCollisionFilteringProperties::OnVarChange));
        }
    }
}

void CCollisionFilteringProperties::OnVarChange(IVariable* var)
{
    // Calculate the new collision class
    m_collisionClass = SCollisionClass(0, 0);
    for (int i = 0; i < 32; i++)
    {
        if (mv_collision_classes_type_flag[i])
        {
            m_collisionClass.type |= 1 << i;
        }
        if (mv_collision_classes_ignore_flag[i])
        {
            m_collisionClass.ignore |= 1 << i;
        }
    }
    m_callbackFunc(var);
}

void CCollisionFilteringProperties::ApplyToPhysicalEntity(IPhysicalEntity* pPhysics)
{
    if (pPhysics)
    {
        pe_params_collision_class pcc;
        pcc.collisionClassOR = m_collisionClass;
        pcc.collisionClassAND = m_collisionClass;
        pPhysics->SetParams(&pcc);
    }
}

int CCollisionFilteringProperties::GetCollisionClassExportId()
{
    // This should only be called during the export phase
    return GetIEditor()->GetObjectManager()->GetPhysicsManager()->RegisterCollisionClass(m_collisionClass);
}




