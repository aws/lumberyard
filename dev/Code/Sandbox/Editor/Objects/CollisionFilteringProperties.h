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

#ifndef CRYINCLUDE_EDITOR_OBJECTS_COLLISIONFILTERINGPROPERTIES_H
#define CRYINCLUDE_EDITOR_OBJECTS_COLLISIONFILTERINGPROPERTIES_H
#pragma once


class CCollisionFilteringProperties
{
public:
    enum
    {
        k_maxCollisionClasses = 32
    };

public:
    CCollisionFilteringProperties();
    ~CCollisionFilteringProperties();

    // Add the variables to the object properties
    void AddToObject(CVarObject* object, IVariable::OnSetCallback func);

    // Helper to add the filtering to the physical entity
    void ApplyToPhysicalEntity(IPhysicalEntity* pPhysics);

    const SCollisionClass& GetCollisionClass() { return m_collisionClass; }
    int GetCollisionClassExportId();

protected:

    void OnVarChange(IVariable* var);

    CVariableArray mv_collision_classes;
    CVariableArray mv_collision_classes_type;
    CVariableArray mv_collision_classes_ignore;
    CVariable<bool> mv_collision_classes_type_flag[k_maxCollisionClasses];
    CVariable<bool> mv_collision_classes_ignore_flag[k_maxCollisionClasses];
    IVariable::OnSetCallback m_callbackFunc;
    SCollisionClass m_collisionClass;

    static bool m_bNamesInitialised;
    static QString m_collision_class_names[k_maxCollisionClasses];
};


#endif // CRYINCLUDE_EDITOR_OBJECTS_COLLISIONFILTERINGPROPERTIES_H
