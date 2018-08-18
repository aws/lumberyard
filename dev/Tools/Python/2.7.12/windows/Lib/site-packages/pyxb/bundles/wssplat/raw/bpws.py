# ./pyxb/bundles/wssplat/raw/bpws.py
# -*- coding: utf-8 -*-
# PyXB bindings for NM:157fc4d84d91f9f337276fbd45fc8a0cd667f150
# Generated 2014-10-19 06:24:55.831963 by PyXB version 1.2.4 using Python 2.7.3.final.0
# Namespace http://schemas.xmlsoap.org/ws/2003/03/business-process/

from __future__ import unicode_literals
import pyxb
import pyxb.binding
import pyxb.binding.saxer
import io
import pyxb.utils.utility
import pyxb.utils.domutils
import sys
import pyxb.utils.six as _six

# Unique identifier for bindings created at the same time
_GenerationUID = pyxb.utils.utility.UniqueIdentifier('urn:uuid:8d5f5634-5782-11e4-bd00-c8600024e903')

# Version of PyXB used to generate the bindings
_PyXBVersion = '1.2.4'
# Generated bindings are not compatible across PyXB versions
if pyxb.__version__ != _PyXBVersion:
    raise pyxb.PyXBVersionError(_PyXBVersion)

# Import bindings for namespaces imported into schema
import pyxb.binding.datatypes

# NOTE: All namespace declarations are reserved within the binding
Namespace = pyxb.namespace.NamespaceForURI('http://schemas.xmlsoap.org/ws/2003/03/business-process/', create_if_missing=True)
Namespace.configureCategories(['typeBinding', 'elementBinding'])

def CreateFromDocument (xml_text, default_namespace=None, location_base=None):
    """Parse the given XML and use the document element to create a
    Python instance.

    @param xml_text An XML document.  This should be data (Python 2
    str or Python 3 bytes), or a text (Python 2 unicode or Python 3
    str) in the L{pyxb._InputEncoding} encoding.

    @keyword default_namespace The L{pyxb.Namespace} instance to use as the
    default namespace where there is no default namespace in scope.
    If unspecified or C{None}, the namespace of the module containing
    this function will be used.

    @keyword location_base: An object to be recorded as the base of all
    L{pyxb.utils.utility.Location} instances associated with events and
    objects handled by the parser.  You might pass the URI from which
    the document was obtained.
    """

    if pyxb.XMLStyle_saxer != pyxb._XMLStyle:
        dom = pyxb.utils.domutils.StringToDOM(xml_text)
        return CreateFromDOM(dom.documentElement, default_namespace=default_namespace)
    if default_namespace is None:
        default_namespace = Namespace.fallbackNamespace()
    saxer = pyxb.binding.saxer.make_parser(fallback_namespace=default_namespace, location_base=location_base)
    handler = saxer.getContentHandler()
    xmld = xml_text
    if isinstance(xmld, _six.text_type):
        xmld = xmld.encode(pyxb._InputEncoding)
    saxer.parse(io.BytesIO(xmld))
    instance = handler.rootObject()
    return instance

def CreateFromDOM (node, default_namespace=None):
    """Create a Python instance from the given DOM node.
    The node tag must correspond to an element declaration in this module.

    @deprecated: Forcing use of DOM interface is unnecessary; use L{CreateFromDocument}."""
    if default_namespace is None:
        default_namespace = Namespace.fallbackNamespace()
    return pyxb.binding.basis.element.AnyCreateFromDOM(node, default_namespace)


# List simple type: [anonymous]
# superclasses pyxb.binding.datatypes.anySimpleType
class STD_ANON (pyxb.binding.basis.STD_list):

    """Simple type that is a list of pyxb.binding.datatypes.QName."""

    _ExpandedName = None
    _XSDLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 276, 20)
    _Documentation = None

    _ItemType = pyxb.binding.datatypes.QName
STD_ANON._InitializeFacetMap()

# Atomic simple type: [anonymous]
class STD_ANON_ (pyxb.binding.datatypes.string, pyxb.binding.basis.enumeration_mixin):

    """An atomic simple type."""

    _ExpandedName = None
    _XSDLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 370, 20)
    _Documentation = None
STD_ANON_._CF_enumeration = pyxb.binding.facets.CF_enumeration(value_datatype=STD_ANON_, enum_prefix=None)
STD_ANON_.in_ = STD_ANON_._CF_enumeration.addEnumeration(unicode_value='in', tag='in_')
STD_ANON_.out = STD_ANON_._CF_enumeration.addEnumeration(unicode_value='out', tag='out')
STD_ANON_.out_in = STD_ANON_._CF_enumeration.addEnumeration(unicode_value='out-in', tag='out_in')
STD_ANON_._InitializeFacetMap(STD_ANON_._CF_enumeration)

# Atomic simple type: {http://schemas.xmlsoap.org/ws/2003/03/business-process/}tBoolean-expr
class tBoolean_expr (pyxb.binding.datatypes.string):

    """An atomic simple type."""

    _ExpandedName = pyxb.namespace.ExpandedName(Namespace, 'tBoolean-expr')
    _XSDLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 654, 4)
    _Documentation = None
tBoolean_expr._InitializeFacetMap()
Namespace.addCategoryObject('typeBinding', 'tBoolean-expr', tBoolean_expr)

# Atomic simple type: {http://schemas.xmlsoap.org/ws/2003/03/business-process/}tDuration-expr
class tDuration_expr (pyxb.binding.datatypes.string):

    """An atomic simple type."""

    _ExpandedName = pyxb.namespace.ExpandedName(Namespace, 'tDuration-expr')
    _XSDLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 658, 4)
    _Documentation = None
tDuration_expr._InitializeFacetMap()
Namespace.addCategoryObject('typeBinding', 'tDuration-expr', tDuration_expr)

# Atomic simple type: {http://schemas.xmlsoap.org/ws/2003/03/business-process/}tDeadline-expr
class tDeadline_expr (pyxb.binding.datatypes.string):

    """An atomic simple type."""

    _ExpandedName = pyxb.namespace.ExpandedName(Namespace, 'tDeadline-expr')
    _XSDLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 662, 4)
    _Documentation = None
tDeadline_expr._InitializeFacetMap()
Namespace.addCategoryObject('typeBinding', 'tDeadline-expr', tDeadline_expr)

# Atomic simple type: {http://schemas.xmlsoap.org/ws/2003/03/business-process/}tBoolean
class tBoolean (pyxb.binding.datatypes.string, pyxb.binding.basis.enumeration_mixin):

    """An atomic simple type."""

    _ExpandedName = pyxb.namespace.ExpandedName(Namespace, 'tBoolean')
    _XSDLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 666, 4)
    _Documentation = None
tBoolean._CF_enumeration = pyxb.binding.facets.CF_enumeration(value_datatype=tBoolean, enum_prefix=None)
tBoolean.yes = tBoolean._CF_enumeration.addEnumeration(unicode_value='yes', tag='yes')
tBoolean.no = tBoolean._CF_enumeration.addEnumeration(unicode_value='no', tag='no')
tBoolean._InitializeFacetMap(tBoolean._CF_enumeration)
Namespace.addCategoryObject('typeBinding', 'tBoolean', tBoolean)

# Atomic simple type: {http://schemas.xmlsoap.org/ws/2003/03/business-process/}tRoles
class tRoles (pyxb.binding.datatypes.string, pyxb.binding.basis.enumeration_mixin):

    """An atomic simple type."""

    _ExpandedName = pyxb.namespace.ExpandedName(Namespace, 'tRoles')
    _XSDLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 673, 4)
    _Documentation = None
tRoles._CF_enumeration = pyxb.binding.facets.CF_enumeration(value_datatype=tRoles, enum_prefix=None)
tRoles.myRole = tRoles._CF_enumeration.addEnumeration(unicode_value='myRole', tag='myRole')
tRoles.partnerRole = tRoles._CF_enumeration.addEnumeration(unicode_value='partnerRole', tag='partnerRole')
tRoles._InitializeFacetMap(tRoles._CF_enumeration)
Namespace.addCategoryObject('typeBinding', 'tRoles', tRoles)

# Complex type {http://schemas.xmlsoap.org/ws/2003/03/business-process/}tExtensibleElements with content type ELEMENT_ONLY
class tExtensibleElements (pyxb.binding.basis.complexTypeDefinition):
    """
                 This type is extended by other component types 
                 to allow elements and attributes from 
                 other namespaces to be added. 
            """
    _TypeDefinition = None
    _ContentTypeTag = pyxb.binding.basis.complexTypeDefinition._CT_ELEMENT_ONLY
    _Abstract = False
    _ExpandedName = pyxb.namespace.ExpandedName(Namespace, 'tExtensibleElements')
    _XSDLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 11, 4)
    _ElementMap = {}
    _AttributeMap = {}
    # Base type is pyxb.binding.datatypes.anyType
    _AttributeWildcard = pyxb.binding.content.Wildcard(process_contents=pyxb.binding.content.Wildcard.PC_lax, namespace_constraint=(pyxb.binding.content.Wildcard.NC_not, 'http://schemas.xmlsoap.org/ws/2003/03/business-process/'))
    _HasWildcardElement = True
    _ElementMap.update({
        
    })
    _AttributeMap.update({
        
    })
Namespace.addCategoryObject('typeBinding', 'tExtensibleElements', tExtensibleElements)


# Complex type {http://schemas.xmlsoap.org/ws/2003/03/business-process/}tVariable with content type EMPTY
class tVariable (pyxb.binding.basis.complexTypeDefinition):
    """Complex type {http://schemas.xmlsoap.org/ws/2003/03/business-process/}tVariable with content type EMPTY"""
    _TypeDefinition = None
    _ContentTypeTag = pyxb.binding.basis.complexTypeDefinition._CT_EMPTY
    _Abstract = False
    _ExpandedName = pyxb.namespace.ExpandedName(Namespace, 'tVariable')
    _XSDLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 247, 4)
    _ElementMap = {}
    _AttributeMap = {}
    # Base type is pyxb.binding.datatypes.anyType
    
    # Attribute name uses Python identifier name
    __name = pyxb.binding.content.AttributeUse(pyxb.namespace.ExpandedName(None, 'name'), 'name', '__httpschemas_xmlsoap_orgws200303business_process_tVariable_name', pyxb.binding.datatypes.NCName, required=True)
    __name._DeclarationLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 251, 16)
    __name._UseLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 251, 16)
    
    name = property(__name.value, __name.set, None, None)

    
    # Attribute messageType uses Python identifier messageType
    __messageType = pyxb.binding.content.AttributeUse(pyxb.namespace.ExpandedName(None, 'messageType'), 'messageType', '__httpschemas_xmlsoap_orgws200303business_process_tVariable_messageType', pyxb.binding.datatypes.QName)
    __messageType._DeclarationLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 252, 16)
    __messageType._UseLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 252, 16)
    
    messageType = property(__messageType.value, __messageType.set, None, None)

    
    # Attribute type uses Python identifier type
    __type = pyxb.binding.content.AttributeUse(pyxb.namespace.ExpandedName(None, 'type'), 'type', '__httpschemas_xmlsoap_orgws200303business_process_tVariable_type', pyxb.binding.datatypes.QName)
    __type._DeclarationLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 253, 16)
    __type._UseLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 253, 16)
    
    type = property(__type.value, __type.set, None, None)

    
    # Attribute element uses Python identifier element
    __element = pyxb.binding.content.AttributeUse(pyxb.namespace.ExpandedName(None, 'element'), 'element', '__httpschemas_xmlsoap_orgws200303business_process_tVariable_element', pyxb.binding.datatypes.QName)
    __element._DeclarationLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 254, 16)
    __element._UseLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 254, 16)
    
    element = property(__element.value, __element.set, None, None)

    _AttributeWildcard = pyxb.binding.content.Wildcard(process_contents=pyxb.binding.content.Wildcard.PC_lax, namespace_constraint=(pyxb.binding.content.Wildcard.NC_not, 'http://schemas.xmlsoap.org/ws/2003/03/business-process/'))
    _ElementMap.update({
        
    })
    _AttributeMap.update({
        __name.name() : __name,
        __messageType.name() : __messageType,
        __type.name() : __type,
        __element.name() : __element
    })
Namespace.addCategoryObject('typeBinding', 'tVariable', tVariable)


# Complex type {http://schemas.xmlsoap.org/ws/2003/03/business-process/}tProcess with content type ELEMENT_ONLY
class tProcess (tExtensibleElements):
    """Complex type {http://schemas.xmlsoap.org/ws/2003/03/business-process/}tProcess with content type ELEMENT_ONLY"""
    _TypeDefinition = None
    _ContentTypeTag = pyxb.binding.basis.complexTypeDefinition._CT_ELEMENT_ONLY
    _Abstract = False
    _ExpandedName = pyxb.namespace.ExpandedName(Namespace, 'tProcess')
    _XSDLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 28, 4)
    _ElementMap = tExtensibleElements._ElementMap.copy()
    _AttributeMap = tExtensibleElements._AttributeMap.copy()
    # Base type is tExtensibleElements
    
    # Element {http://schemas.xmlsoap.org/ws/2003/03/business-process/}partnerLinks uses Python identifier partnerLinks
    __partnerLinks = pyxb.binding.content.ElementDeclaration(pyxb.namespace.ExpandedName(Namespace, 'partnerLinks'), 'partnerLinks', '__httpschemas_xmlsoap_orgws200303business_process_tProcess_httpschemas_xmlsoap_orgws200303business_processpartnerLinks', False, pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 32, 20), )

    
    partnerLinks = property(__partnerLinks.value, __partnerLinks.set, None, None)

    
    # Element {http://schemas.xmlsoap.org/ws/2003/03/business-process/}partners uses Python identifier partners
    __partners = pyxb.binding.content.ElementDeclaration(pyxb.namespace.ExpandedName(Namespace, 'partners'), 'partners', '__httpschemas_xmlsoap_orgws200303business_process_tProcess_httpschemas_xmlsoap_orgws200303business_processpartners', False, pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 34, 20), )

    
    partners = property(__partners.value, __partners.set, None, None)

    
    # Element {http://schemas.xmlsoap.org/ws/2003/03/business-process/}variables uses Python identifier variables
    __variables = pyxb.binding.content.ElementDeclaration(pyxb.namespace.ExpandedName(Namespace, 'variables'), 'variables', '__httpschemas_xmlsoap_orgws200303business_process_tProcess_httpschemas_xmlsoap_orgws200303business_processvariables', False, pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 36, 20), )

    
    variables = property(__variables.value, __variables.set, None, None)

    
    # Element {http://schemas.xmlsoap.org/ws/2003/03/business-process/}correlationSets uses Python identifier correlationSets
    __correlationSets = pyxb.binding.content.ElementDeclaration(pyxb.namespace.ExpandedName(Namespace, 'correlationSets'), 'correlationSets', '__httpschemas_xmlsoap_orgws200303business_process_tProcess_httpschemas_xmlsoap_orgws200303business_processcorrelationSets', False, pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 39, 20), )

    
    correlationSets = property(__correlationSets.value, __correlationSets.set, None, None)

    
    # Element {http://schemas.xmlsoap.org/ws/2003/03/business-process/}faultHandlers uses Python identifier faultHandlers
    __faultHandlers = pyxb.binding.content.ElementDeclaration(pyxb.namespace.ExpandedName(Namespace, 'faultHandlers'), 'faultHandlers', '__httpschemas_xmlsoap_orgws200303business_process_tProcess_httpschemas_xmlsoap_orgws200303business_processfaultHandlers', False, pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 41, 20), )

    
    faultHandlers = property(__faultHandlers.value, __faultHandlers.set, None, None)

    
    # Element {http://schemas.xmlsoap.org/ws/2003/03/business-process/}compensationHandler uses Python identifier compensationHandler
    __compensationHandler = pyxb.binding.content.ElementDeclaration(pyxb.namespace.ExpandedName(Namespace, 'compensationHandler'), 'compensationHandler', '__httpschemas_xmlsoap_orgws200303business_process_tProcess_httpschemas_xmlsoap_orgws200303business_processcompensationHandler', False, pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 43, 20), )

    
    compensationHandler = property(__compensationHandler.value, __compensationHandler.set, None, None)

    
    # Element {http://schemas.xmlsoap.org/ws/2003/03/business-process/}eventHandlers uses Python identifier eventHandlers
    __eventHandlers = pyxb.binding.content.ElementDeclaration(pyxb.namespace.ExpandedName(Namespace, 'eventHandlers'), 'eventHandlers', '__httpschemas_xmlsoap_orgws200303business_process_tProcess_httpschemas_xmlsoap_orgws200303business_processeventHandlers', False, pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 45, 20), )

    
    eventHandlers = property(__eventHandlers.value, __eventHandlers.set, None, None)

    
    # Element {http://schemas.xmlsoap.org/ws/2003/03/business-process/}empty uses Python identifier empty
    __empty = pyxb.binding.content.ElementDeclaration(pyxb.namespace.ExpandedName(Namespace, 'empty'), 'empty', '__httpschemas_xmlsoap_orgws200303business_process_tProcess_httpschemas_xmlsoap_orgws200303business_processempty', False, pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 69, 12), )

    
    empty = property(__empty.value, __empty.set, None, None)

    
    # Element {http://schemas.xmlsoap.org/ws/2003/03/business-process/}invoke uses Python identifier invoke
    __invoke = pyxb.binding.content.ElementDeclaration(pyxb.namespace.ExpandedName(Namespace, 'invoke'), 'invoke', '__httpschemas_xmlsoap_orgws200303business_process_tProcess_httpschemas_xmlsoap_orgws200303business_processinvoke', False, pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 70, 12), )

    
    invoke = property(__invoke.value, __invoke.set, None, None)

    
    # Element {http://schemas.xmlsoap.org/ws/2003/03/business-process/}receive uses Python identifier receive
    __receive = pyxb.binding.content.ElementDeclaration(pyxb.namespace.ExpandedName(Namespace, 'receive'), 'receive', '__httpschemas_xmlsoap_orgws200303business_process_tProcess_httpschemas_xmlsoap_orgws200303business_processreceive', False, pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 71, 12), )

    
    receive = property(__receive.value, __receive.set, None, None)

    
    # Element {http://schemas.xmlsoap.org/ws/2003/03/business-process/}reply uses Python identifier reply
    __reply = pyxb.binding.content.ElementDeclaration(pyxb.namespace.ExpandedName(Namespace, 'reply'), 'reply', '__httpschemas_xmlsoap_orgws200303business_process_tProcess_httpschemas_xmlsoap_orgws200303business_processreply', False, pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 72, 12), )

    
    reply = property(__reply.value, __reply.set, None, None)

    
    # Element {http://schemas.xmlsoap.org/ws/2003/03/business-process/}assign uses Python identifier assign
    __assign = pyxb.binding.content.ElementDeclaration(pyxb.namespace.ExpandedName(Namespace, 'assign'), 'assign', '__httpschemas_xmlsoap_orgws200303business_process_tProcess_httpschemas_xmlsoap_orgws200303business_processassign', False, pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 73, 12), )

    
    assign = property(__assign.value, __assign.set, None, None)

    
    # Element {http://schemas.xmlsoap.org/ws/2003/03/business-process/}wait uses Python identifier wait
    __wait = pyxb.binding.content.ElementDeclaration(pyxb.namespace.ExpandedName(Namespace, 'wait'), 'wait', '__httpschemas_xmlsoap_orgws200303business_process_tProcess_httpschemas_xmlsoap_orgws200303business_processwait', False, pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 74, 12), )

    
    wait = property(__wait.value, __wait.set, None, None)

    
    # Element {http://schemas.xmlsoap.org/ws/2003/03/business-process/}throw uses Python identifier throw
    __throw = pyxb.binding.content.ElementDeclaration(pyxb.namespace.ExpandedName(Namespace, 'throw'), 'throw', '__httpschemas_xmlsoap_orgws200303business_process_tProcess_httpschemas_xmlsoap_orgws200303business_processthrow', False, pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 75, 12), )

    
    throw = property(__throw.value, __throw.set, None, None)

    
    # Element {http://schemas.xmlsoap.org/ws/2003/03/business-process/}terminate uses Python identifier terminate
    __terminate = pyxb.binding.content.ElementDeclaration(pyxb.namespace.ExpandedName(Namespace, 'terminate'), 'terminate', '__httpschemas_xmlsoap_orgws200303business_process_tProcess_httpschemas_xmlsoap_orgws200303business_processterminate', False, pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 76, 12), )

    
    terminate = property(__terminate.value, __terminate.set, None, None)

    
    # Element {http://schemas.xmlsoap.org/ws/2003/03/business-process/}flow uses Python identifier flow
    __flow = pyxb.binding.content.ElementDeclaration(pyxb.namespace.ExpandedName(Namespace, 'flow'), 'flow', '__httpschemas_xmlsoap_orgws200303business_process_tProcess_httpschemas_xmlsoap_orgws200303business_processflow', False, pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 77, 12), )

    
    flow = property(__flow.value, __flow.set, None, None)

    
    # Element {http://schemas.xmlsoap.org/ws/2003/03/business-process/}switch uses Python identifier switch
    __switch = pyxb.binding.content.ElementDeclaration(pyxb.namespace.ExpandedName(Namespace, 'switch'), 'switch', '__httpschemas_xmlsoap_orgws200303business_process_tProcess_httpschemas_xmlsoap_orgws200303business_processswitch', False, pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 78, 12), )

    
    switch = property(__switch.value, __switch.set, None, None)

    
    # Element {http://schemas.xmlsoap.org/ws/2003/03/business-process/}while uses Python identifier while_
    __while = pyxb.binding.content.ElementDeclaration(pyxb.namespace.ExpandedName(Namespace, 'while'), 'while_', '__httpschemas_xmlsoap_orgws200303business_process_tProcess_httpschemas_xmlsoap_orgws200303business_processwhile', False, pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 79, 12), )

    
    while_ = property(__while.value, __while.set, None, None)

    
    # Element {http://schemas.xmlsoap.org/ws/2003/03/business-process/}sequence uses Python identifier sequence
    __sequence = pyxb.binding.content.ElementDeclaration(pyxb.namespace.ExpandedName(Namespace, 'sequence'), 'sequence', '__httpschemas_xmlsoap_orgws200303business_process_tProcess_httpschemas_xmlsoap_orgws200303business_processsequence', False, pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 80, 12), )

    
    sequence = property(__sequence.value, __sequence.set, None, None)

    
    # Element {http://schemas.xmlsoap.org/ws/2003/03/business-process/}pick uses Python identifier pick
    __pick = pyxb.binding.content.ElementDeclaration(pyxb.namespace.ExpandedName(Namespace, 'pick'), 'pick', '__httpschemas_xmlsoap_orgws200303business_process_tProcess_httpschemas_xmlsoap_orgws200303business_processpick', False, pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 81, 12), )

    
    pick = property(__pick.value, __pick.set, None, None)

    
    # Element {http://schemas.xmlsoap.org/ws/2003/03/business-process/}scope uses Python identifier scope
    __scope = pyxb.binding.content.ElementDeclaration(pyxb.namespace.ExpandedName(Namespace, 'scope'), 'scope', '__httpschemas_xmlsoap_orgws200303business_process_tProcess_httpschemas_xmlsoap_orgws200303business_processscope', False, pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 82, 12), )

    
    scope = property(__scope.value, __scope.set, None, None)

    
    # Attribute name uses Python identifier name
    __name = pyxb.binding.content.AttributeUse(pyxb.namespace.ExpandedName(None, 'name'), 'name', '__httpschemas_xmlsoap_orgws200303business_process_tProcess_name', pyxb.binding.datatypes.NCName, required=True)
    __name._DeclarationLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 49, 16)
    __name._UseLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 49, 16)
    
    name = property(__name.value, __name.set, None, None)

    
    # Attribute targetNamespace uses Python identifier targetNamespace
    __targetNamespace = pyxb.binding.content.AttributeUse(pyxb.namespace.ExpandedName(None, 'targetNamespace'), 'targetNamespace', '__httpschemas_xmlsoap_orgws200303business_process_tProcess_targetNamespace', pyxb.binding.datatypes.anyURI, required=True)
    __targetNamespace._DeclarationLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 51, 16)
    __targetNamespace._UseLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 51, 16)
    
    targetNamespace = property(__targetNamespace.value, __targetNamespace.set, None, None)

    
    # Attribute queryLanguage uses Python identifier queryLanguage
    __queryLanguage = pyxb.binding.content.AttributeUse(pyxb.namespace.ExpandedName(None, 'queryLanguage'), 'queryLanguage', '__httpschemas_xmlsoap_orgws200303business_process_tProcess_queryLanguage', pyxb.binding.datatypes.anyURI, unicode_default='http://www.w3.org/TR/1999/REC-xpath-19991116')
    __queryLanguage._DeclarationLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 53, 16)
    __queryLanguage._UseLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 53, 16)
    
    queryLanguage = property(__queryLanguage.value, __queryLanguage.set, None, None)

    
    # Attribute expressionLanguage uses Python identifier expressionLanguage
    __expressionLanguage = pyxb.binding.content.AttributeUse(pyxb.namespace.ExpandedName(None, 'expressionLanguage'), 'expressionLanguage', '__httpschemas_xmlsoap_orgws200303business_process_tProcess_expressionLanguage', pyxb.binding.datatypes.anyURI, unicode_default='http://www.w3.org/TR/1999/REC-xpath-19991116')
    __expressionLanguage._DeclarationLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 55, 16)
    __expressionLanguage._UseLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 55, 16)
    
    expressionLanguage = property(__expressionLanguage.value, __expressionLanguage.set, None, None)

    
    # Attribute suppressJoinFailure uses Python identifier suppressJoinFailure
    __suppressJoinFailure = pyxb.binding.content.AttributeUse(pyxb.namespace.ExpandedName(None, 'suppressJoinFailure'), 'suppressJoinFailure', '__httpschemas_xmlsoap_orgws200303business_process_tProcess_suppressJoinFailure', tBoolean, unicode_default='no')
    __suppressJoinFailure._DeclarationLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 57, 16)
    __suppressJoinFailure._UseLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 57, 16)
    
    suppressJoinFailure = property(__suppressJoinFailure.value, __suppressJoinFailure.set, None, None)

    
    # Attribute enableInstanceCompensation uses Python identifier enableInstanceCompensation
    __enableInstanceCompensation = pyxb.binding.content.AttributeUse(pyxb.namespace.ExpandedName(None, 'enableInstanceCompensation'), 'enableInstanceCompensation', '__httpschemas_xmlsoap_orgws200303business_process_tProcess_enableInstanceCompensation', tBoolean, unicode_default='no')
    __enableInstanceCompensation._DeclarationLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 59, 16)
    __enableInstanceCompensation._UseLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 59, 16)
    
    enableInstanceCompensation = property(__enableInstanceCompensation.value, __enableInstanceCompensation.set, None, None)

    
    # Attribute abstractProcess uses Python identifier abstractProcess
    __abstractProcess = pyxb.binding.content.AttributeUse(pyxb.namespace.ExpandedName(None, 'abstractProcess'), 'abstractProcess', '__httpschemas_xmlsoap_orgws200303business_process_tProcess_abstractProcess', tBoolean, unicode_default='no')
    __abstractProcess._DeclarationLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 61, 16)
    __abstractProcess._UseLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 61, 16)
    
    abstractProcess = property(__abstractProcess.value, __abstractProcess.set, None, None)

    _AttributeWildcard = pyxb.binding.content.Wildcard(process_contents=pyxb.binding.content.Wildcard.PC_lax, namespace_constraint=(pyxb.binding.content.Wildcard.NC_not, 'http://schemas.xmlsoap.org/ws/2003/03/business-process/'))
    _HasWildcardElement = True
    _ElementMap.update({
        __partnerLinks.name() : __partnerLinks,
        __partners.name() : __partners,
        __variables.name() : __variables,
        __correlationSets.name() : __correlationSets,
        __faultHandlers.name() : __faultHandlers,
        __compensationHandler.name() : __compensationHandler,
        __eventHandlers.name() : __eventHandlers,
        __empty.name() : __empty,
        __invoke.name() : __invoke,
        __receive.name() : __receive,
        __reply.name() : __reply,
        __assign.name() : __assign,
        __wait.name() : __wait,
        __throw.name() : __throw,
        __terminate.name() : __terminate,
        __flow.name() : __flow,
        __switch.name() : __switch,
        __while.name() : __while,
        __sequence.name() : __sequence,
        __pick.name() : __pick,
        __scope.name() : __scope
    })
    _AttributeMap.update({
        __name.name() : __name,
        __targetNamespace.name() : __targetNamespace,
        __queryLanguage.name() : __queryLanguage,
        __expressionLanguage.name() : __expressionLanguage,
        __suppressJoinFailure.name() : __suppressJoinFailure,
        __enableInstanceCompensation.name() : __enableInstanceCompensation,
        __abstractProcess.name() : __abstractProcess
    })
Namespace.addCategoryObject('typeBinding', 'tProcess', tProcess)


# Complex type {http://schemas.xmlsoap.org/ws/2003/03/business-process/}tPartnerLinks with content type ELEMENT_ONLY
class tPartnerLinks (tExtensibleElements):
    """Complex type {http://schemas.xmlsoap.org/ws/2003/03/business-process/}tPartnerLinks with content type ELEMENT_ONLY"""
    _TypeDefinition = None
    _ContentTypeTag = pyxb.binding.basis.complexTypeDefinition._CT_ELEMENT_ONLY
    _Abstract = False
    _ExpandedName = pyxb.namespace.ExpandedName(Namespace, 'tPartnerLinks')
    _XSDLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 87, 4)
    _ElementMap = tExtensibleElements._ElementMap.copy()
    _AttributeMap = tExtensibleElements._AttributeMap.copy()
    # Base type is tExtensibleElements
    
    # Element {http://schemas.xmlsoap.org/ws/2003/03/business-process/}partnerLink uses Python identifier partnerLink
    __partnerLink = pyxb.binding.content.ElementDeclaration(pyxb.namespace.ExpandedName(Namespace, 'partnerLink'), 'partnerLink', '__httpschemas_xmlsoap_orgws200303business_process_tPartnerLinks_httpschemas_xmlsoap_orgws200303business_processpartnerLink', True, pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 91, 20), )

    
    partnerLink = property(__partnerLink.value, __partnerLink.set, None, None)

    _AttributeWildcard = pyxb.binding.content.Wildcard(process_contents=pyxb.binding.content.Wildcard.PC_lax, namespace_constraint=(pyxb.binding.content.Wildcard.NC_not, 'http://schemas.xmlsoap.org/ws/2003/03/business-process/'))
    _HasWildcardElement = True
    _ElementMap.update({
        __partnerLink.name() : __partnerLink
    })
    _AttributeMap.update({
        
    })
Namespace.addCategoryObject('typeBinding', 'tPartnerLinks', tPartnerLinks)


# Complex type {http://schemas.xmlsoap.org/ws/2003/03/business-process/}tPartnerLink with content type ELEMENT_ONLY
class tPartnerLink (tExtensibleElements):
    """Complex type {http://schemas.xmlsoap.org/ws/2003/03/business-process/}tPartnerLink with content type ELEMENT_ONLY"""
    _TypeDefinition = None
    _ContentTypeTag = pyxb.binding.basis.complexTypeDefinition._CT_ELEMENT_ONLY
    _Abstract = False
    _ExpandedName = pyxb.namespace.ExpandedName(Namespace, 'tPartnerLink')
    _XSDLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 98, 4)
    _ElementMap = tExtensibleElements._ElementMap.copy()
    _AttributeMap = tExtensibleElements._AttributeMap.copy()
    # Base type is tExtensibleElements
    
    # Attribute name uses Python identifier name
    __name = pyxb.binding.content.AttributeUse(pyxb.namespace.ExpandedName(None, 'name'), 'name', '__httpschemas_xmlsoap_orgws200303business_process_tPartnerLink_name', pyxb.binding.datatypes.NCName, required=True)
    __name._DeclarationLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 101, 16)
    __name._UseLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 101, 16)
    
    name = property(__name.value, __name.set, None, None)

    
    # Attribute partnerLinkType uses Python identifier partnerLinkType
    __partnerLinkType = pyxb.binding.content.AttributeUse(pyxb.namespace.ExpandedName(None, 'partnerLinkType'), 'partnerLinkType', '__httpschemas_xmlsoap_orgws200303business_process_tPartnerLink_partnerLinkType', pyxb.binding.datatypes.QName, required=True)
    __partnerLinkType._DeclarationLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 102, 16)
    __partnerLinkType._UseLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 102, 16)
    
    partnerLinkType = property(__partnerLinkType.value, __partnerLinkType.set, None, None)

    
    # Attribute myRole uses Python identifier myRole
    __myRole = pyxb.binding.content.AttributeUse(pyxb.namespace.ExpandedName(None, 'myRole'), 'myRole', '__httpschemas_xmlsoap_orgws200303business_process_tPartnerLink_myRole', pyxb.binding.datatypes.NCName)
    __myRole._DeclarationLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 104, 16)
    __myRole._UseLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 104, 16)
    
    myRole = property(__myRole.value, __myRole.set, None, None)

    
    # Attribute partnerRole uses Python identifier partnerRole
    __partnerRole = pyxb.binding.content.AttributeUse(pyxb.namespace.ExpandedName(None, 'partnerRole'), 'partnerRole', '__httpschemas_xmlsoap_orgws200303business_process_tPartnerLink_partnerRole', pyxb.binding.datatypes.NCName)
    __partnerRole._DeclarationLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 105, 16)
    __partnerRole._UseLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 105, 16)
    
    partnerRole = property(__partnerRole.value, __partnerRole.set, None, None)

    _AttributeWildcard = pyxb.binding.content.Wildcard(process_contents=pyxb.binding.content.Wildcard.PC_lax, namespace_constraint=(pyxb.binding.content.Wildcard.NC_not, 'http://schemas.xmlsoap.org/ws/2003/03/business-process/'))
    _HasWildcardElement = True
    _ElementMap.update({
        
    })
    _AttributeMap.update({
        __name.name() : __name,
        __partnerLinkType.name() : __partnerLinkType,
        __myRole.name() : __myRole,
        __partnerRole.name() : __partnerRole
    })
Namespace.addCategoryObject('typeBinding', 'tPartnerLink', tPartnerLink)


# Complex type {http://schemas.xmlsoap.org/ws/2003/03/business-process/}tPartners with content type ELEMENT_ONLY
class tPartners (tExtensibleElements):
    """Complex type {http://schemas.xmlsoap.org/ws/2003/03/business-process/}tPartners with content type ELEMENT_ONLY"""
    _TypeDefinition = None
    _ContentTypeTag = pyxb.binding.basis.complexTypeDefinition._CT_ELEMENT_ONLY
    _Abstract = False
    _ExpandedName = pyxb.namespace.ExpandedName(Namespace, 'tPartners')
    _XSDLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 111, 4)
    _ElementMap = tExtensibleElements._ElementMap.copy()
    _AttributeMap = tExtensibleElements._AttributeMap.copy()
    # Base type is tExtensibleElements
    
    # Element {http://schemas.xmlsoap.org/ws/2003/03/business-process/}partner uses Python identifier partner
    __partner = pyxb.binding.content.ElementDeclaration(pyxb.namespace.ExpandedName(Namespace, 'partner'), 'partner', '__httpschemas_xmlsoap_orgws200303business_process_tPartners_httpschemas_xmlsoap_orgws200303business_processpartner', True, pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 115, 20), )

    
    partner = property(__partner.value, __partner.set, None, None)

    _AttributeWildcard = pyxb.binding.content.Wildcard(process_contents=pyxb.binding.content.Wildcard.PC_lax, namespace_constraint=(pyxb.binding.content.Wildcard.NC_not, 'http://schemas.xmlsoap.org/ws/2003/03/business-process/'))
    _HasWildcardElement = True
    _ElementMap.update({
        __partner.name() : __partner
    })
    _AttributeMap.update({
        
    })
Namespace.addCategoryObject('typeBinding', 'tPartners', tPartners)


# Complex type {http://schemas.xmlsoap.org/ws/2003/03/business-process/}tPartner with content type ELEMENT_ONLY
class tPartner (tExtensibleElements):
    """Complex type {http://schemas.xmlsoap.org/ws/2003/03/business-process/}tPartner with content type ELEMENT_ONLY"""
    _TypeDefinition = None
    _ContentTypeTag = pyxb.binding.basis.complexTypeDefinition._CT_ELEMENT_ONLY
    _Abstract = False
    _ExpandedName = pyxb.namespace.ExpandedName(Namespace, 'tPartner')
    _XSDLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 122, 4)
    _ElementMap = tExtensibleElements._ElementMap.copy()
    _AttributeMap = tExtensibleElements._AttributeMap.copy()
    # Base type is tExtensibleElements
    
    # Element {http://schemas.xmlsoap.org/ws/2003/03/business-process/}partnerLink uses Python identifier partnerLink
    __partnerLink = pyxb.binding.content.ElementDeclaration(pyxb.namespace.ExpandedName(Namespace, 'partnerLink'), 'partnerLink', '__httpschemas_xmlsoap_orgws200303business_process_tPartner_httpschemas_xmlsoap_orgws200303business_processpartnerLink', True, pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 126, 18), )

    
    partnerLink = property(__partnerLink.value, __partnerLink.set, None, None)

    
    # Attribute name uses Python identifier name
    __name = pyxb.binding.content.AttributeUse(pyxb.namespace.ExpandedName(None, 'name'), 'name', '__httpschemas_xmlsoap_orgws200303business_process_tPartner_name', pyxb.binding.datatypes.NCName, required=True)
    __name._DeclarationLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 138, 15)
    __name._UseLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 138, 15)
    
    name = property(__name.value, __name.set, None, None)

    _AttributeWildcard = pyxb.binding.content.Wildcard(process_contents=pyxb.binding.content.Wildcard.PC_lax, namespace_constraint=(pyxb.binding.content.Wildcard.NC_not, 'http://schemas.xmlsoap.org/ws/2003/03/business-process/'))
    _HasWildcardElement = True
    _ElementMap.update({
        __partnerLink.name() : __partnerLink
    })
    _AttributeMap.update({
        __name.name() : __name
    })
Namespace.addCategoryObject('typeBinding', 'tPartner', tPartner)


# Complex type [anonymous] with content type ELEMENT_ONLY
class CTD_ANON (tExtensibleElements):
    """Complex type [anonymous] with content type ELEMENT_ONLY"""
    _TypeDefinition = None
    _ContentTypeTag = pyxb.binding.basis.complexTypeDefinition._CT_ELEMENT_ONLY
    _Abstract = False
    _ExpandedName = None
    _XSDLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 128, 21)
    _ElementMap = tExtensibleElements._ElementMap.copy()
    _AttributeMap = tExtensibleElements._AttributeMap.copy()
    # Base type is tExtensibleElements
    
    # Attribute name uses Python identifier name
    __name = pyxb.binding.content.AttributeUse(pyxb.namespace.ExpandedName(None, 'name'), 'name', '__httpschemas_xmlsoap_orgws200303business_process_CTD_ANON_name', pyxb.binding.datatypes.NCName, required=True)
    __name._DeclarationLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 131, 31)
    __name._UseLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 131, 31)
    
    name = property(__name.value, __name.set, None, None)

    _AttributeWildcard = pyxb.binding.content.Wildcard(process_contents=pyxb.binding.content.Wildcard.PC_lax, namespace_constraint=(pyxb.binding.content.Wildcard.NC_not, 'http://schemas.xmlsoap.org/ws/2003/03/business-process/'))
    _HasWildcardElement = True
    _ElementMap.update({
        
    })
    _AttributeMap.update({
        __name.name() : __name
    })



# Complex type {http://schemas.xmlsoap.org/ws/2003/03/business-process/}tFaultHandlers with content type ELEMENT_ONLY
class tFaultHandlers (tExtensibleElements):
    """Complex type {http://schemas.xmlsoap.org/ws/2003/03/business-process/}tFaultHandlers with content type ELEMENT_ONLY"""
    _TypeDefinition = None
    _ContentTypeTag = pyxb.binding.basis.complexTypeDefinition._CT_ELEMENT_ONLY
    _Abstract = False
    _ExpandedName = pyxb.namespace.ExpandedName(Namespace, 'tFaultHandlers')
    _XSDLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 143, 4)
    _ElementMap = tExtensibleElements._ElementMap.copy()
    _AttributeMap = tExtensibleElements._AttributeMap.copy()
    # Base type is tExtensibleElements
    
    # Element {http://schemas.xmlsoap.org/ws/2003/03/business-process/}catch uses Python identifier catch
    __catch = pyxb.binding.content.ElementDeclaration(pyxb.namespace.ExpandedName(Namespace, 'catch'), 'catch', '__httpschemas_xmlsoap_orgws200303business_process_tFaultHandlers_httpschemas_xmlsoap_orgws200303business_processcatch', True, pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 148, 20), )

    
    catch = property(__catch.value, __catch.set, None, None)

    
    # Element {http://schemas.xmlsoap.org/ws/2003/03/business-process/}catchAll uses Python identifier catchAll
    __catchAll = pyxb.binding.content.ElementDeclaration(pyxb.namespace.ExpandedName(Namespace, 'catchAll'), 'catchAll', '__httpschemas_xmlsoap_orgws200303business_process_tFaultHandlers_httpschemas_xmlsoap_orgws200303business_processcatchAll', False, pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 150, 20), )

    
    catchAll = property(__catchAll.value, __catchAll.set, None, None)

    _AttributeWildcard = pyxb.binding.content.Wildcard(process_contents=pyxb.binding.content.Wildcard.PC_lax, namespace_constraint=(pyxb.binding.content.Wildcard.NC_not, 'http://schemas.xmlsoap.org/ws/2003/03/business-process/'))
    _HasWildcardElement = True
    _ElementMap.update({
        __catch.name() : __catch,
        __catchAll.name() : __catchAll
    })
    _AttributeMap.update({
        
    })
Namespace.addCategoryObject('typeBinding', 'tFaultHandlers', tFaultHandlers)


# Complex type {http://schemas.xmlsoap.org/ws/2003/03/business-process/}tActivityContainer with content type ELEMENT_ONLY
class tActivityContainer (tExtensibleElements):
    """Complex type {http://schemas.xmlsoap.org/ws/2003/03/business-process/}tActivityContainer with content type ELEMENT_ONLY"""
    _TypeDefinition = None
    _ContentTypeTag = pyxb.binding.basis.complexTypeDefinition._CT_ELEMENT_ONLY
    _Abstract = False
    _ExpandedName = pyxb.namespace.ExpandedName(Namespace, 'tActivityContainer')
    _XSDLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 167, 4)
    _ElementMap = tExtensibleElements._ElementMap.copy()
    _AttributeMap = tExtensibleElements._AttributeMap.copy()
    # Base type is tExtensibleElements
    
    # Element {http://schemas.xmlsoap.org/ws/2003/03/business-process/}empty uses Python identifier empty
    __empty = pyxb.binding.content.ElementDeclaration(pyxb.namespace.ExpandedName(Namespace, 'empty'), 'empty', '__httpschemas_xmlsoap_orgws200303business_process_tActivityContainer_httpschemas_xmlsoap_orgws200303business_processempty', False, pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 69, 12), )

    
    empty = property(__empty.value, __empty.set, None, None)

    
    # Element {http://schemas.xmlsoap.org/ws/2003/03/business-process/}invoke uses Python identifier invoke
    __invoke = pyxb.binding.content.ElementDeclaration(pyxb.namespace.ExpandedName(Namespace, 'invoke'), 'invoke', '__httpschemas_xmlsoap_orgws200303business_process_tActivityContainer_httpschemas_xmlsoap_orgws200303business_processinvoke', False, pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 70, 12), )

    
    invoke = property(__invoke.value, __invoke.set, None, None)

    
    # Element {http://schemas.xmlsoap.org/ws/2003/03/business-process/}receive uses Python identifier receive
    __receive = pyxb.binding.content.ElementDeclaration(pyxb.namespace.ExpandedName(Namespace, 'receive'), 'receive', '__httpschemas_xmlsoap_orgws200303business_process_tActivityContainer_httpschemas_xmlsoap_orgws200303business_processreceive', False, pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 71, 12), )

    
    receive = property(__receive.value, __receive.set, None, None)

    
    # Element {http://schemas.xmlsoap.org/ws/2003/03/business-process/}reply uses Python identifier reply
    __reply = pyxb.binding.content.ElementDeclaration(pyxb.namespace.ExpandedName(Namespace, 'reply'), 'reply', '__httpschemas_xmlsoap_orgws200303business_process_tActivityContainer_httpschemas_xmlsoap_orgws200303business_processreply', False, pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 72, 12), )

    
    reply = property(__reply.value, __reply.set, None, None)

    
    # Element {http://schemas.xmlsoap.org/ws/2003/03/business-process/}assign uses Python identifier assign
    __assign = pyxb.binding.content.ElementDeclaration(pyxb.namespace.ExpandedName(Namespace, 'assign'), 'assign', '__httpschemas_xmlsoap_orgws200303business_process_tActivityContainer_httpschemas_xmlsoap_orgws200303business_processassign', False, pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 73, 12), )

    
    assign = property(__assign.value, __assign.set, None, None)

    
    # Element {http://schemas.xmlsoap.org/ws/2003/03/business-process/}wait uses Python identifier wait
    __wait = pyxb.binding.content.ElementDeclaration(pyxb.namespace.ExpandedName(Namespace, 'wait'), 'wait', '__httpschemas_xmlsoap_orgws200303business_process_tActivityContainer_httpschemas_xmlsoap_orgws200303business_processwait', False, pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 74, 12), )

    
    wait = property(__wait.value, __wait.set, None, None)

    
    # Element {http://schemas.xmlsoap.org/ws/2003/03/business-process/}throw uses Python identifier throw
    __throw = pyxb.binding.content.ElementDeclaration(pyxb.namespace.ExpandedName(Namespace, 'throw'), 'throw', '__httpschemas_xmlsoap_orgws200303business_process_tActivityContainer_httpschemas_xmlsoap_orgws200303business_processthrow', False, pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 75, 12), )

    
    throw = property(__throw.value, __throw.set, None, None)

    
    # Element {http://schemas.xmlsoap.org/ws/2003/03/business-process/}terminate uses Python identifier terminate
    __terminate = pyxb.binding.content.ElementDeclaration(pyxb.namespace.ExpandedName(Namespace, 'terminate'), 'terminate', '__httpschemas_xmlsoap_orgws200303business_process_tActivityContainer_httpschemas_xmlsoap_orgws200303business_processterminate', False, pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 76, 12), )

    
    terminate = property(__terminate.value, __terminate.set, None, None)

    
    # Element {http://schemas.xmlsoap.org/ws/2003/03/business-process/}flow uses Python identifier flow
    __flow = pyxb.binding.content.ElementDeclaration(pyxb.namespace.ExpandedName(Namespace, 'flow'), 'flow', '__httpschemas_xmlsoap_orgws200303business_process_tActivityContainer_httpschemas_xmlsoap_orgws200303business_processflow', False, pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 77, 12), )

    
    flow = property(__flow.value, __flow.set, None, None)

    
    # Element {http://schemas.xmlsoap.org/ws/2003/03/business-process/}switch uses Python identifier switch
    __switch = pyxb.binding.content.ElementDeclaration(pyxb.namespace.ExpandedName(Namespace, 'switch'), 'switch', '__httpschemas_xmlsoap_orgws200303business_process_tActivityContainer_httpschemas_xmlsoap_orgws200303business_processswitch', False, pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 78, 12), )

    
    switch = property(__switch.value, __switch.set, None, None)

    
    # Element {http://schemas.xmlsoap.org/ws/2003/03/business-process/}while uses Python identifier while_
    __while = pyxb.binding.content.ElementDeclaration(pyxb.namespace.ExpandedName(Namespace, 'while'), 'while_', '__httpschemas_xmlsoap_orgws200303business_process_tActivityContainer_httpschemas_xmlsoap_orgws200303business_processwhile', False, pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 79, 12), )

    
    while_ = property(__while.value, __while.set, None, None)

    
    # Element {http://schemas.xmlsoap.org/ws/2003/03/business-process/}sequence uses Python identifier sequence
    __sequence = pyxb.binding.content.ElementDeclaration(pyxb.namespace.ExpandedName(Namespace, 'sequence'), 'sequence', '__httpschemas_xmlsoap_orgws200303business_process_tActivityContainer_httpschemas_xmlsoap_orgws200303business_processsequence', False, pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 80, 12), )

    
    sequence = property(__sequence.value, __sequence.set, None, None)

    
    # Element {http://schemas.xmlsoap.org/ws/2003/03/business-process/}pick uses Python identifier pick
    __pick = pyxb.binding.content.ElementDeclaration(pyxb.namespace.ExpandedName(Namespace, 'pick'), 'pick', '__httpschemas_xmlsoap_orgws200303business_process_tActivityContainer_httpschemas_xmlsoap_orgws200303business_processpick', False, pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 81, 12), )

    
    pick = property(__pick.value, __pick.set, None, None)

    
    # Element {http://schemas.xmlsoap.org/ws/2003/03/business-process/}scope uses Python identifier scope
    __scope = pyxb.binding.content.ElementDeclaration(pyxb.namespace.ExpandedName(Namespace, 'scope'), 'scope', '__httpschemas_xmlsoap_orgws200303business_process_tActivityContainer_httpschemas_xmlsoap_orgws200303business_processscope', False, pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 82, 12), )

    
    scope = property(__scope.value, __scope.set, None, None)

    _AttributeWildcard = pyxb.binding.content.Wildcard(process_contents=pyxb.binding.content.Wildcard.PC_lax, namespace_constraint=(pyxb.binding.content.Wildcard.NC_not, 'http://schemas.xmlsoap.org/ws/2003/03/business-process/'))
    _HasWildcardElement = True
    _ElementMap.update({
        __empty.name() : __empty,
        __invoke.name() : __invoke,
        __receive.name() : __receive,
        __reply.name() : __reply,
        __assign.name() : __assign,
        __wait.name() : __wait,
        __throw.name() : __throw,
        __terminate.name() : __terminate,
        __flow.name() : __flow,
        __switch.name() : __switch,
        __while.name() : __while,
        __sequence.name() : __sequence,
        __pick.name() : __pick,
        __scope.name() : __scope
    })
    _AttributeMap.update({
        
    })
Namespace.addCategoryObject('typeBinding', 'tActivityContainer', tActivityContainer)


# Complex type {http://schemas.xmlsoap.org/ws/2003/03/business-process/}tActivityOrCompensateContainer with content type ELEMENT_ONLY
class tActivityOrCompensateContainer (tExtensibleElements):
    """Complex type {http://schemas.xmlsoap.org/ws/2003/03/business-process/}tActivityOrCompensateContainer with content type ELEMENT_ONLY"""
    _TypeDefinition = None
    _ContentTypeTag = pyxb.binding.basis.complexTypeDefinition._CT_ELEMENT_ONLY
    _Abstract = False
    _ExpandedName = pyxb.namespace.ExpandedName(Namespace, 'tActivityOrCompensateContainer')
    _XSDLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 176, 4)
    _ElementMap = tExtensibleElements._ElementMap.copy()
    _AttributeMap = tExtensibleElements._AttributeMap.copy()
    # Base type is tExtensibleElements
    
    # Element {http://schemas.xmlsoap.org/ws/2003/03/business-process/}empty uses Python identifier empty
    __empty = pyxb.binding.content.ElementDeclaration(pyxb.namespace.ExpandedName(Namespace, 'empty'), 'empty', '__httpschemas_xmlsoap_orgws200303business_process_tActivityOrCompensateContainer_httpschemas_xmlsoap_orgws200303business_processempty', False, pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 69, 12), )

    
    empty = property(__empty.value, __empty.set, None, None)

    
    # Element {http://schemas.xmlsoap.org/ws/2003/03/business-process/}invoke uses Python identifier invoke
    __invoke = pyxb.binding.content.ElementDeclaration(pyxb.namespace.ExpandedName(Namespace, 'invoke'), 'invoke', '__httpschemas_xmlsoap_orgws200303business_process_tActivityOrCompensateContainer_httpschemas_xmlsoap_orgws200303business_processinvoke', False, pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 70, 12), )

    
    invoke = property(__invoke.value, __invoke.set, None, None)

    
    # Element {http://schemas.xmlsoap.org/ws/2003/03/business-process/}receive uses Python identifier receive
    __receive = pyxb.binding.content.ElementDeclaration(pyxb.namespace.ExpandedName(Namespace, 'receive'), 'receive', '__httpschemas_xmlsoap_orgws200303business_process_tActivityOrCompensateContainer_httpschemas_xmlsoap_orgws200303business_processreceive', False, pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 71, 12), )

    
    receive = property(__receive.value, __receive.set, None, None)

    
    # Element {http://schemas.xmlsoap.org/ws/2003/03/business-process/}reply uses Python identifier reply
    __reply = pyxb.binding.content.ElementDeclaration(pyxb.namespace.ExpandedName(Namespace, 'reply'), 'reply', '__httpschemas_xmlsoap_orgws200303business_process_tActivityOrCompensateContainer_httpschemas_xmlsoap_orgws200303business_processreply', False, pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 72, 12), )

    
    reply = property(__reply.value, __reply.set, None, None)

    
    # Element {http://schemas.xmlsoap.org/ws/2003/03/business-process/}assign uses Python identifier assign
    __assign = pyxb.binding.content.ElementDeclaration(pyxb.namespace.ExpandedName(Namespace, 'assign'), 'assign', '__httpschemas_xmlsoap_orgws200303business_process_tActivityOrCompensateContainer_httpschemas_xmlsoap_orgws200303business_processassign', False, pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 73, 12), )

    
    assign = property(__assign.value, __assign.set, None, None)

    
    # Element {http://schemas.xmlsoap.org/ws/2003/03/business-process/}wait uses Python identifier wait
    __wait = pyxb.binding.content.ElementDeclaration(pyxb.namespace.ExpandedName(Namespace, 'wait'), 'wait', '__httpschemas_xmlsoap_orgws200303business_process_tActivityOrCompensateContainer_httpschemas_xmlsoap_orgws200303business_processwait', False, pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 74, 12), )

    
    wait = property(__wait.value, __wait.set, None, None)

    
    # Element {http://schemas.xmlsoap.org/ws/2003/03/business-process/}throw uses Python identifier throw
    __throw = pyxb.binding.content.ElementDeclaration(pyxb.namespace.ExpandedName(Namespace, 'throw'), 'throw', '__httpschemas_xmlsoap_orgws200303business_process_tActivityOrCompensateContainer_httpschemas_xmlsoap_orgws200303business_processthrow', False, pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 75, 12), )

    
    throw = property(__throw.value, __throw.set, None, None)

    
    # Element {http://schemas.xmlsoap.org/ws/2003/03/business-process/}terminate uses Python identifier terminate
    __terminate = pyxb.binding.content.ElementDeclaration(pyxb.namespace.ExpandedName(Namespace, 'terminate'), 'terminate', '__httpschemas_xmlsoap_orgws200303business_process_tActivityOrCompensateContainer_httpschemas_xmlsoap_orgws200303business_processterminate', False, pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 76, 12), )

    
    terminate = property(__terminate.value, __terminate.set, None, None)

    
    # Element {http://schemas.xmlsoap.org/ws/2003/03/business-process/}flow uses Python identifier flow
    __flow = pyxb.binding.content.ElementDeclaration(pyxb.namespace.ExpandedName(Namespace, 'flow'), 'flow', '__httpschemas_xmlsoap_orgws200303business_process_tActivityOrCompensateContainer_httpschemas_xmlsoap_orgws200303business_processflow', False, pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 77, 12), )

    
    flow = property(__flow.value, __flow.set, None, None)

    
    # Element {http://schemas.xmlsoap.org/ws/2003/03/business-process/}switch uses Python identifier switch
    __switch = pyxb.binding.content.ElementDeclaration(pyxb.namespace.ExpandedName(Namespace, 'switch'), 'switch', '__httpschemas_xmlsoap_orgws200303business_process_tActivityOrCompensateContainer_httpschemas_xmlsoap_orgws200303business_processswitch', False, pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 78, 12), )

    
    switch = property(__switch.value, __switch.set, None, None)

    
    # Element {http://schemas.xmlsoap.org/ws/2003/03/business-process/}while uses Python identifier while_
    __while = pyxb.binding.content.ElementDeclaration(pyxb.namespace.ExpandedName(Namespace, 'while'), 'while_', '__httpschemas_xmlsoap_orgws200303business_process_tActivityOrCompensateContainer_httpschemas_xmlsoap_orgws200303business_processwhile', False, pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 79, 12), )

    
    while_ = property(__while.value, __while.set, None, None)

    
    # Element {http://schemas.xmlsoap.org/ws/2003/03/business-process/}sequence uses Python identifier sequence
    __sequence = pyxb.binding.content.ElementDeclaration(pyxb.namespace.ExpandedName(Namespace, 'sequence'), 'sequence', '__httpschemas_xmlsoap_orgws200303business_process_tActivityOrCompensateContainer_httpschemas_xmlsoap_orgws200303business_processsequence', False, pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 80, 12), )

    
    sequence = property(__sequence.value, __sequence.set, None, None)

    
    # Element {http://schemas.xmlsoap.org/ws/2003/03/business-process/}pick uses Python identifier pick
    __pick = pyxb.binding.content.ElementDeclaration(pyxb.namespace.ExpandedName(Namespace, 'pick'), 'pick', '__httpschemas_xmlsoap_orgws200303business_process_tActivityOrCompensateContainer_httpschemas_xmlsoap_orgws200303business_processpick', False, pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 81, 12), )

    
    pick = property(__pick.value, __pick.set, None, None)

    
    # Element {http://schemas.xmlsoap.org/ws/2003/03/business-process/}scope uses Python identifier scope
    __scope = pyxb.binding.content.ElementDeclaration(pyxb.namespace.ExpandedName(Namespace, 'scope'), 'scope', '__httpschemas_xmlsoap_orgws200303business_process_tActivityOrCompensateContainer_httpschemas_xmlsoap_orgws200303business_processscope', False, pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 82, 12), )

    
    scope = property(__scope.value, __scope.set, None, None)

    
    # Element {http://schemas.xmlsoap.org/ws/2003/03/business-process/}compensate uses Python identifier compensate
    __compensate = pyxb.binding.content.ElementDeclaration(pyxb.namespace.ExpandedName(Namespace, 'compensate'), 'compensate', '__httpschemas_xmlsoap_orgws200303business_process_tActivityOrCompensateContainer_httpschemas_xmlsoap_orgws200303business_processcompensate', False, pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 181, 20), )

    
    compensate = property(__compensate.value, __compensate.set, None, None)

    _AttributeWildcard = pyxb.binding.content.Wildcard(process_contents=pyxb.binding.content.Wildcard.PC_lax, namespace_constraint=(pyxb.binding.content.Wildcard.NC_not, 'http://schemas.xmlsoap.org/ws/2003/03/business-process/'))
    _HasWildcardElement = True
    _ElementMap.update({
        __empty.name() : __empty,
        __invoke.name() : __invoke,
        __receive.name() : __receive,
        __reply.name() : __reply,
        __assign.name() : __assign,
        __wait.name() : __wait,
        __throw.name() : __throw,
        __terminate.name() : __terminate,
        __flow.name() : __flow,
        __switch.name() : __switch,
        __while.name() : __while,
        __sequence.name() : __sequence,
        __pick.name() : __pick,
        __scope.name() : __scope,
        __compensate.name() : __compensate
    })
    _AttributeMap.update({
        
    })
Namespace.addCategoryObject('typeBinding', 'tActivityOrCompensateContainer', tActivityOrCompensateContainer)


# Complex type {http://schemas.xmlsoap.org/ws/2003/03/business-process/}tEventHandlers with content type ELEMENT_ONLY
class tEventHandlers (tExtensibleElements):
    """Complex type {http://schemas.xmlsoap.org/ws/2003/03/business-process/}tEventHandlers with content type ELEMENT_ONLY"""
    _TypeDefinition = None
    _ContentTypeTag = pyxb.binding.basis.complexTypeDefinition._CT_ELEMENT_ONLY
    _Abstract = False
    _ExpandedName = pyxb.namespace.ExpandedName(Namespace, 'tEventHandlers')
    _XSDLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 187, 4)
    _ElementMap = tExtensibleElements._ElementMap.copy()
    _AttributeMap = tExtensibleElements._AttributeMap.copy()
    # Base type is tExtensibleElements
    
    # Element {http://schemas.xmlsoap.org/ws/2003/03/business-process/}onMessage uses Python identifier onMessage
    __onMessage = pyxb.binding.content.ElementDeclaration(pyxb.namespace.ExpandedName(Namespace, 'onMessage'), 'onMessage', '__httpschemas_xmlsoap_orgws200303business_process_tEventHandlers_httpschemas_xmlsoap_orgws200303business_processonMessage', True, pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 191, 19), )

    
    onMessage = property(__onMessage.value, __onMessage.set, None, None)

    
    # Element {http://schemas.xmlsoap.org/ws/2003/03/business-process/}onAlarm uses Python identifier onAlarm
    __onAlarm = pyxb.binding.content.ElementDeclaration(pyxb.namespace.ExpandedName(Namespace, 'onAlarm'), 'onAlarm', '__httpschemas_xmlsoap_orgws200303business_process_tEventHandlers_httpschemas_xmlsoap_orgws200303business_processonAlarm', True, pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 193, 19), )

    
    onAlarm = property(__onAlarm.value, __onAlarm.set, None, None)

    _AttributeWildcard = pyxb.binding.content.Wildcard(process_contents=pyxb.binding.content.Wildcard.PC_lax, namespace_constraint=(pyxb.binding.content.Wildcard.NC_not, 'http://schemas.xmlsoap.org/ws/2003/03/business-process/'))
    _HasWildcardElement = True
    _ElementMap.update({
        __onMessage.name() : __onMessage,
        __onAlarm.name() : __onAlarm
    })
    _AttributeMap.update({
        
    })
Namespace.addCategoryObject('typeBinding', 'tEventHandlers', tEventHandlers)


# Complex type {http://schemas.xmlsoap.org/ws/2003/03/business-process/}tOnMessage with content type ELEMENT_ONLY
class tOnMessage (tExtensibleElements):
    """Complex type {http://schemas.xmlsoap.org/ws/2003/03/business-process/}tOnMessage with content type ELEMENT_ONLY"""
    _TypeDefinition = None
    _ContentTypeTag = pyxb.binding.basis.complexTypeDefinition._CT_ELEMENT_ONLY
    _Abstract = False
    _ExpandedName = pyxb.namespace.ExpandedName(Namespace, 'tOnMessage')
    _XSDLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 201, 4)
    _ElementMap = tExtensibleElements._ElementMap.copy()
    _AttributeMap = tExtensibleElements._AttributeMap.copy()
    # Base type is tExtensibleElements
    
    # Element {http://schemas.xmlsoap.org/ws/2003/03/business-process/}empty uses Python identifier empty
    __empty = pyxb.binding.content.ElementDeclaration(pyxb.namespace.ExpandedName(Namespace, 'empty'), 'empty', '__httpschemas_xmlsoap_orgws200303business_process_tOnMessage_httpschemas_xmlsoap_orgws200303business_processempty', False, pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 69, 12), )

    
    empty = property(__empty.value, __empty.set, None, None)

    
    # Element {http://schemas.xmlsoap.org/ws/2003/03/business-process/}invoke uses Python identifier invoke
    __invoke = pyxb.binding.content.ElementDeclaration(pyxb.namespace.ExpandedName(Namespace, 'invoke'), 'invoke', '__httpschemas_xmlsoap_orgws200303business_process_tOnMessage_httpschemas_xmlsoap_orgws200303business_processinvoke', False, pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 70, 12), )

    
    invoke = property(__invoke.value, __invoke.set, None, None)

    
    # Element {http://schemas.xmlsoap.org/ws/2003/03/business-process/}receive uses Python identifier receive
    __receive = pyxb.binding.content.ElementDeclaration(pyxb.namespace.ExpandedName(Namespace, 'receive'), 'receive', '__httpschemas_xmlsoap_orgws200303business_process_tOnMessage_httpschemas_xmlsoap_orgws200303business_processreceive', False, pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 71, 12), )

    
    receive = property(__receive.value, __receive.set, None, None)

    
    # Element {http://schemas.xmlsoap.org/ws/2003/03/business-process/}reply uses Python identifier reply
    __reply = pyxb.binding.content.ElementDeclaration(pyxb.namespace.ExpandedName(Namespace, 'reply'), 'reply', '__httpschemas_xmlsoap_orgws200303business_process_tOnMessage_httpschemas_xmlsoap_orgws200303business_processreply', False, pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 72, 12), )

    
    reply = property(__reply.value, __reply.set, None, None)

    
    # Element {http://schemas.xmlsoap.org/ws/2003/03/business-process/}assign uses Python identifier assign
    __assign = pyxb.binding.content.ElementDeclaration(pyxb.namespace.ExpandedName(Namespace, 'assign'), 'assign', '__httpschemas_xmlsoap_orgws200303business_process_tOnMessage_httpschemas_xmlsoap_orgws200303business_processassign', False, pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 73, 12), )

    
    assign = property(__assign.value, __assign.set, None, None)

    
    # Element {http://schemas.xmlsoap.org/ws/2003/03/business-process/}wait uses Python identifier wait
    __wait = pyxb.binding.content.ElementDeclaration(pyxb.namespace.ExpandedName(Namespace, 'wait'), 'wait', '__httpschemas_xmlsoap_orgws200303business_process_tOnMessage_httpschemas_xmlsoap_orgws200303business_processwait', False, pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 74, 12), )

    
    wait = property(__wait.value, __wait.set, None, None)

    
    # Element {http://schemas.xmlsoap.org/ws/2003/03/business-process/}throw uses Python identifier throw
    __throw = pyxb.binding.content.ElementDeclaration(pyxb.namespace.ExpandedName(Namespace, 'throw'), 'throw', '__httpschemas_xmlsoap_orgws200303business_process_tOnMessage_httpschemas_xmlsoap_orgws200303business_processthrow', False, pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 75, 12), )

    
    throw = property(__throw.value, __throw.set, None, None)

    
    # Element {http://schemas.xmlsoap.org/ws/2003/03/business-process/}terminate uses Python identifier terminate
    __terminate = pyxb.binding.content.ElementDeclaration(pyxb.namespace.ExpandedName(Namespace, 'terminate'), 'terminate', '__httpschemas_xmlsoap_orgws200303business_process_tOnMessage_httpschemas_xmlsoap_orgws200303business_processterminate', False, pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 76, 12), )

    
    terminate = property(__terminate.value, __terminate.set, None, None)

    
    # Element {http://schemas.xmlsoap.org/ws/2003/03/business-process/}flow uses Python identifier flow
    __flow = pyxb.binding.content.ElementDeclaration(pyxb.namespace.ExpandedName(Namespace, 'flow'), 'flow', '__httpschemas_xmlsoap_orgws200303business_process_tOnMessage_httpschemas_xmlsoap_orgws200303business_processflow', False, pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 77, 12), )

    
    flow = property(__flow.value, __flow.set, None, None)

    
    # Element {http://schemas.xmlsoap.org/ws/2003/03/business-process/}switch uses Python identifier switch
    __switch = pyxb.binding.content.ElementDeclaration(pyxb.namespace.ExpandedName(Namespace, 'switch'), 'switch', '__httpschemas_xmlsoap_orgws200303business_process_tOnMessage_httpschemas_xmlsoap_orgws200303business_processswitch', False, pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 78, 12), )

    
    switch = property(__switch.value, __switch.set, None, None)

    
    # Element {http://schemas.xmlsoap.org/ws/2003/03/business-process/}while uses Python identifier while_
    __while = pyxb.binding.content.ElementDeclaration(pyxb.namespace.ExpandedName(Namespace, 'while'), 'while_', '__httpschemas_xmlsoap_orgws200303business_process_tOnMessage_httpschemas_xmlsoap_orgws200303business_processwhile', False, pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 79, 12), )

    
    while_ = property(__while.value, __while.set, None, None)

    
    # Element {http://schemas.xmlsoap.org/ws/2003/03/business-process/}sequence uses Python identifier sequence
    __sequence = pyxb.binding.content.ElementDeclaration(pyxb.namespace.ExpandedName(Namespace, 'sequence'), 'sequence', '__httpschemas_xmlsoap_orgws200303business_process_tOnMessage_httpschemas_xmlsoap_orgws200303business_processsequence', False, pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 80, 12), )

    
    sequence = property(__sequence.value, __sequence.set, None, None)

    
    # Element {http://schemas.xmlsoap.org/ws/2003/03/business-process/}pick uses Python identifier pick
    __pick = pyxb.binding.content.ElementDeclaration(pyxb.namespace.ExpandedName(Namespace, 'pick'), 'pick', '__httpschemas_xmlsoap_orgws200303business_process_tOnMessage_httpschemas_xmlsoap_orgws200303business_processpick', False, pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 81, 12), )

    
    pick = property(__pick.value, __pick.set, None, None)

    
    # Element {http://schemas.xmlsoap.org/ws/2003/03/business-process/}scope uses Python identifier scope
    __scope = pyxb.binding.content.ElementDeclaration(pyxb.namespace.ExpandedName(Namespace, 'scope'), 'scope', '__httpschemas_xmlsoap_orgws200303business_process_tOnMessage_httpschemas_xmlsoap_orgws200303business_processscope', False, pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 82, 12), )

    
    scope = property(__scope.value, __scope.set, None, None)

    
    # Element {http://schemas.xmlsoap.org/ws/2003/03/business-process/}correlations uses Python identifier correlations
    __correlations = pyxb.binding.content.ElementDeclaration(pyxb.namespace.ExpandedName(Namespace, 'correlations'), 'correlations', '__httpschemas_xmlsoap_orgws200303business_process_tOnMessage_httpschemas_xmlsoap_orgws200303business_processcorrelations', False, pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 205, 20), )

    
    correlations = property(__correlations.value, __correlations.set, None, None)

    
    # Attribute partnerLink uses Python identifier partnerLink
    __partnerLink = pyxb.binding.content.AttributeUse(pyxb.namespace.ExpandedName(None, 'partnerLink'), 'partnerLink', '__httpschemas_xmlsoap_orgws200303business_process_tOnMessage_partnerLink', pyxb.binding.datatypes.NCName, required=True)
    __partnerLink._DeclarationLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 209, 16)
    __partnerLink._UseLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 209, 16)
    
    partnerLink = property(__partnerLink.value, __partnerLink.set, None, None)

    
    # Attribute portType uses Python identifier portType
    __portType = pyxb.binding.content.AttributeUse(pyxb.namespace.ExpandedName(None, 'portType'), 'portType', '__httpschemas_xmlsoap_orgws200303business_process_tOnMessage_portType', pyxb.binding.datatypes.QName, required=True)
    __portType._DeclarationLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 210, 16)
    __portType._UseLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 210, 16)
    
    portType = property(__portType.value, __portType.set, None, None)

    
    # Attribute operation uses Python identifier operation
    __operation = pyxb.binding.content.AttributeUse(pyxb.namespace.ExpandedName(None, 'operation'), 'operation', '__httpschemas_xmlsoap_orgws200303business_process_tOnMessage_operation', pyxb.binding.datatypes.NCName, required=True)
    __operation._DeclarationLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 211, 16)
    __operation._UseLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 211, 16)
    
    operation = property(__operation.value, __operation.set, None, None)

    
    # Attribute variable uses Python identifier variable
    __variable = pyxb.binding.content.AttributeUse(pyxb.namespace.ExpandedName(None, 'variable'), 'variable', '__httpschemas_xmlsoap_orgws200303business_process_tOnMessage_variable', pyxb.binding.datatypes.NCName)
    __variable._DeclarationLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 212, 16)
    __variable._UseLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 212, 16)
    
    variable = property(__variable.value, __variable.set, None, None)

    _AttributeWildcard = pyxb.binding.content.Wildcard(process_contents=pyxb.binding.content.Wildcard.PC_lax, namespace_constraint=(pyxb.binding.content.Wildcard.NC_not, 'http://schemas.xmlsoap.org/ws/2003/03/business-process/'))
    _HasWildcardElement = True
    _ElementMap.update({
        __empty.name() : __empty,
        __invoke.name() : __invoke,
        __receive.name() : __receive,
        __reply.name() : __reply,
        __assign.name() : __assign,
        __wait.name() : __wait,
        __throw.name() : __throw,
        __terminate.name() : __terminate,
        __flow.name() : __flow,
        __switch.name() : __switch,
        __while.name() : __while,
        __sequence.name() : __sequence,
        __pick.name() : __pick,
        __scope.name() : __scope,
        __correlations.name() : __correlations
    })
    _AttributeMap.update({
        __partnerLink.name() : __partnerLink,
        __portType.name() : __portType,
        __operation.name() : __operation,
        __variable.name() : __variable
    })
Namespace.addCategoryObject('typeBinding', 'tOnMessage', tOnMessage)


# Complex type {http://schemas.xmlsoap.org/ws/2003/03/business-process/}tVariables with content type ELEMENT_ONLY
class tVariables (tExtensibleElements):
    """Complex type {http://schemas.xmlsoap.org/ws/2003/03/business-process/}tVariables with content type ELEMENT_ONLY"""
    _TypeDefinition = None
    _ContentTypeTag = pyxb.binding.basis.complexTypeDefinition._CT_ELEMENT_ONLY
    _Abstract = False
    _ExpandedName = pyxb.namespace.ExpandedName(Namespace, 'tVariables')
    _XSDLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 234, 4)
    _ElementMap = tExtensibleElements._ElementMap.copy()
    _AttributeMap = tExtensibleElements._AttributeMap.copy()
    # Base type is tExtensibleElements
    
    # Element {http://schemas.xmlsoap.org/ws/2003/03/business-process/}variable uses Python identifier variable
    __variable = pyxb.binding.content.ElementDeclaration(pyxb.namespace.ExpandedName(Namespace, 'variable'), 'variable', '__httpschemas_xmlsoap_orgws200303business_process_tVariables_httpschemas_xmlsoap_orgws200303business_processvariable', True, pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 238, 20), )

    
    variable = property(__variable.value, __variable.set, None, None)

    _AttributeWildcard = pyxb.binding.content.Wildcard(process_contents=pyxb.binding.content.Wildcard.PC_lax, namespace_constraint=(pyxb.binding.content.Wildcard.NC_not, 'http://schemas.xmlsoap.org/ws/2003/03/business-process/'))
    _HasWildcardElement = True
    _ElementMap.update({
        __variable.name() : __variable
    })
    _AttributeMap.update({
        
    })
Namespace.addCategoryObject('typeBinding', 'tVariables', tVariables)


# Complex type {http://schemas.xmlsoap.org/ws/2003/03/business-process/}tCorrelationSets with content type ELEMENT_ONLY
class tCorrelationSets (tExtensibleElements):
    """Complex type {http://schemas.xmlsoap.org/ws/2003/03/business-process/}tCorrelationSets with content type ELEMENT_ONLY"""
    _TypeDefinition = None
    _ContentTypeTag = pyxb.binding.basis.complexTypeDefinition._CT_ELEMENT_ONLY
    _Abstract = False
    _ExpandedName = pyxb.namespace.ExpandedName(Namespace, 'tCorrelationSets')
    _XSDLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 259, 4)
    _ElementMap = tExtensibleElements._ElementMap.copy()
    _AttributeMap = tExtensibleElements._AttributeMap.copy()
    # Base type is tExtensibleElements
    
    # Element {http://schemas.xmlsoap.org/ws/2003/03/business-process/}correlationSet uses Python identifier correlationSet
    __correlationSet = pyxb.binding.content.ElementDeclaration(pyxb.namespace.ExpandedName(Namespace, 'correlationSet'), 'correlationSet', '__httpschemas_xmlsoap_orgws200303business_process_tCorrelationSets_httpschemas_xmlsoap_orgws200303business_processcorrelationSet', True, pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 263, 20), )

    
    correlationSet = property(__correlationSet.value, __correlationSet.set, None, None)

    _AttributeWildcard = pyxb.binding.content.Wildcard(process_contents=pyxb.binding.content.Wildcard.PC_lax, namespace_constraint=(pyxb.binding.content.Wildcard.NC_not, 'http://schemas.xmlsoap.org/ws/2003/03/business-process/'))
    _HasWildcardElement = True
    _ElementMap.update({
        __correlationSet.name() : __correlationSet
    })
    _AttributeMap.update({
        
    })
Namespace.addCategoryObject('typeBinding', 'tCorrelationSets', tCorrelationSets)


# Complex type {http://schemas.xmlsoap.org/ws/2003/03/business-process/}tCorrelationSet with content type ELEMENT_ONLY
class tCorrelationSet (tExtensibleElements):
    """Complex type {http://schemas.xmlsoap.org/ws/2003/03/business-process/}tCorrelationSet with content type ELEMENT_ONLY"""
    _TypeDefinition = None
    _ContentTypeTag = pyxb.binding.basis.complexTypeDefinition._CT_ELEMENT_ONLY
    _Abstract = False
    _ExpandedName = pyxb.namespace.ExpandedName(Namespace, 'tCorrelationSet')
    _XSDLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 272, 4)
    _ElementMap = tExtensibleElements._ElementMap.copy()
    _AttributeMap = tExtensibleElements._AttributeMap.copy()
    # Base type is tExtensibleElements
    
    # Attribute properties uses Python identifier properties
    __properties = pyxb.binding.content.AttributeUse(pyxb.namespace.ExpandedName(None, 'properties'), 'properties', '__httpschemas_xmlsoap_orgws200303business_process_tCorrelationSet_properties', STD_ANON, required=True)
    __properties._DeclarationLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 275, 16)
    __properties._UseLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 275, 16)
    
    properties = property(__properties.value, __properties.set, None, None)

    
    # Attribute name uses Python identifier name
    __name = pyxb.binding.content.AttributeUse(pyxb.namespace.ExpandedName(None, 'name'), 'name', '__httpschemas_xmlsoap_orgws200303business_process_tCorrelationSet_name', pyxb.binding.datatypes.NCName, required=True)
    __name._DeclarationLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 280, 16)
    __name._UseLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 280, 16)
    
    name = property(__name.value, __name.set, None, None)

    _AttributeWildcard = pyxb.binding.content.Wildcard(process_contents=pyxb.binding.content.Wildcard.PC_lax, namespace_constraint=(pyxb.binding.content.Wildcard.NC_not, 'http://schemas.xmlsoap.org/ws/2003/03/business-process/'))
    _HasWildcardElement = True
    _ElementMap.update({
        
    })
    _AttributeMap.update({
        __properties.name() : __properties,
        __name.name() : __name
    })
Namespace.addCategoryObject('typeBinding', 'tCorrelationSet', tCorrelationSet)


# Complex type {http://schemas.xmlsoap.org/ws/2003/03/business-process/}tActivity with content type ELEMENT_ONLY
class tActivity (tExtensibleElements):
    """Complex type {http://schemas.xmlsoap.org/ws/2003/03/business-process/}tActivity with content type ELEMENT_ONLY"""
    _TypeDefinition = None
    _ContentTypeTag = pyxb.binding.basis.complexTypeDefinition._CT_ELEMENT_ONLY
    _Abstract = False
    _ExpandedName = pyxb.namespace.ExpandedName(Namespace, 'tActivity')
    _XSDLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 286, 4)
    _ElementMap = tExtensibleElements._ElementMap.copy()
    _AttributeMap = tExtensibleElements._AttributeMap.copy()
    # Base type is tExtensibleElements
    
    # Element {http://schemas.xmlsoap.org/ws/2003/03/business-process/}target uses Python identifier target
    __target = pyxb.binding.content.ElementDeclaration(pyxb.namespace.ExpandedName(Namespace, 'target'), 'target', '__httpschemas_xmlsoap_orgws200303business_process_tActivity_httpschemas_xmlsoap_orgws200303business_processtarget', True, pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 290, 20), )

    
    target = property(__target.value, __target.set, None, None)

    
    # Element {http://schemas.xmlsoap.org/ws/2003/03/business-process/}source uses Python identifier source
    __source = pyxb.binding.content.ElementDeclaration(pyxb.namespace.ExpandedName(Namespace, 'source'), 'source', '__httpschemas_xmlsoap_orgws200303business_process_tActivity_httpschemas_xmlsoap_orgws200303business_processsource', True, pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 292, 20), )

    
    source = property(__source.value, __source.set, None, None)

    
    # Attribute name uses Python identifier name
    __name = pyxb.binding.content.AttributeUse(pyxb.namespace.ExpandedName(None, 'name'), 'name', '__httpschemas_xmlsoap_orgws200303business_process_tActivity_name', pyxb.binding.datatypes.NCName)
    __name._DeclarationLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 295, 16)
    __name._UseLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 295, 16)
    
    name = property(__name.value, __name.set, None, None)

    
    # Attribute joinCondition uses Python identifier joinCondition
    __joinCondition = pyxb.binding.content.AttributeUse(pyxb.namespace.ExpandedName(None, 'joinCondition'), 'joinCondition', '__httpschemas_xmlsoap_orgws200303business_process_tActivity_joinCondition', tBoolean_expr)
    __joinCondition._DeclarationLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 296, 16)
    __joinCondition._UseLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 296, 16)
    
    joinCondition = property(__joinCondition.value, __joinCondition.set, None, None)

    
    # Attribute suppressJoinFailure uses Python identifier suppressJoinFailure
    __suppressJoinFailure = pyxb.binding.content.AttributeUse(pyxb.namespace.ExpandedName(None, 'suppressJoinFailure'), 'suppressJoinFailure', '__httpschemas_xmlsoap_orgws200303business_process_tActivity_suppressJoinFailure', tBoolean, unicode_default='no')
    __suppressJoinFailure._DeclarationLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 298, 16)
    __suppressJoinFailure._UseLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 298, 16)
    
    suppressJoinFailure = property(__suppressJoinFailure.value, __suppressJoinFailure.set, None, None)

    _AttributeWildcard = pyxb.binding.content.Wildcard(process_contents=pyxb.binding.content.Wildcard.PC_lax, namespace_constraint=(pyxb.binding.content.Wildcard.NC_not, 'http://schemas.xmlsoap.org/ws/2003/03/business-process/'))
    _HasWildcardElement = True
    _ElementMap.update({
        __target.name() : __target,
        __source.name() : __source
    })
    _AttributeMap.update({
        __name.name() : __name,
        __joinCondition.name() : __joinCondition,
        __suppressJoinFailure.name() : __suppressJoinFailure
    })
Namespace.addCategoryObject('typeBinding', 'tActivity', tActivity)


# Complex type {http://schemas.xmlsoap.org/ws/2003/03/business-process/}tSource with content type ELEMENT_ONLY
class tSource (tExtensibleElements):
    """Complex type {http://schemas.xmlsoap.org/ws/2003/03/business-process/}tSource with content type ELEMENT_ONLY"""
    _TypeDefinition = None
    _ContentTypeTag = pyxb.binding.basis.complexTypeDefinition._CT_ELEMENT_ONLY
    _Abstract = False
    _ExpandedName = pyxb.namespace.ExpandedName(Namespace, 'tSource')
    _XSDLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 305, 4)
    _ElementMap = tExtensibleElements._ElementMap.copy()
    _AttributeMap = tExtensibleElements._AttributeMap.copy()
    # Base type is tExtensibleElements
    
    # Attribute linkName uses Python identifier linkName
    __linkName = pyxb.binding.content.AttributeUse(pyxb.namespace.ExpandedName(None, 'linkName'), 'linkName', '__httpschemas_xmlsoap_orgws200303business_process_tSource_linkName', pyxb.binding.datatypes.NCName, required=True)
    __linkName._DeclarationLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 308, 16)
    __linkName._UseLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 308, 16)
    
    linkName = property(__linkName.value, __linkName.set, None, None)

    
    # Attribute transitionCondition uses Python identifier transitionCondition
    __transitionCondition = pyxb.binding.content.AttributeUse(pyxb.namespace.ExpandedName(None, 'transitionCondition'), 'transitionCondition', '__httpschemas_xmlsoap_orgws200303business_process_tSource_transitionCondition', tBoolean_expr)
    __transitionCondition._DeclarationLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 309, 16)
    __transitionCondition._UseLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 309, 16)
    
    transitionCondition = property(__transitionCondition.value, __transitionCondition.set, None, None)

    _AttributeWildcard = pyxb.binding.content.Wildcard(process_contents=pyxb.binding.content.Wildcard.PC_lax, namespace_constraint=(pyxb.binding.content.Wildcard.NC_not, 'http://schemas.xmlsoap.org/ws/2003/03/business-process/'))
    _HasWildcardElement = True
    _ElementMap.update({
        
    })
    _AttributeMap.update({
        __linkName.name() : __linkName,
        __transitionCondition.name() : __transitionCondition
    })
Namespace.addCategoryObject('typeBinding', 'tSource', tSource)


# Complex type {http://schemas.xmlsoap.org/ws/2003/03/business-process/}tTarget with content type ELEMENT_ONLY
class tTarget (tExtensibleElements):
    """Complex type {http://schemas.xmlsoap.org/ws/2003/03/business-process/}tTarget with content type ELEMENT_ONLY"""
    _TypeDefinition = None
    _ContentTypeTag = pyxb.binding.basis.complexTypeDefinition._CT_ELEMENT_ONLY
    _Abstract = False
    _ExpandedName = pyxb.namespace.ExpandedName(Namespace, 'tTarget')
    _XSDLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 316, 4)
    _ElementMap = tExtensibleElements._ElementMap.copy()
    _AttributeMap = tExtensibleElements._AttributeMap.copy()
    # Base type is tExtensibleElements
    
    # Attribute linkName uses Python identifier linkName
    __linkName = pyxb.binding.content.AttributeUse(pyxb.namespace.ExpandedName(None, 'linkName'), 'linkName', '__httpschemas_xmlsoap_orgws200303business_process_tTarget_linkName', pyxb.binding.datatypes.NCName, required=True)
    __linkName._DeclarationLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 319, 16)
    __linkName._UseLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 319, 16)
    
    linkName = property(__linkName.value, __linkName.set, None, None)

    _AttributeWildcard = pyxb.binding.content.Wildcard(process_contents=pyxb.binding.content.Wildcard.PC_lax, namespace_constraint=(pyxb.binding.content.Wildcard.NC_not, 'http://schemas.xmlsoap.org/ws/2003/03/business-process/'))
    _HasWildcardElement = True
    _ElementMap.update({
        
    })
    _AttributeMap.update({
        __linkName.name() : __linkName
    })
Namespace.addCategoryObject('typeBinding', 'tTarget', tTarget)


# Complex type {http://schemas.xmlsoap.org/ws/2003/03/business-process/}tCorrelations with content type ELEMENT_ONLY
class tCorrelations (tExtensibleElements):
    """Complex type {http://schemas.xmlsoap.org/ws/2003/03/business-process/}tCorrelations with content type ELEMENT_ONLY"""
    _TypeDefinition = None
    _ContentTypeTag = pyxb.binding.basis.complexTypeDefinition._CT_ELEMENT_ONLY
    _Abstract = False
    _ExpandedName = pyxb.namespace.ExpandedName(Namespace, 'tCorrelations')
    _XSDLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 331, 4)
    _ElementMap = tExtensibleElements._ElementMap.copy()
    _AttributeMap = tExtensibleElements._AttributeMap.copy()
    # Base type is tExtensibleElements
    
    # Element {http://schemas.xmlsoap.org/ws/2003/03/business-process/}correlation uses Python identifier correlation
    __correlation = pyxb.binding.content.ElementDeclaration(pyxb.namespace.ExpandedName(Namespace, 'correlation'), 'correlation', '__httpschemas_xmlsoap_orgws200303business_process_tCorrelations_httpschemas_xmlsoap_orgws200303business_processcorrelation', True, pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 335, 18), )

    
    correlation = property(__correlation.value, __correlation.set, None, None)

    _AttributeWildcard = pyxb.binding.content.Wildcard(process_contents=pyxb.binding.content.Wildcard.PC_lax, namespace_constraint=(pyxb.binding.content.Wildcard.NC_not, 'http://schemas.xmlsoap.org/ws/2003/03/business-process/'))
    _HasWildcardElement = True
    _ElementMap.update({
        __correlation.name() : __correlation
    })
    _AttributeMap.update({
        
    })
Namespace.addCategoryObject('typeBinding', 'tCorrelations', tCorrelations)


# Complex type {http://schemas.xmlsoap.org/ws/2003/03/business-process/}tCorrelation with content type ELEMENT_ONLY
class tCorrelation (tExtensibleElements):
    """Complex type {http://schemas.xmlsoap.org/ws/2003/03/business-process/}tCorrelation with content type ELEMENT_ONLY"""
    _TypeDefinition = None
    _ContentTypeTag = pyxb.binding.basis.complexTypeDefinition._CT_ELEMENT_ONLY
    _Abstract = False
    _ExpandedName = pyxb.namespace.ExpandedName(Namespace, 'tCorrelation')
    _XSDLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 342, 3)
    _ElementMap = tExtensibleElements._ElementMap.copy()
    _AttributeMap = tExtensibleElements._AttributeMap.copy()
    # Base type is tExtensibleElements
    
    # Attribute set uses Python identifier set_
    __set = pyxb.binding.content.AttributeUse(pyxb.namespace.ExpandedName(None, 'set'), 'set_', '__httpschemas_xmlsoap_orgws200303business_process_tCorrelation_set', pyxb.binding.datatypes.NCName, required=True)
    __set._DeclarationLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 345, 16)
    __set._UseLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 345, 16)
    
    set_ = property(__set.value, __set.set, None, None)

    
    # Attribute initiate uses Python identifier initiate
    __initiate = pyxb.binding.content.AttributeUse(pyxb.namespace.ExpandedName(None, 'initiate'), 'initiate', '__httpschemas_xmlsoap_orgws200303business_process_tCorrelation_initiate', tBoolean, unicode_default='no')
    __initiate._DeclarationLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 346, 16)
    __initiate._UseLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 346, 16)
    
    initiate = property(__initiate.value, __initiate.set, None, None)

    _AttributeWildcard = pyxb.binding.content.Wildcard(process_contents=pyxb.binding.content.Wildcard.PC_lax, namespace_constraint=(pyxb.binding.content.Wildcard.NC_not, 'http://schemas.xmlsoap.org/ws/2003/03/business-process/'))
    _HasWildcardElement = True
    _ElementMap.update({
        
    })
    _AttributeMap.update({
        __set.name() : __set,
        __initiate.name() : __initiate
    })
Namespace.addCategoryObject('typeBinding', 'tCorrelation', tCorrelation)


# Complex type {http://schemas.xmlsoap.org/ws/2003/03/business-process/}tCorrelationsWithPattern with content type ELEMENT_ONLY
class tCorrelationsWithPattern (tExtensibleElements):
    """Complex type {http://schemas.xmlsoap.org/ws/2003/03/business-process/}tCorrelationsWithPattern with content type ELEMENT_ONLY"""
    _TypeDefinition = None
    _ContentTypeTag = pyxb.binding.basis.complexTypeDefinition._CT_ELEMENT_ONLY
    _Abstract = False
    _ExpandedName = pyxb.namespace.ExpandedName(Namespace, 'tCorrelationsWithPattern')
    _XSDLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 353, 4)
    _ElementMap = tExtensibleElements._ElementMap.copy()
    _AttributeMap = tExtensibleElements._AttributeMap.copy()
    # Base type is tExtensibleElements
    
    # Element {http://schemas.xmlsoap.org/ws/2003/03/business-process/}correlation uses Python identifier correlation
    __correlation = pyxb.binding.content.ElementDeclaration(pyxb.namespace.ExpandedName(Namespace, 'correlation'), 'correlation', '__httpschemas_xmlsoap_orgws200303business_process_tCorrelationsWithPattern_httpschemas_xmlsoap_orgws200303business_processcorrelation', True, pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 357, 20), )

    
    correlation = property(__correlation.value, __correlation.set, None, None)

    _AttributeWildcard = pyxb.binding.content.Wildcard(process_contents=pyxb.binding.content.Wildcard.PC_lax, namespace_constraint=(pyxb.binding.content.Wildcard.NC_not, 'http://schemas.xmlsoap.org/ws/2003/03/business-process/'))
    _HasWildcardElement = True
    _ElementMap.update({
        __correlation.name() : __correlation
    })
    _AttributeMap.update({
        
    })
Namespace.addCategoryObject('typeBinding', 'tCorrelationsWithPattern', tCorrelationsWithPattern)


# Complex type {http://schemas.xmlsoap.org/ws/2003/03/business-process/}tCopy with content type ELEMENT_ONLY
class tCopy (tExtensibleElements):
    """Complex type {http://schemas.xmlsoap.org/ws/2003/03/business-process/}tCopy with content type ELEMENT_ONLY"""
    _TypeDefinition = None
    _ContentTypeTag = pyxb.binding.basis.complexTypeDefinition._CT_ELEMENT_ONLY
    _Abstract = False
    _ExpandedName = pyxb.namespace.ExpandedName(Namespace, 'tCopy')
    _XSDLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 455, 4)
    _ElementMap = tExtensibleElements._ElementMap.copy()
    _AttributeMap = tExtensibleElements._AttributeMap.copy()
    # Base type is tExtensibleElements
    
    # Element {http://schemas.xmlsoap.org/ws/2003/03/business-process/}from uses Python identifier from_
    __from = pyxb.binding.content.ElementDeclaration(pyxb.namespace.ExpandedName(Namespace, 'from'), 'from_', '__httpschemas_xmlsoap_orgws200303business_process_tCopy_httpschemas_xmlsoap_orgws200303business_processfrom', False, pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 466, 4), )

    
    from_ = property(__from.value, __from.set, None, None)

    
    # Element {http://schemas.xmlsoap.org/ws/2003/03/business-process/}to uses Python identifier to
    __to = pyxb.binding.content.ElementDeclaration(pyxb.namespace.ExpandedName(Namespace, 'to'), 'to', '__httpschemas_xmlsoap_orgws200303business_process_tCopy_httpschemas_xmlsoap_orgws200303business_processto', False, pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 482, 4), )

    
    to = property(__to.value, __to.set, None, None)

    _AttributeWildcard = pyxb.binding.content.Wildcard(process_contents=pyxb.binding.content.Wildcard.PC_lax, namespace_constraint=(pyxb.binding.content.Wildcard.NC_not, 'http://schemas.xmlsoap.org/ws/2003/03/business-process/'))
    _HasWildcardElement = True
    _ElementMap.update({
        __from.name() : __from,
        __to.name() : __to
    })
    _AttributeMap.update({
        
    })
Namespace.addCategoryObject('typeBinding', 'tCopy', tCopy)


# Complex type {http://schemas.xmlsoap.org/ws/2003/03/business-process/}tFrom with content type ELEMENT_ONLY
class tFrom (tExtensibleElements):
    """Complex type {http://schemas.xmlsoap.org/ws/2003/03/business-process/}tFrom with content type ELEMENT_ONLY"""
    _TypeDefinition = None
    _ContentTypeTag = pyxb.binding.basis.complexTypeDefinition._CT_ELEMENT_ONLY
    _Abstract = False
    _ExpandedName = pyxb.namespace.ExpandedName(Namespace, 'tFrom')
    _XSDLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 467, 4)
    _ElementMap = tExtensibleElements._ElementMap.copy()
    _AttributeMap = tExtensibleElements._AttributeMap.copy()
    # Base type is tExtensibleElements
    
    # Attribute variable uses Python identifier variable
    __variable = pyxb.binding.content.AttributeUse(pyxb.namespace.ExpandedName(None, 'variable'), 'variable', '__httpschemas_xmlsoap_orgws200303business_process_tFrom_variable', pyxb.binding.datatypes.NCName)
    __variable._DeclarationLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 470, 16)
    __variable._UseLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 470, 16)
    
    variable = property(__variable.value, __variable.set, None, None)

    
    # Attribute part uses Python identifier part
    __part = pyxb.binding.content.AttributeUse(pyxb.namespace.ExpandedName(None, 'part'), 'part', '__httpschemas_xmlsoap_orgws200303business_process_tFrom_part', pyxb.binding.datatypes.NCName)
    __part._DeclarationLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 471, 16)
    __part._UseLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 471, 16)
    
    part = property(__part.value, __part.set, None, None)

    
    # Attribute query uses Python identifier query
    __query = pyxb.binding.content.AttributeUse(pyxb.namespace.ExpandedName(None, 'query'), 'query', '__httpschemas_xmlsoap_orgws200303business_process_tFrom_query', pyxb.binding.datatypes.string)
    __query._DeclarationLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 472, 16)
    __query._UseLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 472, 16)
    
    query = property(__query.value, __query.set, None, None)

    
    # Attribute property uses Python identifier property_
    __property = pyxb.binding.content.AttributeUse(pyxb.namespace.ExpandedName(None, 'property'), 'property_', '__httpschemas_xmlsoap_orgws200303business_process_tFrom_property', pyxb.binding.datatypes.QName)
    __property._DeclarationLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 473, 16)
    __property._UseLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 473, 16)
    
    property_ = property(__property.value, __property.set, None, None)

    
    # Attribute partnerLink uses Python identifier partnerLink
    __partnerLink = pyxb.binding.content.AttributeUse(pyxb.namespace.ExpandedName(None, 'partnerLink'), 'partnerLink', '__httpschemas_xmlsoap_orgws200303business_process_tFrom_partnerLink', pyxb.binding.datatypes.NCName)
    __partnerLink._DeclarationLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 474, 16)
    __partnerLink._UseLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 474, 16)
    
    partnerLink = property(__partnerLink.value, __partnerLink.set, None, None)

    
    # Attribute endpointReference uses Python identifier endpointReference
    __endpointReference = pyxb.binding.content.AttributeUse(pyxb.namespace.ExpandedName(None, 'endpointReference'), 'endpointReference', '__httpschemas_xmlsoap_orgws200303business_process_tFrom_endpointReference', tRoles)
    __endpointReference._DeclarationLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 475, 16)
    __endpointReference._UseLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 475, 16)
    
    endpointReference = property(__endpointReference.value, __endpointReference.set, None, None)

    
    # Attribute expression uses Python identifier expression
    __expression = pyxb.binding.content.AttributeUse(pyxb.namespace.ExpandedName(None, 'expression'), 'expression', '__httpschemas_xmlsoap_orgws200303business_process_tFrom_expression', pyxb.binding.datatypes.string)
    __expression._DeclarationLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 476, 16)
    __expression._UseLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 476, 16)
    
    expression = property(__expression.value, __expression.set, None, None)

    
    # Attribute opaque uses Python identifier opaque
    __opaque = pyxb.binding.content.AttributeUse(pyxb.namespace.ExpandedName(None, 'opaque'), 'opaque', '__httpschemas_xmlsoap_orgws200303business_process_tFrom_opaque', tBoolean)
    __opaque._DeclarationLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 477, 16)
    __opaque._UseLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 477, 16)
    
    opaque = property(__opaque.value, __opaque.set, None, None)

    _AttributeWildcard = pyxb.binding.content.Wildcard(process_contents=pyxb.binding.content.Wildcard.PC_lax, namespace_constraint=(pyxb.binding.content.Wildcard.NC_not, 'http://schemas.xmlsoap.org/ws/2003/03/business-process/'))
    _HasWildcardElement = True
    _ElementMap.update({
        
    })
    _AttributeMap.update({
        __variable.name() : __variable,
        __part.name() : __part,
        __query.name() : __query,
        __property.name() : __property,
        __partnerLink.name() : __partnerLink,
        __endpointReference.name() : __endpointReference,
        __expression.name() : __expression,
        __opaque.name() : __opaque
    })
Namespace.addCategoryObject('typeBinding', 'tFrom', tFrom)


# Complex type {http://schemas.xmlsoap.org/ws/2003/03/business-process/}tLinks with content type ELEMENT_ONLY
class tLinks (tExtensibleElements):
    """Complex type {http://schemas.xmlsoap.org/ws/2003/03/business-process/}tLinks with content type ELEMENT_ONLY"""
    _TypeDefinition = None
    _ContentTypeTag = pyxb.binding.basis.complexTypeDefinition._CT_ELEMENT_ONLY
    _Abstract = False
    _ExpandedName = pyxb.namespace.ExpandedName(Namespace, 'tLinks')
    _XSDLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 543, 4)
    _ElementMap = tExtensibleElements._ElementMap.copy()
    _AttributeMap = tExtensibleElements._AttributeMap.copy()
    # Base type is tExtensibleElements
    
    # Element {http://schemas.xmlsoap.org/ws/2003/03/business-process/}link uses Python identifier link
    __link = pyxb.binding.content.ElementDeclaration(pyxb.namespace.ExpandedName(Namespace, 'link'), 'link', '__httpschemas_xmlsoap_orgws200303business_process_tLinks_httpschemas_xmlsoap_orgws200303business_processlink', True, pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 547, 20), )

    
    link = property(__link.value, __link.set, None, None)

    _AttributeWildcard = pyxb.binding.content.Wildcard(process_contents=pyxb.binding.content.Wildcard.PC_lax, namespace_constraint=(pyxb.binding.content.Wildcard.NC_not, 'http://schemas.xmlsoap.org/ws/2003/03/business-process/'))
    _HasWildcardElement = True
    _ElementMap.update({
        __link.name() : __link
    })
    _AttributeMap.update({
        
    })
Namespace.addCategoryObject('typeBinding', 'tLinks', tLinks)


# Complex type {http://schemas.xmlsoap.org/ws/2003/03/business-process/}tLink with content type ELEMENT_ONLY
class tLink (tExtensibleElements):
    """Complex type {http://schemas.xmlsoap.org/ws/2003/03/business-process/}tLink with content type ELEMENT_ONLY"""
    _TypeDefinition = None
    _ContentTypeTag = pyxb.binding.basis.complexTypeDefinition._CT_ELEMENT_ONLY
    _Abstract = False
    _ExpandedName = pyxb.namespace.ExpandedName(Namespace, 'tLink')
    _XSDLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 555, 4)
    _ElementMap = tExtensibleElements._ElementMap.copy()
    _AttributeMap = tExtensibleElements._AttributeMap.copy()
    # Base type is tExtensibleElements
    
    # Attribute name uses Python identifier name
    __name = pyxb.binding.content.AttributeUse(pyxb.namespace.ExpandedName(None, 'name'), 'name', '__httpschemas_xmlsoap_orgws200303business_process_tLink_name', pyxb.binding.datatypes.NCName, required=True)
    __name._DeclarationLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 558, 16)
    __name._UseLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 558, 16)
    
    name = property(__name.value, __name.set, None, None)

    _AttributeWildcard = pyxb.binding.content.Wildcard(process_contents=pyxb.binding.content.Wildcard.PC_lax, namespace_constraint=(pyxb.binding.content.Wildcard.NC_not, 'http://schemas.xmlsoap.org/ws/2003/03/business-process/'))
    _HasWildcardElement = True
    _ElementMap.update({
        
    })
    _AttributeMap.update({
        __name.name() : __name
    })
Namespace.addCategoryObject('typeBinding', 'tLink', tLink)


# Complex type {http://schemas.xmlsoap.org/ws/2003/03/business-process/}tCatch with content type ELEMENT_ONLY
class tCatch (tActivityOrCompensateContainer):
    """Complex type {http://schemas.xmlsoap.org/ws/2003/03/business-process/}tCatch with content type ELEMENT_ONLY"""
    _TypeDefinition = None
    _ContentTypeTag = pyxb.binding.basis.complexTypeDefinition._CT_ELEMENT_ONLY
    _Abstract = False
    _ExpandedName = pyxb.namespace.ExpandedName(Namespace, 'tCatch')
    _XSDLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 157, 4)
    _ElementMap = tActivityOrCompensateContainer._ElementMap.copy()
    _AttributeMap = tActivityOrCompensateContainer._AttributeMap.copy()
    # Base type is tActivityOrCompensateContainer
    
    # Element empty ({http://schemas.xmlsoap.org/ws/2003/03/business-process/}empty) inherited from {http://schemas.xmlsoap.org/ws/2003/03/business-process/}tActivityOrCompensateContainer
    
    # Element invoke ({http://schemas.xmlsoap.org/ws/2003/03/business-process/}invoke) inherited from {http://schemas.xmlsoap.org/ws/2003/03/business-process/}tActivityOrCompensateContainer
    
    # Element receive ({http://schemas.xmlsoap.org/ws/2003/03/business-process/}receive) inherited from {http://schemas.xmlsoap.org/ws/2003/03/business-process/}tActivityOrCompensateContainer
    
    # Element reply ({http://schemas.xmlsoap.org/ws/2003/03/business-process/}reply) inherited from {http://schemas.xmlsoap.org/ws/2003/03/business-process/}tActivityOrCompensateContainer
    
    # Element assign ({http://schemas.xmlsoap.org/ws/2003/03/business-process/}assign) inherited from {http://schemas.xmlsoap.org/ws/2003/03/business-process/}tActivityOrCompensateContainer
    
    # Element wait ({http://schemas.xmlsoap.org/ws/2003/03/business-process/}wait) inherited from {http://schemas.xmlsoap.org/ws/2003/03/business-process/}tActivityOrCompensateContainer
    
    # Element throw ({http://schemas.xmlsoap.org/ws/2003/03/business-process/}throw) inherited from {http://schemas.xmlsoap.org/ws/2003/03/business-process/}tActivityOrCompensateContainer
    
    # Element terminate ({http://schemas.xmlsoap.org/ws/2003/03/business-process/}terminate) inherited from {http://schemas.xmlsoap.org/ws/2003/03/business-process/}tActivityOrCompensateContainer
    
    # Element flow ({http://schemas.xmlsoap.org/ws/2003/03/business-process/}flow) inherited from {http://schemas.xmlsoap.org/ws/2003/03/business-process/}tActivityOrCompensateContainer
    
    # Element switch ({http://schemas.xmlsoap.org/ws/2003/03/business-process/}switch) inherited from {http://schemas.xmlsoap.org/ws/2003/03/business-process/}tActivityOrCompensateContainer
    
    # Element while_ ({http://schemas.xmlsoap.org/ws/2003/03/business-process/}while) inherited from {http://schemas.xmlsoap.org/ws/2003/03/business-process/}tActivityOrCompensateContainer
    
    # Element sequence ({http://schemas.xmlsoap.org/ws/2003/03/business-process/}sequence) inherited from {http://schemas.xmlsoap.org/ws/2003/03/business-process/}tActivityOrCompensateContainer
    
    # Element pick ({http://schemas.xmlsoap.org/ws/2003/03/business-process/}pick) inherited from {http://schemas.xmlsoap.org/ws/2003/03/business-process/}tActivityOrCompensateContainer
    
    # Element scope ({http://schemas.xmlsoap.org/ws/2003/03/business-process/}scope) inherited from {http://schemas.xmlsoap.org/ws/2003/03/business-process/}tActivityOrCompensateContainer
    
    # Element compensate ({http://schemas.xmlsoap.org/ws/2003/03/business-process/}compensate) inherited from {http://schemas.xmlsoap.org/ws/2003/03/business-process/}tActivityOrCompensateContainer
    
    # Attribute faultName uses Python identifier faultName
    __faultName = pyxb.binding.content.AttributeUse(pyxb.namespace.ExpandedName(None, 'faultName'), 'faultName', '__httpschemas_xmlsoap_orgws200303business_process_tCatch_faultName', pyxb.binding.datatypes.QName)
    __faultName._DeclarationLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 160, 16)
    __faultName._UseLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 160, 16)
    
    faultName = property(__faultName.value, __faultName.set, None, None)

    
    # Attribute faultVariable uses Python identifier faultVariable
    __faultVariable = pyxb.binding.content.AttributeUse(pyxb.namespace.ExpandedName(None, 'faultVariable'), 'faultVariable', '__httpschemas_xmlsoap_orgws200303business_process_tCatch_faultVariable', pyxb.binding.datatypes.NCName)
    __faultVariable._DeclarationLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 161, 16)
    __faultVariable._UseLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 161, 16)
    
    faultVariable = property(__faultVariable.value, __faultVariable.set, None, None)

    _AttributeWildcard = pyxb.binding.content.Wildcard(process_contents=pyxb.binding.content.Wildcard.PC_lax, namespace_constraint=(pyxb.binding.content.Wildcard.NC_not, 'http://schemas.xmlsoap.org/ws/2003/03/business-process/'))
    _HasWildcardElement = True
    _ElementMap.update({
        
    })
    _AttributeMap.update({
        __faultName.name() : __faultName,
        __faultVariable.name() : __faultVariable
    })
Namespace.addCategoryObject('typeBinding', 'tCatch', tCatch)


# Complex type {http://schemas.xmlsoap.org/ws/2003/03/business-process/}tOnAlarm with content type ELEMENT_ONLY
class tOnAlarm (tActivityContainer):
    """Complex type {http://schemas.xmlsoap.org/ws/2003/03/business-process/}tOnAlarm with content type ELEMENT_ONLY"""
    _TypeDefinition = None
    _ContentTypeTag = pyxb.binding.basis.complexTypeDefinition._CT_ELEMENT_ONLY
    _Abstract = False
    _ExpandedName = pyxb.namespace.ExpandedName(Namespace, 'tOnAlarm')
    _XSDLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 219, 4)
    _ElementMap = tActivityContainer._ElementMap.copy()
    _AttributeMap = tActivityContainer._AttributeMap.copy()
    # Base type is tActivityContainer
    
    # Element empty ({http://schemas.xmlsoap.org/ws/2003/03/business-process/}empty) inherited from {http://schemas.xmlsoap.org/ws/2003/03/business-process/}tActivityContainer
    
    # Element invoke ({http://schemas.xmlsoap.org/ws/2003/03/business-process/}invoke) inherited from {http://schemas.xmlsoap.org/ws/2003/03/business-process/}tActivityContainer
    
    # Element receive ({http://schemas.xmlsoap.org/ws/2003/03/business-process/}receive) inherited from {http://schemas.xmlsoap.org/ws/2003/03/business-process/}tActivityContainer
    
    # Element reply ({http://schemas.xmlsoap.org/ws/2003/03/business-process/}reply) inherited from {http://schemas.xmlsoap.org/ws/2003/03/business-process/}tActivityContainer
    
    # Element assign ({http://schemas.xmlsoap.org/ws/2003/03/business-process/}assign) inherited from {http://schemas.xmlsoap.org/ws/2003/03/business-process/}tActivityContainer
    
    # Element wait ({http://schemas.xmlsoap.org/ws/2003/03/business-process/}wait) inherited from {http://schemas.xmlsoap.org/ws/2003/03/business-process/}tActivityContainer
    
    # Element throw ({http://schemas.xmlsoap.org/ws/2003/03/business-process/}throw) inherited from {http://schemas.xmlsoap.org/ws/2003/03/business-process/}tActivityContainer
    
    # Element terminate ({http://schemas.xmlsoap.org/ws/2003/03/business-process/}terminate) inherited from {http://schemas.xmlsoap.org/ws/2003/03/business-process/}tActivityContainer
    
    # Element flow ({http://schemas.xmlsoap.org/ws/2003/03/business-process/}flow) inherited from {http://schemas.xmlsoap.org/ws/2003/03/business-process/}tActivityContainer
    
    # Element switch ({http://schemas.xmlsoap.org/ws/2003/03/business-process/}switch) inherited from {http://schemas.xmlsoap.org/ws/2003/03/business-process/}tActivityContainer
    
    # Element while_ ({http://schemas.xmlsoap.org/ws/2003/03/business-process/}while) inherited from {http://schemas.xmlsoap.org/ws/2003/03/business-process/}tActivityContainer
    
    # Element sequence ({http://schemas.xmlsoap.org/ws/2003/03/business-process/}sequence) inherited from {http://schemas.xmlsoap.org/ws/2003/03/business-process/}tActivityContainer
    
    # Element pick ({http://schemas.xmlsoap.org/ws/2003/03/business-process/}pick) inherited from {http://schemas.xmlsoap.org/ws/2003/03/business-process/}tActivityContainer
    
    # Element scope ({http://schemas.xmlsoap.org/ws/2003/03/business-process/}scope) inherited from {http://schemas.xmlsoap.org/ws/2003/03/business-process/}tActivityContainer
    
    # Attribute for uses Python identifier for_
    __for = pyxb.binding.content.AttributeUse(pyxb.namespace.ExpandedName(None, 'for'), 'for_', '__httpschemas_xmlsoap_orgws200303business_process_tOnAlarm_for', tDuration_expr)
    __for._DeclarationLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 222, 16)
    __for._UseLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 222, 16)
    
    for_ = property(__for.value, __for.set, None, None)

    
    # Attribute until uses Python identifier until
    __until = pyxb.binding.content.AttributeUse(pyxb.namespace.ExpandedName(None, 'until'), 'until', '__httpschemas_xmlsoap_orgws200303business_process_tOnAlarm_until', tDeadline_expr)
    __until._DeclarationLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 223, 16)
    __until._UseLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 223, 16)
    
    until = property(__until.value, __until.set, None, None)

    _AttributeWildcard = pyxb.binding.content.Wildcard(process_contents=pyxb.binding.content.Wildcard.PC_lax, namespace_constraint=(pyxb.binding.content.Wildcard.NC_not, 'http://schemas.xmlsoap.org/ws/2003/03/business-process/'))
    _HasWildcardElement = True
    _ElementMap.update({
        
    })
    _AttributeMap.update({
        __for.name() : __for,
        __until.name() : __until
    })
Namespace.addCategoryObject('typeBinding', 'tOnAlarm', tOnAlarm)


# Complex type {http://schemas.xmlsoap.org/ws/2003/03/business-process/}tCompensationHandler with content type ELEMENT_ONLY
class tCompensationHandler (tActivityOrCompensateContainer):
    """Complex type {http://schemas.xmlsoap.org/ws/2003/03/business-process/}tCompensationHandler with content type ELEMENT_ONLY"""
    _TypeDefinition = None
    _ContentTypeTag = pyxb.binding.basis.complexTypeDefinition._CT_ELEMENT_ONLY
    _Abstract = False
    _ExpandedName = pyxb.namespace.ExpandedName(Namespace, 'tCompensationHandler')
    _XSDLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 228, 4)
    _ElementMap = tActivityOrCompensateContainer._ElementMap.copy()
    _AttributeMap = tActivityOrCompensateContainer._AttributeMap.copy()
    # Base type is tActivityOrCompensateContainer
    
    # Element empty ({http://schemas.xmlsoap.org/ws/2003/03/business-process/}empty) inherited from {http://schemas.xmlsoap.org/ws/2003/03/business-process/}tActivityOrCompensateContainer
    
    # Element invoke ({http://schemas.xmlsoap.org/ws/2003/03/business-process/}invoke) inherited from {http://schemas.xmlsoap.org/ws/2003/03/business-process/}tActivityOrCompensateContainer
    
    # Element receive ({http://schemas.xmlsoap.org/ws/2003/03/business-process/}receive) inherited from {http://schemas.xmlsoap.org/ws/2003/03/business-process/}tActivityOrCompensateContainer
    
    # Element reply ({http://schemas.xmlsoap.org/ws/2003/03/business-process/}reply) inherited from {http://schemas.xmlsoap.org/ws/2003/03/business-process/}tActivityOrCompensateContainer
    
    # Element assign ({http://schemas.xmlsoap.org/ws/2003/03/business-process/}assign) inherited from {http://schemas.xmlsoap.org/ws/2003/03/business-process/}tActivityOrCompensateContainer
    
    # Element wait ({http://schemas.xmlsoap.org/ws/2003/03/business-process/}wait) inherited from {http://schemas.xmlsoap.org/ws/2003/03/business-process/}tActivityOrCompensateContainer
    
    # Element throw ({http://schemas.xmlsoap.org/ws/2003/03/business-process/}throw) inherited from {http://schemas.xmlsoap.org/ws/2003/03/business-process/}tActivityOrCompensateContainer
    
    # Element terminate ({http://schemas.xmlsoap.org/ws/2003/03/business-process/}terminate) inherited from {http://schemas.xmlsoap.org/ws/2003/03/business-process/}tActivityOrCompensateContainer
    
    # Element flow ({http://schemas.xmlsoap.org/ws/2003/03/business-process/}flow) inherited from {http://schemas.xmlsoap.org/ws/2003/03/business-process/}tActivityOrCompensateContainer
    
    # Element switch ({http://schemas.xmlsoap.org/ws/2003/03/business-process/}switch) inherited from {http://schemas.xmlsoap.org/ws/2003/03/business-process/}tActivityOrCompensateContainer
    
    # Element while_ ({http://schemas.xmlsoap.org/ws/2003/03/business-process/}while) inherited from {http://schemas.xmlsoap.org/ws/2003/03/business-process/}tActivityOrCompensateContainer
    
    # Element sequence ({http://schemas.xmlsoap.org/ws/2003/03/business-process/}sequence) inherited from {http://schemas.xmlsoap.org/ws/2003/03/business-process/}tActivityOrCompensateContainer
    
    # Element pick ({http://schemas.xmlsoap.org/ws/2003/03/business-process/}pick) inherited from {http://schemas.xmlsoap.org/ws/2003/03/business-process/}tActivityOrCompensateContainer
    
    # Element scope ({http://schemas.xmlsoap.org/ws/2003/03/business-process/}scope) inherited from {http://schemas.xmlsoap.org/ws/2003/03/business-process/}tActivityOrCompensateContainer
    
    # Element compensate ({http://schemas.xmlsoap.org/ws/2003/03/business-process/}compensate) inherited from {http://schemas.xmlsoap.org/ws/2003/03/business-process/}tActivityOrCompensateContainer
    _AttributeWildcard = pyxb.binding.content.Wildcard(process_contents=pyxb.binding.content.Wildcard.PC_lax, namespace_constraint=(pyxb.binding.content.Wildcard.NC_not, 'http://schemas.xmlsoap.org/ws/2003/03/business-process/'))
    _HasWildcardElement = True
    _ElementMap.update({
        
    })
    _AttributeMap.update({
        
    })
Namespace.addCategoryObject('typeBinding', 'tCompensationHandler', tCompensationHandler)


# Complex type {http://schemas.xmlsoap.org/ws/2003/03/business-process/}tEmpty with content type ELEMENT_ONLY
class tEmpty (tActivity):
    """Complex type {http://schemas.xmlsoap.org/ws/2003/03/business-process/}tEmpty with content type ELEMENT_ONLY"""
    _TypeDefinition = None
    _ContentTypeTag = pyxb.binding.basis.complexTypeDefinition._CT_ELEMENT_ONLY
    _Abstract = False
    _ExpandedName = pyxb.namespace.ExpandedName(Namespace, 'tEmpty')
    _XSDLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 325, 4)
    _ElementMap = tActivity._ElementMap.copy()
    _AttributeMap = tActivity._AttributeMap.copy()
    # Base type is tActivity
    
    # Element target ({http://schemas.xmlsoap.org/ws/2003/03/business-process/}target) inherited from {http://schemas.xmlsoap.org/ws/2003/03/business-process/}tActivity
    
    # Element source ({http://schemas.xmlsoap.org/ws/2003/03/business-process/}source) inherited from {http://schemas.xmlsoap.org/ws/2003/03/business-process/}tActivity
    
    # Attribute name inherited from {http://schemas.xmlsoap.org/ws/2003/03/business-process/}tActivity
    
    # Attribute joinCondition inherited from {http://schemas.xmlsoap.org/ws/2003/03/business-process/}tActivity
    
    # Attribute suppressJoinFailure inherited from {http://schemas.xmlsoap.org/ws/2003/03/business-process/}tActivity
    _AttributeWildcard = pyxb.binding.content.Wildcard(process_contents=pyxb.binding.content.Wildcard.PC_lax, namespace_constraint=(pyxb.binding.content.Wildcard.NC_not, 'http://schemas.xmlsoap.org/ws/2003/03/business-process/'))
    _HasWildcardElement = True
    _ElementMap.update({
        
    })
    _AttributeMap.update({
        
    })
Namespace.addCategoryObject('typeBinding', 'tEmpty', tEmpty)


# Complex type {http://schemas.xmlsoap.org/ws/2003/03/business-process/}tCorrelationWithPattern with content type ELEMENT_ONLY
class tCorrelationWithPattern (tCorrelation):
    """Complex type {http://schemas.xmlsoap.org/ws/2003/03/business-process/}tCorrelationWithPattern with content type ELEMENT_ONLY"""
    _TypeDefinition = None
    _ContentTypeTag = pyxb.binding.basis.complexTypeDefinition._CT_ELEMENT_ONLY
    _Abstract = False
    _ExpandedName = pyxb.namespace.ExpandedName(Namespace, 'tCorrelationWithPattern')
    _XSDLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 366, 4)
    _ElementMap = tCorrelation._ElementMap.copy()
    _AttributeMap = tCorrelation._AttributeMap.copy()
    # Base type is tCorrelation
    
    # Attribute set_ inherited from {http://schemas.xmlsoap.org/ws/2003/03/business-process/}tCorrelation
    
    # Attribute initiate inherited from {http://schemas.xmlsoap.org/ws/2003/03/business-process/}tCorrelation
    
    # Attribute pattern uses Python identifier pattern
    __pattern = pyxb.binding.content.AttributeUse(pyxb.namespace.ExpandedName(None, 'pattern'), 'pattern', '__httpschemas_xmlsoap_orgws200303business_process_tCorrelationWithPattern_pattern', STD_ANON_)
    __pattern._DeclarationLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 369, 16)
    __pattern._UseLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 369, 16)
    
    pattern = property(__pattern.value, __pattern.set, None, None)

    _AttributeWildcard = pyxb.binding.content.Wildcard(process_contents=pyxb.binding.content.Wildcard.PC_lax, namespace_constraint=(pyxb.binding.content.Wildcard.NC_not, 'http://schemas.xmlsoap.org/ws/2003/03/business-process/'))
    _HasWildcardElement = True
    _ElementMap.update({
        
    })
    _AttributeMap.update({
        __pattern.name() : __pattern
    })
Namespace.addCategoryObject('typeBinding', 'tCorrelationWithPattern', tCorrelationWithPattern)


# Complex type {http://schemas.xmlsoap.org/ws/2003/03/business-process/}tInvoke with content type ELEMENT_ONLY
class tInvoke (tActivity):
    """Complex type {http://schemas.xmlsoap.org/ws/2003/03/business-process/}tInvoke with content type ELEMENT_ONLY"""
    _TypeDefinition = None
    _ContentTypeTag = pyxb.binding.basis.complexTypeDefinition._CT_ELEMENT_ONLY
    _Abstract = False
    _ExpandedName = pyxb.namespace.ExpandedName(Namespace, 'tInvoke')
    _XSDLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 384, 4)
    _ElementMap = tActivity._ElementMap.copy()
    _AttributeMap = tActivity._AttributeMap.copy()
    # Base type is tActivity
    
    # Element target ({http://schemas.xmlsoap.org/ws/2003/03/business-process/}target) inherited from {http://schemas.xmlsoap.org/ws/2003/03/business-process/}tActivity
    
    # Element source ({http://schemas.xmlsoap.org/ws/2003/03/business-process/}source) inherited from {http://schemas.xmlsoap.org/ws/2003/03/business-process/}tActivity
    
    # Element {http://schemas.xmlsoap.org/ws/2003/03/business-process/}correlations uses Python identifier correlations
    __correlations = pyxb.binding.content.ElementDeclaration(pyxb.namespace.ExpandedName(Namespace, 'correlations'), 'correlations', '__httpschemas_xmlsoap_orgws200303business_process_tInvoke_httpschemas_xmlsoap_orgws200303business_processcorrelations', False, pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 388, 20), )

    
    correlations = property(__correlations.value, __correlations.set, None, None)

    
    # Element {http://schemas.xmlsoap.org/ws/2003/03/business-process/}catch uses Python identifier catch
    __catch = pyxb.binding.content.ElementDeclaration(pyxb.namespace.ExpandedName(Namespace, 'catch'), 'catch', '__httpschemas_xmlsoap_orgws200303business_process_tInvoke_httpschemas_xmlsoap_orgws200303business_processcatch', True, pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 391, 20), )

    
    catch = property(__catch.value, __catch.set, None, None)

    
    # Element {http://schemas.xmlsoap.org/ws/2003/03/business-process/}catchAll uses Python identifier catchAll
    __catchAll = pyxb.binding.content.ElementDeclaration(pyxb.namespace.ExpandedName(Namespace, 'catchAll'), 'catchAll', '__httpschemas_xmlsoap_orgws200303business_process_tInvoke_httpschemas_xmlsoap_orgws200303business_processcatchAll', False, pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 393, 20), )

    
    catchAll = property(__catchAll.value, __catchAll.set, None, None)

    
    # Element {http://schemas.xmlsoap.org/ws/2003/03/business-process/}compensationHandler uses Python identifier compensationHandler
    __compensationHandler = pyxb.binding.content.ElementDeclaration(pyxb.namespace.ExpandedName(Namespace, 'compensationHandler'), 'compensationHandler', '__httpschemas_xmlsoap_orgws200303business_process_tInvoke_httpschemas_xmlsoap_orgws200303business_processcompensationHandler', False, pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 396, 20), )

    
    compensationHandler = property(__compensationHandler.value, __compensationHandler.set, None, None)

    
    # Attribute name inherited from {http://schemas.xmlsoap.org/ws/2003/03/business-process/}tActivity
    
    # Attribute joinCondition inherited from {http://schemas.xmlsoap.org/ws/2003/03/business-process/}tActivity
    
    # Attribute suppressJoinFailure inherited from {http://schemas.xmlsoap.org/ws/2003/03/business-process/}tActivity
    
    # Attribute partnerLink uses Python identifier partnerLink
    __partnerLink = pyxb.binding.content.AttributeUse(pyxb.namespace.ExpandedName(None, 'partnerLink'), 'partnerLink', '__httpschemas_xmlsoap_orgws200303business_process_tInvoke_partnerLink', pyxb.binding.datatypes.NCName, required=True)
    __partnerLink._DeclarationLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 399, 16)
    __partnerLink._UseLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 399, 16)
    
    partnerLink = property(__partnerLink.value, __partnerLink.set, None, None)

    
    # Attribute portType uses Python identifier portType
    __portType = pyxb.binding.content.AttributeUse(pyxb.namespace.ExpandedName(None, 'portType'), 'portType', '__httpschemas_xmlsoap_orgws200303business_process_tInvoke_portType', pyxb.binding.datatypes.QName, required=True)
    __portType._DeclarationLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 400, 16)
    __portType._UseLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 400, 16)
    
    portType = property(__portType.value, __portType.set, None, None)

    
    # Attribute operation uses Python identifier operation
    __operation = pyxb.binding.content.AttributeUse(pyxb.namespace.ExpandedName(None, 'operation'), 'operation', '__httpschemas_xmlsoap_orgws200303business_process_tInvoke_operation', pyxb.binding.datatypes.NCName, required=True)
    __operation._DeclarationLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 401, 16)
    __operation._UseLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 401, 16)
    
    operation = property(__operation.value, __operation.set, None, None)

    
    # Attribute inputVariable uses Python identifier inputVariable
    __inputVariable = pyxb.binding.content.AttributeUse(pyxb.namespace.ExpandedName(None, 'inputVariable'), 'inputVariable', '__httpschemas_xmlsoap_orgws200303business_process_tInvoke_inputVariable', pyxb.binding.datatypes.NCName)
    __inputVariable._DeclarationLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 402, 16)
    __inputVariable._UseLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 402, 16)
    
    inputVariable = property(__inputVariable.value, __inputVariable.set, None, None)

    
    # Attribute outputVariable uses Python identifier outputVariable
    __outputVariable = pyxb.binding.content.AttributeUse(pyxb.namespace.ExpandedName(None, 'outputVariable'), 'outputVariable', '__httpschemas_xmlsoap_orgws200303business_process_tInvoke_outputVariable', pyxb.binding.datatypes.NCName)
    __outputVariable._DeclarationLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 404, 16)
    __outputVariable._UseLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 404, 16)
    
    outputVariable = property(__outputVariable.value, __outputVariable.set, None, None)

    _AttributeWildcard = pyxb.binding.content.Wildcard(process_contents=pyxb.binding.content.Wildcard.PC_lax, namespace_constraint=(pyxb.binding.content.Wildcard.NC_not, 'http://schemas.xmlsoap.org/ws/2003/03/business-process/'))
    _HasWildcardElement = True
    _ElementMap.update({
        __correlations.name() : __correlations,
        __catch.name() : __catch,
        __catchAll.name() : __catchAll,
        __compensationHandler.name() : __compensationHandler
    })
    _AttributeMap.update({
        __partnerLink.name() : __partnerLink,
        __portType.name() : __portType,
        __operation.name() : __operation,
        __inputVariable.name() : __inputVariable,
        __outputVariable.name() : __outputVariable
    })
Namespace.addCategoryObject('typeBinding', 'tInvoke', tInvoke)


# Complex type {http://schemas.xmlsoap.org/ws/2003/03/business-process/}tReceive with content type ELEMENT_ONLY
class tReceive (tActivity):
    """Complex type {http://schemas.xmlsoap.org/ws/2003/03/business-process/}tReceive with content type ELEMENT_ONLY"""
    _TypeDefinition = None
    _ContentTypeTag = pyxb.binding.basis.complexTypeDefinition._CT_ELEMENT_ONLY
    _Abstract = False
    _ExpandedName = pyxb.namespace.ExpandedName(Namespace, 'tReceive')
    _XSDLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 410, 4)
    _ElementMap = tActivity._ElementMap.copy()
    _AttributeMap = tActivity._AttributeMap.copy()
    # Base type is tActivity
    
    # Element target ({http://schemas.xmlsoap.org/ws/2003/03/business-process/}target) inherited from {http://schemas.xmlsoap.org/ws/2003/03/business-process/}tActivity
    
    # Element source ({http://schemas.xmlsoap.org/ws/2003/03/business-process/}source) inherited from {http://schemas.xmlsoap.org/ws/2003/03/business-process/}tActivity
    
    # Element {http://schemas.xmlsoap.org/ws/2003/03/business-process/}correlations uses Python identifier correlations
    __correlations = pyxb.binding.content.ElementDeclaration(pyxb.namespace.ExpandedName(Namespace, 'correlations'), 'correlations', '__httpschemas_xmlsoap_orgws200303business_process_tReceive_httpschemas_xmlsoap_orgws200303business_processcorrelations', False, pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 414, 20), )

    
    correlations = property(__correlations.value, __correlations.set, None, None)

    
    # Attribute name inherited from {http://schemas.xmlsoap.org/ws/2003/03/business-process/}tActivity
    
    # Attribute joinCondition inherited from {http://schemas.xmlsoap.org/ws/2003/03/business-process/}tActivity
    
    # Attribute suppressJoinFailure inherited from {http://schemas.xmlsoap.org/ws/2003/03/business-process/}tActivity
    
    # Attribute partnerLink uses Python identifier partnerLink
    __partnerLink = pyxb.binding.content.AttributeUse(pyxb.namespace.ExpandedName(None, 'partnerLink'), 'partnerLink', '__httpschemas_xmlsoap_orgws200303business_process_tReceive_partnerLink', pyxb.binding.datatypes.NCName, required=True)
    __partnerLink._DeclarationLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 417, 16)
    __partnerLink._UseLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 417, 16)
    
    partnerLink = property(__partnerLink.value, __partnerLink.set, None, None)

    
    # Attribute portType uses Python identifier portType
    __portType = pyxb.binding.content.AttributeUse(pyxb.namespace.ExpandedName(None, 'portType'), 'portType', '__httpschemas_xmlsoap_orgws200303business_process_tReceive_portType', pyxb.binding.datatypes.QName, required=True)
    __portType._DeclarationLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 418, 16)
    __portType._UseLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 418, 16)
    
    portType = property(__portType.value, __portType.set, None, None)

    
    # Attribute operation uses Python identifier operation
    __operation = pyxb.binding.content.AttributeUse(pyxb.namespace.ExpandedName(None, 'operation'), 'operation', '__httpschemas_xmlsoap_orgws200303business_process_tReceive_operation', pyxb.binding.datatypes.NCName, required=True)
    __operation._DeclarationLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 419, 16)
    __operation._UseLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 419, 16)
    
    operation = property(__operation.value, __operation.set, None, None)

    
    # Attribute variable uses Python identifier variable
    __variable = pyxb.binding.content.AttributeUse(pyxb.namespace.ExpandedName(None, 'variable'), 'variable', '__httpschemas_xmlsoap_orgws200303business_process_tReceive_variable', pyxb.binding.datatypes.NCName)
    __variable._DeclarationLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 420, 16)
    __variable._UseLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 420, 16)
    
    variable = property(__variable.value, __variable.set, None, None)

    
    # Attribute createInstance uses Python identifier createInstance
    __createInstance = pyxb.binding.content.AttributeUse(pyxb.namespace.ExpandedName(None, 'createInstance'), 'createInstance', '__httpschemas_xmlsoap_orgws200303business_process_tReceive_createInstance', tBoolean, unicode_default='no')
    __createInstance._DeclarationLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 421, 16)
    __createInstance._UseLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 421, 16)
    
    createInstance = property(__createInstance.value, __createInstance.set, None, None)

    _AttributeWildcard = pyxb.binding.content.Wildcard(process_contents=pyxb.binding.content.Wildcard.PC_lax, namespace_constraint=(pyxb.binding.content.Wildcard.NC_not, 'http://schemas.xmlsoap.org/ws/2003/03/business-process/'))
    _HasWildcardElement = True
    _ElementMap.update({
        __correlations.name() : __correlations
    })
    _AttributeMap.update({
        __partnerLink.name() : __partnerLink,
        __portType.name() : __portType,
        __operation.name() : __operation,
        __variable.name() : __variable,
        __createInstance.name() : __createInstance
    })
Namespace.addCategoryObject('typeBinding', 'tReceive', tReceive)


# Complex type {http://schemas.xmlsoap.org/ws/2003/03/business-process/}tReply with content type ELEMENT_ONLY
class tReply (tActivity):
    """Complex type {http://schemas.xmlsoap.org/ws/2003/03/business-process/}tReply with content type ELEMENT_ONLY"""
    _TypeDefinition = None
    _ContentTypeTag = pyxb.binding.basis.complexTypeDefinition._CT_ELEMENT_ONLY
    _Abstract = False
    _ExpandedName = pyxb.namespace.ExpandedName(Namespace, 'tReply')
    _XSDLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 427, 4)
    _ElementMap = tActivity._ElementMap.copy()
    _AttributeMap = tActivity._AttributeMap.copy()
    # Base type is tActivity
    
    # Element target ({http://schemas.xmlsoap.org/ws/2003/03/business-process/}target) inherited from {http://schemas.xmlsoap.org/ws/2003/03/business-process/}tActivity
    
    # Element source ({http://schemas.xmlsoap.org/ws/2003/03/business-process/}source) inherited from {http://schemas.xmlsoap.org/ws/2003/03/business-process/}tActivity
    
    # Element {http://schemas.xmlsoap.org/ws/2003/03/business-process/}correlations uses Python identifier correlations
    __correlations = pyxb.binding.content.ElementDeclaration(pyxb.namespace.ExpandedName(Namespace, 'correlations'), 'correlations', '__httpschemas_xmlsoap_orgws200303business_process_tReply_httpschemas_xmlsoap_orgws200303business_processcorrelations', False, pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 431, 20), )

    
    correlations = property(__correlations.value, __correlations.set, None, None)

    
    # Attribute name inherited from {http://schemas.xmlsoap.org/ws/2003/03/business-process/}tActivity
    
    # Attribute joinCondition inherited from {http://schemas.xmlsoap.org/ws/2003/03/business-process/}tActivity
    
    # Attribute suppressJoinFailure inherited from {http://schemas.xmlsoap.org/ws/2003/03/business-process/}tActivity
    
    # Attribute partnerLink uses Python identifier partnerLink
    __partnerLink = pyxb.binding.content.AttributeUse(pyxb.namespace.ExpandedName(None, 'partnerLink'), 'partnerLink', '__httpschemas_xmlsoap_orgws200303business_process_tReply_partnerLink', pyxb.binding.datatypes.NCName, required=True)
    __partnerLink._DeclarationLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 434, 16)
    __partnerLink._UseLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 434, 16)
    
    partnerLink = property(__partnerLink.value, __partnerLink.set, None, None)

    
    # Attribute portType uses Python identifier portType
    __portType = pyxb.binding.content.AttributeUse(pyxb.namespace.ExpandedName(None, 'portType'), 'portType', '__httpschemas_xmlsoap_orgws200303business_process_tReply_portType', pyxb.binding.datatypes.QName, required=True)
    __portType._DeclarationLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 435, 16)
    __portType._UseLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 435, 16)
    
    portType = property(__portType.value, __portType.set, None, None)

    
    # Attribute operation uses Python identifier operation
    __operation = pyxb.binding.content.AttributeUse(pyxb.namespace.ExpandedName(None, 'operation'), 'operation', '__httpschemas_xmlsoap_orgws200303business_process_tReply_operation', pyxb.binding.datatypes.NCName, required=True)
    __operation._DeclarationLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 436, 16)
    __operation._UseLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 436, 16)
    
    operation = property(__operation.value, __operation.set, None, None)

    
    # Attribute variable uses Python identifier variable
    __variable = pyxb.binding.content.AttributeUse(pyxb.namespace.ExpandedName(None, 'variable'), 'variable', '__httpschemas_xmlsoap_orgws200303business_process_tReply_variable', pyxb.binding.datatypes.NCName)
    __variable._DeclarationLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 437, 16)
    __variable._UseLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 437, 16)
    
    variable = property(__variable.value, __variable.set, None, None)

    
    # Attribute faultName uses Python identifier faultName
    __faultName = pyxb.binding.content.AttributeUse(pyxb.namespace.ExpandedName(None, 'faultName'), 'faultName', '__httpschemas_xmlsoap_orgws200303business_process_tReply_faultName', pyxb.binding.datatypes.QName)
    __faultName._DeclarationLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 439, 16)
    __faultName._UseLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 439, 16)
    
    faultName = property(__faultName.value, __faultName.set, None, None)

    _AttributeWildcard = pyxb.binding.content.Wildcard(process_contents=pyxb.binding.content.Wildcard.PC_lax, namespace_constraint=(pyxb.binding.content.Wildcard.NC_not, 'http://schemas.xmlsoap.org/ws/2003/03/business-process/'))
    _HasWildcardElement = True
    _ElementMap.update({
        __correlations.name() : __correlations
    })
    _AttributeMap.update({
        __partnerLink.name() : __partnerLink,
        __portType.name() : __portType,
        __operation.name() : __operation,
        __variable.name() : __variable,
        __faultName.name() : __faultName
    })
Namespace.addCategoryObject('typeBinding', 'tReply', tReply)


# Complex type {http://schemas.xmlsoap.org/ws/2003/03/business-process/}tAssign with content type ELEMENT_ONLY
class tAssign (tActivity):
    """Complex type {http://schemas.xmlsoap.org/ws/2003/03/business-process/}tAssign with content type ELEMENT_ONLY"""
    _TypeDefinition = None
    _ContentTypeTag = pyxb.binding.basis.complexTypeDefinition._CT_ELEMENT_ONLY
    _Abstract = False
    _ExpandedName = pyxb.namespace.ExpandedName(Namespace, 'tAssign')
    _XSDLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 444, 4)
    _ElementMap = tActivity._ElementMap.copy()
    _AttributeMap = tActivity._AttributeMap.copy()
    # Base type is tActivity
    
    # Element target ({http://schemas.xmlsoap.org/ws/2003/03/business-process/}target) inherited from {http://schemas.xmlsoap.org/ws/2003/03/business-process/}tActivity
    
    # Element source ({http://schemas.xmlsoap.org/ws/2003/03/business-process/}source) inherited from {http://schemas.xmlsoap.org/ws/2003/03/business-process/}tActivity
    
    # Element {http://schemas.xmlsoap.org/ws/2003/03/business-process/}copy uses Python identifier copy
    __copy = pyxb.binding.content.ElementDeclaration(pyxb.namespace.ExpandedName(Namespace, 'copy'), 'copy', '__httpschemas_xmlsoap_orgws200303business_process_tAssign_httpschemas_xmlsoap_orgws200303business_processcopy', True, pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 448, 20), )

    
    copy = property(__copy.value, __copy.set, None, None)

    
    # Attribute name inherited from {http://schemas.xmlsoap.org/ws/2003/03/business-process/}tActivity
    
    # Attribute joinCondition inherited from {http://schemas.xmlsoap.org/ws/2003/03/business-process/}tActivity
    
    # Attribute suppressJoinFailure inherited from {http://schemas.xmlsoap.org/ws/2003/03/business-process/}tActivity
    _AttributeWildcard = pyxb.binding.content.Wildcard(process_contents=pyxb.binding.content.Wildcard.PC_lax, namespace_constraint=(pyxb.binding.content.Wildcard.NC_not, 'http://schemas.xmlsoap.org/ws/2003/03/business-process/'))
    _HasWildcardElement = True
    _ElementMap.update({
        __copy.name() : __copy
    })
    _AttributeMap.update({
        
    })
Namespace.addCategoryObject('typeBinding', 'tAssign', tAssign)


# Complex type [anonymous] with content type EMPTY
class CTD_ANON_ (tFrom):
    """Complex type [anonymous] with content type EMPTY"""
    _TypeDefinition = None
    _ContentTypeTag = pyxb.binding.basis.complexTypeDefinition._CT_EMPTY
    _Abstract = False
    _ExpandedName = None
    _XSDLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 483, 8)
    _ElementMap = tFrom._ElementMap.copy()
    _AttributeMap = tFrom._AttributeMap.copy()
    # Base type is tFrom
    
    # Attribute variable inherited from {http://schemas.xmlsoap.org/ws/2003/03/business-process/}tFrom
    
    # Attribute part inherited from {http://schemas.xmlsoap.org/ws/2003/03/business-process/}tFrom
    
    # Attribute query inherited from {http://schemas.xmlsoap.org/ws/2003/03/business-process/}tFrom
    
    # Attribute property_ inherited from {http://schemas.xmlsoap.org/ws/2003/03/business-process/}tFrom
    
    # Attribute partnerLink inherited from {http://schemas.xmlsoap.org/ws/2003/03/business-process/}tFrom
    
    # Attribute expression is restricted from parent
    
    # Attribute expression uses Python identifier expression
    __expression = pyxb.binding.content.AttributeUse(pyxb.namespace.ExpandedName(None, 'expression'), 'expression', '__httpschemas_xmlsoap_orgws200303business_process_tFrom_expression', pyxb.binding.datatypes.string, prohibited=True)
    __expression._DeclarationLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 486, 20)
    __expression._UseLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 486, 20)
    
    expression = property(__expression.value, __expression.set, None, None)

    
    # Attribute opaque is restricted from parent
    
    # Attribute opaque uses Python identifier opaque
    __opaque = pyxb.binding.content.AttributeUse(pyxb.namespace.ExpandedName(None, 'opaque'), 'opaque', '__httpschemas_xmlsoap_orgws200303business_process_tFrom_opaque', tBoolean, prohibited=True)
    __opaque._DeclarationLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 488, 20)
    __opaque._UseLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 488, 20)
    
    opaque = property(__opaque.value, __opaque.set, None, None)

    
    # Attribute endpointReference is restricted from parent
    
    # Attribute endpointReference uses Python identifier endpointReference
    __endpointReference = pyxb.binding.content.AttributeUse(pyxb.namespace.ExpandedName(None, 'endpointReference'), 'endpointReference', '__httpschemas_xmlsoap_orgws200303business_process_tFrom_endpointReference', tRoles, prohibited=True)
    __endpointReference._DeclarationLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 490, 20)
    __endpointReference._UseLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 490, 20)
    
    endpointReference = property(__endpointReference.value, __endpointReference.set, None, None)

    _ElementMap.update({
        
    })
    _AttributeMap.update({
        __expression.name() : __expression,
        __opaque.name() : __opaque,
        __endpointReference.name() : __endpointReference
    })



# Complex type {http://schemas.xmlsoap.org/ws/2003/03/business-process/}tWait with content type ELEMENT_ONLY
class tWait (tActivity):
    """Complex type {http://schemas.xmlsoap.org/ws/2003/03/business-process/}tWait with content type ELEMENT_ONLY"""
    _TypeDefinition = None
    _ContentTypeTag = pyxb.binding.basis.complexTypeDefinition._CT_ELEMENT_ONLY
    _Abstract = False
    _ExpandedName = pyxb.namespace.ExpandedName(Namespace, 'tWait')
    _XSDLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 498, 4)
    _ElementMap = tActivity._ElementMap.copy()
    _AttributeMap = tActivity._AttributeMap.copy()
    # Base type is tActivity
    
    # Element target ({http://schemas.xmlsoap.org/ws/2003/03/business-process/}target) inherited from {http://schemas.xmlsoap.org/ws/2003/03/business-process/}tActivity
    
    # Element source ({http://schemas.xmlsoap.org/ws/2003/03/business-process/}source) inherited from {http://schemas.xmlsoap.org/ws/2003/03/business-process/}tActivity
    
    # Attribute name inherited from {http://schemas.xmlsoap.org/ws/2003/03/business-process/}tActivity
    
    # Attribute joinCondition inherited from {http://schemas.xmlsoap.org/ws/2003/03/business-process/}tActivity
    
    # Attribute suppressJoinFailure inherited from {http://schemas.xmlsoap.org/ws/2003/03/business-process/}tActivity
    
    # Attribute for uses Python identifier for_
    __for = pyxb.binding.content.AttributeUse(pyxb.namespace.ExpandedName(None, 'for'), 'for_', '__httpschemas_xmlsoap_orgws200303business_process_tWait_for', tDuration_expr)
    __for._DeclarationLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 501, 16)
    __for._UseLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 501, 16)
    
    for_ = property(__for.value, __for.set, None, None)

    
    # Attribute until uses Python identifier until
    __until = pyxb.binding.content.AttributeUse(pyxb.namespace.ExpandedName(None, 'until'), 'until', '__httpschemas_xmlsoap_orgws200303business_process_tWait_until', tDeadline_expr)
    __until._DeclarationLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 503, 16)
    __until._UseLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 503, 16)
    
    until = property(__until.value, __until.set, None, None)

    _AttributeWildcard = pyxb.binding.content.Wildcard(process_contents=pyxb.binding.content.Wildcard.PC_lax, namespace_constraint=(pyxb.binding.content.Wildcard.NC_not, 'http://schemas.xmlsoap.org/ws/2003/03/business-process/'))
    _HasWildcardElement = True
    _ElementMap.update({
        
    })
    _AttributeMap.update({
        __for.name() : __for,
        __until.name() : __until
    })
Namespace.addCategoryObject('typeBinding', 'tWait', tWait)


# Complex type {http://schemas.xmlsoap.org/ws/2003/03/business-process/}tThrow with content type ELEMENT_ONLY
class tThrow (tActivity):
    """Complex type {http://schemas.xmlsoap.org/ws/2003/03/business-process/}tThrow with content type ELEMENT_ONLY"""
    _TypeDefinition = None
    _ContentTypeTag = pyxb.binding.basis.complexTypeDefinition._CT_ELEMENT_ONLY
    _Abstract = False
    _ExpandedName = pyxb.namespace.ExpandedName(Namespace, 'tThrow')
    _XSDLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 509, 4)
    _ElementMap = tActivity._ElementMap.copy()
    _AttributeMap = tActivity._AttributeMap.copy()
    # Base type is tActivity
    
    # Element target ({http://schemas.xmlsoap.org/ws/2003/03/business-process/}target) inherited from {http://schemas.xmlsoap.org/ws/2003/03/business-process/}tActivity
    
    # Element source ({http://schemas.xmlsoap.org/ws/2003/03/business-process/}source) inherited from {http://schemas.xmlsoap.org/ws/2003/03/business-process/}tActivity
    
    # Attribute name inherited from {http://schemas.xmlsoap.org/ws/2003/03/business-process/}tActivity
    
    # Attribute joinCondition inherited from {http://schemas.xmlsoap.org/ws/2003/03/business-process/}tActivity
    
    # Attribute suppressJoinFailure inherited from {http://schemas.xmlsoap.org/ws/2003/03/business-process/}tActivity
    
    # Attribute faultName uses Python identifier faultName
    __faultName = pyxb.binding.content.AttributeUse(pyxb.namespace.ExpandedName(None, 'faultName'), 'faultName', '__httpschemas_xmlsoap_orgws200303business_process_tThrow_faultName', pyxb.binding.datatypes.QName, required=True)
    __faultName._DeclarationLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 512, 16)
    __faultName._UseLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 512, 16)
    
    faultName = property(__faultName.value, __faultName.set, None, None)

    
    # Attribute faultVariable uses Python identifier faultVariable
    __faultVariable = pyxb.binding.content.AttributeUse(pyxb.namespace.ExpandedName(None, 'faultVariable'), 'faultVariable', '__httpschemas_xmlsoap_orgws200303business_process_tThrow_faultVariable', pyxb.binding.datatypes.NCName)
    __faultVariable._DeclarationLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 513, 16)
    __faultVariable._UseLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 513, 16)
    
    faultVariable = property(__faultVariable.value, __faultVariable.set, None, None)

    _AttributeWildcard = pyxb.binding.content.Wildcard(process_contents=pyxb.binding.content.Wildcard.PC_lax, namespace_constraint=(pyxb.binding.content.Wildcard.NC_not, 'http://schemas.xmlsoap.org/ws/2003/03/business-process/'))
    _HasWildcardElement = True
    _ElementMap.update({
        
    })
    _AttributeMap.update({
        __faultName.name() : __faultName,
        __faultVariable.name() : __faultVariable
    })
Namespace.addCategoryObject('typeBinding', 'tThrow', tThrow)


# Complex type {http://schemas.xmlsoap.org/ws/2003/03/business-process/}tCompensate with content type ELEMENT_ONLY
class tCompensate (tActivity):
    """Complex type {http://schemas.xmlsoap.org/ws/2003/03/business-process/}tCompensate with content type ELEMENT_ONLY"""
    _TypeDefinition = None
    _ContentTypeTag = pyxb.binding.basis.complexTypeDefinition._CT_ELEMENT_ONLY
    _Abstract = False
    _ExpandedName = pyxb.namespace.ExpandedName(Namespace, 'tCompensate')
    _XSDLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 518, 4)
    _ElementMap = tActivity._ElementMap.copy()
    _AttributeMap = tActivity._AttributeMap.copy()
    # Base type is tActivity
    
    # Element target ({http://schemas.xmlsoap.org/ws/2003/03/business-process/}target) inherited from {http://schemas.xmlsoap.org/ws/2003/03/business-process/}tActivity
    
    # Element source ({http://schemas.xmlsoap.org/ws/2003/03/business-process/}source) inherited from {http://schemas.xmlsoap.org/ws/2003/03/business-process/}tActivity
    
    # Attribute name inherited from {http://schemas.xmlsoap.org/ws/2003/03/business-process/}tActivity
    
    # Attribute joinCondition inherited from {http://schemas.xmlsoap.org/ws/2003/03/business-process/}tActivity
    
    # Attribute suppressJoinFailure inherited from {http://schemas.xmlsoap.org/ws/2003/03/business-process/}tActivity
    
    # Attribute scope uses Python identifier scope
    __scope = pyxb.binding.content.AttributeUse(pyxb.namespace.ExpandedName(None, 'scope'), 'scope', '__httpschemas_xmlsoap_orgws200303business_process_tCompensate_scope', pyxb.binding.datatypes.NCName)
    __scope._DeclarationLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 521, 16)
    __scope._UseLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 521, 16)
    
    scope = property(__scope.value, __scope.set, None, None)

    _AttributeWildcard = pyxb.binding.content.Wildcard(process_contents=pyxb.binding.content.Wildcard.PC_lax, namespace_constraint=(pyxb.binding.content.Wildcard.NC_not, 'http://schemas.xmlsoap.org/ws/2003/03/business-process/'))
    _HasWildcardElement = True
    _ElementMap.update({
        
    })
    _AttributeMap.update({
        __scope.name() : __scope
    })
Namespace.addCategoryObject('typeBinding', 'tCompensate', tCompensate)


# Complex type {http://schemas.xmlsoap.org/ws/2003/03/business-process/}tTerminate with content type ELEMENT_ONLY
class tTerminate (tActivity):
    """Complex type {http://schemas.xmlsoap.org/ws/2003/03/business-process/}tTerminate with content type ELEMENT_ONLY"""
    _TypeDefinition = None
    _ContentTypeTag = pyxb.binding.basis.complexTypeDefinition._CT_ELEMENT_ONLY
    _Abstract = False
    _ExpandedName = pyxb.namespace.ExpandedName(Namespace, 'tTerminate')
    _XSDLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 526, 4)
    _ElementMap = tActivity._ElementMap.copy()
    _AttributeMap = tActivity._AttributeMap.copy()
    # Base type is tActivity
    
    # Element target ({http://schemas.xmlsoap.org/ws/2003/03/business-process/}target) inherited from {http://schemas.xmlsoap.org/ws/2003/03/business-process/}tActivity
    
    # Element source ({http://schemas.xmlsoap.org/ws/2003/03/business-process/}source) inherited from {http://schemas.xmlsoap.org/ws/2003/03/business-process/}tActivity
    
    # Attribute name inherited from {http://schemas.xmlsoap.org/ws/2003/03/business-process/}tActivity
    
    # Attribute joinCondition inherited from {http://schemas.xmlsoap.org/ws/2003/03/business-process/}tActivity
    
    # Attribute suppressJoinFailure inherited from {http://schemas.xmlsoap.org/ws/2003/03/business-process/}tActivity
    _AttributeWildcard = pyxb.binding.content.Wildcard(process_contents=pyxb.binding.content.Wildcard.PC_lax, namespace_constraint=(pyxb.binding.content.Wildcard.NC_not, 'http://schemas.xmlsoap.org/ws/2003/03/business-process/'))
    _HasWildcardElement = True
    _ElementMap.update({
        
    })
    _AttributeMap.update({
        
    })
Namespace.addCategoryObject('typeBinding', 'tTerminate', tTerminate)


# Complex type {http://schemas.xmlsoap.org/ws/2003/03/business-process/}tFlow with content type ELEMENT_ONLY
class tFlow (tActivity):
    """Complex type {http://schemas.xmlsoap.org/ws/2003/03/business-process/}tFlow with content type ELEMENT_ONLY"""
    _TypeDefinition = None
    _ContentTypeTag = pyxb.binding.basis.complexTypeDefinition._CT_ELEMENT_ONLY
    _Abstract = False
    _ExpandedName = pyxb.namespace.ExpandedName(Namespace, 'tFlow')
    _XSDLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 532, 4)
    _ElementMap = tActivity._ElementMap.copy()
    _AttributeMap = tActivity._AttributeMap.copy()
    # Base type is tActivity
    
    # Element {http://schemas.xmlsoap.org/ws/2003/03/business-process/}empty uses Python identifier empty
    __empty = pyxb.binding.content.ElementDeclaration(pyxb.namespace.ExpandedName(Namespace, 'empty'), 'empty', '__httpschemas_xmlsoap_orgws200303business_process_tFlow_httpschemas_xmlsoap_orgws200303business_processempty', True, pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 69, 12), )

    
    empty = property(__empty.value, __empty.set, None, None)

    
    # Element {http://schemas.xmlsoap.org/ws/2003/03/business-process/}invoke uses Python identifier invoke
    __invoke = pyxb.binding.content.ElementDeclaration(pyxb.namespace.ExpandedName(Namespace, 'invoke'), 'invoke', '__httpschemas_xmlsoap_orgws200303business_process_tFlow_httpschemas_xmlsoap_orgws200303business_processinvoke', True, pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 70, 12), )

    
    invoke = property(__invoke.value, __invoke.set, None, None)

    
    # Element {http://schemas.xmlsoap.org/ws/2003/03/business-process/}receive uses Python identifier receive
    __receive = pyxb.binding.content.ElementDeclaration(pyxb.namespace.ExpandedName(Namespace, 'receive'), 'receive', '__httpschemas_xmlsoap_orgws200303business_process_tFlow_httpschemas_xmlsoap_orgws200303business_processreceive', True, pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 71, 12), )

    
    receive = property(__receive.value, __receive.set, None, None)

    
    # Element {http://schemas.xmlsoap.org/ws/2003/03/business-process/}reply uses Python identifier reply
    __reply = pyxb.binding.content.ElementDeclaration(pyxb.namespace.ExpandedName(Namespace, 'reply'), 'reply', '__httpschemas_xmlsoap_orgws200303business_process_tFlow_httpschemas_xmlsoap_orgws200303business_processreply', True, pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 72, 12), )

    
    reply = property(__reply.value, __reply.set, None, None)

    
    # Element {http://schemas.xmlsoap.org/ws/2003/03/business-process/}assign uses Python identifier assign
    __assign = pyxb.binding.content.ElementDeclaration(pyxb.namespace.ExpandedName(Namespace, 'assign'), 'assign', '__httpschemas_xmlsoap_orgws200303business_process_tFlow_httpschemas_xmlsoap_orgws200303business_processassign', True, pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 73, 12), )

    
    assign = property(__assign.value, __assign.set, None, None)

    
    # Element {http://schemas.xmlsoap.org/ws/2003/03/business-process/}wait uses Python identifier wait
    __wait = pyxb.binding.content.ElementDeclaration(pyxb.namespace.ExpandedName(Namespace, 'wait'), 'wait', '__httpschemas_xmlsoap_orgws200303business_process_tFlow_httpschemas_xmlsoap_orgws200303business_processwait', True, pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 74, 12), )

    
    wait = property(__wait.value, __wait.set, None, None)

    
    # Element {http://schemas.xmlsoap.org/ws/2003/03/business-process/}throw uses Python identifier throw
    __throw = pyxb.binding.content.ElementDeclaration(pyxb.namespace.ExpandedName(Namespace, 'throw'), 'throw', '__httpschemas_xmlsoap_orgws200303business_process_tFlow_httpschemas_xmlsoap_orgws200303business_processthrow', True, pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 75, 12), )

    
    throw = property(__throw.value, __throw.set, None, None)

    
    # Element {http://schemas.xmlsoap.org/ws/2003/03/business-process/}terminate uses Python identifier terminate
    __terminate = pyxb.binding.content.ElementDeclaration(pyxb.namespace.ExpandedName(Namespace, 'terminate'), 'terminate', '__httpschemas_xmlsoap_orgws200303business_process_tFlow_httpschemas_xmlsoap_orgws200303business_processterminate', True, pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 76, 12), )

    
    terminate = property(__terminate.value, __terminate.set, None, None)

    
    # Element {http://schemas.xmlsoap.org/ws/2003/03/business-process/}flow uses Python identifier flow
    __flow = pyxb.binding.content.ElementDeclaration(pyxb.namespace.ExpandedName(Namespace, 'flow'), 'flow', '__httpschemas_xmlsoap_orgws200303business_process_tFlow_httpschemas_xmlsoap_orgws200303business_processflow', True, pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 77, 12), )

    
    flow = property(__flow.value, __flow.set, None, None)

    
    # Element {http://schemas.xmlsoap.org/ws/2003/03/business-process/}switch uses Python identifier switch
    __switch = pyxb.binding.content.ElementDeclaration(pyxb.namespace.ExpandedName(Namespace, 'switch'), 'switch', '__httpschemas_xmlsoap_orgws200303business_process_tFlow_httpschemas_xmlsoap_orgws200303business_processswitch', True, pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 78, 12), )

    
    switch = property(__switch.value, __switch.set, None, None)

    
    # Element {http://schemas.xmlsoap.org/ws/2003/03/business-process/}while uses Python identifier while_
    __while = pyxb.binding.content.ElementDeclaration(pyxb.namespace.ExpandedName(Namespace, 'while'), 'while_', '__httpschemas_xmlsoap_orgws200303business_process_tFlow_httpschemas_xmlsoap_orgws200303business_processwhile', True, pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 79, 12), )

    
    while_ = property(__while.value, __while.set, None, None)

    
    # Element {http://schemas.xmlsoap.org/ws/2003/03/business-process/}sequence uses Python identifier sequence
    __sequence = pyxb.binding.content.ElementDeclaration(pyxb.namespace.ExpandedName(Namespace, 'sequence'), 'sequence', '__httpschemas_xmlsoap_orgws200303business_process_tFlow_httpschemas_xmlsoap_orgws200303business_processsequence', True, pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 80, 12), )

    
    sequence = property(__sequence.value, __sequence.set, None, None)

    
    # Element {http://schemas.xmlsoap.org/ws/2003/03/business-process/}pick uses Python identifier pick
    __pick = pyxb.binding.content.ElementDeclaration(pyxb.namespace.ExpandedName(Namespace, 'pick'), 'pick', '__httpschemas_xmlsoap_orgws200303business_process_tFlow_httpschemas_xmlsoap_orgws200303business_processpick', True, pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 81, 12), )

    
    pick = property(__pick.value, __pick.set, None, None)

    
    # Element {http://schemas.xmlsoap.org/ws/2003/03/business-process/}scope uses Python identifier scope
    __scope = pyxb.binding.content.ElementDeclaration(pyxb.namespace.ExpandedName(Namespace, 'scope'), 'scope', '__httpschemas_xmlsoap_orgws200303business_process_tFlow_httpschemas_xmlsoap_orgws200303business_processscope', True, pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 82, 12), )

    
    scope = property(__scope.value, __scope.set, None, None)

    
    # Element target ({http://schemas.xmlsoap.org/ws/2003/03/business-process/}target) inherited from {http://schemas.xmlsoap.org/ws/2003/03/business-process/}tActivity
    
    # Element source ({http://schemas.xmlsoap.org/ws/2003/03/business-process/}source) inherited from {http://schemas.xmlsoap.org/ws/2003/03/business-process/}tActivity
    
    # Element {http://schemas.xmlsoap.org/ws/2003/03/business-process/}links uses Python identifier links
    __links = pyxb.binding.content.ElementDeclaration(pyxb.namespace.ExpandedName(Namespace, 'links'), 'links', '__httpschemas_xmlsoap_orgws200303business_process_tFlow_httpschemas_xmlsoap_orgws200303business_processlinks', False, pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 536, 20), )

    
    links = property(__links.value, __links.set, None, None)

    
    # Attribute name inherited from {http://schemas.xmlsoap.org/ws/2003/03/business-process/}tActivity
    
    # Attribute joinCondition inherited from {http://schemas.xmlsoap.org/ws/2003/03/business-process/}tActivity
    
    # Attribute suppressJoinFailure inherited from {http://schemas.xmlsoap.org/ws/2003/03/business-process/}tActivity
    _AttributeWildcard = pyxb.binding.content.Wildcard(process_contents=pyxb.binding.content.Wildcard.PC_lax, namespace_constraint=(pyxb.binding.content.Wildcard.NC_not, 'http://schemas.xmlsoap.org/ws/2003/03/business-process/'))
    _HasWildcardElement = True
    _ElementMap.update({
        __empty.name() : __empty,
        __invoke.name() : __invoke,
        __receive.name() : __receive,
        __reply.name() : __reply,
        __assign.name() : __assign,
        __wait.name() : __wait,
        __throw.name() : __throw,
        __terminate.name() : __terminate,
        __flow.name() : __flow,
        __switch.name() : __switch,
        __while.name() : __while,
        __sequence.name() : __sequence,
        __pick.name() : __pick,
        __scope.name() : __scope,
        __links.name() : __links
    })
    _AttributeMap.update({
        
    })
Namespace.addCategoryObject('typeBinding', 'tFlow', tFlow)


# Complex type {http://schemas.xmlsoap.org/ws/2003/03/business-process/}tSwitch with content type ELEMENT_ONLY
class tSwitch (tActivity):
    """Complex type {http://schemas.xmlsoap.org/ws/2003/03/business-process/}tSwitch with content type ELEMENT_ONLY"""
    _TypeDefinition = None
    _ContentTypeTag = pyxb.binding.basis.complexTypeDefinition._CT_ELEMENT_ONLY
    _Abstract = False
    _ExpandedName = pyxb.namespace.ExpandedName(Namespace, 'tSwitch')
    _XSDLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 563, 4)
    _ElementMap = tActivity._ElementMap.copy()
    _AttributeMap = tActivity._AttributeMap.copy()
    # Base type is tActivity
    
    # Element target ({http://schemas.xmlsoap.org/ws/2003/03/business-process/}target) inherited from {http://schemas.xmlsoap.org/ws/2003/03/business-process/}tActivity
    
    # Element source ({http://schemas.xmlsoap.org/ws/2003/03/business-process/}source) inherited from {http://schemas.xmlsoap.org/ws/2003/03/business-process/}tActivity
    
    # Element {http://schemas.xmlsoap.org/ws/2003/03/business-process/}case uses Python identifier case
    __case = pyxb.binding.content.ElementDeclaration(pyxb.namespace.ExpandedName(Namespace, 'case'), 'case', '__httpschemas_xmlsoap_orgws200303business_process_tSwitch_httpschemas_xmlsoap_orgws200303business_processcase', True, pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 567, 20), )

    
    case = property(__case.value, __case.set, None, None)

    
    # Element {http://schemas.xmlsoap.org/ws/2003/03/business-process/}otherwise uses Python identifier otherwise
    __otherwise = pyxb.binding.content.ElementDeclaration(pyxb.namespace.ExpandedName(Namespace, 'otherwise'), 'otherwise', '__httpschemas_xmlsoap_orgws200303business_process_tSwitch_httpschemas_xmlsoap_orgws200303business_processotherwise', False, pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 578, 20), )

    
    otherwise = property(__otherwise.value, __otherwise.set, None, None)

    
    # Attribute name inherited from {http://schemas.xmlsoap.org/ws/2003/03/business-process/}tActivity
    
    # Attribute joinCondition inherited from {http://schemas.xmlsoap.org/ws/2003/03/business-process/}tActivity
    
    # Attribute suppressJoinFailure inherited from {http://schemas.xmlsoap.org/ws/2003/03/business-process/}tActivity
    _AttributeWildcard = pyxb.binding.content.Wildcard(process_contents=pyxb.binding.content.Wildcard.PC_lax, namespace_constraint=(pyxb.binding.content.Wildcard.NC_not, 'http://schemas.xmlsoap.org/ws/2003/03/business-process/'))
    _HasWildcardElement = True
    _ElementMap.update({
        __case.name() : __case,
        __otherwise.name() : __otherwise
    })
    _AttributeMap.update({
        
    })
Namespace.addCategoryObject('typeBinding', 'tSwitch', tSwitch)


# Complex type [anonymous] with content type ELEMENT_ONLY
class CTD_ANON_2 (tActivityContainer):
    """Complex type [anonymous] with content type ELEMENT_ONLY"""
    _TypeDefinition = None
    _ContentTypeTag = pyxb.binding.basis.complexTypeDefinition._CT_ELEMENT_ONLY
    _Abstract = False
    _ExpandedName = None
    _XSDLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 568, 24)
    _ElementMap = tActivityContainer._ElementMap.copy()
    _AttributeMap = tActivityContainer._AttributeMap.copy()
    # Base type is tActivityContainer
    
    # Element empty ({http://schemas.xmlsoap.org/ws/2003/03/business-process/}empty) inherited from {http://schemas.xmlsoap.org/ws/2003/03/business-process/}tActivityContainer
    
    # Element invoke ({http://schemas.xmlsoap.org/ws/2003/03/business-process/}invoke) inherited from {http://schemas.xmlsoap.org/ws/2003/03/business-process/}tActivityContainer
    
    # Element receive ({http://schemas.xmlsoap.org/ws/2003/03/business-process/}receive) inherited from {http://schemas.xmlsoap.org/ws/2003/03/business-process/}tActivityContainer
    
    # Element reply ({http://schemas.xmlsoap.org/ws/2003/03/business-process/}reply) inherited from {http://schemas.xmlsoap.org/ws/2003/03/business-process/}tActivityContainer
    
    # Element assign ({http://schemas.xmlsoap.org/ws/2003/03/business-process/}assign) inherited from {http://schemas.xmlsoap.org/ws/2003/03/business-process/}tActivityContainer
    
    # Element wait ({http://schemas.xmlsoap.org/ws/2003/03/business-process/}wait) inherited from {http://schemas.xmlsoap.org/ws/2003/03/business-process/}tActivityContainer
    
    # Element throw ({http://schemas.xmlsoap.org/ws/2003/03/business-process/}throw) inherited from {http://schemas.xmlsoap.org/ws/2003/03/business-process/}tActivityContainer
    
    # Element terminate ({http://schemas.xmlsoap.org/ws/2003/03/business-process/}terminate) inherited from {http://schemas.xmlsoap.org/ws/2003/03/business-process/}tActivityContainer
    
    # Element flow ({http://schemas.xmlsoap.org/ws/2003/03/business-process/}flow) inherited from {http://schemas.xmlsoap.org/ws/2003/03/business-process/}tActivityContainer
    
    # Element switch ({http://schemas.xmlsoap.org/ws/2003/03/business-process/}switch) inherited from {http://schemas.xmlsoap.org/ws/2003/03/business-process/}tActivityContainer
    
    # Element while_ ({http://schemas.xmlsoap.org/ws/2003/03/business-process/}while) inherited from {http://schemas.xmlsoap.org/ws/2003/03/business-process/}tActivityContainer
    
    # Element sequence ({http://schemas.xmlsoap.org/ws/2003/03/business-process/}sequence) inherited from {http://schemas.xmlsoap.org/ws/2003/03/business-process/}tActivityContainer
    
    # Element pick ({http://schemas.xmlsoap.org/ws/2003/03/business-process/}pick) inherited from {http://schemas.xmlsoap.org/ws/2003/03/business-process/}tActivityContainer
    
    # Element scope ({http://schemas.xmlsoap.org/ws/2003/03/business-process/}scope) inherited from {http://schemas.xmlsoap.org/ws/2003/03/business-process/}tActivityContainer
    
    # Attribute condition uses Python identifier condition
    __condition = pyxb.binding.content.AttributeUse(pyxb.namespace.ExpandedName(None, 'condition'), 'condition', '__httpschemas_xmlsoap_orgws200303business_process_CTD_ANON_2_condition', tBoolean_expr, required=True)
    __condition._DeclarationLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 571, 36)
    __condition._UseLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 571, 36)
    
    condition = property(__condition.value, __condition.set, None, None)

    _AttributeWildcard = pyxb.binding.content.Wildcard(process_contents=pyxb.binding.content.Wildcard.PC_lax, namespace_constraint=(pyxb.binding.content.Wildcard.NC_not, 'http://schemas.xmlsoap.org/ws/2003/03/business-process/'))
    _HasWildcardElement = True
    _ElementMap.update({
        
    })
    _AttributeMap.update({
        __condition.name() : __condition
    })



# Complex type {http://schemas.xmlsoap.org/ws/2003/03/business-process/}tWhile with content type ELEMENT_ONLY
class tWhile (tActivity):
    """Complex type {http://schemas.xmlsoap.org/ws/2003/03/business-process/}tWhile with content type ELEMENT_ONLY"""
    _TypeDefinition = None
    _ContentTypeTag = pyxb.binding.basis.complexTypeDefinition._CT_ELEMENT_ONLY
    _Abstract = False
    _ExpandedName = pyxb.namespace.ExpandedName(Namespace, 'tWhile')
    _XSDLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 586, 4)
    _ElementMap = tActivity._ElementMap.copy()
    _AttributeMap = tActivity._AttributeMap.copy()
    # Base type is tActivity
    
    # Element {http://schemas.xmlsoap.org/ws/2003/03/business-process/}empty uses Python identifier empty
    __empty = pyxb.binding.content.ElementDeclaration(pyxb.namespace.ExpandedName(Namespace, 'empty'), 'empty', '__httpschemas_xmlsoap_orgws200303business_process_tWhile_httpschemas_xmlsoap_orgws200303business_processempty', False, pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 69, 12), )

    
    empty = property(__empty.value, __empty.set, None, None)

    
    # Element {http://schemas.xmlsoap.org/ws/2003/03/business-process/}invoke uses Python identifier invoke
    __invoke = pyxb.binding.content.ElementDeclaration(pyxb.namespace.ExpandedName(Namespace, 'invoke'), 'invoke', '__httpschemas_xmlsoap_orgws200303business_process_tWhile_httpschemas_xmlsoap_orgws200303business_processinvoke', False, pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 70, 12), )

    
    invoke = property(__invoke.value, __invoke.set, None, None)

    
    # Element {http://schemas.xmlsoap.org/ws/2003/03/business-process/}receive uses Python identifier receive
    __receive = pyxb.binding.content.ElementDeclaration(pyxb.namespace.ExpandedName(Namespace, 'receive'), 'receive', '__httpschemas_xmlsoap_orgws200303business_process_tWhile_httpschemas_xmlsoap_orgws200303business_processreceive', False, pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 71, 12), )

    
    receive = property(__receive.value, __receive.set, None, None)

    
    # Element {http://schemas.xmlsoap.org/ws/2003/03/business-process/}reply uses Python identifier reply
    __reply = pyxb.binding.content.ElementDeclaration(pyxb.namespace.ExpandedName(Namespace, 'reply'), 'reply', '__httpschemas_xmlsoap_orgws200303business_process_tWhile_httpschemas_xmlsoap_orgws200303business_processreply', False, pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 72, 12), )

    
    reply = property(__reply.value, __reply.set, None, None)

    
    # Element {http://schemas.xmlsoap.org/ws/2003/03/business-process/}assign uses Python identifier assign
    __assign = pyxb.binding.content.ElementDeclaration(pyxb.namespace.ExpandedName(Namespace, 'assign'), 'assign', '__httpschemas_xmlsoap_orgws200303business_process_tWhile_httpschemas_xmlsoap_orgws200303business_processassign', False, pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 73, 12), )

    
    assign = property(__assign.value, __assign.set, None, None)

    
    # Element {http://schemas.xmlsoap.org/ws/2003/03/business-process/}wait uses Python identifier wait
    __wait = pyxb.binding.content.ElementDeclaration(pyxb.namespace.ExpandedName(Namespace, 'wait'), 'wait', '__httpschemas_xmlsoap_orgws200303business_process_tWhile_httpschemas_xmlsoap_orgws200303business_processwait', False, pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 74, 12), )

    
    wait = property(__wait.value, __wait.set, None, None)

    
    # Element {http://schemas.xmlsoap.org/ws/2003/03/business-process/}throw uses Python identifier throw
    __throw = pyxb.binding.content.ElementDeclaration(pyxb.namespace.ExpandedName(Namespace, 'throw'), 'throw', '__httpschemas_xmlsoap_orgws200303business_process_tWhile_httpschemas_xmlsoap_orgws200303business_processthrow', False, pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 75, 12), )

    
    throw = property(__throw.value, __throw.set, None, None)

    
    # Element {http://schemas.xmlsoap.org/ws/2003/03/business-process/}terminate uses Python identifier terminate
    __terminate = pyxb.binding.content.ElementDeclaration(pyxb.namespace.ExpandedName(Namespace, 'terminate'), 'terminate', '__httpschemas_xmlsoap_orgws200303business_process_tWhile_httpschemas_xmlsoap_orgws200303business_processterminate', False, pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 76, 12), )

    
    terminate = property(__terminate.value, __terminate.set, None, None)

    
    # Element {http://schemas.xmlsoap.org/ws/2003/03/business-process/}flow uses Python identifier flow
    __flow = pyxb.binding.content.ElementDeclaration(pyxb.namespace.ExpandedName(Namespace, 'flow'), 'flow', '__httpschemas_xmlsoap_orgws200303business_process_tWhile_httpschemas_xmlsoap_orgws200303business_processflow', False, pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 77, 12), )

    
    flow = property(__flow.value, __flow.set, None, None)

    
    # Element {http://schemas.xmlsoap.org/ws/2003/03/business-process/}switch uses Python identifier switch
    __switch = pyxb.binding.content.ElementDeclaration(pyxb.namespace.ExpandedName(Namespace, 'switch'), 'switch', '__httpschemas_xmlsoap_orgws200303business_process_tWhile_httpschemas_xmlsoap_orgws200303business_processswitch', False, pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 78, 12), )

    
    switch = property(__switch.value, __switch.set, None, None)

    
    # Element {http://schemas.xmlsoap.org/ws/2003/03/business-process/}while uses Python identifier while_
    __while = pyxb.binding.content.ElementDeclaration(pyxb.namespace.ExpandedName(Namespace, 'while'), 'while_', '__httpschemas_xmlsoap_orgws200303business_process_tWhile_httpschemas_xmlsoap_orgws200303business_processwhile', False, pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 79, 12), )

    
    while_ = property(__while.value, __while.set, None, None)

    
    # Element {http://schemas.xmlsoap.org/ws/2003/03/business-process/}sequence uses Python identifier sequence
    __sequence = pyxb.binding.content.ElementDeclaration(pyxb.namespace.ExpandedName(Namespace, 'sequence'), 'sequence', '__httpschemas_xmlsoap_orgws200303business_process_tWhile_httpschemas_xmlsoap_orgws200303business_processsequence', False, pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 80, 12), )

    
    sequence = property(__sequence.value, __sequence.set, None, None)

    
    # Element {http://schemas.xmlsoap.org/ws/2003/03/business-process/}pick uses Python identifier pick
    __pick = pyxb.binding.content.ElementDeclaration(pyxb.namespace.ExpandedName(Namespace, 'pick'), 'pick', '__httpschemas_xmlsoap_orgws200303business_process_tWhile_httpschemas_xmlsoap_orgws200303business_processpick', False, pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 81, 12), )

    
    pick = property(__pick.value, __pick.set, None, None)

    
    # Element {http://schemas.xmlsoap.org/ws/2003/03/business-process/}scope uses Python identifier scope
    __scope = pyxb.binding.content.ElementDeclaration(pyxb.namespace.ExpandedName(Namespace, 'scope'), 'scope', '__httpschemas_xmlsoap_orgws200303business_process_tWhile_httpschemas_xmlsoap_orgws200303business_processscope', False, pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 82, 12), )

    
    scope = property(__scope.value, __scope.set, None, None)

    
    # Element target ({http://schemas.xmlsoap.org/ws/2003/03/business-process/}target) inherited from {http://schemas.xmlsoap.org/ws/2003/03/business-process/}tActivity
    
    # Element source ({http://schemas.xmlsoap.org/ws/2003/03/business-process/}source) inherited from {http://schemas.xmlsoap.org/ws/2003/03/business-process/}tActivity
    
    # Attribute name inherited from {http://schemas.xmlsoap.org/ws/2003/03/business-process/}tActivity
    
    # Attribute joinCondition inherited from {http://schemas.xmlsoap.org/ws/2003/03/business-process/}tActivity
    
    # Attribute suppressJoinFailure inherited from {http://schemas.xmlsoap.org/ws/2003/03/business-process/}tActivity
    
    # Attribute condition uses Python identifier condition
    __condition = pyxb.binding.content.AttributeUse(pyxb.namespace.ExpandedName(None, 'condition'), 'condition', '__httpschemas_xmlsoap_orgws200303business_process_tWhile_condition', tBoolean_expr, required=True)
    __condition._DeclarationLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 592, 16)
    __condition._UseLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 592, 16)
    
    condition = property(__condition.value, __condition.set, None, None)

    _AttributeWildcard = pyxb.binding.content.Wildcard(process_contents=pyxb.binding.content.Wildcard.PC_lax, namespace_constraint=(pyxb.binding.content.Wildcard.NC_not, 'http://schemas.xmlsoap.org/ws/2003/03/business-process/'))
    _HasWildcardElement = True
    _ElementMap.update({
        __empty.name() : __empty,
        __invoke.name() : __invoke,
        __receive.name() : __receive,
        __reply.name() : __reply,
        __assign.name() : __assign,
        __wait.name() : __wait,
        __throw.name() : __throw,
        __terminate.name() : __terminate,
        __flow.name() : __flow,
        __switch.name() : __switch,
        __while.name() : __while,
        __sequence.name() : __sequence,
        __pick.name() : __pick,
        __scope.name() : __scope
    })
    _AttributeMap.update({
        __condition.name() : __condition
    })
Namespace.addCategoryObject('typeBinding', 'tWhile', tWhile)


# Complex type {http://schemas.xmlsoap.org/ws/2003/03/business-process/}tSequence with content type ELEMENT_ONLY
class tSequence (tActivity):
    """Complex type {http://schemas.xmlsoap.org/ws/2003/03/business-process/}tSequence with content type ELEMENT_ONLY"""
    _TypeDefinition = None
    _ContentTypeTag = pyxb.binding.basis.complexTypeDefinition._CT_ELEMENT_ONLY
    _Abstract = False
    _ExpandedName = pyxb.namespace.ExpandedName(Namespace, 'tSequence')
    _XSDLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 599, 4)
    _ElementMap = tActivity._ElementMap.copy()
    _AttributeMap = tActivity._AttributeMap.copy()
    # Base type is tActivity
    
    # Element {http://schemas.xmlsoap.org/ws/2003/03/business-process/}empty uses Python identifier empty
    __empty = pyxb.binding.content.ElementDeclaration(pyxb.namespace.ExpandedName(Namespace, 'empty'), 'empty', '__httpschemas_xmlsoap_orgws200303business_process_tSequence_httpschemas_xmlsoap_orgws200303business_processempty', True, pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 69, 12), )

    
    empty = property(__empty.value, __empty.set, None, None)

    
    # Element {http://schemas.xmlsoap.org/ws/2003/03/business-process/}invoke uses Python identifier invoke
    __invoke = pyxb.binding.content.ElementDeclaration(pyxb.namespace.ExpandedName(Namespace, 'invoke'), 'invoke', '__httpschemas_xmlsoap_orgws200303business_process_tSequence_httpschemas_xmlsoap_orgws200303business_processinvoke', True, pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 70, 12), )

    
    invoke = property(__invoke.value, __invoke.set, None, None)

    
    # Element {http://schemas.xmlsoap.org/ws/2003/03/business-process/}receive uses Python identifier receive
    __receive = pyxb.binding.content.ElementDeclaration(pyxb.namespace.ExpandedName(Namespace, 'receive'), 'receive', '__httpschemas_xmlsoap_orgws200303business_process_tSequence_httpschemas_xmlsoap_orgws200303business_processreceive', True, pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 71, 12), )

    
    receive = property(__receive.value, __receive.set, None, None)

    
    # Element {http://schemas.xmlsoap.org/ws/2003/03/business-process/}reply uses Python identifier reply
    __reply = pyxb.binding.content.ElementDeclaration(pyxb.namespace.ExpandedName(Namespace, 'reply'), 'reply', '__httpschemas_xmlsoap_orgws200303business_process_tSequence_httpschemas_xmlsoap_orgws200303business_processreply', True, pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 72, 12), )

    
    reply = property(__reply.value, __reply.set, None, None)

    
    # Element {http://schemas.xmlsoap.org/ws/2003/03/business-process/}assign uses Python identifier assign
    __assign = pyxb.binding.content.ElementDeclaration(pyxb.namespace.ExpandedName(Namespace, 'assign'), 'assign', '__httpschemas_xmlsoap_orgws200303business_process_tSequence_httpschemas_xmlsoap_orgws200303business_processassign', True, pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 73, 12), )

    
    assign = property(__assign.value, __assign.set, None, None)

    
    # Element {http://schemas.xmlsoap.org/ws/2003/03/business-process/}wait uses Python identifier wait
    __wait = pyxb.binding.content.ElementDeclaration(pyxb.namespace.ExpandedName(Namespace, 'wait'), 'wait', '__httpschemas_xmlsoap_orgws200303business_process_tSequence_httpschemas_xmlsoap_orgws200303business_processwait', True, pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 74, 12), )

    
    wait = property(__wait.value, __wait.set, None, None)

    
    # Element {http://schemas.xmlsoap.org/ws/2003/03/business-process/}throw uses Python identifier throw
    __throw = pyxb.binding.content.ElementDeclaration(pyxb.namespace.ExpandedName(Namespace, 'throw'), 'throw', '__httpschemas_xmlsoap_orgws200303business_process_tSequence_httpschemas_xmlsoap_orgws200303business_processthrow', True, pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 75, 12), )

    
    throw = property(__throw.value, __throw.set, None, None)

    
    # Element {http://schemas.xmlsoap.org/ws/2003/03/business-process/}terminate uses Python identifier terminate
    __terminate = pyxb.binding.content.ElementDeclaration(pyxb.namespace.ExpandedName(Namespace, 'terminate'), 'terminate', '__httpschemas_xmlsoap_orgws200303business_process_tSequence_httpschemas_xmlsoap_orgws200303business_processterminate', True, pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 76, 12), )

    
    terminate = property(__terminate.value, __terminate.set, None, None)

    
    # Element {http://schemas.xmlsoap.org/ws/2003/03/business-process/}flow uses Python identifier flow
    __flow = pyxb.binding.content.ElementDeclaration(pyxb.namespace.ExpandedName(Namespace, 'flow'), 'flow', '__httpschemas_xmlsoap_orgws200303business_process_tSequence_httpschemas_xmlsoap_orgws200303business_processflow', True, pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 77, 12), )

    
    flow = property(__flow.value, __flow.set, None, None)

    
    # Element {http://schemas.xmlsoap.org/ws/2003/03/business-process/}switch uses Python identifier switch
    __switch = pyxb.binding.content.ElementDeclaration(pyxb.namespace.ExpandedName(Namespace, 'switch'), 'switch', '__httpschemas_xmlsoap_orgws200303business_process_tSequence_httpschemas_xmlsoap_orgws200303business_processswitch', True, pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 78, 12), )

    
    switch = property(__switch.value, __switch.set, None, None)

    
    # Element {http://schemas.xmlsoap.org/ws/2003/03/business-process/}while uses Python identifier while_
    __while = pyxb.binding.content.ElementDeclaration(pyxb.namespace.ExpandedName(Namespace, 'while'), 'while_', '__httpschemas_xmlsoap_orgws200303business_process_tSequence_httpschemas_xmlsoap_orgws200303business_processwhile', True, pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 79, 12), )

    
    while_ = property(__while.value, __while.set, None, None)

    
    # Element {http://schemas.xmlsoap.org/ws/2003/03/business-process/}sequence uses Python identifier sequence
    __sequence = pyxb.binding.content.ElementDeclaration(pyxb.namespace.ExpandedName(Namespace, 'sequence'), 'sequence', '__httpschemas_xmlsoap_orgws200303business_process_tSequence_httpschemas_xmlsoap_orgws200303business_processsequence', True, pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 80, 12), )

    
    sequence = property(__sequence.value, __sequence.set, None, None)

    
    # Element {http://schemas.xmlsoap.org/ws/2003/03/business-process/}pick uses Python identifier pick
    __pick = pyxb.binding.content.ElementDeclaration(pyxb.namespace.ExpandedName(Namespace, 'pick'), 'pick', '__httpschemas_xmlsoap_orgws200303business_process_tSequence_httpschemas_xmlsoap_orgws200303business_processpick', True, pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 81, 12), )

    
    pick = property(__pick.value, __pick.set, None, None)

    
    # Element {http://schemas.xmlsoap.org/ws/2003/03/business-process/}scope uses Python identifier scope
    __scope = pyxb.binding.content.ElementDeclaration(pyxb.namespace.ExpandedName(Namespace, 'scope'), 'scope', '__httpschemas_xmlsoap_orgws200303business_process_tSequence_httpschemas_xmlsoap_orgws200303business_processscope', True, pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 82, 12), )

    
    scope = property(__scope.value, __scope.set, None, None)

    
    # Element target ({http://schemas.xmlsoap.org/ws/2003/03/business-process/}target) inherited from {http://schemas.xmlsoap.org/ws/2003/03/business-process/}tActivity
    
    # Element source ({http://schemas.xmlsoap.org/ws/2003/03/business-process/}source) inherited from {http://schemas.xmlsoap.org/ws/2003/03/business-process/}tActivity
    
    # Attribute name inherited from {http://schemas.xmlsoap.org/ws/2003/03/business-process/}tActivity
    
    # Attribute joinCondition inherited from {http://schemas.xmlsoap.org/ws/2003/03/business-process/}tActivity
    
    # Attribute suppressJoinFailure inherited from {http://schemas.xmlsoap.org/ws/2003/03/business-process/}tActivity
    _AttributeWildcard = pyxb.binding.content.Wildcard(process_contents=pyxb.binding.content.Wildcard.PC_lax, namespace_constraint=(pyxb.binding.content.Wildcard.NC_not, 'http://schemas.xmlsoap.org/ws/2003/03/business-process/'))
    _HasWildcardElement = True
    _ElementMap.update({
        __empty.name() : __empty,
        __invoke.name() : __invoke,
        __receive.name() : __receive,
        __reply.name() : __reply,
        __assign.name() : __assign,
        __wait.name() : __wait,
        __throw.name() : __throw,
        __terminate.name() : __terminate,
        __flow.name() : __flow,
        __switch.name() : __switch,
        __while.name() : __while,
        __sequence.name() : __sequence,
        __pick.name() : __pick,
        __scope.name() : __scope
    })
    _AttributeMap.update({
        
    })
Namespace.addCategoryObject('typeBinding', 'tSequence', tSequence)


# Complex type {http://schemas.xmlsoap.org/ws/2003/03/business-process/}tPick with content type ELEMENT_ONLY
class tPick (tActivity):
    """Complex type {http://schemas.xmlsoap.org/ws/2003/03/business-process/}tPick with content type ELEMENT_ONLY"""
    _TypeDefinition = None
    _ContentTypeTag = pyxb.binding.basis.complexTypeDefinition._CT_ELEMENT_ONLY
    _Abstract = False
    _ExpandedName = pyxb.namespace.ExpandedName(Namespace, 'tPick')
    _XSDLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 609, 4)
    _ElementMap = tActivity._ElementMap.copy()
    _AttributeMap = tActivity._AttributeMap.copy()
    # Base type is tActivity
    
    # Element target ({http://schemas.xmlsoap.org/ws/2003/03/business-process/}target) inherited from {http://schemas.xmlsoap.org/ws/2003/03/business-process/}tActivity
    
    # Element source ({http://schemas.xmlsoap.org/ws/2003/03/business-process/}source) inherited from {http://schemas.xmlsoap.org/ws/2003/03/business-process/}tActivity
    
    # Element {http://schemas.xmlsoap.org/ws/2003/03/business-process/}onMessage uses Python identifier onMessage
    __onMessage = pyxb.binding.content.ElementDeclaration(pyxb.namespace.ExpandedName(Namespace, 'onMessage'), 'onMessage', '__httpschemas_xmlsoap_orgws200303business_process_tPick_httpschemas_xmlsoap_orgws200303business_processonMessage', True, pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 613, 20), )

    
    onMessage = property(__onMessage.value, __onMessage.set, None, None)

    
    # Element {http://schemas.xmlsoap.org/ws/2003/03/business-process/}onAlarm uses Python identifier onAlarm
    __onAlarm = pyxb.binding.content.ElementDeclaration(pyxb.namespace.ExpandedName(Namespace, 'onAlarm'), 'onAlarm', '__httpschemas_xmlsoap_orgws200303business_process_tPick_httpschemas_xmlsoap_orgws200303business_processonAlarm', True, pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 616, 20), )

    
    onAlarm = property(__onAlarm.value, __onAlarm.set, None, None)

    
    # Attribute name inherited from {http://schemas.xmlsoap.org/ws/2003/03/business-process/}tActivity
    
    # Attribute joinCondition inherited from {http://schemas.xmlsoap.org/ws/2003/03/business-process/}tActivity
    
    # Attribute suppressJoinFailure inherited from {http://schemas.xmlsoap.org/ws/2003/03/business-process/}tActivity
    
    # Attribute createInstance uses Python identifier createInstance
    __createInstance = pyxb.binding.content.AttributeUse(pyxb.namespace.ExpandedName(None, 'createInstance'), 'createInstance', '__httpschemas_xmlsoap_orgws200303business_process_tPick_createInstance', tBoolean, unicode_default='no')
    __createInstance._DeclarationLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 620, 16)
    __createInstance._UseLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 620, 16)
    
    createInstance = property(__createInstance.value, __createInstance.set, None, None)

    _AttributeWildcard = pyxb.binding.content.Wildcard(process_contents=pyxb.binding.content.Wildcard.PC_lax, namespace_constraint=(pyxb.binding.content.Wildcard.NC_not, 'http://schemas.xmlsoap.org/ws/2003/03/business-process/'))
    _HasWildcardElement = True
    _ElementMap.update({
        __onMessage.name() : __onMessage,
        __onAlarm.name() : __onAlarm
    })
    _AttributeMap.update({
        __createInstance.name() : __createInstance
    })
Namespace.addCategoryObject('typeBinding', 'tPick', tPick)


# Complex type {http://schemas.xmlsoap.org/ws/2003/03/business-process/}tScope with content type ELEMENT_ONLY
class tScope (tActivity):
    """Complex type {http://schemas.xmlsoap.org/ws/2003/03/business-process/}tScope with content type ELEMENT_ONLY"""
    _TypeDefinition = None
    _ContentTypeTag = pyxb.binding.basis.complexTypeDefinition._CT_ELEMENT_ONLY
    _Abstract = False
    _ExpandedName = pyxb.namespace.ExpandedName(Namespace, 'tScope')
    _XSDLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 626, 4)
    _ElementMap = tActivity._ElementMap.copy()
    _AttributeMap = tActivity._AttributeMap.copy()
    # Base type is tActivity
    
    # Element {http://schemas.xmlsoap.org/ws/2003/03/business-process/}empty uses Python identifier empty
    __empty = pyxb.binding.content.ElementDeclaration(pyxb.namespace.ExpandedName(Namespace, 'empty'), 'empty', '__httpschemas_xmlsoap_orgws200303business_process_tScope_httpschemas_xmlsoap_orgws200303business_processempty', False, pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 69, 12), )

    
    empty = property(__empty.value, __empty.set, None, None)

    
    # Element {http://schemas.xmlsoap.org/ws/2003/03/business-process/}invoke uses Python identifier invoke
    __invoke = pyxb.binding.content.ElementDeclaration(pyxb.namespace.ExpandedName(Namespace, 'invoke'), 'invoke', '__httpschemas_xmlsoap_orgws200303business_process_tScope_httpschemas_xmlsoap_orgws200303business_processinvoke', False, pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 70, 12), )

    
    invoke = property(__invoke.value, __invoke.set, None, None)

    
    # Element {http://schemas.xmlsoap.org/ws/2003/03/business-process/}receive uses Python identifier receive
    __receive = pyxb.binding.content.ElementDeclaration(pyxb.namespace.ExpandedName(Namespace, 'receive'), 'receive', '__httpschemas_xmlsoap_orgws200303business_process_tScope_httpschemas_xmlsoap_orgws200303business_processreceive', False, pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 71, 12), )

    
    receive = property(__receive.value, __receive.set, None, None)

    
    # Element {http://schemas.xmlsoap.org/ws/2003/03/business-process/}reply uses Python identifier reply
    __reply = pyxb.binding.content.ElementDeclaration(pyxb.namespace.ExpandedName(Namespace, 'reply'), 'reply', '__httpschemas_xmlsoap_orgws200303business_process_tScope_httpschemas_xmlsoap_orgws200303business_processreply', False, pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 72, 12), )

    
    reply = property(__reply.value, __reply.set, None, None)

    
    # Element {http://schemas.xmlsoap.org/ws/2003/03/business-process/}assign uses Python identifier assign
    __assign = pyxb.binding.content.ElementDeclaration(pyxb.namespace.ExpandedName(Namespace, 'assign'), 'assign', '__httpschemas_xmlsoap_orgws200303business_process_tScope_httpschemas_xmlsoap_orgws200303business_processassign', False, pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 73, 12), )

    
    assign = property(__assign.value, __assign.set, None, None)

    
    # Element {http://schemas.xmlsoap.org/ws/2003/03/business-process/}wait uses Python identifier wait
    __wait = pyxb.binding.content.ElementDeclaration(pyxb.namespace.ExpandedName(Namespace, 'wait'), 'wait', '__httpschemas_xmlsoap_orgws200303business_process_tScope_httpschemas_xmlsoap_orgws200303business_processwait', False, pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 74, 12), )

    
    wait = property(__wait.value, __wait.set, None, None)

    
    # Element {http://schemas.xmlsoap.org/ws/2003/03/business-process/}throw uses Python identifier throw
    __throw = pyxb.binding.content.ElementDeclaration(pyxb.namespace.ExpandedName(Namespace, 'throw'), 'throw', '__httpschemas_xmlsoap_orgws200303business_process_tScope_httpschemas_xmlsoap_orgws200303business_processthrow', False, pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 75, 12), )

    
    throw = property(__throw.value, __throw.set, None, None)

    
    # Element {http://schemas.xmlsoap.org/ws/2003/03/business-process/}terminate uses Python identifier terminate
    __terminate = pyxb.binding.content.ElementDeclaration(pyxb.namespace.ExpandedName(Namespace, 'terminate'), 'terminate', '__httpschemas_xmlsoap_orgws200303business_process_tScope_httpschemas_xmlsoap_orgws200303business_processterminate', False, pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 76, 12), )

    
    terminate = property(__terminate.value, __terminate.set, None, None)

    
    # Element {http://schemas.xmlsoap.org/ws/2003/03/business-process/}flow uses Python identifier flow
    __flow = pyxb.binding.content.ElementDeclaration(pyxb.namespace.ExpandedName(Namespace, 'flow'), 'flow', '__httpschemas_xmlsoap_orgws200303business_process_tScope_httpschemas_xmlsoap_orgws200303business_processflow', False, pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 77, 12), )

    
    flow = property(__flow.value, __flow.set, None, None)

    
    # Element {http://schemas.xmlsoap.org/ws/2003/03/business-process/}switch uses Python identifier switch
    __switch = pyxb.binding.content.ElementDeclaration(pyxb.namespace.ExpandedName(Namespace, 'switch'), 'switch', '__httpschemas_xmlsoap_orgws200303business_process_tScope_httpschemas_xmlsoap_orgws200303business_processswitch', False, pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 78, 12), )

    
    switch = property(__switch.value, __switch.set, None, None)

    
    # Element {http://schemas.xmlsoap.org/ws/2003/03/business-process/}while uses Python identifier while_
    __while = pyxb.binding.content.ElementDeclaration(pyxb.namespace.ExpandedName(Namespace, 'while'), 'while_', '__httpschemas_xmlsoap_orgws200303business_process_tScope_httpschemas_xmlsoap_orgws200303business_processwhile', False, pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 79, 12), )

    
    while_ = property(__while.value, __while.set, None, None)

    
    # Element {http://schemas.xmlsoap.org/ws/2003/03/business-process/}sequence uses Python identifier sequence
    __sequence = pyxb.binding.content.ElementDeclaration(pyxb.namespace.ExpandedName(Namespace, 'sequence'), 'sequence', '__httpschemas_xmlsoap_orgws200303business_process_tScope_httpschemas_xmlsoap_orgws200303business_processsequence', False, pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 80, 12), )

    
    sequence = property(__sequence.value, __sequence.set, None, None)

    
    # Element {http://schemas.xmlsoap.org/ws/2003/03/business-process/}pick uses Python identifier pick
    __pick = pyxb.binding.content.ElementDeclaration(pyxb.namespace.ExpandedName(Namespace, 'pick'), 'pick', '__httpschemas_xmlsoap_orgws200303business_process_tScope_httpschemas_xmlsoap_orgws200303business_processpick', False, pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 81, 12), )

    
    pick = property(__pick.value, __pick.set, None, None)

    
    # Element {http://schemas.xmlsoap.org/ws/2003/03/business-process/}scope uses Python identifier scope
    __scope = pyxb.binding.content.ElementDeclaration(pyxb.namespace.ExpandedName(Namespace, 'scope'), 'scope', '__httpschemas_xmlsoap_orgws200303business_process_tScope_httpschemas_xmlsoap_orgws200303business_processscope', False, pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 82, 12), )

    
    scope = property(__scope.value, __scope.set, None, None)

    
    # Element target ({http://schemas.xmlsoap.org/ws/2003/03/business-process/}target) inherited from {http://schemas.xmlsoap.org/ws/2003/03/business-process/}tActivity
    
    # Element source ({http://schemas.xmlsoap.org/ws/2003/03/business-process/}source) inherited from {http://schemas.xmlsoap.org/ws/2003/03/business-process/}tActivity
    
    # Element {http://schemas.xmlsoap.org/ws/2003/03/business-process/}variables uses Python identifier variables
    __variables = pyxb.binding.content.ElementDeclaration(pyxb.namespace.ExpandedName(Namespace, 'variables'), 'variables', '__httpschemas_xmlsoap_orgws200303business_process_tScope_httpschemas_xmlsoap_orgws200303business_processvariables', False, pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 630, 20), )

    
    variables = property(__variables.value, __variables.set, None, None)

    
    # Element {http://schemas.xmlsoap.org/ws/2003/03/business-process/}correlationSets uses Python identifier correlationSets
    __correlationSets = pyxb.binding.content.ElementDeclaration(pyxb.namespace.ExpandedName(Namespace, 'correlationSets'), 'correlationSets', '__httpschemas_xmlsoap_orgws200303business_process_tScope_httpschemas_xmlsoap_orgws200303business_processcorrelationSets', False, pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 633, 20), )

    
    correlationSets = property(__correlationSets.value, __correlationSets.set, None, None)

    
    # Element {http://schemas.xmlsoap.org/ws/2003/03/business-process/}faultHandlers uses Python identifier faultHandlers
    __faultHandlers = pyxb.binding.content.ElementDeclaration(pyxb.namespace.ExpandedName(Namespace, 'faultHandlers'), 'faultHandlers', '__httpschemas_xmlsoap_orgws200303business_process_tScope_httpschemas_xmlsoap_orgws200303business_processfaultHandlers', False, pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 636, 20), )

    
    faultHandlers = property(__faultHandlers.value, __faultHandlers.set, None, None)

    
    # Element {http://schemas.xmlsoap.org/ws/2003/03/business-process/}compensationHandler uses Python identifier compensationHandler
    __compensationHandler = pyxb.binding.content.ElementDeclaration(pyxb.namespace.ExpandedName(Namespace, 'compensationHandler'), 'compensationHandler', '__httpschemas_xmlsoap_orgws200303business_process_tScope_httpschemas_xmlsoap_orgws200303business_processcompensationHandler', False, pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 639, 20), )

    
    compensationHandler = property(__compensationHandler.value, __compensationHandler.set, None, None)

    
    # Element {http://schemas.xmlsoap.org/ws/2003/03/business-process/}eventHandlers uses Python identifier eventHandlers
    __eventHandlers = pyxb.binding.content.ElementDeclaration(pyxb.namespace.ExpandedName(Namespace, 'eventHandlers'), 'eventHandlers', '__httpschemas_xmlsoap_orgws200303business_process_tScope_httpschemas_xmlsoap_orgws200303business_processeventHandlers', False, pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 642, 20), )

    
    eventHandlers = property(__eventHandlers.value, __eventHandlers.set, None, None)

    
    # Attribute name inherited from {http://schemas.xmlsoap.org/ws/2003/03/business-process/}tActivity
    
    # Attribute joinCondition inherited from {http://schemas.xmlsoap.org/ws/2003/03/business-process/}tActivity
    
    # Attribute suppressJoinFailure inherited from {http://schemas.xmlsoap.org/ws/2003/03/business-process/}tActivity
    
    # Attribute variableAccessSerializable uses Python identifier variableAccessSerializable
    __variableAccessSerializable = pyxb.binding.content.AttributeUse(pyxb.namespace.ExpandedName(None, 'variableAccessSerializable'), 'variableAccessSerializable', '__httpschemas_xmlsoap_orgws200303business_process_tScope_variableAccessSerializable', tBoolean, unicode_default='no')
    __variableAccessSerializable._DeclarationLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 647, 16)
    __variableAccessSerializable._UseLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 647, 16)
    
    variableAccessSerializable = property(__variableAccessSerializable.value, __variableAccessSerializable.set, None, None)

    _AttributeWildcard = pyxb.binding.content.Wildcard(process_contents=pyxb.binding.content.Wildcard.PC_lax, namespace_constraint=(pyxb.binding.content.Wildcard.NC_not, 'http://schemas.xmlsoap.org/ws/2003/03/business-process/'))
    _HasWildcardElement = True
    _ElementMap.update({
        __empty.name() : __empty,
        __invoke.name() : __invoke,
        __receive.name() : __receive,
        __reply.name() : __reply,
        __assign.name() : __assign,
        __wait.name() : __wait,
        __throw.name() : __throw,
        __terminate.name() : __terminate,
        __flow.name() : __flow,
        __switch.name() : __switch,
        __while.name() : __while,
        __sequence.name() : __sequence,
        __pick.name() : __pick,
        __scope.name() : __scope,
        __variables.name() : __variables,
        __correlationSets.name() : __correlationSets,
        __faultHandlers.name() : __faultHandlers,
        __compensationHandler.name() : __compensationHandler,
        __eventHandlers.name() : __eventHandlers
    })
    _AttributeMap.update({
        __variableAccessSerializable.name() : __variableAccessSerializable
    })
Namespace.addCategoryObject('typeBinding', 'tScope', tScope)


process = pyxb.binding.basis.element(pyxb.namespace.ExpandedName(Namespace, 'process'), tProcess, location=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 27, 4))
Namespace.addCategoryObject('elementBinding', process.name().localName(), process)

from_ = pyxb.binding.basis.element(pyxb.namespace.ExpandedName(Namespace, 'from'), tFrom, location=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 466, 4))
Namespace.addCategoryObject('elementBinding', from_.name().localName(), from_)

to = pyxb.binding.basis.element(pyxb.namespace.ExpandedName(Namespace, 'to'), CTD_ANON_, location=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 482, 4))
Namespace.addCategoryObject('elementBinding', to.name().localName(), to)



def _BuildAutomaton ():
    # Remove this helper function from the namespace after it is invoked
    global _BuildAutomaton
    del _BuildAutomaton
    import pyxb.utils.fac as fac

    counters = set()
    cc_0 = fac.CounterCondition(min=0, max=None, metadata=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 20, 12))
    counters.add(cc_0)
    states = []
    final_update = set()
    final_update.add(fac.UpdateInstruction(cc_0, False))
    symbol = pyxb.binding.content.WildcardUse(pyxb.binding.content.Wildcard(process_contents=pyxb.binding.content.Wildcard.PC_lax, namespace_constraint=(pyxb.binding.content.Wildcard.NC_not, 'http://schemas.xmlsoap.org/ws/2003/03/business-process/')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 20, 12))
    st_0 = fac.State(symbol, is_initial=True, final_update=final_update, is_unordered_catenation=False)
    states.append(st_0)
    transitions = []
    transitions.append(fac.Transition(st_0, [
        fac.UpdateInstruction(cc_0, True) ]))
    st_0._set_transitionSet(transitions)
    return fac.Automaton(states, counters, True, containing_state=None)
tExtensibleElements._Automaton = _BuildAutomaton()




tProcess._AddElement(pyxb.binding.basis.element(pyxb.namespace.ExpandedName(Namespace, 'partnerLinks'), tPartnerLinks, scope=tProcess, location=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 32, 20)))

tProcess._AddElement(pyxb.binding.basis.element(pyxb.namespace.ExpandedName(Namespace, 'partners'), tPartners, scope=tProcess, location=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 34, 20)))

tProcess._AddElement(pyxb.binding.basis.element(pyxb.namespace.ExpandedName(Namespace, 'variables'), tVariables, scope=tProcess, location=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 36, 20)))

tProcess._AddElement(pyxb.binding.basis.element(pyxb.namespace.ExpandedName(Namespace, 'correlationSets'), tCorrelationSets, scope=tProcess, location=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 39, 20)))

tProcess._AddElement(pyxb.binding.basis.element(pyxb.namespace.ExpandedName(Namespace, 'faultHandlers'), tFaultHandlers, scope=tProcess, location=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 41, 20)))

tProcess._AddElement(pyxb.binding.basis.element(pyxb.namespace.ExpandedName(Namespace, 'compensationHandler'), tCompensationHandler, scope=tProcess, location=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 43, 20)))

tProcess._AddElement(pyxb.binding.basis.element(pyxb.namespace.ExpandedName(Namespace, 'eventHandlers'), tEventHandlers, scope=tProcess, location=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 45, 20)))

tProcess._AddElement(pyxb.binding.basis.element(pyxb.namespace.ExpandedName(Namespace, 'empty'), tEmpty, scope=tProcess, location=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 69, 12)))

tProcess._AddElement(pyxb.binding.basis.element(pyxb.namespace.ExpandedName(Namespace, 'invoke'), tInvoke, scope=tProcess, location=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 70, 12)))

tProcess._AddElement(pyxb.binding.basis.element(pyxb.namespace.ExpandedName(Namespace, 'receive'), tReceive, scope=tProcess, location=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 71, 12)))

tProcess._AddElement(pyxb.binding.basis.element(pyxb.namespace.ExpandedName(Namespace, 'reply'), tReply, scope=tProcess, location=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 72, 12)))

tProcess._AddElement(pyxb.binding.basis.element(pyxb.namespace.ExpandedName(Namespace, 'assign'), tAssign, scope=tProcess, location=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 73, 12)))

tProcess._AddElement(pyxb.binding.basis.element(pyxb.namespace.ExpandedName(Namespace, 'wait'), tWait, scope=tProcess, location=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 74, 12)))

tProcess._AddElement(pyxb.binding.basis.element(pyxb.namespace.ExpandedName(Namespace, 'throw'), tThrow, scope=tProcess, location=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 75, 12)))

tProcess._AddElement(pyxb.binding.basis.element(pyxb.namespace.ExpandedName(Namespace, 'terminate'), tTerminate, scope=tProcess, location=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 76, 12)))

tProcess._AddElement(pyxb.binding.basis.element(pyxb.namespace.ExpandedName(Namespace, 'flow'), tFlow, scope=tProcess, location=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 77, 12)))

tProcess._AddElement(pyxb.binding.basis.element(pyxb.namespace.ExpandedName(Namespace, 'switch'), tSwitch, scope=tProcess, location=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 78, 12)))

tProcess._AddElement(pyxb.binding.basis.element(pyxb.namespace.ExpandedName(Namespace, 'while'), tWhile, scope=tProcess, location=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 79, 12)))

tProcess._AddElement(pyxb.binding.basis.element(pyxb.namespace.ExpandedName(Namespace, 'sequence'), tSequence, scope=tProcess, location=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 80, 12)))

tProcess._AddElement(pyxb.binding.basis.element(pyxb.namespace.ExpandedName(Namespace, 'pick'), tPick, scope=tProcess, location=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 81, 12)))

tProcess._AddElement(pyxb.binding.basis.element(pyxb.namespace.ExpandedName(Namespace, 'scope'), tScope, scope=tProcess, location=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 82, 12)))

def _BuildAutomaton_ ():
    # Remove this helper function from the namespace after it is invoked
    global _BuildAutomaton_
    del _BuildAutomaton_
    import pyxb.utils.fac as fac

    counters = set()
    cc_0 = fac.CounterCondition(min=0, max=None, metadata=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 20, 12))
    counters.add(cc_0)
    cc_1 = fac.CounterCondition(min=0, max=1, metadata=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 32, 20))
    counters.add(cc_1)
    cc_2 = fac.CounterCondition(min=0, max=1, metadata=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 34, 20))
    counters.add(cc_2)
    cc_3 = fac.CounterCondition(min=0, max=1, metadata=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 36, 20))
    counters.add(cc_3)
    cc_4 = fac.CounterCondition(min=0, max=1, metadata=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 39, 20))
    counters.add(cc_4)
    cc_5 = fac.CounterCondition(min=0, max=1, metadata=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 41, 20))
    counters.add(cc_5)
    cc_6 = fac.CounterCondition(min=0, max=1, metadata=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 43, 20))
    counters.add(cc_6)
    cc_7 = fac.CounterCondition(min=0, max=1, metadata=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 45, 20))
    counters.add(cc_7)
    states = []
    final_update = None
    symbol = pyxb.binding.content.WildcardUse(pyxb.binding.content.Wildcard(process_contents=pyxb.binding.content.Wildcard.PC_lax, namespace_constraint=(pyxb.binding.content.Wildcard.NC_not, 'http://schemas.xmlsoap.org/ws/2003/03/business-process/')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 20, 12))
    st_0 = fac.State(symbol, is_initial=True, final_update=final_update, is_unordered_catenation=False)
    states.append(st_0)
    final_update = None
    symbol = pyxb.binding.content.ElementUse(tProcess._UseForTag(pyxb.namespace.ExpandedName(Namespace, 'partnerLinks')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 32, 20))
    st_1 = fac.State(symbol, is_initial=True, final_update=final_update, is_unordered_catenation=False)
    states.append(st_1)
    final_update = None
    symbol = pyxb.binding.content.ElementUse(tProcess._UseForTag(pyxb.namespace.ExpandedName(Namespace, 'partners')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 34, 20))
    st_2 = fac.State(symbol, is_initial=True, final_update=final_update, is_unordered_catenation=False)
    states.append(st_2)
    final_update = None
    symbol = pyxb.binding.content.ElementUse(tProcess._UseForTag(pyxb.namespace.ExpandedName(Namespace, 'variables')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 36, 20))
    st_3 = fac.State(symbol, is_initial=True, final_update=final_update, is_unordered_catenation=False)
    states.append(st_3)
    final_update = None
    symbol = pyxb.binding.content.ElementUse(tProcess._UseForTag(pyxb.namespace.ExpandedName(Namespace, 'correlationSets')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 39, 20))
    st_4 = fac.State(symbol, is_initial=True, final_update=final_update, is_unordered_catenation=False)
    states.append(st_4)
    final_update = None
    symbol = pyxb.binding.content.ElementUse(tProcess._UseForTag(pyxb.namespace.ExpandedName(Namespace, 'faultHandlers')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 41, 20))
    st_5 = fac.State(symbol, is_initial=True, final_update=final_update, is_unordered_catenation=False)
    states.append(st_5)
    final_update = None
    symbol = pyxb.binding.content.ElementUse(tProcess._UseForTag(pyxb.namespace.ExpandedName(Namespace, 'compensationHandler')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 43, 20))
    st_6 = fac.State(symbol, is_initial=True, final_update=final_update, is_unordered_catenation=False)
    states.append(st_6)
    final_update = None
    symbol = pyxb.binding.content.ElementUse(tProcess._UseForTag(pyxb.namespace.ExpandedName(Namespace, 'eventHandlers')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 45, 20))
    st_7 = fac.State(symbol, is_initial=True, final_update=final_update, is_unordered_catenation=False)
    states.append(st_7)
    final_update = set()
    symbol = pyxb.binding.content.ElementUse(tProcess._UseForTag(pyxb.namespace.ExpandedName(Namespace, 'empty')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 69, 12))
    st_8 = fac.State(symbol, is_initial=True, final_update=final_update, is_unordered_catenation=False)
    states.append(st_8)
    final_update = set()
    symbol = pyxb.binding.content.ElementUse(tProcess._UseForTag(pyxb.namespace.ExpandedName(Namespace, 'invoke')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 70, 12))
    st_9 = fac.State(symbol, is_initial=True, final_update=final_update, is_unordered_catenation=False)
    states.append(st_9)
    final_update = set()
    symbol = pyxb.binding.content.ElementUse(tProcess._UseForTag(pyxb.namespace.ExpandedName(Namespace, 'receive')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 71, 12))
    st_10 = fac.State(symbol, is_initial=True, final_update=final_update, is_unordered_catenation=False)
    states.append(st_10)
    final_update = set()
    symbol = pyxb.binding.content.ElementUse(tProcess._UseForTag(pyxb.namespace.ExpandedName(Namespace, 'reply')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 72, 12))
    st_11 = fac.State(symbol, is_initial=True, final_update=final_update, is_unordered_catenation=False)
    states.append(st_11)
    final_update = set()
    symbol = pyxb.binding.content.ElementUse(tProcess._UseForTag(pyxb.namespace.ExpandedName(Namespace, 'assign')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 73, 12))
    st_12 = fac.State(symbol, is_initial=True, final_update=final_update, is_unordered_catenation=False)
    states.append(st_12)
    final_update = set()
    symbol = pyxb.binding.content.ElementUse(tProcess._UseForTag(pyxb.namespace.ExpandedName(Namespace, 'wait')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 74, 12))
    st_13 = fac.State(symbol, is_initial=True, final_update=final_update, is_unordered_catenation=False)
    states.append(st_13)
    final_update = set()
    symbol = pyxb.binding.content.ElementUse(tProcess._UseForTag(pyxb.namespace.ExpandedName(Namespace, 'throw')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 75, 12))
    st_14 = fac.State(symbol, is_initial=True, final_update=final_update, is_unordered_catenation=False)
    states.append(st_14)
    final_update = set()
    symbol = pyxb.binding.content.ElementUse(tProcess._UseForTag(pyxb.namespace.ExpandedName(Namespace, 'terminate')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 76, 12))
    st_15 = fac.State(symbol, is_initial=True, final_update=final_update, is_unordered_catenation=False)
    states.append(st_15)
    final_update = set()
    symbol = pyxb.binding.content.ElementUse(tProcess._UseForTag(pyxb.namespace.ExpandedName(Namespace, 'flow')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 77, 12))
    st_16 = fac.State(symbol, is_initial=True, final_update=final_update, is_unordered_catenation=False)
    states.append(st_16)
    final_update = set()
    symbol = pyxb.binding.content.ElementUse(tProcess._UseForTag(pyxb.namespace.ExpandedName(Namespace, 'switch')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 78, 12))
    st_17 = fac.State(symbol, is_initial=True, final_update=final_update, is_unordered_catenation=False)
    states.append(st_17)
    final_update = set()
    symbol = pyxb.binding.content.ElementUse(tProcess._UseForTag(pyxb.namespace.ExpandedName(Namespace, 'while')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 79, 12))
    st_18 = fac.State(symbol, is_initial=True, final_update=final_update, is_unordered_catenation=False)
    states.append(st_18)
    final_update = set()
    symbol = pyxb.binding.content.ElementUse(tProcess._UseForTag(pyxb.namespace.ExpandedName(Namespace, 'sequence')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 80, 12))
    st_19 = fac.State(symbol, is_initial=True, final_update=final_update, is_unordered_catenation=False)
    states.append(st_19)
    final_update = set()
    symbol = pyxb.binding.content.ElementUse(tProcess._UseForTag(pyxb.namespace.ExpandedName(Namespace, 'pick')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 81, 12))
    st_20 = fac.State(symbol, is_initial=True, final_update=final_update, is_unordered_catenation=False)
    states.append(st_20)
    final_update = set()
    symbol = pyxb.binding.content.ElementUse(tProcess._UseForTag(pyxb.namespace.ExpandedName(Namespace, 'scope')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 82, 12))
    st_21 = fac.State(symbol, is_initial=True, final_update=final_update, is_unordered_catenation=False)
    states.append(st_21)
    transitions = []
    transitions.append(fac.Transition(st_0, [
        fac.UpdateInstruction(cc_0, True) ]))
    transitions.append(fac.Transition(st_1, [
        fac.UpdateInstruction(cc_0, False) ]))
    transitions.append(fac.Transition(st_2, [
        fac.UpdateInstruction(cc_0, False) ]))
    transitions.append(fac.Transition(st_3, [
        fac.UpdateInstruction(cc_0, False) ]))
    transitions.append(fac.Transition(st_4, [
        fac.UpdateInstruction(cc_0, False) ]))
    transitions.append(fac.Transition(st_5, [
        fac.UpdateInstruction(cc_0, False) ]))
    transitions.append(fac.Transition(st_6, [
        fac.UpdateInstruction(cc_0, False) ]))
    transitions.append(fac.Transition(st_7, [
        fac.UpdateInstruction(cc_0, False) ]))
    transitions.append(fac.Transition(st_8, [
        fac.UpdateInstruction(cc_0, False) ]))
    transitions.append(fac.Transition(st_9, [
        fac.UpdateInstruction(cc_0, False) ]))
    transitions.append(fac.Transition(st_10, [
        fac.UpdateInstruction(cc_0, False) ]))
    transitions.append(fac.Transition(st_11, [
        fac.UpdateInstruction(cc_0, False) ]))
    transitions.append(fac.Transition(st_12, [
        fac.UpdateInstruction(cc_0, False) ]))
    transitions.append(fac.Transition(st_13, [
        fac.UpdateInstruction(cc_0, False) ]))
    transitions.append(fac.Transition(st_14, [
        fac.UpdateInstruction(cc_0, False) ]))
    transitions.append(fac.Transition(st_15, [
        fac.UpdateInstruction(cc_0, False) ]))
    transitions.append(fac.Transition(st_16, [
        fac.UpdateInstruction(cc_0, False) ]))
    transitions.append(fac.Transition(st_17, [
        fac.UpdateInstruction(cc_0, False) ]))
    transitions.append(fac.Transition(st_18, [
        fac.UpdateInstruction(cc_0, False) ]))
    transitions.append(fac.Transition(st_19, [
        fac.UpdateInstruction(cc_0, False) ]))
    transitions.append(fac.Transition(st_20, [
        fac.UpdateInstruction(cc_0, False) ]))
    transitions.append(fac.Transition(st_21, [
        fac.UpdateInstruction(cc_0, False) ]))
    st_0._set_transitionSet(transitions)
    transitions = []
    transitions.append(fac.Transition(st_1, [
        fac.UpdateInstruction(cc_1, True) ]))
    transitions.append(fac.Transition(st_2, [
        fac.UpdateInstruction(cc_1, False) ]))
    transitions.append(fac.Transition(st_3, [
        fac.UpdateInstruction(cc_1, False) ]))
    transitions.append(fac.Transition(st_4, [
        fac.UpdateInstruction(cc_1, False) ]))
    transitions.append(fac.Transition(st_5, [
        fac.UpdateInstruction(cc_1, False) ]))
    transitions.append(fac.Transition(st_6, [
        fac.UpdateInstruction(cc_1, False) ]))
    transitions.append(fac.Transition(st_7, [
        fac.UpdateInstruction(cc_1, False) ]))
    transitions.append(fac.Transition(st_8, [
        fac.UpdateInstruction(cc_1, False) ]))
    transitions.append(fac.Transition(st_9, [
        fac.UpdateInstruction(cc_1, False) ]))
    transitions.append(fac.Transition(st_10, [
        fac.UpdateInstruction(cc_1, False) ]))
    transitions.append(fac.Transition(st_11, [
        fac.UpdateInstruction(cc_1, False) ]))
    transitions.append(fac.Transition(st_12, [
        fac.UpdateInstruction(cc_1, False) ]))
    transitions.append(fac.Transition(st_13, [
        fac.UpdateInstruction(cc_1, False) ]))
    transitions.append(fac.Transition(st_14, [
        fac.UpdateInstruction(cc_1, False) ]))
    transitions.append(fac.Transition(st_15, [
        fac.UpdateInstruction(cc_1, False) ]))
    transitions.append(fac.Transition(st_16, [
        fac.UpdateInstruction(cc_1, False) ]))
    transitions.append(fac.Transition(st_17, [
        fac.UpdateInstruction(cc_1, False) ]))
    transitions.append(fac.Transition(st_18, [
        fac.UpdateInstruction(cc_1, False) ]))
    transitions.append(fac.Transition(st_19, [
        fac.UpdateInstruction(cc_1, False) ]))
    transitions.append(fac.Transition(st_20, [
        fac.UpdateInstruction(cc_1, False) ]))
    transitions.append(fac.Transition(st_21, [
        fac.UpdateInstruction(cc_1, False) ]))
    st_1._set_transitionSet(transitions)
    transitions = []
    transitions.append(fac.Transition(st_2, [
        fac.UpdateInstruction(cc_2, True) ]))
    transitions.append(fac.Transition(st_3, [
        fac.UpdateInstruction(cc_2, False) ]))
    transitions.append(fac.Transition(st_4, [
        fac.UpdateInstruction(cc_2, False) ]))
    transitions.append(fac.Transition(st_5, [
        fac.UpdateInstruction(cc_2, False) ]))
    transitions.append(fac.Transition(st_6, [
        fac.UpdateInstruction(cc_2, False) ]))
    transitions.append(fac.Transition(st_7, [
        fac.UpdateInstruction(cc_2, False) ]))
    transitions.append(fac.Transition(st_8, [
        fac.UpdateInstruction(cc_2, False) ]))
    transitions.append(fac.Transition(st_9, [
        fac.UpdateInstruction(cc_2, False) ]))
    transitions.append(fac.Transition(st_10, [
        fac.UpdateInstruction(cc_2, False) ]))
    transitions.append(fac.Transition(st_11, [
        fac.UpdateInstruction(cc_2, False) ]))
    transitions.append(fac.Transition(st_12, [
        fac.UpdateInstruction(cc_2, False) ]))
    transitions.append(fac.Transition(st_13, [
        fac.UpdateInstruction(cc_2, False) ]))
    transitions.append(fac.Transition(st_14, [
        fac.UpdateInstruction(cc_2, False) ]))
    transitions.append(fac.Transition(st_15, [
        fac.UpdateInstruction(cc_2, False) ]))
    transitions.append(fac.Transition(st_16, [
        fac.UpdateInstruction(cc_2, False) ]))
    transitions.append(fac.Transition(st_17, [
        fac.UpdateInstruction(cc_2, False) ]))
    transitions.append(fac.Transition(st_18, [
        fac.UpdateInstruction(cc_2, False) ]))
    transitions.append(fac.Transition(st_19, [
        fac.UpdateInstruction(cc_2, False) ]))
    transitions.append(fac.Transition(st_20, [
        fac.UpdateInstruction(cc_2, False) ]))
    transitions.append(fac.Transition(st_21, [
        fac.UpdateInstruction(cc_2, False) ]))
    st_2._set_transitionSet(transitions)
    transitions = []
    transitions.append(fac.Transition(st_3, [
        fac.UpdateInstruction(cc_3, True) ]))
    transitions.append(fac.Transition(st_4, [
        fac.UpdateInstruction(cc_3, False) ]))
    transitions.append(fac.Transition(st_5, [
        fac.UpdateInstruction(cc_3, False) ]))
    transitions.append(fac.Transition(st_6, [
        fac.UpdateInstruction(cc_3, False) ]))
    transitions.append(fac.Transition(st_7, [
        fac.UpdateInstruction(cc_3, False) ]))
    transitions.append(fac.Transition(st_8, [
        fac.UpdateInstruction(cc_3, False) ]))
    transitions.append(fac.Transition(st_9, [
        fac.UpdateInstruction(cc_3, False) ]))
    transitions.append(fac.Transition(st_10, [
        fac.UpdateInstruction(cc_3, False) ]))
    transitions.append(fac.Transition(st_11, [
        fac.UpdateInstruction(cc_3, False) ]))
    transitions.append(fac.Transition(st_12, [
        fac.UpdateInstruction(cc_3, False) ]))
    transitions.append(fac.Transition(st_13, [
        fac.UpdateInstruction(cc_3, False) ]))
    transitions.append(fac.Transition(st_14, [
        fac.UpdateInstruction(cc_3, False) ]))
    transitions.append(fac.Transition(st_15, [
        fac.UpdateInstruction(cc_3, False) ]))
    transitions.append(fac.Transition(st_16, [
        fac.UpdateInstruction(cc_3, False) ]))
    transitions.append(fac.Transition(st_17, [
        fac.UpdateInstruction(cc_3, False) ]))
    transitions.append(fac.Transition(st_18, [
        fac.UpdateInstruction(cc_3, False) ]))
    transitions.append(fac.Transition(st_19, [
        fac.UpdateInstruction(cc_3, False) ]))
    transitions.append(fac.Transition(st_20, [
        fac.UpdateInstruction(cc_3, False) ]))
    transitions.append(fac.Transition(st_21, [
        fac.UpdateInstruction(cc_3, False) ]))
    st_3._set_transitionSet(transitions)
    transitions = []
    transitions.append(fac.Transition(st_4, [
        fac.UpdateInstruction(cc_4, True) ]))
    transitions.append(fac.Transition(st_5, [
        fac.UpdateInstruction(cc_4, False) ]))
    transitions.append(fac.Transition(st_6, [
        fac.UpdateInstruction(cc_4, False) ]))
    transitions.append(fac.Transition(st_7, [
        fac.UpdateInstruction(cc_4, False) ]))
    transitions.append(fac.Transition(st_8, [
        fac.UpdateInstruction(cc_4, False) ]))
    transitions.append(fac.Transition(st_9, [
        fac.UpdateInstruction(cc_4, False) ]))
    transitions.append(fac.Transition(st_10, [
        fac.UpdateInstruction(cc_4, False) ]))
    transitions.append(fac.Transition(st_11, [
        fac.UpdateInstruction(cc_4, False) ]))
    transitions.append(fac.Transition(st_12, [
        fac.UpdateInstruction(cc_4, False) ]))
    transitions.append(fac.Transition(st_13, [
        fac.UpdateInstruction(cc_4, False) ]))
    transitions.append(fac.Transition(st_14, [
        fac.UpdateInstruction(cc_4, False) ]))
    transitions.append(fac.Transition(st_15, [
        fac.UpdateInstruction(cc_4, False) ]))
    transitions.append(fac.Transition(st_16, [
        fac.UpdateInstruction(cc_4, False) ]))
    transitions.append(fac.Transition(st_17, [
        fac.UpdateInstruction(cc_4, False) ]))
    transitions.append(fac.Transition(st_18, [
        fac.UpdateInstruction(cc_4, False) ]))
    transitions.append(fac.Transition(st_19, [
        fac.UpdateInstruction(cc_4, False) ]))
    transitions.append(fac.Transition(st_20, [
        fac.UpdateInstruction(cc_4, False) ]))
    transitions.append(fac.Transition(st_21, [
        fac.UpdateInstruction(cc_4, False) ]))
    st_4._set_transitionSet(transitions)
    transitions = []
    transitions.append(fac.Transition(st_5, [
        fac.UpdateInstruction(cc_5, True) ]))
    transitions.append(fac.Transition(st_6, [
        fac.UpdateInstruction(cc_5, False) ]))
    transitions.append(fac.Transition(st_7, [
        fac.UpdateInstruction(cc_5, False) ]))
    transitions.append(fac.Transition(st_8, [
        fac.UpdateInstruction(cc_5, False) ]))
    transitions.append(fac.Transition(st_9, [
        fac.UpdateInstruction(cc_5, False) ]))
    transitions.append(fac.Transition(st_10, [
        fac.UpdateInstruction(cc_5, False) ]))
    transitions.append(fac.Transition(st_11, [
        fac.UpdateInstruction(cc_5, False) ]))
    transitions.append(fac.Transition(st_12, [
        fac.UpdateInstruction(cc_5, False) ]))
    transitions.append(fac.Transition(st_13, [
        fac.UpdateInstruction(cc_5, False) ]))
    transitions.append(fac.Transition(st_14, [
        fac.UpdateInstruction(cc_5, False) ]))
    transitions.append(fac.Transition(st_15, [
        fac.UpdateInstruction(cc_5, False) ]))
    transitions.append(fac.Transition(st_16, [
        fac.UpdateInstruction(cc_5, False) ]))
    transitions.append(fac.Transition(st_17, [
        fac.UpdateInstruction(cc_5, False) ]))
    transitions.append(fac.Transition(st_18, [
        fac.UpdateInstruction(cc_5, False) ]))
    transitions.append(fac.Transition(st_19, [
        fac.UpdateInstruction(cc_5, False) ]))
    transitions.append(fac.Transition(st_20, [
        fac.UpdateInstruction(cc_5, False) ]))
    transitions.append(fac.Transition(st_21, [
        fac.UpdateInstruction(cc_5, False) ]))
    st_5._set_transitionSet(transitions)
    transitions = []
    transitions.append(fac.Transition(st_6, [
        fac.UpdateInstruction(cc_6, True) ]))
    transitions.append(fac.Transition(st_7, [
        fac.UpdateInstruction(cc_6, False) ]))
    transitions.append(fac.Transition(st_8, [
        fac.UpdateInstruction(cc_6, False) ]))
    transitions.append(fac.Transition(st_9, [
        fac.UpdateInstruction(cc_6, False) ]))
    transitions.append(fac.Transition(st_10, [
        fac.UpdateInstruction(cc_6, False) ]))
    transitions.append(fac.Transition(st_11, [
        fac.UpdateInstruction(cc_6, False) ]))
    transitions.append(fac.Transition(st_12, [
        fac.UpdateInstruction(cc_6, False) ]))
    transitions.append(fac.Transition(st_13, [
        fac.UpdateInstruction(cc_6, False) ]))
    transitions.append(fac.Transition(st_14, [
        fac.UpdateInstruction(cc_6, False) ]))
    transitions.append(fac.Transition(st_15, [
        fac.UpdateInstruction(cc_6, False) ]))
    transitions.append(fac.Transition(st_16, [
        fac.UpdateInstruction(cc_6, False) ]))
    transitions.append(fac.Transition(st_17, [
        fac.UpdateInstruction(cc_6, False) ]))
    transitions.append(fac.Transition(st_18, [
        fac.UpdateInstruction(cc_6, False) ]))
    transitions.append(fac.Transition(st_19, [
        fac.UpdateInstruction(cc_6, False) ]))
    transitions.append(fac.Transition(st_20, [
        fac.UpdateInstruction(cc_6, False) ]))
    transitions.append(fac.Transition(st_21, [
        fac.UpdateInstruction(cc_6, False) ]))
    st_6._set_transitionSet(transitions)
    transitions = []
    transitions.append(fac.Transition(st_7, [
        fac.UpdateInstruction(cc_7, True) ]))
    transitions.append(fac.Transition(st_8, [
        fac.UpdateInstruction(cc_7, False) ]))
    transitions.append(fac.Transition(st_9, [
        fac.UpdateInstruction(cc_7, False) ]))
    transitions.append(fac.Transition(st_10, [
        fac.UpdateInstruction(cc_7, False) ]))
    transitions.append(fac.Transition(st_11, [
        fac.UpdateInstruction(cc_7, False) ]))
    transitions.append(fac.Transition(st_12, [
        fac.UpdateInstruction(cc_7, False) ]))
    transitions.append(fac.Transition(st_13, [
        fac.UpdateInstruction(cc_7, False) ]))
    transitions.append(fac.Transition(st_14, [
        fac.UpdateInstruction(cc_7, False) ]))
    transitions.append(fac.Transition(st_15, [
        fac.UpdateInstruction(cc_7, False) ]))
    transitions.append(fac.Transition(st_16, [
        fac.UpdateInstruction(cc_7, False) ]))
    transitions.append(fac.Transition(st_17, [
        fac.UpdateInstruction(cc_7, False) ]))
    transitions.append(fac.Transition(st_18, [
        fac.UpdateInstruction(cc_7, False) ]))
    transitions.append(fac.Transition(st_19, [
        fac.UpdateInstruction(cc_7, False) ]))
    transitions.append(fac.Transition(st_20, [
        fac.UpdateInstruction(cc_7, False) ]))
    transitions.append(fac.Transition(st_21, [
        fac.UpdateInstruction(cc_7, False) ]))
    st_7._set_transitionSet(transitions)
    transitions = []
    st_8._set_transitionSet(transitions)
    transitions = []
    st_9._set_transitionSet(transitions)
    transitions = []
    st_10._set_transitionSet(transitions)
    transitions = []
    st_11._set_transitionSet(transitions)
    transitions = []
    st_12._set_transitionSet(transitions)
    transitions = []
    st_13._set_transitionSet(transitions)
    transitions = []
    st_14._set_transitionSet(transitions)
    transitions = []
    st_15._set_transitionSet(transitions)
    transitions = []
    st_16._set_transitionSet(transitions)
    transitions = []
    st_17._set_transitionSet(transitions)
    transitions = []
    st_18._set_transitionSet(transitions)
    transitions = []
    st_19._set_transitionSet(transitions)
    transitions = []
    st_20._set_transitionSet(transitions)
    transitions = []
    st_21._set_transitionSet(transitions)
    return fac.Automaton(states, counters, False, containing_state=None)
tProcess._Automaton = _BuildAutomaton_()




tPartnerLinks._AddElement(pyxb.binding.basis.element(pyxb.namespace.ExpandedName(Namespace, 'partnerLink'), tPartnerLink, scope=tPartnerLinks, location=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 91, 20)))

def _BuildAutomaton_2 ():
    # Remove this helper function from the namespace after it is invoked
    global _BuildAutomaton_2
    del _BuildAutomaton_2
    import pyxb.utils.fac as fac

    counters = set()
    cc_0 = fac.CounterCondition(min=0, max=None, metadata=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 20, 12))
    counters.add(cc_0)
    states = []
    final_update = None
    symbol = pyxb.binding.content.WildcardUse(pyxb.binding.content.Wildcard(process_contents=pyxb.binding.content.Wildcard.PC_lax, namespace_constraint=(pyxb.binding.content.Wildcard.NC_not, 'http://schemas.xmlsoap.org/ws/2003/03/business-process/')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 20, 12))
    st_0 = fac.State(symbol, is_initial=True, final_update=final_update, is_unordered_catenation=False)
    states.append(st_0)
    final_update = set()
    symbol = pyxb.binding.content.ElementUse(tPartnerLinks._UseForTag(pyxb.namespace.ExpandedName(Namespace, 'partnerLink')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 91, 20))
    st_1 = fac.State(symbol, is_initial=True, final_update=final_update, is_unordered_catenation=False)
    states.append(st_1)
    transitions = []
    transitions.append(fac.Transition(st_0, [
        fac.UpdateInstruction(cc_0, True) ]))
    transitions.append(fac.Transition(st_1, [
        fac.UpdateInstruction(cc_0, False) ]))
    st_0._set_transitionSet(transitions)
    transitions = []
    transitions.append(fac.Transition(st_1, [
         ]))
    st_1._set_transitionSet(transitions)
    return fac.Automaton(states, counters, False, containing_state=None)
tPartnerLinks._Automaton = _BuildAutomaton_2()




def _BuildAutomaton_3 ():
    # Remove this helper function from the namespace after it is invoked
    global _BuildAutomaton_3
    del _BuildAutomaton_3
    import pyxb.utils.fac as fac

    counters = set()
    cc_0 = fac.CounterCondition(min=0, max=None, metadata=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 20, 12))
    counters.add(cc_0)
    states = []
    final_update = set()
    final_update.add(fac.UpdateInstruction(cc_0, False))
    symbol = pyxb.binding.content.WildcardUse(pyxb.binding.content.Wildcard(process_contents=pyxb.binding.content.Wildcard.PC_lax, namespace_constraint=(pyxb.binding.content.Wildcard.NC_not, 'http://schemas.xmlsoap.org/ws/2003/03/business-process/')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 20, 12))
    st_0 = fac.State(symbol, is_initial=True, final_update=final_update, is_unordered_catenation=False)
    states.append(st_0)
    transitions = []
    transitions.append(fac.Transition(st_0, [
        fac.UpdateInstruction(cc_0, True) ]))
    st_0._set_transitionSet(transitions)
    return fac.Automaton(states, counters, True, containing_state=None)
tPartnerLink._Automaton = _BuildAutomaton_3()




tPartners._AddElement(pyxb.binding.basis.element(pyxb.namespace.ExpandedName(Namespace, 'partner'), tPartner, scope=tPartners, location=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 115, 20)))

def _BuildAutomaton_4 ():
    # Remove this helper function from the namespace after it is invoked
    global _BuildAutomaton_4
    del _BuildAutomaton_4
    import pyxb.utils.fac as fac

    counters = set()
    cc_0 = fac.CounterCondition(min=0, max=None, metadata=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 20, 12))
    counters.add(cc_0)
    states = []
    final_update = None
    symbol = pyxb.binding.content.WildcardUse(pyxb.binding.content.Wildcard(process_contents=pyxb.binding.content.Wildcard.PC_lax, namespace_constraint=(pyxb.binding.content.Wildcard.NC_not, 'http://schemas.xmlsoap.org/ws/2003/03/business-process/')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 20, 12))
    st_0 = fac.State(symbol, is_initial=True, final_update=final_update, is_unordered_catenation=False)
    states.append(st_0)
    final_update = set()
    symbol = pyxb.binding.content.ElementUse(tPartners._UseForTag(pyxb.namespace.ExpandedName(Namespace, 'partner')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 115, 20))
    st_1 = fac.State(symbol, is_initial=True, final_update=final_update, is_unordered_catenation=False)
    states.append(st_1)
    transitions = []
    transitions.append(fac.Transition(st_0, [
        fac.UpdateInstruction(cc_0, True) ]))
    transitions.append(fac.Transition(st_1, [
        fac.UpdateInstruction(cc_0, False) ]))
    st_0._set_transitionSet(transitions)
    transitions = []
    transitions.append(fac.Transition(st_1, [
         ]))
    st_1._set_transitionSet(transitions)
    return fac.Automaton(states, counters, False, containing_state=None)
tPartners._Automaton = _BuildAutomaton_4()




tPartner._AddElement(pyxb.binding.basis.element(pyxb.namespace.ExpandedName(Namespace, 'partnerLink'), CTD_ANON, scope=tPartner, location=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 126, 18)))

def _BuildAutomaton_5 ():
    # Remove this helper function from the namespace after it is invoked
    global _BuildAutomaton_5
    del _BuildAutomaton_5
    import pyxb.utils.fac as fac

    counters = set()
    cc_0 = fac.CounterCondition(min=0, max=None, metadata=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 20, 12))
    counters.add(cc_0)
    states = []
    final_update = None
    symbol = pyxb.binding.content.WildcardUse(pyxb.binding.content.Wildcard(process_contents=pyxb.binding.content.Wildcard.PC_lax, namespace_constraint=(pyxb.binding.content.Wildcard.NC_not, 'http://schemas.xmlsoap.org/ws/2003/03/business-process/')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 20, 12))
    st_0 = fac.State(symbol, is_initial=True, final_update=final_update, is_unordered_catenation=False)
    states.append(st_0)
    final_update = set()
    symbol = pyxb.binding.content.ElementUse(tPartner._UseForTag(pyxb.namespace.ExpandedName(Namespace, 'partnerLink')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 126, 18))
    st_1 = fac.State(symbol, is_initial=True, final_update=final_update, is_unordered_catenation=False)
    states.append(st_1)
    transitions = []
    transitions.append(fac.Transition(st_0, [
        fac.UpdateInstruction(cc_0, True) ]))
    transitions.append(fac.Transition(st_1, [
        fac.UpdateInstruction(cc_0, False) ]))
    st_0._set_transitionSet(transitions)
    transitions = []
    transitions.append(fac.Transition(st_1, [
         ]))
    st_1._set_transitionSet(transitions)
    return fac.Automaton(states, counters, False, containing_state=None)
tPartner._Automaton = _BuildAutomaton_5()




def _BuildAutomaton_6 ():
    # Remove this helper function from the namespace after it is invoked
    global _BuildAutomaton_6
    del _BuildAutomaton_6
    import pyxb.utils.fac as fac

    counters = set()
    cc_0 = fac.CounterCondition(min=0, max=None, metadata=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 20, 12))
    counters.add(cc_0)
    states = []
    final_update = set()
    final_update.add(fac.UpdateInstruction(cc_0, False))
    symbol = pyxb.binding.content.WildcardUse(pyxb.binding.content.Wildcard(process_contents=pyxb.binding.content.Wildcard.PC_lax, namespace_constraint=(pyxb.binding.content.Wildcard.NC_not, 'http://schemas.xmlsoap.org/ws/2003/03/business-process/')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 20, 12))
    st_0 = fac.State(symbol, is_initial=True, final_update=final_update, is_unordered_catenation=False)
    states.append(st_0)
    transitions = []
    transitions.append(fac.Transition(st_0, [
        fac.UpdateInstruction(cc_0, True) ]))
    st_0._set_transitionSet(transitions)
    return fac.Automaton(states, counters, True, containing_state=None)
CTD_ANON._Automaton = _BuildAutomaton_6()




tFaultHandlers._AddElement(pyxb.binding.basis.element(pyxb.namespace.ExpandedName(Namespace, 'catch'), tCatch, scope=tFaultHandlers, location=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 148, 20)))

tFaultHandlers._AddElement(pyxb.binding.basis.element(pyxb.namespace.ExpandedName(Namespace, 'catchAll'), tActivityOrCompensateContainer, scope=tFaultHandlers, location=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 150, 20)))

def _BuildAutomaton_7 ():
    # Remove this helper function from the namespace after it is invoked
    global _BuildAutomaton_7
    del _BuildAutomaton_7
    import pyxb.utils.fac as fac

    counters = set()
    cc_0 = fac.CounterCondition(min=0, max=None, metadata=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 20, 12))
    counters.add(cc_0)
    cc_1 = fac.CounterCondition(min=0, max=None, metadata=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 148, 20))
    counters.add(cc_1)
    cc_2 = fac.CounterCondition(min=0, max=1, metadata=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 150, 20))
    counters.add(cc_2)
    states = []
    final_update = set()
    final_update.add(fac.UpdateInstruction(cc_0, False))
    symbol = pyxb.binding.content.WildcardUse(pyxb.binding.content.Wildcard(process_contents=pyxb.binding.content.Wildcard.PC_lax, namespace_constraint=(pyxb.binding.content.Wildcard.NC_not, 'http://schemas.xmlsoap.org/ws/2003/03/business-process/')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 20, 12))
    st_0 = fac.State(symbol, is_initial=True, final_update=final_update, is_unordered_catenation=False)
    states.append(st_0)
    final_update = set()
    final_update.add(fac.UpdateInstruction(cc_1, False))
    symbol = pyxb.binding.content.ElementUse(tFaultHandlers._UseForTag(pyxb.namespace.ExpandedName(Namespace, 'catch')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 148, 20))
    st_1 = fac.State(symbol, is_initial=True, final_update=final_update, is_unordered_catenation=False)
    states.append(st_1)
    final_update = set()
    final_update.add(fac.UpdateInstruction(cc_2, False))
    symbol = pyxb.binding.content.ElementUse(tFaultHandlers._UseForTag(pyxb.namespace.ExpandedName(Namespace, 'catchAll')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 150, 20))
    st_2 = fac.State(symbol, is_initial=True, final_update=final_update, is_unordered_catenation=False)
    states.append(st_2)
    transitions = []
    transitions.append(fac.Transition(st_0, [
        fac.UpdateInstruction(cc_0, True) ]))
    transitions.append(fac.Transition(st_1, [
        fac.UpdateInstruction(cc_0, False) ]))
    transitions.append(fac.Transition(st_2, [
        fac.UpdateInstruction(cc_0, False) ]))
    st_0._set_transitionSet(transitions)
    transitions = []
    transitions.append(fac.Transition(st_1, [
        fac.UpdateInstruction(cc_1, True) ]))
    transitions.append(fac.Transition(st_2, [
        fac.UpdateInstruction(cc_1, False) ]))
    st_1._set_transitionSet(transitions)
    transitions = []
    transitions.append(fac.Transition(st_2, [
        fac.UpdateInstruction(cc_2, True) ]))
    st_2._set_transitionSet(transitions)
    return fac.Automaton(states, counters, True, containing_state=None)
tFaultHandlers._Automaton = _BuildAutomaton_7()




tActivityContainer._AddElement(pyxb.binding.basis.element(pyxb.namespace.ExpandedName(Namespace, 'empty'), tEmpty, scope=tActivityContainer, location=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 69, 12)))

tActivityContainer._AddElement(pyxb.binding.basis.element(pyxb.namespace.ExpandedName(Namespace, 'invoke'), tInvoke, scope=tActivityContainer, location=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 70, 12)))

tActivityContainer._AddElement(pyxb.binding.basis.element(pyxb.namespace.ExpandedName(Namespace, 'receive'), tReceive, scope=tActivityContainer, location=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 71, 12)))

tActivityContainer._AddElement(pyxb.binding.basis.element(pyxb.namespace.ExpandedName(Namespace, 'reply'), tReply, scope=tActivityContainer, location=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 72, 12)))

tActivityContainer._AddElement(pyxb.binding.basis.element(pyxb.namespace.ExpandedName(Namespace, 'assign'), tAssign, scope=tActivityContainer, location=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 73, 12)))

tActivityContainer._AddElement(pyxb.binding.basis.element(pyxb.namespace.ExpandedName(Namespace, 'wait'), tWait, scope=tActivityContainer, location=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 74, 12)))

tActivityContainer._AddElement(pyxb.binding.basis.element(pyxb.namespace.ExpandedName(Namespace, 'throw'), tThrow, scope=tActivityContainer, location=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 75, 12)))

tActivityContainer._AddElement(pyxb.binding.basis.element(pyxb.namespace.ExpandedName(Namespace, 'terminate'), tTerminate, scope=tActivityContainer, location=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 76, 12)))

tActivityContainer._AddElement(pyxb.binding.basis.element(pyxb.namespace.ExpandedName(Namespace, 'flow'), tFlow, scope=tActivityContainer, location=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 77, 12)))

tActivityContainer._AddElement(pyxb.binding.basis.element(pyxb.namespace.ExpandedName(Namespace, 'switch'), tSwitch, scope=tActivityContainer, location=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 78, 12)))

tActivityContainer._AddElement(pyxb.binding.basis.element(pyxb.namespace.ExpandedName(Namespace, 'while'), tWhile, scope=tActivityContainer, location=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 79, 12)))

tActivityContainer._AddElement(pyxb.binding.basis.element(pyxb.namespace.ExpandedName(Namespace, 'sequence'), tSequence, scope=tActivityContainer, location=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 80, 12)))

tActivityContainer._AddElement(pyxb.binding.basis.element(pyxb.namespace.ExpandedName(Namespace, 'pick'), tPick, scope=tActivityContainer, location=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 81, 12)))

tActivityContainer._AddElement(pyxb.binding.basis.element(pyxb.namespace.ExpandedName(Namespace, 'scope'), tScope, scope=tActivityContainer, location=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 82, 12)))

def _BuildAutomaton_8 ():
    # Remove this helper function from the namespace after it is invoked
    global _BuildAutomaton_8
    del _BuildAutomaton_8
    import pyxb.utils.fac as fac

    counters = set()
    cc_0 = fac.CounterCondition(min=0, max=None, metadata=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 20, 12))
    counters.add(cc_0)
    states = []
    final_update = None
    symbol = pyxb.binding.content.WildcardUse(pyxb.binding.content.Wildcard(process_contents=pyxb.binding.content.Wildcard.PC_lax, namespace_constraint=(pyxb.binding.content.Wildcard.NC_not, 'http://schemas.xmlsoap.org/ws/2003/03/business-process/')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 20, 12))
    st_0 = fac.State(symbol, is_initial=True, final_update=final_update, is_unordered_catenation=False)
    states.append(st_0)
    final_update = set()
    symbol = pyxb.binding.content.ElementUse(tActivityContainer._UseForTag(pyxb.namespace.ExpandedName(Namespace, 'empty')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 69, 12))
    st_1 = fac.State(symbol, is_initial=True, final_update=final_update, is_unordered_catenation=False)
    states.append(st_1)
    final_update = set()
    symbol = pyxb.binding.content.ElementUse(tActivityContainer._UseForTag(pyxb.namespace.ExpandedName(Namespace, 'invoke')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 70, 12))
    st_2 = fac.State(symbol, is_initial=True, final_update=final_update, is_unordered_catenation=False)
    states.append(st_2)
    final_update = set()
    symbol = pyxb.binding.content.ElementUse(tActivityContainer._UseForTag(pyxb.namespace.ExpandedName(Namespace, 'receive')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 71, 12))
    st_3 = fac.State(symbol, is_initial=True, final_update=final_update, is_unordered_catenation=False)
    states.append(st_3)
    final_update = set()
    symbol = pyxb.binding.content.ElementUse(tActivityContainer._UseForTag(pyxb.namespace.ExpandedName(Namespace, 'reply')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 72, 12))
    st_4 = fac.State(symbol, is_initial=True, final_update=final_update, is_unordered_catenation=False)
    states.append(st_4)
    final_update = set()
    symbol = pyxb.binding.content.ElementUse(tActivityContainer._UseForTag(pyxb.namespace.ExpandedName(Namespace, 'assign')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 73, 12))
    st_5 = fac.State(symbol, is_initial=True, final_update=final_update, is_unordered_catenation=False)
    states.append(st_5)
    final_update = set()
    symbol = pyxb.binding.content.ElementUse(tActivityContainer._UseForTag(pyxb.namespace.ExpandedName(Namespace, 'wait')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 74, 12))
    st_6 = fac.State(symbol, is_initial=True, final_update=final_update, is_unordered_catenation=False)
    states.append(st_6)
    final_update = set()
    symbol = pyxb.binding.content.ElementUse(tActivityContainer._UseForTag(pyxb.namespace.ExpandedName(Namespace, 'throw')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 75, 12))
    st_7 = fac.State(symbol, is_initial=True, final_update=final_update, is_unordered_catenation=False)
    states.append(st_7)
    final_update = set()
    symbol = pyxb.binding.content.ElementUse(tActivityContainer._UseForTag(pyxb.namespace.ExpandedName(Namespace, 'terminate')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 76, 12))
    st_8 = fac.State(symbol, is_initial=True, final_update=final_update, is_unordered_catenation=False)
    states.append(st_8)
    final_update = set()
    symbol = pyxb.binding.content.ElementUse(tActivityContainer._UseForTag(pyxb.namespace.ExpandedName(Namespace, 'flow')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 77, 12))
    st_9 = fac.State(symbol, is_initial=True, final_update=final_update, is_unordered_catenation=False)
    states.append(st_9)
    final_update = set()
    symbol = pyxb.binding.content.ElementUse(tActivityContainer._UseForTag(pyxb.namespace.ExpandedName(Namespace, 'switch')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 78, 12))
    st_10 = fac.State(symbol, is_initial=True, final_update=final_update, is_unordered_catenation=False)
    states.append(st_10)
    final_update = set()
    symbol = pyxb.binding.content.ElementUse(tActivityContainer._UseForTag(pyxb.namespace.ExpandedName(Namespace, 'while')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 79, 12))
    st_11 = fac.State(symbol, is_initial=True, final_update=final_update, is_unordered_catenation=False)
    states.append(st_11)
    final_update = set()
    symbol = pyxb.binding.content.ElementUse(tActivityContainer._UseForTag(pyxb.namespace.ExpandedName(Namespace, 'sequence')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 80, 12))
    st_12 = fac.State(symbol, is_initial=True, final_update=final_update, is_unordered_catenation=False)
    states.append(st_12)
    final_update = set()
    symbol = pyxb.binding.content.ElementUse(tActivityContainer._UseForTag(pyxb.namespace.ExpandedName(Namespace, 'pick')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 81, 12))
    st_13 = fac.State(symbol, is_initial=True, final_update=final_update, is_unordered_catenation=False)
    states.append(st_13)
    final_update = set()
    symbol = pyxb.binding.content.ElementUse(tActivityContainer._UseForTag(pyxb.namespace.ExpandedName(Namespace, 'scope')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 82, 12))
    st_14 = fac.State(symbol, is_initial=True, final_update=final_update, is_unordered_catenation=False)
    states.append(st_14)
    transitions = []
    transitions.append(fac.Transition(st_0, [
        fac.UpdateInstruction(cc_0, True) ]))
    transitions.append(fac.Transition(st_1, [
        fac.UpdateInstruction(cc_0, False) ]))
    transitions.append(fac.Transition(st_2, [
        fac.UpdateInstruction(cc_0, False) ]))
    transitions.append(fac.Transition(st_3, [
        fac.UpdateInstruction(cc_0, False) ]))
    transitions.append(fac.Transition(st_4, [
        fac.UpdateInstruction(cc_0, False) ]))
    transitions.append(fac.Transition(st_5, [
        fac.UpdateInstruction(cc_0, False) ]))
    transitions.append(fac.Transition(st_6, [
        fac.UpdateInstruction(cc_0, False) ]))
    transitions.append(fac.Transition(st_7, [
        fac.UpdateInstruction(cc_0, False) ]))
    transitions.append(fac.Transition(st_8, [
        fac.UpdateInstruction(cc_0, False) ]))
    transitions.append(fac.Transition(st_9, [
        fac.UpdateInstruction(cc_0, False) ]))
    transitions.append(fac.Transition(st_10, [
        fac.UpdateInstruction(cc_0, False) ]))
    transitions.append(fac.Transition(st_11, [
        fac.UpdateInstruction(cc_0, False) ]))
    transitions.append(fac.Transition(st_12, [
        fac.UpdateInstruction(cc_0, False) ]))
    transitions.append(fac.Transition(st_13, [
        fac.UpdateInstruction(cc_0, False) ]))
    transitions.append(fac.Transition(st_14, [
        fac.UpdateInstruction(cc_0, False) ]))
    st_0._set_transitionSet(transitions)
    transitions = []
    st_1._set_transitionSet(transitions)
    transitions = []
    st_2._set_transitionSet(transitions)
    transitions = []
    st_3._set_transitionSet(transitions)
    transitions = []
    st_4._set_transitionSet(transitions)
    transitions = []
    st_5._set_transitionSet(transitions)
    transitions = []
    st_6._set_transitionSet(transitions)
    transitions = []
    st_7._set_transitionSet(transitions)
    transitions = []
    st_8._set_transitionSet(transitions)
    transitions = []
    st_9._set_transitionSet(transitions)
    transitions = []
    st_10._set_transitionSet(transitions)
    transitions = []
    st_11._set_transitionSet(transitions)
    transitions = []
    st_12._set_transitionSet(transitions)
    transitions = []
    st_13._set_transitionSet(transitions)
    transitions = []
    st_14._set_transitionSet(transitions)
    return fac.Automaton(states, counters, False, containing_state=None)
tActivityContainer._Automaton = _BuildAutomaton_8()




tActivityOrCompensateContainer._AddElement(pyxb.binding.basis.element(pyxb.namespace.ExpandedName(Namespace, 'empty'), tEmpty, scope=tActivityOrCompensateContainer, location=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 69, 12)))

tActivityOrCompensateContainer._AddElement(pyxb.binding.basis.element(pyxb.namespace.ExpandedName(Namespace, 'invoke'), tInvoke, scope=tActivityOrCompensateContainer, location=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 70, 12)))

tActivityOrCompensateContainer._AddElement(pyxb.binding.basis.element(pyxb.namespace.ExpandedName(Namespace, 'receive'), tReceive, scope=tActivityOrCompensateContainer, location=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 71, 12)))

tActivityOrCompensateContainer._AddElement(pyxb.binding.basis.element(pyxb.namespace.ExpandedName(Namespace, 'reply'), tReply, scope=tActivityOrCompensateContainer, location=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 72, 12)))

tActivityOrCompensateContainer._AddElement(pyxb.binding.basis.element(pyxb.namespace.ExpandedName(Namespace, 'assign'), tAssign, scope=tActivityOrCompensateContainer, location=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 73, 12)))

tActivityOrCompensateContainer._AddElement(pyxb.binding.basis.element(pyxb.namespace.ExpandedName(Namespace, 'wait'), tWait, scope=tActivityOrCompensateContainer, location=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 74, 12)))

tActivityOrCompensateContainer._AddElement(pyxb.binding.basis.element(pyxb.namespace.ExpandedName(Namespace, 'throw'), tThrow, scope=tActivityOrCompensateContainer, location=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 75, 12)))

tActivityOrCompensateContainer._AddElement(pyxb.binding.basis.element(pyxb.namespace.ExpandedName(Namespace, 'terminate'), tTerminate, scope=tActivityOrCompensateContainer, location=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 76, 12)))

tActivityOrCompensateContainer._AddElement(pyxb.binding.basis.element(pyxb.namespace.ExpandedName(Namespace, 'flow'), tFlow, scope=tActivityOrCompensateContainer, location=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 77, 12)))

tActivityOrCompensateContainer._AddElement(pyxb.binding.basis.element(pyxb.namespace.ExpandedName(Namespace, 'switch'), tSwitch, scope=tActivityOrCompensateContainer, location=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 78, 12)))

tActivityOrCompensateContainer._AddElement(pyxb.binding.basis.element(pyxb.namespace.ExpandedName(Namespace, 'while'), tWhile, scope=tActivityOrCompensateContainer, location=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 79, 12)))

tActivityOrCompensateContainer._AddElement(pyxb.binding.basis.element(pyxb.namespace.ExpandedName(Namespace, 'sequence'), tSequence, scope=tActivityOrCompensateContainer, location=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 80, 12)))

tActivityOrCompensateContainer._AddElement(pyxb.binding.basis.element(pyxb.namespace.ExpandedName(Namespace, 'pick'), tPick, scope=tActivityOrCompensateContainer, location=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 81, 12)))

tActivityOrCompensateContainer._AddElement(pyxb.binding.basis.element(pyxb.namespace.ExpandedName(Namespace, 'scope'), tScope, scope=tActivityOrCompensateContainer, location=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 82, 12)))

tActivityOrCompensateContainer._AddElement(pyxb.binding.basis.element(pyxb.namespace.ExpandedName(Namespace, 'compensate'), tCompensate, scope=tActivityOrCompensateContainer, location=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 181, 20)))

def _BuildAutomaton_9 ():
    # Remove this helper function from the namespace after it is invoked
    global _BuildAutomaton_9
    del _BuildAutomaton_9
    import pyxb.utils.fac as fac

    counters = set()
    cc_0 = fac.CounterCondition(min=0, max=None, metadata=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 20, 12))
    counters.add(cc_0)
    states = []
    final_update = None
    symbol = pyxb.binding.content.WildcardUse(pyxb.binding.content.Wildcard(process_contents=pyxb.binding.content.Wildcard.PC_lax, namespace_constraint=(pyxb.binding.content.Wildcard.NC_not, 'http://schemas.xmlsoap.org/ws/2003/03/business-process/')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 20, 12))
    st_0 = fac.State(symbol, is_initial=True, final_update=final_update, is_unordered_catenation=False)
    states.append(st_0)
    final_update = set()
    symbol = pyxb.binding.content.ElementUse(tActivityOrCompensateContainer._UseForTag(pyxb.namespace.ExpandedName(Namespace, 'empty')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 69, 12))
    st_1 = fac.State(symbol, is_initial=True, final_update=final_update, is_unordered_catenation=False)
    states.append(st_1)
    final_update = set()
    symbol = pyxb.binding.content.ElementUse(tActivityOrCompensateContainer._UseForTag(pyxb.namespace.ExpandedName(Namespace, 'invoke')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 70, 12))
    st_2 = fac.State(symbol, is_initial=True, final_update=final_update, is_unordered_catenation=False)
    states.append(st_2)
    final_update = set()
    symbol = pyxb.binding.content.ElementUse(tActivityOrCompensateContainer._UseForTag(pyxb.namespace.ExpandedName(Namespace, 'receive')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 71, 12))
    st_3 = fac.State(symbol, is_initial=True, final_update=final_update, is_unordered_catenation=False)
    states.append(st_3)
    final_update = set()
    symbol = pyxb.binding.content.ElementUse(tActivityOrCompensateContainer._UseForTag(pyxb.namespace.ExpandedName(Namespace, 'reply')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 72, 12))
    st_4 = fac.State(symbol, is_initial=True, final_update=final_update, is_unordered_catenation=False)
    states.append(st_4)
    final_update = set()
    symbol = pyxb.binding.content.ElementUse(tActivityOrCompensateContainer._UseForTag(pyxb.namespace.ExpandedName(Namespace, 'assign')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 73, 12))
    st_5 = fac.State(symbol, is_initial=True, final_update=final_update, is_unordered_catenation=False)
    states.append(st_5)
    final_update = set()
    symbol = pyxb.binding.content.ElementUse(tActivityOrCompensateContainer._UseForTag(pyxb.namespace.ExpandedName(Namespace, 'wait')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 74, 12))
    st_6 = fac.State(symbol, is_initial=True, final_update=final_update, is_unordered_catenation=False)
    states.append(st_6)
    final_update = set()
    symbol = pyxb.binding.content.ElementUse(tActivityOrCompensateContainer._UseForTag(pyxb.namespace.ExpandedName(Namespace, 'throw')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 75, 12))
    st_7 = fac.State(symbol, is_initial=True, final_update=final_update, is_unordered_catenation=False)
    states.append(st_7)
    final_update = set()
    symbol = pyxb.binding.content.ElementUse(tActivityOrCompensateContainer._UseForTag(pyxb.namespace.ExpandedName(Namespace, 'terminate')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 76, 12))
    st_8 = fac.State(symbol, is_initial=True, final_update=final_update, is_unordered_catenation=False)
    states.append(st_8)
    final_update = set()
    symbol = pyxb.binding.content.ElementUse(tActivityOrCompensateContainer._UseForTag(pyxb.namespace.ExpandedName(Namespace, 'flow')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 77, 12))
    st_9 = fac.State(symbol, is_initial=True, final_update=final_update, is_unordered_catenation=False)
    states.append(st_9)
    final_update = set()
    symbol = pyxb.binding.content.ElementUse(tActivityOrCompensateContainer._UseForTag(pyxb.namespace.ExpandedName(Namespace, 'switch')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 78, 12))
    st_10 = fac.State(symbol, is_initial=True, final_update=final_update, is_unordered_catenation=False)
    states.append(st_10)
    final_update = set()
    symbol = pyxb.binding.content.ElementUse(tActivityOrCompensateContainer._UseForTag(pyxb.namespace.ExpandedName(Namespace, 'while')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 79, 12))
    st_11 = fac.State(symbol, is_initial=True, final_update=final_update, is_unordered_catenation=False)
    states.append(st_11)
    final_update = set()
    symbol = pyxb.binding.content.ElementUse(tActivityOrCompensateContainer._UseForTag(pyxb.namespace.ExpandedName(Namespace, 'sequence')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 80, 12))
    st_12 = fac.State(symbol, is_initial=True, final_update=final_update, is_unordered_catenation=False)
    states.append(st_12)
    final_update = set()
    symbol = pyxb.binding.content.ElementUse(tActivityOrCompensateContainer._UseForTag(pyxb.namespace.ExpandedName(Namespace, 'pick')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 81, 12))
    st_13 = fac.State(symbol, is_initial=True, final_update=final_update, is_unordered_catenation=False)
    states.append(st_13)
    final_update = set()
    symbol = pyxb.binding.content.ElementUse(tActivityOrCompensateContainer._UseForTag(pyxb.namespace.ExpandedName(Namespace, 'scope')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 82, 12))
    st_14 = fac.State(symbol, is_initial=True, final_update=final_update, is_unordered_catenation=False)
    states.append(st_14)
    final_update = set()
    symbol = pyxb.binding.content.ElementUse(tActivityOrCompensateContainer._UseForTag(pyxb.namespace.ExpandedName(Namespace, 'compensate')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 181, 20))
    st_15 = fac.State(symbol, is_initial=True, final_update=final_update, is_unordered_catenation=False)
    states.append(st_15)
    transitions = []
    transitions.append(fac.Transition(st_0, [
        fac.UpdateInstruction(cc_0, True) ]))
    transitions.append(fac.Transition(st_1, [
        fac.UpdateInstruction(cc_0, False) ]))
    transitions.append(fac.Transition(st_2, [
        fac.UpdateInstruction(cc_0, False) ]))
    transitions.append(fac.Transition(st_3, [
        fac.UpdateInstruction(cc_0, False) ]))
    transitions.append(fac.Transition(st_4, [
        fac.UpdateInstruction(cc_0, False) ]))
    transitions.append(fac.Transition(st_5, [
        fac.UpdateInstruction(cc_0, False) ]))
    transitions.append(fac.Transition(st_6, [
        fac.UpdateInstruction(cc_0, False) ]))
    transitions.append(fac.Transition(st_7, [
        fac.UpdateInstruction(cc_0, False) ]))
    transitions.append(fac.Transition(st_8, [
        fac.UpdateInstruction(cc_0, False) ]))
    transitions.append(fac.Transition(st_9, [
        fac.UpdateInstruction(cc_0, False) ]))
    transitions.append(fac.Transition(st_10, [
        fac.UpdateInstruction(cc_0, False) ]))
    transitions.append(fac.Transition(st_11, [
        fac.UpdateInstruction(cc_0, False) ]))
    transitions.append(fac.Transition(st_12, [
        fac.UpdateInstruction(cc_0, False) ]))
    transitions.append(fac.Transition(st_13, [
        fac.UpdateInstruction(cc_0, False) ]))
    transitions.append(fac.Transition(st_14, [
        fac.UpdateInstruction(cc_0, False) ]))
    transitions.append(fac.Transition(st_15, [
        fac.UpdateInstruction(cc_0, False) ]))
    st_0._set_transitionSet(transitions)
    transitions = []
    st_1._set_transitionSet(transitions)
    transitions = []
    st_2._set_transitionSet(transitions)
    transitions = []
    st_3._set_transitionSet(transitions)
    transitions = []
    st_4._set_transitionSet(transitions)
    transitions = []
    st_5._set_transitionSet(transitions)
    transitions = []
    st_6._set_transitionSet(transitions)
    transitions = []
    st_7._set_transitionSet(transitions)
    transitions = []
    st_8._set_transitionSet(transitions)
    transitions = []
    st_9._set_transitionSet(transitions)
    transitions = []
    st_10._set_transitionSet(transitions)
    transitions = []
    st_11._set_transitionSet(transitions)
    transitions = []
    st_12._set_transitionSet(transitions)
    transitions = []
    st_13._set_transitionSet(transitions)
    transitions = []
    st_14._set_transitionSet(transitions)
    transitions = []
    st_15._set_transitionSet(transitions)
    return fac.Automaton(states, counters, False, containing_state=None)
tActivityOrCompensateContainer._Automaton = _BuildAutomaton_9()




tEventHandlers._AddElement(pyxb.binding.basis.element(pyxb.namespace.ExpandedName(Namespace, 'onMessage'), tOnMessage, scope=tEventHandlers, location=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 191, 19)))

tEventHandlers._AddElement(pyxb.binding.basis.element(pyxb.namespace.ExpandedName(Namespace, 'onAlarm'), tOnAlarm, scope=tEventHandlers, location=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 193, 19)))

def _BuildAutomaton_10 ():
    # Remove this helper function from the namespace after it is invoked
    global _BuildAutomaton_10
    del _BuildAutomaton_10
    import pyxb.utils.fac as fac

    counters = set()
    cc_0 = fac.CounterCondition(min=0, max=None, metadata=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 20, 12))
    counters.add(cc_0)
    cc_1 = fac.CounterCondition(min=0, max=None, metadata=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 191, 19))
    counters.add(cc_1)
    cc_2 = fac.CounterCondition(min=0, max=None, metadata=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 193, 19))
    counters.add(cc_2)
    states = []
    final_update = set()
    final_update.add(fac.UpdateInstruction(cc_0, False))
    symbol = pyxb.binding.content.WildcardUse(pyxb.binding.content.Wildcard(process_contents=pyxb.binding.content.Wildcard.PC_lax, namespace_constraint=(pyxb.binding.content.Wildcard.NC_not, 'http://schemas.xmlsoap.org/ws/2003/03/business-process/')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 20, 12))
    st_0 = fac.State(symbol, is_initial=True, final_update=final_update, is_unordered_catenation=False)
    states.append(st_0)
    final_update = set()
    final_update.add(fac.UpdateInstruction(cc_1, False))
    symbol = pyxb.binding.content.ElementUse(tEventHandlers._UseForTag(pyxb.namespace.ExpandedName(Namespace, 'onMessage')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 191, 19))
    st_1 = fac.State(symbol, is_initial=True, final_update=final_update, is_unordered_catenation=False)
    states.append(st_1)
    final_update = set()
    final_update.add(fac.UpdateInstruction(cc_2, False))
    symbol = pyxb.binding.content.ElementUse(tEventHandlers._UseForTag(pyxb.namespace.ExpandedName(Namespace, 'onAlarm')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 193, 19))
    st_2 = fac.State(symbol, is_initial=True, final_update=final_update, is_unordered_catenation=False)
    states.append(st_2)
    transitions = []
    transitions.append(fac.Transition(st_0, [
        fac.UpdateInstruction(cc_0, True) ]))
    transitions.append(fac.Transition(st_1, [
        fac.UpdateInstruction(cc_0, False) ]))
    transitions.append(fac.Transition(st_2, [
        fac.UpdateInstruction(cc_0, False) ]))
    st_0._set_transitionSet(transitions)
    transitions = []
    transitions.append(fac.Transition(st_1, [
        fac.UpdateInstruction(cc_1, True) ]))
    transitions.append(fac.Transition(st_2, [
        fac.UpdateInstruction(cc_1, False) ]))
    st_1._set_transitionSet(transitions)
    transitions = []
    transitions.append(fac.Transition(st_2, [
        fac.UpdateInstruction(cc_2, True) ]))
    st_2._set_transitionSet(transitions)
    return fac.Automaton(states, counters, True, containing_state=None)
tEventHandlers._Automaton = _BuildAutomaton_10()




tOnMessage._AddElement(pyxb.binding.basis.element(pyxb.namespace.ExpandedName(Namespace, 'empty'), tEmpty, scope=tOnMessage, location=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 69, 12)))

tOnMessage._AddElement(pyxb.binding.basis.element(pyxb.namespace.ExpandedName(Namespace, 'invoke'), tInvoke, scope=tOnMessage, location=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 70, 12)))

tOnMessage._AddElement(pyxb.binding.basis.element(pyxb.namespace.ExpandedName(Namespace, 'receive'), tReceive, scope=tOnMessage, location=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 71, 12)))

tOnMessage._AddElement(pyxb.binding.basis.element(pyxb.namespace.ExpandedName(Namespace, 'reply'), tReply, scope=tOnMessage, location=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 72, 12)))

tOnMessage._AddElement(pyxb.binding.basis.element(pyxb.namespace.ExpandedName(Namespace, 'assign'), tAssign, scope=tOnMessage, location=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 73, 12)))

tOnMessage._AddElement(pyxb.binding.basis.element(pyxb.namespace.ExpandedName(Namespace, 'wait'), tWait, scope=tOnMessage, location=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 74, 12)))

tOnMessage._AddElement(pyxb.binding.basis.element(pyxb.namespace.ExpandedName(Namespace, 'throw'), tThrow, scope=tOnMessage, location=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 75, 12)))

tOnMessage._AddElement(pyxb.binding.basis.element(pyxb.namespace.ExpandedName(Namespace, 'terminate'), tTerminate, scope=tOnMessage, location=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 76, 12)))

tOnMessage._AddElement(pyxb.binding.basis.element(pyxb.namespace.ExpandedName(Namespace, 'flow'), tFlow, scope=tOnMessage, location=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 77, 12)))

tOnMessage._AddElement(pyxb.binding.basis.element(pyxb.namespace.ExpandedName(Namespace, 'switch'), tSwitch, scope=tOnMessage, location=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 78, 12)))

tOnMessage._AddElement(pyxb.binding.basis.element(pyxb.namespace.ExpandedName(Namespace, 'while'), tWhile, scope=tOnMessage, location=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 79, 12)))

tOnMessage._AddElement(pyxb.binding.basis.element(pyxb.namespace.ExpandedName(Namespace, 'sequence'), tSequence, scope=tOnMessage, location=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 80, 12)))

tOnMessage._AddElement(pyxb.binding.basis.element(pyxb.namespace.ExpandedName(Namespace, 'pick'), tPick, scope=tOnMessage, location=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 81, 12)))

tOnMessage._AddElement(pyxb.binding.basis.element(pyxb.namespace.ExpandedName(Namespace, 'scope'), tScope, scope=tOnMessage, location=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 82, 12)))

tOnMessage._AddElement(pyxb.binding.basis.element(pyxb.namespace.ExpandedName(Namespace, 'correlations'), tCorrelations, scope=tOnMessage, location=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 205, 20)))

def _BuildAutomaton_11 ():
    # Remove this helper function from the namespace after it is invoked
    global _BuildAutomaton_11
    del _BuildAutomaton_11
    import pyxb.utils.fac as fac

    counters = set()
    cc_0 = fac.CounterCondition(min=0, max=None, metadata=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 20, 12))
    counters.add(cc_0)
    cc_1 = fac.CounterCondition(min=0, max=1, metadata=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 205, 20))
    counters.add(cc_1)
    states = []
    final_update = None
    symbol = pyxb.binding.content.WildcardUse(pyxb.binding.content.Wildcard(process_contents=pyxb.binding.content.Wildcard.PC_lax, namespace_constraint=(pyxb.binding.content.Wildcard.NC_not, 'http://schemas.xmlsoap.org/ws/2003/03/business-process/')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 20, 12))
    st_0 = fac.State(symbol, is_initial=True, final_update=final_update, is_unordered_catenation=False)
    states.append(st_0)
    final_update = None
    symbol = pyxb.binding.content.ElementUse(tOnMessage._UseForTag(pyxb.namespace.ExpandedName(Namespace, 'correlations')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 205, 20))
    st_1 = fac.State(symbol, is_initial=True, final_update=final_update, is_unordered_catenation=False)
    states.append(st_1)
    final_update = set()
    symbol = pyxb.binding.content.ElementUse(tOnMessage._UseForTag(pyxb.namespace.ExpandedName(Namespace, 'empty')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 69, 12))
    st_2 = fac.State(symbol, is_initial=True, final_update=final_update, is_unordered_catenation=False)
    states.append(st_2)
    final_update = set()
    symbol = pyxb.binding.content.ElementUse(tOnMessage._UseForTag(pyxb.namespace.ExpandedName(Namespace, 'invoke')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 70, 12))
    st_3 = fac.State(symbol, is_initial=True, final_update=final_update, is_unordered_catenation=False)
    states.append(st_3)
    final_update = set()
    symbol = pyxb.binding.content.ElementUse(tOnMessage._UseForTag(pyxb.namespace.ExpandedName(Namespace, 'receive')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 71, 12))
    st_4 = fac.State(symbol, is_initial=True, final_update=final_update, is_unordered_catenation=False)
    states.append(st_4)
    final_update = set()
    symbol = pyxb.binding.content.ElementUse(tOnMessage._UseForTag(pyxb.namespace.ExpandedName(Namespace, 'reply')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 72, 12))
    st_5 = fac.State(symbol, is_initial=True, final_update=final_update, is_unordered_catenation=False)
    states.append(st_5)
    final_update = set()
    symbol = pyxb.binding.content.ElementUse(tOnMessage._UseForTag(pyxb.namespace.ExpandedName(Namespace, 'assign')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 73, 12))
    st_6 = fac.State(symbol, is_initial=True, final_update=final_update, is_unordered_catenation=False)
    states.append(st_6)
    final_update = set()
    symbol = pyxb.binding.content.ElementUse(tOnMessage._UseForTag(pyxb.namespace.ExpandedName(Namespace, 'wait')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 74, 12))
    st_7 = fac.State(symbol, is_initial=True, final_update=final_update, is_unordered_catenation=False)
    states.append(st_7)
    final_update = set()
    symbol = pyxb.binding.content.ElementUse(tOnMessage._UseForTag(pyxb.namespace.ExpandedName(Namespace, 'throw')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 75, 12))
    st_8 = fac.State(symbol, is_initial=True, final_update=final_update, is_unordered_catenation=False)
    states.append(st_8)
    final_update = set()
    symbol = pyxb.binding.content.ElementUse(tOnMessage._UseForTag(pyxb.namespace.ExpandedName(Namespace, 'terminate')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 76, 12))
    st_9 = fac.State(symbol, is_initial=True, final_update=final_update, is_unordered_catenation=False)
    states.append(st_9)
    final_update = set()
    symbol = pyxb.binding.content.ElementUse(tOnMessage._UseForTag(pyxb.namespace.ExpandedName(Namespace, 'flow')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 77, 12))
    st_10 = fac.State(symbol, is_initial=True, final_update=final_update, is_unordered_catenation=False)
    states.append(st_10)
    final_update = set()
    symbol = pyxb.binding.content.ElementUse(tOnMessage._UseForTag(pyxb.namespace.ExpandedName(Namespace, 'switch')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 78, 12))
    st_11 = fac.State(symbol, is_initial=True, final_update=final_update, is_unordered_catenation=False)
    states.append(st_11)
    final_update = set()
    symbol = pyxb.binding.content.ElementUse(tOnMessage._UseForTag(pyxb.namespace.ExpandedName(Namespace, 'while')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 79, 12))
    st_12 = fac.State(symbol, is_initial=True, final_update=final_update, is_unordered_catenation=False)
    states.append(st_12)
    final_update = set()
    symbol = pyxb.binding.content.ElementUse(tOnMessage._UseForTag(pyxb.namespace.ExpandedName(Namespace, 'sequence')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 80, 12))
    st_13 = fac.State(symbol, is_initial=True, final_update=final_update, is_unordered_catenation=False)
    states.append(st_13)
    final_update = set()
    symbol = pyxb.binding.content.ElementUse(tOnMessage._UseForTag(pyxb.namespace.ExpandedName(Namespace, 'pick')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 81, 12))
    st_14 = fac.State(symbol, is_initial=True, final_update=final_update, is_unordered_catenation=False)
    states.append(st_14)
    final_update = set()
    symbol = pyxb.binding.content.ElementUse(tOnMessage._UseForTag(pyxb.namespace.ExpandedName(Namespace, 'scope')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 82, 12))
    st_15 = fac.State(symbol, is_initial=True, final_update=final_update, is_unordered_catenation=False)
    states.append(st_15)
    transitions = []
    transitions.append(fac.Transition(st_0, [
        fac.UpdateInstruction(cc_0, True) ]))
    transitions.append(fac.Transition(st_1, [
        fac.UpdateInstruction(cc_0, False) ]))
    transitions.append(fac.Transition(st_2, [
        fac.UpdateInstruction(cc_0, False) ]))
    transitions.append(fac.Transition(st_3, [
        fac.UpdateInstruction(cc_0, False) ]))
    transitions.append(fac.Transition(st_4, [
        fac.UpdateInstruction(cc_0, False) ]))
    transitions.append(fac.Transition(st_5, [
        fac.UpdateInstruction(cc_0, False) ]))
    transitions.append(fac.Transition(st_6, [
        fac.UpdateInstruction(cc_0, False) ]))
    transitions.append(fac.Transition(st_7, [
        fac.UpdateInstruction(cc_0, False) ]))
    transitions.append(fac.Transition(st_8, [
        fac.UpdateInstruction(cc_0, False) ]))
    transitions.append(fac.Transition(st_9, [
        fac.UpdateInstruction(cc_0, False) ]))
    transitions.append(fac.Transition(st_10, [
        fac.UpdateInstruction(cc_0, False) ]))
    transitions.append(fac.Transition(st_11, [
        fac.UpdateInstruction(cc_0, False) ]))
    transitions.append(fac.Transition(st_12, [
        fac.UpdateInstruction(cc_0, False) ]))
    transitions.append(fac.Transition(st_13, [
        fac.UpdateInstruction(cc_0, False) ]))
    transitions.append(fac.Transition(st_14, [
        fac.UpdateInstruction(cc_0, False) ]))
    transitions.append(fac.Transition(st_15, [
        fac.UpdateInstruction(cc_0, False) ]))
    st_0._set_transitionSet(transitions)
    transitions = []
    transitions.append(fac.Transition(st_1, [
        fac.UpdateInstruction(cc_1, True) ]))
    transitions.append(fac.Transition(st_2, [
        fac.UpdateInstruction(cc_1, False) ]))
    transitions.append(fac.Transition(st_3, [
        fac.UpdateInstruction(cc_1, False) ]))
    transitions.append(fac.Transition(st_4, [
        fac.UpdateInstruction(cc_1, False) ]))
    transitions.append(fac.Transition(st_5, [
        fac.UpdateInstruction(cc_1, False) ]))
    transitions.append(fac.Transition(st_6, [
        fac.UpdateInstruction(cc_1, False) ]))
    transitions.append(fac.Transition(st_7, [
        fac.UpdateInstruction(cc_1, False) ]))
    transitions.append(fac.Transition(st_8, [
        fac.UpdateInstruction(cc_1, False) ]))
    transitions.append(fac.Transition(st_9, [
        fac.UpdateInstruction(cc_1, False) ]))
    transitions.append(fac.Transition(st_10, [
        fac.UpdateInstruction(cc_1, False) ]))
    transitions.append(fac.Transition(st_11, [
        fac.UpdateInstruction(cc_1, False) ]))
    transitions.append(fac.Transition(st_12, [
        fac.UpdateInstruction(cc_1, False) ]))
    transitions.append(fac.Transition(st_13, [
        fac.UpdateInstruction(cc_1, False) ]))
    transitions.append(fac.Transition(st_14, [
        fac.UpdateInstruction(cc_1, False) ]))
    transitions.append(fac.Transition(st_15, [
        fac.UpdateInstruction(cc_1, False) ]))
    st_1._set_transitionSet(transitions)
    transitions = []
    st_2._set_transitionSet(transitions)
    transitions = []
    st_3._set_transitionSet(transitions)
    transitions = []
    st_4._set_transitionSet(transitions)
    transitions = []
    st_5._set_transitionSet(transitions)
    transitions = []
    st_6._set_transitionSet(transitions)
    transitions = []
    st_7._set_transitionSet(transitions)
    transitions = []
    st_8._set_transitionSet(transitions)
    transitions = []
    st_9._set_transitionSet(transitions)
    transitions = []
    st_10._set_transitionSet(transitions)
    transitions = []
    st_11._set_transitionSet(transitions)
    transitions = []
    st_12._set_transitionSet(transitions)
    transitions = []
    st_13._set_transitionSet(transitions)
    transitions = []
    st_14._set_transitionSet(transitions)
    transitions = []
    st_15._set_transitionSet(transitions)
    return fac.Automaton(states, counters, False, containing_state=None)
tOnMessage._Automaton = _BuildAutomaton_11()




tVariables._AddElement(pyxb.binding.basis.element(pyxb.namespace.ExpandedName(Namespace, 'variable'), tVariable, scope=tVariables, location=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 238, 20)))

def _BuildAutomaton_12 ():
    # Remove this helper function from the namespace after it is invoked
    global _BuildAutomaton_12
    del _BuildAutomaton_12
    import pyxb.utils.fac as fac

    counters = set()
    cc_0 = fac.CounterCondition(min=0, max=None, metadata=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 20, 12))
    counters.add(cc_0)
    states = []
    final_update = None
    symbol = pyxb.binding.content.WildcardUse(pyxb.binding.content.Wildcard(process_contents=pyxb.binding.content.Wildcard.PC_lax, namespace_constraint=(pyxb.binding.content.Wildcard.NC_not, 'http://schemas.xmlsoap.org/ws/2003/03/business-process/')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 20, 12))
    st_0 = fac.State(symbol, is_initial=True, final_update=final_update, is_unordered_catenation=False)
    states.append(st_0)
    final_update = set()
    symbol = pyxb.binding.content.ElementUse(tVariables._UseForTag(pyxb.namespace.ExpandedName(Namespace, 'variable')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 238, 20))
    st_1 = fac.State(symbol, is_initial=True, final_update=final_update, is_unordered_catenation=False)
    states.append(st_1)
    transitions = []
    transitions.append(fac.Transition(st_0, [
        fac.UpdateInstruction(cc_0, True) ]))
    transitions.append(fac.Transition(st_1, [
        fac.UpdateInstruction(cc_0, False) ]))
    st_0._set_transitionSet(transitions)
    transitions = []
    transitions.append(fac.Transition(st_1, [
         ]))
    st_1._set_transitionSet(transitions)
    return fac.Automaton(states, counters, False, containing_state=None)
tVariables._Automaton = _BuildAutomaton_12()




tCorrelationSets._AddElement(pyxb.binding.basis.element(pyxb.namespace.ExpandedName(Namespace, 'correlationSet'), tCorrelationSet, scope=tCorrelationSets, location=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 263, 20)))

def _BuildAutomaton_13 ():
    # Remove this helper function from the namespace after it is invoked
    global _BuildAutomaton_13
    del _BuildAutomaton_13
    import pyxb.utils.fac as fac

    counters = set()
    cc_0 = fac.CounterCondition(min=0, max=None, metadata=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 20, 12))
    counters.add(cc_0)
    states = []
    final_update = None
    symbol = pyxb.binding.content.WildcardUse(pyxb.binding.content.Wildcard(process_contents=pyxb.binding.content.Wildcard.PC_lax, namespace_constraint=(pyxb.binding.content.Wildcard.NC_not, 'http://schemas.xmlsoap.org/ws/2003/03/business-process/')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 20, 12))
    st_0 = fac.State(symbol, is_initial=True, final_update=final_update, is_unordered_catenation=False)
    states.append(st_0)
    final_update = set()
    symbol = pyxb.binding.content.ElementUse(tCorrelationSets._UseForTag(pyxb.namespace.ExpandedName(Namespace, 'correlationSet')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 263, 20))
    st_1 = fac.State(symbol, is_initial=True, final_update=final_update, is_unordered_catenation=False)
    states.append(st_1)
    transitions = []
    transitions.append(fac.Transition(st_0, [
        fac.UpdateInstruction(cc_0, True) ]))
    transitions.append(fac.Transition(st_1, [
        fac.UpdateInstruction(cc_0, False) ]))
    st_0._set_transitionSet(transitions)
    transitions = []
    transitions.append(fac.Transition(st_1, [
         ]))
    st_1._set_transitionSet(transitions)
    return fac.Automaton(states, counters, False, containing_state=None)
tCorrelationSets._Automaton = _BuildAutomaton_13()




def _BuildAutomaton_14 ():
    # Remove this helper function from the namespace after it is invoked
    global _BuildAutomaton_14
    del _BuildAutomaton_14
    import pyxb.utils.fac as fac

    counters = set()
    cc_0 = fac.CounterCondition(min=0, max=None, metadata=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 20, 12))
    counters.add(cc_0)
    states = []
    final_update = set()
    final_update.add(fac.UpdateInstruction(cc_0, False))
    symbol = pyxb.binding.content.WildcardUse(pyxb.binding.content.Wildcard(process_contents=pyxb.binding.content.Wildcard.PC_lax, namespace_constraint=(pyxb.binding.content.Wildcard.NC_not, 'http://schemas.xmlsoap.org/ws/2003/03/business-process/')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 20, 12))
    st_0 = fac.State(symbol, is_initial=True, final_update=final_update, is_unordered_catenation=False)
    states.append(st_0)
    transitions = []
    transitions.append(fac.Transition(st_0, [
        fac.UpdateInstruction(cc_0, True) ]))
    st_0._set_transitionSet(transitions)
    return fac.Automaton(states, counters, True, containing_state=None)
tCorrelationSet._Automaton = _BuildAutomaton_14()




tActivity._AddElement(pyxb.binding.basis.element(pyxb.namespace.ExpandedName(Namespace, 'target'), tTarget, scope=tActivity, location=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 290, 20)))

tActivity._AddElement(pyxb.binding.basis.element(pyxb.namespace.ExpandedName(Namespace, 'source'), tSource, scope=tActivity, location=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 292, 20)))

def _BuildAutomaton_15 ():
    # Remove this helper function from the namespace after it is invoked
    global _BuildAutomaton_15
    del _BuildAutomaton_15
    import pyxb.utils.fac as fac

    counters = set()
    cc_0 = fac.CounterCondition(min=0, max=None, metadata=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 20, 12))
    counters.add(cc_0)
    cc_1 = fac.CounterCondition(min=0, max=None, metadata=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 290, 20))
    counters.add(cc_1)
    cc_2 = fac.CounterCondition(min=0, max=None, metadata=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 292, 20))
    counters.add(cc_2)
    states = []
    final_update = set()
    final_update.add(fac.UpdateInstruction(cc_0, False))
    symbol = pyxb.binding.content.WildcardUse(pyxb.binding.content.Wildcard(process_contents=pyxb.binding.content.Wildcard.PC_lax, namespace_constraint=(pyxb.binding.content.Wildcard.NC_not, 'http://schemas.xmlsoap.org/ws/2003/03/business-process/')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 20, 12))
    st_0 = fac.State(symbol, is_initial=True, final_update=final_update, is_unordered_catenation=False)
    states.append(st_0)
    final_update = set()
    final_update.add(fac.UpdateInstruction(cc_1, False))
    symbol = pyxb.binding.content.ElementUse(tActivity._UseForTag(pyxb.namespace.ExpandedName(Namespace, 'target')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 290, 20))
    st_1 = fac.State(symbol, is_initial=True, final_update=final_update, is_unordered_catenation=False)
    states.append(st_1)
    final_update = set()
    final_update.add(fac.UpdateInstruction(cc_2, False))
    symbol = pyxb.binding.content.ElementUse(tActivity._UseForTag(pyxb.namespace.ExpandedName(Namespace, 'source')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 292, 20))
    st_2 = fac.State(symbol, is_initial=True, final_update=final_update, is_unordered_catenation=False)
    states.append(st_2)
    transitions = []
    transitions.append(fac.Transition(st_0, [
        fac.UpdateInstruction(cc_0, True) ]))
    transitions.append(fac.Transition(st_1, [
        fac.UpdateInstruction(cc_0, False) ]))
    transitions.append(fac.Transition(st_2, [
        fac.UpdateInstruction(cc_0, False) ]))
    st_0._set_transitionSet(transitions)
    transitions = []
    transitions.append(fac.Transition(st_1, [
        fac.UpdateInstruction(cc_1, True) ]))
    transitions.append(fac.Transition(st_2, [
        fac.UpdateInstruction(cc_1, False) ]))
    st_1._set_transitionSet(transitions)
    transitions = []
    transitions.append(fac.Transition(st_2, [
        fac.UpdateInstruction(cc_2, True) ]))
    st_2._set_transitionSet(transitions)
    return fac.Automaton(states, counters, True, containing_state=None)
tActivity._Automaton = _BuildAutomaton_15()




def _BuildAutomaton_16 ():
    # Remove this helper function from the namespace after it is invoked
    global _BuildAutomaton_16
    del _BuildAutomaton_16
    import pyxb.utils.fac as fac

    counters = set()
    cc_0 = fac.CounterCondition(min=0, max=None, metadata=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 20, 12))
    counters.add(cc_0)
    states = []
    final_update = set()
    final_update.add(fac.UpdateInstruction(cc_0, False))
    symbol = pyxb.binding.content.WildcardUse(pyxb.binding.content.Wildcard(process_contents=pyxb.binding.content.Wildcard.PC_lax, namespace_constraint=(pyxb.binding.content.Wildcard.NC_not, 'http://schemas.xmlsoap.org/ws/2003/03/business-process/')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 20, 12))
    st_0 = fac.State(symbol, is_initial=True, final_update=final_update, is_unordered_catenation=False)
    states.append(st_0)
    transitions = []
    transitions.append(fac.Transition(st_0, [
        fac.UpdateInstruction(cc_0, True) ]))
    st_0._set_transitionSet(transitions)
    return fac.Automaton(states, counters, True, containing_state=None)
tSource._Automaton = _BuildAutomaton_16()




def _BuildAutomaton_17 ():
    # Remove this helper function from the namespace after it is invoked
    global _BuildAutomaton_17
    del _BuildAutomaton_17
    import pyxb.utils.fac as fac

    counters = set()
    cc_0 = fac.CounterCondition(min=0, max=None, metadata=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 20, 12))
    counters.add(cc_0)
    states = []
    final_update = set()
    final_update.add(fac.UpdateInstruction(cc_0, False))
    symbol = pyxb.binding.content.WildcardUse(pyxb.binding.content.Wildcard(process_contents=pyxb.binding.content.Wildcard.PC_lax, namespace_constraint=(pyxb.binding.content.Wildcard.NC_not, 'http://schemas.xmlsoap.org/ws/2003/03/business-process/')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 20, 12))
    st_0 = fac.State(symbol, is_initial=True, final_update=final_update, is_unordered_catenation=False)
    states.append(st_0)
    transitions = []
    transitions.append(fac.Transition(st_0, [
        fac.UpdateInstruction(cc_0, True) ]))
    st_0._set_transitionSet(transitions)
    return fac.Automaton(states, counters, True, containing_state=None)
tTarget._Automaton = _BuildAutomaton_17()




tCorrelations._AddElement(pyxb.binding.basis.element(pyxb.namespace.ExpandedName(Namespace, 'correlation'), tCorrelation, scope=tCorrelations, location=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 335, 18)))

def _BuildAutomaton_18 ():
    # Remove this helper function from the namespace after it is invoked
    global _BuildAutomaton_18
    del _BuildAutomaton_18
    import pyxb.utils.fac as fac

    counters = set()
    cc_0 = fac.CounterCondition(min=0, max=None, metadata=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 20, 12))
    counters.add(cc_0)
    states = []
    final_update = None
    symbol = pyxb.binding.content.WildcardUse(pyxb.binding.content.Wildcard(process_contents=pyxb.binding.content.Wildcard.PC_lax, namespace_constraint=(pyxb.binding.content.Wildcard.NC_not, 'http://schemas.xmlsoap.org/ws/2003/03/business-process/')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 20, 12))
    st_0 = fac.State(symbol, is_initial=True, final_update=final_update, is_unordered_catenation=False)
    states.append(st_0)
    final_update = set()
    symbol = pyxb.binding.content.ElementUse(tCorrelations._UseForTag(pyxb.namespace.ExpandedName(Namespace, 'correlation')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 335, 18))
    st_1 = fac.State(symbol, is_initial=True, final_update=final_update, is_unordered_catenation=False)
    states.append(st_1)
    transitions = []
    transitions.append(fac.Transition(st_0, [
        fac.UpdateInstruction(cc_0, True) ]))
    transitions.append(fac.Transition(st_1, [
        fac.UpdateInstruction(cc_0, False) ]))
    st_0._set_transitionSet(transitions)
    transitions = []
    transitions.append(fac.Transition(st_1, [
         ]))
    st_1._set_transitionSet(transitions)
    return fac.Automaton(states, counters, False, containing_state=None)
tCorrelations._Automaton = _BuildAutomaton_18()




def _BuildAutomaton_19 ():
    # Remove this helper function from the namespace after it is invoked
    global _BuildAutomaton_19
    del _BuildAutomaton_19
    import pyxb.utils.fac as fac

    counters = set()
    cc_0 = fac.CounterCondition(min=0, max=None, metadata=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 20, 12))
    counters.add(cc_0)
    states = []
    final_update = set()
    final_update.add(fac.UpdateInstruction(cc_0, False))
    symbol = pyxb.binding.content.WildcardUse(pyxb.binding.content.Wildcard(process_contents=pyxb.binding.content.Wildcard.PC_lax, namespace_constraint=(pyxb.binding.content.Wildcard.NC_not, 'http://schemas.xmlsoap.org/ws/2003/03/business-process/')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 20, 12))
    st_0 = fac.State(symbol, is_initial=True, final_update=final_update, is_unordered_catenation=False)
    states.append(st_0)
    transitions = []
    transitions.append(fac.Transition(st_0, [
        fac.UpdateInstruction(cc_0, True) ]))
    st_0._set_transitionSet(transitions)
    return fac.Automaton(states, counters, True, containing_state=None)
tCorrelation._Automaton = _BuildAutomaton_19()




tCorrelationsWithPattern._AddElement(pyxb.binding.basis.element(pyxb.namespace.ExpandedName(Namespace, 'correlation'), tCorrelationWithPattern, scope=tCorrelationsWithPattern, location=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 357, 20)))

def _BuildAutomaton_20 ():
    # Remove this helper function from the namespace after it is invoked
    global _BuildAutomaton_20
    del _BuildAutomaton_20
    import pyxb.utils.fac as fac

    counters = set()
    cc_0 = fac.CounterCondition(min=0, max=None, metadata=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 20, 12))
    counters.add(cc_0)
    states = []
    final_update = None
    symbol = pyxb.binding.content.WildcardUse(pyxb.binding.content.Wildcard(process_contents=pyxb.binding.content.Wildcard.PC_lax, namespace_constraint=(pyxb.binding.content.Wildcard.NC_not, 'http://schemas.xmlsoap.org/ws/2003/03/business-process/')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 20, 12))
    st_0 = fac.State(symbol, is_initial=True, final_update=final_update, is_unordered_catenation=False)
    states.append(st_0)
    final_update = set()
    symbol = pyxb.binding.content.ElementUse(tCorrelationsWithPattern._UseForTag(pyxb.namespace.ExpandedName(Namespace, 'correlation')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 357, 20))
    st_1 = fac.State(symbol, is_initial=True, final_update=final_update, is_unordered_catenation=False)
    states.append(st_1)
    transitions = []
    transitions.append(fac.Transition(st_0, [
        fac.UpdateInstruction(cc_0, True) ]))
    transitions.append(fac.Transition(st_1, [
        fac.UpdateInstruction(cc_0, False) ]))
    st_0._set_transitionSet(transitions)
    transitions = []
    transitions.append(fac.Transition(st_1, [
         ]))
    st_1._set_transitionSet(transitions)
    return fac.Automaton(states, counters, False, containing_state=None)
tCorrelationsWithPattern._Automaton = _BuildAutomaton_20()




tCopy._AddElement(pyxb.binding.basis.element(pyxb.namespace.ExpandedName(Namespace, 'from'), tFrom, scope=tCopy, location=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 466, 4)))

tCopy._AddElement(pyxb.binding.basis.element(pyxb.namespace.ExpandedName(Namespace, 'to'), CTD_ANON_, scope=tCopy, location=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 482, 4)))

def _BuildAutomaton_21 ():
    # Remove this helper function from the namespace after it is invoked
    global _BuildAutomaton_21
    del _BuildAutomaton_21
    import pyxb.utils.fac as fac

    counters = set()
    cc_0 = fac.CounterCondition(min=0, max=None, metadata=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 20, 12))
    counters.add(cc_0)
    states = []
    final_update = None
    symbol = pyxb.binding.content.WildcardUse(pyxb.binding.content.Wildcard(process_contents=pyxb.binding.content.Wildcard.PC_lax, namespace_constraint=(pyxb.binding.content.Wildcard.NC_not, 'http://schemas.xmlsoap.org/ws/2003/03/business-process/')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 20, 12))
    st_0 = fac.State(symbol, is_initial=True, final_update=final_update, is_unordered_catenation=False)
    states.append(st_0)
    final_update = None
    symbol = pyxb.binding.content.ElementUse(tCopy._UseForTag(pyxb.namespace.ExpandedName(Namespace, 'from')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 459, 20))
    st_1 = fac.State(symbol, is_initial=True, final_update=final_update, is_unordered_catenation=False)
    states.append(st_1)
    final_update = set()
    symbol = pyxb.binding.content.ElementUse(tCopy._UseForTag(pyxb.namespace.ExpandedName(Namespace, 'to')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 460, 20))
    st_2 = fac.State(symbol, is_initial=False, final_update=final_update, is_unordered_catenation=False)
    states.append(st_2)
    transitions = []
    transitions.append(fac.Transition(st_0, [
        fac.UpdateInstruction(cc_0, True) ]))
    transitions.append(fac.Transition(st_1, [
        fac.UpdateInstruction(cc_0, False) ]))
    st_0._set_transitionSet(transitions)
    transitions = []
    transitions.append(fac.Transition(st_2, [
         ]))
    st_1._set_transitionSet(transitions)
    transitions = []
    st_2._set_transitionSet(transitions)
    return fac.Automaton(states, counters, False, containing_state=None)
tCopy._Automaton = _BuildAutomaton_21()




def _BuildAutomaton_22 ():
    # Remove this helper function from the namespace after it is invoked
    global _BuildAutomaton_22
    del _BuildAutomaton_22
    import pyxb.utils.fac as fac

    counters = set()
    cc_0 = fac.CounterCondition(min=0, max=None, metadata=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 20, 12))
    counters.add(cc_0)
    states = []
    final_update = set()
    final_update.add(fac.UpdateInstruction(cc_0, False))
    symbol = pyxb.binding.content.WildcardUse(pyxb.binding.content.Wildcard(process_contents=pyxb.binding.content.Wildcard.PC_lax, namespace_constraint=(pyxb.binding.content.Wildcard.NC_not, 'http://schemas.xmlsoap.org/ws/2003/03/business-process/')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 20, 12))
    st_0 = fac.State(symbol, is_initial=True, final_update=final_update, is_unordered_catenation=False)
    states.append(st_0)
    transitions = []
    transitions.append(fac.Transition(st_0, [
        fac.UpdateInstruction(cc_0, True) ]))
    st_0._set_transitionSet(transitions)
    return fac.Automaton(states, counters, True, containing_state=None)
tFrom._Automaton = _BuildAutomaton_22()




tLinks._AddElement(pyxb.binding.basis.element(pyxb.namespace.ExpandedName(Namespace, 'link'), tLink, scope=tLinks, location=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 547, 20)))

def _BuildAutomaton_23 ():
    # Remove this helper function from the namespace after it is invoked
    global _BuildAutomaton_23
    del _BuildAutomaton_23
    import pyxb.utils.fac as fac

    counters = set()
    cc_0 = fac.CounterCondition(min=0, max=None, metadata=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 20, 12))
    counters.add(cc_0)
    states = []
    final_update = None
    symbol = pyxb.binding.content.WildcardUse(pyxb.binding.content.Wildcard(process_contents=pyxb.binding.content.Wildcard.PC_lax, namespace_constraint=(pyxb.binding.content.Wildcard.NC_not, 'http://schemas.xmlsoap.org/ws/2003/03/business-process/')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 20, 12))
    st_0 = fac.State(symbol, is_initial=True, final_update=final_update, is_unordered_catenation=False)
    states.append(st_0)
    final_update = set()
    symbol = pyxb.binding.content.ElementUse(tLinks._UseForTag(pyxb.namespace.ExpandedName(Namespace, 'link')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 547, 20))
    st_1 = fac.State(symbol, is_initial=True, final_update=final_update, is_unordered_catenation=False)
    states.append(st_1)
    transitions = []
    transitions.append(fac.Transition(st_0, [
        fac.UpdateInstruction(cc_0, True) ]))
    transitions.append(fac.Transition(st_1, [
        fac.UpdateInstruction(cc_0, False) ]))
    st_0._set_transitionSet(transitions)
    transitions = []
    transitions.append(fac.Transition(st_1, [
         ]))
    st_1._set_transitionSet(transitions)
    return fac.Automaton(states, counters, False, containing_state=None)
tLinks._Automaton = _BuildAutomaton_23()




def _BuildAutomaton_24 ():
    # Remove this helper function from the namespace after it is invoked
    global _BuildAutomaton_24
    del _BuildAutomaton_24
    import pyxb.utils.fac as fac

    counters = set()
    cc_0 = fac.CounterCondition(min=0, max=None, metadata=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 20, 12))
    counters.add(cc_0)
    states = []
    final_update = set()
    final_update.add(fac.UpdateInstruction(cc_0, False))
    symbol = pyxb.binding.content.WildcardUse(pyxb.binding.content.Wildcard(process_contents=pyxb.binding.content.Wildcard.PC_lax, namespace_constraint=(pyxb.binding.content.Wildcard.NC_not, 'http://schemas.xmlsoap.org/ws/2003/03/business-process/')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 20, 12))
    st_0 = fac.State(symbol, is_initial=True, final_update=final_update, is_unordered_catenation=False)
    states.append(st_0)
    transitions = []
    transitions.append(fac.Transition(st_0, [
        fac.UpdateInstruction(cc_0, True) ]))
    st_0._set_transitionSet(transitions)
    return fac.Automaton(states, counters, True, containing_state=None)
tLink._Automaton = _BuildAutomaton_24()




def _BuildAutomaton_25 ():
    # Remove this helper function from the namespace after it is invoked
    global _BuildAutomaton_25
    del _BuildAutomaton_25
    import pyxb.utils.fac as fac

    counters = set()
    cc_0 = fac.CounterCondition(min=0, max=None, metadata=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 20, 12))
    counters.add(cc_0)
    states = []
    final_update = None
    symbol = pyxb.binding.content.WildcardUse(pyxb.binding.content.Wildcard(process_contents=pyxb.binding.content.Wildcard.PC_lax, namespace_constraint=(pyxb.binding.content.Wildcard.NC_not, 'http://schemas.xmlsoap.org/ws/2003/03/business-process/')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 20, 12))
    st_0 = fac.State(symbol, is_initial=True, final_update=final_update, is_unordered_catenation=False)
    states.append(st_0)
    final_update = set()
    symbol = pyxb.binding.content.ElementUse(tCatch._UseForTag(pyxb.namespace.ExpandedName(Namespace, 'empty')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 69, 12))
    st_1 = fac.State(symbol, is_initial=True, final_update=final_update, is_unordered_catenation=False)
    states.append(st_1)
    final_update = set()
    symbol = pyxb.binding.content.ElementUse(tCatch._UseForTag(pyxb.namespace.ExpandedName(Namespace, 'invoke')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 70, 12))
    st_2 = fac.State(symbol, is_initial=True, final_update=final_update, is_unordered_catenation=False)
    states.append(st_2)
    final_update = set()
    symbol = pyxb.binding.content.ElementUse(tCatch._UseForTag(pyxb.namespace.ExpandedName(Namespace, 'receive')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 71, 12))
    st_3 = fac.State(symbol, is_initial=True, final_update=final_update, is_unordered_catenation=False)
    states.append(st_3)
    final_update = set()
    symbol = pyxb.binding.content.ElementUse(tCatch._UseForTag(pyxb.namespace.ExpandedName(Namespace, 'reply')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 72, 12))
    st_4 = fac.State(symbol, is_initial=True, final_update=final_update, is_unordered_catenation=False)
    states.append(st_4)
    final_update = set()
    symbol = pyxb.binding.content.ElementUse(tCatch._UseForTag(pyxb.namespace.ExpandedName(Namespace, 'assign')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 73, 12))
    st_5 = fac.State(symbol, is_initial=True, final_update=final_update, is_unordered_catenation=False)
    states.append(st_5)
    final_update = set()
    symbol = pyxb.binding.content.ElementUse(tCatch._UseForTag(pyxb.namespace.ExpandedName(Namespace, 'wait')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 74, 12))
    st_6 = fac.State(symbol, is_initial=True, final_update=final_update, is_unordered_catenation=False)
    states.append(st_6)
    final_update = set()
    symbol = pyxb.binding.content.ElementUse(tCatch._UseForTag(pyxb.namespace.ExpandedName(Namespace, 'throw')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 75, 12))
    st_7 = fac.State(symbol, is_initial=True, final_update=final_update, is_unordered_catenation=False)
    states.append(st_7)
    final_update = set()
    symbol = pyxb.binding.content.ElementUse(tCatch._UseForTag(pyxb.namespace.ExpandedName(Namespace, 'terminate')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 76, 12))
    st_8 = fac.State(symbol, is_initial=True, final_update=final_update, is_unordered_catenation=False)
    states.append(st_8)
    final_update = set()
    symbol = pyxb.binding.content.ElementUse(tCatch._UseForTag(pyxb.namespace.ExpandedName(Namespace, 'flow')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 77, 12))
    st_9 = fac.State(symbol, is_initial=True, final_update=final_update, is_unordered_catenation=False)
    states.append(st_9)
    final_update = set()
    symbol = pyxb.binding.content.ElementUse(tCatch._UseForTag(pyxb.namespace.ExpandedName(Namespace, 'switch')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 78, 12))
    st_10 = fac.State(symbol, is_initial=True, final_update=final_update, is_unordered_catenation=False)
    states.append(st_10)
    final_update = set()
    symbol = pyxb.binding.content.ElementUse(tCatch._UseForTag(pyxb.namespace.ExpandedName(Namespace, 'while')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 79, 12))
    st_11 = fac.State(symbol, is_initial=True, final_update=final_update, is_unordered_catenation=False)
    states.append(st_11)
    final_update = set()
    symbol = pyxb.binding.content.ElementUse(tCatch._UseForTag(pyxb.namespace.ExpandedName(Namespace, 'sequence')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 80, 12))
    st_12 = fac.State(symbol, is_initial=True, final_update=final_update, is_unordered_catenation=False)
    states.append(st_12)
    final_update = set()
    symbol = pyxb.binding.content.ElementUse(tCatch._UseForTag(pyxb.namespace.ExpandedName(Namespace, 'pick')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 81, 12))
    st_13 = fac.State(symbol, is_initial=True, final_update=final_update, is_unordered_catenation=False)
    states.append(st_13)
    final_update = set()
    symbol = pyxb.binding.content.ElementUse(tCatch._UseForTag(pyxb.namespace.ExpandedName(Namespace, 'scope')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 82, 12))
    st_14 = fac.State(symbol, is_initial=True, final_update=final_update, is_unordered_catenation=False)
    states.append(st_14)
    final_update = set()
    symbol = pyxb.binding.content.ElementUse(tCatch._UseForTag(pyxb.namespace.ExpandedName(Namespace, 'compensate')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 181, 20))
    st_15 = fac.State(symbol, is_initial=True, final_update=final_update, is_unordered_catenation=False)
    states.append(st_15)
    transitions = []
    transitions.append(fac.Transition(st_0, [
        fac.UpdateInstruction(cc_0, True) ]))
    transitions.append(fac.Transition(st_1, [
        fac.UpdateInstruction(cc_0, False) ]))
    transitions.append(fac.Transition(st_2, [
        fac.UpdateInstruction(cc_0, False) ]))
    transitions.append(fac.Transition(st_3, [
        fac.UpdateInstruction(cc_0, False) ]))
    transitions.append(fac.Transition(st_4, [
        fac.UpdateInstruction(cc_0, False) ]))
    transitions.append(fac.Transition(st_5, [
        fac.UpdateInstruction(cc_0, False) ]))
    transitions.append(fac.Transition(st_6, [
        fac.UpdateInstruction(cc_0, False) ]))
    transitions.append(fac.Transition(st_7, [
        fac.UpdateInstruction(cc_0, False) ]))
    transitions.append(fac.Transition(st_8, [
        fac.UpdateInstruction(cc_0, False) ]))
    transitions.append(fac.Transition(st_9, [
        fac.UpdateInstruction(cc_0, False) ]))
    transitions.append(fac.Transition(st_10, [
        fac.UpdateInstruction(cc_0, False) ]))
    transitions.append(fac.Transition(st_11, [
        fac.UpdateInstruction(cc_0, False) ]))
    transitions.append(fac.Transition(st_12, [
        fac.UpdateInstruction(cc_0, False) ]))
    transitions.append(fac.Transition(st_13, [
        fac.UpdateInstruction(cc_0, False) ]))
    transitions.append(fac.Transition(st_14, [
        fac.UpdateInstruction(cc_0, False) ]))
    transitions.append(fac.Transition(st_15, [
        fac.UpdateInstruction(cc_0, False) ]))
    st_0._set_transitionSet(transitions)
    transitions = []
    st_1._set_transitionSet(transitions)
    transitions = []
    st_2._set_transitionSet(transitions)
    transitions = []
    st_3._set_transitionSet(transitions)
    transitions = []
    st_4._set_transitionSet(transitions)
    transitions = []
    st_5._set_transitionSet(transitions)
    transitions = []
    st_6._set_transitionSet(transitions)
    transitions = []
    st_7._set_transitionSet(transitions)
    transitions = []
    st_8._set_transitionSet(transitions)
    transitions = []
    st_9._set_transitionSet(transitions)
    transitions = []
    st_10._set_transitionSet(transitions)
    transitions = []
    st_11._set_transitionSet(transitions)
    transitions = []
    st_12._set_transitionSet(transitions)
    transitions = []
    st_13._set_transitionSet(transitions)
    transitions = []
    st_14._set_transitionSet(transitions)
    transitions = []
    st_15._set_transitionSet(transitions)
    return fac.Automaton(states, counters, False, containing_state=None)
tCatch._Automaton = _BuildAutomaton_25()




def _BuildAutomaton_26 ():
    # Remove this helper function from the namespace after it is invoked
    global _BuildAutomaton_26
    del _BuildAutomaton_26
    import pyxb.utils.fac as fac

    counters = set()
    cc_0 = fac.CounterCondition(min=0, max=None, metadata=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 20, 12))
    counters.add(cc_0)
    states = []
    final_update = None
    symbol = pyxb.binding.content.WildcardUse(pyxb.binding.content.Wildcard(process_contents=pyxb.binding.content.Wildcard.PC_lax, namespace_constraint=(pyxb.binding.content.Wildcard.NC_not, 'http://schemas.xmlsoap.org/ws/2003/03/business-process/')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 20, 12))
    st_0 = fac.State(symbol, is_initial=True, final_update=final_update, is_unordered_catenation=False)
    states.append(st_0)
    final_update = set()
    symbol = pyxb.binding.content.ElementUse(tOnAlarm._UseForTag(pyxb.namespace.ExpandedName(Namespace, 'empty')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 69, 12))
    st_1 = fac.State(symbol, is_initial=True, final_update=final_update, is_unordered_catenation=False)
    states.append(st_1)
    final_update = set()
    symbol = pyxb.binding.content.ElementUse(tOnAlarm._UseForTag(pyxb.namespace.ExpandedName(Namespace, 'invoke')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 70, 12))
    st_2 = fac.State(symbol, is_initial=True, final_update=final_update, is_unordered_catenation=False)
    states.append(st_2)
    final_update = set()
    symbol = pyxb.binding.content.ElementUse(tOnAlarm._UseForTag(pyxb.namespace.ExpandedName(Namespace, 'receive')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 71, 12))
    st_3 = fac.State(symbol, is_initial=True, final_update=final_update, is_unordered_catenation=False)
    states.append(st_3)
    final_update = set()
    symbol = pyxb.binding.content.ElementUse(tOnAlarm._UseForTag(pyxb.namespace.ExpandedName(Namespace, 'reply')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 72, 12))
    st_4 = fac.State(symbol, is_initial=True, final_update=final_update, is_unordered_catenation=False)
    states.append(st_4)
    final_update = set()
    symbol = pyxb.binding.content.ElementUse(tOnAlarm._UseForTag(pyxb.namespace.ExpandedName(Namespace, 'assign')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 73, 12))
    st_5 = fac.State(symbol, is_initial=True, final_update=final_update, is_unordered_catenation=False)
    states.append(st_5)
    final_update = set()
    symbol = pyxb.binding.content.ElementUse(tOnAlarm._UseForTag(pyxb.namespace.ExpandedName(Namespace, 'wait')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 74, 12))
    st_6 = fac.State(symbol, is_initial=True, final_update=final_update, is_unordered_catenation=False)
    states.append(st_6)
    final_update = set()
    symbol = pyxb.binding.content.ElementUse(tOnAlarm._UseForTag(pyxb.namespace.ExpandedName(Namespace, 'throw')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 75, 12))
    st_7 = fac.State(symbol, is_initial=True, final_update=final_update, is_unordered_catenation=False)
    states.append(st_7)
    final_update = set()
    symbol = pyxb.binding.content.ElementUse(tOnAlarm._UseForTag(pyxb.namespace.ExpandedName(Namespace, 'terminate')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 76, 12))
    st_8 = fac.State(symbol, is_initial=True, final_update=final_update, is_unordered_catenation=False)
    states.append(st_8)
    final_update = set()
    symbol = pyxb.binding.content.ElementUse(tOnAlarm._UseForTag(pyxb.namespace.ExpandedName(Namespace, 'flow')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 77, 12))
    st_9 = fac.State(symbol, is_initial=True, final_update=final_update, is_unordered_catenation=False)
    states.append(st_9)
    final_update = set()
    symbol = pyxb.binding.content.ElementUse(tOnAlarm._UseForTag(pyxb.namespace.ExpandedName(Namespace, 'switch')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 78, 12))
    st_10 = fac.State(symbol, is_initial=True, final_update=final_update, is_unordered_catenation=False)
    states.append(st_10)
    final_update = set()
    symbol = pyxb.binding.content.ElementUse(tOnAlarm._UseForTag(pyxb.namespace.ExpandedName(Namespace, 'while')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 79, 12))
    st_11 = fac.State(symbol, is_initial=True, final_update=final_update, is_unordered_catenation=False)
    states.append(st_11)
    final_update = set()
    symbol = pyxb.binding.content.ElementUse(tOnAlarm._UseForTag(pyxb.namespace.ExpandedName(Namespace, 'sequence')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 80, 12))
    st_12 = fac.State(symbol, is_initial=True, final_update=final_update, is_unordered_catenation=False)
    states.append(st_12)
    final_update = set()
    symbol = pyxb.binding.content.ElementUse(tOnAlarm._UseForTag(pyxb.namespace.ExpandedName(Namespace, 'pick')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 81, 12))
    st_13 = fac.State(symbol, is_initial=True, final_update=final_update, is_unordered_catenation=False)
    states.append(st_13)
    final_update = set()
    symbol = pyxb.binding.content.ElementUse(tOnAlarm._UseForTag(pyxb.namespace.ExpandedName(Namespace, 'scope')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 82, 12))
    st_14 = fac.State(symbol, is_initial=True, final_update=final_update, is_unordered_catenation=False)
    states.append(st_14)
    transitions = []
    transitions.append(fac.Transition(st_0, [
        fac.UpdateInstruction(cc_0, True) ]))
    transitions.append(fac.Transition(st_1, [
        fac.UpdateInstruction(cc_0, False) ]))
    transitions.append(fac.Transition(st_2, [
        fac.UpdateInstruction(cc_0, False) ]))
    transitions.append(fac.Transition(st_3, [
        fac.UpdateInstruction(cc_0, False) ]))
    transitions.append(fac.Transition(st_4, [
        fac.UpdateInstruction(cc_0, False) ]))
    transitions.append(fac.Transition(st_5, [
        fac.UpdateInstruction(cc_0, False) ]))
    transitions.append(fac.Transition(st_6, [
        fac.UpdateInstruction(cc_0, False) ]))
    transitions.append(fac.Transition(st_7, [
        fac.UpdateInstruction(cc_0, False) ]))
    transitions.append(fac.Transition(st_8, [
        fac.UpdateInstruction(cc_0, False) ]))
    transitions.append(fac.Transition(st_9, [
        fac.UpdateInstruction(cc_0, False) ]))
    transitions.append(fac.Transition(st_10, [
        fac.UpdateInstruction(cc_0, False) ]))
    transitions.append(fac.Transition(st_11, [
        fac.UpdateInstruction(cc_0, False) ]))
    transitions.append(fac.Transition(st_12, [
        fac.UpdateInstruction(cc_0, False) ]))
    transitions.append(fac.Transition(st_13, [
        fac.UpdateInstruction(cc_0, False) ]))
    transitions.append(fac.Transition(st_14, [
        fac.UpdateInstruction(cc_0, False) ]))
    st_0._set_transitionSet(transitions)
    transitions = []
    st_1._set_transitionSet(transitions)
    transitions = []
    st_2._set_transitionSet(transitions)
    transitions = []
    st_3._set_transitionSet(transitions)
    transitions = []
    st_4._set_transitionSet(transitions)
    transitions = []
    st_5._set_transitionSet(transitions)
    transitions = []
    st_6._set_transitionSet(transitions)
    transitions = []
    st_7._set_transitionSet(transitions)
    transitions = []
    st_8._set_transitionSet(transitions)
    transitions = []
    st_9._set_transitionSet(transitions)
    transitions = []
    st_10._set_transitionSet(transitions)
    transitions = []
    st_11._set_transitionSet(transitions)
    transitions = []
    st_12._set_transitionSet(transitions)
    transitions = []
    st_13._set_transitionSet(transitions)
    transitions = []
    st_14._set_transitionSet(transitions)
    return fac.Automaton(states, counters, False, containing_state=None)
tOnAlarm._Automaton = _BuildAutomaton_26()




def _BuildAutomaton_27 ():
    # Remove this helper function from the namespace after it is invoked
    global _BuildAutomaton_27
    del _BuildAutomaton_27
    import pyxb.utils.fac as fac

    counters = set()
    cc_0 = fac.CounterCondition(min=0, max=None, metadata=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 20, 12))
    counters.add(cc_0)
    states = []
    final_update = None
    symbol = pyxb.binding.content.WildcardUse(pyxb.binding.content.Wildcard(process_contents=pyxb.binding.content.Wildcard.PC_lax, namespace_constraint=(pyxb.binding.content.Wildcard.NC_not, 'http://schemas.xmlsoap.org/ws/2003/03/business-process/')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 20, 12))
    st_0 = fac.State(symbol, is_initial=True, final_update=final_update, is_unordered_catenation=False)
    states.append(st_0)
    final_update = set()
    symbol = pyxb.binding.content.ElementUse(tCompensationHandler._UseForTag(pyxb.namespace.ExpandedName(Namespace, 'empty')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 69, 12))
    st_1 = fac.State(symbol, is_initial=True, final_update=final_update, is_unordered_catenation=False)
    states.append(st_1)
    final_update = set()
    symbol = pyxb.binding.content.ElementUse(tCompensationHandler._UseForTag(pyxb.namespace.ExpandedName(Namespace, 'invoke')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 70, 12))
    st_2 = fac.State(symbol, is_initial=True, final_update=final_update, is_unordered_catenation=False)
    states.append(st_2)
    final_update = set()
    symbol = pyxb.binding.content.ElementUse(tCompensationHandler._UseForTag(pyxb.namespace.ExpandedName(Namespace, 'receive')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 71, 12))
    st_3 = fac.State(symbol, is_initial=True, final_update=final_update, is_unordered_catenation=False)
    states.append(st_3)
    final_update = set()
    symbol = pyxb.binding.content.ElementUse(tCompensationHandler._UseForTag(pyxb.namespace.ExpandedName(Namespace, 'reply')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 72, 12))
    st_4 = fac.State(symbol, is_initial=True, final_update=final_update, is_unordered_catenation=False)
    states.append(st_4)
    final_update = set()
    symbol = pyxb.binding.content.ElementUse(tCompensationHandler._UseForTag(pyxb.namespace.ExpandedName(Namespace, 'assign')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 73, 12))
    st_5 = fac.State(symbol, is_initial=True, final_update=final_update, is_unordered_catenation=False)
    states.append(st_5)
    final_update = set()
    symbol = pyxb.binding.content.ElementUse(tCompensationHandler._UseForTag(pyxb.namespace.ExpandedName(Namespace, 'wait')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 74, 12))
    st_6 = fac.State(symbol, is_initial=True, final_update=final_update, is_unordered_catenation=False)
    states.append(st_6)
    final_update = set()
    symbol = pyxb.binding.content.ElementUse(tCompensationHandler._UseForTag(pyxb.namespace.ExpandedName(Namespace, 'throw')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 75, 12))
    st_7 = fac.State(symbol, is_initial=True, final_update=final_update, is_unordered_catenation=False)
    states.append(st_7)
    final_update = set()
    symbol = pyxb.binding.content.ElementUse(tCompensationHandler._UseForTag(pyxb.namespace.ExpandedName(Namespace, 'terminate')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 76, 12))
    st_8 = fac.State(symbol, is_initial=True, final_update=final_update, is_unordered_catenation=False)
    states.append(st_8)
    final_update = set()
    symbol = pyxb.binding.content.ElementUse(tCompensationHandler._UseForTag(pyxb.namespace.ExpandedName(Namespace, 'flow')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 77, 12))
    st_9 = fac.State(symbol, is_initial=True, final_update=final_update, is_unordered_catenation=False)
    states.append(st_9)
    final_update = set()
    symbol = pyxb.binding.content.ElementUse(tCompensationHandler._UseForTag(pyxb.namespace.ExpandedName(Namespace, 'switch')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 78, 12))
    st_10 = fac.State(symbol, is_initial=True, final_update=final_update, is_unordered_catenation=False)
    states.append(st_10)
    final_update = set()
    symbol = pyxb.binding.content.ElementUse(tCompensationHandler._UseForTag(pyxb.namespace.ExpandedName(Namespace, 'while')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 79, 12))
    st_11 = fac.State(symbol, is_initial=True, final_update=final_update, is_unordered_catenation=False)
    states.append(st_11)
    final_update = set()
    symbol = pyxb.binding.content.ElementUse(tCompensationHandler._UseForTag(pyxb.namespace.ExpandedName(Namespace, 'sequence')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 80, 12))
    st_12 = fac.State(symbol, is_initial=True, final_update=final_update, is_unordered_catenation=False)
    states.append(st_12)
    final_update = set()
    symbol = pyxb.binding.content.ElementUse(tCompensationHandler._UseForTag(pyxb.namespace.ExpandedName(Namespace, 'pick')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 81, 12))
    st_13 = fac.State(symbol, is_initial=True, final_update=final_update, is_unordered_catenation=False)
    states.append(st_13)
    final_update = set()
    symbol = pyxb.binding.content.ElementUse(tCompensationHandler._UseForTag(pyxb.namespace.ExpandedName(Namespace, 'scope')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 82, 12))
    st_14 = fac.State(symbol, is_initial=True, final_update=final_update, is_unordered_catenation=False)
    states.append(st_14)
    final_update = set()
    symbol = pyxb.binding.content.ElementUse(tCompensationHandler._UseForTag(pyxb.namespace.ExpandedName(Namespace, 'compensate')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 181, 20))
    st_15 = fac.State(symbol, is_initial=True, final_update=final_update, is_unordered_catenation=False)
    states.append(st_15)
    transitions = []
    transitions.append(fac.Transition(st_0, [
        fac.UpdateInstruction(cc_0, True) ]))
    transitions.append(fac.Transition(st_1, [
        fac.UpdateInstruction(cc_0, False) ]))
    transitions.append(fac.Transition(st_2, [
        fac.UpdateInstruction(cc_0, False) ]))
    transitions.append(fac.Transition(st_3, [
        fac.UpdateInstruction(cc_0, False) ]))
    transitions.append(fac.Transition(st_4, [
        fac.UpdateInstruction(cc_0, False) ]))
    transitions.append(fac.Transition(st_5, [
        fac.UpdateInstruction(cc_0, False) ]))
    transitions.append(fac.Transition(st_6, [
        fac.UpdateInstruction(cc_0, False) ]))
    transitions.append(fac.Transition(st_7, [
        fac.UpdateInstruction(cc_0, False) ]))
    transitions.append(fac.Transition(st_8, [
        fac.UpdateInstruction(cc_0, False) ]))
    transitions.append(fac.Transition(st_9, [
        fac.UpdateInstruction(cc_0, False) ]))
    transitions.append(fac.Transition(st_10, [
        fac.UpdateInstruction(cc_0, False) ]))
    transitions.append(fac.Transition(st_11, [
        fac.UpdateInstruction(cc_0, False) ]))
    transitions.append(fac.Transition(st_12, [
        fac.UpdateInstruction(cc_0, False) ]))
    transitions.append(fac.Transition(st_13, [
        fac.UpdateInstruction(cc_0, False) ]))
    transitions.append(fac.Transition(st_14, [
        fac.UpdateInstruction(cc_0, False) ]))
    transitions.append(fac.Transition(st_15, [
        fac.UpdateInstruction(cc_0, False) ]))
    st_0._set_transitionSet(transitions)
    transitions = []
    st_1._set_transitionSet(transitions)
    transitions = []
    st_2._set_transitionSet(transitions)
    transitions = []
    st_3._set_transitionSet(transitions)
    transitions = []
    st_4._set_transitionSet(transitions)
    transitions = []
    st_5._set_transitionSet(transitions)
    transitions = []
    st_6._set_transitionSet(transitions)
    transitions = []
    st_7._set_transitionSet(transitions)
    transitions = []
    st_8._set_transitionSet(transitions)
    transitions = []
    st_9._set_transitionSet(transitions)
    transitions = []
    st_10._set_transitionSet(transitions)
    transitions = []
    st_11._set_transitionSet(transitions)
    transitions = []
    st_12._set_transitionSet(transitions)
    transitions = []
    st_13._set_transitionSet(transitions)
    transitions = []
    st_14._set_transitionSet(transitions)
    transitions = []
    st_15._set_transitionSet(transitions)
    return fac.Automaton(states, counters, False, containing_state=None)
tCompensationHandler._Automaton = _BuildAutomaton_27()




def _BuildAutomaton_28 ():
    # Remove this helper function from the namespace after it is invoked
    global _BuildAutomaton_28
    del _BuildAutomaton_28
    import pyxb.utils.fac as fac

    counters = set()
    cc_0 = fac.CounterCondition(min=0, max=None, metadata=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 20, 12))
    counters.add(cc_0)
    cc_1 = fac.CounterCondition(min=0, max=None, metadata=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 290, 20))
    counters.add(cc_1)
    cc_2 = fac.CounterCondition(min=0, max=None, metadata=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 292, 20))
    counters.add(cc_2)
    states = []
    final_update = set()
    final_update.add(fac.UpdateInstruction(cc_0, False))
    symbol = pyxb.binding.content.WildcardUse(pyxb.binding.content.Wildcard(process_contents=pyxb.binding.content.Wildcard.PC_lax, namespace_constraint=(pyxb.binding.content.Wildcard.NC_not, 'http://schemas.xmlsoap.org/ws/2003/03/business-process/')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 20, 12))
    st_0 = fac.State(symbol, is_initial=True, final_update=final_update, is_unordered_catenation=False)
    states.append(st_0)
    final_update = set()
    final_update.add(fac.UpdateInstruction(cc_1, False))
    symbol = pyxb.binding.content.ElementUse(tEmpty._UseForTag(pyxb.namespace.ExpandedName(Namespace, 'target')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 290, 20))
    st_1 = fac.State(symbol, is_initial=True, final_update=final_update, is_unordered_catenation=False)
    states.append(st_1)
    final_update = set()
    final_update.add(fac.UpdateInstruction(cc_2, False))
    symbol = pyxb.binding.content.ElementUse(tEmpty._UseForTag(pyxb.namespace.ExpandedName(Namespace, 'source')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 292, 20))
    st_2 = fac.State(symbol, is_initial=True, final_update=final_update, is_unordered_catenation=False)
    states.append(st_2)
    transitions = []
    transitions.append(fac.Transition(st_0, [
        fac.UpdateInstruction(cc_0, True) ]))
    transitions.append(fac.Transition(st_1, [
        fac.UpdateInstruction(cc_0, False) ]))
    transitions.append(fac.Transition(st_2, [
        fac.UpdateInstruction(cc_0, False) ]))
    st_0._set_transitionSet(transitions)
    transitions = []
    transitions.append(fac.Transition(st_1, [
        fac.UpdateInstruction(cc_1, True) ]))
    transitions.append(fac.Transition(st_2, [
        fac.UpdateInstruction(cc_1, False) ]))
    st_1._set_transitionSet(transitions)
    transitions = []
    transitions.append(fac.Transition(st_2, [
        fac.UpdateInstruction(cc_2, True) ]))
    st_2._set_transitionSet(transitions)
    return fac.Automaton(states, counters, True, containing_state=None)
tEmpty._Automaton = _BuildAutomaton_28()




def _BuildAutomaton_29 ():
    # Remove this helper function from the namespace after it is invoked
    global _BuildAutomaton_29
    del _BuildAutomaton_29
    import pyxb.utils.fac as fac

    counters = set()
    cc_0 = fac.CounterCondition(min=0, max=None, metadata=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 20, 12))
    counters.add(cc_0)
    states = []
    final_update = set()
    final_update.add(fac.UpdateInstruction(cc_0, False))
    symbol = pyxb.binding.content.WildcardUse(pyxb.binding.content.Wildcard(process_contents=pyxb.binding.content.Wildcard.PC_lax, namespace_constraint=(pyxb.binding.content.Wildcard.NC_not, 'http://schemas.xmlsoap.org/ws/2003/03/business-process/')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 20, 12))
    st_0 = fac.State(symbol, is_initial=True, final_update=final_update, is_unordered_catenation=False)
    states.append(st_0)
    transitions = []
    transitions.append(fac.Transition(st_0, [
        fac.UpdateInstruction(cc_0, True) ]))
    st_0._set_transitionSet(transitions)
    return fac.Automaton(states, counters, True, containing_state=None)
tCorrelationWithPattern._Automaton = _BuildAutomaton_29()




tInvoke._AddElement(pyxb.binding.basis.element(pyxb.namespace.ExpandedName(Namespace, 'correlations'), tCorrelationsWithPattern, scope=tInvoke, location=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 388, 20)))

tInvoke._AddElement(pyxb.binding.basis.element(pyxb.namespace.ExpandedName(Namespace, 'catch'), tCatch, scope=tInvoke, location=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 391, 20)))

tInvoke._AddElement(pyxb.binding.basis.element(pyxb.namespace.ExpandedName(Namespace, 'catchAll'), tActivityOrCompensateContainer, scope=tInvoke, location=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 393, 20)))

tInvoke._AddElement(pyxb.binding.basis.element(pyxb.namespace.ExpandedName(Namespace, 'compensationHandler'), tCompensationHandler, scope=tInvoke, location=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 396, 20)))

def _BuildAutomaton_30 ():
    # Remove this helper function from the namespace after it is invoked
    global _BuildAutomaton_30
    del _BuildAutomaton_30
    import pyxb.utils.fac as fac

    counters = set()
    cc_0 = fac.CounterCondition(min=0, max=None, metadata=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 20, 12))
    counters.add(cc_0)
    cc_1 = fac.CounterCondition(min=0, max=None, metadata=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 290, 20))
    counters.add(cc_1)
    cc_2 = fac.CounterCondition(min=0, max=None, metadata=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 292, 20))
    counters.add(cc_2)
    cc_3 = fac.CounterCondition(min=0, max=1, metadata=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 388, 20))
    counters.add(cc_3)
    cc_4 = fac.CounterCondition(min=0, max=None, metadata=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 391, 20))
    counters.add(cc_4)
    cc_5 = fac.CounterCondition(min=0, max=1, metadata=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 393, 20))
    counters.add(cc_5)
    cc_6 = fac.CounterCondition(min=0, max=1, metadata=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 396, 20))
    counters.add(cc_6)
    states = []
    final_update = set()
    final_update.add(fac.UpdateInstruction(cc_0, False))
    symbol = pyxb.binding.content.WildcardUse(pyxb.binding.content.Wildcard(process_contents=pyxb.binding.content.Wildcard.PC_lax, namespace_constraint=(pyxb.binding.content.Wildcard.NC_not, 'http://schemas.xmlsoap.org/ws/2003/03/business-process/')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 20, 12))
    st_0 = fac.State(symbol, is_initial=True, final_update=final_update, is_unordered_catenation=False)
    states.append(st_0)
    final_update = set()
    final_update.add(fac.UpdateInstruction(cc_1, False))
    symbol = pyxb.binding.content.ElementUse(tInvoke._UseForTag(pyxb.namespace.ExpandedName(Namespace, 'target')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 290, 20))
    st_1 = fac.State(symbol, is_initial=True, final_update=final_update, is_unordered_catenation=False)
    states.append(st_1)
    final_update = set()
    final_update.add(fac.UpdateInstruction(cc_2, False))
    symbol = pyxb.binding.content.ElementUse(tInvoke._UseForTag(pyxb.namespace.ExpandedName(Namespace, 'source')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 292, 20))
    st_2 = fac.State(symbol, is_initial=True, final_update=final_update, is_unordered_catenation=False)
    states.append(st_2)
    final_update = set()
    final_update.add(fac.UpdateInstruction(cc_3, False))
    symbol = pyxb.binding.content.ElementUse(tInvoke._UseForTag(pyxb.namespace.ExpandedName(Namespace, 'correlations')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 388, 20))
    st_3 = fac.State(symbol, is_initial=True, final_update=final_update, is_unordered_catenation=False)
    states.append(st_3)
    final_update = set()
    final_update.add(fac.UpdateInstruction(cc_4, False))
    symbol = pyxb.binding.content.ElementUse(tInvoke._UseForTag(pyxb.namespace.ExpandedName(Namespace, 'catch')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 391, 20))
    st_4 = fac.State(symbol, is_initial=True, final_update=final_update, is_unordered_catenation=False)
    states.append(st_4)
    final_update = set()
    final_update.add(fac.UpdateInstruction(cc_5, False))
    symbol = pyxb.binding.content.ElementUse(tInvoke._UseForTag(pyxb.namespace.ExpandedName(Namespace, 'catchAll')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 393, 20))
    st_5 = fac.State(symbol, is_initial=True, final_update=final_update, is_unordered_catenation=False)
    states.append(st_5)
    final_update = set()
    final_update.add(fac.UpdateInstruction(cc_6, False))
    symbol = pyxb.binding.content.ElementUse(tInvoke._UseForTag(pyxb.namespace.ExpandedName(Namespace, 'compensationHandler')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 396, 20))
    st_6 = fac.State(symbol, is_initial=True, final_update=final_update, is_unordered_catenation=False)
    states.append(st_6)
    transitions = []
    transitions.append(fac.Transition(st_0, [
        fac.UpdateInstruction(cc_0, True) ]))
    transitions.append(fac.Transition(st_1, [
        fac.UpdateInstruction(cc_0, False) ]))
    transitions.append(fac.Transition(st_2, [
        fac.UpdateInstruction(cc_0, False) ]))
    transitions.append(fac.Transition(st_3, [
        fac.UpdateInstruction(cc_0, False) ]))
    transitions.append(fac.Transition(st_4, [
        fac.UpdateInstruction(cc_0, False) ]))
    transitions.append(fac.Transition(st_5, [
        fac.UpdateInstruction(cc_0, False) ]))
    transitions.append(fac.Transition(st_6, [
        fac.UpdateInstruction(cc_0, False) ]))
    st_0._set_transitionSet(transitions)
    transitions = []
    transitions.append(fac.Transition(st_1, [
        fac.UpdateInstruction(cc_1, True) ]))
    transitions.append(fac.Transition(st_2, [
        fac.UpdateInstruction(cc_1, False) ]))
    transitions.append(fac.Transition(st_3, [
        fac.UpdateInstruction(cc_1, False) ]))
    transitions.append(fac.Transition(st_4, [
        fac.UpdateInstruction(cc_1, False) ]))
    transitions.append(fac.Transition(st_5, [
        fac.UpdateInstruction(cc_1, False) ]))
    transitions.append(fac.Transition(st_6, [
        fac.UpdateInstruction(cc_1, False) ]))
    st_1._set_transitionSet(transitions)
    transitions = []
    transitions.append(fac.Transition(st_2, [
        fac.UpdateInstruction(cc_2, True) ]))
    transitions.append(fac.Transition(st_3, [
        fac.UpdateInstruction(cc_2, False) ]))
    transitions.append(fac.Transition(st_4, [
        fac.UpdateInstruction(cc_2, False) ]))
    transitions.append(fac.Transition(st_5, [
        fac.UpdateInstruction(cc_2, False) ]))
    transitions.append(fac.Transition(st_6, [
        fac.UpdateInstruction(cc_2, False) ]))
    st_2._set_transitionSet(transitions)
    transitions = []
    transitions.append(fac.Transition(st_3, [
        fac.UpdateInstruction(cc_3, True) ]))
    transitions.append(fac.Transition(st_4, [
        fac.UpdateInstruction(cc_3, False) ]))
    transitions.append(fac.Transition(st_5, [
        fac.UpdateInstruction(cc_3, False) ]))
    transitions.append(fac.Transition(st_6, [
        fac.UpdateInstruction(cc_3, False) ]))
    st_3._set_transitionSet(transitions)
    transitions = []
    transitions.append(fac.Transition(st_4, [
        fac.UpdateInstruction(cc_4, True) ]))
    transitions.append(fac.Transition(st_5, [
        fac.UpdateInstruction(cc_4, False) ]))
    transitions.append(fac.Transition(st_6, [
        fac.UpdateInstruction(cc_4, False) ]))
    st_4._set_transitionSet(transitions)
    transitions = []
    transitions.append(fac.Transition(st_5, [
        fac.UpdateInstruction(cc_5, True) ]))
    transitions.append(fac.Transition(st_6, [
        fac.UpdateInstruction(cc_5, False) ]))
    st_5._set_transitionSet(transitions)
    transitions = []
    transitions.append(fac.Transition(st_6, [
        fac.UpdateInstruction(cc_6, True) ]))
    st_6._set_transitionSet(transitions)
    return fac.Automaton(states, counters, True, containing_state=None)
tInvoke._Automaton = _BuildAutomaton_30()




tReceive._AddElement(pyxb.binding.basis.element(pyxb.namespace.ExpandedName(Namespace, 'correlations'), tCorrelations, scope=tReceive, location=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 414, 20)))

def _BuildAutomaton_31 ():
    # Remove this helper function from the namespace after it is invoked
    global _BuildAutomaton_31
    del _BuildAutomaton_31
    import pyxb.utils.fac as fac

    counters = set()
    cc_0 = fac.CounterCondition(min=0, max=None, metadata=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 20, 12))
    counters.add(cc_0)
    cc_1 = fac.CounterCondition(min=0, max=None, metadata=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 290, 20))
    counters.add(cc_1)
    cc_2 = fac.CounterCondition(min=0, max=None, metadata=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 292, 20))
    counters.add(cc_2)
    cc_3 = fac.CounterCondition(min=0, max=1, metadata=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 414, 20))
    counters.add(cc_3)
    states = []
    final_update = set()
    final_update.add(fac.UpdateInstruction(cc_0, False))
    symbol = pyxb.binding.content.WildcardUse(pyxb.binding.content.Wildcard(process_contents=pyxb.binding.content.Wildcard.PC_lax, namespace_constraint=(pyxb.binding.content.Wildcard.NC_not, 'http://schemas.xmlsoap.org/ws/2003/03/business-process/')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 20, 12))
    st_0 = fac.State(symbol, is_initial=True, final_update=final_update, is_unordered_catenation=False)
    states.append(st_0)
    final_update = set()
    final_update.add(fac.UpdateInstruction(cc_1, False))
    symbol = pyxb.binding.content.ElementUse(tReceive._UseForTag(pyxb.namespace.ExpandedName(Namespace, 'target')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 290, 20))
    st_1 = fac.State(symbol, is_initial=True, final_update=final_update, is_unordered_catenation=False)
    states.append(st_1)
    final_update = set()
    final_update.add(fac.UpdateInstruction(cc_2, False))
    symbol = pyxb.binding.content.ElementUse(tReceive._UseForTag(pyxb.namespace.ExpandedName(Namespace, 'source')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 292, 20))
    st_2 = fac.State(symbol, is_initial=True, final_update=final_update, is_unordered_catenation=False)
    states.append(st_2)
    final_update = set()
    final_update.add(fac.UpdateInstruction(cc_3, False))
    symbol = pyxb.binding.content.ElementUse(tReceive._UseForTag(pyxb.namespace.ExpandedName(Namespace, 'correlations')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 414, 20))
    st_3 = fac.State(symbol, is_initial=True, final_update=final_update, is_unordered_catenation=False)
    states.append(st_3)
    transitions = []
    transitions.append(fac.Transition(st_0, [
        fac.UpdateInstruction(cc_0, True) ]))
    transitions.append(fac.Transition(st_1, [
        fac.UpdateInstruction(cc_0, False) ]))
    transitions.append(fac.Transition(st_2, [
        fac.UpdateInstruction(cc_0, False) ]))
    transitions.append(fac.Transition(st_3, [
        fac.UpdateInstruction(cc_0, False) ]))
    st_0._set_transitionSet(transitions)
    transitions = []
    transitions.append(fac.Transition(st_1, [
        fac.UpdateInstruction(cc_1, True) ]))
    transitions.append(fac.Transition(st_2, [
        fac.UpdateInstruction(cc_1, False) ]))
    transitions.append(fac.Transition(st_3, [
        fac.UpdateInstruction(cc_1, False) ]))
    st_1._set_transitionSet(transitions)
    transitions = []
    transitions.append(fac.Transition(st_2, [
        fac.UpdateInstruction(cc_2, True) ]))
    transitions.append(fac.Transition(st_3, [
        fac.UpdateInstruction(cc_2, False) ]))
    st_2._set_transitionSet(transitions)
    transitions = []
    transitions.append(fac.Transition(st_3, [
        fac.UpdateInstruction(cc_3, True) ]))
    st_3._set_transitionSet(transitions)
    return fac.Automaton(states, counters, True, containing_state=None)
tReceive._Automaton = _BuildAutomaton_31()




tReply._AddElement(pyxb.binding.basis.element(pyxb.namespace.ExpandedName(Namespace, 'correlations'), tCorrelations, scope=tReply, location=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 431, 20)))

def _BuildAutomaton_32 ():
    # Remove this helper function from the namespace after it is invoked
    global _BuildAutomaton_32
    del _BuildAutomaton_32
    import pyxb.utils.fac as fac

    counters = set()
    cc_0 = fac.CounterCondition(min=0, max=None, metadata=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 20, 12))
    counters.add(cc_0)
    cc_1 = fac.CounterCondition(min=0, max=None, metadata=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 290, 20))
    counters.add(cc_1)
    cc_2 = fac.CounterCondition(min=0, max=None, metadata=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 292, 20))
    counters.add(cc_2)
    cc_3 = fac.CounterCondition(min=0, max=1, metadata=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 431, 20))
    counters.add(cc_3)
    states = []
    final_update = set()
    final_update.add(fac.UpdateInstruction(cc_0, False))
    symbol = pyxb.binding.content.WildcardUse(pyxb.binding.content.Wildcard(process_contents=pyxb.binding.content.Wildcard.PC_lax, namespace_constraint=(pyxb.binding.content.Wildcard.NC_not, 'http://schemas.xmlsoap.org/ws/2003/03/business-process/')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 20, 12))
    st_0 = fac.State(symbol, is_initial=True, final_update=final_update, is_unordered_catenation=False)
    states.append(st_0)
    final_update = set()
    final_update.add(fac.UpdateInstruction(cc_1, False))
    symbol = pyxb.binding.content.ElementUse(tReply._UseForTag(pyxb.namespace.ExpandedName(Namespace, 'target')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 290, 20))
    st_1 = fac.State(symbol, is_initial=True, final_update=final_update, is_unordered_catenation=False)
    states.append(st_1)
    final_update = set()
    final_update.add(fac.UpdateInstruction(cc_2, False))
    symbol = pyxb.binding.content.ElementUse(tReply._UseForTag(pyxb.namespace.ExpandedName(Namespace, 'source')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 292, 20))
    st_2 = fac.State(symbol, is_initial=True, final_update=final_update, is_unordered_catenation=False)
    states.append(st_2)
    final_update = set()
    final_update.add(fac.UpdateInstruction(cc_3, False))
    symbol = pyxb.binding.content.ElementUse(tReply._UseForTag(pyxb.namespace.ExpandedName(Namespace, 'correlations')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 431, 20))
    st_3 = fac.State(symbol, is_initial=True, final_update=final_update, is_unordered_catenation=False)
    states.append(st_3)
    transitions = []
    transitions.append(fac.Transition(st_0, [
        fac.UpdateInstruction(cc_0, True) ]))
    transitions.append(fac.Transition(st_1, [
        fac.UpdateInstruction(cc_0, False) ]))
    transitions.append(fac.Transition(st_2, [
        fac.UpdateInstruction(cc_0, False) ]))
    transitions.append(fac.Transition(st_3, [
        fac.UpdateInstruction(cc_0, False) ]))
    st_0._set_transitionSet(transitions)
    transitions = []
    transitions.append(fac.Transition(st_1, [
        fac.UpdateInstruction(cc_1, True) ]))
    transitions.append(fac.Transition(st_2, [
        fac.UpdateInstruction(cc_1, False) ]))
    transitions.append(fac.Transition(st_3, [
        fac.UpdateInstruction(cc_1, False) ]))
    st_1._set_transitionSet(transitions)
    transitions = []
    transitions.append(fac.Transition(st_2, [
        fac.UpdateInstruction(cc_2, True) ]))
    transitions.append(fac.Transition(st_3, [
        fac.UpdateInstruction(cc_2, False) ]))
    st_2._set_transitionSet(transitions)
    transitions = []
    transitions.append(fac.Transition(st_3, [
        fac.UpdateInstruction(cc_3, True) ]))
    st_3._set_transitionSet(transitions)
    return fac.Automaton(states, counters, True, containing_state=None)
tReply._Automaton = _BuildAutomaton_32()




tAssign._AddElement(pyxb.binding.basis.element(pyxb.namespace.ExpandedName(Namespace, 'copy'), tCopy, scope=tAssign, location=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 448, 20)))

def _BuildAutomaton_33 ():
    # Remove this helper function from the namespace after it is invoked
    global _BuildAutomaton_33
    del _BuildAutomaton_33
    import pyxb.utils.fac as fac

    counters = set()
    cc_0 = fac.CounterCondition(min=0, max=None, metadata=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 20, 12))
    counters.add(cc_0)
    cc_1 = fac.CounterCondition(min=0, max=None, metadata=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 290, 20))
    counters.add(cc_1)
    cc_2 = fac.CounterCondition(min=0, max=None, metadata=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 292, 20))
    counters.add(cc_2)
    states = []
    final_update = None
    symbol = pyxb.binding.content.WildcardUse(pyxb.binding.content.Wildcard(process_contents=pyxb.binding.content.Wildcard.PC_lax, namespace_constraint=(pyxb.binding.content.Wildcard.NC_not, 'http://schemas.xmlsoap.org/ws/2003/03/business-process/')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 20, 12))
    st_0 = fac.State(symbol, is_initial=True, final_update=final_update, is_unordered_catenation=False)
    states.append(st_0)
    final_update = None
    symbol = pyxb.binding.content.ElementUse(tAssign._UseForTag(pyxb.namespace.ExpandedName(Namespace, 'target')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 290, 20))
    st_1 = fac.State(symbol, is_initial=True, final_update=final_update, is_unordered_catenation=False)
    states.append(st_1)
    final_update = None
    symbol = pyxb.binding.content.ElementUse(tAssign._UseForTag(pyxb.namespace.ExpandedName(Namespace, 'source')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 292, 20))
    st_2 = fac.State(symbol, is_initial=True, final_update=final_update, is_unordered_catenation=False)
    states.append(st_2)
    final_update = set()
    symbol = pyxb.binding.content.ElementUse(tAssign._UseForTag(pyxb.namespace.ExpandedName(Namespace, 'copy')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 448, 20))
    st_3 = fac.State(symbol, is_initial=True, final_update=final_update, is_unordered_catenation=False)
    states.append(st_3)
    transitions = []
    transitions.append(fac.Transition(st_0, [
        fac.UpdateInstruction(cc_0, True) ]))
    transitions.append(fac.Transition(st_1, [
        fac.UpdateInstruction(cc_0, False) ]))
    transitions.append(fac.Transition(st_2, [
        fac.UpdateInstruction(cc_0, False) ]))
    transitions.append(fac.Transition(st_3, [
        fac.UpdateInstruction(cc_0, False) ]))
    st_0._set_transitionSet(transitions)
    transitions = []
    transitions.append(fac.Transition(st_1, [
        fac.UpdateInstruction(cc_1, True) ]))
    transitions.append(fac.Transition(st_2, [
        fac.UpdateInstruction(cc_1, False) ]))
    transitions.append(fac.Transition(st_3, [
        fac.UpdateInstruction(cc_1, False) ]))
    st_1._set_transitionSet(transitions)
    transitions = []
    transitions.append(fac.Transition(st_2, [
        fac.UpdateInstruction(cc_2, True) ]))
    transitions.append(fac.Transition(st_3, [
        fac.UpdateInstruction(cc_2, False) ]))
    st_2._set_transitionSet(transitions)
    transitions = []
    transitions.append(fac.Transition(st_3, [
         ]))
    st_3._set_transitionSet(transitions)
    return fac.Automaton(states, counters, False, containing_state=None)
tAssign._Automaton = _BuildAutomaton_33()




def _BuildAutomaton_34 ():
    # Remove this helper function from the namespace after it is invoked
    global _BuildAutomaton_34
    del _BuildAutomaton_34
    import pyxb.utils.fac as fac

    counters = set()
    cc_0 = fac.CounterCondition(min=0, max=None, metadata=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 20, 12))
    counters.add(cc_0)
    cc_1 = fac.CounterCondition(min=0, max=None, metadata=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 290, 20))
    counters.add(cc_1)
    cc_2 = fac.CounterCondition(min=0, max=None, metadata=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 292, 20))
    counters.add(cc_2)
    states = []
    final_update = set()
    final_update.add(fac.UpdateInstruction(cc_0, False))
    symbol = pyxb.binding.content.WildcardUse(pyxb.binding.content.Wildcard(process_contents=pyxb.binding.content.Wildcard.PC_lax, namespace_constraint=(pyxb.binding.content.Wildcard.NC_not, 'http://schemas.xmlsoap.org/ws/2003/03/business-process/')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 20, 12))
    st_0 = fac.State(symbol, is_initial=True, final_update=final_update, is_unordered_catenation=False)
    states.append(st_0)
    final_update = set()
    final_update.add(fac.UpdateInstruction(cc_1, False))
    symbol = pyxb.binding.content.ElementUse(tWait._UseForTag(pyxb.namespace.ExpandedName(Namespace, 'target')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 290, 20))
    st_1 = fac.State(symbol, is_initial=True, final_update=final_update, is_unordered_catenation=False)
    states.append(st_1)
    final_update = set()
    final_update.add(fac.UpdateInstruction(cc_2, False))
    symbol = pyxb.binding.content.ElementUse(tWait._UseForTag(pyxb.namespace.ExpandedName(Namespace, 'source')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 292, 20))
    st_2 = fac.State(symbol, is_initial=True, final_update=final_update, is_unordered_catenation=False)
    states.append(st_2)
    transitions = []
    transitions.append(fac.Transition(st_0, [
        fac.UpdateInstruction(cc_0, True) ]))
    transitions.append(fac.Transition(st_1, [
        fac.UpdateInstruction(cc_0, False) ]))
    transitions.append(fac.Transition(st_2, [
        fac.UpdateInstruction(cc_0, False) ]))
    st_0._set_transitionSet(transitions)
    transitions = []
    transitions.append(fac.Transition(st_1, [
        fac.UpdateInstruction(cc_1, True) ]))
    transitions.append(fac.Transition(st_2, [
        fac.UpdateInstruction(cc_1, False) ]))
    st_1._set_transitionSet(transitions)
    transitions = []
    transitions.append(fac.Transition(st_2, [
        fac.UpdateInstruction(cc_2, True) ]))
    st_2._set_transitionSet(transitions)
    return fac.Automaton(states, counters, True, containing_state=None)
tWait._Automaton = _BuildAutomaton_34()




def _BuildAutomaton_35 ():
    # Remove this helper function from the namespace after it is invoked
    global _BuildAutomaton_35
    del _BuildAutomaton_35
    import pyxb.utils.fac as fac

    counters = set()
    cc_0 = fac.CounterCondition(min=0, max=None, metadata=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 20, 12))
    counters.add(cc_0)
    cc_1 = fac.CounterCondition(min=0, max=None, metadata=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 290, 20))
    counters.add(cc_1)
    cc_2 = fac.CounterCondition(min=0, max=None, metadata=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 292, 20))
    counters.add(cc_2)
    states = []
    final_update = set()
    final_update.add(fac.UpdateInstruction(cc_0, False))
    symbol = pyxb.binding.content.WildcardUse(pyxb.binding.content.Wildcard(process_contents=pyxb.binding.content.Wildcard.PC_lax, namespace_constraint=(pyxb.binding.content.Wildcard.NC_not, 'http://schemas.xmlsoap.org/ws/2003/03/business-process/')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 20, 12))
    st_0 = fac.State(symbol, is_initial=True, final_update=final_update, is_unordered_catenation=False)
    states.append(st_0)
    final_update = set()
    final_update.add(fac.UpdateInstruction(cc_1, False))
    symbol = pyxb.binding.content.ElementUse(tThrow._UseForTag(pyxb.namespace.ExpandedName(Namespace, 'target')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 290, 20))
    st_1 = fac.State(symbol, is_initial=True, final_update=final_update, is_unordered_catenation=False)
    states.append(st_1)
    final_update = set()
    final_update.add(fac.UpdateInstruction(cc_2, False))
    symbol = pyxb.binding.content.ElementUse(tThrow._UseForTag(pyxb.namespace.ExpandedName(Namespace, 'source')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 292, 20))
    st_2 = fac.State(symbol, is_initial=True, final_update=final_update, is_unordered_catenation=False)
    states.append(st_2)
    transitions = []
    transitions.append(fac.Transition(st_0, [
        fac.UpdateInstruction(cc_0, True) ]))
    transitions.append(fac.Transition(st_1, [
        fac.UpdateInstruction(cc_0, False) ]))
    transitions.append(fac.Transition(st_2, [
        fac.UpdateInstruction(cc_0, False) ]))
    st_0._set_transitionSet(transitions)
    transitions = []
    transitions.append(fac.Transition(st_1, [
        fac.UpdateInstruction(cc_1, True) ]))
    transitions.append(fac.Transition(st_2, [
        fac.UpdateInstruction(cc_1, False) ]))
    st_1._set_transitionSet(transitions)
    transitions = []
    transitions.append(fac.Transition(st_2, [
        fac.UpdateInstruction(cc_2, True) ]))
    st_2._set_transitionSet(transitions)
    return fac.Automaton(states, counters, True, containing_state=None)
tThrow._Automaton = _BuildAutomaton_35()




def _BuildAutomaton_36 ():
    # Remove this helper function from the namespace after it is invoked
    global _BuildAutomaton_36
    del _BuildAutomaton_36
    import pyxb.utils.fac as fac

    counters = set()
    cc_0 = fac.CounterCondition(min=0, max=None, metadata=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 20, 12))
    counters.add(cc_0)
    cc_1 = fac.CounterCondition(min=0, max=None, metadata=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 290, 20))
    counters.add(cc_1)
    cc_2 = fac.CounterCondition(min=0, max=None, metadata=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 292, 20))
    counters.add(cc_2)
    states = []
    final_update = set()
    final_update.add(fac.UpdateInstruction(cc_0, False))
    symbol = pyxb.binding.content.WildcardUse(pyxb.binding.content.Wildcard(process_contents=pyxb.binding.content.Wildcard.PC_lax, namespace_constraint=(pyxb.binding.content.Wildcard.NC_not, 'http://schemas.xmlsoap.org/ws/2003/03/business-process/')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 20, 12))
    st_0 = fac.State(symbol, is_initial=True, final_update=final_update, is_unordered_catenation=False)
    states.append(st_0)
    final_update = set()
    final_update.add(fac.UpdateInstruction(cc_1, False))
    symbol = pyxb.binding.content.ElementUse(tCompensate._UseForTag(pyxb.namespace.ExpandedName(Namespace, 'target')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 290, 20))
    st_1 = fac.State(symbol, is_initial=True, final_update=final_update, is_unordered_catenation=False)
    states.append(st_1)
    final_update = set()
    final_update.add(fac.UpdateInstruction(cc_2, False))
    symbol = pyxb.binding.content.ElementUse(tCompensate._UseForTag(pyxb.namespace.ExpandedName(Namespace, 'source')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 292, 20))
    st_2 = fac.State(symbol, is_initial=True, final_update=final_update, is_unordered_catenation=False)
    states.append(st_2)
    transitions = []
    transitions.append(fac.Transition(st_0, [
        fac.UpdateInstruction(cc_0, True) ]))
    transitions.append(fac.Transition(st_1, [
        fac.UpdateInstruction(cc_0, False) ]))
    transitions.append(fac.Transition(st_2, [
        fac.UpdateInstruction(cc_0, False) ]))
    st_0._set_transitionSet(transitions)
    transitions = []
    transitions.append(fac.Transition(st_1, [
        fac.UpdateInstruction(cc_1, True) ]))
    transitions.append(fac.Transition(st_2, [
        fac.UpdateInstruction(cc_1, False) ]))
    st_1._set_transitionSet(transitions)
    transitions = []
    transitions.append(fac.Transition(st_2, [
        fac.UpdateInstruction(cc_2, True) ]))
    st_2._set_transitionSet(transitions)
    return fac.Automaton(states, counters, True, containing_state=None)
tCompensate._Automaton = _BuildAutomaton_36()




def _BuildAutomaton_37 ():
    # Remove this helper function from the namespace after it is invoked
    global _BuildAutomaton_37
    del _BuildAutomaton_37
    import pyxb.utils.fac as fac

    counters = set()
    cc_0 = fac.CounterCondition(min=0, max=None, metadata=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 20, 12))
    counters.add(cc_0)
    cc_1 = fac.CounterCondition(min=0, max=None, metadata=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 290, 20))
    counters.add(cc_1)
    cc_2 = fac.CounterCondition(min=0, max=None, metadata=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 292, 20))
    counters.add(cc_2)
    states = []
    final_update = set()
    final_update.add(fac.UpdateInstruction(cc_0, False))
    symbol = pyxb.binding.content.WildcardUse(pyxb.binding.content.Wildcard(process_contents=pyxb.binding.content.Wildcard.PC_lax, namespace_constraint=(pyxb.binding.content.Wildcard.NC_not, 'http://schemas.xmlsoap.org/ws/2003/03/business-process/')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 20, 12))
    st_0 = fac.State(symbol, is_initial=True, final_update=final_update, is_unordered_catenation=False)
    states.append(st_0)
    final_update = set()
    final_update.add(fac.UpdateInstruction(cc_1, False))
    symbol = pyxb.binding.content.ElementUse(tTerminate._UseForTag(pyxb.namespace.ExpandedName(Namespace, 'target')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 290, 20))
    st_1 = fac.State(symbol, is_initial=True, final_update=final_update, is_unordered_catenation=False)
    states.append(st_1)
    final_update = set()
    final_update.add(fac.UpdateInstruction(cc_2, False))
    symbol = pyxb.binding.content.ElementUse(tTerminate._UseForTag(pyxb.namespace.ExpandedName(Namespace, 'source')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 292, 20))
    st_2 = fac.State(symbol, is_initial=True, final_update=final_update, is_unordered_catenation=False)
    states.append(st_2)
    transitions = []
    transitions.append(fac.Transition(st_0, [
        fac.UpdateInstruction(cc_0, True) ]))
    transitions.append(fac.Transition(st_1, [
        fac.UpdateInstruction(cc_0, False) ]))
    transitions.append(fac.Transition(st_2, [
        fac.UpdateInstruction(cc_0, False) ]))
    st_0._set_transitionSet(transitions)
    transitions = []
    transitions.append(fac.Transition(st_1, [
        fac.UpdateInstruction(cc_1, True) ]))
    transitions.append(fac.Transition(st_2, [
        fac.UpdateInstruction(cc_1, False) ]))
    st_1._set_transitionSet(transitions)
    transitions = []
    transitions.append(fac.Transition(st_2, [
        fac.UpdateInstruction(cc_2, True) ]))
    st_2._set_transitionSet(transitions)
    return fac.Automaton(states, counters, True, containing_state=None)
tTerminate._Automaton = _BuildAutomaton_37()




tFlow._AddElement(pyxb.binding.basis.element(pyxb.namespace.ExpandedName(Namespace, 'empty'), tEmpty, scope=tFlow, location=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 69, 12)))

tFlow._AddElement(pyxb.binding.basis.element(pyxb.namespace.ExpandedName(Namespace, 'invoke'), tInvoke, scope=tFlow, location=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 70, 12)))

tFlow._AddElement(pyxb.binding.basis.element(pyxb.namespace.ExpandedName(Namespace, 'receive'), tReceive, scope=tFlow, location=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 71, 12)))

tFlow._AddElement(pyxb.binding.basis.element(pyxb.namespace.ExpandedName(Namespace, 'reply'), tReply, scope=tFlow, location=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 72, 12)))

tFlow._AddElement(pyxb.binding.basis.element(pyxb.namespace.ExpandedName(Namespace, 'assign'), tAssign, scope=tFlow, location=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 73, 12)))

tFlow._AddElement(pyxb.binding.basis.element(pyxb.namespace.ExpandedName(Namespace, 'wait'), tWait, scope=tFlow, location=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 74, 12)))

tFlow._AddElement(pyxb.binding.basis.element(pyxb.namespace.ExpandedName(Namespace, 'throw'), tThrow, scope=tFlow, location=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 75, 12)))

tFlow._AddElement(pyxb.binding.basis.element(pyxb.namespace.ExpandedName(Namespace, 'terminate'), tTerminate, scope=tFlow, location=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 76, 12)))

tFlow._AddElement(pyxb.binding.basis.element(pyxb.namespace.ExpandedName(Namespace, 'flow'), tFlow, scope=tFlow, location=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 77, 12)))

tFlow._AddElement(pyxb.binding.basis.element(pyxb.namespace.ExpandedName(Namespace, 'switch'), tSwitch, scope=tFlow, location=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 78, 12)))

tFlow._AddElement(pyxb.binding.basis.element(pyxb.namespace.ExpandedName(Namespace, 'while'), tWhile, scope=tFlow, location=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 79, 12)))

tFlow._AddElement(pyxb.binding.basis.element(pyxb.namespace.ExpandedName(Namespace, 'sequence'), tSequence, scope=tFlow, location=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 80, 12)))

tFlow._AddElement(pyxb.binding.basis.element(pyxb.namespace.ExpandedName(Namespace, 'pick'), tPick, scope=tFlow, location=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 81, 12)))

tFlow._AddElement(pyxb.binding.basis.element(pyxb.namespace.ExpandedName(Namespace, 'scope'), tScope, scope=tFlow, location=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 82, 12)))

tFlow._AddElement(pyxb.binding.basis.element(pyxb.namespace.ExpandedName(Namespace, 'links'), tLinks, scope=tFlow, location=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 536, 20)))

def _BuildAutomaton_38 ():
    # Remove this helper function from the namespace after it is invoked
    global _BuildAutomaton_38
    del _BuildAutomaton_38
    import pyxb.utils.fac as fac

    counters = set()
    cc_0 = fac.CounterCondition(min=0, max=None, metadata=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 20, 12))
    counters.add(cc_0)
    cc_1 = fac.CounterCondition(min=0, max=None, metadata=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 290, 20))
    counters.add(cc_1)
    cc_2 = fac.CounterCondition(min=0, max=None, metadata=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 292, 20))
    counters.add(cc_2)
    cc_3 = fac.CounterCondition(min=0, max=1, metadata=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 536, 20))
    counters.add(cc_3)
    states = []
    final_update = None
    symbol = pyxb.binding.content.WildcardUse(pyxb.binding.content.Wildcard(process_contents=pyxb.binding.content.Wildcard.PC_lax, namespace_constraint=(pyxb.binding.content.Wildcard.NC_not, 'http://schemas.xmlsoap.org/ws/2003/03/business-process/')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 20, 12))
    st_0 = fac.State(symbol, is_initial=True, final_update=final_update, is_unordered_catenation=False)
    states.append(st_0)
    final_update = None
    symbol = pyxb.binding.content.ElementUse(tFlow._UseForTag(pyxb.namespace.ExpandedName(Namespace, 'target')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 290, 20))
    st_1 = fac.State(symbol, is_initial=True, final_update=final_update, is_unordered_catenation=False)
    states.append(st_1)
    final_update = None
    symbol = pyxb.binding.content.ElementUse(tFlow._UseForTag(pyxb.namespace.ExpandedName(Namespace, 'source')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 292, 20))
    st_2 = fac.State(symbol, is_initial=True, final_update=final_update, is_unordered_catenation=False)
    states.append(st_2)
    final_update = None
    symbol = pyxb.binding.content.ElementUse(tFlow._UseForTag(pyxb.namespace.ExpandedName(Namespace, 'links')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 536, 20))
    st_3 = fac.State(symbol, is_initial=True, final_update=final_update, is_unordered_catenation=False)
    states.append(st_3)
    final_update = set()
    symbol = pyxb.binding.content.ElementUse(tFlow._UseForTag(pyxb.namespace.ExpandedName(Namespace, 'empty')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 69, 12))
    st_4 = fac.State(symbol, is_initial=True, final_update=final_update, is_unordered_catenation=False)
    states.append(st_4)
    final_update = set()
    symbol = pyxb.binding.content.ElementUse(tFlow._UseForTag(pyxb.namespace.ExpandedName(Namespace, 'invoke')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 70, 12))
    st_5 = fac.State(symbol, is_initial=True, final_update=final_update, is_unordered_catenation=False)
    states.append(st_5)
    final_update = set()
    symbol = pyxb.binding.content.ElementUse(tFlow._UseForTag(pyxb.namespace.ExpandedName(Namespace, 'receive')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 71, 12))
    st_6 = fac.State(symbol, is_initial=True, final_update=final_update, is_unordered_catenation=False)
    states.append(st_6)
    final_update = set()
    symbol = pyxb.binding.content.ElementUse(tFlow._UseForTag(pyxb.namespace.ExpandedName(Namespace, 'reply')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 72, 12))
    st_7 = fac.State(symbol, is_initial=True, final_update=final_update, is_unordered_catenation=False)
    states.append(st_7)
    final_update = set()
    symbol = pyxb.binding.content.ElementUse(tFlow._UseForTag(pyxb.namespace.ExpandedName(Namespace, 'assign')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 73, 12))
    st_8 = fac.State(symbol, is_initial=True, final_update=final_update, is_unordered_catenation=False)
    states.append(st_8)
    final_update = set()
    symbol = pyxb.binding.content.ElementUse(tFlow._UseForTag(pyxb.namespace.ExpandedName(Namespace, 'wait')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 74, 12))
    st_9 = fac.State(symbol, is_initial=True, final_update=final_update, is_unordered_catenation=False)
    states.append(st_9)
    final_update = set()
    symbol = pyxb.binding.content.ElementUse(tFlow._UseForTag(pyxb.namespace.ExpandedName(Namespace, 'throw')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 75, 12))
    st_10 = fac.State(symbol, is_initial=True, final_update=final_update, is_unordered_catenation=False)
    states.append(st_10)
    final_update = set()
    symbol = pyxb.binding.content.ElementUse(tFlow._UseForTag(pyxb.namespace.ExpandedName(Namespace, 'terminate')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 76, 12))
    st_11 = fac.State(symbol, is_initial=True, final_update=final_update, is_unordered_catenation=False)
    states.append(st_11)
    final_update = set()
    symbol = pyxb.binding.content.ElementUse(tFlow._UseForTag(pyxb.namespace.ExpandedName(Namespace, 'flow')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 77, 12))
    st_12 = fac.State(symbol, is_initial=True, final_update=final_update, is_unordered_catenation=False)
    states.append(st_12)
    final_update = set()
    symbol = pyxb.binding.content.ElementUse(tFlow._UseForTag(pyxb.namespace.ExpandedName(Namespace, 'switch')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 78, 12))
    st_13 = fac.State(symbol, is_initial=True, final_update=final_update, is_unordered_catenation=False)
    states.append(st_13)
    final_update = set()
    symbol = pyxb.binding.content.ElementUse(tFlow._UseForTag(pyxb.namespace.ExpandedName(Namespace, 'while')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 79, 12))
    st_14 = fac.State(symbol, is_initial=True, final_update=final_update, is_unordered_catenation=False)
    states.append(st_14)
    final_update = set()
    symbol = pyxb.binding.content.ElementUse(tFlow._UseForTag(pyxb.namespace.ExpandedName(Namespace, 'sequence')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 80, 12))
    st_15 = fac.State(symbol, is_initial=True, final_update=final_update, is_unordered_catenation=False)
    states.append(st_15)
    final_update = set()
    symbol = pyxb.binding.content.ElementUse(tFlow._UseForTag(pyxb.namespace.ExpandedName(Namespace, 'pick')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 81, 12))
    st_16 = fac.State(symbol, is_initial=True, final_update=final_update, is_unordered_catenation=False)
    states.append(st_16)
    final_update = set()
    symbol = pyxb.binding.content.ElementUse(tFlow._UseForTag(pyxb.namespace.ExpandedName(Namespace, 'scope')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 82, 12))
    st_17 = fac.State(symbol, is_initial=True, final_update=final_update, is_unordered_catenation=False)
    states.append(st_17)
    transitions = []
    transitions.append(fac.Transition(st_0, [
        fac.UpdateInstruction(cc_0, True) ]))
    transitions.append(fac.Transition(st_1, [
        fac.UpdateInstruction(cc_0, False) ]))
    transitions.append(fac.Transition(st_2, [
        fac.UpdateInstruction(cc_0, False) ]))
    transitions.append(fac.Transition(st_3, [
        fac.UpdateInstruction(cc_0, False) ]))
    transitions.append(fac.Transition(st_4, [
        fac.UpdateInstruction(cc_0, False) ]))
    transitions.append(fac.Transition(st_5, [
        fac.UpdateInstruction(cc_0, False) ]))
    transitions.append(fac.Transition(st_6, [
        fac.UpdateInstruction(cc_0, False) ]))
    transitions.append(fac.Transition(st_7, [
        fac.UpdateInstruction(cc_0, False) ]))
    transitions.append(fac.Transition(st_8, [
        fac.UpdateInstruction(cc_0, False) ]))
    transitions.append(fac.Transition(st_9, [
        fac.UpdateInstruction(cc_0, False) ]))
    transitions.append(fac.Transition(st_10, [
        fac.UpdateInstruction(cc_0, False) ]))
    transitions.append(fac.Transition(st_11, [
        fac.UpdateInstruction(cc_0, False) ]))
    transitions.append(fac.Transition(st_12, [
        fac.UpdateInstruction(cc_0, False) ]))
    transitions.append(fac.Transition(st_13, [
        fac.UpdateInstruction(cc_0, False) ]))
    transitions.append(fac.Transition(st_14, [
        fac.UpdateInstruction(cc_0, False) ]))
    transitions.append(fac.Transition(st_15, [
        fac.UpdateInstruction(cc_0, False) ]))
    transitions.append(fac.Transition(st_16, [
        fac.UpdateInstruction(cc_0, False) ]))
    transitions.append(fac.Transition(st_17, [
        fac.UpdateInstruction(cc_0, False) ]))
    st_0._set_transitionSet(transitions)
    transitions = []
    transitions.append(fac.Transition(st_1, [
        fac.UpdateInstruction(cc_1, True) ]))
    transitions.append(fac.Transition(st_2, [
        fac.UpdateInstruction(cc_1, False) ]))
    transitions.append(fac.Transition(st_3, [
        fac.UpdateInstruction(cc_1, False) ]))
    transitions.append(fac.Transition(st_4, [
        fac.UpdateInstruction(cc_1, False) ]))
    transitions.append(fac.Transition(st_5, [
        fac.UpdateInstruction(cc_1, False) ]))
    transitions.append(fac.Transition(st_6, [
        fac.UpdateInstruction(cc_1, False) ]))
    transitions.append(fac.Transition(st_7, [
        fac.UpdateInstruction(cc_1, False) ]))
    transitions.append(fac.Transition(st_8, [
        fac.UpdateInstruction(cc_1, False) ]))
    transitions.append(fac.Transition(st_9, [
        fac.UpdateInstruction(cc_1, False) ]))
    transitions.append(fac.Transition(st_10, [
        fac.UpdateInstruction(cc_1, False) ]))
    transitions.append(fac.Transition(st_11, [
        fac.UpdateInstruction(cc_1, False) ]))
    transitions.append(fac.Transition(st_12, [
        fac.UpdateInstruction(cc_1, False) ]))
    transitions.append(fac.Transition(st_13, [
        fac.UpdateInstruction(cc_1, False) ]))
    transitions.append(fac.Transition(st_14, [
        fac.UpdateInstruction(cc_1, False) ]))
    transitions.append(fac.Transition(st_15, [
        fac.UpdateInstruction(cc_1, False) ]))
    transitions.append(fac.Transition(st_16, [
        fac.UpdateInstruction(cc_1, False) ]))
    transitions.append(fac.Transition(st_17, [
        fac.UpdateInstruction(cc_1, False) ]))
    st_1._set_transitionSet(transitions)
    transitions = []
    transitions.append(fac.Transition(st_2, [
        fac.UpdateInstruction(cc_2, True) ]))
    transitions.append(fac.Transition(st_3, [
        fac.UpdateInstruction(cc_2, False) ]))
    transitions.append(fac.Transition(st_4, [
        fac.UpdateInstruction(cc_2, False) ]))
    transitions.append(fac.Transition(st_5, [
        fac.UpdateInstruction(cc_2, False) ]))
    transitions.append(fac.Transition(st_6, [
        fac.UpdateInstruction(cc_2, False) ]))
    transitions.append(fac.Transition(st_7, [
        fac.UpdateInstruction(cc_2, False) ]))
    transitions.append(fac.Transition(st_8, [
        fac.UpdateInstruction(cc_2, False) ]))
    transitions.append(fac.Transition(st_9, [
        fac.UpdateInstruction(cc_2, False) ]))
    transitions.append(fac.Transition(st_10, [
        fac.UpdateInstruction(cc_2, False) ]))
    transitions.append(fac.Transition(st_11, [
        fac.UpdateInstruction(cc_2, False) ]))
    transitions.append(fac.Transition(st_12, [
        fac.UpdateInstruction(cc_2, False) ]))
    transitions.append(fac.Transition(st_13, [
        fac.UpdateInstruction(cc_2, False) ]))
    transitions.append(fac.Transition(st_14, [
        fac.UpdateInstruction(cc_2, False) ]))
    transitions.append(fac.Transition(st_15, [
        fac.UpdateInstruction(cc_2, False) ]))
    transitions.append(fac.Transition(st_16, [
        fac.UpdateInstruction(cc_2, False) ]))
    transitions.append(fac.Transition(st_17, [
        fac.UpdateInstruction(cc_2, False) ]))
    st_2._set_transitionSet(transitions)
    transitions = []
    transitions.append(fac.Transition(st_3, [
        fac.UpdateInstruction(cc_3, True) ]))
    transitions.append(fac.Transition(st_4, [
        fac.UpdateInstruction(cc_3, False) ]))
    transitions.append(fac.Transition(st_5, [
        fac.UpdateInstruction(cc_3, False) ]))
    transitions.append(fac.Transition(st_6, [
        fac.UpdateInstruction(cc_3, False) ]))
    transitions.append(fac.Transition(st_7, [
        fac.UpdateInstruction(cc_3, False) ]))
    transitions.append(fac.Transition(st_8, [
        fac.UpdateInstruction(cc_3, False) ]))
    transitions.append(fac.Transition(st_9, [
        fac.UpdateInstruction(cc_3, False) ]))
    transitions.append(fac.Transition(st_10, [
        fac.UpdateInstruction(cc_3, False) ]))
    transitions.append(fac.Transition(st_11, [
        fac.UpdateInstruction(cc_3, False) ]))
    transitions.append(fac.Transition(st_12, [
        fac.UpdateInstruction(cc_3, False) ]))
    transitions.append(fac.Transition(st_13, [
        fac.UpdateInstruction(cc_3, False) ]))
    transitions.append(fac.Transition(st_14, [
        fac.UpdateInstruction(cc_3, False) ]))
    transitions.append(fac.Transition(st_15, [
        fac.UpdateInstruction(cc_3, False) ]))
    transitions.append(fac.Transition(st_16, [
        fac.UpdateInstruction(cc_3, False) ]))
    transitions.append(fac.Transition(st_17, [
        fac.UpdateInstruction(cc_3, False) ]))
    st_3._set_transitionSet(transitions)
    transitions = []
    transitions.append(fac.Transition(st_4, [
         ]))
    transitions.append(fac.Transition(st_5, [
         ]))
    transitions.append(fac.Transition(st_6, [
         ]))
    transitions.append(fac.Transition(st_7, [
         ]))
    transitions.append(fac.Transition(st_8, [
         ]))
    transitions.append(fac.Transition(st_9, [
         ]))
    transitions.append(fac.Transition(st_10, [
         ]))
    transitions.append(fac.Transition(st_11, [
         ]))
    transitions.append(fac.Transition(st_12, [
         ]))
    transitions.append(fac.Transition(st_13, [
         ]))
    transitions.append(fac.Transition(st_14, [
         ]))
    transitions.append(fac.Transition(st_15, [
         ]))
    transitions.append(fac.Transition(st_16, [
         ]))
    transitions.append(fac.Transition(st_17, [
         ]))
    st_4._set_transitionSet(transitions)
    transitions = []
    transitions.append(fac.Transition(st_4, [
         ]))
    transitions.append(fac.Transition(st_5, [
         ]))
    transitions.append(fac.Transition(st_6, [
         ]))
    transitions.append(fac.Transition(st_7, [
         ]))
    transitions.append(fac.Transition(st_8, [
         ]))
    transitions.append(fac.Transition(st_9, [
         ]))
    transitions.append(fac.Transition(st_10, [
         ]))
    transitions.append(fac.Transition(st_11, [
         ]))
    transitions.append(fac.Transition(st_12, [
         ]))
    transitions.append(fac.Transition(st_13, [
         ]))
    transitions.append(fac.Transition(st_14, [
         ]))
    transitions.append(fac.Transition(st_15, [
         ]))
    transitions.append(fac.Transition(st_16, [
         ]))
    transitions.append(fac.Transition(st_17, [
         ]))
    st_5._set_transitionSet(transitions)
    transitions = []
    transitions.append(fac.Transition(st_4, [
         ]))
    transitions.append(fac.Transition(st_5, [
         ]))
    transitions.append(fac.Transition(st_6, [
         ]))
    transitions.append(fac.Transition(st_7, [
         ]))
    transitions.append(fac.Transition(st_8, [
         ]))
    transitions.append(fac.Transition(st_9, [
         ]))
    transitions.append(fac.Transition(st_10, [
         ]))
    transitions.append(fac.Transition(st_11, [
         ]))
    transitions.append(fac.Transition(st_12, [
         ]))
    transitions.append(fac.Transition(st_13, [
         ]))
    transitions.append(fac.Transition(st_14, [
         ]))
    transitions.append(fac.Transition(st_15, [
         ]))
    transitions.append(fac.Transition(st_16, [
         ]))
    transitions.append(fac.Transition(st_17, [
         ]))
    st_6._set_transitionSet(transitions)
    transitions = []
    transitions.append(fac.Transition(st_4, [
         ]))
    transitions.append(fac.Transition(st_5, [
         ]))
    transitions.append(fac.Transition(st_6, [
         ]))
    transitions.append(fac.Transition(st_7, [
         ]))
    transitions.append(fac.Transition(st_8, [
         ]))
    transitions.append(fac.Transition(st_9, [
         ]))
    transitions.append(fac.Transition(st_10, [
         ]))
    transitions.append(fac.Transition(st_11, [
         ]))
    transitions.append(fac.Transition(st_12, [
         ]))
    transitions.append(fac.Transition(st_13, [
         ]))
    transitions.append(fac.Transition(st_14, [
         ]))
    transitions.append(fac.Transition(st_15, [
         ]))
    transitions.append(fac.Transition(st_16, [
         ]))
    transitions.append(fac.Transition(st_17, [
         ]))
    st_7._set_transitionSet(transitions)
    transitions = []
    transitions.append(fac.Transition(st_4, [
         ]))
    transitions.append(fac.Transition(st_5, [
         ]))
    transitions.append(fac.Transition(st_6, [
         ]))
    transitions.append(fac.Transition(st_7, [
         ]))
    transitions.append(fac.Transition(st_8, [
         ]))
    transitions.append(fac.Transition(st_9, [
         ]))
    transitions.append(fac.Transition(st_10, [
         ]))
    transitions.append(fac.Transition(st_11, [
         ]))
    transitions.append(fac.Transition(st_12, [
         ]))
    transitions.append(fac.Transition(st_13, [
         ]))
    transitions.append(fac.Transition(st_14, [
         ]))
    transitions.append(fac.Transition(st_15, [
         ]))
    transitions.append(fac.Transition(st_16, [
         ]))
    transitions.append(fac.Transition(st_17, [
         ]))
    st_8._set_transitionSet(transitions)
    transitions = []
    transitions.append(fac.Transition(st_4, [
         ]))
    transitions.append(fac.Transition(st_5, [
         ]))
    transitions.append(fac.Transition(st_6, [
         ]))
    transitions.append(fac.Transition(st_7, [
         ]))
    transitions.append(fac.Transition(st_8, [
         ]))
    transitions.append(fac.Transition(st_9, [
         ]))
    transitions.append(fac.Transition(st_10, [
         ]))
    transitions.append(fac.Transition(st_11, [
         ]))
    transitions.append(fac.Transition(st_12, [
         ]))
    transitions.append(fac.Transition(st_13, [
         ]))
    transitions.append(fac.Transition(st_14, [
         ]))
    transitions.append(fac.Transition(st_15, [
         ]))
    transitions.append(fac.Transition(st_16, [
         ]))
    transitions.append(fac.Transition(st_17, [
         ]))
    st_9._set_transitionSet(transitions)
    transitions = []
    transitions.append(fac.Transition(st_4, [
         ]))
    transitions.append(fac.Transition(st_5, [
         ]))
    transitions.append(fac.Transition(st_6, [
         ]))
    transitions.append(fac.Transition(st_7, [
         ]))
    transitions.append(fac.Transition(st_8, [
         ]))
    transitions.append(fac.Transition(st_9, [
         ]))
    transitions.append(fac.Transition(st_10, [
         ]))
    transitions.append(fac.Transition(st_11, [
         ]))
    transitions.append(fac.Transition(st_12, [
         ]))
    transitions.append(fac.Transition(st_13, [
         ]))
    transitions.append(fac.Transition(st_14, [
         ]))
    transitions.append(fac.Transition(st_15, [
         ]))
    transitions.append(fac.Transition(st_16, [
         ]))
    transitions.append(fac.Transition(st_17, [
         ]))
    st_10._set_transitionSet(transitions)
    transitions = []
    transitions.append(fac.Transition(st_4, [
         ]))
    transitions.append(fac.Transition(st_5, [
         ]))
    transitions.append(fac.Transition(st_6, [
         ]))
    transitions.append(fac.Transition(st_7, [
         ]))
    transitions.append(fac.Transition(st_8, [
         ]))
    transitions.append(fac.Transition(st_9, [
         ]))
    transitions.append(fac.Transition(st_10, [
         ]))
    transitions.append(fac.Transition(st_11, [
         ]))
    transitions.append(fac.Transition(st_12, [
         ]))
    transitions.append(fac.Transition(st_13, [
         ]))
    transitions.append(fac.Transition(st_14, [
         ]))
    transitions.append(fac.Transition(st_15, [
         ]))
    transitions.append(fac.Transition(st_16, [
         ]))
    transitions.append(fac.Transition(st_17, [
         ]))
    st_11._set_transitionSet(transitions)
    transitions = []
    transitions.append(fac.Transition(st_4, [
         ]))
    transitions.append(fac.Transition(st_5, [
         ]))
    transitions.append(fac.Transition(st_6, [
         ]))
    transitions.append(fac.Transition(st_7, [
         ]))
    transitions.append(fac.Transition(st_8, [
         ]))
    transitions.append(fac.Transition(st_9, [
         ]))
    transitions.append(fac.Transition(st_10, [
         ]))
    transitions.append(fac.Transition(st_11, [
         ]))
    transitions.append(fac.Transition(st_12, [
         ]))
    transitions.append(fac.Transition(st_13, [
         ]))
    transitions.append(fac.Transition(st_14, [
         ]))
    transitions.append(fac.Transition(st_15, [
         ]))
    transitions.append(fac.Transition(st_16, [
         ]))
    transitions.append(fac.Transition(st_17, [
         ]))
    st_12._set_transitionSet(transitions)
    transitions = []
    transitions.append(fac.Transition(st_4, [
         ]))
    transitions.append(fac.Transition(st_5, [
         ]))
    transitions.append(fac.Transition(st_6, [
         ]))
    transitions.append(fac.Transition(st_7, [
         ]))
    transitions.append(fac.Transition(st_8, [
         ]))
    transitions.append(fac.Transition(st_9, [
         ]))
    transitions.append(fac.Transition(st_10, [
         ]))
    transitions.append(fac.Transition(st_11, [
         ]))
    transitions.append(fac.Transition(st_12, [
         ]))
    transitions.append(fac.Transition(st_13, [
         ]))
    transitions.append(fac.Transition(st_14, [
         ]))
    transitions.append(fac.Transition(st_15, [
         ]))
    transitions.append(fac.Transition(st_16, [
         ]))
    transitions.append(fac.Transition(st_17, [
         ]))
    st_13._set_transitionSet(transitions)
    transitions = []
    transitions.append(fac.Transition(st_4, [
         ]))
    transitions.append(fac.Transition(st_5, [
         ]))
    transitions.append(fac.Transition(st_6, [
         ]))
    transitions.append(fac.Transition(st_7, [
         ]))
    transitions.append(fac.Transition(st_8, [
         ]))
    transitions.append(fac.Transition(st_9, [
         ]))
    transitions.append(fac.Transition(st_10, [
         ]))
    transitions.append(fac.Transition(st_11, [
         ]))
    transitions.append(fac.Transition(st_12, [
         ]))
    transitions.append(fac.Transition(st_13, [
         ]))
    transitions.append(fac.Transition(st_14, [
         ]))
    transitions.append(fac.Transition(st_15, [
         ]))
    transitions.append(fac.Transition(st_16, [
         ]))
    transitions.append(fac.Transition(st_17, [
         ]))
    st_14._set_transitionSet(transitions)
    transitions = []
    transitions.append(fac.Transition(st_4, [
         ]))
    transitions.append(fac.Transition(st_5, [
         ]))
    transitions.append(fac.Transition(st_6, [
         ]))
    transitions.append(fac.Transition(st_7, [
         ]))
    transitions.append(fac.Transition(st_8, [
         ]))
    transitions.append(fac.Transition(st_9, [
         ]))
    transitions.append(fac.Transition(st_10, [
         ]))
    transitions.append(fac.Transition(st_11, [
         ]))
    transitions.append(fac.Transition(st_12, [
         ]))
    transitions.append(fac.Transition(st_13, [
         ]))
    transitions.append(fac.Transition(st_14, [
         ]))
    transitions.append(fac.Transition(st_15, [
         ]))
    transitions.append(fac.Transition(st_16, [
         ]))
    transitions.append(fac.Transition(st_17, [
         ]))
    st_15._set_transitionSet(transitions)
    transitions = []
    transitions.append(fac.Transition(st_4, [
         ]))
    transitions.append(fac.Transition(st_5, [
         ]))
    transitions.append(fac.Transition(st_6, [
         ]))
    transitions.append(fac.Transition(st_7, [
         ]))
    transitions.append(fac.Transition(st_8, [
         ]))
    transitions.append(fac.Transition(st_9, [
         ]))
    transitions.append(fac.Transition(st_10, [
         ]))
    transitions.append(fac.Transition(st_11, [
         ]))
    transitions.append(fac.Transition(st_12, [
         ]))
    transitions.append(fac.Transition(st_13, [
         ]))
    transitions.append(fac.Transition(st_14, [
         ]))
    transitions.append(fac.Transition(st_15, [
         ]))
    transitions.append(fac.Transition(st_16, [
         ]))
    transitions.append(fac.Transition(st_17, [
         ]))
    st_16._set_transitionSet(transitions)
    transitions = []
    transitions.append(fac.Transition(st_4, [
         ]))
    transitions.append(fac.Transition(st_5, [
         ]))
    transitions.append(fac.Transition(st_6, [
         ]))
    transitions.append(fac.Transition(st_7, [
         ]))
    transitions.append(fac.Transition(st_8, [
         ]))
    transitions.append(fac.Transition(st_9, [
         ]))
    transitions.append(fac.Transition(st_10, [
         ]))
    transitions.append(fac.Transition(st_11, [
         ]))
    transitions.append(fac.Transition(st_12, [
         ]))
    transitions.append(fac.Transition(st_13, [
         ]))
    transitions.append(fac.Transition(st_14, [
         ]))
    transitions.append(fac.Transition(st_15, [
         ]))
    transitions.append(fac.Transition(st_16, [
         ]))
    transitions.append(fac.Transition(st_17, [
         ]))
    st_17._set_transitionSet(transitions)
    return fac.Automaton(states, counters, False, containing_state=None)
tFlow._Automaton = _BuildAutomaton_38()




tSwitch._AddElement(pyxb.binding.basis.element(pyxb.namespace.ExpandedName(Namespace, 'case'), CTD_ANON_2, scope=tSwitch, location=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 567, 20)))

tSwitch._AddElement(pyxb.binding.basis.element(pyxb.namespace.ExpandedName(Namespace, 'otherwise'), tActivityContainer, scope=tSwitch, location=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 578, 20)))

def _BuildAutomaton_39 ():
    # Remove this helper function from the namespace after it is invoked
    global _BuildAutomaton_39
    del _BuildAutomaton_39
    import pyxb.utils.fac as fac

    counters = set()
    cc_0 = fac.CounterCondition(min=0, max=None, metadata=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 20, 12))
    counters.add(cc_0)
    cc_1 = fac.CounterCondition(min=0, max=None, metadata=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 290, 20))
    counters.add(cc_1)
    cc_2 = fac.CounterCondition(min=0, max=None, metadata=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 292, 20))
    counters.add(cc_2)
    cc_3 = fac.CounterCondition(min=0, max=1, metadata=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 578, 20))
    counters.add(cc_3)
    states = []
    final_update = None
    symbol = pyxb.binding.content.WildcardUse(pyxb.binding.content.Wildcard(process_contents=pyxb.binding.content.Wildcard.PC_lax, namespace_constraint=(pyxb.binding.content.Wildcard.NC_not, 'http://schemas.xmlsoap.org/ws/2003/03/business-process/')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 20, 12))
    st_0 = fac.State(symbol, is_initial=True, final_update=final_update, is_unordered_catenation=False)
    states.append(st_0)
    final_update = None
    symbol = pyxb.binding.content.ElementUse(tSwitch._UseForTag(pyxb.namespace.ExpandedName(Namespace, 'target')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 290, 20))
    st_1 = fac.State(symbol, is_initial=True, final_update=final_update, is_unordered_catenation=False)
    states.append(st_1)
    final_update = None
    symbol = pyxb.binding.content.ElementUse(tSwitch._UseForTag(pyxb.namespace.ExpandedName(Namespace, 'source')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 292, 20))
    st_2 = fac.State(symbol, is_initial=True, final_update=final_update, is_unordered_catenation=False)
    states.append(st_2)
    final_update = set()
    symbol = pyxb.binding.content.ElementUse(tSwitch._UseForTag(pyxb.namespace.ExpandedName(Namespace, 'case')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 567, 20))
    st_3 = fac.State(symbol, is_initial=True, final_update=final_update, is_unordered_catenation=False)
    states.append(st_3)
    final_update = set()
    final_update.add(fac.UpdateInstruction(cc_3, False))
    symbol = pyxb.binding.content.ElementUse(tSwitch._UseForTag(pyxb.namespace.ExpandedName(Namespace, 'otherwise')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 578, 20))
    st_4 = fac.State(symbol, is_initial=False, final_update=final_update, is_unordered_catenation=False)
    states.append(st_4)
    transitions = []
    transitions.append(fac.Transition(st_0, [
        fac.UpdateInstruction(cc_0, True) ]))
    transitions.append(fac.Transition(st_1, [
        fac.UpdateInstruction(cc_0, False) ]))
    transitions.append(fac.Transition(st_2, [
        fac.UpdateInstruction(cc_0, False) ]))
    transitions.append(fac.Transition(st_3, [
        fac.UpdateInstruction(cc_0, False) ]))
    st_0._set_transitionSet(transitions)
    transitions = []
    transitions.append(fac.Transition(st_1, [
        fac.UpdateInstruction(cc_1, True) ]))
    transitions.append(fac.Transition(st_2, [
        fac.UpdateInstruction(cc_1, False) ]))
    transitions.append(fac.Transition(st_3, [
        fac.UpdateInstruction(cc_1, False) ]))
    st_1._set_transitionSet(transitions)
    transitions = []
    transitions.append(fac.Transition(st_2, [
        fac.UpdateInstruction(cc_2, True) ]))
    transitions.append(fac.Transition(st_3, [
        fac.UpdateInstruction(cc_2, False) ]))
    st_2._set_transitionSet(transitions)
    transitions = []
    transitions.append(fac.Transition(st_3, [
         ]))
    transitions.append(fac.Transition(st_4, [
         ]))
    st_3._set_transitionSet(transitions)
    transitions = []
    transitions.append(fac.Transition(st_4, [
        fac.UpdateInstruction(cc_3, True) ]))
    st_4._set_transitionSet(transitions)
    return fac.Automaton(states, counters, False, containing_state=None)
tSwitch._Automaton = _BuildAutomaton_39()




def _BuildAutomaton_40 ():
    # Remove this helper function from the namespace after it is invoked
    global _BuildAutomaton_40
    del _BuildAutomaton_40
    import pyxb.utils.fac as fac

    counters = set()
    cc_0 = fac.CounterCondition(min=0, max=None, metadata=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 20, 12))
    counters.add(cc_0)
    states = []
    final_update = None
    symbol = pyxb.binding.content.WildcardUse(pyxb.binding.content.Wildcard(process_contents=pyxb.binding.content.Wildcard.PC_lax, namespace_constraint=(pyxb.binding.content.Wildcard.NC_not, 'http://schemas.xmlsoap.org/ws/2003/03/business-process/')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 20, 12))
    st_0 = fac.State(symbol, is_initial=True, final_update=final_update, is_unordered_catenation=False)
    states.append(st_0)
    final_update = set()
    symbol = pyxb.binding.content.ElementUse(CTD_ANON_2._UseForTag(pyxb.namespace.ExpandedName(Namespace, 'empty')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 69, 12))
    st_1 = fac.State(symbol, is_initial=True, final_update=final_update, is_unordered_catenation=False)
    states.append(st_1)
    final_update = set()
    symbol = pyxb.binding.content.ElementUse(CTD_ANON_2._UseForTag(pyxb.namespace.ExpandedName(Namespace, 'invoke')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 70, 12))
    st_2 = fac.State(symbol, is_initial=True, final_update=final_update, is_unordered_catenation=False)
    states.append(st_2)
    final_update = set()
    symbol = pyxb.binding.content.ElementUse(CTD_ANON_2._UseForTag(pyxb.namespace.ExpandedName(Namespace, 'receive')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 71, 12))
    st_3 = fac.State(symbol, is_initial=True, final_update=final_update, is_unordered_catenation=False)
    states.append(st_3)
    final_update = set()
    symbol = pyxb.binding.content.ElementUse(CTD_ANON_2._UseForTag(pyxb.namespace.ExpandedName(Namespace, 'reply')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 72, 12))
    st_4 = fac.State(symbol, is_initial=True, final_update=final_update, is_unordered_catenation=False)
    states.append(st_4)
    final_update = set()
    symbol = pyxb.binding.content.ElementUse(CTD_ANON_2._UseForTag(pyxb.namespace.ExpandedName(Namespace, 'assign')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 73, 12))
    st_5 = fac.State(symbol, is_initial=True, final_update=final_update, is_unordered_catenation=False)
    states.append(st_5)
    final_update = set()
    symbol = pyxb.binding.content.ElementUse(CTD_ANON_2._UseForTag(pyxb.namespace.ExpandedName(Namespace, 'wait')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 74, 12))
    st_6 = fac.State(symbol, is_initial=True, final_update=final_update, is_unordered_catenation=False)
    states.append(st_6)
    final_update = set()
    symbol = pyxb.binding.content.ElementUse(CTD_ANON_2._UseForTag(pyxb.namespace.ExpandedName(Namespace, 'throw')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 75, 12))
    st_7 = fac.State(symbol, is_initial=True, final_update=final_update, is_unordered_catenation=False)
    states.append(st_7)
    final_update = set()
    symbol = pyxb.binding.content.ElementUse(CTD_ANON_2._UseForTag(pyxb.namespace.ExpandedName(Namespace, 'terminate')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 76, 12))
    st_8 = fac.State(symbol, is_initial=True, final_update=final_update, is_unordered_catenation=False)
    states.append(st_8)
    final_update = set()
    symbol = pyxb.binding.content.ElementUse(CTD_ANON_2._UseForTag(pyxb.namespace.ExpandedName(Namespace, 'flow')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 77, 12))
    st_9 = fac.State(symbol, is_initial=True, final_update=final_update, is_unordered_catenation=False)
    states.append(st_9)
    final_update = set()
    symbol = pyxb.binding.content.ElementUse(CTD_ANON_2._UseForTag(pyxb.namespace.ExpandedName(Namespace, 'switch')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 78, 12))
    st_10 = fac.State(symbol, is_initial=True, final_update=final_update, is_unordered_catenation=False)
    states.append(st_10)
    final_update = set()
    symbol = pyxb.binding.content.ElementUse(CTD_ANON_2._UseForTag(pyxb.namespace.ExpandedName(Namespace, 'while')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 79, 12))
    st_11 = fac.State(symbol, is_initial=True, final_update=final_update, is_unordered_catenation=False)
    states.append(st_11)
    final_update = set()
    symbol = pyxb.binding.content.ElementUse(CTD_ANON_2._UseForTag(pyxb.namespace.ExpandedName(Namespace, 'sequence')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 80, 12))
    st_12 = fac.State(symbol, is_initial=True, final_update=final_update, is_unordered_catenation=False)
    states.append(st_12)
    final_update = set()
    symbol = pyxb.binding.content.ElementUse(CTD_ANON_2._UseForTag(pyxb.namespace.ExpandedName(Namespace, 'pick')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 81, 12))
    st_13 = fac.State(symbol, is_initial=True, final_update=final_update, is_unordered_catenation=False)
    states.append(st_13)
    final_update = set()
    symbol = pyxb.binding.content.ElementUse(CTD_ANON_2._UseForTag(pyxb.namespace.ExpandedName(Namespace, 'scope')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 82, 12))
    st_14 = fac.State(symbol, is_initial=True, final_update=final_update, is_unordered_catenation=False)
    states.append(st_14)
    transitions = []
    transitions.append(fac.Transition(st_0, [
        fac.UpdateInstruction(cc_0, True) ]))
    transitions.append(fac.Transition(st_1, [
        fac.UpdateInstruction(cc_0, False) ]))
    transitions.append(fac.Transition(st_2, [
        fac.UpdateInstruction(cc_0, False) ]))
    transitions.append(fac.Transition(st_3, [
        fac.UpdateInstruction(cc_0, False) ]))
    transitions.append(fac.Transition(st_4, [
        fac.UpdateInstruction(cc_0, False) ]))
    transitions.append(fac.Transition(st_5, [
        fac.UpdateInstruction(cc_0, False) ]))
    transitions.append(fac.Transition(st_6, [
        fac.UpdateInstruction(cc_0, False) ]))
    transitions.append(fac.Transition(st_7, [
        fac.UpdateInstruction(cc_0, False) ]))
    transitions.append(fac.Transition(st_8, [
        fac.UpdateInstruction(cc_0, False) ]))
    transitions.append(fac.Transition(st_9, [
        fac.UpdateInstruction(cc_0, False) ]))
    transitions.append(fac.Transition(st_10, [
        fac.UpdateInstruction(cc_0, False) ]))
    transitions.append(fac.Transition(st_11, [
        fac.UpdateInstruction(cc_0, False) ]))
    transitions.append(fac.Transition(st_12, [
        fac.UpdateInstruction(cc_0, False) ]))
    transitions.append(fac.Transition(st_13, [
        fac.UpdateInstruction(cc_0, False) ]))
    transitions.append(fac.Transition(st_14, [
        fac.UpdateInstruction(cc_0, False) ]))
    st_0._set_transitionSet(transitions)
    transitions = []
    st_1._set_transitionSet(transitions)
    transitions = []
    st_2._set_transitionSet(transitions)
    transitions = []
    st_3._set_transitionSet(transitions)
    transitions = []
    st_4._set_transitionSet(transitions)
    transitions = []
    st_5._set_transitionSet(transitions)
    transitions = []
    st_6._set_transitionSet(transitions)
    transitions = []
    st_7._set_transitionSet(transitions)
    transitions = []
    st_8._set_transitionSet(transitions)
    transitions = []
    st_9._set_transitionSet(transitions)
    transitions = []
    st_10._set_transitionSet(transitions)
    transitions = []
    st_11._set_transitionSet(transitions)
    transitions = []
    st_12._set_transitionSet(transitions)
    transitions = []
    st_13._set_transitionSet(transitions)
    transitions = []
    st_14._set_transitionSet(transitions)
    return fac.Automaton(states, counters, False, containing_state=None)
CTD_ANON_2._Automaton = _BuildAutomaton_40()




tWhile._AddElement(pyxb.binding.basis.element(pyxb.namespace.ExpandedName(Namespace, 'empty'), tEmpty, scope=tWhile, location=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 69, 12)))

tWhile._AddElement(pyxb.binding.basis.element(pyxb.namespace.ExpandedName(Namespace, 'invoke'), tInvoke, scope=tWhile, location=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 70, 12)))

tWhile._AddElement(pyxb.binding.basis.element(pyxb.namespace.ExpandedName(Namespace, 'receive'), tReceive, scope=tWhile, location=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 71, 12)))

tWhile._AddElement(pyxb.binding.basis.element(pyxb.namespace.ExpandedName(Namespace, 'reply'), tReply, scope=tWhile, location=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 72, 12)))

tWhile._AddElement(pyxb.binding.basis.element(pyxb.namespace.ExpandedName(Namespace, 'assign'), tAssign, scope=tWhile, location=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 73, 12)))

tWhile._AddElement(pyxb.binding.basis.element(pyxb.namespace.ExpandedName(Namespace, 'wait'), tWait, scope=tWhile, location=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 74, 12)))

tWhile._AddElement(pyxb.binding.basis.element(pyxb.namespace.ExpandedName(Namespace, 'throw'), tThrow, scope=tWhile, location=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 75, 12)))

tWhile._AddElement(pyxb.binding.basis.element(pyxb.namespace.ExpandedName(Namespace, 'terminate'), tTerminate, scope=tWhile, location=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 76, 12)))

tWhile._AddElement(pyxb.binding.basis.element(pyxb.namespace.ExpandedName(Namespace, 'flow'), tFlow, scope=tWhile, location=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 77, 12)))

tWhile._AddElement(pyxb.binding.basis.element(pyxb.namespace.ExpandedName(Namespace, 'switch'), tSwitch, scope=tWhile, location=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 78, 12)))

tWhile._AddElement(pyxb.binding.basis.element(pyxb.namespace.ExpandedName(Namespace, 'while'), tWhile, scope=tWhile, location=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 79, 12)))

tWhile._AddElement(pyxb.binding.basis.element(pyxb.namespace.ExpandedName(Namespace, 'sequence'), tSequence, scope=tWhile, location=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 80, 12)))

tWhile._AddElement(pyxb.binding.basis.element(pyxb.namespace.ExpandedName(Namespace, 'pick'), tPick, scope=tWhile, location=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 81, 12)))

tWhile._AddElement(pyxb.binding.basis.element(pyxb.namespace.ExpandedName(Namespace, 'scope'), tScope, scope=tWhile, location=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 82, 12)))

def _BuildAutomaton_41 ():
    # Remove this helper function from the namespace after it is invoked
    global _BuildAutomaton_41
    del _BuildAutomaton_41
    import pyxb.utils.fac as fac

    counters = set()
    cc_0 = fac.CounterCondition(min=0, max=None, metadata=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 20, 12))
    counters.add(cc_0)
    cc_1 = fac.CounterCondition(min=0, max=None, metadata=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 290, 20))
    counters.add(cc_1)
    cc_2 = fac.CounterCondition(min=0, max=None, metadata=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 292, 20))
    counters.add(cc_2)
    states = []
    final_update = None
    symbol = pyxb.binding.content.WildcardUse(pyxb.binding.content.Wildcard(process_contents=pyxb.binding.content.Wildcard.PC_lax, namespace_constraint=(pyxb.binding.content.Wildcard.NC_not, 'http://schemas.xmlsoap.org/ws/2003/03/business-process/')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 20, 12))
    st_0 = fac.State(symbol, is_initial=True, final_update=final_update, is_unordered_catenation=False)
    states.append(st_0)
    final_update = None
    symbol = pyxb.binding.content.ElementUse(tWhile._UseForTag(pyxb.namespace.ExpandedName(Namespace, 'target')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 290, 20))
    st_1 = fac.State(symbol, is_initial=True, final_update=final_update, is_unordered_catenation=False)
    states.append(st_1)
    final_update = None
    symbol = pyxb.binding.content.ElementUse(tWhile._UseForTag(pyxb.namespace.ExpandedName(Namespace, 'source')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 292, 20))
    st_2 = fac.State(symbol, is_initial=True, final_update=final_update, is_unordered_catenation=False)
    states.append(st_2)
    final_update = set()
    symbol = pyxb.binding.content.ElementUse(tWhile._UseForTag(pyxb.namespace.ExpandedName(Namespace, 'empty')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 69, 12))
    st_3 = fac.State(symbol, is_initial=True, final_update=final_update, is_unordered_catenation=False)
    states.append(st_3)
    final_update = set()
    symbol = pyxb.binding.content.ElementUse(tWhile._UseForTag(pyxb.namespace.ExpandedName(Namespace, 'invoke')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 70, 12))
    st_4 = fac.State(symbol, is_initial=True, final_update=final_update, is_unordered_catenation=False)
    states.append(st_4)
    final_update = set()
    symbol = pyxb.binding.content.ElementUse(tWhile._UseForTag(pyxb.namespace.ExpandedName(Namespace, 'receive')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 71, 12))
    st_5 = fac.State(symbol, is_initial=True, final_update=final_update, is_unordered_catenation=False)
    states.append(st_5)
    final_update = set()
    symbol = pyxb.binding.content.ElementUse(tWhile._UseForTag(pyxb.namespace.ExpandedName(Namespace, 'reply')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 72, 12))
    st_6 = fac.State(symbol, is_initial=True, final_update=final_update, is_unordered_catenation=False)
    states.append(st_6)
    final_update = set()
    symbol = pyxb.binding.content.ElementUse(tWhile._UseForTag(pyxb.namespace.ExpandedName(Namespace, 'assign')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 73, 12))
    st_7 = fac.State(symbol, is_initial=True, final_update=final_update, is_unordered_catenation=False)
    states.append(st_7)
    final_update = set()
    symbol = pyxb.binding.content.ElementUse(tWhile._UseForTag(pyxb.namespace.ExpandedName(Namespace, 'wait')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 74, 12))
    st_8 = fac.State(symbol, is_initial=True, final_update=final_update, is_unordered_catenation=False)
    states.append(st_8)
    final_update = set()
    symbol = pyxb.binding.content.ElementUse(tWhile._UseForTag(pyxb.namespace.ExpandedName(Namespace, 'throw')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 75, 12))
    st_9 = fac.State(symbol, is_initial=True, final_update=final_update, is_unordered_catenation=False)
    states.append(st_9)
    final_update = set()
    symbol = pyxb.binding.content.ElementUse(tWhile._UseForTag(pyxb.namespace.ExpandedName(Namespace, 'terminate')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 76, 12))
    st_10 = fac.State(symbol, is_initial=True, final_update=final_update, is_unordered_catenation=False)
    states.append(st_10)
    final_update = set()
    symbol = pyxb.binding.content.ElementUse(tWhile._UseForTag(pyxb.namespace.ExpandedName(Namespace, 'flow')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 77, 12))
    st_11 = fac.State(symbol, is_initial=True, final_update=final_update, is_unordered_catenation=False)
    states.append(st_11)
    final_update = set()
    symbol = pyxb.binding.content.ElementUse(tWhile._UseForTag(pyxb.namespace.ExpandedName(Namespace, 'switch')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 78, 12))
    st_12 = fac.State(symbol, is_initial=True, final_update=final_update, is_unordered_catenation=False)
    states.append(st_12)
    final_update = set()
    symbol = pyxb.binding.content.ElementUse(tWhile._UseForTag(pyxb.namespace.ExpandedName(Namespace, 'while')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 79, 12))
    st_13 = fac.State(symbol, is_initial=True, final_update=final_update, is_unordered_catenation=False)
    states.append(st_13)
    final_update = set()
    symbol = pyxb.binding.content.ElementUse(tWhile._UseForTag(pyxb.namespace.ExpandedName(Namespace, 'sequence')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 80, 12))
    st_14 = fac.State(symbol, is_initial=True, final_update=final_update, is_unordered_catenation=False)
    states.append(st_14)
    final_update = set()
    symbol = pyxb.binding.content.ElementUse(tWhile._UseForTag(pyxb.namespace.ExpandedName(Namespace, 'pick')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 81, 12))
    st_15 = fac.State(symbol, is_initial=True, final_update=final_update, is_unordered_catenation=False)
    states.append(st_15)
    final_update = set()
    symbol = pyxb.binding.content.ElementUse(tWhile._UseForTag(pyxb.namespace.ExpandedName(Namespace, 'scope')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 82, 12))
    st_16 = fac.State(symbol, is_initial=True, final_update=final_update, is_unordered_catenation=False)
    states.append(st_16)
    transitions = []
    transitions.append(fac.Transition(st_0, [
        fac.UpdateInstruction(cc_0, True) ]))
    transitions.append(fac.Transition(st_1, [
        fac.UpdateInstruction(cc_0, False) ]))
    transitions.append(fac.Transition(st_2, [
        fac.UpdateInstruction(cc_0, False) ]))
    transitions.append(fac.Transition(st_3, [
        fac.UpdateInstruction(cc_0, False) ]))
    transitions.append(fac.Transition(st_4, [
        fac.UpdateInstruction(cc_0, False) ]))
    transitions.append(fac.Transition(st_5, [
        fac.UpdateInstruction(cc_0, False) ]))
    transitions.append(fac.Transition(st_6, [
        fac.UpdateInstruction(cc_0, False) ]))
    transitions.append(fac.Transition(st_7, [
        fac.UpdateInstruction(cc_0, False) ]))
    transitions.append(fac.Transition(st_8, [
        fac.UpdateInstruction(cc_0, False) ]))
    transitions.append(fac.Transition(st_9, [
        fac.UpdateInstruction(cc_0, False) ]))
    transitions.append(fac.Transition(st_10, [
        fac.UpdateInstruction(cc_0, False) ]))
    transitions.append(fac.Transition(st_11, [
        fac.UpdateInstruction(cc_0, False) ]))
    transitions.append(fac.Transition(st_12, [
        fac.UpdateInstruction(cc_0, False) ]))
    transitions.append(fac.Transition(st_13, [
        fac.UpdateInstruction(cc_0, False) ]))
    transitions.append(fac.Transition(st_14, [
        fac.UpdateInstruction(cc_0, False) ]))
    transitions.append(fac.Transition(st_15, [
        fac.UpdateInstruction(cc_0, False) ]))
    transitions.append(fac.Transition(st_16, [
        fac.UpdateInstruction(cc_0, False) ]))
    st_0._set_transitionSet(transitions)
    transitions = []
    transitions.append(fac.Transition(st_1, [
        fac.UpdateInstruction(cc_1, True) ]))
    transitions.append(fac.Transition(st_2, [
        fac.UpdateInstruction(cc_1, False) ]))
    transitions.append(fac.Transition(st_3, [
        fac.UpdateInstruction(cc_1, False) ]))
    transitions.append(fac.Transition(st_4, [
        fac.UpdateInstruction(cc_1, False) ]))
    transitions.append(fac.Transition(st_5, [
        fac.UpdateInstruction(cc_1, False) ]))
    transitions.append(fac.Transition(st_6, [
        fac.UpdateInstruction(cc_1, False) ]))
    transitions.append(fac.Transition(st_7, [
        fac.UpdateInstruction(cc_1, False) ]))
    transitions.append(fac.Transition(st_8, [
        fac.UpdateInstruction(cc_1, False) ]))
    transitions.append(fac.Transition(st_9, [
        fac.UpdateInstruction(cc_1, False) ]))
    transitions.append(fac.Transition(st_10, [
        fac.UpdateInstruction(cc_1, False) ]))
    transitions.append(fac.Transition(st_11, [
        fac.UpdateInstruction(cc_1, False) ]))
    transitions.append(fac.Transition(st_12, [
        fac.UpdateInstruction(cc_1, False) ]))
    transitions.append(fac.Transition(st_13, [
        fac.UpdateInstruction(cc_1, False) ]))
    transitions.append(fac.Transition(st_14, [
        fac.UpdateInstruction(cc_1, False) ]))
    transitions.append(fac.Transition(st_15, [
        fac.UpdateInstruction(cc_1, False) ]))
    transitions.append(fac.Transition(st_16, [
        fac.UpdateInstruction(cc_1, False) ]))
    st_1._set_transitionSet(transitions)
    transitions = []
    transitions.append(fac.Transition(st_2, [
        fac.UpdateInstruction(cc_2, True) ]))
    transitions.append(fac.Transition(st_3, [
        fac.UpdateInstruction(cc_2, False) ]))
    transitions.append(fac.Transition(st_4, [
        fac.UpdateInstruction(cc_2, False) ]))
    transitions.append(fac.Transition(st_5, [
        fac.UpdateInstruction(cc_2, False) ]))
    transitions.append(fac.Transition(st_6, [
        fac.UpdateInstruction(cc_2, False) ]))
    transitions.append(fac.Transition(st_7, [
        fac.UpdateInstruction(cc_2, False) ]))
    transitions.append(fac.Transition(st_8, [
        fac.UpdateInstruction(cc_2, False) ]))
    transitions.append(fac.Transition(st_9, [
        fac.UpdateInstruction(cc_2, False) ]))
    transitions.append(fac.Transition(st_10, [
        fac.UpdateInstruction(cc_2, False) ]))
    transitions.append(fac.Transition(st_11, [
        fac.UpdateInstruction(cc_2, False) ]))
    transitions.append(fac.Transition(st_12, [
        fac.UpdateInstruction(cc_2, False) ]))
    transitions.append(fac.Transition(st_13, [
        fac.UpdateInstruction(cc_2, False) ]))
    transitions.append(fac.Transition(st_14, [
        fac.UpdateInstruction(cc_2, False) ]))
    transitions.append(fac.Transition(st_15, [
        fac.UpdateInstruction(cc_2, False) ]))
    transitions.append(fac.Transition(st_16, [
        fac.UpdateInstruction(cc_2, False) ]))
    st_2._set_transitionSet(transitions)
    transitions = []
    st_3._set_transitionSet(transitions)
    transitions = []
    st_4._set_transitionSet(transitions)
    transitions = []
    st_5._set_transitionSet(transitions)
    transitions = []
    st_6._set_transitionSet(transitions)
    transitions = []
    st_7._set_transitionSet(transitions)
    transitions = []
    st_8._set_transitionSet(transitions)
    transitions = []
    st_9._set_transitionSet(transitions)
    transitions = []
    st_10._set_transitionSet(transitions)
    transitions = []
    st_11._set_transitionSet(transitions)
    transitions = []
    st_12._set_transitionSet(transitions)
    transitions = []
    st_13._set_transitionSet(transitions)
    transitions = []
    st_14._set_transitionSet(transitions)
    transitions = []
    st_15._set_transitionSet(transitions)
    transitions = []
    st_16._set_transitionSet(transitions)
    return fac.Automaton(states, counters, False, containing_state=None)
tWhile._Automaton = _BuildAutomaton_41()




tSequence._AddElement(pyxb.binding.basis.element(pyxb.namespace.ExpandedName(Namespace, 'empty'), tEmpty, scope=tSequence, location=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 69, 12)))

tSequence._AddElement(pyxb.binding.basis.element(pyxb.namespace.ExpandedName(Namespace, 'invoke'), tInvoke, scope=tSequence, location=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 70, 12)))

tSequence._AddElement(pyxb.binding.basis.element(pyxb.namespace.ExpandedName(Namespace, 'receive'), tReceive, scope=tSequence, location=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 71, 12)))

tSequence._AddElement(pyxb.binding.basis.element(pyxb.namespace.ExpandedName(Namespace, 'reply'), tReply, scope=tSequence, location=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 72, 12)))

tSequence._AddElement(pyxb.binding.basis.element(pyxb.namespace.ExpandedName(Namespace, 'assign'), tAssign, scope=tSequence, location=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 73, 12)))

tSequence._AddElement(pyxb.binding.basis.element(pyxb.namespace.ExpandedName(Namespace, 'wait'), tWait, scope=tSequence, location=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 74, 12)))

tSequence._AddElement(pyxb.binding.basis.element(pyxb.namespace.ExpandedName(Namespace, 'throw'), tThrow, scope=tSequence, location=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 75, 12)))

tSequence._AddElement(pyxb.binding.basis.element(pyxb.namespace.ExpandedName(Namespace, 'terminate'), tTerminate, scope=tSequence, location=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 76, 12)))

tSequence._AddElement(pyxb.binding.basis.element(pyxb.namespace.ExpandedName(Namespace, 'flow'), tFlow, scope=tSequence, location=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 77, 12)))

tSequence._AddElement(pyxb.binding.basis.element(pyxb.namespace.ExpandedName(Namespace, 'switch'), tSwitch, scope=tSequence, location=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 78, 12)))

tSequence._AddElement(pyxb.binding.basis.element(pyxb.namespace.ExpandedName(Namespace, 'while'), tWhile, scope=tSequence, location=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 79, 12)))

tSequence._AddElement(pyxb.binding.basis.element(pyxb.namespace.ExpandedName(Namespace, 'sequence'), tSequence, scope=tSequence, location=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 80, 12)))

tSequence._AddElement(pyxb.binding.basis.element(pyxb.namespace.ExpandedName(Namespace, 'pick'), tPick, scope=tSequence, location=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 81, 12)))

tSequence._AddElement(pyxb.binding.basis.element(pyxb.namespace.ExpandedName(Namespace, 'scope'), tScope, scope=tSequence, location=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 82, 12)))

def _BuildAutomaton_42 ():
    # Remove this helper function from the namespace after it is invoked
    global _BuildAutomaton_42
    del _BuildAutomaton_42
    import pyxb.utils.fac as fac

    counters = set()
    cc_0 = fac.CounterCondition(min=0, max=None, metadata=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 20, 12))
    counters.add(cc_0)
    cc_1 = fac.CounterCondition(min=0, max=None, metadata=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 290, 20))
    counters.add(cc_1)
    cc_2 = fac.CounterCondition(min=0, max=None, metadata=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 292, 20))
    counters.add(cc_2)
    states = []
    final_update = None
    symbol = pyxb.binding.content.WildcardUse(pyxb.binding.content.Wildcard(process_contents=pyxb.binding.content.Wildcard.PC_lax, namespace_constraint=(pyxb.binding.content.Wildcard.NC_not, 'http://schemas.xmlsoap.org/ws/2003/03/business-process/')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 20, 12))
    st_0 = fac.State(symbol, is_initial=True, final_update=final_update, is_unordered_catenation=False)
    states.append(st_0)
    final_update = None
    symbol = pyxb.binding.content.ElementUse(tSequence._UseForTag(pyxb.namespace.ExpandedName(Namespace, 'target')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 290, 20))
    st_1 = fac.State(symbol, is_initial=True, final_update=final_update, is_unordered_catenation=False)
    states.append(st_1)
    final_update = None
    symbol = pyxb.binding.content.ElementUse(tSequence._UseForTag(pyxb.namespace.ExpandedName(Namespace, 'source')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 292, 20))
    st_2 = fac.State(symbol, is_initial=True, final_update=final_update, is_unordered_catenation=False)
    states.append(st_2)
    final_update = set()
    symbol = pyxb.binding.content.ElementUse(tSequence._UseForTag(pyxb.namespace.ExpandedName(Namespace, 'empty')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 69, 12))
    st_3 = fac.State(symbol, is_initial=True, final_update=final_update, is_unordered_catenation=False)
    states.append(st_3)
    final_update = set()
    symbol = pyxb.binding.content.ElementUse(tSequence._UseForTag(pyxb.namespace.ExpandedName(Namespace, 'invoke')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 70, 12))
    st_4 = fac.State(symbol, is_initial=True, final_update=final_update, is_unordered_catenation=False)
    states.append(st_4)
    final_update = set()
    symbol = pyxb.binding.content.ElementUse(tSequence._UseForTag(pyxb.namespace.ExpandedName(Namespace, 'receive')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 71, 12))
    st_5 = fac.State(symbol, is_initial=True, final_update=final_update, is_unordered_catenation=False)
    states.append(st_5)
    final_update = set()
    symbol = pyxb.binding.content.ElementUse(tSequence._UseForTag(pyxb.namespace.ExpandedName(Namespace, 'reply')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 72, 12))
    st_6 = fac.State(symbol, is_initial=True, final_update=final_update, is_unordered_catenation=False)
    states.append(st_6)
    final_update = set()
    symbol = pyxb.binding.content.ElementUse(tSequence._UseForTag(pyxb.namespace.ExpandedName(Namespace, 'assign')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 73, 12))
    st_7 = fac.State(symbol, is_initial=True, final_update=final_update, is_unordered_catenation=False)
    states.append(st_7)
    final_update = set()
    symbol = pyxb.binding.content.ElementUse(tSequence._UseForTag(pyxb.namespace.ExpandedName(Namespace, 'wait')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 74, 12))
    st_8 = fac.State(symbol, is_initial=True, final_update=final_update, is_unordered_catenation=False)
    states.append(st_8)
    final_update = set()
    symbol = pyxb.binding.content.ElementUse(tSequence._UseForTag(pyxb.namespace.ExpandedName(Namespace, 'throw')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 75, 12))
    st_9 = fac.State(symbol, is_initial=True, final_update=final_update, is_unordered_catenation=False)
    states.append(st_9)
    final_update = set()
    symbol = pyxb.binding.content.ElementUse(tSequence._UseForTag(pyxb.namespace.ExpandedName(Namespace, 'terminate')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 76, 12))
    st_10 = fac.State(symbol, is_initial=True, final_update=final_update, is_unordered_catenation=False)
    states.append(st_10)
    final_update = set()
    symbol = pyxb.binding.content.ElementUse(tSequence._UseForTag(pyxb.namespace.ExpandedName(Namespace, 'flow')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 77, 12))
    st_11 = fac.State(symbol, is_initial=True, final_update=final_update, is_unordered_catenation=False)
    states.append(st_11)
    final_update = set()
    symbol = pyxb.binding.content.ElementUse(tSequence._UseForTag(pyxb.namespace.ExpandedName(Namespace, 'switch')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 78, 12))
    st_12 = fac.State(symbol, is_initial=True, final_update=final_update, is_unordered_catenation=False)
    states.append(st_12)
    final_update = set()
    symbol = pyxb.binding.content.ElementUse(tSequence._UseForTag(pyxb.namespace.ExpandedName(Namespace, 'while')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 79, 12))
    st_13 = fac.State(symbol, is_initial=True, final_update=final_update, is_unordered_catenation=False)
    states.append(st_13)
    final_update = set()
    symbol = pyxb.binding.content.ElementUse(tSequence._UseForTag(pyxb.namespace.ExpandedName(Namespace, 'sequence')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 80, 12))
    st_14 = fac.State(symbol, is_initial=True, final_update=final_update, is_unordered_catenation=False)
    states.append(st_14)
    final_update = set()
    symbol = pyxb.binding.content.ElementUse(tSequence._UseForTag(pyxb.namespace.ExpandedName(Namespace, 'pick')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 81, 12))
    st_15 = fac.State(symbol, is_initial=True, final_update=final_update, is_unordered_catenation=False)
    states.append(st_15)
    final_update = set()
    symbol = pyxb.binding.content.ElementUse(tSequence._UseForTag(pyxb.namespace.ExpandedName(Namespace, 'scope')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 82, 12))
    st_16 = fac.State(symbol, is_initial=True, final_update=final_update, is_unordered_catenation=False)
    states.append(st_16)
    transitions = []
    transitions.append(fac.Transition(st_0, [
        fac.UpdateInstruction(cc_0, True) ]))
    transitions.append(fac.Transition(st_1, [
        fac.UpdateInstruction(cc_0, False) ]))
    transitions.append(fac.Transition(st_2, [
        fac.UpdateInstruction(cc_0, False) ]))
    transitions.append(fac.Transition(st_3, [
        fac.UpdateInstruction(cc_0, False) ]))
    transitions.append(fac.Transition(st_4, [
        fac.UpdateInstruction(cc_0, False) ]))
    transitions.append(fac.Transition(st_5, [
        fac.UpdateInstruction(cc_0, False) ]))
    transitions.append(fac.Transition(st_6, [
        fac.UpdateInstruction(cc_0, False) ]))
    transitions.append(fac.Transition(st_7, [
        fac.UpdateInstruction(cc_0, False) ]))
    transitions.append(fac.Transition(st_8, [
        fac.UpdateInstruction(cc_0, False) ]))
    transitions.append(fac.Transition(st_9, [
        fac.UpdateInstruction(cc_0, False) ]))
    transitions.append(fac.Transition(st_10, [
        fac.UpdateInstruction(cc_0, False) ]))
    transitions.append(fac.Transition(st_11, [
        fac.UpdateInstruction(cc_0, False) ]))
    transitions.append(fac.Transition(st_12, [
        fac.UpdateInstruction(cc_0, False) ]))
    transitions.append(fac.Transition(st_13, [
        fac.UpdateInstruction(cc_0, False) ]))
    transitions.append(fac.Transition(st_14, [
        fac.UpdateInstruction(cc_0, False) ]))
    transitions.append(fac.Transition(st_15, [
        fac.UpdateInstruction(cc_0, False) ]))
    transitions.append(fac.Transition(st_16, [
        fac.UpdateInstruction(cc_0, False) ]))
    st_0._set_transitionSet(transitions)
    transitions = []
    transitions.append(fac.Transition(st_1, [
        fac.UpdateInstruction(cc_1, True) ]))
    transitions.append(fac.Transition(st_2, [
        fac.UpdateInstruction(cc_1, False) ]))
    transitions.append(fac.Transition(st_3, [
        fac.UpdateInstruction(cc_1, False) ]))
    transitions.append(fac.Transition(st_4, [
        fac.UpdateInstruction(cc_1, False) ]))
    transitions.append(fac.Transition(st_5, [
        fac.UpdateInstruction(cc_1, False) ]))
    transitions.append(fac.Transition(st_6, [
        fac.UpdateInstruction(cc_1, False) ]))
    transitions.append(fac.Transition(st_7, [
        fac.UpdateInstruction(cc_1, False) ]))
    transitions.append(fac.Transition(st_8, [
        fac.UpdateInstruction(cc_1, False) ]))
    transitions.append(fac.Transition(st_9, [
        fac.UpdateInstruction(cc_1, False) ]))
    transitions.append(fac.Transition(st_10, [
        fac.UpdateInstruction(cc_1, False) ]))
    transitions.append(fac.Transition(st_11, [
        fac.UpdateInstruction(cc_1, False) ]))
    transitions.append(fac.Transition(st_12, [
        fac.UpdateInstruction(cc_1, False) ]))
    transitions.append(fac.Transition(st_13, [
        fac.UpdateInstruction(cc_1, False) ]))
    transitions.append(fac.Transition(st_14, [
        fac.UpdateInstruction(cc_1, False) ]))
    transitions.append(fac.Transition(st_15, [
        fac.UpdateInstruction(cc_1, False) ]))
    transitions.append(fac.Transition(st_16, [
        fac.UpdateInstruction(cc_1, False) ]))
    st_1._set_transitionSet(transitions)
    transitions = []
    transitions.append(fac.Transition(st_2, [
        fac.UpdateInstruction(cc_2, True) ]))
    transitions.append(fac.Transition(st_3, [
        fac.UpdateInstruction(cc_2, False) ]))
    transitions.append(fac.Transition(st_4, [
        fac.UpdateInstruction(cc_2, False) ]))
    transitions.append(fac.Transition(st_5, [
        fac.UpdateInstruction(cc_2, False) ]))
    transitions.append(fac.Transition(st_6, [
        fac.UpdateInstruction(cc_2, False) ]))
    transitions.append(fac.Transition(st_7, [
        fac.UpdateInstruction(cc_2, False) ]))
    transitions.append(fac.Transition(st_8, [
        fac.UpdateInstruction(cc_2, False) ]))
    transitions.append(fac.Transition(st_9, [
        fac.UpdateInstruction(cc_2, False) ]))
    transitions.append(fac.Transition(st_10, [
        fac.UpdateInstruction(cc_2, False) ]))
    transitions.append(fac.Transition(st_11, [
        fac.UpdateInstruction(cc_2, False) ]))
    transitions.append(fac.Transition(st_12, [
        fac.UpdateInstruction(cc_2, False) ]))
    transitions.append(fac.Transition(st_13, [
        fac.UpdateInstruction(cc_2, False) ]))
    transitions.append(fac.Transition(st_14, [
        fac.UpdateInstruction(cc_2, False) ]))
    transitions.append(fac.Transition(st_15, [
        fac.UpdateInstruction(cc_2, False) ]))
    transitions.append(fac.Transition(st_16, [
        fac.UpdateInstruction(cc_2, False) ]))
    st_2._set_transitionSet(transitions)
    transitions = []
    transitions.append(fac.Transition(st_3, [
         ]))
    transitions.append(fac.Transition(st_4, [
         ]))
    transitions.append(fac.Transition(st_5, [
         ]))
    transitions.append(fac.Transition(st_6, [
         ]))
    transitions.append(fac.Transition(st_7, [
         ]))
    transitions.append(fac.Transition(st_8, [
         ]))
    transitions.append(fac.Transition(st_9, [
         ]))
    transitions.append(fac.Transition(st_10, [
         ]))
    transitions.append(fac.Transition(st_11, [
         ]))
    transitions.append(fac.Transition(st_12, [
         ]))
    transitions.append(fac.Transition(st_13, [
         ]))
    transitions.append(fac.Transition(st_14, [
         ]))
    transitions.append(fac.Transition(st_15, [
         ]))
    transitions.append(fac.Transition(st_16, [
         ]))
    st_3._set_transitionSet(transitions)
    transitions = []
    transitions.append(fac.Transition(st_3, [
         ]))
    transitions.append(fac.Transition(st_4, [
         ]))
    transitions.append(fac.Transition(st_5, [
         ]))
    transitions.append(fac.Transition(st_6, [
         ]))
    transitions.append(fac.Transition(st_7, [
         ]))
    transitions.append(fac.Transition(st_8, [
         ]))
    transitions.append(fac.Transition(st_9, [
         ]))
    transitions.append(fac.Transition(st_10, [
         ]))
    transitions.append(fac.Transition(st_11, [
         ]))
    transitions.append(fac.Transition(st_12, [
         ]))
    transitions.append(fac.Transition(st_13, [
         ]))
    transitions.append(fac.Transition(st_14, [
         ]))
    transitions.append(fac.Transition(st_15, [
         ]))
    transitions.append(fac.Transition(st_16, [
         ]))
    st_4._set_transitionSet(transitions)
    transitions = []
    transitions.append(fac.Transition(st_3, [
         ]))
    transitions.append(fac.Transition(st_4, [
         ]))
    transitions.append(fac.Transition(st_5, [
         ]))
    transitions.append(fac.Transition(st_6, [
         ]))
    transitions.append(fac.Transition(st_7, [
         ]))
    transitions.append(fac.Transition(st_8, [
         ]))
    transitions.append(fac.Transition(st_9, [
         ]))
    transitions.append(fac.Transition(st_10, [
         ]))
    transitions.append(fac.Transition(st_11, [
         ]))
    transitions.append(fac.Transition(st_12, [
         ]))
    transitions.append(fac.Transition(st_13, [
         ]))
    transitions.append(fac.Transition(st_14, [
         ]))
    transitions.append(fac.Transition(st_15, [
         ]))
    transitions.append(fac.Transition(st_16, [
         ]))
    st_5._set_transitionSet(transitions)
    transitions = []
    transitions.append(fac.Transition(st_3, [
         ]))
    transitions.append(fac.Transition(st_4, [
         ]))
    transitions.append(fac.Transition(st_5, [
         ]))
    transitions.append(fac.Transition(st_6, [
         ]))
    transitions.append(fac.Transition(st_7, [
         ]))
    transitions.append(fac.Transition(st_8, [
         ]))
    transitions.append(fac.Transition(st_9, [
         ]))
    transitions.append(fac.Transition(st_10, [
         ]))
    transitions.append(fac.Transition(st_11, [
         ]))
    transitions.append(fac.Transition(st_12, [
         ]))
    transitions.append(fac.Transition(st_13, [
         ]))
    transitions.append(fac.Transition(st_14, [
         ]))
    transitions.append(fac.Transition(st_15, [
         ]))
    transitions.append(fac.Transition(st_16, [
         ]))
    st_6._set_transitionSet(transitions)
    transitions = []
    transitions.append(fac.Transition(st_3, [
         ]))
    transitions.append(fac.Transition(st_4, [
         ]))
    transitions.append(fac.Transition(st_5, [
         ]))
    transitions.append(fac.Transition(st_6, [
         ]))
    transitions.append(fac.Transition(st_7, [
         ]))
    transitions.append(fac.Transition(st_8, [
         ]))
    transitions.append(fac.Transition(st_9, [
         ]))
    transitions.append(fac.Transition(st_10, [
         ]))
    transitions.append(fac.Transition(st_11, [
         ]))
    transitions.append(fac.Transition(st_12, [
         ]))
    transitions.append(fac.Transition(st_13, [
         ]))
    transitions.append(fac.Transition(st_14, [
         ]))
    transitions.append(fac.Transition(st_15, [
         ]))
    transitions.append(fac.Transition(st_16, [
         ]))
    st_7._set_transitionSet(transitions)
    transitions = []
    transitions.append(fac.Transition(st_3, [
         ]))
    transitions.append(fac.Transition(st_4, [
         ]))
    transitions.append(fac.Transition(st_5, [
         ]))
    transitions.append(fac.Transition(st_6, [
         ]))
    transitions.append(fac.Transition(st_7, [
         ]))
    transitions.append(fac.Transition(st_8, [
         ]))
    transitions.append(fac.Transition(st_9, [
         ]))
    transitions.append(fac.Transition(st_10, [
         ]))
    transitions.append(fac.Transition(st_11, [
         ]))
    transitions.append(fac.Transition(st_12, [
         ]))
    transitions.append(fac.Transition(st_13, [
         ]))
    transitions.append(fac.Transition(st_14, [
         ]))
    transitions.append(fac.Transition(st_15, [
         ]))
    transitions.append(fac.Transition(st_16, [
         ]))
    st_8._set_transitionSet(transitions)
    transitions = []
    transitions.append(fac.Transition(st_3, [
         ]))
    transitions.append(fac.Transition(st_4, [
         ]))
    transitions.append(fac.Transition(st_5, [
         ]))
    transitions.append(fac.Transition(st_6, [
         ]))
    transitions.append(fac.Transition(st_7, [
         ]))
    transitions.append(fac.Transition(st_8, [
         ]))
    transitions.append(fac.Transition(st_9, [
         ]))
    transitions.append(fac.Transition(st_10, [
         ]))
    transitions.append(fac.Transition(st_11, [
         ]))
    transitions.append(fac.Transition(st_12, [
         ]))
    transitions.append(fac.Transition(st_13, [
         ]))
    transitions.append(fac.Transition(st_14, [
         ]))
    transitions.append(fac.Transition(st_15, [
         ]))
    transitions.append(fac.Transition(st_16, [
         ]))
    st_9._set_transitionSet(transitions)
    transitions = []
    transitions.append(fac.Transition(st_3, [
         ]))
    transitions.append(fac.Transition(st_4, [
         ]))
    transitions.append(fac.Transition(st_5, [
         ]))
    transitions.append(fac.Transition(st_6, [
         ]))
    transitions.append(fac.Transition(st_7, [
         ]))
    transitions.append(fac.Transition(st_8, [
         ]))
    transitions.append(fac.Transition(st_9, [
         ]))
    transitions.append(fac.Transition(st_10, [
         ]))
    transitions.append(fac.Transition(st_11, [
         ]))
    transitions.append(fac.Transition(st_12, [
         ]))
    transitions.append(fac.Transition(st_13, [
         ]))
    transitions.append(fac.Transition(st_14, [
         ]))
    transitions.append(fac.Transition(st_15, [
         ]))
    transitions.append(fac.Transition(st_16, [
         ]))
    st_10._set_transitionSet(transitions)
    transitions = []
    transitions.append(fac.Transition(st_3, [
         ]))
    transitions.append(fac.Transition(st_4, [
         ]))
    transitions.append(fac.Transition(st_5, [
         ]))
    transitions.append(fac.Transition(st_6, [
         ]))
    transitions.append(fac.Transition(st_7, [
         ]))
    transitions.append(fac.Transition(st_8, [
         ]))
    transitions.append(fac.Transition(st_9, [
         ]))
    transitions.append(fac.Transition(st_10, [
         ]))
    transitions.append(fac.Transition(st_11, [
         ]))
    transitions.append(fac.Transition(st_12, [
         ]))
    transitions.append(fac.Transition(st_13, [
         ]))
    transitions.append(fac.Transition(st_14, [
         ]))
    transitions.append(fac.Transition(st_15, [
         ]))
    transitions.append(fac.Transition(st_16, [
         ]))
    st_11._set_transitionSet(transitions)
    transitions = []
    transitions.append(fac.Transition(st_3, [
         ]))
    transitions.append(fac.Transition(st_4, [
         ]))
    transitions.append(fac.Transition(st_5, [
         ]))
    transitions.append(fac.Transition(st_6, [
         ]))
    transitions.append(fac.Transition(st_7, [
         ]))
    transitions.append(fac.Transition(st_8, [
         ]))
    transitions.append(fac.Transition(st_9, [
         ]))
    transitions.append(fac.Transition(st_10, [
         ]))
    transitions.append(fac.Transition(st_11, [
         ]))
    transitions.append(fac.Transition(st_12, [
         ]))
    transitions.append(fac.Transition(st_13, [
         ]))
    transitions.append(fac.Transition(st_14, [
         ]))
    transitions.append(fac.Transition(st_15, [
         ]))
    transitions.append(fac.Transition(st_16, [
         ]))
    st_12._set_transitionSet(transitions)
    transitions = []
    transitions.append(fac.Transition(st_3, [
         ]))
    transitions.append(fac.Transition(st_4, [
         ]))
    transitions.append(fac.Transition(st_5, [
         ]))
    transitions.append(fac.Transition(st_6, [
         ]))
    transitions.append(fac.Transition(st_7, [
         ]))
    transitions.append(fac.Transition(st_8, [
         ]))
    transitions.append(fac.Transition(st_9, [
         ]))
    transitions.append(fac.Transition(st_10, [
         ]))
    transitions.append(fac.Transition(st_11, [
         ]))
    transitions.append(fac.Transition(st_12, [
         ]))
    transitions.append(fac.Transition(st_13, [
         ]))
    transitions.append(fac.Transition(st_14, [
         ]))
    transitions.append(fac.Transition(st_15, [
         ]))
    transitions.append(fac.Transition(st_16, [
         ]))
    st_13._set_transitionSet(transitions)
    transitions = []
    transitions.append(fac.Transition(st_3, [
         ]))
    transitions.append(fac.Transition(st_4, [
         ]))
    transitions.append(fac.Transition(st_5, [
         ]))
    transitions.append(fac.Transition(st_6, [
         ]))
    transitions.append(fac.Transition(st_7, [
         ]))
    transitions.append(fac.Transition(st_8, [
         ]))
    transitions.append(fac.Transition(st_9, [
         ]))
    transitions.append(fac.Transition(st_10, [
         ]))
    transitions.append(fac.Transition(st_11, [
         ]))
    transitions.append(fac.Transition(st_12, [
         ]))
    transitions.append(fac.Transition(st_13, [
         ]))
    transitions.append(fac.Transition(st_14, [
         ]))
    transitions.append(fac.Transition(st_15, [
         ]))
    transitions.append(fac.Transition(st_16, [
         ]))
    st_14._set_transitionSet(transitions)
    transitions = []
    transitions.append(fac.Transition(st_3, [
         ]))
    transitions.append(fac.Transition(st_4, [
         ]))
    transitions.append(fac.Transition(st_5, [
         ]))
    transitions.append(fac.Transition(st_6, [
         ]))
    transitions.append(fac.Transition(st_7, [
         ]))
    transitions.append(fac.Transition(st_8, [
         ]))
    transitions.append(fac.Transition(st_9, [
         ]))
    transitions.append(fac.Transition(st_10, [
         ]))
    transitions.append(fac.Transition(st_11, [
         ]))
    transitions.append(fac.Transition(st_12, [
         ]))
    transitions.append(fac.Transition(st_13, [
         ]))
    transitions.append(fac.Transition(st_14, [
         ]))
    transitions.append(fac.Transition(st_15, [
         ]))
    transitions.append(fac.Transition(st_16, [
         ]))
    st_15._set_transitionSet(transitions)
    transitions = []
    transitions.append(fac.Transition(st_3, [
         ]))
    transitions.append(fac.Transition(st_4, [
         ]))
    transitions.append(fac.Transition(st_5, [
         ]))
    transitions.append(fac.Transition(st_6, [
         ]))
    transitions.append(fac.Transition(st_7, [
         ]))
    transitions.append(fac.Transition(st_8, [
         ]))
    transitions.append(fac.Transition(st_9, [
         ]))
    transitions.append(fac.Transition(st_10, [
         ]))
    transitions.append(fac.Transition(st_11, [
         ]))
    transitions.append(fac.Transition(st_12, [
         ]))
    transitions.append(fac.Transition(st_13, [
         ]))
    transitions.append(fac.Transition(st_14, [
         ]))
    transitions.append(fac.Transition(st_15, [
         ]))
    transitions.append(fac.Transition(st_16, [
         ]))
    st_16._set_transitionSet(transitions)
    return fac.Automaton(states, counters, False, containing_state=None)
tSequence._Automaton = _BuildAutomaton_42()




tPick._AddElement(pyxb.binding.basis.element(pyxb.namespace.ExpandedName(Namespace, 'onMessage'), tOnMessage, scope=tPick, location=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 613, 20)))

tPick._AddElement(pyxb.binding.basis.element(pyxb.namespace.ExpandedName(Namespace, 'onAlarm'), tOnAlarm, scope=tPick, location=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 616, 20)))

def _BuildAutomaton_43 ():
    # Remove this helper function from the namespace after it is invoked
    global _BuildAutomaton_43
    del _BuildAutomaton_43
    import pyxb.utils.fac as fac

    counters = set()
    cc_0 = fac.CounterCondition(min=0, max=None, metadata=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 20, 12))
    counters.add(cc_0)
    cc_1 = fac.CounterCondition(min=0, max=None, metadata=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 290, 20))
    counters.add(cc_1)
    cc_2 = fac.CounterCondition(min=0, max=None, metadata=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 292, 20))
    counters.add(cc_2)
    cc_3 = fac.CounterCondition(min=0, max=None, metadata=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 616, 20))
    counters.add(cc_3)
    states = []
    final_update = None
    symbol = pyxb.binding.content.WildcardUse(pyxb.binding.content.Wildcard(process_contents=pyxb.binding.content.Wildcard.PC_lax, namespace_constraint=(pyxb.binding.content.Wildcard.NC_not, 'http://schemas.xmlsoap.org/ws/2003/03/business-process/')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 20, 12))
    st_0 = fac.State(symbol, is_initial=True, final_update=final_update, is_unordered_catenation=False)
    states.append(st_0)
    final_update = None
    symbol = pyxb.binding.content.ElementUse(tPick._UseForTag(pyxb.namespace.ExpandedName(Namespace, 'target')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 290, 20))
    st_1 = fac.State(symbol, is_initial=True, final_update=final_update, is_unordered_catenation=False)
    states.append(st_1)
    final_update = None
    symbol = pyxb.binding.content.ElementUse(tPick._UseForTag(pyxb.namespace.ExpandedName(Namespace, 'source')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 292, 20))
    st_2 = fac.State(symbol, is_initial=True, final_update=final_update, is_unordered_catenation=False)
    states.append(st_2)
    final_update = set()
    symbol = pyxb.binding.content.ElementUse(tPick._UseForTag(pyxb.namespace.ExpandedName(Namespace, 'onMessage')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 613, 20))
    st_3 = fac.State(symbol, is_initial=True, final_update=final_update, is_unordered_catenation=False)
    states.append(st_3)
    final_update = set()
    final_update.add(fac.UpdateInstruction(cc_3, False))
    symbol = pyxb.binding.content.ElementUse(tPick._UseForTag(pyxb.namespace.ExpandedName(Namespace, 'onAlarm')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 616, 20))
    st_4 = fac.State(symbol, is_initial=False, final_update=final_update, is_unordered_catenation=False)
    states.append(st_4)
    transitions = []
    transitions.append(fac.Transition(st_0, [
        fac.UpdateInstruction(cc_0, True) ]))
    transitions.append(fac.Transition(st_1, [
        fac.UpdateInstruction(cc_0, False) ]))
    transitions.append(fac.Transition(st_2, [
        fac.UpdateInstruction(cc_0, False) ]))
    transitions.append(fac.Transition(st_3, [
        fac.UpdateInstruction(cc_0, False) ]))
    st_0._set_transitionSet(transitions)
    transitions = []
    transitions.append(fac.Transition(st_1, [
        fac.UpdateInstruction(cc_1, True) ]))
    transitions.append(fac.Transition(st_2, [
        fac.UpdateInstruction(cc_1, False) ]))
    transitions.append(fac.Transition(st_3, [
        fac.UpdateInstruction(cc_1, False) ]))
    st_1._set_transitionSet(transitions)
    transitions = []
    transitions.append(fac.Transition(st_2, [
        fac.UpdateInstruction(cc_2, True) ]))
    transitions.append(fac.Transition(st_3, [
        fac.UpdateInstruction(cc_2, False) ]))
    st_2._set_transitionSet(transitions)
    transitions = []
    transitions.append(fac.Transition(st_3, [
         ]))
    transitions.append(fac.Transition(st_4, [
         ]))
    st_3._set_transitionSet(transitions)
    transitions = []
    transitions.append(fac.Transition(st_4, [
        fac.UpdateInstruction(cc_3, True) ]))
    st_4._set_transitionSet(transitions)
    return fac.Automaton(states, counters, False, containing_state=None)
tPick._Automaton = _BuildAutomaton_43()




tScope._AddElement(pyxb.binding.basis.element(pyxb.namespace.ExpandedName(Namespace, 'empty'), tEmpty, scope=tScope, location=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 69, 12)))

tScope._AddElement(pyxb.binding.basis.element(pyxb.namespace.ExpandedName(Namespace, 'invoke'), tInvoke, scope=tScope, location=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 70, 12)))

tScope._AddElement(pyxb.binding.basis.element(pyxb.namespace.ExpandedName(Namespace, 'receive'), tReceive, scope=tScope, location=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 71, 12)))

tScope._AddElement(pyxb.binding.basis.element(pyxb.namespace.ExpandedName(Namespace, 'reply'), tReply, scope=tScope, location=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 72, 12)))

tScope._AddElement(pyxb.binding.basis.element(pyxb.namespace.ExpandedName(Namespace, 'assign'), tAssign, scope=tScope, location=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 73, 12)))

tScope._AddElement(pyxb.binding.basis.element(pyxb.namespace.ExpandedName(Namespace, 'wait'), tWait, scope=tScope, location=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 74, 12)))

tScope._AddElement(pyxb.binding.basis.element(pyxb.namespace.ExpandedName(Namespace, 'throw'), tThrow, scope=tScope, location=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 75, 12)))

tScope._AddElement(pyxb.binding.basis.element(pyxb.namespace.ExpandedName(Namespace, 'terminate'), tTerminate, scope=tScope, location=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 76, 12)))

tScope._AddElement(pyxb.binding.basis.element(pyxb.namespace.ExpandedName(Namespace, 'flow'), tFlow, scope=tScope, location=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 77, 12)))

tScope._AddElement(pyxb.binding.basis.element(pyxb.namespace.ExpandedName(Namespace, 'switch'), tSwitch, scope=tScope, location=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 78, 12)))

tScope._AddElement(pyxb.binding.basis.element(pyxb.namespace.ExpandedName(Namespace, 'while'), tWhile, scope=tScope, location=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 79, 12)))

tScope._AddElement(pyxb.binding.basis.element(pyxb.namespace.ExpandedName(Namespace, 'sequence'), tSequence, scope=tScope, location=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 80, 12)))

tScope._AddElement(pyxb.binding.basis.element(pyxb.namespace.ExpandedName(Namespace, 'pick'), tPick, scope=tScope, location=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 81, 12)))

tScope._AddElement(pyxb.binding.basis.element(pyxb.namespace.ExpandedName(Namespace, 'scope'), tScope, scope=tScope, location=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 82, 12)))

tScope._AddElement(pyxb.binding.basis.element(pyxb.namespace.ExpandedName(Namespace, 'variables'), tVariables, scope=tScope, location=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 630, 20)))

tScope._AddElement(pyxb.binding.basis.element(pyxb.namespace.ExpandedName(Namespace, 'correlationSets'), tCorrelationSets, scope=tScope, location=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 633, 20)))

tScope._AddElement(pyxb.binding.basis.element(pyxb.namespace.ExpandedName(Namespace, 'faultHandlers'), tFaultHandlers, scope=tScope, location=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 636, 20)))

tScope._AddElement(pyxb.binding.basis.element(pyxb.namespace.ExpandedName(Namespace, 'compensationHandler'), tCompensationHandler, scope=tScope, location=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 639, 20)))

tScope._AddElement(pyxb.binding.basis.element(pyxb.namespace.ExpandedName(Namespace, 'eventHandlers'), tEventHandlers, scope=tScope, location=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 642, 20)))

def _BuildAutomaton_44 ():
    # Remove this helper function from the namespace after it is invoked
    global _BuildAutomaton_44
    del _BuildAutomaton_44
    import pyxb.utils.fac as fac

    counters = set()
    cc_0 = fac.CounterCondition(min=0, max=None, metadata=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 20, 12))
    counters.add(cc_0)
    cc_1 = fac.CounterCondition(min=0, max=None, metadata=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 290, 20))
    counters.add(cc_1)
    cc_2 = fac.CounterCondition(min=0, max=None, metadata=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 292, 20))
    counters.add(cc_2)
    cc_3 = fac.CounterCondition(min=0, max=1, metadata=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 630, 20))
    counters.add(cc_3)
    cc_4 = fac.CounterCondition(min=0, max=1, metadata=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 633, 20))
    counters.add(cc_4)
    cc_5 = fac.CounterCondition(min=0, max=1, metadata=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 636, 20))
    counters.add(cc_5)
    cc_6 = fac.CounterCondition(min=0, max=1, metadata=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 639, 20))
    counters.add(cc_6)
    cc_7 = fac.CounterCondition(min=0, max=1, metadata=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 642, 20))
    counters.add(cc_7)
    states = []
    final_update = None
    symbol = pyxb.binding.content.WildcardUse(pyxb.binding.content.Wildcard(process_contents=pyxb.binding.content.Wildcard.PC_lax, namespace_constraint=(pyxb.binding.content.Wildcard.NC_not, 'http://schemas.xmlsoap.org/ws/2003/03/business-process/')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 20, 12))
    st_0 = fac.State(symbol, is_initial=True, final_update=final_update, is_unordered_catenation=False)
    states.append(st_0)
    final_update = None
    symbol = pyxb.binding.content.ElementUse(tScope._UseForTag(pyxb.namespace.ExpandedName(Namespace, 'target')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 290, 20))
    st_1 = fac.State(symbol, is_initial=True, final_update=final_update, is_unordered_catenation=False)
    states.append(st_1)
    final_update = None
    symbol = pyxb.binding.content.ElementUse(tScope._UseForTag(pyxb.namespace.ExpandedName(Namespace, 'source')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 292, 20))
    st_2 = fac.State(symbol, is_initial=True, final_update=final_update, is_unordered_catenation=False)
    states.append(st_2)
    final_update = None
    symbol = pyxb.binding.content.ElementUse(tScope._UseForTag(pyxb.namespace.ExpandedName(Namespace, 'variables')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 630, 20))
    st_3 = fac.State(symbol, is_initial=True, final_update=final_update, is_unordered_catenation=False)
    states.append(st_3)
    final_update = None
    symbol = pyxb.binding.content.ElementUse(tScope._UseForTag(pyxb.namespace.ExpandedName(Namespace, 'correlationSets')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 633, 20))
    st_4 = fac.State(symbol, is_initial=True, final_update=final_update, is_unordered_catenation=False)
    states.append(st_4)
    final_update = None
    symbol = pyxb.binding.content.ElementUse(tScope._UseForTag(pyxb.namespace.ExpandedName(Namespace, 'faultHandlers')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 636, 20))
    st_5 = fac.State(symbol, is_initial=True, final_update=final_update, is_unordered_catenation=False)
    states.append(st_5)
    final_update = None
    symbol = pyxb.binding.content.ElementUse(tScope._UseForTag(pyxb.namespace.ExpandedName(Namespace, 'compensationHandler')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 639, 20))
    st_6 = fac.State(symbol, is_initial=True, final_update=final_update, is_unordered_catenation=False)
    states.append(st_6)
    final_update = None
    symbol = pyxb.binding.content.ElementUse(tScope._UseForTag(pyxb.namespace.ExpandedName(Namespace, 'eventHandlers')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 642, 20))
    st_7 = fac.State(symbol, is_initial=True, final_update=final_update, is_unordered_catenation=False)
    states.append(st_7)
    final_update = set()
    symbol = pyxb.binding.content.ElementUse(tScope._UseForTag(pyxb.namespace.ExpandedName(Namespace, 'empty')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 69, 12))
    st_8 = fac.State(symbol, is_initial=True, final_update=final_update, is_unordered_catenation=False)
    states.append(st_8)
    final_update = set()
    symbol = pyxb.binding.content.ElementUse(tScope._UseForTag(pyxb.namespace.ExpandedName(Namespace, 'invoke')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 70, 12))
    st_9 = fac.State(symbol, is_initial=True, final_update=final_update, is_unordered_catenation=False)
    states.append(st_9)
    final_update = set()
    symbol = pyxb.binding.content.ElementUse(tScope._UseForTag(pyxb.namespace.ExpandedName(Namespace, 'receive')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 71, 12))
    st_10 = fac.State(symbol, is_initial=True, final_update=final_update, is_unordered_catenation=False)
    states.append(st_10)
    final_update = set()
    symbol = pyxb.binding.content.ElementUse(tScope._UseForTag(pyxb.namespace.ExpandedName(Namespace, 'reply')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 72, 12))
    st_11 = fac.State(symbol, is_initial=True, final_update=final_update, is_unordered_catenation=False)
    states.append(st_11)
    final_update = set()
    symbol = pyxb.binding.content.ElementUse(tScope._UseForTag(pyxb.namespace.ExpandedName(Namespace, 'assign')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 73, 12))
    st_12 = fac.State(symbol, is_initial=True, final_update=final_update, is_unordered_catenation=False)
    states.append(st_12)
    final_update = set()
    symbol = pyxb.binding.content.ElementUse(tScope._UseForTag(pyxb.namespace.ExpandedName(Namespace, 'wait')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 74, 12))
    st_13 = fac.State(symbol, is_initial=True, final_update=final_update, is_unordered_catenation=False)
    states.append(st_13)
    final_update = set()
    symbol = pyxb.binding.content.ElementUse(tScope._UseForTag(pyxb.namespace.ExpandedName(Namespace, 'throw')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 75, 12))
    st_14 = fac.State(symbol, is_initial=True, final_update=final_update, is_unordered_catenation=False)
    states.append(st_14)
    final_update = set()
    symbol = pyxb.binding.content.ElementUse(tScope._UseForTag(pyxb.namespace.ExpandedName(Namespace, 'terminate')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 76, 12))
    st_15 = fac.State(symbol, is_initial=True, final_update=final_update, is_unordered_catenation=False)
    states.append(st_15)
    final_update = set()
    symbol = pyxb.binding.content.ElementUse(tScope._UseForTag(pyxb.namespace.ExpandedName(Namespace, 'flow')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 77, 12))
    st_16 = fac.State(symbol, is_initial=True, final_update=final_update, is_unordered_catenation=False)
    states.append(st_16)
    final_update = set()
    symbol = pyxb.binding.content.ElementUse(tScope._UseForTag(pyxb.namespace.ExpandedName(Namespace, 'switch')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 78, 12))
    st_17 = fac.State(symbol, is_initial=True, final_update=final_update, is_unordered_catenation=False)
    states.append(st_17)
    final_update = set()
    symbol = pyxb.binding.content.ElementUse(tScope._UseForTag(pyxb.namespace.ExpandedName(Namespace, 'while')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 79, 12))
    st_18 = fac.State(symbol, is_initial=True, final_update=final_update, is_unordered_catenation=False)
    states.append(st_18)
    final_update = set()
    symbol = pyxb.binding.content.ElementUse(tScope._UseForTag(pyxb.namespace.ExpandedName(Namespace, 'sequence')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 80, 12))
    st_19 = fac.State(symbol, is_initial=True, final_update=final_update, is_unordered_catenation=False)
    states.append(st_19)
    final_update = set()
    symbol = pyxb.binding.content.ElementUse(tScope._UseForTag(pyxb.namespace.ExpandedName(Namespace, 'pick')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 81, 12))
    st_20 = fac.State(symbol, is_initial=True, final_update=final_update, is_unordered_catenation=False)
    states.append(st_20)
    final_update = set()
    symbol = pyxb.binding.content.ElementUse(tScope._UseForTag(pyxb.namespace.ExpandedName(Namespace, 'scope')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/bpws.xsd', 82, 12))
    st_21 = fac.State(symbol, is_initial=True, final_update=final_update, is_unordered_catenation=False)
    states.append(st_21)
    transitions = []
    transitions.append(fac.Transition(st_0, [
        fac.UpdateInstruction(cc_0, True) ]))
    transitions.append(fac.Transition(st_1, [
        fac.UpdateInstruction(cc_0, False) ]))
    transitions.append(fac.Transition(st_2, [
        fac.UpdateInstruction(cc_0, False) ]))
    transitions.append(fac.Transition(st_3, [
        fac.UpdateInstruction(cc_0, False) ]))
    transitions.append(fac.Transition(st_4, [
        fac.UpdateInstruction(cc_0, False) ]))
    transitions.append(fac.Transition(st_5, [
        fac.UpdateInstruction(cc_0, False) ]))
    transitions.append(fac.Transition(st_6, [
        fac.UpdateInstruction(cc_0, False) ]))
    transitions.append(fac.Transition(st_7, [
        fac.UpdateInstruction(cc_0, False) ]))
    transitions.append(fac.Transition(st_8, [
        fac.UpdateInstruction(cc_0, False) ]))
    transitions.append(fac.Transition(st_9, [
        fac.UpdateInstruction(cc_0, False) ]))
    transitions.append(fac.Transition(st_10, [
        fac.UpdateInstruction(cc_0, False) ]))
    transitions.append(fac.Transition(st_11, [
        fac.UpdateInstruction(cc_0, False) ]))
    transitions.append(fac.Transition(st_12, [
        fac.UpdateInstruction(cc_0, False) ]))
    transitions.append(fac.Transition(st_13, [
        fac.UpdateInstruction(cc_0, False) ]))
    transitions.append(fac.Transition(st_14, [
        fac.UpdateInstruction(cc_0, False) ]))
    transitions.append(fac.Transition(st_15, [
        fac.UpdateInstruction(cc_0, False) ]))
    transitions.append(fac.Transition(st_16, [
        fac.UpdateInstruction(cc_0, False) ]))
    transitions.append(fac.Transition(st_17, [
        fac.UpdateInstruction(cc_0, False) ]))
    transitions.append(fac.Transition(st_18, [
        fac.UpdateInstruction(cc_0, False) ]))
    transitions.append(fac.Transition(st_19, [
        fac.UpdateInstruction(cc_0, False) ]))
    transitions.append(fac.Transition(st_20, [
        fac.UpdateInstruction(cc_0, False) ]))
    transitions.append(fac.Transition(st_21, [
        fac.UpdateInstruction(cc_0, False) ]))
    st_0._set_transitionSet(transitions)
    transitions = []
    transitions.append(fac.Transition(st_1, [
        fac.UpdateInstruction(cc_1, True) ]))
    transitions.append(fac.Transition(st_2, [
        fac.UpdateInstruction(cc_1, False) ]))
    transitions.append(fac.Transition(st_3, [
        fac.UpdateInstruction(cc_1, False) ]))
    transitions.append(fac.Transition(st_4, [
        fac.UpdateInstruction(cc_1, False) ]))
    transitions.append(fac.Transition(st_5, [
        fac.UpdateInstruction(cc_1, False) ]))
    transitions.append(fac.Transition(st_6, [
        fac.UpdateInstruction(cc_1, False) ]))
    transitions.append(fac.Transition(st_7, [
        fac.UpdateInstruction(cc_1, False) ]))
    transitions.append(fac.Transition(st_8, [
        fac.UpdateInstruction(cc_1, False) ]))
    transitions.append(fac.Transition(st_9, [
        fac.UpdateInstruction(cc_1, False) ]))
    transitions.append(fac.Transition(st_10, [
        fac.UpdateInstruction(cc_1, False) ]))
    transitions.append(fac.Transition(st_11, [
        fac.UpdateInstruction(cc_1, False) ]))
    transitions.append(fac.Transition(st_12, [
        fac.UpdateInstruction(cc_1, False) ]))
    transitions.append(fac.Transition(st_13, [
        fac.UpdateInstruction(cc_1, False) ]))
    transitions.append(fac.Transition(st_14, [
        fac.UpdateInstruction(cc_1, False) ]))
    transitions.append(fac.Transition(st_15, [
        fac.UpdateInstruction(cc_1, False) ]))
    transitions.append(fac.Transition(st_16, [
        fac.UpdateInstruction(cc_1, False) ]))
    transitions.append(fac.Transition(st_17, [
        fac.UpdateInstruction(cc_1, False) ]))
    transitions.append(fac.Transition(st_18, [
        fac.UpdateInstruction(cc_1, False) ]))
    transitions.append(fac.Transition(st_19, [
        fac.UpdateInstruction(cc_1, False) ]))
    transitions.append(fac.Transition(st_20, [
        fac.UpdateInstruction(cc_1, False) ]))
    transitions.append(fac.Transition(st_21, [
        fac.UpdateInstruction(cc_1, False) ]))
    st_1._set_transitionSet(transitions)
    transitions = []
    transitions.append(fac.Transition(st_2, [
        fac.UpdateInstruction(cc_2, True) ]))
    transitions.append(fac.Transition(st_3, [
        fac.UpdateInstruction(cc_2, False) ]))
    transitions.append(fac.Transition(st_4, [
        fac.UpdateInstruction(cc_2, False) ]))
    transitions.append(fac.Transition(st_5, [
        fac.UpdateInstruction(cc_2, False) ]))
    transitions.append(fac.Transition(st_6, [
        fac.UpdateInstruction(cc_2, False) ]))
    transitions.append(fac.Transition(st_7, [
        fac.UpdateInstruction(cc_2, False) ]))
    transitions.append(fac.Transition(st_8, [
        fac.UpdateInstruction(cc_2, False) ]))
    transitions.append(fac.Transition(st_9, [
        fac.UpdateInstruction(cc_2, False) ]))
    transitions.append(fac.Transition(st_10, [
        fac.UpdateInstruction(cc_2, False) ]))
    transitions.append(fac.Transition(st_11, [
        fac.UpdateInstruction(cc_2, False) ]))
    transitions.append(fac.Transition(st_12, [
        fac.UpdateInstruction(cc_2, False) ]))
    transitions.append(fac.Transition(st_13, [
        fac.UpdateInstruction(cc_2, False) ]))
    transitions.append(fac.Transition(st_14, [
        fac.UpdateInstruction(cc_2, False) ]))
    transitions.append(fac.Transition(st_15, [
        fac.UpdateInstruction(cc_2, False) ]))
    transitions.append(fac.Transition(st_16, [
        fac.UpdateInstruction(cc_2, False) ]))
    transitions.append(fac.Transition(st_17, [
        fac.UpdateInstruction(cc_2, False) ]))
    transitions.append(fac.Transition(st_18, [
        fac.UpdateInstruction(cc_2, False) ]))
    transitions.append(fac.Transition(st_19, [
        fac.UpdateInstruction(cc_2, False) ]))
    transitions.append(fac.Transition(st_20, [
        fac.UpdateInstruction(cc_2, False) ]))
    transitions.append(fac.Transition(st_21, [
        fac.UpdateInstruction(cc_2, False) ]))
    st_2._set_transitionSet(transitions)
    transitions = []
    transitions.append(fac.Transition(st_3, [
        fac.UpdateInstruction(cc_3, True) ]))
    transitions.append(fac.Transition(st_4, [
        fac.UpdateInstruction(cc_3, False) ]))
    transitions.append(fac.Transition(st_5, [
        fac.UpdateInstruction(cc_3, False) ]))
    transitions.append(fac.Transition(st_6, [
        fac.UpdateInstruction(cc_3, False) ]))
    transitions.append(fac.Transition(st_7, [
        fac.UpdateInstruction(cc_3, False) ]))
    transitions.append(fac.Transition(st_8, [
        fac.UpdateInstruction(cc_3, False) ]))
    transitions.append(fac.Transition(st_9, [
        fac.UpdateInstruction(cc_3, False) ]))
    transitions.append(fac.Transition(st_10, [
        fac.UpdateInstruction(cc_3, False) ]))
    transitions.append(fac.Transition(st_11, [
        fac.UpdateInstruction(cc_3, False) ]))
    transitions.append(fac.Transition(st_12, [
        fac.UpdateInstruction(cc_3, False) ]))
    transitions.append(fac.Transition(st_13, [
        fac.UpdateInstruction(cc_3, False) ]))
    transitions.append(fac.Transition(st_14, [
        fac.UpdateInstruction(cc_3, False) ]))
    transitions.append(fac.Transition(st_15, [
        fac.UpdateInstruction(cc_3, False) ]))
    transitions.append(fac.Transition(st_16, [
        fac.UpdateInstruction(cc_3, False) ]))
    transitions.append(fac.Transition(st_17, [
        fac.UpdateInstruction(cc_3, False) ]))
    transitions.append(fac.Transition(st_18, [
        fac.UpdateInstruction(cc_3, False) ]))
    transitions.append(fac.Transition(st_19, [
        fac.UpdateInstruction(cc_3, False) ]))
    transitions.append(fac.Transition(st_20, [
        fac.UpdateInstruction(cc_3, False) ]))
    transitions.append(fac.Transition(st_21, [
        fac.UpdateInstruction(cc_3, False) ]))
    st_3._set_transitionSet(transitions)
    transitions = []
    transitions.append(fac.Transition(st_4, [
        fac.UpdateInstruction(cc_4, True) ]))
    transitions.append(fac.Transition(st_5, [
        fac.UpdateInstruction(cc_4, False) ]))
    transitions.append(fac.Transition(st_6, [
        fac.UpdateInstruction(cc_4, False) ]))
    transitions.append(fac.Transition(st_7, [
        fac.UpdateInstruction(cc_4, False) ]))
    transitions.append(fac.Transition(st_8, [
        fac.UpdateInstruction(cc_4, False) ]))
    transitions.append(fac.Transition(st_9, [
        fac.UpdateInstruction(cc_4, False) ]))
    transitions.append(fac.Transition(st_10, [
        fac.UpdateInstruction(cc_4, False) ]))
    transitions.append(fac.Transition(st_11, [
        fac.UpdateInstruction(cc_4, False) ]))
    transitions.append(fac.Transition(st_12, [
        fac.UpdateInstruction(cc_4, False) ]))
    transitions.append(fac.Transition(st_13, [
        fac.UpdateInstruction(cc_4, False) ]))
    transitions.append(fac.Transition(st_14, [
        fac.UpdateInstruction(cc_4, False) ]))
    transitions.append(fac.Transition(st_15, [
        fac.UpdateInstruction(cc_4, False) ]))
    transitions.append(fac.Transition(st_16, [
        fac.UpdateInstruction(cc_4, False) ]))
    transitions.append(fac.Transition(st_17, [
        fac.UpdateInstruction(cc_4, False) ]))
    transitions.append(fac.Transition(st_18, [
        fac.UpdateInstruction(cc_4, False) ]))
    transitions.append(fac.Transition(st_19, [
        fac.UpdateInstruction(cc_4, False) ]))
    transitions.append(fac.Transition(st_20, [
        fac.UpdateInstruction(cc_4, False) ]))
    transitions.append(fac.Transition(st_21, [
        fac.UpdateInstruction(cc_4, False) ]))
    st_4._set_transitionSet(transitions)
    transitions = []
    transitions.append(fac.Transition(st_5, [
        fac.UpdateInstruction(cc_5, True) ]))
    transitions.append(fac.Transition(st_6, [
        fac.UpdateInstruction(cc_5, False) ]))
    transitions.append(fac.Transition(st_7, [
        fac.UpdateInstruction(cc_5, False) ]))
    transitions.append(fac.Transition(st_8, [
        fac.UpdateInstruction(cc_5, False) ]))
    transitions.append(fac.Transition(st_9, [
        fac.UpdateInstruction(cc_5, False) ]))
    transitions.append(fac.Transition(st_10, [
        fac.UpdateInstruction(cc_5, False) ]))
    transitions.append(fac.Transition(st_11, [
        fac.UpdateInstruction(cc_5, False) ]))
    transitions.append(fac.Transition(st_12, [
        fac.UpdateInstruction(cc_5, False) ]))
    transitions.append(fac.Transition(st_13, [
        fac.UpdateInstruction(cc_5, False) ]))
    transitions.append(fac.Transition(st_14, [
        fac.UpdateInstruction(cc_5, False) ]))
    transitions.append(fac.Transition(st_15, [
        fac.UpdateInstruction(cc_5, False) ]))
    transitions.append(fac.Transition(st_16, [
        fac.UpdateInstruction(cc_5, False) ]))
    transitions.append(fac.Transition(st_17, [
        fac.UpdateInstruction(cc_5, False) ]))
    transitions.append(fac.Transition(st_18, [
        fac.UpdateInstruction(cc_5, False) ]))
    transitions.append(fac.Transition(st_19, [
        fac.UpdateInstruction(cc_5, False) ]))
    transitions.append(fac.Transition(st_20, [
        fac.UpdateInstruction(cc_5, False) ]))
    transitions.append(fac.Transition(st_21, [
        fac.UpdateInstruction(cc_5, False) ]))
    st_5._set_transitionSet(transitions)
    transitions = []
    transitions.append(fac.Transition(st_6, [
        fac.UpdateInstruction(cc_6, True) ]))
    transitions.append(fac.Transition(st_7, [
        fac.UpdateInstruction(cc_6, False) ]))
    transitions.append(fac.Transition(st_8, [
        fac.UpdateInstruction(cc_6, False) ]))
    transitions.append(fac.Transition(st_9, [
        fac.UpdateInstruction(cc_6, False) ]))
    transitions.append(fac.Transition(st_10, [
        fac.UpdateInstruction(cc_6, False) ]))
    transitions.append(fac.Transition(st_11, [
        fac.UpdateInstruction(cc_6, False) ]))
    transitions.append(fac.Transition(st_12, [
        fac.UpdateInstruction(cc_6, False) ]))
    transitions.append(fac.Transition(st_13, [
        fac.UpdateInstruction(cc_6, False) ]))
    transitions.append(fac.Transition(st_14, [
        fac.UpdateInstruction(cc_6, False) ]))
    transitions.append(fac.Transition(st_15, [
        fac.UpdateInstruction(cc_6, False) ]))
    transitions.append(fac.Transition(st_16, [
        fac.UpdateInstruction(cc_6, False) ]))
    transitions.append(fac.Transition(st_17, [
        fac.UpdateInstruction(cc_6, False) ]))
    transitions.append(fac.Transition(st_18, [
        fac.UpdateInstruction(cc_6, False) ]))
    transitions.append(fac.Transition(st_19, [
        fac.UpdateInstruction(cc_6, False) ]))
    transitions.append(fac.Transition(st_20, [
        fac.UpdateInstruction(cc_6, False) ]))
    transitions.append(fac.Transition(st_21, [
        fac.UpdateInstruction(cc_6, False) ]))
    st_6._set_transitionSet(transitions)
    transitions = []
    transitions.append(fac.Transition(st_7, [
        fac.UpdateInstruction(cc_7, True) ]))
    transitions.append(fac.Transition(st_8, [
        fac.UpdateInstruction(cc_7, False) ]))
    transitions.append(fac.Transition(st_9, [
        fac.UpdateInstruction(cc_7, False) ]))
    transitions.append(fac.Transition(st_10, [
        fac.UpdateInstruction(cc_7, False) ]))
    transitions.append(fac.Transition(st_11, [
        fac.UpdateInstruction(cc_7, False) ]))
    transitions.append(fac.Transition(st_12, [
        fac.UpdateInstruction(cc_7, False) ]))
    transitions.append(fac.Transition(st_13, [
        fac.UpdateInstruction(cc_7, False) ]))
    transitions.append(fac.Transition(st_14, [
        fac.UpdateInstruction(cc_7, False) ]))
    transitions.append(fac.Transition(st_15, [
        fac.UpdateInstruction(cc_7, False) ]))
    transitions.append(fac.Transition(st_16, [
        fac.UpdateInstruction(cc_7, False) ]))
    transitions.append(fac.Transition(st_17, [
        fac.UpdateInstruction(cc_7, False) ]))
    transitions.append(fac.Transition(st_18, [
        fac.UpdateInstruction(cc_7, False) ]))
    transitions.append(fac.Transition(st_19, [
        fac.UpdateInstruction(cc_7, False) ]))
    transitions.append(fac.Transition(st_20, [
        fac.UpdateInstruction(cc_7, False) ]))
    transitions.append(fac.Transition(st_21, [
        fac.UpdateInstruction(cc_7, False) ]))
    st_7._set_transitionSet(transitions)
    transitions = []
    st_8._set_transitionSet(transitions)
    transitions = []
    st_9._set_transitionSet(transitions)
    transitions = []
    st_10._set_transitionSet(transitions)
    transitions = []
    st_11._set_transitionSet(transitions)
    transitions = []
    st_12._set_transitionSet(transitions)
    transitions = []
    st_13._set_transitionSet(transitions)
    transitions = []
    st_14._set_transitionSet(transitions)
    transitions = []
    st_15._set_transitionSet(transitions)
    transitions = []
    st_16._set_transitionSet(transitions)
    transitions = []
    st_17._set_transitionSet(transitions)
    transitions = []
    st_18._set_transitionSet(transitions)
    transitions = []
    st_19._set_transitionSet(transitions)
    transitions = []
    st_20._set_transitionSet(transitions)
    transitions = []
    st_21._set_transitionSet(transitions)
    return fac.Automaton(states, counters, False, containing_state=None)
tScope._Automaton = _BuildAutomaton_44()

