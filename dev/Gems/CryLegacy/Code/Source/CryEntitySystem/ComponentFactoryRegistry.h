// Portions of this file Copyright (C) Amazon.com, Inc. or its affiliates.
#ifndef CRYINCLUDE_CRYENTITYSYSTEM_COMPONENTFACTORYREGISTRY_H
#define CRYINCLUDE_CRYENTITYSYSTEM_COMPONENTFACTORYREGISTRY_H
#pragma once

#include <AzCore/std/smart_ptr/unique_ptr.h>
#include <AzCore/std/containers/unordered_map.h>

class CComponentFactoryRegistry : public IComponentFactoryRegistry
{
public:
	bool RegisterFactory(AZStd::unique_ptr<IComponentFactoryBase>&& factory) override;
	bool ReleaseFactory(const ComponentType& type) override;

private:
	IComponentFactoryBase* GetFactoryInternal(const ComponentType& type) override;

	AZStd::unordered_map<ComponentType, AZStd::unique_ptr<IComponentFactoryBase>> m_factories;
};

#endif // CRYINCLUDE_CRYENTITYSYSTEM_COMPONENTTYPEREGISTRY_H
