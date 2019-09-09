import boto3
import json

from errors import HandledError

def __filter_logs(log_group_name, filter_pattern):
    cw_client = boto3.client('logs')

    response = cw_client.filter_log_events(
        logGroupName=log_group_name,
        filterPattern=filter_pattern,
        interleaved=True
    )
    events = response['events']
    while response.get('nextToken', None):
        response = cw_client.filter_log_events(
            logGroupName=log_group_name,
            filterPattern=filter_pattern,
            nextToken=response['nextToken'],
            interleaved=True
        )
        events.extend(response['events'])

    sorted_events = sorted(events, key=lambda k: k['timestamp'])
    return sorted_events


def __get_log_group_name(function_name):
    return "/aws/lambda/{}".format(function_name)


def __resolve_logical_name(context, logical_name, deployment):
    resource_group_name = logical_name.split('.')[0]
    function_name = logical_name.split('.')[1]

    rg_id = context.config.get_resource_group_stack_id(
        deployment, resource_group_name)

    resources = context.stack.describe_resources(
        rg_id, recursive=False)
    if not function_name in resources:
        raise HandledError("There is no {} resource in the {} resource group".format(function_name, resource_group_name))

    function_description = resources[function_name]
    if function_description['ResourceType'] != 'AWS::Lambda::Function':
        raise HandledError("The {} resource in the {} resource group is not a lambda function".format(
            function_name, resource_group_name))

    return function_description['PhysicalResourceId']

def __get_logs(context, function_name, request_id, deployment):
    log_group_name = __get_log_group_name(function_name)
    logs = __filter_logs(log_group_name, '"Request ({})"'.format(request_id))
    for l in logs:
        context.view._output_message(l['message'].rstrip())

def get_logs(context, args):
    if args.deployment is None:
        if context.config.default_deployment is None:
            raise HandledError(
                'No deployment was specified and there is no default deployment configured.')
        args.deployment = context.config.default_deployment
    function = __resolve_logical_name(context, args.function, args.deployment)
    __get_logs(context, function, args.request, args.deployment)

