// Portions of this file Copyright (C) Amazon.com, Inc. or its affiliates.
#ifndef CRYINCLUDE_CRYCOMMON_ICOMPONENTFACTORYREGISTRY_H
#define CRYINCLUDE_CRYCOMMON_ICOMPONENTFACTORYREGISTRY_H
#pragma once

#include <memory>
#include <IComponentFactory.h>
#include <ISystem.h>

//! Registry for all instances of IComponentFactory.
class IComponentFactoryRegistry
{
public:
	virtual ~IComponentFactoryRegistry() {}

	//! \return Whether factory was successfully registered.
	virtual bool RegisterFactory(std::unique_ptr<IComponentFactoryBase>&& factory) = 0;

	//! \return Whether factory was successfully unregistered and destroyed.
	virtual bool ReleaseFactory(const ComponentType& type) = 0;

	//! \return Factory which creates components of this type.
	//!         Returns nullptr if no such factory registered.
	template<class TComponentType>
	IComponentFactory<TComponentType>* GetFactory()
	{
		IComponentFactoryBase* factory = GetFactoryInternal(TComponentType::Type());
		return static_cast<IComponentFactory<TComponentType>*>(factory);
	}

	//! Iterate over all existing ComponentFactoryCreationNodes within this
	//! module, creating factories and registering them.
	static void RegisterAllComponentFactoryNodes(IComponentFactoryRegistry& registry);

protected:

	virtual IComponentFactoryBase* GetFactoryInternal(const ComponentType& type) = 0;

};

//
// Implementation
//

inline void IComponentFactoryRegistry::RegisterAllComponentFactoryNodes(IComponentFactoryRegistry& registry)
{
	// Don't let this run more than once per module.
	static bool hasRun = false;
	if (hasRun)
	{
		return;
	}
	hasRun = true;

	for (const ComponentFactoryCreationNode* node : ComponentFactoryCreationNode::GetGlobalList())
	{
		if (!node->GetCreateFactoryFunction())
		{
			CryWarning(VALIDATOR_MODULE_ENTITYSYSTEM, VALIDATOR_ERROR, "ComponentFactoryCreationNode is lacking creation function.");
			continue;
		}

		std::unique_ptr<IComponentFactoryBase> factory = node->GetCreateFactoryFunction()();
		if (!factory)
		{
			CryWarning(VALIDATOR_MODULE_ENTITYSYSTEM, VALIDATOR_ERROR, "ComponentFactoryCreationNode failed to create factory.");
			continue;
		}

		const ComponentType& type = factory->GetComponentType();
		bool success = registry.RegisterFactory(std::move(factory));
		if (!success)
		{
			CryWarning(VALIDATOR_MODULE_ENTITYSYSTEM, VALIDATOR_WARNING,
						"Failed to register factory, another factory is already registered for components of type '%s'.",
						type.Name());
			continue;
		}
	}
}

#endif // CRYINCLUDE_CRYCOMMON_ICOMPONENTFACTORYREGISTRY_H
