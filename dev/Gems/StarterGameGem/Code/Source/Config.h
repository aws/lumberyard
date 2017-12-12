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

#include <AzCore/base.h>
#include <AzCore/Memory/SystemAllocator.h>

namespace AZ
{
	class ReflectContext;
}

namespace StarterGameGem
{
	/*!
	* Reflects globals for getting the current build config in Lua.
	*/
	class Config
	{
	public:
		AZ_TYPE_INFO(Config, "{513BEE61-8C95-4D67-9113-56A177435DC2}");
		AZ_CLASS_ALLOCATOR(Config, AZ::SystemAllocator, 0);

		Config() = default;
		~Config() = default;

		static void Reflect(AZ::ReflectContext* reflection);

	};

} // namespace StarterGameGem
