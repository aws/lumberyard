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
	//////////////////////////////////////////////////////////
	// The purpose of this component is to allow users in the editor to create a list of something:
	// whether that's a list of entities, strings, integers, etc.
	// That list can then be referenced in Lua.
	//
	// This system is partially a work-around for the fact that Lua doesn't yet accept aggregate
	// results from C++ functions, but also so that a list can be re-used. For example, this could
	// be placed on an independent entity and then multiple entities could reference its list.
	//
	// To create a new object type you can ultimately copy+paste an existing one (ObjectString, for
	// example) and change the name and type the vector holds. Also be sure to change the GUID of
	// the new struct.
	// After creating the new struct:
	// - change the container type (the type held by the vector)
	// - add that type to the 'ObjectFromList' struct (and expose it in 'ObjectListerComponent::Reflect()'
	// - add the 'Reflect()' call to 'ObjectListerComponent::Reflect()'
	// - add a static_cast to your type inside 'ObjectListerComponent::GenerateReturnFromVoidPtr()'
	//////////////////////////////////////////////////////////


	// The base member variable type for the ObjectListerComponent which allows the user to then
	// select one of the inherited types.
	struct ObjectBase
	{
		AZ_RTTI(ObjectBase, "{3255D471-6B8D-4688-968F-FDA35A921C3D}");

		virtual ~ObjectBase() = default;

		virtual AZ::u32 GetListSize() = 0;
		virtual void* GetItemAt(AZ::u32 index) = 0;

		static void Reflect(AZ::ReflectContext* context);
	};

	// An object lister type that the user can select to make a list of entities.
	struct ObjectEntity
		: public ObjectBase
	{
		AZ_RTTI(ObjectEntity, "{4A00E05E-83A7-468E-BC41-A3648B21F3C2}", ObjectBase);
		AZ_CLASS_ALLOCATOR(ObjectEntity, AZ::SystemAllocator, 0);

		~ObjectEntity() override = default;

		AZ::u32 GetListSize() override;
		void* GetItemAt(AZ::u32 index) override;

		static void Reflect(AZ::ReflectContext* context);

		AZStd::vector<AZ::EntityId> m_entities;
	};

	// An object lister type that the user can select to make a list of strings.
	struct ObjectString
		: public ObjectBase
	{
		AZ_RTTI(ObjectString, "{7BFB0E6C-5FB4-4003-96A3-B966A85BF35F}", ObjectBase);
		AZ_CLASS_ALLOCATOR(ObjectString, AZ::SystemAllocator, 0);

		~ObjectString() override = default;

		AZ::u32 GetListSize() override;
		void* GetItemAt(AZ::u32 index) override;

		static void Reflect(AZ::ReflectContext* context);

		AZStd::vector<AZStd::string> m_strings;
	};

	// Used as a return type to Lua that 'should' only have one field filled but contains a field
	// for every possible type (it's basically a poor man's union).
	struct ObjectFromList
	{
		AZ_TYPE_INFO(ObjectFromList, "{26824299-3334-4EBA-8CD3-1BB5454CD360}");
		AZ_CLASS_ALLOCATOR(ObjectFromList, AZ::SystemAllocator, 0);

		AZ::EntityId m_entityId;
		AZStd::string m_string;
	};


	/*!
	* ObjectListerComponentRequests
	* Messages serviced by the ObjectListerComponent
	*/
	class ObjectListerComponentRequests
		: public AZ::ComponentBus
	{
	public:
		virtual ~ObjectListerComponentRequests() {}

		//! Spawns a particle.
		virtual AZ::u32 GetListSize(AZ::u32 listID) = 0;
		virtual ObjectFromList GetListItem(AZ::u32 listID, AZ::u32 index) = 0;

	};

	using ObjectListerComponentRequestsBus = AZ::EBus<ObjectListerComponentRequests>;


	class ObjectListerComponent
		: public AZ::Component
		, private ObjectListerComponentRequestsBus::Handler
	{
	public:
		AZ_COMPONENT(ObjectListerComponent, "{95440FD8-31C8-4064-8A8A-521320E758B3}");

		//////////////////////////////////////////////////////////////////////////
		// AZ::Component interface implementation
		void Init() override;
		void Activate() override;
		void Deactivate() override;
		//////////////////////////////////////////////////////////////////////////

		// Required Reflect function.
		static void Reflect(AZ::ReflectContext* context);

		//////////////////////////////////////////////////////////////////////////
		// ObjectListerComponentRequestsBus::Handler
		AZ::u32 GetListSize(AZ::u32 listID) override;
		ObjectFromList GetListItem(AZ::u32 listID, AZ::u32 index) override;

	protected:
		ObjectFromList GenerateReturnFromVoidPtr(void* item) const;

		AZStd::vector<AZStd::shared_ptr<ObjectBase>> m_objects;

	};

} // namespace StarterGameGem
