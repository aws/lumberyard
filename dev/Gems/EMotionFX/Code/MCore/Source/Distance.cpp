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
#include "Distance.h"


namespace MCore
{
    // convert it into another unit type
    const MCore::Distance& Distance::ConvertTo(EUnitType targetUnitType)
    {
        mDistance = mDistanceMeters * GetConversionFactorFromMeters(targetUnitType);
        mUnitType = targetUnitType;
        return *this;
    }


    // from a given unit type into meters
    double Distance::GetConversionFactorToMeters(EUnitType unitType)
    {
        switch (unitType)
        {
        case UNITTYPE_MILLIMETERS:
            return 0.001;
        case UNITTYPE_CENTIMETERS:
            return 0.01;
        case UNITTYPE_DECIMETERS:
            return 0.1;
        case UNITTYPE_METERS:
            return 1.0;
        case UNITTYPE_KILOMETERS:
            return 1000.0;
        case UNITTYPE_INCHES:
            return 0.0254;
        case UNITTYPE_FEET:
            return 0.3048;
        case UNITTYPE_YARDS:
            return 0.9144;
        case UNITTYPE_MILES:
            return 1609.344;
        default:
            MCORE_ASSERT(false);
            return 1.0;
        }
    }


    // from meters into a given unit type
    double Distance::GetConversionFactorFromMeters(EUnitType unitType)
    {
        switch (unitType)
        {
        case UNITTYPE_MILLIMETERS:
            return 1000.0;
        case UNITTYPE_CENTIMETERS:
            return 100.0;
        case UNITTYPE_DECIMETERS:
            return 10.0;
        case UNITTYPE_METERS:
            return 1.0;
        case UNITTYPE_KILOMETERS:
            return 0.001;
        case UNITTYPE_INCHES:
            return 39.370078740157;
        case UNITTYPE_FEET:
            return 3.2808398950131;
        case UNITTYPE_YARDS:
            return 1.0936132983377;
        case UNITTYPE_MILES:
            return 0.00062137119223733;
        default:
            MCORE_ASSERT(false);
            return 1.0;
        }
    }


    // convert the type into a string
    const char* Distance::UnitTypeToString(EUnitType unitType)
    {
        switch (unitType)
        {
        case UNITTYPE_MILLIMETERS:
            return "millimeters";
        case UNITTYPE_CENTIMETERS:
            return "centimeters";
        case UNITTYPE_DECIMETERS:
            return "decimeters";
        case UNITTYPE_METERS:
            return "meters";
        case UNITTYPE_KILOMETERS:
            return "kilometers";
        case UNITTYPE_INCHES:
            return "inches";
        case UNITTYPE_FEET:
            return "feet";
        case UNITTYPE_YARDS:
            return "yards";
        case UNITTYPE_MILES:
            return "miles";
        default:
            return "unknown";
        }
    }


    // convert a string into a unit type
    bool Distance::StringToUnitType(const MCore::String& str, EUnitType* outUnitType)
    {
        if (str.CheckIfIsEqualNoCase("millimeters") || str.CheckIfIsEqualNoCase("millimeter")   || str.CheckIfIsEqualNoCase("mm")  )
        {
            *outUnitType = UNITTYPE_MILLIMETERS;
            return true;
        }
        if (str.CheckIfIsEqualNoCase("centimeters") || str.CheckIfIsEqualNoCase("centimeter")   || str.CheckIfIsEqualNoCase("cm")  )
        {
            *outUnitType = UNITTYPE_CENTIMETERS;
            return true;
        }
        if (str.CheckIfIsEqualNoCase("meters")      || str.CheckIfIsEqualNoCase("meter")        || str.CheckIfIsEqualNoCase("m")   )
        {
            *outUnitType = UNITTYPE_METERS;
            return true;
        }
        if (str.CheckIfIsEqualNoCase("decimeters")  || str.CheckIfIsEqualNoCase("decimeter")    || str.CheckIfIsEqualNoCase("dm")  )
        {
            *outUnitType = UNITTYPE_DECIMETERS;
            return true;
        }
        if (str.CheckIfIsEqualNoCase("kilometers")  || str.CheckIfIsEqualNoCase("kilometer")    || str.CheckIfIsEqualNoCase("km")  )
        {
            *outUnitType = UNITTYPE_KILOMETERS;
            return true;
        }
        if (str.CheckIfIsEqualNoCase("inches")      || str.CheckIfIsEqualNoCase("inch")         || str.CheckIfIsEqualNoCase("in")  )
        {
            *outUnitType = UNITTYPE_INCHES;
            return true;
        }
        if (str.CheckIfIsEqualNoCase("feet")        || str.CheckIfIsEqualNoCase("foot")         || str.CheckIfIsEqualNoCase("ft")  )
        {
            *outUnitType = UNITTYPE_FEET;
            return true;
        }
        if (str.CheckIfIsEqualNoCase("yards")       || str.CheckIfIsEqualNoCase("yard")         || str.CheckIfIsEqualNoCase("yd")  )
        {
            *outUnitType = UNITTYPE_YARDS;
            return true;
        }
        if (str.CheckIfIsEqualNoCase("miles")       || str.CheckIfIsEqualNoCase("mile")         || str.CheckIfIsEqualNoCase("mi")  )
        {
            *outUnitType = UNITTYPE_MILES;
            return true;
        }
        return false;
    }


    // update the distance in meters
    void Distance::UpdateDistanceMeters()
    {
        mDistanceMeters = mDistance * GetConversionFactorToMeters(mUnitType);
    }


    // get the conversion factor between two unit types
    double Distance::GetConversionFactor(EUnitType sourceType, EUnitType targetType)
    {
        Distance dist(1.0, sourceType);
        dist.ConvertTo(targetType);
        return dist.GetDistance();
    }


    // convert a singnle value quickly
    double Distance::ConvertValue(float value, EUnitType sourceType, EUnitType targetType)
    {
        return Distance(value, sourceType).ConvertTo(targetType).GetDistance();
    }
}   // namespace MCore
