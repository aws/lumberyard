/**
 *  yasli - Serialization Library.
 *  Copyright (C) 2007-2011 Evgeny Andreeshchev <eugene.andreeshchev@gmail.com>
 *                          Alexander Kotliar <alexander.kotliar@gmail.com>
 *
 *  This code is distributed under the MIT License:
 *                          http://www.opensource.org/licenses/MIT
 */
#ifndef CRYINCLUDE_EDITORCOMMON_QPROPERTYTREE_FACTORY_H
#define CRYINCLUDE_EDITORCOMMON_QPROPERTYTREE_FACTORY_H
#pragma once

#include <map>
#include "Serialization/Assert.h"

template<class _Key, class _Product, class _KeyPred = std::less<_Key>>
class Factory {
public:
	typedef std::map<_Key, AZStd::function<_Product *()>, _KeyPred> Creators;

	Factory() {}

	struct Creator{
		Creator(Factory& factory, const _Key& key, AZStd::function<_Product *()> construction_function_) {
			factory.add(key, construction_function_);
		}
		Creator(const _Key& key, AZStd::function<_Product *()> construction_function_) {
			Factory::the().add(key, construction_function_);
		}
	};

	void add(const _Key& key, AZStd::function<_Product *()> creator) {
		YASLI_ASSERT(creators_.find(key) == creators_.end());
		YASLI_ASSERT(creator);
		creators_[key] = creator;
	}

	void remove(const _Key& key) {
		auto& entry = creators_.find(key);
		if (entry != creators_.end()) {
			creators_.erase(entry);
		}
	}

	_Product* create(const _Key& key) const {
		typename Creators::const_iterator it = creators_.find(key);
		if (it != creators_.end()) {
			return it->second();
		}
		else
			return 0;
	}

	std::size_t size() const { return creators_.size(); }

	_Product* createByIndex(int index) const {
		YASLI_ASSERT(index >= 0 && index < creators_.size());
		typename Creators::const_iterator it = creators_.begin();
		std::advance(it, index);
		return it->second();
	}


	const Creators& creators() const { return creators_; }

	static Factory& the()
	{
		static Factory genericFactory;
		return genericFactory;
	}
protected:
	Creators creators_;
};

#define REGISTER_IN_FACTORY(factory, key, product, construction_function)                  \
	static factory::Creator factory##product##Creator(key, construction_function);

#define REGISTER_IN_FACTORY_INSTANCE(factory, factoryType, key, product)              \
	static factoryType::Creator<product> factoryType##product##Creator(factory, key);

#define DECLARE_SEGMENT(fileName) int dataSegment##fileName;

#define FORCE_SEGMENT(fileName) \
	extern int dataSegment##fileName; \
	int* dataSegmentPtr##fileName = &dataSegment##fileName;


#endif // CRYINCLUDE_EDITORCOMMON_QPROPERTYTREE_FACTORY_H
