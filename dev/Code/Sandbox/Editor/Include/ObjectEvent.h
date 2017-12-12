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

#ifndef CRYINCLUDE_EDITOR_INCLUDE_OBJECTEVENT_H
#define CRYINCLUDE_EDITOR_INCLUDE_OBJECTEVENT_H
#pragma once

//! Standart objects types.
enum ObjectType
{
    OBJTYPE_GROUP           = 1 << 0,
    OBJTYPE_TAGPOINT        = 1 << 1,
    OBJTYPE_AIPOINT         = 1 << 2,
    OBJTYPE_ENTITY          = 1 << 3,
    OBJTYPE_SHAPE           = 1 << 4,
    OBJTYPE_VOLUME          = 1 << 5,
    OBJTYPE_BRUSH           = 1 << 6,
    OBJTYPE_PREFAB          = 1 << 7,
    OBJTYPE_SOLID           = 1 << 8,
    OBJTYPE_CLOUD           = 1 << 9,
    OBJTYPE_CLOUDGROUP      = 1 << 10,
    OBJTYPE_VOXEL           = 1 << 11,
    OBJTYPE_ROAD            = 1 << 12,
    OBJTYPE_OTHER           = 1 << 13,
    OBJTYPE_DECAL           = 1 << 14,
    OBJTYPE_DISTANCECLOUD   = 1 << 15,
    OBJTYPE_TELEMETRY       = 1 << 16,
    OBJTYPE_REFPICTURE      = 1 << 17,
    OBJTYPE_GEOMCACHE = 1 << 18,
    OBJTYPE_VOLUMESOLID = 1 << 19,
    OBJTYPE_DUMMY = 1 << 20,
    OBJTYPE_AZENTITY        = 1 << 21,
    OBJTYPE_ANY = 0xFFFFFFFF,
};

//////////////////////////////////////////////////////////////////////////
//! Events that objects may want to handle.
//! Passed to OnEvent method of CBaseObject.
enum ObjectEvent
{
    EVENT_INGAME    = 1,    //!< Signals that editor is switching into the game mode.
    EVENT_OUTOFGAME,        //!< Signals that editor is switching out of the game mode.
    EVENT_REFRESH,          //!< Signals that editor is refreshing level.
    EVENT_PLACE_STATIC, //!< Signals that editor needs to place all static objects on terrain.
    EVENT_REMOVE_STATIC,//!< Signals that editor needs to remove all static objects from terrain.
    EVENT_DBLCLICK,         //!< Signals that object have been double clicked.
    EVENT_KEEP_HEIGHT,  //!< Signals that object must preserve its height over changed terrain.
    EVENT_UNLOAD_ENTITY,//!< Signals that entities scripts must be unloaded.
    EVENT_RELOAD_ENTITY,//!< Signals that entities scripts must be reloaded.
    EVENT_RELOAD_TEXTURES,//!< Signals that all posible textures in objects should be reloaded.
    EVENT_RELOAD_GEOM,  //!< Signals that all posible geometries should be reloaded.
    EVENT_UNLOAD_GEOM,  //!< Signals that all posible geometries should be unloaded.
    EVENT_MISSION_CHANGE,   //!< Signals that mission have been changed.
    EVENT_CLEAR_AIGRAPH,        //!< Signals that ai graph is about to be deleted (used for clearing cached AiPoint GraphNodes).
    EVENT_ATTACH_AIPOINTS,  //! Signals to AiPoints that they should attach themselves to the Nav Graph.
    EVENT_PREFAB_REMAKE,//!< Recreate all objects in prefabs.
    EVENT_ALIGN_TOGRID, //!< Object should align itself to the grid.

    EVENT_PHYSICS_GETSTATE,//!< Signals that entities should accept thier phyical state from game.
    EVENT_PHYSICS_RESETSTATE,//!< Signals that physics state must be reseted on objects.
    EVENT_PHYSICS_APPLYSTATE,//!< Signals that teh stored physics state must be applied to objects.

    EVENT_PRE_EXPORT, //!< Signals that the game is about to be exported, prepare any data if the object needs to

    EVENT_FREE_GAME_DATA,//!< Object should free game data that its holding.
    EVENT_CONFIG_SPEC_CHANGE,   //!< Called when config spec changed.
    EVENT_HIDE_HELPER, //!< Signals that happens when Helper mode switches to be hidden.
};

#endif // CRYINCLUDE_EDITOR_INCLUDE_OBJECTEVENT_H

