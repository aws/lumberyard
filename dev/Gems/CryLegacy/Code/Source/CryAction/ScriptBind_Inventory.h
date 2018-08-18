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

// Description : Script Binding for Inventory


#ifndef CRYINCLUDE_CRYACTION_SCRIPTBIND_INVENTORY_H
#define CRYINCLUDE_CRYACTION_SCRIPTBIND_INVENTORY_H
#pragma once


#include <IScriptSystem.h>
#include <ScriptHelpers.h>


struct IItemSystem;
struct IGameFramework;
class CInventory;


class CScriptBind_Inventory
    : public CScriptableBase
{
public:
    CScriptBind_Inventory(ISystem* pSystem, IGameFramework* pGameFramework);
    virtual ~CScriptBind_Inventory();
    void Release() { delete this; };
    void AttachTo(CInventory* pInventory);
    void DetachFrom(CInventory* pInventory);

    virtual void GetMemoryUsage(ICrySizer* pSizer) const
    {
        pSizer->AddObject(this, sizeof(*this));
    }

    //! <code>Inventory.Destroy()</code>
    //! <description>Destroys the inventory.</description>
    int Destroy(IFunctionHandler* pH);

    //! <code>Inventory.Clear()</code>
    //! <description>Clears the inventory.</description>
    int Clear(IFunctionHandler* pH);

    //! <code>Inventory.Dump()</code>
    //! <description>Dumps the inventory.</description>
    int Dump(IFunctionHandler* pH);

    //! <code>Inventory.GetItemByClass( className )</code>
    //!     <param name="className">Class name.</param>
    //! <description>Gets item by class name.</description>
    int GetItemByClass(IFunctionHandler* pH, const char* className);

    //! <code>Inventory.GetGrenadeWeaponByClass( className )</code>
    //!     <param name="className">Class name.</param>
    //! <description>Gets grenade weapon by class name.</description>
    int GetGrenadeWeaponByClass(IFunctionHandler* pH, const char* className);

    //! <code>Inventory.HasAccessory( accessoryName )</code>
    //!     <param name="accessoryName">Accessory name.</param>
    //! <description>Checks if the inventory contains the specified accessory.</description>
    int HasAccessory(IFunctionHandler* pH, const char* accessoryName);

    //! <code>Inventory.GetCurrentItemId()</code>
    //! <description>Gets the identifier of the current item.</description>
    int GetCurrentItemId(IFunctionHandler* pH);

    //! <code>Inventory.GetCurrentItem()</code>
    //! <description>Gets the current item.</description>
    int GetCurrentItem(IFunctionHandler* pH);

    //! <code>Inventory.GetAmmoCount(ammoName)</code>
    //!     <param name="ammoName">Ammunition name.</param>
    //! <description>Gets the amount of the specified ammunition name.</description>
    int GetAmmoCount(IFunctionHandler* pH, const char* ammoName);

    //! <code>Inventory.GetAmmoCapacity( ammoName )</code>
    //!     <param name="ammoName">Ammunition name.</param>
    //! <description>Gets the capacity for the specified ammunition.</description>
    int GetAmmoCapacity(IFunctionHandler* pH, const char* ammoName);

    //! <code>Inventory.SetAmmoCount( ammoName, count )</code>
    //!     <param name="ammoName">Ammunition name.</param>
    //!     <param name="count">Ammunition amount.</param>
    //! <description>Sets the amount of the specified ammunition.</description>
    int SetAmmoCount(IFunctionHandler* pH, const char* ammoName, int count);

private:
    void RegisterGlobals();
    void RegisterMethods();

    CInventory* GetInventory(IFunctionHandler* pH);

    ISystem* m_pSystem;
    IEntitySystem* m_pEntitySystem;
    IGameFramework* m_pGameFramework;
};


#endif // CRYINCLUDE_CRYACTION_SCRIPTBIND_INVENTORY_H
