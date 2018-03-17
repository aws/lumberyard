#pragma once

#include <AzCore/Component/ComponentApplication.h>

namespace ScriptCanvasTests
{
    class Application : public AZ::ComponentApplication
    {
    public:

        // We require an EditContext to be created as early as possible
        // as we rely on EditContext attributes for detecting Graph entry points.
        void CreateReflectionManager() override
        {
            AZ::ComponentApplication::CreateReflectionManager();
            GetSerializeContext()->CreateEditContext();
        }
    };
}

