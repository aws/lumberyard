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

//#include <LmbrCentral/Physics/PersistentDataSystemComponentBus.h>
#include <AzCore/Component/Component.h>

#include <AzCore/EBus/EBus.h>
#include <AzCore/Component/EntityId.h>
#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/std/any.h>
#include <GameplayEventBus.h>

namespace StarterGameGem
{


	/*!
	* Requests for the physics system
	*/
	class PersistentDataSystemRequests
		: public AZ::EBusTraits
	{
	public:
		////////////////////////////////////////////////////////////////////////
		// EBusTraits
		// singleton pattern
		static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;
		static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::Single;
		////////////////////////////////////////////////////////////////////////

		virtual ~PersistentDataSystemRequests() = default;

		virtual bool HasData(const AZStd::string& name) = 0;
		virtual bool SetData(const AZStd::string& name, const AZStd::any& value, const bool addIfNotExist = false) = 0;
		virtual AZStd::any GetData(const AZStd::string& name) = 0;
		virtual void RemoveData(const AZStd::string& name) = 0;
		// when this callback gets called, the data with it will be the new value
		virtual void RegisterDataChangeCallback(const AZStd::string& name, const AZ::GameplayNotificationId& messageID) = 0;
		virtual void UnRegisterDataChangeCallback(const AZStd::string& name, const AZ::GameplayNotificationId& messageID) = 0;

		virtual void ClearAllData() = 0;
	};

	using PersistentDataSystemRequestBus = AZ::EBus<PersistentDataSystemRequests>;

	/*!
	 * System component which listens for IPhysicalWorld events,
	 * filters for events that involve AZ::Entities,
	 * and broadcasts these events on the EntityPhysicsEventBus.
	 */
	class PersistentDataSystemComponent
		: public AZ::Component
		, public PersistentDataSystemRequestBus::Handler
	{
	public:
		AZ_COMPONENT(PersistentDataSystemComponent, "{A2087EFC-9F55-4603-B40B-6A368C102E66}");

		static void Reflect(AZ::ReflectContext* context);

		static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
		{
			provided.push_back(AZ_CRC("PersistentDataSystemService"));
		}

		static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
		{
			incompatible.push_back(AZ_CRC("PersistentDataSystemService"));
		}

		~PersistentDataSystemComponent() override {}

		static PersistentDataSystemComponent* GetInstance();


		enum class ParentActivationTransformMode : AZ::u32
		{
			MaintainOriginalRelativeTransform,  ///< Child will snap to originally-configured parent-relative transform when parent is activated.
			MaintainCurrentWorldTransform,      ///< Child will still follow parent, but will maintain its current world transform when parent is activated.
		};
		ParentActivationTransformMode           m_parentActivationTransformMode;

		enum class eComparison : AZ::u32
		{
			Equal,
			GreaterThan,
			GreaterThanOrEqual,
			LessThan,
			LessThanOrEqual,
		};

		enum class eBasicDataTypes : AZ::u32
		{
			Number,
			String,
			Bool,
			Other,

			Empty,
		};

		enum class eDataManipulationTypes : AZ::u32
		{
			AddOnly,
			SetOnly,
			AddOrSet,

			Toggle, // valid only for bools

			Increment, // valid only for numbers

			Append, // valid only for strings
			Prepend, // valid only for strings
			// possible other string operations to be added later
		};

		static eBasicDataTypes GetBasicType(const AZStd::any& value);
		// ture if passes test, otherwise false, including type missmatch
		bool Compare(const AZStd::string& name, const AZStd::any& value, const eComparison compareType);
		// returns tru if operation could be performed
		bool Manipulate(const AZStd::string& name, const AZStd::any& value, const eDataManipulationTypes manipulationType);

		////////////////////////////////////////////////////////////////////////
		// PersistentDataSystemRequests
		bool HasData(const AZStd::string& name) override;
		bool SetData(const AZStd::string& name, const AZStd::any& value, const bool addIfNotExist = false) override;
		AZStd::any GetData(const AZStd::string& name) override;
		void RemoveData(const AZStd::string& name) override;
		// when this callback gets called, the data with it will be the new value
		void RegisterDataChangeCallback(const AZStd::string& name, const AZ::GameplayNotificationId& messageID) override;
		void UnRegisterDataChangeCallback(const AZStd::string& name, const AZ::GameplayNotificationId& messageID) override;
		void ClearAllData() override;
		////////////////////////////////////////////////////////////////////////

	private:
		////////////////////////////////////////////////////////////////////////
		// AZ::Component
		void Activate() override;
		void Deactivate() override;
		////////////////////////////////////////////////////////////////////////

		// my things here 

		class cDataSet
		{
		public:
			cDataSet(const AZStd::string& name, const AZStd::any& value)
				: m_name(name)
				, m_value(value)
			{}

			~cDataSet() = default;

			void SetValue(const AZStd::any& value);
			const AZStd::any& GetValue() const { return m_value; }
			const AZStd::string& GetName() const { return m_name; }

			void RegisterCallback(const AZ::GameplayNotificationId& messageID);
			void UnRegisterCallback(const AZ::GameplayNotificationId& messageID);

		private:
			const AZStd::string m_name;
			AZStd::any m_value;
			AZStd::list<AZ::GameplayNotificationId> m_callbacks;


			bool AlreadyHaveCallback(const AZ::GameplayNotificationId& messageID, AZStd::list<AZ::GameplayNotificationId>::iterator& currentCallback);
		};

		AZStd::list<cDataSet> m_Datas;

		bool FindData(const AZStd::string& name, AZStd::list<cDataSet>::iterator& currentData);
		void AddData(const AZStd::string& name, const AZStd::any& value);
		//bool RemoveData(const AZStd::string& name, AZStd::list<cDataSet>::iterator& currentData) const ;
	};
} // namespace LmbrCentral
