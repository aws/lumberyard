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

#include <AzCore/Component/Component.h>
#include <AzToolsFramework/API/EditorPythonConsoleBus.h>
#include <AzToolsFramework/API/EditorPythonRunnerRequestsBus.h>
#include <AzCore/std/parallel/semaphore.h>

namespace EditorPythonBindings
{
    /**
      * Manages the Python interpreter inside this Gem (Editor only)
      * - redirects the Python standard output and error streams to AZ_TracePrintf and AZ_Warning, respectively
      */      
    class PythonSystemComponent
        : public AZ::Component
        , protected AzToolsFramework::EditorPythonEventsInterface
        , protected AzToolsFramework::EditorPythonRunnerRequestBus::Handler
    {
    public:
        AZ_COMPONENT(PythonSystemComponent, "{97F88B0F-CF68-4623-9541-549E59EE5F0C}");

        static void Reflect(AZ::ReflectContext* context);

        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided);
        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible);

    protected:
        ////////////////////////////////////////////////////////////////////////
        // AZ::Component interface implementation
        void Activate() override;
        void Deactivate() override;
        ////////////////////////////////////////////////////////////////////////

        ////////////////////////////////////////////////////////////////////////
        // AzToolsFramework::EditorPythonEventsInterface
        bool StartPython() override;
        bool StopPython() override;
        void WaitForInitialization() override;
        ////////////////////////////////////////////////////////////////////////

        ////////////////////////////////////////////////////////////////////////
        // AzToolsFramework::EditorPythonRunnerRequestBus::Handler interface implementation
        void ExecuteByString(AZStd::string_view script) override;
        void ExecuteByFilename(AZStd::string_view filename) override;
        void ExecuteByFilenameWithArgs(AZStd::string_view filename, const AZStd::vector<AZStd::string_view>& args) override;
        void ExecuteByFilenameAsTest(AZStd::string_view filename, const AZStd::vector<AZStd::string_view>& args) override;
        ////////////////////////////////////////////////////////////////////////
        
    private:
        // handle multiple Python initializers
        AZStd::atomic_int m_initalizeWaiterCount {0};
        AZStd::semaphore m_initalizeWaiter;
    
        enum class Result
        {
            Okay,
            Error_IsNotInitialized,
            Error_InvalidFilename,
            Error_MissingFile,
            Error_FileOpenValidation,
            Error_InternalException,
            Error_PythonException,
        };
        Result EvaluateFile(AZStd::string_view filename, const AZStd::vector<AZStd::string_view>& args);

        // bootstrap logic and data
        using PythonPathStack = AZStd::vector<AZStd::string>;
        void DiscoverPythonPaths(PythonPathStack& pythonPathStack);
        void ExecuteBoostrapScripts(const PythonPathStack& pythonPathStack);

        // starts the Python interpreter 
        bool StartPythonInterpreter(const PythonPathStack& pythonPathStack);
        bool StopPythonInterpreter();
    };
}
