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

// Description : Common definitions for Entity Pool


#ifndef CRYINCLUDE_CRYENTITYSYSTEM_ENTITYPOOLCOMMON_H
#define CRYINCLUDE_CRYENTITYSYSTEM_ENTITYPOOLCOMMON_H
#pragma once


// Include debugging info about pools in build.
#ifndef _RELEASE
    #define ENTITY_POOL_DEBUGGING
#endif //_RELEASE

// Perform signature tests on Entity classes included in Pool definitions.
// This is a saftey check to make sure Pool definitions are properly defined.
#if defined(ENTITY_POOL_DEBUGGING) && (defined(WIN32) || defined(WIN64))
    #define ENTITY_POOL_SIGNATURE_TESTING
#endif //_RELEASE

typedef uint32 TEntityPoolId;
static const TEntityPoolId INVALID_ENTITY_POOL = ~0;

typedef uint32 TEntityPoolDefinitionId;
static const TEntityPoolDefinitionId INVALID_ENTITY_POOL_DEFINITION = ~0;

class CEntityPool;
class CEntityPoolDefinition;
class CEntityPoolManager;

#endif // CRYINCLUDE_CRYENTITYSYSTEM_ENTITYPOOLCOMMON_H
