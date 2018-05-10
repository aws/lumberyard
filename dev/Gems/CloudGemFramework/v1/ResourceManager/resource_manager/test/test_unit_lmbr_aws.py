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

import platform
import os
import subprocess

from time import sleep

def get_lymetrics_library_path():
    # assumes this file is ...\dev\Gems\CloudGemFramework\v?\ResourceManager\resource_manager\metrics.py
    # we want ...\dev\Tools\InternalSDKs\LyMetrics\bin
    path = os.path.abspath(os.path.join(__file__, '..', '..', '..', '..', '..', '..', '..'))
    path = os.path.join(path, 'Tools', 'InternalSDKs', 'LyMetrics', 'bin')
    if platform.system() == "Windows":
        path = os.path.join(path, 'windows', 'intel64')
        for build in ['Release', 'Debug']:
            for vs in ['vs2017', 'vs2015', 'vs2013']:
                dll = os.path.join(path, vs, build, 'LyMetricsProducer_python.dll')
                if os.path.exists(dll):
                    return dll
    return None

lymetrics_library_path = get_lymetrics_library_path()
if lymetrics_library_path is None:
    raise RuntimeError('No lymetrics library was found.')

print '***********************************'
print os.environ
print '***********************************'

# Put metrics into offline mode so we can verify the expected output.
# This has to happen before the AWSResourceManager.metrics module is
# initialized... which seems to happen before this code is even run in
# some cases, so we also set this in the batch files that launch the 
# tests.
os.environ["LYMETRICS"] = "TEST" 

print os.environ
print '***********************************'

import resource_manager.metrics
print '****** loaded dll', resource_manager.metrics.lymetrics_dll

import lmbr_aws_test_support

class UnitTest_CloudGemFramework_ResourceManager(lmbr_aws_test_support.lmbr_aws_TestCase):

    def setUp(self):        
        self.prepare_test_envionment("test_unit_lmbr_aws")
        
    def test_unit_lmbr_aws_end_to_end(self):
        self.run_all_tests()

    def __100_verify_lmbr_aws_works_and_metrics_are_generated(self):

        # Setting the LYMETRICS environment variable to TEST above puts 
        # the metrics system into offline mode, which causes events to 
        # be written to files in the user's LyMetricsCache directory 
        # instead of being posted.

        metrics_dir = self.get_metrics_directory_path()

        print 'Deleting all files from {}'.format(metrics_dir)
        if os.path.exists(metrics_dir):
            self.delete_all_files(metrics_dir)
        
        self.lmbr_aws('resource-group', 'list')

        print 'Waiting for background metrics thread...'
        sleep(5)

        print 'Looking for metrics files in {}'.format(metrics_dir)
        self.assertEquals(2, self.count_files_with_text(metrics_dir, 'ResourceManagement_Command'))
        self.assertEquals(2, self.count_files_with_text(metrics_dir, 'ResourceGroupList'))
        self.assertEquals(1, self.count_files_with_text(metrics_dir, 'Success'))
        self.assertEquals(1, self.count_files_with_text(metrics_dir, 'Attempt'))


    def get_metrics_directory_path(self):

        base_metrics_path = os.environ.get('USERPROFILE')
        if not base_metrics_path:
            print 'No USERPROFILE environment variable set, checking HOME'
            base_metrics_path = os.environ.get('HOME')
            if not base_metrics_path:
                print 'No HOME environment variable set, expanding userprofile'
                base_metrics_path = os.path.expanduser('~')
                if not base_metrics_path:
                    print 'Failed to get path from os.path.expanduser(\'~\')'

        return os.path.join(base_metrics_path, 'LyMetricsCache')


    def delete_all_files(self, directory_path):
        if not os.path.isdir(directory_path):
            print 'Could not clean up invalid directory {}'.format(directory_path)
            return

        for file_path in os.listdir(directory_path):
            os.remove(os.path.join(directory_path, file_path))


    def count_files_with_text(self, directory_path, text):
        count = 0

        if not os.path.isdir(directory_path):
            print 'Could not count files in invalid directory {}'.format(directory_path)
            return count

        for file_path in os.listdir(directory_path):
            with open(os.path.join(directory_path, file_path)) as file:
                file_contents = file.read()
                if text in file_contents:
                    count = count + 1
        return count

