#pragma once

#include <AzCore/Preprocessor/CodeGen.h>

#include "Enums.generated.h"

namespace Enums
{
    enum CStyleEnumTest
    {
        CSE_Value1 = 42,
        CSE_Value2 = 96,
        CSE_Value3 = 1024,
    };
    AZCG_Enum(Enums::CStyleEnumTest,
        AzEnum::Uuid("{5ADC5AC9-2CCA-4FA6-A7F3-AEF54398C1F2}"),
        AzEnum::Name("C-Style Enum Test"),
        AzEnum::Description("Enum for testing traditional C-style enumerations"),
        AzEnum::Values({
            AzEnum::Value(CStyleEnumTest::CSE_Value1, "Value1"),
            AzEnum::Value(CStyleEnumTest::CSE_Value2, "Value2"),
            AzEnum::Value(CStyleEnumTest::CSE_Value3, "Value3")
        })
    );

    enum class LazyEnum
    {
        Value_A,
        Value_B,
        Value_C,
    };
    AZCG_Enum(Enums::LazyEnum, AzEnum::Uuid("{01301A3A-802C-462E-BE98-E65821E4EAA2}"));
}

enum class EnumClassTest
{
    ValueA,
    ValueB,
    ValueC,
    Count,
};
AZCG_Enum(EnumClassTest,
    AzEnum::Uuid("{8ACF5186-5965-4121-B28C-1523527A7635}"),
    AzEnum::Values({
        AzEnum::Value(EnumClassTest::ValueA, "ValueA"),
        AzEnum::Value(EnumClassTest::ValueB, "ValueB"),
        AzEnum::Value(EnumClassTest::ValueC, "ValueC")
    })
);

#include "Enums.generated.inline"
