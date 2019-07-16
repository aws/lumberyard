import json
import util
from resource_manager_common import constant

def write_to_project_file(context, pools):
    pools_file = context.config.join_aws_directory_path(
        constant.COGNITO_POOLS_FILENAME)
    existing_pools = util.load_json(
        pools_file, optional=True, default=None)

    if existing_pools is None:
        existing_pools = {}

    changes = __merge_common_groups(pools, existing_pools)
    changes  = changes or __add_new_groups(pools, existing_pools)
    if changes:
        util.validate_writable_list(context, [pools_file])
        with open(pools_file, 'w') as outfile:
            json.dump(existing_pools, outfile, indent=4)

def __add_new_groups(new_pools, old_pools):
    new_groups = set(new_pools.keys()) - set(old_pools.keys())
    for group in new_groups:
        old_pools[group] = new_pools[group]
    return len(new_groups) > 0


def __merge_common_groups(new_pools, old_pools):
    changes = False
    for group in new_pools.keys():
        if not group in old_pools:
            continue
        for resource_name, resource_info in new_pools[group].iteritems():
            if resource_name in old_pools[group]:
                if old_pools[group][resource_name] != resource_info:
                    print "{}:{} is already present, not saving {}".format(group, resource_name, resource_info)
            else:
                old_pools[group][resource_name] = resource_info
                changes = True
    return changes
