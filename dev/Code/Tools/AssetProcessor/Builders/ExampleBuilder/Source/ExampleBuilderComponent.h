/*
* All or portions of this file Copyright(c) Amazon.com, Inc.or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution(the "License").All use of this software is governed by the License,
*or, if provided, by the license below or the license accompanying this file.Do not
* remove or modify any license notices.This file is distributed on an "AS IS" BASIS,
*WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/

#ifndef EXAMPLEBUILDER_H
#define EXAMPLEBUILDER_H

#pragma once

#include <AzCore/Component/Component.h>
#include <AssetBuilderSDK/AssetBuilderBusses.h>
#include <AssetBuilderSDK/AssetBuilderSDK.h>

namespace ExampleBuilder
{
    //! Here is an example of a builder worker that actually performs the building of assets.
    //! In this example, we only register one, but you can have as many different builders in a single builder module as you want.
    class ExampleBuilderWorker
        : public AssetBuilderSDK::AssetBuilderCommandBus::Handler // this will deliver you the "shut down!" message on another thread.
    {
    public:
        ExampleBuilderWorker();
        ~ExampleBuilderWorker();

        //! Asset Builder Callback Functions
        void CreateJobs(const AssetBuilderSDK::CreateJobsRequest& request, AssetBuilderSDK::CreateJobsResponse& response);
        void ProcessJob(const AssetBuilderSDK::ProcessJobRequest& request, AssetBuilderSDK::ProcessJobResponse& response);

        //////////////////////////////////////////////////////////////////////////
        //!AssetBuilderSDK::AssetBuilderCommandBus interface
        void ShutDown() override; // if you get this you must fail all existing jobs and return.
        //////////////////////////////////////////////////////////////////////////

        static AZ::Uuid GetUUID();

    private:
        bool m_isShuttingDown = false;
    };

    //! Here's an example of the lifecycle Component you must implement.
    //! You must have at least one component to handle the lifecycle of your module.
    //! You could make this also be a builder if you also listen to the builder bus, and register itself as a builder
    //! But for the purposes of clarity, we will make it just be the lifecycle component in this example plugin
    class BuilderPluginComponent
        : public AZ::Component
    {
    public:
        AZ_COMPONENT(BuilderPluginComponent, "{8872211E-F704-48A9-B7EB-7B80596D871D}")
        static void Reflect(AZ::ReflectContext* context);

        BuilderPluginComponent(); // avoid initialization here.

        //////////////////////////////////////////////////////////////////////////
        // AZ::Component
        virtual void Init(); // create objects, allocate memory and initialize yourself without reaching out to the outside world
        virtual void Activate(); // reach out to the outside world and connect up to what you need to, register things, etc.
        virtual void Deactivate(); // unregister things, disconnect from the outside world
        //////////////////////////////////////////////////////////////////////////

        virtual ~BuilderPluginComponent(); // free memory an uninitialize yourself.

    private:
        ExampleBuilderWorker m_exampleBuilder;
    };
}

#endif //EXAMPLEBUILDER_H