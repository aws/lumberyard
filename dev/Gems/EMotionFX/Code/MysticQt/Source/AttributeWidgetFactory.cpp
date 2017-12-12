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

// include required headers
#include "AttributeWidgetFactory.h"
#include <MCore/Source/LogManager.h>
#include "AttributeWidgetCreators.h"


namespace MysticQt
{
    // constructor
    AttributeWidgetFactory::AttributeWidgetFactory()
    {
        // reserve space for the registered creators
        mRegisteredCreators.Reserve(20);
        mRegisteredCreators.SetMemoryCategory(MEMCATEGORY_MYSTICQT_ATTRIBUTEWIDGETS);

        // register default creators
        RegisterCreator(new CheckboxAttributeWidgetCreator());
        RegisterCreator(new FloatSpinnerAttributeWidgetCreator());
        RegisterCreator(new IntSpinnerAttributeWidgetCreator());
        RegisterCreator(new FloatSliderAttributeWidgetCreator());
        RegisterCreator(new IntSliderAttributeWidgetCreator());
        RegisterCreator(new ComboBoxAttributeWidgetCreator());
        RegisterCreator(new Vector2AttributeWidgetCreator());
        RegisterCreator(new Vector3AttributeWidgetCreator());
        RegisterCreator(new Vector4AttributeWidgetCreator());
        RegisterCreator(new StringAttributeWidgetCreator());
        RegisterCreator(new ColorAttributeWidgetCreator());
        RegisterCreator(new AttributeSetAttributeWidgetCreator());
    }


    // destructor
    AttributeWidgetFactory::~AttributeWidgetFactory()
    {
        UnregisterAllCreators();
        ClearCallbacks();
    }


    // create a given attribute widget creator
    AttributeWidget* AttributeWidgetFactory::CreateAttributeWidget(MCore::Array<MCore::Attribute*>& attributes, MCore::AttributeSettings* attributeSettings, void* customData, bool readOnly, bool forceInitMinMaxAttributes, bool resetMinMaxAttributes, AttributeWidgetType attributeWidgetType, bool creationMode, AttributeChangedFunction func)
    {
        // try to find the registered creator index
        uint32 interfaceTypeID = attributeSettings->GetInterfaceType();
        if ((interfaceTypeID == MCORE_INVALIDINDEX32 || interfaceTypeID == MCore::ATTRIBUTE_INTERFACETYPE_DEFAULT) && attributes[0])
        {
            interfaceTypeID = attributes[0]->GetDefaultInterfaceType();
        }

        const uint32 index = FindRegisteredCreatorByTypeID(interfaceTypeID, attributeWidgetType);
        if (index == MCORE_INVALIDINDEX32)
        {
            MCore::LogWarning("AttributeWidgetFactory::CreateAttributeWidgetByTypeID() - failed to create attribute widget creator with type ID %d, as no such node type has been registered", attributeSettings->GetInterfaceType());
            return nullptr;
        }

        // init the attributes
        mRegisteredCreators[index]->InitAttributes(attributes, attributeSettings, forceInitMinMaxAttributes, resetMinMaxAttributes);

        // init the min values
        if (attributeWidgetType == ATTRIBUTE_MIN && resetMinMaxAttributes)
        {
            const uint32 numAttributes = attributes.GetLength();
            for (uint32 i = 0; i < numAttributes; ++i)
            {
                if (attributes[i])
                {
                    attributes[i]->InitFrom(mRegisteredCreators[index]->GetInitialMinValue());
                }
            }
        }

        // init the max values
        if (attributeWidgetType == ATTRIBUTE_MAX && resetMinMaxAttributes)
        {
            const uint32 numAttributes = attributes.GetLength();
            for (uint32 i = 0; i < numAttributes; ++i)
            {
                if (attributes[i])
                {
                    attributes[i]->InitFrom(mRegisteredCreators[index]->GetInitialMaxValue());
                }
            }
        }

        // create a clone of the registered attribute widget creator
        AttributeWidget* result = mRegisteredCreators[index]->Clone(attributes, attributeSettings, customData, readOnly, creationMode, func);
        return result;
    }


    AttributeWidget* AttributeWidgetFactory::CreateAttributeWidget(MCore::Attribute* attribute, MCore::AttributeSettings* attributeSettings, void* customData, bool readOnly, bool forceInitMinMaxAttributes, bool resetMinMaxAttributes, AttributeWidgetType attributeWidgetType, bool creationMode, AttributeChangedFunction func)
    {
        MCore::Array<MCore::Attribute*> attributes;
        attributes.Add(attribute);

        return CreateAttributeWidget(attributes, attributeSettings, customData, readOnly, forceInitMinMaxAttributes, resetMinMaxAttributes, attributeWidgetType, creationMode, func);
    }


    void AttributeWidgetFactory::InitAttributes(MCore::Array<MCore::Attribute*>& attributes, MCore::AttributeSettings* attributeSettings, bool forceInitMinMaxAttributes, AttributeWidgetType attributeWidgetType)
    {
        // try to find the registered creator index
        const uint32 index = FindRegisteredCreatorByTypeID(attributeSettings->GetInterfaceType(), attributeWidgetType);
        if (index == MCORE_INVALIDINDEX32)
        {
            MCore::LogWarning("AttributeWidgetFactory::InitAttributes() - failed to init attribute widget creator with type ID %d, as no such node type has been registered", attributeSettings->GetInterfaceType());
        }

        mRegisteredCreators[index]->InitAttributes(attributes, attributeSettings, forceInitMinMaxAttributes);
    }


    // unregister all creators
    void AttributeWidgetFactory::UnregisterAllCreators()
    {
        const uint32 numCreators = mRegisteredCreators.GetLength();
        for (uint32 i = 0; i < numCreators; ++i)
        {
            delete mRegisteredCreators[i];
        }

        // clear the array
        mRegisteredCreators.Clear();
    }


    // unregister a creator with a given type ID
    bool AttributeWidgetFactory::UnregisterCreatorByTypeID(uint32 typeID)
    {
        // try to find the creator index
        const uint32 index = FindRegisteredCreatorByTypeID(typeID);
        if (index == MCORE_INVALIDINDEX32)
        {
            return false;
        }

        // remove the creator
        delete mRegisteredCreators[index];
        mRegisteredCreators.Remove(index);

        // successfully unregistered
        return true;
    }


    // register a given creator
    bool AttributeWidgetFactory::RegisterCreator(AttributeWidgetCreator* creator)
    {
        // make sure we didn't already register this node
        if (CheckIfHasRegisteredCreatorByTypeID(creator->GetType()))
        {
            MCore::LogWarning("AttributeWidgetFactory::RegisterCreator() - Already registered the given creator, skipping registration (type=%s)", creator->GetTypeString());
            MCORE_ASSERT(false);
            return false;
        }

        // register it
        mRegisteredCreators.Add(creator);
        return true;
    }


    // check if we have a registered creator of the given type
    bool AttributeWidgetFactory::CheckIfHasRegisteredCreatorByTypeID(uint32 typeID) const
    {
        return (FindRegisteredCreatorByTypeID(typeID) != MCORE_INVALIDINDEX32);
    }


    // find registered creator by its type ID
    uint32 AttributeWidgetFactory::FindRegisteredCreatorByTypeID(uint32 typeID, AttributeWidgetType attributeWidgetType) const
    {
        uint32 result = MCORE_INVALIDINDEX32;

        // for all registered creators
        const uint32 numCreators = mRegisteredCreators.GetLength();
        for (uint32 i = 0; i < numCreators; ++i)
        {
            if (mRegisteredCreators[i]->GetType() == typeID)
            {
                result = i;
                break;
            }
        }

        switch (attributeWidgetType)
        {
        case ATTRIBUTE_NORMAL:
        {
            return result;
            break;
        }
        case ATTRIBUTE_DEFAULT:
        {
            return FindRegisteredCreatorByTypeID(mRegisteredCreators[result]->GetDefaultType(), ATTRIBUTE_NORMAL);
            break;
        }
        case ATTRIBUTE_MIN:
        {
            return FindRegisteredCreatorByTypeID(mRegisteredCreators[result]->GetMinMaxType(), ATTRIBUTE_NORMAL);
            break;
        }
        case ATTRIBUTE_MAX:
        {
            return FindRegisteredCreatorByTypeID(mRegisteredCreators[result]->GetMinMaxType(), ATTRIBUTE_NORMAL);
            break;
        }
        }
        ;

        return MCORE_INVALIDINDEX32;
    }


    uint32 AttributeWidgetFactory::FindCreatorIndexByType(uint32 typeID)
    {
        // for all registered creators
        const uint32 numCreators = mRegisteredCreators.GetLength();
        for (uint32 i = 0; i < numCreators; ++i)
        {
            if (mRegisteredCreators[i]->GetType() == typeID)
            {
                return i;
            }
        }

        return MCORE_INVALIDINDEX32;
    }


    // get rid of all callbacks
    void AttributeWidgetFactory::ClearCallbacks()
    {
        // get the number of callbacks, iterate through and remove them
        const uint32 numCallbacks = mCallbacks.GetLength();
        for (uint32 i = 0; i < numCallbacks; ++i)
        {
            delete mCallbacks[i];
        }

        mCallbacks.Clear();
    }


    // remove the given callback
    void AttributeWidgetFactory::RemoveCallback(Callback* callback, bool delFromMem)
    {
        // get the number of callbacks, iterate through and call the callback
        const uint32 numCallbacks = mCallbacks.GetLength();
        for (uint32 i = 0; i < numCallbacks; ++i)
        {
            // check if the current callback is the given one
            if (mCallbacks[i] == callback)
            {
                if (delFromMem)
                {
                    delete mCallbacks[i];
                }

                mCallbacks.Remove(i);
                return;
            }
        }
    }


    // inform all callbacks about that an attribute changed
    void AttributeWidgetFactory::OnAttributeChanged()
    {
        // get the number of callbacks, iterate through and call the callback
        const uint32 numCallbacks = mCallbacks.GetLength();
        for (uint32 i = 0; i < numCallbacks; ++i)
        {
            mCallbacks[i]->OnAttributeChanged();
        }
    }


    // inform all callbacks about that an attribute changed the structure of the data
    void AttributeWidgetFactory::OnStructureChanged()
    {
        // get the number of callbacks, iterate through and call the callback
        const uint32 numCallbacks = mCallbacks.GetLength();
        for (uint32 i = 0; i < numCallbacks; ++i)
        {
            mCallbacks[i]->OnStructureChanged();
        }
    }

    // inform all callbacks about that an attribute changed the object
    void AttributeWidgetFactory::OnObjectChanged()
    {
        // get the number of callbacks, iterate through and call the callback
        const uint32 numCallbacks = mCallbacks.GetLength();
        for (uint32 i = 0; i < numCallbacks; ++i)
        {
            mCallbacks[i]->OnObjectChanged();
        }
    }

    // Inform the attribute window to fully reinit the attribute set.
    void AttributeWidgetFactory::OnUpdateAttributeWindow()
    {
        const uint32 numCallbacks = mCallbacks.GetLength();
        for (uint32 i = 0; i < numCallbacks; ++i)
        {
            mCallbacks[i]->OnUpdateAttributeWindow();
        }
    }
} // namespace MysticQt

#include <MysticQt/Source/AttributeWidgetFactory.moc>