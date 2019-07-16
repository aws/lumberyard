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

#include "Precompiled.h"

#include "TimeOfDay.h"
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/RTTI/BehaviorContext.h>
#include <ISystem.h>
#include <I3DEngine.h>
#include <ITimeOfDay.h>
#include <ILevelSystem.h>
#include <IGame.h>
#include <IGameFramework.h>

namespace GraphicsReflectContext
{
    void TimeOfDay::SetTime(float time, bool forceUpdate)
    {
        ITimeOfDay* tod = gEnv->p3DEngine->GetTimeOfDay();
        if (tod)
        {
            tod->SetTime(time, forceUpdate);
        }
    }
    
    
    float TimeOfDay::GetTime()
    {
        ITimeOfDay* tod = gEnv->p3DEngine->GetTimeOfDay();
        if (tod)
        {
            return tod->GetTime();
        }
        return 0;
    }
    
    
    void TimeOfDay::SetSpeed(float speed)
    {
        ITimeOfDay* tod = gEnv->p3DEngine->GetTimeOfDay();
        if (tod)
        {
            ITimeOfDay::SAdvancedInfo info;
            tod->GetAdvancedInfo(info);
            info.fAnimSpeed = speed;
            tod->SetAdvancedInfo(info);
        }
    }
    
    
    float TimeOfDay::GetSpeed()
    {
        ITimeOfDay* tod = gEnv->p3DEngine->GetTimeOfDay();
        if (tod)
        {
            ITimeOfDay::SAdvancedInfo info;
            tod->GetAdvancedInfo(info);
            return info.fAnimSpeed;
        }
        return 0;
    }

    bool TimeOfDay::LoadDefinitionFile(AZStd::string_view fileName)
    {
        // get the file name
        AZStd::string pathAndfileName;
        if (fileName.empty())
        {
            AZ_Warning("Editor", false, "TimeOfDayLoadDefinitionFileNode: tod file name is empty.");
            return false;
        }
        ILevel* currentLevel = gEnv->pSystem->GetILevelSystem()->GetCurrentLevel();
        AZStd::string path = currentLevel ? currentLevel->GetLevelInfo()->GetPath() : "";
        pathAndfileName = AZStd::string::format("%s/%s", path.c_str(), fileName.data());

        // try to load it
        XmlNodeRef root = GetISystem()->LoadXmlFromFile(pathAndfileName.c_str());
        if (root == nullptr)
        {
            AZ_Warning("Editor", false, "TimeOfDayLoadDefinitionFileNode: Could not load tod file %s.", pathAndfileName.c_str());
            return false;
        }

        // get the TimeofDay interface
        ITimeOfDay* pTimeOfDay = gEnv->p3DEngine->GetTimeOfDay();
        if (pTimeOfDay == nullptr)
        {
            AZ_Warning("Editor", false, "TimeOfDayLoadDefinitionFileNode: Could not obtain ITimeOfDay interface from engine.");
            return false;
        }

        // try to serialize from that file
        pTimeOfDay->Serialize(root, true);
        pTimeOfDay->Update(true, true);

        return true;
    }

    void TimeOfDay::Reflect(AZ::ReflectContext* context)
    {
        if (AZ::BehaviorContext* behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
        {
            behaviorContext->Class<TimeOfDay>()
                ->Attribute(AZ::Script::Attributes::Category, "Rendering")
                ->Method("SetTime",
                    &TimeOfDay::SetTime,
                    { {
                        { "Time",           "Current time [0-24] (in hours)",                             behaviorContext->MakeDefaultValue(0.0f)  },
                        { "ForceUpdate",    "Indicates whether the whole sky should be updated immediately",        behaviorContext->MakeDefaultValue(false) }
                    } })
                ->Attribute(AZ::Script::Attributes::ToolTip, "Sets the current time of day")

                ->Method("GetTime", &TimeOfDay::GetTime,  { { } })
                ->Attribute(AZ::Script::Attributes::ToolTip, "Returns the current time of day")

                ->Method("SetSpeed",
                    &TimeOfDay::SetSpeed,
                    { {
                        { "Speed",          "Current speed",                                             behaviorContext->MakeDefaultValue(0.0f) }
                    } })
                ->Attribute(AZ::Script::Attributes::ToolTip, "Sets the current time of day speed multiplier")

                ->Method("GetSpeed", &TimeOfDay::GetSpeed, { {} })
                ->Attribute(AZ::Script::Attributes::ToolTip, "Returns the current time of day speed multiplier")

                ->Method("LoadDefinitionFile",
                    &TimeOfDay::LoadDefinitionFile,
                    { {
                        { "FileName",         "Name of the xml file to be read (must be in level folder)", behaviorContext->MakeDefaultValue(AZStd::string_view()) }
                        } })
                ->Attribute(AZ::Script::Attributes::ToolTip, "Load a TimeOfDay xml file and update the sky accordingly")
                ;
        }
    }
}

