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

#include <AzCore/PlatformDef.h> // for AZ_COMMAND_LINE_LEN


struct IOutputPrintSink;


namespace LumberyardLauncher
{
    struct PlatformMainInfo
    {
        typedef bool (*ResourceLimitUpdater)();
        typedef void (*OnPostApplicationStart)();

        PlatformMainInfo() = default;

        //! Copy the command line into a buffer as is or reconstruct a
        //! quoted version of the command line from the arg c/v.  The
        //! internal buffer is fixed to \ref AZ_COMMAND_LINE_LEN meaning
        //! this call can fail if the command line exceeds that length
        bool CopyCommandLine(const char* commandLine);
        bool CopyCommandLine(int argc, char** argv);
        bool AddArgument(const char* arg);


        char m_commandLine[AZ_COMMAND_LINE_LEN] = { 0 };
        size_t m_commandLineLen = 0;

        ResourceLimitUpdater m_updateResourceLimits = nullptr; //!< callback for updating system resources, if necessary
        OnPostApplicationStart m_onPostAppStart = nullptr;  //!< callback notifying the platform specific entry point that AzGameFramework::GameApplication::Start has been called
        AZ::IAllocatorAllocate* m_allocator = nullptr; //!< Used to allocate the temporary bootstrap memory, as well as the main \ref SystemAllocator heap. If null, OSAllocator will be used

        const char* m_appResourcesPath = "."; //!< Path to the device specific assets, default is equivalent to blank path in ParseEngineConfig
        const char* m_appWriteStoragePath = nullptr; //!< Path to writeable storage if different than assets path, used to override userPath and logPath

        const char* m_additionalVfsResolution = nullptr; //!< additional things to check if VFS is not working for the desired platform

        void* m_window = nullptr; //!< maps to \ref SSystemInitParams::hWnd
        void* m_instance = nullptr; //!< maps to \ref SSystemInitParams::hInstance
        IOutputPrintSink* m_printSink = nullptr; //!< maps to \ref SSystemInitParams::pPrintSync 
    };

    enum class ReturnCode : unsigned char
    {
        Success = 0,

        ErrExePath,             //!< Failed to get the executable path
        ErrBootstrapMismatch,   //!< Failed to validate launcher compiler defines with bootstrap values
        ErrCommandLine,         //!< Failed to copy the command line
        ErrValidation,          //!< Failed to validate secret
        ErrResourceLimit,       //!< Failed to increase unix resource limits
        ErrAppDescriptor,       //!< Failed to locate the application descriptor file
        ErrLegacyGameLib,       //!< Failed to load required legacy game project library
        ErrLegacyGameStartup,   //!< Failed to locate the CreateGameStartup symbol from the legacy game project library
        ErrCrySystemLib,        //!< Failed to load required CrySystem library
        ErrCrySystemInterface,  //!< Failed to create the CrySystem interface
        ErrCryEnvironment,      //!< Failed to initialize the CryEngine environment
    };

    const char* GetReturnCodeString(ReturnCode code);

    //! The main entry point for all lumberyard launchers
    ReturnCode Run(const PlatformMainInfo& mainInfo = PlatformMainInfo());
}

