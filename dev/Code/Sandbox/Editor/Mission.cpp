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

// Description : CMission class implementation.


#include "StdAfx.h"
#include "Mission.h"
#include "CryEditDoc.h"
#include "TerrainLighting.h"
#include "GameEngine.h"
#include "MissionScript.h"
#include "imoviesystem.h"
#include "VegetationMap.h"

#include "Objects/EntityObject.h"
#include "Objects/EntityScript.h"

#include <ITimeOfDay.h>
#include <IEntitySystem.h>
#include <I3DEngine.h>
#include "IAISystem.h"

namespace
{
    const char* kTimeOfDayFile = "TimeOfDay.xml";
    const char* kTimeOfDayRoot = "TimeOfDay";
    const char* kEnvironmentFile = "Environment.xml";
    const char* kEnvironmentRoot = "Environment";
};


//////////////////////////////////////////////////////////////////////////
CMission::CMission(CCryEditDoc* doc)
{
    m_doc = doc;
    m_objects = XmlHelpers::CreateXmlNode("Objects");
    m_layers = XmlHelpers::CreateXmlNode("ObjectLayers");
    //m_exportData = XmlNodeRef( "ExportData" );
    m_weaponsAmmo = XmlHelpers::CreateXmlNode("Ammo");
    m_timeOfDay = XmlHelpers::CreateXmlNode("TimeOfDay");
    m_environment = XmlHelpers::CreateXmlNode("Environment");
    CXmlTemplate::SetValues(m_doc->GetEnvironmentTemplate(), m_environment);

    m_time = 12; // 12 PM by default.

    m_lighting = new LightingSettings();            // default values are set in the constructor

    m_sMusicScript = "";

    m_pScript = new CMissionScript();
    m_numCGFObjects = 0;

    m_minimap.vCenter = Vec2(512, 512);
    m_minimap.vExtends = Vec2(512, 512);
    m_minimap.textureWidth = m_minimap.textureHeight = 1024;
}

//////////////////////////////////////////////////////////////////////////
CMission::~CMission()
{
    delete m_lighting;
    delete m_pScript;
}

//////////////////////////////////////////////////////////////////////////
CMission* CMission::Clone()
{
    CMission* m = new CMission(m_doc);
    m->SetName(m_name);
    m->SetDescription(m_description);
    m->m_objects = m_objects->clone();
    m->m_layers = m_layers->clone();
    m->m_environment = m_environment->clone();
    m->m_time = m_time;
    m->m_sMusicScript = m_sMusicScript;
    return m;
}

//////////////////////////////////////////////////////////////////////////
void CMission::SetMusicScriptFilename(const QString& sMusicScript)
{
    m_sMusicScript = sMusicScript;
    if (!m_sMusicScript.isEmpty())
    {
        //if (!GetIEditor()->GetSystem()->LoadMusicDataFromLUA(m_sMusicScript))
        //GetIEditor()->GetSystem()->GetILog()->Log("WARNING: Cannot load music-script %s !", m_sMusicScript);
    }
}

//////////////////////////////////////////////////////////////////////////
void CMission::Serialize(CXmlArchive& ar, bool bParts)
{
    if (ar.bLoading)
    {
        // Load.
        ar.root->getAttr("Name", m_name);
        ar.root->getAttr("Description", m_description);

        //time_t time = 0;
        //ar.root->getAttr( "Time",time );
        //m_time = time;

        m_sPlayerEquipPack = QString();
        ar.root->getAttr("PlayerEquipPack", m_sPlayerEquipPack);
        QString sMusicScript;
        ar.root->getAttr("MusicScript", m_sMusicScript);
        SetMusicScriptFilename(sMusicScript);
        QString sScript;
        ar.root->getAttr("Script", sScript);
        m_pScript->SetScriptFile(sScript);

        XmlNodeRef objects = ar.root->findChild("Objects");
        if (objects)
        {
            m_objects = objects;
        }

        XmlNodeRef layers = ar.root->findChild("ObjectLayers");
        if (layers)
        {
            m_layers = layers;
        }

        SerializeTimeOfDay(ar);

        m_Animations = ar.root->findChild("MovieData");

        /*
        XmlNodeRef expData = ar.root->findChild( "ExportData" );
        if (expData)
        {
            m_exportData = expData;
        }
        */
        SerializeEnvironment(ar);

        m_lighting->Serialize(ar.root, ar.bLoading);

        m_usedWeapons.clear();

        XmlNodeRef weapons = ar.root->findChild("Weapons");
        if (weapons)
        {
            XmlNodeRef usedWeapons = weapons->findChild("Used");
            if (usedWeapons)
            {
                QString weaponName;
                for (int i = 0; i < usedWeapons->getChildCount(); i++)
                {
                    XmlNodeRef weapon = usedWeapons->getChild(i);
                    if (weapon->getAttr("Name", weaponName))
                    {
                        m_usedWeapons.push_back(weaponName);
                    }
                }
            }
            XmlNodeRef ammo = weapons->findChild("Ammo");
            if (ammo)
            {
                m_weaponsAmmo = ammo->clone();
            }
        }

        XmlNodeRef minimapNode = ar.root->findChild("MiniMap");
        if (minimapNode)
        {
            minimapNode->getAttr("CenterX", m_minimap.vCenter.x);
            minimapNode->getAttr("CenterY", m_minimap.vCenter.y);
            minimapNode->getAttr("ExtendsX", m_minimap.vExtends.x);
            minimapNode->getAttr("ExtendsY", m_minimap.vExtends.y);
            //          minimapNode->getAttr( "CameraHeight",m_minimap.cameraHeight );
            minimapNode->getAttr("TexWidth", m_minimap.textureWidth);
            minimapNode->getAttr("TexHeight", m_minimap.textureHeight);
        }
    }
    else
    {
        ar.root->setAttr("Name", m_name.toUtf8().data());
        ar.root->setAttr("Description", m_description.toUtf8().data());

        //time_t time = m_time.GetTime();
        //ar.root->setAttr( "Time",time );

        ar.root->setAttr("PlayerEquipPack", m_sPlayerEquipPack.toUtf8().data());
        ar.root->setAttr("MusicScript", m_sMusicScript.toUtf8().data());
        ar.root->setAttr("Script", m_pScript->GetFilename().toUtf8().data());

        QString timeStr;
        int nHour = floor(m_time);
        int nMins = (m_time - floor(m_time)) * 60.0f;
        timeStr = QStringLiteral("%1:%2").arg(nHour, 2, 10, QLatin1Char('0')).arg(nMins, 2, 10, QLatin1Char('0'));
        ar.root->setAttr("MissionTime", timeStr.toUtf8().data());

        // Saving.
        XmlNodeRef layers = m_layers->clone();
        layers->setTag("ObjectLayers");
        ar.root->addChild(layers);

        ///XmlNodeRef objects = m_objects->clone();
        m_objects->setTag("Objects");
        ar.root->addChild(m_objects);

        if (bParts)
        {
            SerializeTimeOfDay(ar);
            SerializeEnvironment(ar);
        }

        m_lighting->Serialize(ar.root, ar.bLoading);

        XmlNodeRef weapons = ar.root->newChild("Weapons");
        XmlNodeRef usedWeapons = weapons->newChild("Used");
        for (int i = 0; i < m_usedWeapons.size(); i++)
        {
            XmlNodeRef weapon = usedWeapons->newChild("Weapon");
            weapon->setAttr("Name", m_usedWeapons[i].toUtf8().data());
        }
        weapons->addChild(m_weaponsAmmo->clone());

        XmlNodeRef minimapNode = ar.root->newChild("MiniMap");
        minimapNode->setAttr("CenterX", m_minimap.vCenter.x);
        minimapNode->setAttr("CenterY", m_minimap.vCenter.y);
        minimapNode->setAttr("ExtendsX", m_minimap.vExtends.x);
        minimapNode->setAttr("ExtendsY", m_minimap.vExtends.y);
        //      minimapNode->setAttr( "CameraHeight",m_minimap.cameraHeight );
        minimapNode->setAttr("TexWidth", m_minimap.textureWidth);
        minimapNode->setAttr("TexHeight", m_minimap.textureHeight);
    }
}

//////////////////////////////////////////////////////////////////////////
void CMission::Export(XmlNodeRef& root, XmlNodeRef& objectsNode)
{
    // Also save exported objects data.
    root->setAttr("Name", m_name.toUtf8().data());
    root->setAttr("Description", m_description.toUtf8().data());

    QString timeStr;
    int nHour = floor(m_time);
    int nMins = (m_time - floor(m_time)) * 60.0f;
    timeStr = QStringLiteral("%1:%2").arg(nHour, 2, 10, QLatin1Char('0')).arg(nMins, 2, 10, QLatin1Char('0'));
    root->setAttr("Time", timeStr.toUtf8().data());

    root->setAttr("PlayerEquipPack", m_sPlayerEquipPack.toUtf8().data());
    root->setAttr("MusicScript", m_sMusicScript.toUtf8().data());
    root->setAttr("Script", m_pScript->GetFilename().toUtf8().data());

    // Saving.
    //XmlNodeRef objects = m_exportData->clone();
    //objects->setTag( "Objects" );
    //root->addChild( objects );

    XmlNodeRef envNode = m_environment->clone();
    m_lighting->Serialize(envNode, false);
    root->addChild(envNode);

    m_timeOfDay->setAttr("Time", m_time);
    root->addChild(m_timeOfDay);

    XmlNodeRef minimapNode = root->newChild("MiniMap");
    minimapNode->setAttr("CenterX", m_minimap.vCenter.x);
    minimapNode->setAttr("CenterY", m_minimap.vCenter.y);
    minimapNode->setAttr("ExtendsX", m_minimap.vExtends.x);
    minimapNode->setAttr("ExtendsY", m_minimap.vExtends.y);
    //  minimapNode->setAttr( "CameraHeight",m_minimap.cameraHeight );
    minimapNode->setAttr("TexWidth", m_minimap.textureWidth);
    minimapNode->setAttr("TexHeight", m_minimap.textureHeight);

    XmlNodeRef weapons = root->newChild("Weapons");
    XmlNodeRef usedWeapons = weapons->newChild("Used");
    for (int i = 0; i < m_usedWeapons.size(); i++)
    {
        XmlNodeRef weapon = usedWeapons->newChild("Weapon");
        weapon->setAttr("Name", m_usedWeapons[i].toUtf8().data());
        IEntity* pEnt = GetIEditor()->GetSystem()->GetIEntitySystem()->FindEntityByName(m_usedWeapons[i].toUtf8().data());
        if (pEnt)
        {
            weapon->setAttr("id", pEnt->GetId());
        }
    }
    weapons->addChild(m_weaponsAmmo->clone());

    IObjectManager* pObjMan = GetIEditor()->GetObjectManager();

    //////////////////////////////////////////////////////////////////////////
    // Serialize objects.
    //////////////////////////////////////////////////////////////////////////
    QString path = QDir::toNativeSeparators(QFileInfo(m_doc->GetPathName()).absolutePath());
    if (!path.endsWith(QDir::separator()))
        path += QDir::separator();

    objectsNode = root->newChild("Objects");
    pObjMan->Export(path, objectsNode, true); // Export shared.
    pObjMan->Export(path, objectsNode, false);  // Export not shared.

    /*
    CObjectManager objectManager;
    XmlNodeRef loadRoot = root->createNode("Root");
    loadRoot->addChild( m_objects );

    std::vector<CObjectClassDesc*> classes;
    GetIEditor()->GetObjectManager()->GetClasses( classes );
    objectManager.SetClasses( classes );
    objectManager.SetCreateGameObject(false);
    objectManager.Serialize( loadRoot,true,SERIALIZE_ALL );
    objectManager.Export( path,objects,false );
    */
}

//////////////////////////////////////////////////////////////////////////
void CMission::ExportLegacyAnimations(XmlNodeRef& root)
{
    XmlNodeRef mission = XmlHelpers::CreateXmlNode("Mission");
    mission->setAttr("Name", m_name.toUtf8().data());

    XmlNodeRef movieDataNode = XmlHelpers::CreateXmlNode("MovieData");
    GetIEditor()->GetMovieSystem()->Serialize(movieDataNode, false);

    for (int i = 0; i < movieDataNode->getChildCount(); i++)
    {
        mission->addChild(movieDataNode->getChild(i));
    }
    root->addChild(mission);
}

//////////////////////////////////////////////////////////////////////////
void CMission::SyncContent(bool bRetrieve, bool bIgnoreObjects, bool bSkipLoadingAI /* = false */)
{
    // Save data from current Document to Mission.
    IObjectManager* objMan = GetIEditor()->GetObjectManager();
    if (bRetrieve)
    {
        // Activating this mission.
        CGameEngine* gameEngine = GetIEditor()->GetGameEngine();

        // Load mission script.
        if (m_pScript)
        {
            m_pScript->Load();
        }

        if (!bIgnoreObjects)
        {
            // Retrieve data from Mission and put to document.
            XmlNodeRef root = XmlHelpers::CreateXmlNode("Root");
            root->addChild(m_objects);
            root->addChild(m_layers);
            objMan->Serialize(root, true, SERIALIZE_ONLY_NOTSHARED);
        }

        m_doc->GetFogTemplate() = m_environment;

        CXmlTemplate::GetValues(m_doc->GetEnvironmentTemplate(), m_environment);

        UpdateUsedWeapons();
        gameEngine->SetPlayerEquipPack(m_sPlayerEquipPack.toUtf8().data());

        gameEngine->ReloadEnvironment();

        GetIEditor()->GetSystem()->GetI3DEngine()->CompleteObjectsGeometry();

        // refresh positions of vegetation objects since voxel mesh is defined only now
        if (CVegetationMap* pVegetationMap = GetIEditor()->GetVegetationMap())
        {
            pVegetationMap->OnHeightMapChanged();
        }

        if (m_Animations)
        {
            GetIEditor()->GetMovieSystem()->Serialize(m_Animations, true);
        }
        GetIEditor()->Notify(eNotify_OnReloadTrackView);

        if (!bSkipLoadingAI)
        {
            gameEngine->LoadAI(gameEngine->GetLevelPath(), GetName());
        }
        objMan->SendEvent(EVENT_MISSION_CHANGE);
        m_doc->ChangeMission();

        if (gEnv->pAISystem)
        {
            gEnv->pAISystem->OnMissionLoaded();
        }

        if (GetIEditor()->Get3DEngine())
        {
            m_numCGFObjects = GetIEditor()->Get3DEngine()->GetLoadedObjectCount();
        }

        // Load time of day.
        GetIEditor()->Get3DEngine()->GetTimeOfDay()->Serialize(m_timeOfDay, true);
    }
    else
    {
        // Save time of day.
        m_timeOfDay = XmlHelpers::CreateXmlNode("TimeOfDay");
        GetIEditor()->Get3DEngine()->GetTimeOfDay()->Serialize(m_timeOfDay, false);

        if (!bIgnoreObjects)
        {
            XmlNodeRef root = XmlHelpers::CreateXmlNode("Root");
            objMan->Serialize(root, false, SERIALIZE_ONLY_NOTSHARED);
            m_objects = root->findChild("Objects");
            XmlNodeRef layers = root->findChild("ObjectLayers");
            if (layers)
            {
                m_layers = layers;
            }
        }
    }
}

//////////////////////////////////////////////////////////////////////////
void CMission::OnEnvironmentChange()
{
    m_environment = XmlHelpers::CreateXmlNode("Environment");
    CXmlTemplate::SetValues(m_doc->GetEnvironmentTemplate(), m_environment);
}

//////////////////////////////////////////////////////////////////////////
void CMission::GetUsedWeapons(QStringList& weapons)
{
    weapons = m_usedWeapons;
}

//////////////////////////////////////////////////////////////////////////
void CMission::SetUsedWeapons(const QStringList& weapons)
{
    m_usedWeapons = weapons;
    UpdateUsedWeapons();
}

//////////////////////////////////////////////////////////////////////////
void CMission::UpdateUsedWeapons()
{
}

//////////////////////////////////////////////////////////////////////////
void CMission::ResetScript()
{
    if (m_pScript)
    {
        m_pScript->OnReset();
    }
}

//////////////////////////////////////////////////////////////////////////
void CMission::AddObjectsNode(XmlNodeRef& node)
{
    for (int i = 0; i < node->getChildCount(); i++)
    {
        m_objects->addChild(node->getChild(i)->clone());
    }
}

//////////////////////////////////////////////////////////////////////////
void CMission::SetLayersNode(XmlNodeRef& node)
{
    m_layers = node->clone();
}

//////////////////////////////////////////////////////////////////////////
void CMission::SetMinimap(const SMinimapInfo& minimap)
{
    m_minimap = minimap;
}


//////////////////////////////////////////////////////////////////////////
void CMission::SaveParts()
{
    // Save Time of Day
    {
        CTempFileHelper helper((GetIEditor()->GetLevelDataFolder() + kTimeOfDayFile).toUtf8().data());

        m_timeOfDay->saveToFile(helper.GetTempFilePath().toUtf8().data());

        if (!helper.UpdateFile(false))
        {
            return;
        }
    }


    // Save Environment
    {
        CTempFileHelper helper((GetIEditor()->GetLevelDataFolder() + kEnvironmentFile).toUtf8().data());

        XmlNodeRef root = m_environment->clone();
        root->setTag(kEnvironmentRoot);
        root->saveToFile(helper.GetTempFilePath().toUtf8().data());

        if (!helper.UpdateFile(false))
        {
            return;
        }
    }
}


//////////////////////////////////////////////////////////////////////////
void CMission::LoadParts()
{
    // Load Time of Day
    {
        QString filename = GetIEditor()->GetLevelDataFolder() + kTimeOfDayFile;
        XmlNodeRef root = XmlHelpers::LoadXmlFromFile(filename.toUtf8().data());
        if (root && !_stricmp(root->getTag(), kTimeOfDayRoot))
        {
            m_timeOfDay = root;
            m_timeOfDay->getAttr("Time", m_time);
        }
    }

    // Load Environment
    {
        QString filename = GetIEditor()->GetLevelDataFolder() + kEnvironmentFile;
        XmlNodeRef root = XmlHelpers::LoadXmlFromFile(filename.toUtf8().data());
        if (root && !_stricmp(root->getTag(), kEnvironmentRoot))
        {
            m_environment = root;
        }
    }
}

//////////////////////////////////////////////////////////////////////////
void CMission::SerializeTimeOfDay(CXmlArchive& ar)
{
    if (ar.bLoading)
    {
        XmlNodeRef todNode = ar.root->findChild("TimeOfDay");
        if (todNode)
        {
            m_timeOfDay = todNode;
            todNode->getAttr("Time", m_time);
        }
        else
        {
            m_timeOfDay = XmlHelpers::CreateXmlNode("TimeOfDay");
        }
    }
    else
    {
        m_timeOfDay->setAttr("Time", m_time);
        ar.root->addChild(m_timeOfDay);
    }
}

//////////////////////////////////////////////////////////////////////////
void CMission::SerializeEnvironment(CXmlArchive& ar)
{
    if (ar.bLoading)
    {
        XmlNodeRef env = ar.root->findChild("Environment");
        if (env)
        {
            m_environment = env;
        }
    }
    else
    {
        XmlNodeRef env = m_environment->clone();
        env->setTag("Environment");
        ar.root->addChild(env);
    }
}

//////////////////////////////////////////////////////////////////////////
const SMinimapInfo& CMission::GetMinimap() const
{
    return m_minimap;
}

