import functools
try:
    import simplejson as json
except ImportError:
    import json
import logging
import string

from jsonschema import RefResolver
from jsonschema.validators import Draft4Validator
from pkg_resources import resource_filename
from six import iteritems


from swagger_spec_validator import ref_validators
from swagger_spec_validator.common import load_json
from swagger_spec_validator.common import SwaggerValidationError
from swagger_spec_validator.common import wrap_exception
from swagger_spec_validator.ref_validators import in_scope

log = logging.getLogger(__name__)


def deref(ref_dict, resolver):
    """Dereference ref_dict (if it is indeed a ref) and return what the
    ref points to.

    :param ref_dict: Something like {'$ref': '#/blah/blah'}
    :type ref_dict: dict
    :param resolver: Ref resolver used to do the de-referencing
    :type resolver: :class:`jsonschema.RefResolver`

    :return: de-referenced value of ref_dict
    :rtype: scalar, list, dict
    """
    if ref_dict is None or not is_ref(ref_dict):
        return ref_dict

    ref = ref_dict['$ref']
    with in_scope(resolver, ref_dict):
        with resolver.resolving(ref) as target:
            log.debug('Resolving {0}'.format(ref))
            return target


@wrap_exception
def validate_spec_url(spec_url):
    """Validates a Swagger 2.0 API Specification at the given URL.

    :param spec_url: the URL of the service's swagger spec.

    :returns: The resolver (with cached remote refs) used during validation
    :rtype: :class:`jsonschema.RefResolver`
    :raises: :py:class:`swagger_spec_validator.SwaggerValidationError`
    """
    log.info('Validating %s' % spec_url)
    return validate_spec(load_json(spec_url), spec_url)


def validate_spec(spec_dict, spec_url='', http_handlers=None):
    """Validates a Swagger 2.0 API Specification given a Swagger Spec.

    :param spec_dict: the json dict of the swagger spec.
    :type spec_dict: dict
    :param spec_url: url from which spec_dict was retrieved. Used for
        dereferencing refs. eg: file:///foo/swagger.json
    :type spec_url: string
    :param http_handlers: used to download any remote $refs in spec_dict with
        a custom http client. Defaults to None in which case the default
        http client built into jsonschema's RefResolver is used. This
        is a mapping from uri scheme to a callable that takes a
        uri.

    :returns: the resolver (with cached remote refs) used during validation
    :rtype: :class:`jsonschema.RefResolver`
    :raises: :py:class:`swagger_spec_validator.SwaggerValidationError`
    """
    swagger_resolver = validate_json(
        spec_dict,
        'schemas/v2.0/schema.json',
        spec_url=spec_url,
        http_handlers=http_handlers)

    bound_deref = functools.partial(deref, resolver=swagger_resolver)
    spec_dict = bound_deref(spec_dict)
    apis = bound_deref(spec_dict['paths'])
    definitions = bound_deref(spec_dict.get('definitions', {}))
    validate_apis(apis, bound_deref)
    validate_definitions(definitions, bound_deref)
    return swagger_resolver


@wrap_exception
def validate_json(spec_dict, schema_path, spec_url='', http_handlers=None):
    """Validate a json document against a json schema.

    :param spec_dict: json document in the form of a list or dict.
    :param schema_path: package relative path of the json schema file.
    :param spec_url: base uri to use when creating a
        RefResolver for the passed in spec_dict.
    :param http_handlers: used to download any remote $refs in spec_dict with
        a custom http client. Defaults to None in which case the default
        http client built into jsonschema's RefResolver is used. This
        is a mapping from uri scheme to a callable that takes a
        uri.

    :return: RefResolver for spec_dict with cached remote $refs used during
        validation.
    :rtype: :class:`jsonschema.RefResolver`
    """
    schema_path = resource_filename('swagger_spec_validator', schema_path)
    with open(schema_path) as schema_file:
        schema = json.loads(schema_file.read())

    schema_resolver = RefResolver('file://{0}'.format(schema_path), schema)

    spec_resolver = RefResolver(spec_url, spec_dict,
                                handlers=http_handlers or {})

    ref_validators.validate(
        spec_dict,
        schema,
        resolver=schema_resolver,
        instance_cls=ref_validators.create_dereffing_validator(spec_resolver),
        cls=Draft4Validator)

    # Since remote $refs were downloaded, pass the resolver back to the caller
    # so that its cached $refs can be re-used.
    return spec_resolver


def validate_apis(apis, deref):
    """Validates semantic errors in #/paths.

    :param apis: dict of all the #/paths
    :param deref: callable that dereferences $refs

    :raises: :py:class:`swagger_spec_validator.SwaggerValidationError`
    :raises: :py:class:`jsonschema.exceptions.ValidationError`
    """
    for api_name, api_body in iteritems(apis):
        api_body = deref(api_body)
        api_params = deref(api_body.get('parameters', []))
        validate_duplicate_param(api_params, deref)
        for oper_name in api_body:
            # don't treat parameters that apply to all api operations as
            # an operation
            if oper_name == 'parameters' or oper_name.startswith('x-'):
                continue
            oper_body = deref(api_body[oper_name])
            oper_params = deref(oper_body.get('parameters', []))
            validate_duplicate_param(oper_params, deref)
            all_path_params = list(set(
                get_path_param_names(api_params, deref) +
                get_path_param_names(oper_params, deref)))
            validate_unresolvable_path_params(api_name, all_path_params)


def validate_definitions(definitions, deref):
    """Validates the semantic errors in #/definitions.

    :param definitions: dict of all the definitions
    :param deref: callable that dereferences $refs

    :raises: :py:class:`swagger_spec_validator.SwaggerValidationError`
    :raises: :py:class:`jsonschema.exceptions.ValidationError`
    """
    for def_name, definition in iteritems(definitions):
        definition = deref(definition)
        required = definition.get('required', [])
        props = definition.get('properties', {}).keys()
        extra_props = list(set(required) - set(props))
        if extra_props:
            msg = "Required list has properties not defined"
            raise SwaggerValidationError("%s: %s" % (msg, extra_props))


def get_path_param_names(params, deref):
    """Fetch all the names of the path parameters of an operation.

    :param params: list of all the params
    :param deref: callable that dereferences $refs

    :returns: list of the name of the path params
    """
    return [
        deref(param)['name']
        for param in params
        if deref(param)['in'] == 'path'
    ]


def validate_duplicate_param(params, deref):
    """Validate no duplicate parameters are present.

    Uniqueness is determined by the tuple ('name', 'in').

    :param params: list of all the params
    :param deref: callable that dereferences $refs

    :raises: :py:class:`swagger_spec_validator.SwaggerValidationError` when
        a duplicate parameter is found.
    """
    seen = set()
    msg = "Duplicate param found with (name, in)"
    for param in params:
        param = deref(param)
        param_key = (param['name'], param['in'])
        if param_key in seen:
            raise SwaggerValidationError("%s: %s" % (msg, param_key))
        seen.add(param_key)


def get_path_params_from_url(path):
    """Parse the path parameters from a path string

    :param path: path url to parse for parameters

    :returns: List of path parameter names
    """
    formatter = string.Formatter()
    path_params = [item[1] for item in formatter.parse(path)]
    return filter(None, path_params)


def validate_unresolvable_path_params(path_name, path_params):
    """Validate that every path parameter listed is also defined.

    :param path_name: complete path name as a string.
    :param path_params: Names of all the eligible path parameters

    :raises: :py:class:`swagger_spec_validator.SwaggerValidationError`
    """
    msg = "Path Parameter used is not defined"
    for path in get_path_params_from_url(path_name):
        if path not in path_params:
            raise SwaggerValidationError("%s: %s" % (msg, path))


def is_ref(spec_dict):
    return isinstance(spec_dict, dict) and '$ref' in spec_dict
