// Portions of this file Copyright (C) Amazon.com, Inc. or its affiliates.
#ifndef CRYINCLUDE_CRYENTITYSYSTEM_COMPONENTFACTORYREGISTRY_H
#define CRYINCLUDE_CRYENTITYSYSTEM_COMPONENTFACTORYREGISTRY_H
#pragma once

#include <unordered_map>

class CComponentFactoryRegistry : public IComponentFactoryRegistry
{
public:
	bool RegisterFactory(std::unique_ptr<IComponentFactoryBase>&& factory) override;
	bool ReleaseFactory(const ComponentType& type) override;

private:
	IComponentFactoryBase* GetFactoryInternal(const ComponentType& type) override;

	std::unordered_map<ComponentType, std::unique_ptr<IComponentFactoryBase>> m_factories;
};

#endif // CRYINCLUDE_CRYENTITYSYSTEM_COMPONENTTYPEREGISTRY_H
