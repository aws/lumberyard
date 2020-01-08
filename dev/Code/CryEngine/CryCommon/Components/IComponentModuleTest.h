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
#ifndef CRYINCLUDE_CRYCOMMON_ICOMPONENTMODULETEST_H
#define CRYINCLUDE_CRYCOMMON_ICOMPONENTMODULETEST_H
#pragma once

#ifdef AZ_TESTS_ENABLED

#include <IComponent.h>

class IComponentModuleTest;
DECLARE_SMART_POINTERS(IComponentModuleTest);

//! Exists to test component functionality when those components are used
//! across module (DLL) boundaries.
//! <p>
//! IComponentModuleTest contains many non-static functions which give access
//! to static data. If the functions were static and declared in
//! CryCommon, then each module would have its own copy of the data.
//! By using virtual member functions we can be sure that we are accessing
//! data solely from the module which implements IComponentModuleTest.
class IComponentModuleTest
    : public IComponent
    , public NoCopy // ensure that allocation is explicit and not accidental
{
public:
    DECLARE_COMPONENT_TYPE("ComponentModuleTest", 0xD591CCA3CE164C51, 0x821ED9E01588D0E4)
    IComponentModuleTest() {}

    //! Check number of times this class's constructor has EVER been run.
    //! This can be used to check that an object is not being duplicated
    //! unnecessarily when passed across module boundaries.
    virtual int GetConstructorRunCount() const = 0;

    //! \return number of times this class's destructor has EVER been run
    virtual int GetDestructorRunCount() const = 0;

    //! A static instance of a shared_ptr lives in the module implementing
    //! IComponentModuleTest. Use this to test that shared_ptrs work
    //! correctly across module boundaries.
    //! \return the static shared_ptr by reference
    virtual IComponentModuleTestPtr& GetStaticSharedPtrRef() = 0;

    //! \return the static shared_ptr by value
    virtual IComponentModuleTestPtr GetStaticSharedPtr() = 0;

    //! Create a new instance of this class, owned by the static shared_ptr.
    //! \return whether creation was successful
    virtual bool CreateStaticSharedPtr() = 0;
};

#endif // AZ_TESTS_ENABLED
#endif // CRYINCLUDE_CRYCOMMON_ICOMPONENTMODULETEST_H
