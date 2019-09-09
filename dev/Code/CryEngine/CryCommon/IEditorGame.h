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

// Description : The Editor->Game communication interface.

#pragma once

#include <AzCore/EBus/EBus.h>

#include <ISystem.h>

struct IFlowSystem;
struct IGameTokenSystem;
namespace Telemetry {
    struct ITelemetryRepository;
}

// For game to access Editor functionality.
struct IGameToEditorInterface
{
    // <interfuscator:shuffle>
    virtual ~IGameToEditorInterface(){}
    virtual void SetUIEnums(const char* sEnumName, const char** sStringsArray, int nStringCount) = 0;
    // </interfuscator:shuffle>
};

struct IEquipmentSystemInterface
{
    struct IEquipmentItemIterator
    {
        struct SEquipmentItem
        {
            const char* name;
            const char* type;
        };
        // <interfuscator:shuffle>
        virtual ~IEquipmentItemIterator(){}
        virtual void AddRef() = 0;
        virtual void Release() = 0;
        virtual bool Next(SEquipmentItem& outItem) = 0;
        // </interfuscator:shuffle>
    };
    typedef _smart_ptr<IEquipmentItemIterator> IEquipmentItemIteratorPtr;

    // <interfuscator:shuffle>
    virtual ~IEquipmentSystemInterface(){}

    // return iterator with available equipment items of a certain type
    // type can be empty to retrieve all items
    virtual IEquipmentSystemInterface::IEquipmentItemIteratorPtr CreateEquipmentItemIterator(const char* type = "") = 0;
    virtual IEquipmentSystemInterface::IEquipmentItemIteratorPtr CreateEquipmentAccessoryIterator(const char* type) = 0;

    // delete all equipment packs
    virtual void DeleteAllEquipmentPacks() = 0;

    // load a single equipment pack from an XmlNode
    // Equipment Pack is basically
    // <EquipPack name="BasicPack">
    //   <Items>
    //      <Scar type="Weapon" />
    //      <SOCOM type="Weapon" />
    //   </Items>
    //   <Ammo Scar="50" SOCOM="70" />
    // </EquipPack>

    virtual bool LoadEquipmentPack(const XmlNodeRef& rootNode) = 0;

    // set the players equipment pack. maybe we enable this, but normally via FG only
    // virtual void SetPlayerEquipmentPackName(const char *packName) = 0;
    // </interfuscator:shuffle>
};

// Summary
//      Interface used by the Editor to interact with the GameDLL
struct IEditorGame
{
    typedef IEditorGame*(* TEntryFunction)();

    struct HelpersDrawMode
    {
        enum EType
        {
            Hide = 0,
            Show
        };
    };

    virtual ~IEditorGame(){}
    virtual bool Init(ISystem* pSystem, IGameToEditorInterface* pEditorInterface) = 0;
    /// Called after all Editor systems have been initialized.
    virtual void PostInit() { };
    AZ_DEPRECATED(virtual int Update(bool haveFocus, unsigned int updateFlags), "Deprecated, please delete overridden functions (main loop now in launcher)") { return 0; }
    virtual void Shutdown() = 0;
    virtual bool SetGameMode(bool bGameMode) = 0;
    virtual IEntity* GetPlayer() = 0;
    virtual void SetPlayerPosAng(Vec3 pos, Vec3 viewDir) = 0;
    virtual void HidePlayer(bool bHide) = 0;
    virtual void OnBeforeLevelLoad() { };
    virtual void OnAfterLevelInit(const char* levelName, const char* levelFolder) { }
    virtual void OnAfterLevelLoad(const char* levelName, const char* levelFolder) { }
    virtual void OnAfterLevelSave() { }
    /// Called when a level is closed, before any systems are shutdown.
    virtual void OnCloseLevel() { }
    virtual bool BuildEntitySerializationList(XmlNodeRef output) { return true; }
    virtual bool GetAdditionalMinimapData(XmlNodeRef output) { return true; }

    virtual IFlowSystem* GetIFlowSystem() = 0;
    virtual IGameTokenSystem* GetIGameTokenSystem() = 0;

    /// This instance will only be used by CEquipPackLib and CEquipPackDialog,
    /// but the instance of IEditorGame is responsible for owning it.
    virtual IEquipmentSystemInterface* GetIEquipmentSystemInterface() { return nullptr; }

    /// Return true to allow ToggleMultiplayerGameRules() to be called when the "Toggle SP/MP Game Rules" option is selected.
    /// Return false to show a warning when attempting to switch.
    virtual bool SupportsMultiplayerGameRules() const { return false; }
    /// Called when the "Toggle SP/MP Game Rules" option is selected (if SupportsMultiplayerGameRules() returns true).
    virtual void ToggleMultiplayerGameRules() { }

    /// Update (and render) all sorts of generic editor 'helpers' that could be used, for example, to
    /// render certain metrics, boundaries, invalid links, etc. Called on Update
    virtual void UpdateHelpers(const HelpersDrawMode::EType drawMode) { }
    /// Called when the visibility of render helpers changes (via Python, hotkey, etc.)
    virtual void OnDisplayRenderUpdated(bool displayHelpers) { }
};


// Summary
//      Defines the EBus Events to create the IGameStartup and IEditorGame system interfaces that are needed to be expose 
//      for a particular game
class EditorGameEvents
    : public AZ::EBusTraits
{
public:
    /// \ref EBusHandlerPolicy
    static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;

    /// \ref EBusAddressPolicy
    static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::Single;

    //! Create the IGameStartup interface handle for the current game module
    virtual IGameStartup* CreateGameStartup() = 0;

    //! Create the IEditorGame interface handle for the current game module for Editor mode
    virtual IEditorGame* CreateEditorGame() = 0;
};

typedef AZ::EBus<EditorGameEvents> EditorGameRequestBus;

