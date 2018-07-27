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

#include "StdAfx.h"
#include "GenerateSpawners.h"
#include "IEntitySystem.h"
#include "IScriptSystem.h"

namespace
{
    QString Tabs(int n)
    {
        QString tabs;
        for (int i = 0; i < n; i++)
        {
            tabs += '\t';
        }
        return tabs;
    }
    inline const char* to_str(float val)
    {
        static char temp[32];
        sprintf_s(temp, "%g", val);
        return temp;
    }
}

static bool ShouldGenerateSpawner(IEntityClass* pClass)
{
    const QString startPath = "Scripts/Entities/AI/";
    // things that we're not smart enough to skip
    const QString exceptionClasses[] = {
        "AIAlertness",
        "AIAnchor",
        "SmartObjectCondition"
    };

    QString filename = pClass->GetScriptFile();
    QString classname = pClass->GetName();

    // check the path name is right
    if (filename.isEmpty())
    {
        return false;
    }
    if (filename.length() < startPath.length())
    {
        return false;
    }
    if (filename.left(startPath.length()) != startPath)
    {
        return false;
    }

    for (int i = 0; i < sizeof(exceptionClasses) / sizeof(*exceptionClasses); i++)
    {
        if (classname == exceptionClasses[i])
        {
            return false;
        }
    }

    // check the table has properties (a good filter)
    SmartScriptTable pTable;
    gEnv->pScriptSystem->GetGlobalValue(pClass->GetName(), pTable);
    if (!pTable)
    {
        return false;
    }
    if (!pTable->HaveValue("Properties"))
    {
        return false;
    }

    return true;
}

static bool OutputFile(const QString& name, const QString& data)
{
    FILE* f = nullptr;
    azfopen(&f, name.toUtf8().data(), "wt");
    if (!f)
    {
        CryLogAlways("Unable to open file %s", name.toUtf8().data());
        return false;
    }
    fwrite(data.toUtf8().data(), data.toUtf8().length(), 1, f);
    fclose(f);
    return true;
}

static bool GenerateEntityForSpawner(IEntityClass* pClass)
{
    QString os;
    os += QString("<Entity Name=\"Spawn") + pClass->GetName() + "\" Script=\"Scripts/Entities/AISpawners/Spawn" + pClass->GetName() + ".lua\"/>\n";
    return OutputFile((Path::GetEditingGameDataFolder() + "\\Entities\\Spawn" + pClass->GetName() + ".ent").c_str(), os);
}

static void CloneTable(QString& os, SmartScriptTable from, const char* table, int tabs)
{
    SmartScriptTable tbl;
    from->GetValue(table, tbl);
    if (!tbl)
    {
        return;
    }

    os += Tabs(tabs) + table + " =\n"
        + Tabs(tabs) + "{\n";
    IScriptTable::Iterator iter = tbl->BeginIteration();
    while (tbl->MoveNext(iter))
    {
        if (!iter.sKey)
        {
            continue;
        }
        switch (iter.value.type)
        {
        case ANY_TSTRING:
            os += Tabs(tabs + 1) + iter.sKey + " = \"" + iter.value.str + "\",\n";
            break;
        case ANY_TNUMBER:
            os += Tabs(tabs + 1) + iter.sKey + " = " + to_str(iter.value.number) + ",\n";
            break;
        case ANY_TTABLE:
            CloneTable(os, tbl, iter.sKey, tabs + 1);
            break;
        }
    }
    tbl->EndIteration(iter);
    os += Tabs(tabs) + "},\n";
}

static bool GenerateLuaForSpawner(IEntityClass* pClass)
{
    QString os;

    SmartScriptTable entityTable;
    gEnv->pScriptSystem->GetGlobalValue(pClass->GetName(), entityTable);

    os += "-- AUTOMATICALLY GENERATED CODE\n";
    os += "-- use sandbox (AI/Generate Spawner Scripts) to regenerate this file\n";
    os += QString("Script.ReloadScript(\"") + pClass->GetScriptFile() + "\")\n";

    os += QString("Spawn") + pClass->GetName() + " =\n";
    os += "{\n";

    os += "\tspawnedEntity = nil,\n";
    CloneTable(os, entityTable, "Properties", 1);
    CloneTable(os, entityTable, "PropertiesInstance", 1);

    os += "}\n";

    os += QString("Spawn") + pClass->GetName() + ".Properties.SpawnedEntityName = \"\"\n";

    // get event information
    std::set<QString> inputEvents, outputEvents;
    std::map<QString, QString> eventTypes;
    SmartScriptTable eventsTable;
    entityTable->GetValue("FlowEvents", eventsTable);
    if (!!eventsTable)
    {
        SmartScriptTable inputs, outputs;
        eventsTable->GetValue("Inputs", inputs);
        eventsTable->GetValue("Outputs", outputs);

        if (!!inputs)
        {
            IScriptTable::Iterator iter = inputs->BeginIteration();
            while (inputs->MoveNext(iter))
            {
                if (!iter.sKey || iter.value.type != ANY_TTABLE)
                {
                    continue;
                }
                const char* type;
                if (iter.value.table->GetAt(2, type))
                {
                    eventTypes[iter.sKey] = type;
                    inputEvents.insert(iter.sKey);
                }
            }
            inputs->EndIteration(iter);
        }
        if (!!outputs)
        {
            IScriptTable::Iterator iter = outputs->BeginIteration();
            while (outputs->MoveNext(iter))
            {
                if (!iter.sKey || iter.value.type != ANY_TSTRING)
                {
                    continue;
                }
                eventTypes[iter.sKey] = iter.value.str;
                outputEvents.insert(iter.sKey);
            }
            outputs->EndIteration(iter);
        }
    }

    // boiler plate (almost) code
    os += QString("function Spawn") + pClass->GetName() + ":Event_Spawn(sender,params)\n"
        + "\tlocal params = {\n"
        + "\t\tclass = \"" + pClass->GetName() + "\",\n"
        + "\t\tposition = self:GetPos(),\n"
        + "\t\torientation = self:GetDirectionVector(1),\n"
        + "\t\tscale = self:GetScale(),\n"
        + "\t\tproperties = self.Properties,\n"
        + "\t\tpropertiesInstance = self.PropertiesInstance,\n"
        + "\t}\n"
        + "\tif self.Properties.SpawnedEntityName ~= \"\" then\n"
        + "\t\tparams.name = self.Properties.SpawnedEntityName\n"
        + "\telse\n"
        + "\t\tparams.name = self:GetName()\n"
        + "\tend\n"
        + "\tlocal ent = System.SpawnEntity(params)\n"
        + "\tif ent then\n"
        + "\t\tself.spawnedEntity = ent.id\n"
        + "\t\tif not ent.Events then ent.Events = {} end\n"
        + "\t\tlocal evts = ent.Events\n";
    for (std::set<QString>::const_iterator iter = outputEvents.begin(); iter != outputEvents.end(); ++iter)
    {
        // setup event munging...
        os += QString("\t\tif not evts.") + *iter + " then evts." + *iter + " = {} end\n"
            + "\t\ttable.insert(evts." + *iter + ", {self.id, \"" + *iter + "\"})\n";
    }
    os += QString("\tend\n")
        + "\tBroadcastEvent(self, \"Spawned\")\n"
        + "end\n"
        + "function Spawn" + pClass->GetName() + ":OnReset()\n"
        + "\tif self.spawnedEntity then\n"
        + "\t\tSystem.RemoveEntity(self.spawnedEntity)\n"
        + "\t\tself.spawnedEntity = nil\n"
        + "\tend\n"
        + "end\n"
        + "function Spawn" + pClass->GetName() + ":GetFlowgraphForwardingEntity()\n"
        + "\treturn self.spawnedEntity\n"
        + "end\n"
        + "function Spawn" + pClass->GetName() + ":Event_Spawned()\n"
        + "\tBroadcastEvent(self, \"Spawned\")\n"
        + "end\n";

    // output the event information
    for (std::map<QString, QString>::const_iterator iter = eventTypes.begin(); iter != eventTypes.end(); ++iter)
    {
        os += QString("function Spawn") + pClass->GetName() + ":Event_" + iter->first + "(sender, param)\n";
        if (outputEvents.find(iter->first) != outputEvents.end())
        {
            os += QString("\tif sender and sender.id == self.spawnedEntity then BroadcastEvent(self, \"") + iter->first + "\") end\n";
        }
        if (inputEvents.find(iter->first) != inputEvents.end())
        {
            os += QString("\tif self.spawnedEntity and ((not sender) or (self.spawnedEntity ~= sender.id)) then\n")
                + "\t\tlocal ent = System.GetEntity(self.spawnedEntity)\n"
                + "\t\tif ent and ent ~= sender then\n"
                + "\t\t\tself.Handle_" + iter->first + "(ent, sender, param)\n"
                + "\t\tend\n"
                + "\tend\n";
        }
        if (iter->first == "OnDeath" || iter->first == "Die")
        {
            os += "\tif sender and sender.id == self.spawnedEntity then self.spawnedEntity = nil end\n";
        }
        os += "end\n";
    }
    os += QString("Spawn") + pClass->GetName() + ".FlowEvents =\n"
        + "{\n"
        + "\tInputs = \n"
        + "\t{\n"
        + "\t\tSpawn = { Spawn" + pClass->GetName() + ".Event_Spawn, \"bool\" },\n";
    for (std::set<QString>::const_iterator iter = inputEvents.begin(); iter != inputEvents.end(); ++iter)
    {
        os += QString("\t\t") + *iter + " = { Spawn" + pClass->GetName() + ".Event_" + *iter + ", \"" + eventTypes[*iter] + "\" },\n";
    }
    os += QString("\t},\n")
        + "\tOutputs = \n"
        + "\t{\n"
        + "\t\tSpawned = \"bool\",\n";
    for (std::set<QString>::const_iterator iter = outputEvents.begin(); iter != outputEvents.end(); ++iter)
    {
        os += QString("\t\t") + *iter + " = \"" + eventTypes[*iter] + "\",\n";
    }
    os += QString("\t}\n")
        + "}\n";

    for (std::set<QString>::const_iterator iter = inputEvents.begin(); iter != inputEvents.end(); ++iter)
    {
        os += QString("Spawn") + pClass->GetName() + ".Handle_" + *iter + " = " + pClass->GetName() + ".FlowEvents.Inputs." + *iter + "[1]\n";
    }

    return OutputFile((Path::GetEditingGameDataFolder() + "\\Scripts\\Entities\\AISpawners\\Spawn" + pClass->GetName() + ".lua").c_str(), os);
}

static bool GenerateSpawner(IEntityClass* pClass)
{
    GenerateEntityForSpawner(pClass);
    return GenerateLuaForSpawner(pClass);
}

void GenerateSpawners()
{
    if (!gEnv->pEntitySystem)
    {
        return;
    }

    // iterate over all entity classes, and try to find those that are spawning AI's
    IEntityClass* pClass;
    IEntityClassRegistry* pReg = gEnv->pEntitySystem->GetClassRegistry();
    for (pReg->IteratorMoveFirst(); pClass = pReg->IteratorNext(); )
    {
        if (ShouldGenerateSpawner(pClass))
        {
            if (!GenerateSpawner(pClass))
            {
                CryLogAlways("Couldn't generate spawner for %s", pClass->GetName());
            }
        }
    }
}
