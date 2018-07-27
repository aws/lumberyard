import copy
import enum
import json


_failed_task_types = {
    'ActivityTaskTimedOut': "activityTaskTimedOutEventAttributes",
    'ActivityTaskFailed': "activityTaskFailedEventAttributes"
}


class TaskState(enum.IntEnum):
    INITIAL = 0,
    RUNNING = 1,
    COMPLETED = 2


class ProcessingNode(object):
    def __init__(self, parent, data):
        self.divide_task_ids = []
        self.divide_inputs = []
        self.divide_paths = []
        self.merge_task_id = None
        self.merge_input = None
        self.merge_path = None
        self.merge_dependency_count = 0
        self.parent = parent

        for k, v in data.items():
            setattr(self, k, v)


class Task(object):
    def __init__(self, owning_node, task_type, task_path, task_input):
        self.owning_node = owning_node
        self.task_type = task_type
        self.task_input = task_input
        self.task_path = task_path
        self.child_input = []
        self.state = TaskState.INITIAL


def run_decider(config):
    while True:
        response = config.swf_client.poll_for_decision_task(
            domain=config.domain,
            taskList={'name': config.task_list_name},
            maximumPageSize=1000,
            identity=config.identity
        )

        if 'taskToken' not in response:
            continue

        # Every time we successfully poll for a new decision task we must reconstruct the entire dependency state from
        # the history of events leading up to now.
        all_tasks = {}
        all_nodes = {}
        activity_id_lookup = {}
        leaf_nodes = set()
        ready_merge_nodes = set()
        open_task_count = 0
        task_counts_by_type = {task.name: [0, 0] for task in (config.div_task, config.merge_task, config.build_task)}
        failed_tasks = []

        while True:
            for event in response['events']:
                event_type = event['eventType']

                if event_type == "ActivityTaskScheduled":
                    # A scheduled task means that the task it refers to is in the RUNNING state.
                    event_id = event['eventId']
                    attrs = event['activityTaskScheduledEventAttributes']
                    activity_id = attrs['activityId']
                    activity_id_lookup[event_id] = activity_id
                    task = all_tasks[activity_id]
                    task.state = TaskState.RUNNING
                    if task.task_type == config.merge_task:
                        # Don't run merge nodes that have already been scheduled
                        ready_merge_nodes.discard(task.owning_node)

                    # Track how many tasks are running so we know when we're done
                    open_task_count += 1
                    task_counts_by_type[task.task_type.name][1] += 1

                elif event_type == "ActivityTaskCompleted":
                    # A completed task means that the task it refers to is in the COMPLETED state.
                    # Based on the type of task and the output, we may need to spawn new tasks.
                    scheduled_event_id = event['activityTaskCompletedEventAttributes']['scheduledEventId']
                    activity_id = activity_id_lookup[scheduled_event_id]
                    task = all_tasks[activity_id]
                    task.state = TaskState.COMPLETED
                    data = json.loads(event['activityTaskCompletedEventAttributes']['result'])

                    # Track how many tasks are running so we know when we're done
                    open_task_count -= 1
                    task_counts_by_type[task.task_type.name][0] += 1

                    if task.task_type == config.div_task:
                        # Divide tasks produce a merge task and the child tasks for the task that was divided.
                        # Create a ProcessingNode to represent these combined tasks.
                        node = ProcessingNode(task.owning_node, data)
                        all_nodes[activity_id] = node
                        leaf_nodes.add(activity_id)
                        leaf_nodes.discard(node.parent)

                        # Create the merge task that will combine the merge results of further subdivisions
                        merge_task = config.merge_task if len(node.divide_task_ids) > 0 else config.build_task
                        all_tasks[node.merge_task_id] = Task(
                            owning_node=activity_id,
                            task_type=merge_task,
                            task_path=node.merge_path,
                            task_input=node.merge_input
                        )

                        # Create any children tasks that resulted from this division
                        for child_id, child_path, child_input in zip(node.divide_task_ids, node.divide_paths, node.divide_inputs):
                            all_tasks[child_id] = Task(
                                owning_node=activity_id,
                                task_type=config.div_task,
                                task_path=node.merge_path + [child_path],
                                task_input=child_input
                            )

                    else:
                        # Build and merge tasks need to inform their parent ProcessingTask of their completion, as well
                        # as feed their output to the parent's input.
                        node_id = task.owning_node
                        node = all_nodes[node_id]

                        if node.parent:
                            parent_node = all_nodes[node.parent]
                            parent_merge_task = all_tasks[parent_node.merge_task_id]
                            parent_merge_task.child_input.append(data)
                            parent_node.merge_dependency_count += 1
                            if parent_node.merge_dependency_count == len(parent_node.divide_task_ids):
                                # This parent has all of its dependencies satisifed, so mark it as ready to run.
                                # (This will be cancelled if later events reveal that it has already been scheduled.)
                                ready_merge_nodes.add(node.parent)

                elif event_type == "WorkflowExecutionStarted":
                    # The initial task is a divide task with empty input.
                    all_tasks['begin'] = Task(
                        owning_node=None,
                        task_type=config.div_task,
                        task_path=[],
                        task_input=json.loads(event['workflowExecutionStartedEventAttributes']['input'])
                    )

                else:
                    failed_attribute_key = _failed_task_types.get(event_type, None)

                    if failed_attribute_key:
                        # A failed or timed-out task needs to be rescheduled.
                        scheduled_event_id = event[failed_attribute_key]['scheduledEventId']
                        activity_id = activity_id_lookup[scheduled_event_id]
                        task = all_tasks[activity_id]
                        task.state = TaskState.INITIAL
                        failed_tasks.append([activity_id, event['eventTimestamp'].isoformat(timespec="seconds")])

            # Keep paging through the entire task history
            if 'nextPageToken' in response:
                response = config.swf_client.poll_for_decision_task(
                    domain=config.domain,
                    taskList={'name': config.task_list_name},
                    maximumPageSize=1000,
                    identity=config.identity,
                    nextPageToken=response['nextPageToken']
                )
            else:
                break

        # Now that state has been reconstructed, determine what tasks we need to run and generate decisions for them.
        run_tasks = []

        # Process any leaf nodes that exist.
        #
        # There will always be leaf nodes once the initial subdivision has occurred. The only time there are no leaf
        # nodes is when the decider is running for the first time and no activities have been dispatched yet.
        #
        # A leaf node won't necessarily remain a leaf node later on in the workflow. It may have division tasks that
        # are waiting to be run, and once those run they will create new leaf nodes beneath the current one.
        if len(leaf_nodes):
            for leaf_id in leaf_nodes:
                leaf_node = all_nodes[leaf_id]
                if len(leaf_node.divide_task_ids):
                    # "Divide" tasks in leaf nodes that are still in the INITIAL state should be run.
                    for divide_id in leaf_node.divide_task_ids:
                        divide_task = all_tasks[divide_id]
                        if divide_task.state == TaskState.INITIAL:
                            run_tasks.append((divide_id, divide_task))
                else:
                    # Special case: if there are no divide tasks in a leaf node then that means it is a solitary task
                    # at the lowest level of subdivision. Such tasks in the INITIAL state should be run.
                    leaf_task = all_tasks[leaf_node.merge_task_id]
                    if leaf_task.state == TaskState.INITIAL:
                        run_tasks.append((leaf_node.merge_task_id, leaf_task))

            # Any "merge" nodes that were marked as ready to run should also be run.
            for merge_id in ready_merge_nodes:
                merge_node = all_nodes[merge_id]
                merge_task_id = merge_node.merge_task_id
                run_tasks.append((merge_task_id, all_tasks[merge_task_id]))

        elif all_tasks['begin'].state == TaskState.INITIAL:
            # Run the start-up task if it hasn't been run yet.
            run_tasks.append(("begin", all_tasks['begin']))

        # Generate decisions for the tasks we need to run.
        decisions = []
        decision_prototype = {
            'decisionType': "ScheduleActivityTask",
            'scheduleActivityTaskDecisionAttributes': {
                'activityType': None,
                'scheduleToCloseTimeout': 'NONE',
                'scheduleToStartTimeout': 'NONE',
                'startToCloseTimeout': 'NONE',
                'heartbeatTimeout': '300',
                'taskList': {
                    'name': config.task_list_name
                }
            }
        }

        if len(run_tasks):
            for task_id, task in run_tasks:
                decision = copy.deepcopy(decision_prototype)
                attrs = decision['scheduleActivityTaskDecisionAttributes']
                attrs['activityType'] = task.task_type.as_dict
                attrs['activityId'] = task_id
                attrs['input'] = json.dumps({'main': task.task_input, 'path': task.task_path, 'merge': task.child_input})
                decisions.append(decision)
        elif open_task_count == 0:
            # If no more tasks are running, then we're done the workflow.
            decision = {
                'decisionType': 'CompleteWorkflowExecution',
                'completeWorkflowExecutionDecisionAttributes': {
                    'result': ""
                }
            }
            decisions.append(decision)

            # Do we need to shut ourselves down?
            if config.config_bucket:
                try:
                    s3_response = config.s3_client.get_object(
                        Bucket=config.config_bucket,
                        Key="fleet_configuration.json"
                    )

                    fleet_config = json.load(s3_response['Body'])
                    if fleet_config.get('automaticallyTakeDown', False):
                        autoscale_group_name = fleet_config.get('autoScalingGroupName', None)
                        if autoscale_group_name:
                            autoscale_client = config.session.client("autoscaling", region_name=config.region)
                            autoscale_response = autoscale_client.update_auto_scaling_group(
                                AutoScalingGroupName=autoscale_group_name,
                                MinSize=0,
                                DesiredCapacity=0
                            )
                            print(autoscale_response)

                        else:
                            raise RuntimeError("No 'autoScalingGroupName' in fleet_config.json")

                except Exception as e:
                    print("An error occurred checking the shutdown status: %s" % str(e))

        # Update the overall progress in the workflow's execution context
        execution_context = json.dumps({
            'progress': task_counts_by_type,
            'failed': failed_tasks
        })

        # Respond to SWF with the decisions
        config.swf_client.respond_decision_task_completed(
            taskToken=response['taskToken'],
            decisions=decisions,
            executionContext=execution_context
        )
