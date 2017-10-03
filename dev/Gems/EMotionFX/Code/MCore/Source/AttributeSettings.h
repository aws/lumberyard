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

// include the required headers
#include "StandardHeaders.h"
#include "Array.h"
#include "UnicodeString.h"
#include "Endian.h"


namespace MCore
{
    // forward declarations
    class Attribute;
    class Stream;


    /**
     * The attribute settings class, which describes the default, minimum and maximum value of a given attribute, next to its name and interface type.
     */
    class MCORE_API AttributeSettings
    {
        MCORE_MEMORYOBJECTCATEGORY(AttributeSettings, MCORE_DEFAULT_ALIGNMENT, MCORE_MEMCATEGORY_ATTRIBUTES);

    public:
        enum
        {
            FLAGINDEX_REINITGUI_ONVALUECHANGE       = 0,    // do we need to trigger a GUI reinit when the value of the interface widget linked to this attribute info is changed?
            FLAGINDEX_REFERENCE_OTHERATTRIBUTE      = 1,    // this attribute references another attribute
            FLAGINDEX_REINITOBJECT_ONVALUECHANGE    = 2,    // we need to reinitialize the object (can be a unique data update in the EMotion FX anim graph nodes for example)
            FLAGINDEX_CAN_SCALE                     = 3,    // this attribute has to be scaled when the Scale call is performed
            FLAGINDEX_REINIT_ATTRIBUTEWINDOW        = 4     // Set this flag in case a change of the attribute should trigger a complete reinit of the attributes window (used for attributes that influence other attributes).
        };

        static AttributeSettings* Create();
        static AttributeSettings* Create(const char* internalName);
        void Destroy(bool lock = true);

        bool GetReinitGuiOnValueChange() const;
        void SetReinitGuiOnValueChange(bool enabled);

        bool GetReinitObjectOnValueChange() const;
        void SetReinitObjectOnValueChange(bool enabled);

        bool GetCanScale() const;
        void SetCanScale(bool enabled);

        uint16 GetFlags() const;
        bool GetFlag(uint32 index) const;
        void SetFlag(uint32 index, bool enabled);
        void SetFlags(uint16 flags);

        void SetInternalName(const char* internalName);
        void SetName(const char* name);
        void SetDescription(const char* description);
        void SetInterfaceType(uint32 interfaceTypeID);
        uint32 GetInterfaceType() const;

        const char* GetInternalName() const;
        const char* GetName() const;
        const char* GetDescription() const;

        const String& GetInternalNameString() const;
        const String& GetNameString() const;
        const String& GetDescriptionString() const;

        void SetReferencesOtherAttribute(bool doesReference);
        bool GetReferencesOtherAttribute() const;

        uint32 GetNameID() const;
        uint32 GetInternalNameID() const;

        const Array<uint32>& GetComboValues() const;
        Array<uint32>& GetComboValues();
        const char* GetComboValue(uint32 index) const;
        const String& GetComboValueString(uint32 index) const;
        uint32 GetNumComboValues() const;
        void ReserveComboValues(uint32 numToReserve);
        void ResizeComboValues(uint32 numToResize);
        void AddComboValue(const char* value);
        void SetComboValue(uint32 index, const char* value);

        Attribute* GetDefaultValue() const;
        Attribute* GetMinValue() const;
        Attribute* GetMaxValue() const;

        void SetDefaultValue(Attribute* value, bool destroyCurrent = true);
        void SetMinValue(Attribute* value, bool destroyCurrent = true);
        void SetMaxValue(Attribute* value, bool destroyCurrent = true);

        void InitFrom(const AttributeSettings* other);
        AttributeSettings* Clone() const;
        AttributeSettings& operator=(const AttributeSettings& other);

        bool Write(Stream* stream, Endian::EEndianType targetEndianType) const;
        bool Read(Stream* stream, Endian::EEndianType endianType);
        uint32 CalcStreamSize() const;

        bool ConvertToString(String& outString) const;
        bool InitFromString(const String& valueString);

        void BuildToolTipString(String& outString, Attribute* value);   // value is allowed to be nullptr

        Attribute* GetParent() const;
        bool GetHasParent() const;
        void SetParent(Attribute* parent);
        AttributeSettings* FindParentSettings() const;

        void Scale(float scaleFactor);


    private:
        Attribute*          mDefaultValue;      /**< The default value. */
        Attribute*          mMinValue;          /**< The minimum value, can be nullptr. */
        Attribute*          mMaxValue;          /**< The maximum value, can be nullptr. */
        Attribute*          mParent;            /**< The parent attribute where the described one is a child of, or nullptr in case it has no parent. */
        Array<uint32>       mComboValues;       /**< The string ID's of the combo box. */
        String              mDescription;       /**< The description of the attribute. */
        uint32              mName;              /**< The name id as it will appear in the interface. */
        uint32              mInternalName;      /**< The internal name id, which is a short name used for lookups. */
        uint32              mInterfaceType;     /**< The interface type ID. */
        uint16              mFlags;             /**< Some flags. */

        AttributeSettings();
        AttributeSettings(const char* internalName);
        ~AttributeSettings();
    };
}   // namespace MCore
