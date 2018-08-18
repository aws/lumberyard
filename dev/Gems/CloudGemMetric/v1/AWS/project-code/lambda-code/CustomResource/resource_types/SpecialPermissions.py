from cgf_utils import custom_resource_response

def handler(event, context):
    return custom_resource_response.success_response({}, "*")
