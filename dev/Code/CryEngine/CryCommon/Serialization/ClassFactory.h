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
// Original file Copyright Crytek GMBH or its affiliates, used under license.

#ifndef CRYINCLUDE_CRYCOMMON_SERIALIZATION_CLASSFACTORY_H
#define CRYINCLUDE_CRYCOMMON_SERIALIZATION_CLASSFACTORY_H
#pragma once

#include <map>
#include <vector>

#include "Serialization/Assert.h"
#include "Serialization/IClassFactory.h"
#include "Serialization/TypeID.h"

namespace Serialization {
    class IArchive;

    class ClassFactoryManager
    {
    public:
        static ClassFactoryManager& the()
        {
            static ClassFactoryManager factoryManager;
            return factoryManager;
        }

        const IClassFactory* find(TypeID baseType) const
        {
            Factories::const_iterator it = factories_.find(baseType);
            if (it == factories_.end())
            {
                return 0;
            }
            else
            {
                return it->second;
            }
        }

        void registerFactory(TypeID type, const IClassFactory* factory)
        {
            factories_[type] = factory;
        }
    protected:
        typedef std::map<TypeID, const IClassFactory*> Factories;
        Factories factories_;
    };

    template<class BaseType>
    class ClassFactory
        : public IClassFactory
    {
    public:
        static ClassFactory& the()
        {
            static ClassFactory factory;
            return factory;
        }

        class CreatorBase
        {
        public:
            virtual ~CreatorBase() {}
            virtual BaseType* create() const = 0;
            virtual const TypeDescription& description() const{ return *description_; }
            virtual void* vptr() const{ return vptr_; }
            virtual TypeID typeID() const = 0;
        protected:
            const TypeDescription* description_;
            void* vptr_;
        };

        static void* extractVPtr(BaseType* ptr)
        {
            return *((void**)ptr);
        }

        template<class Derived>
        struct Annotation
        {
            Annotation(IClassFactory* factory, const char* name, const char* value) { static_cast<ClassFactory<BaseType>*>(factory)->addAnnotation<Derived>(name, value); }
        };

        template<class Derived>
        class Creator
            : public CreatorBase
        {
        public:
            Creator(const TypeDescription* description, ClassFactory* factory = nullptr)
            {
                this->description_ = description;

                Derived vptrProbe;
                this->vptr_ = extractVPtr(&vptrProbe);

                if (!factory)
                {
                    factory = &ClassFactory::the();
                }

                factory->registerCreator(this);
            }
            BaseType* create() const override { return new Derived(); }
            TypeID typeID() const override { return Serialization::TypeID::get<Derived>(); }
        };

        ClassFactory()
            : IClassFactory(TypeID::get<BaseType>())
        {
            ClassFactoryManager::the().registerFactory(baseType_, this);
        }

        typedef std::map<string, const CreatorBase*> TypeToCreatorMap;

        virtual BaseType* create(const char* registeredName) const
        {
            if (!registeredName)
            {
                return 0;
            }
            if (registeredName[0] == '\0')
            {
                return 0;
            }
            typename TypeToCreatorMap::const_iterator it = typeToCreatorMap_.find(registeredName);
            if (it != typeToCreatorMap_.end())
            {
                return it->second->create();
            }
            else
            {
                return 0;
            }
        }

        virtual const char* getRegisteredTypeName(BaseType* ptr) const
        {
            if (ptr == 0)
            {
                return "";
            }
            void* vptr = extractVPtr(ptr);
            typename VPtrToCreatorMap::const_iterator it = vptrToCreatorMap_.find(vptr);
            if (it == vptrToCreatorMap_.end())
            {
                return "";
            }
            return it->second->description().name();
        }

        BaseType* createByIndex(int index) const
        {
            YASLI_ASSERT(size_t(index) < creators_.size());
            return creators_[index]->create();
        }

        void serializeNewByIndex(IArchive& ar, int index, const char* name, const char* label)
        {
            YASLI_ESCAPE(size_t(index) < creators_.size(), return );
            BaseType* ptr = creators_[index]->create();
            ar(*ptr, name, label);
            delete ptr;
        }
        // from ClassFactoryInterface:
        size_t size() const{ return creators_.size(); }
        const TypeDescription* descriptionByIndex(int index) const override
        {
            if (size_t(index) >= int(creators_.size()))
            {
                return 0;
            }
            return &creators_[index]->description();
        }

        const TypeDescription* descriptionByRegisteredName(const char* name) const override
        {
            const size_t numCreators = creators_.size();
            for (size_t i = 0; i < numCreators; ++i)
            {
                if (strcmp(creators_[i]->description().name(), name) == 0)
                {
                    return &creators_[i]->description();
                }
            }
            return 0;
        }
        // ^^^

        TypeID typeIDByRegisteredName(const char* registeredTypeName) const
        {
            RegisteredNameToTypeID::const_iterator it = registeredNameToTypeID_.find(registeredTypeName);
            if (it == registeredNameToTypeID_.end())
            {
                return TypeID();
            }
            return it->second;
        }

        const char* findAnnotation(const char* registeredTypeName, const char* name) const
        {
            TypeID typeID = typeIDByRegisteredName(registeredTypeName);
            AnnotationMap::const_iterator it = annotations_.find(typeID);
            if (it == annotations_.end())
            {
                return "";
            }
            for (size_t i = 0; i < it->second.size(); ++i)
            {
                if (strcmp(it->second[i].first, name) == 0)
                {
                    return it->second[i].second;
                }
            }
            return "";
        }
        void unregisterCreator(const TypeDescription& typeDescription)
        {
            auto creator = typeToCreatorMap_.find(typeDescription.name());
            if (creator != typeToCreatorMap_.end())
            {
                creators_.erase(std::find(creators_.begin(), creators_.end(), creator->second));
                vptrToCreatorMap_.erase(vptrToCreatorMap_.find(creator->second->vptr()));
                typeToCreatorMap_.erase(creator);
            }
        }

    protected:
        virtual void registerCreator(CreatorBase* creator)
        {
            if (!typeToCreatorMap_.insert(std::make_pair(creator->description().name(), creator)).second)
            {
                YASLI_ASSERT(0 && "Type registered twice in the same factory. Was SERIALIZATION_CLASS_NAME put into header file by mistake?");
            }
            creators_.push_back(creator);
            registeredNameToTypeID_[creator->description().name()] = creator->typeID();
            vptrToCreatorMap_[creator->vptr()] =  creator;
        }

        template<class T>
        void addAnnotation(const char* name, const char* value)
        {
            addAnnotation(Serialization::TypeID::get<T>(), name, value);
        }

        virtual void addAnnotation(const Serialization::TypeID& id, const char* name, const char* value)
        {
            annotations_[id].push_back(std::make_pair(name, value));
        }

        TypeToCreatorMap typeToCreatorMap_;
        std::vector<CreatorBase*> creators_;

        typedef std::map<void*, CreatorBase*> VPtrToCreatorMap;
        VPtrToCreatorMap vptrToCreatorMap_;

        typedef std::map<string, TypeID> RegisteredNameToTypeID;
        RegisteredNameToTypeID registeredNameToTypeID_;

        typedef std::map<TypeID, std::vector<std::pair<const char*, const char*> > > AnnotationMap;
        AnnotationMap annotations_;
    };
}

#define SERIALIZATION_CLASS_NULL(BaseType, name)                                                          \
    namespace {                                                                                           \
        bool BaseType##_NullRegistered = Serialization::ClassFactory<BaseType>::the().setNullLabel(name); \
    }

#define SERIALIZATION_CLASS_NAME(BaseType, Type, name, label)                                                                   \
    static const Serialization::TypeDescription Type##BaseType##_DerivedDescription(name, label);                               \
    static Serialization::ClassFactory<BaseType>::Creator<Type> Type##BaseType##_Creator(&Type##BaseType##_DerivedDescription); \
    int dummyForType_##Type##BaseType;

#define SERIALIZATION_CLASS_NAME_FOR_FACTORY(Factory, BaseType, Type, name, label)                \
    static const Serialization::TypeDescription Type##BaseType##_DerivedDescription(name, label); \
    static Serialization::ClassFactory<BaseType>::Creator<Type> Type##BaseType##_Creator(&Type##BaseType##_DerivedDescription, &(Factory));

#define SERIALIZATION_CLASS_ANNOTATION(BaseType, Type, attributeName, attributeValue) \
    static Serialization::ClassFactory<BaseType>::Annotation<Type> Type##BaseType##_Annotation(&Serialization::ClassFactory<BaseType>::the(), attributeName, attributeValue);

#define SERIALIZATION_CLASS_ANNOTATION_FOR_FACTORY(factory, BaseType, Type, attributeName, attributeValue) \
    static Serialization::ClassFactory<BaseType>::Annotation<Type> Type##BaseType##_Annotation(&factory, attributeName, attributeValue);

#define SERIALIZATION_FORCE_CLASS(BaseType, Type) \
    extern int dummyForType_##Type##BaseType;     \
    int* dummyForTypePtr_##Type##BaseType = &dummyForType_##Type##BaseType + 1;

#include "ClassFactoryImpl.h"

#endif // CRYINCLUDE_CRYCOMMON_SERIALIZATION_CLASSFACTORY_H
