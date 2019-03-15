#pragma once

#include <AzCore/std/containers/unordered_map.h>
#include <AzCore/std/string/string.h>
#include <QMessageBox>

namespace AzToolsFramework
{
    namespace Layers
    {
        class NameConflictWarning
            : public QMessageBox
        {
        public:
            NameConflictWarning(QWidget* parent, const AZStd::unordered_map<AZStd::string, int>& nameConflictMapping);

        private:
            const QString replacementStr = " > ";
        };
    }
}
