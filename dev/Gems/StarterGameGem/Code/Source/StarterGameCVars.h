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


#pragma once


namespace StarterGameGem
{

	class StarterGameCVars
	{
	public:
		StarterGameCVars();

		static const StarterGameCVars& GetInstance()
		{
			static StarterGameCVars instance;
			return instance;
		}

		int m_viewAISightRange;
		int m_viewAISuspicionRange;
        int m_viewAIStates;

		float m_viewWaypoints;
		int m_viewPaths;

        float m_viewSoundRanges;

	private:
		// Only the gem component should unregister the CVars (to ensure it's only done once).
		friend class StarterGameGemModule;
		static void DeregisterCVars();

	};

} // namespace StarterGameGem
