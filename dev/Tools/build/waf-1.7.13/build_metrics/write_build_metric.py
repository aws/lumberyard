#
# All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
# its licensors.
#
# For complete copyright and license terms please see the LICENSE at the root of this
# distribution (the "License"). All use of this software is governed by the License,
# or, if provided, by the license below or the license accompanying this file. Do not
# remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
#

# THis file is a simple script to write a build result metric in the proper format using existing metric output code

import build_metrics_reporter
import sys

def main():
    if '--enable-build-metrics' not in sys.argv: #metrics weren't enabled for this build, there's nothing to do
        return
    #write_build_metric metric_name metric_value metric_unit
    if len(sys.argv) > 3:
        metric_name = sys.argv[1]
        metric_value = sys.argv[2]
        metric_unit = sys.argv[3]
        metric_unit_enum = build_metrics_reporter.MetricUnit[metric_unit]

        if not build_metrics_reporter.metric_reporter.parse_command_line(sys.argv[4:]):
            print('--metrics-namespace must be set to write a metric. {0} will not be written'.format(metric_name))
        else:
            build_metrics_reporter.start_metric_reporter()
            build_metrics_reporter.submit_build_metric(metric_name, metric_unit_enum, metric_value, [])
    else:
        print('Too few parameters for write_build_metric.  Expected at least 3 Got: {0}'.format(len(sys.argv )-1))

    build_metrics_reporter.metric_reporter.stop(False, True)

if __name__ == '__main__':
    main()

