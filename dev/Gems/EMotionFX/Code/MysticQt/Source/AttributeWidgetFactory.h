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

#ifndef __EMSTUDIO_ATTRIBUTEWIDGETFACTORY_H
#define __EMSTUDIO_ATTRIBUTEWIDGETFACTORY_H

// include required headers
#include <MCore/Source/StandardHeaders.h>
#include <MCore/Source/Array.h>
#include "MysticQtConfig.h"
#include "AttributeWidgetCreators.h"


namespace MysticQt
{
    class MYSTICQT_API AttributeWidgetFactory
    {
        MCORE_MEMORYOBJECTCATEGORY(AttributeWidgetFactory, MCore::MCORE_DEFAULT_ALIGNMENT, MEMCATEGORY_MYSTICQT_ATTRIBUTEWIDGETS);

    public:
        AttributeWidgetFactory();
        ~AttributeWidgetFactory();

        class MYSTICQT_API Callback
        {
        public:
            Callback() {}
            virtual ~Callback() {}
            virtual void OnAttributeChanged() = 0;
            virtual void OnStructureChanged() = 0;
            virtual void OnObjectChanged() = 0;
            virtual void OnUpdateAttributeWindow() = 0;
        };

        enum AttributeWidgetType
        {
            ATTRIBUTE_NORMAL    = 0,
            ATTRIBUTE_DEFAULT   = 1,
            ATTRIBUTE_MIN       = 2,
            ATTRIBUTE_MAX       = 3
        };

        AttributeWidget* CreateAttributeWidget(MCore::Array<MCore::Attribute*>& attributes, MCore::AttributeSettings* attributeSettings, void* customData, bool readOnly, bool forceInitMinMaxAttributes = false, bool resetMinMaxAttributes = true, AttributeWidgetType attributeWidgetType = ATTRIBUTE_NORMAL, bool creationMode = false, AttributeChangedFunction func = nullptr);
        AttributeWidget* CreateAttributeWidget(MCore::Attribute* attribute, MCore::AttributeSettings* attributeSettings, void* customData, bool readOnly, bool forceInitMinMaxAttributes = false, bool resetMinMaxAttributes = true, AttributeWidgetType attributeWidgetType = ATTRIBUTE_NORMAL, bool creationMode = false, AttributeChangedFunction func = nullptr);

        void InitAttributes(MCore::Array<MCore::Attribute*>& attributes, MCore::AttributeSettings* attributeSettings, bool forceInitMinMaxAttributes = false, AttributeWidgetType attributeWidgetType = ATTRIBUTE_NORMAL);

        bool RegisterCreator(AttributeWidgetCreator* creator);
        void UnregisterAllCreators();
        bool UnregisterCreatorByTypeID(uint32 nodeTypeID);
        bool CheckIfHasRegisteredCreatorByTypeID(uint32 nodeTypeID) const;
        uint32 FindRegisteredCreatorByTypeID(uint32 typeID, AttributeWidgetType attributeWidgetType = ATTRIBUTE_NORMAL) const;
        uint32 FindCreatorIndexByType(uint32 typeID);

        // callbacks
        void ClearCallbacks();
        void RemoveCallback(Callback* callback, bool delFromMem = true);
        void OnAttributeChanged();  // inform all callbacks about an attribute change
        void OnStructureChanged();  // inform all callbacks about a structure change
        void OnObjectChanged();     // inform all callbacks about an object change
        void OnUpdateAttributeWindow();
        void AddCallback(Callback* callback)                                        { mCallbacks.Add(callback); }
        MCORE_INLINE uint32 GetNumCallbacks() const                                 { return mCallbacks.GetLength(); }
        MCORE_INLINE Callback* GetCallback(uint32 index) const                      { return mCallbacks[index]; }

        MCORE_INLINE uint32 GetNumRegisteredCreators() const                        { return mRegisteredCreators.GetLength(); }
        MCORE_INLINE AttributeWidgetCreator* GetRegisteredCreator(uint32 index)     { return mRegisteredCreators[index]; }

    private:
        MCore::Array<AttributeWidgetCreator*>   mRegisteredCreators;
        MCore::Array<Callback*>                 mCallbacks;
    };
} // namespace MysticQt


#endif
