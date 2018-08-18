#include "StdAfx.h"

#include "PropertyCtrl.h"
#include "PropertyAnimationCtrl.h"
#include "PropertyResourceCtrl.h"
#include "PropertyGenericCtrl.h"
#include "PropertyMiscCtrl.h"
#include "PropertyMotionCtrl.h"

void RegisterReflectedVarHandlers()
{
    static bool registered = false;
    if (!registered)
    {
        registered = true;
        EBUS_EVENT(AzToolsFramework::PropertyTypeRegistrationMessages::Bus, RegisterPropertyType, aznew AnimationPropertyWidgetHandler());
        EBUS_EVENT(AzToolsFramework::PropertyTypeRegistrationMessages::Bus, RegisterPropertyType, aznew FileResourceSelectorWidgetHandler());
        EBUS_EVENT(AzToolsFramework::PropertyTypeRegistrationMessages::Bus, RegisterPropertyType, aznew ShaderPropertyHandler());
        EBUS_EVENT(AzToolsFramework::PropertyTypeRegistrationMessages::Bus, RegisterPropertyType, aznew MaterialPropertyHandler());
        EBUS_EVENT(AzToolsFramework::PropertyTypeRegistrationMessages::Bus, RegisterPropertyType, aznew AIAnchorPropertyHandler());
        EBUS_EVENT(AzToolsFramework::PropertyTypeRegistrationMessages::Bus, RegisterPropertyType, aznew AiBehaviorPropertyHandler());
#ifdef USE_DEPRECATED_AI_CHARACTER_SYSTEM
        EBUS_EVENT(AzToolsFramework::PropertyTypeRegistrationMessages::Bus, RegisterPropertyType, aznew AiCharacterPropertyHandler());
#endif
        EBUS_EVENT(AzToolsFramework::PropertyTypeRegistrationMessages::Bus, RegisterPropertyType, aznew AIPFPropertiesHandler());
        EBUS_EVENT(AzToolsFramework::PropertyTypeRegistrationMessages::Bus, RegisterPropertyType, aznew AIEntityClassesHandler());
        EBUS_EVENT(AzToolsFramework::PropertyTypeRegistrationMessages::Bus, RegisterPropertyType, aznew SOClassPropertyHandler());
        EBUS_EVENT(AzToolsFramework::PropertyTypeRegistrationMessages::Bus, RegisterPropertyType, aznew SOClassesPropertyHandler());
        EBUS_EVENT(AzToolsFramework::PropertyTypeRegistrationMessages::Bus, RegisterPropertyType, aznew SOStatePropertyHandler());
        EBUS_EVENT(AzToolsFramework::PropertyTypeRegistrationMessages::Bus, RegisterPropertyType, aznew SOStatesPropertyHandler());
        EBUS_EVENT(AzToolsFramework::PropertyTypeRegistrationMessages::Bus, RegisterPropertyType, aznew SOStatePatternPropertyHandler());
        EBUS_EVENT(AzToolsFramework::PropertyTypeRegistrationMessages::Bus, RegisterPropertyType, aznew SOActionPropertyHandler());
        EBUS_EVENT(AzToolsFramework::PropertyTypeRegistrationMessages::Bus, RegisterPropertyType, aznew SOHelperPropertyHandler());
        EBUS_EVENT(AzToolsFramework::PropertyTypeRegistrationMessages::Bus, RegisterPropertyType, aznew SONavHelperPropertyHandler());
        EBUS_EVENT(AzToolsFramework::PropertyTypeRegistrationMessages::Bus, RegisterPropertyType, aznew SOAnimHelperPropertyHandler());
        EBUS_EVENT(AzToolsFramework::PropertyTypeRegistrationMessages::Bus, RegisterPropertyType, aznew SOEventPropertyHandler());
        EBUS_EVENT(AzToolsFramework::PropertyTypeRegistrationMessages::Bus, RegisterPropertyType, aznew SOTemplatePropertyHandler());

        EBUS_EVENT(AzToolsFramework::PropertyTypeRegistrationMessages::Bus, RegisterPropertyType, aznew EquipPropertyHandler());
        EBUS_EVENT(AzToolsFramework::PropertyTypeRegistrationMessages::Bus, RegisterPropertyType, aznew ReverbPresetPropertyHandler());
        EBUS_EVENT(AzToolsFramework::PropertyTypeRegistrationMessages::Bus, RegisterPropertyType, aznew CustomActionPropertyHandler());
        EBUS_EVENT(AzToolsFramework::PropertyTypeRegistrationMessages::Bus, RegisterPropertyType, aznew GameTokenPropertyHandler());
        EBUS_EVENT(AzToolsFramework::PropertyTypeRegistrationMessages::Bus, RegisterPropertyType, aznew MissionObjPropertyHandler());
        EBUS_EVENT(AzToolsFramework::PropertyTypeRegistrationMessages::Bus, RegisterPropertyType, aznew SequencePropertyHandler());
        EBUS_EVENT(AzToolsFramework::PropertyTypeRegistrationMessages::Bus, RegisterPropertyType, aznew SequenceIdPropertyHandler());
        EBUS_EVENT(AzToolsFramework::PropertyTypeRegistrationMessages::Bus, RegisterPropertyType, aznew LocalStringPropertyHandler());
        EBUS_EVENT(AzToolsFramework::PropertyTypeRegistrationMessages::Bus, RegisterPropertyType, aznew LightAnimationPropertyHandler());
        EBUS_EVENT(AzToolsFramework::PropertyTypeRegistrationMessages::Bus, RegisterPropertyType, aznew ParticleNamePropertyHandler());
        EBUS_EVENT(AzToolsFramework::PropertyTypeRegistrationMessages::Bus, RegisterPropertyType, aznew UserPopupWidgetHandler());
        EBUS_EVENT(AzToolsFramework::PropertyTypeRegistrationMessages::Bus, RegisterPropertyType, aznew LensFlareHandler());
        EBUS_EVENT(AzToolsFramework::PropertyTypeRegistrationMessages::Bus, RegisterPropertyType, aznew ColorCurveHandler());
        EBUS_EVENT(AzToolsFramework::PropertyTypeRegistrationMessages::Bus, RegisterPropertyType, aznew FloatCurveHandler());
        EBUS_EVENT(AzToolsFramework::PropertyTypeRegistrationMessages::Bus, RegisterPropertyType, aznew MotionPropertyWidgetHandler());
    }
}

