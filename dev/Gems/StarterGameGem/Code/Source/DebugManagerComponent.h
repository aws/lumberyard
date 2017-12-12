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

#include <AzCore/Component/Component.h>
#include <AzCore/Component/EntityId.h>
#include <AzCore/Memory/SystemAllocator.h>

#include <AzCore/Math/Transform.h>
#include <AzCore/std/smart_ptr/shared_ptr.h>

#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>

namespace AZ
{
	class ReflectContext;
}

namespace StarterGameGem
{

	struct DebugVarBase
	{
		AZ_RTTI(DebugVarBase, "{6CEFB353-B765-4808-8F4F-01970D9C2365}");
		AZ_CLASS_ALLOCATOR(DebugVarBase, AZ::SystemAllocator, 0);

		AZStd::string m_eventName;
	};

	struct DebugVarBool
		: public DebugVarBase
	{
		AZ_RTTI(DebugVarBool, "{33B62F09-93E5-432E-9808-F1963E8BC6DE}", DebugVarBase);
		AZ_CLASS_ALLOCATOR(DebugVarBool, AZ::SystemAllocator, 0);

		bool m_value;
	};

	struct DebugVarFloat
		: public DebugVarBase
	{
		AZ_RTTI(DebugVarFloat, "{15C258BE-67B7-4076-A127-55EE69E21549}", DebugVarBase);
		AZ_CLASS_ALLOCATOR(DebugVarFloat, AZ::SystemAllocator, 0);

		float m_value;
	};

	/*!
	* DebugManagerComponentRequests
	* Messages serviced by the DebugManagerComponent
	*/
	class DebugManagerComponentRequests
		: public AZ::ComponentBus
	{
	public:
		virtual ~DebugManagerComponentRequests() {}

		//! Set the debug variables.
		virtual void SetDebugBool(const AZStd::string& eventName, bool value) = 0;
		virtual void SetDebugFloat(const AZStd::string& eventName, float value) = 0;

		//! Access the debug variables.
		virtual bool GetDebugBool(const AZStd::string& eventName) = 0;
		virtual float GetDebugFloat(const AZStd::string& eventName) = 0;

	};

	using DebugManagerComponentRequestsBus = AZ::EBus<DebugManagerComponentRequests>;


	class DebugManagerComponent
		: public AZ::Component
		, private DebugManagerComponentRequestsBus::Handler
	{
	public:
		AZ_COMPONENT(DebugManagerComponent, "{42FD7720-253C-44AB-9FAF-5F82B462A394}");

		//////////////////////////////////////////////////////////////////////////
		// AZ::Component interface implementation
		void Init() override;
		void Activate() override;
		void Deactivate() override;
		//////////////////////////////////////////////////////////////////////////

		// Required Reflect function.
		static void Reflect(AZ::ReflectContext* context);

		//////////////////////////////////////////////////////////////////////////
		// DebugManagerComponentRequestsBus::Handler
		//! Set the debug variables.
		virtual void SetDebugBool(const AZStd::string& eventName, bool value) override;
		virtual void SetDebugFloat(const AZStd::string& eventName, float value) override;

		//! Access the debug variables.
		virtual bool GetDebugBool(const AZStd::string& eventName) override;
		virtual float GetDebugFloat(const AZStd::string& eventName) override;

	protected:
		AZStd::vector<DebugVarBool> m_bools;
		AZStd::vector<DebugVarFloat> m_floats;

	};

} // namespace StarterGameGem
