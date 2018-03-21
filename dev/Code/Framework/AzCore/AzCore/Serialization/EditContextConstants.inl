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

namespace AZ
{
    namespace Edit
    {
        namespace ClassElements
        {
            const static AZ::Crc32 EditorData = AZ_CRC("EditorData", 0xf44f1a1d);
            const static AZ::Crc32 Group = AZ_CRC("Group", 0x6dc044c5);
        }

        namespace Attributes
        {
            //! AddableByUser : a bool which determines if the component can be added by the user.
            //! Setting this to false effectively hides the component from views where user can create components.
            const static AZ::Crc32 AddableByUser = AZ_CRC("AddableByUser", 0x181bc2f4);
            const static AZ::Crc32 AppearsInAddComponentMenu = AZ_CRC("AppearsInAddComponentMenu", 0x53790e31);
            const static AZ::Crc32 AutoExpand = AZ_CRC("AutoExpand", 0x306ff5c0);
            const static AZ::Crc32 ButtonText = AZ_CRC("ButtonText", 0x79fe5d8b);
            const static AZ::Crc32 Category = AZ_CRC("Category", 0x064c19c1);
            const static AZ::Crc32 Visibility = AZ_CRC("Visibility", 0x518e4300);

            //! Container attributes
            const static AZ::Crc32 ContainerCanBeModified = AZ_CRC("ContainerCanBeModified", 0xd9948f69);
            const static AZ::Crc32 StringList = AZ_CRC("StringList", 0xdf80b99c);

            const static AZ::Crc32 ValueText = AZ_CRC("ValueText", 0x251534d1);

            const static AZ::Crc32 TrueText = AZ_CRC("TrueText", 0x263d9d95);
            const static AZ::Crc32 FalseText = AZ_CRC("FalseText", 0x5f8c95bd);

            const static AZ::Crc32 EnumValues = AZ_CRC("EnumValues", 0xc551a2e0);

            //! Used to bind either a callback or a refresh mode to the changing of a particular property.
            const static AZ::Crc32 AddNotify = AZ_CRC("AddNotify", 0x16f00b95);
            const static AZ::Crc32 RemoveNotify = AZ_CRC("RemoveNotify", 0x16ec95f5);
            const static AZ::Crc32 ChangeNotify = AZ_CRC("ChangeNotify", 0xf793bc19);

            const static AZ::Crc32 NameLabelOverride = AZ_CRC("NameLabelOverride", 0x9ff79cab);
            const static AZ::Crc32 DescriptionTextOverride = AZ_CRC("DescriptionTextOverride", 0x608b64a8);

            const static AZ::Crc32 PrimaryAssetType = AZ_CRC("PrimaryAssetType", 0xa400a5ce);
            const static AZ::Crc32 DynamicElementType = AZ_CRC("DynamicElementType", 0x7c0b82f9);

            //! Component icon attributes
            const static AZ::Crc32 Icon = AZ_CRC("Icon", 0x659429db);
            const static AZ::Crc32 CategoryStyle = AZ_CRC("CategoryStyle", 0xe503b236);
            const static AZ::Crc32 ViewportIcon = AZ_CRC("ViewportIcon", 0xe7f19a70);
            const static AZ::Crc32 HideIcon = AZ_CRC("HideIcon", 0xfe652ee7);
            const static AZ::Crc32 PreferNoViewportIcon = AZ_CRC("PreferNoViewportIcon", 0x04ae9cb2);
            const static AZ::Crc32 DynamicIconOverride = AZ_CRC("DynamicIconOverride", 0xcc4cea6b);

            //! Data attributes
            const static AZ::Crc32 Min = AZ_CRC("Min", 0xa17b1dd0);
            const static AZ::Crc32 Max = AZ_CRC("Max", 0x9d762289);
            const static AZ::Crc32 ReadOnly = AZ_CRC("ReadOnly", 0x3c5ecbf8);
            const static AZ::Crc32 Step = AZ_CRC("Step", 0x43b9fe3c);
            const static AZ::Crc32 Suffix = AZ_CRC("Suffix", 0xb5b087de);
            const static AZ::Crc32 SoftMin = AZ_CRC("SoftMin", 0x155bd9a3);
            const static AZ::Crc32 SoftMax = AZ_CRC("SoftMax", 0x2956e6fa);
            const static AZ::Crc32 Decimals = AZ_CRC("Decimals", 0x7252f046);
            const static AZ::Crc32 DisplayDecimals = AZ_CRC("DisplayDecimals", 0x31f5f3a0);

            const static AZ::Crc32 LabelForX = AZ_CRC("LabelForX", 0x46dc59ea);
            const static AZ::Crc32 LabelForY = AZ_CRC("LabelForY", 0x31db697c);
            const static AZ::Crc32 LabelForZ = AZ_CRC("LabelForZ", 0xa8d238c6);
            const static AZ::Crc32 LabelForW = AZ_CRC("LabelForW", 0xd663447b);

            const static AZ::Crc32 StyleForX = AZ_CRC("StyleForX", 0x4e381088);
            const static AZ::Crc32 StyleForY = AZ_CRC("StyleForY", 0x393f201e);
            const static AZ::Crc32 StyleForZ = AZ_CRC("StyleForZ", 0xa03671a4);
            const static AZ::Crc32 StyleForW = AZ_CRC("StyleForW", 0xde870d19);
            
            const static AZ::Crc32 RequiredService = AZ_CRC("RequiredService", 0x4d7d0865);
            const static AZ::Crc32 IncompatibleService = AZ_CRC("IncompatibleService", 0x06a52aa9);

            /**
            * Specifies the URL to load for a component
            *
            * **Element type to use this with:**   AZ::Edit::ClassElements::EditorData, which you reflect using
            *                                      AZ::EditContext::ClassInfo::ClassElement().
            *
            * **Expected value type:**             `AZStd::string`
            *
            * **Default value:**                   None
            *
            * **Example:**                         The following example shows how to specify a help URL for a given component
            *
            * @code{.cpp}
            * editContext->Class<ScriptEditorComponent>("Lua Script", "The Lua Script component allows you to add arbitrary Lua logic to an entity in the form of a Lua script")
            *   ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
            *   ->Attribute(AZ::Edit::Attributes::HelpPageURL, "http://docs.aws.amazon.com/lumberyard/latest/userguide/component-lua-script.html")
            * @endcode
            */
            const static AZ::Crc32 HelpPageURL = AZ_CRC("HelpPageURL", 0xa344d681);

            //! Combobox parameters.
            const static AZ::Crc32 ComboBoxEditable = AZ_CRC("ComboBoxEditable", 0x7ee76669);

            /// For use with slice creation tools. See SliceCreationFlags below for details.
            const static AZ::Crc32 SliceFlags = AZ_CRC("SliceFlags", 0xa447e1fb);

            // For optional use on Getter Events used for Virtual Properties
            const static AZ::Crc32 PropertyPosition = AZ_CRC("Position", 0x462ce4f5);
            const static AZ::Crc32 PropertyRotation = AZ_CRC("Rotation", 0x297c98f1);
            const static AZ::Crc32 PropertyScale = AZ_CRC("Scale", 0xec462584);

            // Attribute for storing a Id Generator function used by GenerateNewIdsAndFixRefs to remapping old id's to new id's
            const static AZ::Crc32 IdGeneratorFunction = AZ_CRC("IdGeneratorFunction", 0x4269a3fd);

            // Attribute for tagging a System Component for use in certain contexts
            const static AZ::Crc32 SystemComponentTags = AZ_CRC("SystemComponentTags", 0x2d8bebc9);

            // Attribute for providing a custom UI Handler - can be used with Attribute() (or with ElementAttribute() for containers such as vectors, to specify the handler for container elements (i.e. vectors))
            const static AZ::Crc32 Handler = AZ_CRC("Handler", 0x939715cd);
        }


        namespace UIHandlers
        {
            /**
            * Helper for explicitly designating the default UI handler.
            * i.e. DataElement(DefaultHandler, field, ...)
            */
            const static AZ::Crc32 Default = 0;

            const static AZ::Crc32 Button = AZ_CRC("Button", 0x3a06ac3d);
            const static AZ::Crc32 CheckBox = AZ_CRC("CheckBox", 0x1e7b08ed);
            const static AZ::Crc32 Color = AZ_CRC("Color", 0x665648e9);
            const static AZ::Crc32 ComboBox = AZ_CRC("ComboBox", 0x858d0ae9);
            const static AZ::Crc32 RadioButton = AZ_CRC("RadioButton", 0xcbfd6f3a);
            const static AZ::Crc32 EntityId = AZ_CRC("EntityId", 0x63ac0d5e);
            const static AZ::Crc32 LayoutPadding = AZ_CRC("LayoutPadding", 0x87ffd04b);
            const static AZ::Crc32 LineEdit = AZ_CRC("LineEdit", 0x3f15f4ba);
            const static AZ::Crc32 MultiLineEdit = AZ_CRC("MultiLineEdit", 0xf5d93777);
            const static AZ::Crc32 Quaternion = AZ_CRC("Quaternion", 0x4be832b9);
            const static AZ::Crc32 Slider = AZ_CRC("Slider", 0xcfc71007);
            const static AZ::Crc32 SpinBox = AZ_CRC("SpinBox", 0xf3fd1c2d);
            const static AZ::Crc32 Crc = AZ_CRC("Crc", 0x7c6287fd); // for u32 representation of a CRC, only works on u32 values.
            const static AZ::Crc32 Vector2 = AZ_CRC("Vector2", 0xe6775839);
            const static AZ::Crc32 Vector3 = AZ_CRC("Vector3", 0x917068af);
            const static AZ::Crc32 Vector4 = AZ_CRC("Vector4", 0x0f14fd0c);

            // Maintained in the UIHandlers namespace for backwards compatibility; moved to the Attributes namespace now
            const static AZ::Crc32 Handler = Attributes::Handler;
        }

        // Attributes used in internal implementation only
        namespace InternalAttributes
        {
            const static AZ::Crc32 EnumValue = AZ_CRC("EnumValue", 0xe4f32eed);
        }

        /**
         * Notifies the property system to refresh the property grid, along with the level of refresh.
         */
        namespace PropertyRefreshLevels
        {
            const static AZ::Crc32 None = AZ_CRC("RefreshNone", 0x98a5045b);
            const static AZ::Crc32 ValuesOnly = AZ_CRC("RefreshValues", 0x28e720d4);
            const static AZ::Crc32 AttributesAndValues = AZ_CRC("RefreshAttributesAndValues", 0xcbc2147c);
            const static AZ::Crc32 EntireTree = AZ_CRC("RefreshEntireTree", 0xefbc823c);
        }

        /**
         * Specifies the visiblity setting for a particular property.
         */
        namespace PropertyVisibility
        {
            const static AZ::Crc32 Show = AZ_CRC("PropertyVisibility_Show", 0xa43c82dd);
            const static AZ::Crc32 ShowChildrenOnly = AZ_CRC("PropertyVisibility_ShowChildrenOnly", 0xef428f20);
            const static AZ::Crc32 Hide = AZ_CRC("PropertyVisibility_Hide", 0x32ab90f7);
        }

        namespace SliceFlags
        {
            const static AZ::u32 DontGatherReference        = 1 << 0;   ///< Slice creation should not auto-gather entities referenced through this field.
            const static AZ::u32 NotPushable                = 1 << 1;   ///< This field should not be pushed to slices. Can be bound as a dynamic attribute.
            const static AZ::u32 NotPushableOnSliceRoot     = 1 << 2;   ///< This field is not pushable on slice root entities.
            const static AZ::u32 PushWhenHidden             = 1 << 4;   ///< When not displayed in the Push widget and changes are present, push changes to the slice (useful for hidden editor-only components)
            const static AZ::u32 HideOnAdd                  = 1 << 5;   ///< When property/field/component class is being added to an entity, hide from Push Widget display
            const static AZ::u32 HideOnChange               = 1 << 6;   ///< When property/field/component class is being changed on an entity, hide from Push Widget display
            const static AZ::u32 HideOnRemove               = 1 << 7;   ///< When property/field/component class is being removed on an entity, hide from Push Widget display
        }

        namespace UISliceFlags
        {
            // IMPORANT: Start at the first bit NOT used by SliceFlags above.
            const static AZ::u32 PushableEvenIfInvisible = 1 << 3;  ///< Deprecated - Used currently only by UI slices, and will be phased out in the future. Forces display of hidden fields on Push widget.
        }

    } // namespace Edit

} // namespace AZ
