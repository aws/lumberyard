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

#include <AzCore/Component/Component.h>
#include "Maestro/MaestroBus.h"
#include <ITimeOfDay.h>

namespace Maestro
{
    class TimeOfDayCacheSystemComponent
        : public AZ::Component
        , TimeOfDayCacheRequestBus::Handler
    {
    public:

        AZ_COMPONENT(TimeOfDayCacheSystemComponent, "{72112525-DBBE-48A3-9321-4CAE2D65D7FF}");

        static void Reflect(AZ::ReflectContext* reflection)
        {
            if (AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(reflection))
            {
                serialize->Class<TimeOfDayCacheSystemComponent, AZ::Component>()
                    ->Version(0);
            }
        }

        void Activate() override
        {
            TimeOfDayCacheRequestBus::Handler::BusConnect();
        }

        void Deactivate() override
        {
            TimeOfDayCacheRequestBus::Handler::BusDisconnect();
        }

        XmlNodeRef Get(const char* path) override
        {
            if (m_timeOfDayXMLData == nullptr || m_missionName.compare(path) != 0)
            {
                AZ_PROFILE_FUNCTION(AZ::Debug::ProfileCategory::Movie);
                m_missionName = path;
                XmlNodeRef root = gEnv->pSystem->LoadXmlFromFile(gEnv->p3DEngine->GetLevelFilePath(path));
                if (root)
                {
                    m_timeOfDayXMLData = root->findChild("TimeOfDay");
                }

                // We just loaded the xml data, so deserialize it now, and cache the values so we dont need to deserialize again.
                {

                    ITimeOfDay* timeOfDay = gEnv->p3DEngine->GetTimeOfDay();
                    timeOfDay->Serialize(m_timeOfDayXMLData, true);

                    AZ_PROFILE_SCOPE(AZ::Debug::ProfileCategory::Movie, "TimeOfDayCacheSystemComponent::Get:CacheDefaults");
                    timeOfDay->MakeCurrentPresetFromCurrentParams();
                    timeOfDay->GetAdvancedInfo(m_timeOfDayAdvancedInfoDefaults);
                    m_timeOfDayTimeDefault = timeOfDay->GetTime();
                }
            }

            return m_timeOfDayXMLData;
        }

        void ResetTimeOfDayParams() override
        {
            ITimeOfDay* timeOfDay = gEnv->p3DEngine->GetTimeOfDay();
            timeOfDay->SetAdvancedInfo(m_timeOfDayAdvancedInfoDefaults);
            timeOfDay->SetTime(m_timeOfDayTimeDefault);
        }

    private:

        AZStd::string m_missionName;
        XmlNodeRef m_timeOfDayXMLData;
        ITimeOfDay::SAdvancedInfo m_timeOfDayAdvancedInfoDefaults;
        float m_timeOfDayTimeDefault;
    };

}