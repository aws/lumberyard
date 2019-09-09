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

// Description : This is the interface which the Launcher.exe will interact
//               with the game dll. For an implementation of this interface
//               refer to the GameDll project of the title or MOD you are
//               working on.

#pragma once

#include <AzCore/EBus/EBus.h>
#include <ISystem.h>
#include <IGame.h> // <> required for Interfuscator
#include "IGameRef.h"


struct ILog;
struct ILogCallback;
struct IValidator;
struct ISystemUserCallback;

// Summary
//   Interfaces used to initialize a game using CryEngine
struct IGameStartup
{
    // Summary
    //   Entry function to the game
    // Returns
    //   a new instance of the game startup
    // Description
    //   Entry function used to create a new instance of the game
    typedef IGameStartup*(* TEntryFunction)();

    // <interfuscator:shuffle>
    virtual ~IGameStartup(){}

    // Description:
    //      Initialize the game and/or any MOD, and get the IGameMod interface.
    //      The shutdown method, must be called independent of this method's return value.
    // Arguments:
    //      startupParams - Pointer to SSystemInitParams structure containing system initialization setup!
    // Return Value:
    //      Pointer to a IGameMod interface, or 0 if something went wrong with initialization.
    virtual IGameRef Init(SSystemInitParams& startupParams) = 0;

    // Description:
    //      Shuts down the game and any loaded MOD and delete itself.
    virtual void Shutdown() = 0;

    // Deprecated
    AZ_DEPRECATED(virtual int Update(bool haveFocus, unsigned int updateFlags), "Deprecated, please delete overridden functions (main loop now in launcher)") { return 0; }

    // Description:
    //      Returns a restart level and thus triggers a restart.
    // Return Value:
    //      false to continue, true to quit and load a new level, specified by assigning levelName.
    virtual bool GetRestartLevel(char** levelName) { return false; }

    // Description:
    //      Returns whether a patch needs installing
    //  Return Value:
    //      nullptr to ignore, or the path to the executable to run to install a patch.
    virtual const char* GetPatch() const { return nullptr; }

    //      Retrieves the next mod to use in case the engine got a restart request.
    // Return Value:
    //      false to continue, true to quit and load a new mod, specified by
    //      assigning pModNameBuffer (must be shorter than modNameBufferSizeInBytes).
    virtual bool GetRestartMod(char* pModNameBuffer, int modNameBufferSizeInBytes) { return false; }

    // Deprecated:
    AZ_DEPRECATED(virtual int Run(const char* autoStartLevelName), "Deprecated, please delete overridden functions (main loop now in launcher)") { return 0; }

    // Description:
    //      Returns the RSA Public Key used by the engine to decrypt pak files which are encrypted by an offline tool.
    //      Part of the tools package includes a key generator, which will generate a header file with the public key which can be easily included in code.
    //      This *should* ideally be in IGame being game specific info, but paks are loaded very early on during boot and this data is needed to init CryPak.
    virtual const uint8* GetRSAKey(uint32* pKeySize) const {* pKeySize = 0; return NULL; }
    // </interfuscator:shuffle>
};

