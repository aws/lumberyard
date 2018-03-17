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
#include "StarterGameGem_precompiled.h"
#include "Config.h"

#include <AzCore/RTTI/BehaviorContext.h>

namespace StarterGameGem
{

	enum class BuildConfig : int
	{
		Unknown = 0,
		Debug = 1,
		Profile,
		Performance,
		Release,
	};

	BuildConfig Build()
	{
#ifdef _DEBUG
		return BuildConfig::Debug;
#elif _PROFILE
		return BuildConfig::Profile;
#elif PERFORMANCE_BUILD
		return BuildConfig::Performance;
#elif _RELEASE
		return BuildConfig::Release;
#else
		return BuildConfig::Unknown;
#endif
	}

	void Config::Reflect(AZ::ReflectContext* reflection)
	{
		if (AZ::BehaviorContext* behaviorContext = azrtti_cast<AZ::BehaviorContext*>(reflection))
		{
			behaviorContext->Class<Config>("Config")
				->Property("Build",         &Build, nullptr)
				->Property("Unknown",       BehaviorConstant(BuildConfig::Unknown), nullptr)
				->Property("Debug",         BehaviorConstant(BuildConfig::Debug), nullptr)
				->Property("Profile",       BehaviorConstant(BuildConfig::Profile), nullptr)
				->Property("Performance",   BehaviorConstant(BuildConfig::Performance), nullptr)
				->Property("Release",       BehaviorConstant(BuildConfig::Release), nullptr)
			;
		}
	}

}
