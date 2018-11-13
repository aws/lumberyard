import os
import resource_manager.util
import datetime
import time
import test_constant
import resource_manager_common.constant
from filelock import Timeout, FileLock


class SharedResourceManager():

    FILE_SHARED_RESOURCES_CONTEXT = 'tmp_last_running_cgf_test_shared_context'

    CONTEXT_REGISTERED_ATTR = 'ListOfRegistered'
    CONTEXT_REGISTERED_GEM = 'EnabledGems'

    PROJECT_LOCK_FILE_NAME = 'tmp_last_running_cgf_test_project_resource'
    PROJECT_CONTEXT_STATE_NAME = 'ProjectStackState'
    PROJECT_CREATOR = 'OriginatingCloudGem'
    PROJECT_CREATOR_PATH = 'OriginatingCloudGemPath'

    DEPLOYMENT_LOCK_FILE_NAME = 'tmp_last_running_cgf_test_deployment_resource'
    DEPLOYMENT_CONTEXT_STATE_NAME = 'DeploymentStackState'
    LAST_MODIFIED_BY = 'LastModifiedBy'
    LOCAL_PROJECT_SETTING_ATTRIBUTE = 'ProjectStackId'

    @property
    def is_registered_for_shared(self):
        return self.is_registered_for_shared

    def __init__(self):
        self.root_dir = None
        self.is_registered_for_shared = False


    def register_for_shared_resource(self, path):
        self.root_dir = path
        self.update_shared_resource_context_attr(self.CONTEXT_REGISTERED_ATTR,
                                                        lambda items: self.__append_registered_to_list(items, path))
        self.is_registered_for_shared = True

    def unregister_for_shared_resource(self, path):
        self.update_shared_resource_context_attr(self.CONTEXT_REGISTERED_ATTR,
                                                        lambda items: self.__remove_registered_to_list(items, path))
        self.is_registered_for_shared = False

    def append_shared_gem(self, gem_name, version, value, path=None):
        item = {"Name": gem_name}
        if version:
            item["Version"] = version
        if path:
            item["Path"] = path

        if value is None or len(value) == 0:
            value = []
            value.append(item)
            return value

        is_found = False
        for entry in value:
            entry_name = entry.get("Name")
            entry_version = entry.get("Version", None)
            entry_path = entry.get("Path", None)
            if entry_name == gem_name and entry_version == version and path == entry_path:
                is_found = True
                break

        if not is_found:
            value.append(item)

        return value

    def lock_path(self, path, append_relative=True):
        if append_relative:
            file_path = os.path.join(os.path.join(self.root_dir, ".."), "{}.txt".format(path))
        else:
            file_path = path
        lock_path = "{}.lock".format(file_path)
        return file_path, lock_path

    def lock_file_and_execute(self, file_name, func):
        file_path, lock_path = self.lock_path(file_name)
        lock_file = FileLock("{}".format(lock_path))
        print "Acquiring lock file for file ", file_path
        with lock_file:
            print "File locked...", file_path, datetime.datetime.utcnow()
            context = resource_manager.util.load_json(file_path, {})
            func(context)
            resource_manager.util.save_json(file_path, context)
            print "File unlocked...", file_path, datetime.datetime.utcnow()

    def update_context_attribute(self, context, name, value):
        context[name]=value

    def remove_json_file_attribute(self, context, file_name, attr_name):
        file_path, lock_path = self.lock_path(file_name)
        print "REMOVING lock", file_path, attr_name, context
        if attr_name in context:
            del context[attr_name]
        return context

    def remove_json_file_attribute_with_lock(self, file_name, attr_name, append_relative=True):
        file_path, lock_path = self.lock_path(file_name, append_relative)
        context = resource_manager.util.load_json(file_path, {})
        lock_file = FileLock("{}".format(lock_path))
        print "Acquiring lock file for file ", file_path
        with lock_file:
            print "REMOVING lock", file_path, attr_name, context
            if attr_name in context:
                del context[attr_name]
            print context
            resource_manager.util.save_json(file_path, context)

    def sync_registered_gems(self, game_dir, enable_gem_func):
        self.update_shared_resource_context_attr(
            SharedResourceManager.CONTEXT_REGISTERED_GEM,
            lambda registered_gems: (
                self.__sync_registered_gems(game_dir, enable_gem_func, registered_gems)
            ))

    def sync_project_settings_file(self, context, file_path_src, file_path_target):
        # sync the local_project_setttings with the process that created the stack
        shared_project_settings = resource_manager.util.load_json(file_path_src, {})        
        resource_manager.util.save_json(file_path_target, shared_project_settings)       

    def remove_project_settings_file(self, path):
        project_settings = resource_manager.util.load_json(path, {})
        set = project_settings[test_constant.DEFAULT][test_constant.SET]
        if self.LOCAL_PROJECT_SETTING_ATTRIBUTE in  project_settings[set]:
            del project_settings[set][self.LOCAL_PROJECT_SETTING_ATTRIBUTE]
        resource_manager.util.save_json(path, project_settings)

    def update_shared_resource_context_attr(self, attr_name, func):
        file_name = self.FILE_SHARED_RESOURCES_CONTEXT
        file_path, lock_path = self.lock_path(file_name)
        lock_file = FileLock("{}".format(lock_path))
        with lock_file:
            context = resource_manager.util.load_json(file_path, {})
            items = context.get(attr_name, [])
            context[attr_name] = func(items)
            resource_manager.util.save_json(file_path, context)
        #provide a small buffer between IO read/writes to help stability
        time.sleep(1)

    def __sync_registered_gems(self, game_dir, enable_gem_func, shared_deployment_gems):
        context_gems = 'Gems'
        path = os.path.join(game_dir, "gems.json")
        gem_settings = resource_manager.util.load_json(path, {})
        for gemA in shared_deployment_gems:
            shared_gem_name = gemA.get('Name', None)
            shared_gem_version = gemA.get('Version', None)
            shared_gem_path = gemA.get('Path', None)
            found = False
            for gemB in gem_settings[context_gems]:
                name = gemB.get('_comment', None)
                gem_path = gemB.get('Path', None)
                version = None
                if gem_path:
                    parts = gem_path.split("/")
                    if len(parts) == 3:
                        version = parts[2]

                if name == shared_gem_name and version == shared_gem_version:
                    found = True
            if not found:
                if shared_gem_path:
                    print "MISSING the  gem named  '", shared_gem_name, "' at path '", shared_gem_path, "'. Adding version '", shared_gem_version, "'"
                else:
                    print "MISSING the  gem named  '", shared_gem_name, "'. Adding version '", shared_gem_version, "'"
                enable_gem_func(shared_gem_name, shared_gem_version, True, shared_gem_path)

        gem_settings[context_gems] = shared_deployment_gems
        return shared_deployment_gems

    def get_attribute(self, file_name, name, default):
        file_path, lock_path = self.lock_path(file_name)
        lock_file = FileLock("{}".format(lock_path))
        with lock_file:
            context = resource_manager.util.load_json(file_path, {})
            return context.get(name, default)

    def __append_registered_to_list(self, items, item):
        if items is None:
            items = []        
        items.append(item)
        return list(set(items))

    def __remove_registered_to_list(self, items, item):        
        if item in items:
            items.remove(item)
        return items
