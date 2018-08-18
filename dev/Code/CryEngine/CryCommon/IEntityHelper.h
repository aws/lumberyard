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

//! IEntityHelper.h exists because of circular dependencies in the
//! body of some function templates. Both IEntity.h and IEntityHelper.h
//! must be included in any file calling one of these functions.
//! Do not add anything more to IEntityHelper.h unless it also suffers
//! from circular dependencies.

#ifndef CRYINCLUDE_CRYCOMMON_IENTITYHELPER_H
#define CRYINCLUDE_CRYCOMMON_IENTITYHELPER_H
#pragma once

#include "IEntity.h"
#include "IComponent.h"
#include "ISystem.h"

template <class T>
inline typename std::enable_if<std::is_base_of<IComponent, T>::value, AZStd::shared_ptr<T> >::type IEntity::GetOrCreateComponent()
{
    AZStd::shared_ptr<T> component = GetComponent<T>();
    if (!component)
    {
        component = gEnv->pEntitySystem->CreateComponentAndRegister<T>(IComponent::SComponentInitializer(this));
    }
    return component;
}

#endif // CRYINCLUDE_CRYCOMMON_IENTITYHELPER_H