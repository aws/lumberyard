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

#include "CryLegacy_precompiled.h"

//////////////////////////////////////////////////////////////////////////
/// Inventory Flow Node Registration
//////////////////////////////////////////////////////////////////////////

// Lumberyard: We have removed these nodes from the Flow Node catalog because they
// relied on "GameSDK" functionality that is no longer present and/or fully functional.
// Many of these nodes are generic in meaning and could be useful in the future.
// They will need to be re-implemented on an need-basis.
// Preserving the names and intentions of the nodes here for future reference.
// We can always use source control if we ever need to dig them up again.

//REGISTER_FLOW_NODE("Inventory:ItemAdd", CFlowNode_InventoryAddItem);
//  Adds an Item to specified Actor's Inventory.

//REGISTER_FLOW_NODE("Inventory:ItemRemove", CFlowNode_InventoryRemoveItem);
//  Removes an Item from specified Actor's Inventory.

//REGISTER_FLOW_NODE("Inventory:ItemRemoveAll", CFlowNode_InventoryRemoveAllItems);
//  Removes all Items from specified Actor's Inventory.

//REGISTER_FLOW_NODE("Inventory:ItemCheck", CFlowNode_InventoryHasItem);
//  Checks specified Actor's Inventory for existence of an Item.

//REGISTER_FLOW_NODE("Inventory:AmmoCheck", CFlowNode_InventoryHasAmmo);
//  Checks specified Actor's Inventory for existence of a particular Ammo type and returns its count.


//  Select an Item in a specified Actor's Inventory.
//  Item must exist in Inventory.

//REGISTER_FLOW_NODE("Inventory:ItemSelected", CFlowNode_InventoryItemSelected);
//  Track the currently selected Item in a specified Actor's Inventory.
//  Outputs information about the Item (Class, Name, Id).

//REGISTER_FLOW_NODE("Inventory:AmmoAdd", CFlowNode_InventoryAddAmmo);
//  Add a specified amount of an Ammo type to a specified Actor's Inventory.

//  Set a specified amount of an Ammo type on a specified Actor's Inventory.
//REGISTER_FLOW_NODE("Inventory:AmmoRemoveAll", CFlowNode_InventoryRemoveAllAmmo);
//  Removes all Ammo from a specified Actor's Inventory.
//  Optionally removes all Ammo from all Weapons.

//REGISTER_FLOW_NODE("Inventory:EquipPackAdd", CFlowNode_AddEquipmentPack);
//  Add an Equipment Pack to a specified Actor's Inventory.
//  Optionally add to or replace existing Equipment Pack.
//  Optionally selects the primary Weapon in added Equipment Pack.

//REGISTER_FLOW_NODE("Inventory:PlayerInventoryStore", CFlowNode_StorePlayerInventory);
//  Serialize a Player's Inventory (Save).

//REGISTER_FLOW_NODE("Inventory:PlayerInventoryRestore", CFlowNode_RestorePlayerInventory);
//  Restore a Player's Inventory (Load).
