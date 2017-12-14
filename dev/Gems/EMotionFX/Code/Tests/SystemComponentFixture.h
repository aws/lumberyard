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

#pragma once

#include <AzTest/AzTest.h>

#include <AzCore/Component/ComponentApplication.h>
#include <AzFramework/IO/LocalFileIO.h>

namespace EMotionFX
{

    //! A fixture that constructs an EMotionFX::Integration::SystemComponent
    /*!
     * This fixture can be used by any test that needs the EMotionFX runtime to
     * be working. It will construct all necessary allocators for EMotionFX
     * objects to be successfully instantiated.
    */
    class SystemComponentFixture : public ::testing::Test
    {
    public:
        void SetUp() override;

        void TearDown() override;

    private:
        // The ComponentApplication must not be a pointer, because it cannot be
        // dynamically allocated. Calls to new will try to use the SystemAllocator
        // that has not been created yet. If one is created before
        // ComponentApplication::Create() is called, ComponentApplication will
        // complain that there's already a SystemAllocator, as it tries to make one
        // itself.
        AZ::ComponentApplication mApp;

        // The destructor of the LocalFileIO object uses the AZ::OSAllocator. Make
        // sure that it still exists when this fixture is destroyed.
        AZ::IO::LocalFileIO mLocalFileIO;

        AZ::Entity* mSystemEntity;
    };

} // end namespace EMotionFX
