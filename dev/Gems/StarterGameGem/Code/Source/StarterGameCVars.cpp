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
#include "StarterGameCVars.h"

namespace StarterGameGem
{

	StarterGameCVars::StarterGameCVars()
	{
		REGISTER_CVAR2("ai_debugDrawSightRange", &m_viewAISightRange, 0, VF_NULL, "Enable drawing of A.I. sight and aggro ranges.");
        REGISTER_CVAR2("ai_debugDrawSuspicionRange", &m_viewAISuspicionRange, 0, VF_NULL, "Enable drawing of A.I. suspicion ranges.");
        REGISTER_CVAR2("ai_debugDrawAIStates", &m_viewAIStates, 0, VF_NULL, "Enable visualisations of A.I. states.");

		REGISTER_CVAR2("ai_debugDrawWaypoints", &m_viewWaypoints, 0, VF_NULL, "Enable drawing of A.I. waypoints.");
		REGISTER_CVAR2("ai_debugDrawPaths", &m_viewPaths, 0, VF_NULL, "Enable drawing of A.I. paths.");

        REGISTER_CVAR2("s_visualiseSoundRanges", &m_viewSoundRanges, 0, VF_NULL, "Enable drawing of sounds.\n"
            "Usage: value is the duration of the visualisations.\n"
            "Default is 0.0 (off).");
	}

	void StarterGameCVars::DeregisterCVars()
	{
		UNREGISTER_CVAR("ai_debugDrawSightRange");
        UNREGISTER_CVAR("ai_debugDrawSuspicionRange");
        UNREGISTER_CVAR("ai_debugDrawAIStates");
		
		UNREGISTER_CVAR("ai_debugDrawWaypoints");
		UNREGISTER_CVAR("ai_debugDrawPaths");

        UNREGISTER_CVAR("s_visualiseSoundRanges");
	}

}
