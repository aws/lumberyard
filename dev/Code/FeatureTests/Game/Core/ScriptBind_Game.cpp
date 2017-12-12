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
#include "StdAfx.h"
#include "ScriptBind_Game.h"
#include "GameCache.h"
#include <IGameFramework.h>
#include <IScriptSystem.h>
#include "FeatureTestsGame.h"

using namespace LYGame;

//////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////

namespace LYGame
{
    namespace
    {
#define LOG_CACHE_RESOURCES_FROM_LUA 0

        void LogLuaCacheResource(const char* whoIsRequesting, const char* type, const char* resourceName, const int flags)
        {
#if LOG_CACHE_RESOURCES_FROM_LUA
            if (resourceName && resourceName[0])
            {
                CryLog("[GAME CACHE LUA] by '%s' : %s - %s Flags(%d)", whoIsRequesting, type, resourceName, flags);
            }
#endif
        }
    } // namespace
} // namespace LYGame

//////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////

ScriptBind_Game::ScriptBind_Game(IGameFramework* gameFramework)
{
    ISystem* system = gameFramework->GetISystem();
    IScriptSystem* scriptSystem = system->GetIScriptSystem();

    Init(scriptSystem, system);
    SetGlobalName("Game");
    RegisterMethods();
    RegisterGlobals();
}

void ScriptBind_Game::RegisterGlobals()
{
    m_pSS->SetGlobalValue("eGameCacheResourceType_Texture", CacheTypes::eResource_Texture);
    m_pSS->SetGlobalValue("eGameCacheResourceType_TextureDeferredCubemap", CacheTypes::eResource_TextureDeferredCubemap);
    m_pSS->SetGlobalValue("eGameCacheResourceType_StaticObject", CacheTypes::eResource_StaticObject);
    m_pSS->SetGlobalValue("eGameCacheResourceType_Material", CacheTypes::eResource_Material);
    m_pSS->SetGlobalValue("eGameCacheResourceFlag_TextureNoStream", FT_DONT_STREAM);
    m_pSS->SetGlobalValue("eGameCacheResourceFlag_TextureReplicateAllSides", FT_REPLICATE_TO_ALL_SIDES);
}

void ScriptBind_Game::RegisterMethods()
{
#undef SCRIPT_REG_CLASSNAME
    #define SCRIPT_REG_CLASSNAME &ScriptBind_Game::
    SCRIPT_REG_TEMPLFUNC(CacheResource, "whoIsRequesting, resourceName, resourceType, resourceFlags");
#undef SCRIPT_REG_CLASSNAME
}

int ScriptBind_Game::CacheResource(IFunctionHandler* functionHandler, const char* whoIsRequesting, const char* resourceName, int resourceType, int resourceFlags)
{
    // Only cache in pure game mode.
    if (gEnv->IsEditor())
    {
        return functionHandler->EndFunction();
    }

    GameCache& gameCache = g_Game->GetGameCache();

    switch (resourceType)
    {
    case CacheTypes::eResource_Texture:
    {
        gameCache.CacheTexture(resourceName, resourceFlags);
        LogLuaCacheResource(whoIsRequesting, "Texture", resourceName, resourceFlags);
    }
    break;

    case CacheTypes::eResource_TextureDeferredCubemap:
    {
        // Some magic strings ops were copy and pasted from ScriptBind_Entity::ParseLightProperties.
        const char* specularCubemap = resourceName;

        if (specularCubemap && (strlen(specularCubemap) > 0))
        {
            typedef CryFixedStringT<256> FixedString;
            FixedString specularName(specularCubemap);
            const size_t strIndex = specularName.find("_diff");

            if (strIndex != FixedString::npos)
            {
                specularName = specularName.substr(0, strIndex) + specularName.substr(strIndex + 5, specularName.length());
                specularCubemap = specularName.c_str();
            }

            FixedString diffuseCubemap;
            diffuseCubemap.Format("%s%s%s.%s",  PathUtil::AddSlash(PathUtil::GetPath(specularCubemap).c_str()).c_str(),
                PathUtil::GetFileName(specularCubemap).c_str(), "_diff", PathUtil::GetExt(specularCubemap));

            // '\\' in filename causing texture duplication.
            string specularCubemapUnix = PathUtil::ToUnixPath(specularCubemap);
            string diffuseCubemapUnix = PathUtil::ToUnixPath(diffuseCubemap.c_str());

            gameCache.CacheTexture(specularCubemapUnix.c_str(), resourceFlags);
            gameCache.CacheTexture(diffuseCubemapUnix.c_str(), resourceFlags);

            LogLuaCacheResource(whoIsRequesting, "CubeMap Specular", specularCubemapUnix.c_str(), resourceFlags);
            LogLuaCacheResource(whoIsRequesting, "CubeMap Diffuse", diffuseCubemapUnix.c_str(), resourceFlags);
        }
    }
    break;

    case CacheTypes::eResource_StaticObject:
    {
        gameCache.CacheGeometry(resourceName);
        LogLuaCacheResource(whoIsRequesting, "Static Object", resourceName, resourceFlags);
    }
    break;

    case CacheTypes::eResource_Material:
    {
        gameCache.CacheMaterial(resourceName);
        LogLuaCacheResource(whoIsRequesting, "Material", resourceName, resourceFlags);
    }
    break;
    }

    return functionHandler->EndFunction();
}