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
#ifndef CRYINCLUDE_CRYENTITYSYSTEM_COMPONENTMODULETEST_H
#define CRYINCLUDE_CRYENTITYSYSTEM_COMPONENTMODULETEST_H
#pragma once

#include <Components/IComponentModuleTest.h>

//! See IComponentModuleTest for documentation
class CComponentModuleTest
    : public IComponentModuleTest
{
public:
    CComponentModuleTest();
    ~CComponentModuleTest();

    int GetConstructorRunCount() const override;
    int GetDestructorRunCount() const override;

    bool CreateStaticSharedPtr() override;
    IComponentModuleTestPtr GetStaticSharedPtr() override;
    IComponentModuleTestPtr& GetStaticSharedPtrRef() override;
};

#endif // CRYINCLUDE_CRYENTITYSYSTEM_COMPONENTMODULETEST_H
