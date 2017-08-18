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
# $Revision: #3 $

"""
 Tested with Python 2.7.10 32-bit on Windows 7

   Note:
       To use this script you will need to have the following Python modules installed:
           boto3
           requests
           grequests
   Use pip to install them from the command line

    Note that AWS Device Farm is only available in US West (Oregon) so make sure
    you're logged into that region via the AWS CLI

    Example usage:
      - This will create a project called "MyProject" on the Device Farm, upload the APK and start a run
      appTest.py -n MyProjectName -a SamplesProject.apk -s

"""

import boto3
import traceback
import requests
import grequests
import logging
import time
import argparse
import json
import os
import sys


class DeviceFarmDriver:
    # The seconds to sleep between delete requests for a project when using the --deleteallprojects option to
    # avoid a rate throttling exception from being thrown
    sleep_time_for_project_deletion_seconds = 1

    # The number of seconds to sleep between requests to fetch the status of an upload to prevent a rate
    # throttling exception
    sleep_time_for_upload_status_fetch_seconds = 1

    # The number of seconds to sleep between requests to fetch the status of a run to prevent a rate
    # throttling exception
    sleep_time_for_run_completion_check_seconds = 30

    def __init__(self, project_name):
        self.client = boto3.client("devicefarm")
        self.project_name = project_name

    def start_run(self, path_to_app, duration, test_name):
        """
        Create a project, create a device pool, upload the app, start a run and then exit

        :param path_to_app : string for the path and name for the app to use
        :param duration : number of seconds to run the test
        :param test_name : The name of the test specifications to use from test_specs.json

        :return: True if all operations were successful, else False
        """

        if not os.path.exists(path_to_app):
            logging.error("Cannot find app file:%s" % path_to_app)
            return False

        if not self.__create_project():
            return False

        if not self.__load_test_specs(test_name):
            return False

        if not self.__create_device_pool():
            return False

        if not self.__create_upload(path_to_app):
            return False

        if not self.__send_upload():
            return False

        # Save data in case the run below throws an exception and doesn't initialise properly.
        # We will have enough information stored away to retry a run using --repeatrun if this happens
        self.__save_project_data()

        if not self.__create_run(duration):
            return False

        # The run creation has been successful so save project data again to persist the ARN for the run
        return self.__save_project_data()

    def get_run_status(self):
        """
        Use this to periodically obtain the status of the last run. This will exit as soon as it obtains the status
        and doesn't keep track of the last time it was called. It would a good idea to NOT invoke this rapidly and
        repeatedly from a script to prevent rate throttling exceptions

        :return:(string) Status of the run ('PENDING'|'PENDING_CONCURRENCY'|'PENDING_DEVICE'|'PROCESSING'|
                                           'SCHEDULING'|'PREPARING'|'RUNNING'|'COMPLETED'|'STOPPING')
                                           or None in the event of a failure
        """
        try:
            if self.__load_project():
                # API details:
                # https://boto3.readthedocs.io/en/latest/reference/services/devicefarm.html#DeviceFarm.Client.get_run
                response = self.client.get_run(arn=self.run_arn)
                run = response['run']
                return run['status']
            else:
                logging.error("Unable to find run data for the project. "
                              "Make sure you have started a run before trying to get its status")

        except (AttributeError, IOError):
            logging.exception("Error trying to get the status of a run", exc_info=True)
            return None

    def fetch_artifacts_for_project(self):
        """
        Downloads the artifacts for the given project. Note that it doesn't distinguish by device
        :return:None
        """

        try:
            if self.__load_project():
                # API details:
                # https://boto3.readthedocs.io/en/latest/reference/services/devicefarm.html#DeviceFarm.Client.get_run
                response = self.client.get_run(arn=self.run_arn)
                run = response['run']
                if run['status'] == "COMPLETED":
                    # API details:
                    # https://boto3.readthedocs.io/en/latest/reference/services/devicefarm.html#DeviceFarm.Client.list_artifacts
                    artifact_response = self.client.list_artifacts(arn=self.run_arn, type="FILE")
                    artifacts = artifact_response["artifacts"]
                    self.__download_artifacts_for_project(artifacts)
                else:
                    logging.warning("Run not yet complete - current status:[%s]" % run['status'])

        except (AttributeError, IOError):
            logging.exception("Error fetching artifacts", exc_info=True)

    def fetch_artifacts_by_job(self):
        """
        Downloads the artifacts for the given project. Note that this method DOES distinguish by device and will create
        a directory for each device into which the artifacts will be placed
        :return: None
        """
        try:
            if self.__load_project():
                response = self.client.list_jobs(arn=self.run_arn)
                jobs_list = response['jobs']
                job_info_list = list()

                for job in jobs_list:
                    device = job['device']
                    if job['status'] == "COMPLETED":
                        job_info = dict()

                        job_info['arn'] = job['arn']
                        job_info['device_name'] = device['name']

                        job_info_list.append(job_info)
                    else:
                        logging.warning("Skipping artifact retrieval for [%s] as current status is:[%s]"
                                        % (device['name'], job['status']))

                if len(job_info_list) > 0:
                    self.__download_artifacts_by_job(job_info_list)

        except (AttributeError, IOError):
            logging.exception("Error fetching artifacts", exc_info=True)

    def list_jobs(self):
        try:
            if self.__load_project():
                response = self.client.list_jobs(arn=self.run_arn)
                jobs_list = response['jobs']

                for job in jobs_list:
                    logging.info("Device:[%s] Status:[%s] ARN:[%s}" % (job['name'], job['status'], job['arn']))

        except (AttributeError, IOError):
            logging.exception("Error fetching job information", exc_info=True)

    def delete_current_project(self):
        try:
            # API details:
            # https://boto3.readthedocs.io/en/latest/reference/services/devicefarm.html#DeviceFarm.Client.delete_project
            self.client.delete_project(arn=self.project_arn)
            logging.info("Current project deleted")
        except AttributeError:
            logging.exception("Error trying to delete the current project", exc_info=True)

    def list_all_projects(self):
        project_list = []
        try:
            # API details:
            # https://boto3.readthedocs.io/en/latest/reference/services/devicefarm.html#DeviceFarm.Client.list_projects
            response = self.client.list_projects()
            projects = response["projects"]

            for project in projects:
                entry = dict()
                entry["name"] = project['name']
                entry["arn"] = project['arn']

                project_list.append(entry)

        except AttributeError:
            logging.exception("Error listing all projects", exc_info=True)

        finally:
            return project_list

    def delete_all_projects(self):
        try:
            # API details:
            # https://boto3.readthedocs.io/en/latest/reference/services/devicefarm.html#DeviceFarm.Client.list_projects
            response = self.client.list_projects()
            projects = response["projects"]

            for project in projects:
                logging.warning("Deleting:"+project['name'])
                self.client.delete_project(arn=project['arn'])
                # Sleep N seconds to prevent rate throttle exception
                time.sleep(self.sleep_time_for_project_deletion_seconds)

            logging.info("All projects deleted")

        except AttributeError:
            logging.exception("Error deleting all projects", exc_info=True)

    def repeat_run(self, duration):
        if self.__load_project() is False:
            return False

        if self.__create_run(duration) is False:
            return False

        if self.__save_project_data() is False:
            return False

        return True

    def update_app_for_project(self, path_to_app, duration):
        if self.__load_project() is False:
            return False

        if self.__create_upload(path_to_app) is False:
            return False

        if self.__save_project_data() is False:
            return False

        if self.__send_upload() is False:
            return False

        if self.__create_run(duration) is False:
            return False

        if self.__save_project_data() is False:
            return False

        return True

    def list_devices_for_filter(self, test_name):
        if test_name is None:
            logging.error("No test name specified. Check your test_specs.json file for the names you can use i.e. android_fuzz_test")
            return

        if not self.__load_test_specs(test_name):
            return
        devices = self.__get_filtered_device_list()
        for device in devices:
            logging.info("Name: [%s] OS Version: [%s]" % (device.get('name'), device.get('os')))

    #################################################################################################################
    # Private class methods

    def __create_project(self):
        """
        Creates a project on Device Farm. API details:
        https://boto3.readthedocs.io/en/latest/reference/services/devicefarm.html#DeviceFarm.Client.create_project

        :return: True on success, else False
        """

        try:
            response = self.client.create_project(name=self.project_name)
            project_data = response["project"]
            self.project_arn = project_data["arn"]
            logging.info("Created project %s with ARN:%s" % (self.project_name, self.project_arn))

        except AttributeError:
            logging.exception("Failed to create project:", exc_info=True)
            return False

        return True

    def __load_project(self):
        """
        Loads the data from a project file (JSON format) and parses the contents into class attributes. The name of
        the file is the name of the project which was given when the script was started

        :return: True on success, else False
        """
        if not os.path.isfile(self.__get_data_file_name()):
            logging.error("Data file not found for project:%s. " % self.project_name)
            return False
        else:
            try:
                with open(self.__get_data_file_name()) as data_file:
                    project_data = json.load(data_file)
                    self.project_name = project_data['project_name']
                    self.project_arn = project_data['project_arn']
                    self.platform = project_data['platform'].lower()
                    self.app_type = project_data['appType']
                    if 'device_pool_arn' in project_data:
                        self.device_pool_arn = project_data['device_pool_arn']

                    if 'upload_arn' in project_data:
                        self.upload_arn = project_data['upload_arn']

                    if 'run_arn' in project_data:
                        self.run_arn = project_data['run_arn']

            except (AttributeError, IOError, KeyError):
                logging.exception("Error loading data file", exc_info=True)
                return False

        return True

    def __get_data_file_name(self):
        return self.project_name + ".json"

    def __save_project_data(self):
        """
        Saves class attributes to a project file (JSON format) . The name of
        the file is the name of the project which was given when the script was started

        :return: True on success, else False
        """

        entry = dict()

        entry['project_name'] = self.project_name
        entry['project_arn'] = self.project_arn
        entry['platform'] = self.platform
        entry['appType'] = self.app_type

        try:
            entry['device_pool_arn'] = self.device_pool_arn
        except KeyError:
            pass

        try:
            entry['upload_arn'] = self.upload_arn
        except AttributeError:
            pass

        try:
            entry['run_arn'] = self.run_arn
        except AttributeError:
            pass

        try:
            with open(self.__get_data_file_name(), 'w') as outputFile:
                json.dump(entry, outputFile, indent=4)

        except (IOError, AttributeError, KeyError):
            logging.exception("Unable to save data file", exc_info=True)
            return False

        return True

    def __get_filtered_device_list(self):
        """
        Reads the device_filter.json file (which must be located adjacent to this
        script) and populates a list of candidate devices that have passed the filter

        :return: An empty list on failure or a list of device dicts
        """
        candidate_devices = list()

        try:
            response = self.client.list_devices()
            devices = response["devices"]

            # Read in the device filter file
            if self.device_filter is None:
                logging.exception("No devices specified.")
                exit(-1)

            def passes_name_filter(test_device_name):
                # If no name filter was given, assume the device passes this test
                if len(self.device_filter['name_list']) == 0:
                    return True

                for filter_name in self.device_filter['name_list']:
                    if filter_name.lower() in test_device_name.lower():
                        return True

                return False

            def passes_os_filter(test_os_name):
                # If no OS filter was given, assume the device passes this test
                if len(self.device_filter['os_list']) == 0:
                    return True

                for os_name in self.device_filter['os_list']:
                    if os_name in test_os_name:
                        return True

                return False

            for device in devices:
                if device['platform'].lower() != self.platform:
                    continue
                if passes_name_filter(device['name']) and passes_os_filter(device['os']):
                    candidate_devices.append(device)

        except (AttributeError, IOError, KeyError):
            logging.exception("Unable to create device pool", exc_info=True)

        return candidate_devices

    def __create_device_pool(self):
        """
        Creates a device pool using information from the filter.

        :return: True on success, else False
        """

        candidate_devices = self.__get_filtered_device_list()

        device_rules = list()
        device_arns = ""

        logging.info("Creating device pool with the following devices:")
        # Device ARN's have to be encompassed by a double quote before they can be sent
        # to Device Farm so lets do that now. Also, print out the device name

        for candidate in candidate_devices:
            device_arns += '\"' + str(candidate.get('arn')) + '\",'
            logging.info(candidate.get('name'))

        # Creating list syntax as a string for value of the 'value' key.
        device_arns = device_arns[:-1]
        device_rules.append({'attribute': 'ARN',
                             'operator': 'IN',
                             # We could potentially be sending more than one device ARN so we need to make
                             # a list
                             'value': '[' + device_arns + ']'})

        try:
            # API reference:
            # https://boto3.readthedocs.io/en/latest/reference/services/devicefarm.html#DeviceFarm.Client.create_device_pool
            response = self.client.create_device_pool(projectArn=self.project_arn,
                                                      name="Auto", description="Auto-generated", rules=device_rules)

            device_pool = response['devicePool']
            self.device_pool_arn = device_pool["arn"]
            logging.info("Created Device Pool with ARN:" + self.device_pool_arn)

        except (AttributeError, IOError):
            logging.exception("Unable to create device pool", exc_info=True)
            return False

        return True

    def __load_test_specs(self, test_name):
        """
        Uses device_filter.json to create a list to use as a filter
        :return: A dict for the filter.
        """
        try:
            with open("test_specs.json") as fp:
                filter_data = json.load(fp)
                self.device_filter = filter_data[test_name]['device_filter']
                self.app_type = filter_data[test_name]['appType']
                self.platform = filter_data[test_name]['platform'].lower()

        except KeyError:
            logging.exception("There was an error reading the data for the current test spec [%s]" % test_name,
                              exc_info=True)

        except ValueError:
            logging.exception("There is an error in device_filter.json - "
                              "Use a lint program to check the file conforms to JSON standards", exc_info=True)
        except IOError:
            logging.exception("Cannot read device_filter.json")

        finally:
            return True

    def __create_upload(self, path_to_executable):
        """
        Creates an upload for the current project. Note that this does not actually send the app - that will happen
        in __send_upload()
        :param path_to_executable: the OS specific path to the app to be uploaded
        :return: True on success, else False
        """
        try:
            logging.debug("Creating upload for:" + path_to_executable)
            self.uploadPath = path_to_executable
            # Note that it's very important to set the content type when creating the upload or you will
            # receive an error when sending the payload
            # API reference:
            # https://boto3.readthedocs.io/en/latest/reference/services/devicefarm.html#DeviceFarm.Client.create_upload
            response = self.client.create_upload(projectArn=self.project_arn, name=path_to_executable,
                                                 type=self.app_type.upper(), contentType="application/octet-stream")
            upload = response["upload"]
            logging.debug("Status:" + upload["status"])
            logging.debug(str(upload))
            self.uploadURL = upload['url']
            self.upload_arn = upload['arn']

        except AttributeError:
            logging.exception("Error uploading app", exc_info=True)
            return False

        return True

    def __send_upload(self):
        """
        Uses grequests to send the payload to Device Farm. Once the upload starts, we monitor the progress until
        it has been successfully completed

        :return: True on success, else False
        """
        try:
            app_data = open(self.uploadPath, 'rb')

            headers = {"content-type": "application/octet-stream"}

            output = requests.put(self.uploadURL, data=app_data, allow_redirects=True, headers=headers)

            logging.debug("Sending data:" + str(output))

            self.upload_complete = False

            current_upload_status = ""

            while self.upload_complete is False:
                # API reference:
                # https://boto3.readthedocs.io/en/latest/reference/services/devicefarm.html#DeviceFarm.Client.list_uploads
                response = self.client.list_uploads(arn=self.upload_arn)
                # We only need the status of the first upload so grab it from the list returned in the response
                upload = response["uploads"][0]
                upload_status = upload['status']

                if current_upload_status != upload_status:
                    current_upload_status = upload_status
                    logging.debug("Current upload status:" + current_upload_status)

                if upload_status == u'SUCCEEDED':
                    self.upload_complete = True
                    self.app_arn = upload['arn']

                    logging.debug("Upload ARN:" + self.upload_arn)
                    logging.debug("App ARN:" + self.app_arn)

                # sleep for N seconds to avoid rate throttling exception
                time.sleep(self.sleep_time_for_upload_status_fetch_seconds)
                logging.info("Sending data...")

            logging.info("Upload complete")

        except (AttributeError, KeyError):
            logging.exception("Unable to send upload", exc_info=True)
            logging.debug("Upload URL =" + str(self.uploadURL))
            return False

        except IOError:
            logging.exception("Error with file for upload", exc_info=True)
            logging.debug("Upload URL =" + str(self.uploadURL))
            return False

        return True

    def __create_run(self, duration):
        """
        Creates and starts a run of an app for the given duration

        :param duration: Time in seconds to run the app on a device
        :return: True on success, else False
        """
        try:
            test_parameters = {"eventCount": str(duration), "eventThrottle": "1000"}
            test_settings = {"type": "BUILTIN_FUZZ", "parameters": test_parameters}
            scheduled_run_name = "Auto-generated run for project:%s" % self.project_name
            response = self.client.schedule_run(projectArn=self.project_arn, appArn=self.upload_arn,
                                                devicePoolArn=self.device_pool_arn, name=scheduled_run_name,
                                                test=test_settings)
            run = response['run']
            self.run_arn = run['arn']
            logging.info("Started run on Device Farm...")

        except (AttributeError, KeyError):
            logging.exception("Unable to schedule a run", exc_info=True)
            return False

        return True

    def __wait_for_run_completion(self):
        """
        Periodically checks the last started run and sleeps until the run has been completed

        :return: None
        """
        logging.info("Waiting for run to complete.....")

        try:
            while True:
                time.sleep(self.sleep_time_for_run_completion_check_seconds)
                response = self.client.get_run(arn=self.run_arn)
                if response['status'] == "COMPLETED":
                    logging.info("Run complete")
                    return

                logging.info("Current run status:%s" % response['status'])
        except (AttributeError, KeyError):
            logging.exception("Error while waiting for run to complete", exc_info=True)

    def __download_artifacts_for_project(self, artifact_list):
        """
        Downloads all artifacts for a project irrespective of the device
        :param artifact_list:
        :return: None
        """
        def get_artifact_directory_name():
            return self.project_name + "-artifacts"

        try:
            if not os.path.isdir(get_artifact_directory_name()):
                os.makedirs(get_artifact_directory_name())

            # Artifacts can have the same filename (for instance there can be many files called Logcat.logcat)
            # We will keep track of them and use a dict to store the number downloaded for each filename
            # and append this number to form unique file names before writing them out to the local machine
            files_downloaded = dict()

            for artifact in artifact_list:
                url = artifact['url']
                name = artifact['name']
                ext = artifact['extension']
                filename = os.path.join(get_artifact_directory_name(), name + "." + ext)
                if filename in files_downloaded:
                    index = files_downloaded[filename]
                    files_downloaded[filename] += 1
                    temp_name = "%s.%i.%s" % (name, index, ext)
                    filename = os.path.join(get_artifact_directory_name(), temp_name)
                else:
                    files_downloaded[filename] = 0

                logging.info("Downloading %s " % filename)
                logging.debug("URL=%s" % url)

                self.__download_file(url, filename)

            logging.info("Artifact download complete")

        except (KeyError, AttributeError):
            logging.exception("Error downloading artifacts", exc_info=True)

    def __download_artifacts_by_job(self, info_list):
        """
        Downloads the artifacts for each device and puts it into a directory named the same as the device. We use
        the info list to get the name for each device and then fetch the artifacts via the ARN for the job

        :param info_list: a dict of the job arn and device name
        :return: None
        """

        # Create a directory named after the project if it doesn't already exist
        artifact_directory_name = self.project_name + "-artifacts"

        try:
            if not os.path.isdir(artifact_directory_name):
                os.makedirs(artifact_directory_name)

        except IOError:
            logging.exception("Error downloading artifacts by job", exc_info=True)
            return None

        # Artifacts can have the same filename (for instance there can be many files called Logcat.logcat)
        # We will keep track of them and use a dict to store the number downloaded for each filename
        # and append this number to form unique file names before writing them out to the local machine
        files_downloaded = dict()

        def make_file_path(device_name, target_name, target_ext):
            """
            Create a directory and file path based on the device, name and extension for a file
            :param device_name: String for the device name
            :param target_name: name of the file
            :param target_ext: extension of the file
            :return: the qualified path for the file; files of the same that have already been downloaded
                     have an index added to the name (i.e. Logcat.0.logcat)
            """
            file_path = os.path.join(artifact_directory_name, device_name)
            if not os.path.isdir(file_path):
                os.makedirs(file_path)

            temp_name = "%s.%s" % (target_name, target_ext)
            filename = os.path.join(file_path, temp_name)
            if filename in files_downloaded:
                index = files_downloaded[filename]
                files_downloaded[filename] += 1
                temp_name = "%s.%i.%s" % (target_name, index, target_ext)
                filename = os.path.join(file_path, temp_name)
            else:
                files_downloaded[filename] = 0

            return filename

        for job in info_list:
            logging.debug("ARN:" + job['arn'])
            logging.debug("Device:" + job['device_name'])
            response = self.client.list_artifacts(arn=job['arn'], type="FILE")
            artifact_list = response['artifacts']
            for artifact in artifact_list:
                url = artifact['url']
                name = artifact['name']
                ext = artifact['extension']

                path = make_file_path(job['device_name'], name, ext)
                self.__download_file(url, path)

    def __download_file(self, url, destination):
        """
        Helper function to download a file from a URL
        :param url: URL for the file
        :param destination: path and name for the file to be written
        :return: None
        """
        try:
            req = requests.get(url, stream=True)
            with open(destination, "wb") as fp:
                # Note - chunk sizes > 1024 cause a hang
                for data in req.iter_content(chunk_size=1024):
                    if data is not None:
                        fp.write(data)

        except IOError:
            logging.exception("Error downloading file", exc_info=True)


def main(arguments):
    try:
        driver = DeviceFarmDriver(arguments.projectname)

        if arguments.startrun:
            if not hasattr(arguments, "app"):
                logging.error("An app name must be given when creating a project")
                exit(-1)

            if not hasattr(arguments, 'test_name'):
                logging.error('Must declare a test spec for a test run.')
                exit(-1)

            driver.start_run(arguments.app, arguments.runduration, arguments.test_name)
            return

        if arguments.getrunstatus:
            logging.info("Current Status: %s" % driver.get_run_status())
            return

        if arguments.fetchartifacts:
            driver.fetch_artifacts_for_project()
            return

        if arguments.fetchartifactsbyjob:
            driver.fetch_artifacts_by_job()
            return

        if arguments.deleteallprojects:
            driver.delete_all_projects()
            return

        if arguments.listjobs:
            driver.list_jobs()
            return

        if arguments.repeatrun:
            driver.repeat_run(arguments.runduration)
            return

        if arguments.updateapp is not None:
            driver.update_app_for_project(arguments.updateapp, arguments.runduration)
            return

        if arguments.testfilter is not None:
            driver.list_devices_for_filter(arguments.test_name)

    except AttributeError:
        logging.exception("Error parsing arguments for given operation", exc_info=True)


# This will catch any unhandled exceptions and display a callstack as well as the error
def log_unhandled_exceptions(exception_class, exception_instance, exception_traceback):
    logging.critical('Unhandled exception - please report as a bug')
    logging.critical(''.join(traceback.format_tb(exception_traceback)))
    logging.critical('{0}: {1}'.format(exception_class, exception_instance))

if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument('-d', '--runduration', default=600,
                        help="Number of seconds for the run (default is 600 seconds)")

    parser.add_argument('-v', '--verbose', default=False, action="store_true", help="Logs debug output")

    rgroup = parser.add_argument_group('run', 'Arguments to upload an APK and start a run')
    rgroup.add_argument('-n', '--projectname', help="The name of the project to use or create for a run")
    rgroup.add_argument('-a', '--app', help="The path and name of the app (APK/IPA) to use for a run")
    rgroup.add_argument('-tn', '--test_name', default=None, required=True,
                        help="Name of test specifications desired to run. See test_specs.json")

    pgroup = parser.add_mutually_exclusive_group()
    pgroup.add_argument('-s', '--startrun', default=False, action="store_true",
                        help="Create a project on AWS Device Farm using the given project name and starts a run")

    pgroup.add_argument('-r', '--repeatrun', default=False, action="store_true",
                        help="Re-runs a previously uploaded app")

    pgroup.add_argument('-g', '--getrunstatus', default=False, action="store_true",
                        help="Get the status of a run for a project")

    pgroup.add_argument('-f', '--fetchartifacts', default=False, action="store_true",
                        help="Gets all the artifacts for the last run in a project")

    pgroup.add_argument('-fj', '--fetchartifactsbyjob', default=False, action="store_true",
                        help="Gets all the artifacts by each job (i.e. device) for the last run in a project")

    pgroup.add_argument('-u', '--updateapp', help="Upload a new app and start a new run for the given project")

    pgroup.add_argument('-l', '--listjobs', default=False, action="store_true",
                        help="List all the jobs for the project")

    pgroup.add_argument('-tf', '--testfilter', default=False, action="store_true",
                        help="List the devices that will be used for a test given the current filter settings")

    parser.add_argument('-x', '--deleteallprojects', default=False, action="store_true",
                        help="[USE WITH CAUTION!] Delete all projects on AWS Device Farm")

    args = parser.parse_args()

    logging.getLogger("Device Farm Driver")
    logging.basicConfig(format='%(message)s')
    if args.verbose is True:
        logging.getLogger().setLevel(logging.DEBUG)
    else:
        logging.getLogger().setLevel(logging.INFO)

    sys.excepthook = log_unhandled_exceptions

    # Set the appropriate debug levels for requests and boto3 modules. You can change these to logging.DEBUG
    # if you need to diagnose a problem
    requests_log = logging.getLogger("requests.packages.urllib3")
    requests_log.setLevel(logging.WARNING)

    botocore_logger = logging.getLogger("botocore")
    botocore_logger.setLevel(logging.ERROR)

    main(args)
