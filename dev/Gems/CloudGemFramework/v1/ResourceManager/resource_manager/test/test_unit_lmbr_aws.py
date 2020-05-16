#
# All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
# its licensors.
#
# For complete copyright and license terms please see the LICENSE at the root of this
# distribution (the 'License'). All use of this software is governed by the License,
# or, if provided, by the license below or the license accompanying this file. Do not
# remove or modify any license notices. This file is distributed on an 'AS IS' BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
#
# $Revision: #16 $
import os
import pprint
from time import sleep

import resource_manager.metrics
from . import lmbr_aws_test_support

# Put metrics into offline mode so we can verify the expected output.
# This has to happen before loading the the lymetrics dll cached in the ResourceManager.metrics context
os.environ["LYMETRICS"] = "TEST"

env_var = os.environ
pprint.pprint(dict(env_var), width=1)
print('***********************************')

# Note: Tests run one path lower than standard code so need to override path for testing
path_override = os.path.abspath(os.path.join(__file__, '..', '..', '..', '..', '..', '..', '..'))
lymetrics_library_path = resource_manager.metrics.get_lymetrics_library(path_override=path_override)
if lymetrics_library_path is None:
    raise RuntimeError('No lymetrics library was found.')
else:
    print('****** loaded dll {}'.format(resource_manager.metrics.lymetrics_dll))


class UnitTest_CloudGemFramework_ResourceManager_Metrics(lmbr_aws_test_support.lmbr_aws_TestCase):

    def setUp(self):        
        self.prepare_test_environment("test_unit_lmbr_aws")
        
    def test_unit_lmbr_aws_end_to_end(self):
        self.run_all_tests()

    def __100_verify_lmbr_aws_works_and_metrics_are_generated(self):
        # Setting the LYMETRICS environment variable to TEST above puts 
        # the metrics system into offline mode, which causes events to 
        # be written to files in the user's LyMetricsCache directory 
        # instead of being posted.
        metrics_dir = self.get_metrics_directory_path()

        print('Deleting all files from {}'.format(metrics_dir))
        if os.path.exists(metrics_dir):
            self.delete_all_files(metrics_dir)
        
        self.lmbr_aws('resource-group', 'list')

        print('Waiting for background metrics thread to output metrics to {}'.format(metrics_dir))
        sleep(5)

        print('Looking for metrics files in {}'.format(metrics_dir))
        self.assertEqual(2, self.count_files_with_text(metrics_dir, 'ResourceManagement_Command'))
        self.assertEqual(2, self.count_files_with_text(metrics_dir, 'ResourceGroupList'))
        self.assertEqual(1, self.count_files_with_text(metrics_dir, 'Success'))
        self.assertEqual(1, self.count_files_with_text(metrics_dir, 'Attempt'))

    def get_metrics_directory_path(self):
        base_metrics_path = os.environ.get('USERPROFILE')
        if not base_metrics_path:
            print('No USERPROFILE environment variable set, checking HOME')
            base_metrics_path = os.environ.get('HOME')
            if not base_metrics_path:
                print('No HOME environment variable set, expanding userprofile')
                base_metrics_path = os.path.expanduser('~')
                if not base_metrics_path:
                    print('Failed to get path from os.path.expanduser(\'~\')')

        return os.path.join(base_metrics_path, 'LyMetricsCache')

    def delete_all_files(self, directory_path):
        if not os.path.isdir(directory_path):
            print('Could not clean up invalid directory {}'.format(directory_path))
            return

        for file_path in os.listdir(directory_path):
            os.remove(os.path.join(directory_path, file_path))

    def count_files_with_text(self, directory_path, text):
        count = 0

        if not os.path.isdir(directory_path):
            print('Could not count files in invalid directory {}'.format(directory_path))
            return count

        for file_path in os.listdir(directory_path):
            with open(os.path.join(directory_path, file_path)) as file:
                file_contents = file.read()
                if text in file_contents:
                    count = count + 1
        return count

