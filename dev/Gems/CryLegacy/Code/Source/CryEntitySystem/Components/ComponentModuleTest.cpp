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
#include "CryLegacy_precompiled.h"

#include "ComponentModuleTest.h"

DECLARE_DEFAULT_COMPONENT_FACTORY(CComponentModuleTest, IComponentModuleTest)

namespace ComponentModuleTestStatics
{
    static int sConstructorRunCount = 0;
    static int sDestructorRunCount = 0;
    static IComponentModuleTestPtr sSharedPtrInstance;
} // namespace ComponentModuleTestStatics

CComponentModuleTest::CComponentModuleTest()
{
    ComponentModuleTestStatics::sConstructorRunCount++;
}

CComponentModuleTest::~CComponentModuleTest()
{
    ComponentModuleTestStatics::sDestructorRunCount++;
}

int CComponentModuleTest::GetConstructorRunCount() const
{
    return ComponentModuleTestStatics::sConstructorRunCount;
}

int CComponentModuleTest::GetDestructorRunCount() const
{
    return ComponentModuleTestStatics::sDestructorRunCount;
}

bool CComponentModuleTest::CreateStaticSharedPtr()
{
    if (!gEnv->pEntitySystem)
    {
        return false;
    }

    IComponentFactoryRegistry* registry = gEnv->pEntitySystem->GetComponentFactoryRegistry();
    if (!registry)
    {
        return false;
    }

    auto factory = registry->GetFactory<IComponentModuleTest>();
    if (!factory)
    {
        return false;
    }

    ComponentModuleTestStatics::sSharedPtrInstance = factory->CreateComponent();

    return static_cast<bool>(ComponentModuleTestStatics::sSharedPtrInstance);
}

IComponentModuleTestPtr CComponentModuleTest::GetStaticSharedPtr()
{
    return ComponentModuleTestStatics::sSharedPtrInstance;
}

IComponentModuleTestPtr& CComponentModuleTest::GetStaticSharedPtrRef()
{
    return ComponentModuleTestStatics::sSharedPtrInstance;
}
