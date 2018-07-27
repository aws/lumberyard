# ./pyxb/bundles/wssplat/raw/wscoor.py
# -*- coding: utf-8 -*-
# PyXB bindings for NM:53926fc38ff5e8ef8d845111c1f3663a73eea53c
# Generated 2014-10-19 06:24:59.462555 by PyXB version 1.2.4 using Python 2.7.3.final.0
# Namespace http://docs.oasis-open.org/ws-tx/wscoor/2006/06

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
_GenerationUID = pyxb.utils.utility.UniqueIdentifier('urn:uuid:8f96bece-5782-11e4-b2ff-c8600024e903')

# Version of PyXB used to generate the bindings
_PyXBVersion = '1.2.4'
# Generated bindings are not compatible across PyXB versions
if pyxb.__version__ != _PyXBVersion:
    raise pyxb.PyXBVersionError(_PyXBVersion)

# Import bindings for namespaces imported into schema
import pyxb.bundles.wssplat.wsa
import pyxb.binding.datatypes

# NOTE: All namespace declarations are reserved within the binding
Namespace = pyxb.namespace.NamespaceForURI('http://docs.oasis-open.org/ws-tx/wscoor/2006/06', create_if_missing=True)
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


# Atomic simple type: {http://docs.oasis-open.org/ws-tx/wscoor/2006/06}ErrorCodes
class ErrorCodes (pyxb.binding.datatypes.QName, pyxb.binding.basis.enumeration_mixin):

    """An atomic simple type."""

    _ExpandedName = pyxb.namespace.ExpandedName(Namespace, 'ErrorCodes')
    _XSDLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wscoor.xsd', 87, 2)
    _Documentation = None
ErrorCodes._CF_enumeration = pyxb.binding.facets.CF_enumeration(value_datatype=ErrorCodes, enum_prefix=None)
ErrorCodes._CF_enumeration.addEnumeration(value=pyxb.namespace.ExpandedName(Namespace, 'InvalidParameters'), tag=None)
ErrorCodes._CF_enumeration.addEnumeration(value=pyxb.namespace.ExpandedName(Namespace, 'InvalidProtocol'), tag=None)
ErrorCodes._CF_enumeration.addEnumeration(value=pyxb.namespace.ExpandedName(Namespace, 'InvalidState'), tag=None)
ErrorCodes._CF_enumeration.addEnumeration(value=pyxb.namespace.ExpandedName(Namespace, 'CannotCreateContext'), tag=None)
ErrorCodes._CF_enumeration.addEnumeration(value=pyxb.namespace.ExpandedName(Namespace, 'CannotRegisterParticipant'), tag=None)
ErrorCodes._InitializeFacetMap(ErrorCodes._CF_enumeration)
Namespace.addCategoryObject('typeBinding', 'ErrorCodes', ErrorCodes)

# Complex type [anonymous] with content type SIMPLE
class CTD_ANON (pyxb.binding.basis.complexTypeDefinition):
    """Complex type [anonymous] with content type SIMPLE"""
    _TypeDefinition = pyxb.binding.datatypes.unsignedInt
    _ContentTypeTag = pyxb.binding.basis.complexTypeDefinition._CT_SIMPLE
    _Abstract = False
    _ExpandedName = None
    _XSDLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wscoor.xsd', 6, 4)
    _ElementMap = {}
    _AttributeMap = {}
    # Base type is pyxb.binding.datatypes.unsignedInt
    _AttributeWildcard = pyxb.binding.content.Wildcard(process_contents=pyxb.binding.content.Wildcard.PC_strict, namespace_constraint=(pyxb.binding.content.Wildcard.NC_not, 'http://docs.oasis-open.org/ws-tx/wscoor/2006/06'))
    _ElementMap.update({
        
    })
    _AttributeMap.update({
        
    })



# Complex type {http://docs.oasis-open.org/ws-tx/wscoor/2006/06}CoordinationContextType with content type ELEMENT_ONLY
class CoordinationContextType (pyxb.binding.basis.complexTypeDefinition):
    """Complex type {http://docs.oasis-open.org/ws-tx/wscoor/2006/06}CoordinationContextType with content type ELEMENT_ONLY"""
    _TypeDefinition = None
    _ContentTypeTag = pyxb.binding.basis.complexTypeDefinition._CT_ELEMENT_ONLY
    _Abstract = False
    _ExpandedName = pyxb.namespace.ExpandedName(Namespace, 'CoordinationContextType')
    _XSDLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wscoor.xsd', 14, 2)
    _ElementMap = {}
    _AttributeMap = {}
    # Base type is pyxb.binding.datatypes.anyType
    
    # Element {http://docs.oasis-open.org/ws-tx/wscoor/2006/06}Expires uses Python identifier Expires
    __Expires = pyxb.binding.content.ElementDeclaration(pyxb.namespace.ExpandedName(Namespace, 'Expires'), 'Expires', '__httpdocs_oasis_open_orgws_txwscoor200606_CoordinationContextType_httpdocs_oasis_open_orgws_txwscoor200606Expires', False, pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wscoor.xsd', 5, 2), )

    
    Expires = property(__Expires.value, __Expires.set, None, None)

    
    # Element {http://docs.oasis-open.org/ws-tx/wscoor/2006/06}Identifier uses Python identifier Identifier
    __Identifier = pyxb.binding.content.ElementDeclaration(pyxb.namespace.ExpandedName(Namespace, 'Identifier'), 'Identifier', '__httpdocs_oasis_open_orgws_txwscoor200606_CoordinationContextType_httpdocs_oasis_open_orgws_txwscoor200606Identifier', False, pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wscoor.xsd', 16, 6), )

    
    Identifier = property(__Identifier.value, __Identifier.set, None, None)

    
    # Element {http://docs.oasis-open.org/ws-tx/wscoor/2006/06}CoordinationType uses Python identifier CoordinationType
    __CoordinationType = pyxb.binding.content.ElementDeclaration(pyxb.namespace.ExpandedName(Namespace, 'CoordinationType'), 'CoordinationType', '__httpdocs_oasis_open_orgws_txwscoor200606_CoordinationContextType_httpdocs_oasis_open_orgws_txwscoor200606CoordinationType', False, pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wscoor.xsd', 26, 6), )

    
    CoordinationType = property(__CoordinationType.value, __CoordinationType.set, None, None)

    
    # Element {http://docs.oasis-open.org/ws-tx/wscoor/2006/06}RegistrationService uses Python identifier RegistrationService
    __RegistrationService = pyxb.binding.content.ElementDeclaration(pyxb.namespace.ExpandedName(Namespace, 'RegistrationService'), 'RegistrationService', '__httpdocs_oasis_open_orgws_txwscoor200606_CoordinationContextType_httpdocs_oasis_open_orgws_txwscoor200606RegistrationService', False, pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wscoor.xsd', 27, 6), )

    
    RegistrationService = property(__RegistrationService.value, __RegistrationService.set, None, None)

    _AttributeWildcard = pyxb.binding.content.Wildcard(process_contents=pyxb.binding.content.Wildcard.PC_lax, namespace_constraint=(pyxb.binding.content.Wildcard.NC_not, 'http://docs.oasis-open.org/ws-tx/wscoor/2006/06'))
    _ElementMap.update({
        __Expires.name() : __Expires,
        __Identifier.name() : __Identifier,
        __CoordinationType.name() : __CoordinationType,
        __RegistrationService.name() : __RegistrationService
    })
    _AttributeMap.update({
        
    })
Namespace.addCategoryObject('typeBinding', 'CoordinationContextType', CoordinationContextType)


# Complex type [anonymous] with content type SIMPLE
class CTD_ANON_ (pyxb.binding.basis.complexTypeDefinition):
    """Complex type [anonymous] with content type SIMPLE"""
    _TypeDefinition = pyxb.binding.datatypes.anyURI
    _ContentTypeTag = pyxb.binding.basis.complexTypeDefinition._CT_SIMPLE
    _Abstract = False
    _ExpandedName = None
    _XSDLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wscoor.xsd', 17, 8)
    _ElementMap = {}
    _AttributeMap = {}
    # Base type is pyxb.binding.datatypes.anyURI
    _AttributeWildcard = pyxb.binding.content.Wildcard(process_contents=pyxb.binding.content.Wildcard.PC_strict, namespace_constraint=(pyxb.binding.content.Wildcard.NC_not, 'http://docs.oasis-open.org/ws-tx/wscoor/2006/06'))
    _ElementMap.update({
        
    })
    _AttributeMap.update({
        
    })



# Complex type {http://docs.oasis-open.org/ws-tx/wscoor/2006/06}CreateCoordinationContextType with content type ELEMENT_ONLY
class CreateCoordinationContextType (pyxb.binding.basis.complexTypeDefinition):
    """Complex type {http://docs.oasis-open.org/ws-tx/wscoor/2006/06}CreateCoordinationContextType with content type ELEMENT_ONLY"""
    _TypeDefinition = None
    _ContentTypeTag = pyxb.binding.basis.complexTypeDefinition._CT_ELEMENT_ONLY
    _Abstract = False
    _ExpandedName = pyxb.namespace.ExpandedName(Namespace, 'CreateCoordinationContextType')
    _XSDLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wscoor.xsd', 42, 2)
    _ElementMap = {}
    _AttributeMap = {}
    # Base type is pyxb.binding.datatypes.anyType
    
    # Element {http://docs.oasis-open.org/ws-tx/wscoor/2006/06}Expires uses Python identifier Expires
    __Expires = pyxb.binding.content.ElementDeclaration(pyxb.namespace.ExpandedName(Namespace, 'Expires'), 'Expires', '__httpdocs_oasis_open_orgws_txwscoor200606_CreateCoordinationContextType_httpdocs_oasis_open_orgws_txwscoor200606Expires', False, pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wscoor.xsd', 5, 2), )

    
    Expires = property(__Expires.value, __Expires.set, None, None)

    
    # Element {http://docs.oasis-open.org/ws-tx/wscoor/2006/06}CurrentContext uses Python identifier CurrentContext
    __CurrentContext = pyxb.binding.content.ElementDeclaration(pyxb.namespace.ExpandedName(Namespace, 'CurrentContext'), 'CurrentContext', '__httpdocs_oasis_open_orgws_txwscoor200606_CreateCoordinationContextType_httpdocs_oasis_open_orgws_txwscoor200606CurrentContext', False, pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wscoor.xsd', 45, 6), )

    
    CurrentContext = property(__CurrentContext.value, __CurrentContext.set, None, None)

    
    # Element {http://docs.oasis-open.org/ws-tx/wscoor/2006/06}CoordinationType uses Python identifier CoordinationType
    __CoordinationType = pyxb.binding.content.ElementDeclaration(pyxb.namespace.ExpandedName(Namespace, 'CoordinationType'), 'CoordinationType', '__httpdocs_oasis_open_orgws_txwscoor200606_CreateCoordinationContextType_httpdocs_oasis_open_orgws_txwscoor200606CoordinationType', False, pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wscoor.xsd', 56, 6), )

    
    CoordinationType = property(__CoordinationType.value, __CoordinationType.set, None, None)

    _AttributeWildcard = pyxb.binding.content.Wildcard(process_contents=pyxb.binding.content.Wildcard.PC_lax, namespace_constraint=(pyxb.binding.content.Wildcard.NC_not, 'http://docs.oasis-open.org/ws-tx/wscoor/2006/06'))
    _HasWildcardElement = True
    _ElementMap.update({
        __Expires.name() : __Expires,
        __CurrentContext.name() : __CurrentContext,
        __CoordinationType.name() : __CoordinationType
    })
    _AttributeMap.update({
        
    })
Namespace.addCategoryObject('typeBinding', 'CreateCoordinationContextType', CreateCoordinationContextType)


# Complex type {http://docs.oasis-open.org/ws-tx/wscoor/2006/06}CreateCoordinationContextResponseType with content type ELEMENT_ONLY
class CreateCoordinationContextResponseType (pyxb.binding.basis.complexTypeDefinition):
    """Complex type {http://docs.oasis-open.org/ws-tx/wscoor/2006/06}CreateCoordinationContextResponseType with content type ELEMENT_ONLY"""
    _TypeDefinition = None
    _ContentTypeTag = pyxb.binding.basis.complexTypeDefinition._CT_ELEMENT_ONLY
    _Abstract = False
    _ExpandedName = pyxb.namespace.ExpandedName(Namespace, 'CreateCoordinationContextResponseType')
    _XSDLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wscoor.xsd', 62, 2)
    _ElementMap = {}
    _AttributeMap = {}
    # Base type is pyxb.binding.datatypes.anyType
    
    # Element {http://docs.oasis-open.org/ws-tx/wscoor/2006/06}CoordinationContext uses Python identifier CoordinationContext
    __CoordinationContext = pyxb.binding.content.ElementDeclaration(pyxb.namespace.ExpandedName(Namespace, 'CoordinationContext'), 'CoordinationContext', '__httpdocs_oasis_open_orgws_txwscoor200606_CreateCoordinationContextResponseType_httpdocs_oasis_open_orgws_txwscoor200606CoordinationContext', False, pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wscoor.xsd', 31, 2), )

    
    CoordinationContext = property(__CoordinationContext.value, __CoordinationContext.set, None, None)

    _AttributeWildcard = pyxb.binding.content.Wildcard(process_contents=pyxb.binding.content.Wildcard.PC_lax, namespace_constraint=(pyxb.binding.content.Wildcard.NC_not, 'http://docs.oasis-open.org/ws-tx/wscoor/2006/06'))
    _HasWildcardElement = True
    _ElementMap.update({
        __CoordinationContext.name() : __CoordinationContext
    })
    _AttributeMap.update({
        
    })
Namespace.addCategoryObject('typeBinding', 'CreateCoordinationContextResponseType', CreateCoordinationContextResponseType)


# Complex type {http://docs.oasis-open.org/ws-tx/wscoor/2006/06}RegisterType with content type ELEMENT_ONLY
class RegisterType (pyxb.binding.basis.complexTypeDefinition):
    """Complex type {http://docs.oasis-open.org/ws-tx/wscoor/2006/06}RegisterType with content type ELEMENT_ONLY"""
    _TypeDefinition = None
    _ContentTypeTag = pyxb.binding.basis.complexTypeDefinition._CT_ELEMENT_ONLY
    _Abstract = False
    _ExpandedName = pyxb.namespace.ExpandedName(Namespace, 'RegisterType')
    _XSDLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wscoor.xsd', 70, 2)
    _ElementMap = {}
    _AttributeMap = {}
    # Base type is pyxb.binding.datatypes.anyType
    
    # Element {http://docs.oasis-open.org/ws-tx/wscoor/2006/06}ProtocolIdentifier uses Python identifier ProtocolIdentifier
    __ProtocolIdentifier = pyxb.binding.content.ElementDeclaration(pyxb.namespace.ExpandedName(Namespace, 'ProtocolIdentifier'), 'ProtocolIdentifier', '__httpdocs_oasis_open_orgws_txwscoor200606_RegisterType_httpdocs_oasis_open_orgws_txwscoor200606ProtocolIdentifier', False, pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wscoor.xsd', 72, 6), )

    
    ProtocolIdentifier = property(__ProtocolIdentifier.value, __ProtocolIdentifier.set, None, None)

    
    # Element {http://docs.oasis-open.org/ws-tx/wscoor/2006/06}ParticipantProtocolService uses Python identifier ParticipantProtocolService
    __ParticipantProtocolService = pyxb.binding.content.ElementDeclaration(pyxb.namespace.ExpandedName(Namespace, 'ParticipantProtocolService'), 'ParticipantProtocolService', '__httpdocs_oasis_open_orgws_txwscoor200606_RegisterType_httpdocs_oasis_open_orgws_txwscoor200606ParticipantProtocolService', False, pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wscoor.xsd', 73, 6), )

    
    ParticipantProtocolService = property(__ParticipantProtocolService.value, __ParticipantProtocolService.set, None, None)

    _AttributeWildcard = pyxb.binding.content.Wildcard(process_contents=pyxb.binding.content.Wildcard.PC_lax, namespace_constraint=(pyxb.binding.content.Wildcard.NC_not, 'http://docs.oasis-open.org/ws-tx/wscoor/2006/06'))
    _HasWildcardElement = True
    _ElementMap.update({
        __ProtocolIdentifier.name() : __ProtocolIdentifier,
        __ParticipantProtocolService.name() : __ParticipantProtocolService
    })
    _AttributeMap.update({
        
    })
Namespace.addCategoryObject('typeBinding', 'RegisterType', RegisterType)


# Complex type {http://docs.oasis-open.org/ws-tx/wscoor/2006/06}RegisterResponseType with content type ELEMENT_ONLY
class RegisterResponseType (pyxb.binding.basis.complexTypeDefinition):
    """Complex type {http://docs.oasis-open.org/ws-tx/wscoor/2006/06}RegisterResponseType with content type ELEMENT_ONLY"""
    _TypeDefinition = None
    _ContentTypeTag = pyxb.binding.basis.complexTypeDefinition._CT_ELEMENT_ONLY
    _Abstract = False
    _ExpandedName = pyxb.namespace.ExpandedName(Namespace, 'RegisterResponseType')
    _XSDLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wscoor.xsd', 79, 2)
    _ElementMap = {}
    _AttributeMap = {}
    # Base type is pyxb.binding.datatypes.anyType
    
    # Element {http://docs.oasis-open.org/ws-tx/wscoor/2006/06}CoordinatorProtocolService uses Python identifier CoordinatorProtocolService
    __CoordinatorProtocolService = pyxb.binding.content.ElementDeclaration(pyxb.namespace.ExpandedName(Namespace, 'CoordinatorProtocolService'), 'CoordinatorProtocolService', '__httpdocs_oasis_open_orgws_txwscoor200606_RegisterResponseType_httpdocs_oasis_open_orgws_txwscoor200606CoordinatorProtocolService', False, pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wscoor.xsd', 81, 6), )

    
    CoordinatorProtocolService = property(__CoordinatorProtocolService.value, __CoordinatorProtocolService.set, None, None)

    _AttributeWildcard = pyxb.binding.content.Wildcard(process_contents=pyxb.binding.content.Wildcard.PC_lax, namespace_constraint=(pyxb.binding.content.Wildcard.NC_not, 'http://docs.oasis-open.org/ws-tx/wscoor/2006/06'))
    _HasWildcardElement = True
    _ElementMap.update({
        __CoordinatorProtocolService.name() : __CoordinatorProtocolService
    })
    _AttributeMap.update({
        
    })
Namespace.addCategoryObject('typeBinding', 'RegisterResponseType', RegisterResponseType)


# Complex type [anonymous] with content type ELEMENT_ONLY
class CTD_ANON_2 (CoordinationContextType):
    """Complex type [anonymous] with content type ELEMENT_ONLY"""
    _TypeDefinition = None
    _ContentTypeTag = pyxb.binding.basis.complexTypeDefinition._CT_ELEMENT_ONLY
    _Abstract = False
    _ExpandedName = None
    _XSDLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wscoor.xsd', 32, 4)
    _ElementMap = CoordinationContextType._ElementMap.copy()
    _AttributeMap = CoordinationContextType._AttributeMap.copy()
    # Base type is CoordinationContextType
    
    # Element Expires ({http://docs.oasis-open.org/ws-tx/wscoor/2006/06}Expires) inherited from {http://docs.oasis-open.org/ws-tx/wscoor/2006/06}CoordinationContextType
    
    # Element Identifier ({http://docs.oasis-open.org/ws-tx/wscoor/2006/06}Identifier) inherited from {http://docs.oasis-open.org/ws-tx/wscoor/2006/06}CoordinationContextType
    
    # Element CoordinationType ({http://docs.oasis-open.org/ws-tx/wscoor/2006/06}CoordinationType) inherited from {http://docs.oasis-open.org/ws-tx/wscoor/2006/06}CoordinationContextType
    
    # Element RegistrationService ({http://docs.oasis-open.org/ws-tx/wscoor/2006/06}RegistrationService) inherited from {http://docs.oasis-open.org/ws-tx/wscoor/2006/06}CoordinationContextType
    _AttributeWildcard = pyxb.binding.content.Wildcard(process_contents=pyxb.binding.content.Wildcard.PC_lax, namespace_constraint=(pyxb.binding.content.Wildcard.NC_not, 'http://docs.oasis-open.org/ws-tx/wscoor/2006/06'))
    _HasWildcardElement = True
    _ElementMap.update({
        
    })
    _AttributeMap.update({
        
    })



# Complex type [anonymous] with content type ELEMENT_ONLY
class CTD_ANON_3 (CoordinationContextType):
    """Complex type [anonymous] with content type ELEMENT_ONLY"""
    _TypeDefinition = None
    _ContentTypeTag = pyxb.binding.basis.complexTypeDefinition._CT_ELEMENT_ONLY
    _Abstract = False
    _ExpandedName = None
    _XSDLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wscoor.xsd', 46, 8)
    _ElementMap = CoordinationContextType._ElementMap.copy()
    _AttributeMap = CoordinationContextType._AttributeMap.copy()
    # Base type is CoordinationContextType
    
    # Element Expires ({http://docs.oasis-open.org/ws-tx/wscoor/2006/06}Expires) inherited from {http://docs.oasis-open.org/ws-tx/wscoor/2006/06}CoordinationContextType
    
    # Element Identifier ({http://docs.oasis-open.org/ws-tx/wscoor/2006/06}Identifier) inherited from {http://docs.oasis-open.org/ws-tx/wscoor/2006/06}CoordinationContextType
    
    # Element CoordinationType ({http://docs.oasis-open.org/ws-tx/wscoor/2006/06}CoordinationType) inherited from {http://docs.oasis-open.org/ws-tx/wscoor/2006/06}CoordinationContextType
    
    # Element RegistrationService ({http://docs.oasis-open.org/ws-tx/wscoor/2006/06}RegistrationService) inherited from {http://docs.oasis-open.org/ws-tx/wscoor/2006/06}CoordinationContextType
    _AttributeWildcard = pyxb.binding.content.Wildcard(process_contents=pyxb.binding.content.Wildcard.PC_lax, namespace_constraint=(pyxb.binding.content.Wildcard.NC_not, 'http://docs.oasis-open.org/ws-tx/wscoor/2006/06'))
    _HasWildcardElement = True
    _ElementMap.update({
        
    })
    _AttributeMap.update({
        
    })



Expires = pyxb.binding.basis.element(pyxb.namespace.ExpandedName(Namespace, 'Expires'), CTD_ANON, location=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wscoor.xsd', 5, 2))
Namespace.addCategoryObject('elementBinding', Expires.name().localName(), Expires)

CreateCoordinationContext = pyxb.binding.basis.element(pyxb.namespace.ExpandedName(Namespace, 'CreateCoordinationContext'), CreateCoordinationContextType, location=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wscoor.xsd', 61, 2))
Namespace.addCategoryObject('elementBinding', CreateCoordinationContext.name().localName(), CreateCoordinationContext)

CreateCoordinationContextResponse = pyxb.binding.basis.element(pyxb.namespace.ExpandedName(Namespace, 'CreateCoordinationContextResponse'), CreateCoordinationContextResponseType, location=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wscoor.xsd', 69, 2))
Namespace.addCategoryObject('elementBinding', CreateCoordinationContextResponse.name().localName(), CreateCoordinationContextResponse)

Register = pyxb.binding.basis.element(pyxb.namespace.ExpandedName(Namespace, 'Register'), RegisterType, location=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wscoor.xsd', 78, 2))
Namespace.addCategoryObject('elementBinding', Register.name().localName(), Register)

RegisterResponse = pyxb.binding.basis.element(pyxb.namespace.ExpandedName(Namespace, 'RegisterResponse'), RegisterResponseType, location=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wscoor.xsd', 86, 2))
Namespace.addCategoryObject('elementBinding', RegisterResponse.name().localName(), RegisterResponse)

CoordinationContext = pyxb.binding.basis.element(pyxb.namespace.ExpandedName(Namespace, 'CoordinationContext'), CTD_ANON_2, location=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wscoor.xsd', 31, 2))
Namespace.addCategoryObject('elementBinding', CoordinationContext.name().localName(), CoordinationContext)



CoordinationContextType._AddElement(pyxb.binding.basis.element(pyxb.namespace.ExpandedName(Namespace, 'Expires'), CTD_ANON, scope=CoordinationContextType, location=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wscoor.xsd', 5, 2)))

CoordinationContextType._AddElement(pyxb.binding.basis.element(pyxb.namespace.ExpandedName(Namespace, 'Identifier'), CTD_ANON_, scope=CoordinationContextType, location=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wscoor.xsd', 16, 6)))

CoordinationContextType._AddElement(pyxb.binding.basis.element(pyxb.namespace.ExpandedName(Namespace, 'CoordinationType'), pyxb.binding.datatypes.anyURI, scope=CoordinationContextType, location=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wscoor.xsd', 26, 6)))

CoordinationContextType._AddElement(pyxb.binding.basis.element(pyxb.namespace.ExpandedName(Namespace, 'RegistrationService'), pyxb.bundles.wssplat.wsa.EndpointReferenceType, scope=CoordinationContextType, location=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wscoor.xsd', 27, 6)))

def _BuildAutomaton ():
    # Remove this helper function from the namespace after it is invoked
    global _BuildAutomaton
    del _BuildAutomaton
    import pyxb.utils.fac as fac

    counters = set()
    cc_0 = fac.CounterCondition(min=0, max=1, metadata=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wscoor.xsd', 25, 6))
    counters.add(cc_0)
    states = []
    final_update = None
    symbol = pyxb.binding.content.ElementUse(CoordinationContextType._UseForTag(pyxb.namespace.ExpandedName(Namespace, 'Identifier')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wscoor.xsd', 16, 6))
    st_0 = fac.State(symbol, is_initial=True, final_update=final_update, is_unordered_catenation=False)
    states.append(st_0)
    final_update = None
    symbol = pyxb.binding.content.ElementUse(CoordinationContextType._UseForTag(pyxb.namespace.ExpandedName(Namespace, 'Expires')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wscoor.xsd', 25, 6))
    st_1 = fac.State(symbol, is_initial=False, final_update=final_update, is_unordered_catenation=False)
    states.append(st_1)
    final_update = None
    symbol = pyxb.binding.content.ElementUse(CoordinationContextType._UseForTag(pyxb.namespace.ExpandedName(Namespace, 'CoordinationType')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wscoor.xsd', 26, 6))
    st_2 = fac.State(symbol, is_initial=False, final_update=final_update, is_unordered_catenation=False)
    states.append(st_2)
    final_update = set()
    symbol = pyxb.binding.content.ElementUse(CoordinationContextType._UseForTag(pyxb.namespace.ExpandedName(Namespace, 'RegistrationService')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wscoor.xsd', 27, 6))
    st_3 = fac.State(symbol, is_initial=False, final_update=final_update, is_unordered_catenation=False)
    states.append(st_3)
    transitions = []
    transitions.append(fac.Transition(st_1, [
         ]))
    transitions.append(fac.Transition(st_2, [
         ]))
    st_0._set_transitionSet(transitions)
    transitions = []
    transitions.append(fac.Transition(st_1, [
        fac.UpdateInstruction(cc_0, True) ]))
    transitions.append(fac.Transition(st_2, [
        fac.UpdateInstruction(cc_0, False) ]))
    st_1._set_transitionSet(transitions)
    transitions = []
    transitions.append(fac.Transition(st_3, [
         ]))
    st_2._set_transitionSet(transitions)
    transitions = []
    st_3._set_transitionSet(transitions)
    return fac.Automaton(states, counters, False, containing_state=None)
CoordinationContextType._Automaton = _BuildAutomaton()




CreateCoordinationContextType._AddElement(pyxb.binding.basis.element(pyxb.namespace.ExpandedName(Namespace, 'Expires'), CTD_ANON, scope=CreateCoordinationContextType, location=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wscoor.xsd', 5, 2)))

CreateCoordinationContextType._AddElement(pyxb.binding.basis.element(pyxb.namespace.ExpandedName(Namespace, 'CurrentContext'), CTD_ANON_3, scope=CreateCoordinationContextType, location=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wscoor.xsd', 45, 6)))

CreateCoordinationContextType._AddElement(pyxb.binding.basis.element(pyxb.namespace.ExpandedName(Namespace, 'CoordinationType'), pyxb.binding.datatypes.anyURI, scope=CreateCoordinationContextType, location=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wscoor.xsd', 56, 6)))

def _BuildAutomaton_ ():
    # Remove this helper function from the namespace after it is invoked
    global _BuildAutomaton_
    del _BuildAutomaton_
    import pyxb.utils.fac as fac

    counters = set()
    cc_0 = fac.CounterCondition(min=0, max=1, metadata=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wscoor.xsd', 44, 6))
    counters.add(cc_0)
    cc_1 = fac.CounterCondition(min=0, max=1, metadata=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wscoor.xsd', 45, 6))
    counters.add(cc_1)
    cc_2 = fac.CounterCondition(min=0, max=None, metadata=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wscoor.xsd', 57, 6))
    counters.add(cc_2)
    states = []
    final_update = None
    symbol = pyxb.binding.content.ElementUse(CreateCoordinationContextType._UseForTag(pyxb.namespace.ExpandedName(Namespace, 'Expires')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wscoor.xsd', 44, 6))
    st_0 = fac.State(symbol, is_initial=True, final_update=final_update, is_unordered_catenation=False)
    states.append(st_0)
    final_update = None
    symbol = pyxb.binding.content.ElementUse(CreateCoordinationContextType._UseForTag(pyxb.namespace.ExpandedName(Namespace, 'CurrentContext')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wscoor.xsd', 45, 6))
    st_1 = fac.State(symbol, is_initial=True, final_update=final_update, is_unordered_catenation=False)
    states.append(st_1)
    final_update = set()
    symbol = pyxb.binding.content.ElementUse(CreateCoordinationContextType._UseForTag(pyxb.namespace.ExpandedName(Namespace, 'CoordinationType')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wscoor.xsd', 56, 6))
    st_2 = fac.State(symbol, is_initial=True, final_update=final_update, is_unordered_catenation=False)
    states.append(st_2)
    final_update = set()
    final_update.add(fac.UpdateInstruction(cc_2, False))
    symbol = pyxb.binding.content.WildcardUse(pyxb.binding.content.Wildcard(process_contents=pyxb.binding.content.Wildcard.PC_lax, namespace_constraint=pyxb.binding.content.Wildcard.NC_any), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wscoor.xsd', 57, 6))
    st_3 = fac.State(symbol, is_initial=False, final_update=final_update, is_unordered_catenation=False)
    states.append(st_3)
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
    transitions.append(fac.Transition(st_3, [
         ]))
    st_2._set_transitionSet(transitions)
    transitions = []
    transitions.append(fac.Transition(st_3, [
        fac.UpdateInstruction(cc_2, True) ]))
    st_3._set_transitionSet(transitions)
    return fac.Automaton(states, counters, False, containing_state=None)
CreateCoordinationContextType._Automaton = _BuildAutomaton_()




CreateCoordinationContextResponseType._AddElement(pyxb.binding.basis.element(pyxb.namespace.ExpandedName(Namespace, 'CoordinationContext'), CTD_ANON_2, scope=CreateCoordinationContextResponseType, location=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wscoor.xsd', 31, 2)))

def _BuildAutomaton_2 ():
    # Remove this helper function from the namespace after it is invoked
    global _BuildAutomaton_2
    del _BuildAutomaton_2
    import pyxb.utils.fac as fac

    counters = set()
    cc_0 = fac.CounterCondition(min=0, max=None, metadata=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wscoor.xsd', 65, 6))
    counters.add(cc_0)
    states = []
    final_update = set()
    symbol = pyxb.binding.content.ElementUse(CreateCoordinationContextResponseType._UseForTag(pyxb.namespace.ExpandedName(Namespace, 'CoordinationContext')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wscoor.xsd', 64, 6))
    st_0 = fac.State(symbol, is_initial=True, final_update=final_update, is_unordered_catenation=False)
    states.append(st_0)
    final_update = set()
    final_update.add(fac.UpdateInstruction(cc_0, False))
    symbol = pyxb.binding.content.WildcardUse(pyxb.binding.content.Wildcard(process_contents=pyxb.binding.content.Wildcard.PC_lax, namespace_constraint=(pyxb.binding.content.Wildcard.NC_not, 'http://docs.oasis-open.org/ws-tx/wscoor/2006/06')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wscoor.xsd', 65, 6))
    st_1 = fac.State(symbol, is_initial=False, final_update=final_update, is_unordered_catenation=False)
    states.append(st_1)
    transitions = []
    transitions.append(fac.Transition(st_1, [
         ]))
    st_0._set_transitionSet(transitions)
    transitions = []
    transitions.append(fac.Transition(st_1, [
        fac.UpdateInstruction(cc_0, True) ]))
    st_1._set_transitionSet(transitions)
    return fac.Automaton(states, counters, False, containing_state=None)
CreateCoordinationContextResponseType._Automaton = _BuildAutomaton_2()




RegisterType._AddElement(pyxb.binding.basis.element(pyxb.namespace.ExpandedName(Namespace, 'ProtocolIdentifier'), pyxb.binding.datatypes.anyURI, scope=RegisterType, location=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wscoor.xsd', 72, 6)))

RegisterType._AddElement(pyxb.binding.basis.element(pyxb.namespace.ExpandedName(Namespace, 'ParticipantProtocolService'), pyxb.bundles.wssplat.wsa.EndpointReferenceType, scope=RegisterType, location=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wscoor.xsd', 73, 6)))

def _BuildAutomaton_3 ():
    # Remove this helper function from the namespace after it is invoked
    global _BuildAutomaton_3
    del _BuildAutomaton_3
    import pyxb.utils.fac as fac

    counters = set()
    cc_0 = fac.CounterCondition(min=0, max=None, metadata=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wscoor.xsd', 74, 6))
    counters.add(cc_0)
    states = []
    final_update = None
    symbol = pyxb.binding.content.ElementUse(RegisterType._UseForTag(pyxb.namespace.ExpandedName(Namespace, 'ProtocolIdentifier')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wscoor.xsd', 72, 6))
    st_0 = fac.State(symbol, is_initial=True, final_update=final_update, is_unordered_catenation=False)
    states.append(st_0)
    final_update = set()
    symbol = pyxb.binding.content.ElementUse(RegisterType._UseForTag(pyxb.namespace.ExpandedName(Namespace, 'ParticipantProtocolService')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wscoor.xsd', 73, 6))
    st_1 = fac.State(symbol, is_initial=False, final_update=final_update, is_unordered_catenation=False)
    states.append(st_1)
    final_update = set()
    final_update.add(fac.UpdateInstruction(cc_0, False))
    symbol = pyxb.binding.content.WildcardUse(pyxb.binding.content.Wildcard(process_contents=pyxb.binding.content.Wildcard.PC_lax, namespace_constraint=pyxb.binding.content.Wildcard.NC_any), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wscoor.xsd', 74, 6))
    st_2 = fac.State(symbol, is_initial=False, final_update=final_update, is_unordered_catenation=False)
    states.append(st_2)
    transitions = []
    transitions.append(fac.Transition(st_1, [
         ]))
    st_0._set_transitionSet(transitions)
    transitions = []
    transitions.append(fac.Transition(st_2, [
         ]))
    st_1._set_transitionSet(transitions)
    transitions = []
    transitions.append(fac.Transition(st_2, [
        fac.UpdateInstruction(cc_0, True) ]))
    st_2._set_transitionSet(transitions)
    return fac.Automaton(states, counters, False, containing_state=None)
RegisterType._Automaton = _BuildAutomaton_3()




RegisterResponseType._AddElement(pyxb.binding.basis.element(pyxb.namespace.ExpandedName(Namespace, 'CoordinatorProtocolService'), pyxb.bundles.wssplat.wsa.EndpointReferenceType, scope=RegisterResponseType, location=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wscoor.xsd', 81, 6)))

def _BuildAutomaton_4 ():
    # Remove this helper function from the namespace after it is invoked
    global _BuildAutomaton_4
    del _BuildAutomaton_4
    import pyxb.utils.fac as fac

    counters = set()
    cc_0 = fac.CounterCondition(min=0, max=None, metadata=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wscoor.xsd', 82, 6))
    counters.add(cc_0)
    states = []
    final_update = set()
    symbol = pyxb.binding.content.ElementUse(RegisterResponseType._UseForTag(pyxb.namespace.ExpandedName(Namespace, 'CoordinatorProtocolService')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wscoor.xsd', 81, 6))
    st_0 = fac.State(symbol, is_initial=True, final_update=final_update, is_unordered_catenation=False)
    states.append(st_0)
    final_update = set()
    final_update.add(fac.UpdateInstruction(cc_0, False))
    symbol = pyxb.binding.content.WildcardUse(pyxb.binding.content.Wildcard(process_contents=pyxb.binding.content.Wildcard.PC_lax, namespace_constraint=pyxb.binding.content.Wildcard.NC_any), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wscoor.xsd', 82, 6))
    st_1 = fac.State(symbol, is_initial=False, final_update=final_update, is_unordered_catenation=False)
    states.append(st_1)
    transitions = []
    transitions.append(fac.Transition(st_1, [
         ]))
    st_0._set_transitionSet(transitions)
    transitions = []
    transitions.append(fac.Transition(st_1, [
        fac.UpdateInstruction(cc_0, True) ]))
    st_1._set_transitionSet(transitions)
    return fac.Automaton(states, counters, False, containing_state=None)
RegisterResponseType._Automaton = _BuildAutomaton_4()




def _BuildAutomaton_5 ():
    # Remove this helper function from the namespace after it is invoked
    global _BuildAutomaton_5
    del _BuildAutomaton_5
    import pyxb.utils.fac as fac

    counters = set()
    cc_0 = fac.CounterCondition(min=0, max=1, metadata=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wscoor.xsd', 25, 6))
    counters.add(cc_0)
    cc_1 = fac.CounterCondition(min=0, max=None, metadata=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wscoor.xsd', 36, 12))
    counters.add(cc_1)
    states = []
    final_update = None
    symbol = pyxb.binding.content.ElementUse(CTD_ANON_2._UseForTag(pyxb.namespace.ExpandedName(Namespace, 'Identifier')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wscoor.xsd', 16, 6))
    st_0 = fac.State(symbol, is_initial=True, final_update=final_update, is_unordered_catenation=False)
    states.append(st_0)
    final_update = None
    symbol = pyxb.binding.content.ElementUse(CTD_ANON_2._UseForTag(pyxb.namespace.ExpandedName(Namespace, 'Expires')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wscoor.xsd', 25, 6))
    st_1 = fac.State(symbol, is_initial=False, final_update=final_update, is_unordered_catenation=False)
    states.append(st_1)
    final_update = None
    symbol = pyxb.binding.content.ElementUse(CTD_ANON_2._UseForTag(pyxb.namespace.ExpandedName(Namespace, 'CoordinationType')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wscoor.xsd', 26, 6))
    st_2 = fac.State(symbol, is_initial=False, final_update=final_update, is_unordered_catenation=False)
    states.append(st_2)
    final_update = set()
    symbol = pyxb.binding.content.ElementUse(CTD_ANON_2._UseForTag(pyxb.namespace.ExpandedName(Namespace, 'RegistrationService')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wscoor.xsd', 27, 6))
    st_3 = fac.State(symbol, is_initial=False, final_update=final_update, is_unordered_catenation=False)
    states.append(st_3)
    final_update = set()
    final_update.add(fac.UpdateInstruction(cc_1, False))
    symbol = pyxb.binding.content.WildcardUse(pyxb.binding.content.Wildcard(process_contents=pyxb.binding.content.Wildcard.PC_lax, namespace_constraint=(pyxb.binding.content.Wildcard.NC_not, 'http://docs.oasis-open.org/ws-tx/wscoor/2006/06')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wscoor.xsd', 36, 12))
    st_4 = fac.State(symbol, is_initial=False, final_update=final_update, is_unordered_catenation=False)
    states.append(st_4)
    transitions = []
    transitions.append(fac.Transition(st_1, [
         ]))
    transitions.append(fac.Transition(st_2, [
         ]))
    st_0._set_transitionSet(transitions)
    transitions = []
    transitions.append(fac.Transition(st_1, [
        fac.UpdateInstruction(cc_0, True) ]))
    transitions.append(fac.Transition(st_2, [
        fac.UpdateInstruction(cc_0, False) ]))
    st_1._set_transitionSet(transitions)
    transitions = []
    transitions.append(fac.Transition(st_3, [
         ]))
    st_2._set_transitionSet(transitions)
    transitions = []
    transitions.append(fac.Transition(st_4, [
         ]))
    st_3._set_transitionSet(transitions)
    transitions = []
    transitions.append(fac.Transition(st_4, [
        fac.UpdateInstruction(cc_1, True) ]))
    st_4._set_transitionSet(transitions)
    return fac.Automaton(states, counters, False, containing_state=None)
CTD_ANON_2._Automaton = _BuildAutomaton_5()




def _BuildAutomaton_6 ():
    # Remove this helper function from the namespace after it is invoked
    global _BuildAutomaton_6
    del _BuildAutomaton_6
    import pyxb.utils.fac as fac

    counters = set()
    cc_0 = fac.CounterCondition(min=0, max=1, metadata=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wscoor.xsd', 46, 8))
    counters.add(cc_0)
    cc_1 = fac.CounterCondition(min=0, max=1, metadata=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wscoor.xsd', 25, 6))
    counters.add(cc_1)
    cc_2 = fac.CounterCondition(min=0, max=None, metadata=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wscoor.xsd', 50, 16))
    counters.add(cc_2)
    states = []
    final_update = None
    symbol = pyxb.binding.content.ElementUse(CTD_ANON_3._UseForTag(pyxb.namespace.ExpandedName(Namespace, 'Identifier')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wscoor.xsd', 16, 6))
    st_0 = fac.State(symbol, is_initial=True, final_update=final_update, is_unordered_catenation=False)
    states.append(st_0)
    final_update = None
    symbol = pyxb.binding.content.ElementUse(CTD_ANON_3._UseForTag(pyxb.namespace.ExpandedName(Namespace, 'Expires')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wscoor.xsd', 25, 6))
    st_1 = fac.State(symbol, is_initial=False, final_update=final_update, is_unordered_catenation=False)
    states.append(st_1)
    final_update = None
    symbol = pyxb.binding.content.ElementUse(CTD_ANON_3._UseForTag(pyxb.namespace.ExpandedName(Namespace, 'CoordinationType')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wscoor.xsd', 26, 6))
    st_2 = fac.State(symbol, is_initial=False, final_update=final_update, is_unordered_catenation=False)
    states.append(st_2)
    final_update = set()
    final_update.add(fac.UpdateInstruction(cc_0, False))
    symbol = pyxb.binding.content.ElementUse(CTD_ANON_3._UseForTag(pyxb.namespace.ExpandedName(Namespace, 'RegistrationService')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wscoor.xsd', 27, 6))
    st_3 = fac.State(symbol, is_initial=False, final_update=final_update, is_unordered_catenation=False)
    states.append(st_3)
    final_update = set()
    final_update.add(fac.UpdateInstruction(cc_0, False))
    final_update.add(fac.UpdateInstruction(cc_2, False))
    symbol = pyxb.binding.content.WildcardUse(pyxb.binding.content.Wildcard(process_contents=pyxb.binding.content.Wildcard.PC_lax, namespace_constraint=(pyxb.binding.content.Wildcard.NC_not, 'http://docs.oasis-open.org/ws-tx/wscoor/2006/06')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wscoor.xsd', 50, 16))
    st_4 = fac.State(symbol, is_initial=False, final_update=final_update, is_unordered_catenation=False)
    states.append(st_4)
    transitions = []
    transitions.append(fac.Transition(st_1, [
         ]))
    transitions.append(fac.Transition(st_2, [
         ]))
    st_0._set_transitionSet(transitions)
    transitions = []
    transitions.append(fac.Transition(st_1, [
        fac.UpdateInstruction(cc_1, True) ]))
    transitions.append(fac.Transition(st_2, [
        fac.UpdateInstruction(cc_1, False) ]))
    st_1._set_transitionSet(transitions)
    transitions = []
    transitions.append(fac.Transition(st_3, [
         ]))
    st_2._set_transitionSet(transitions)
    transitions = []
    transitions.append(fac.Transition(st_0, [
        fac.UpdateInstruction(cc_0, True) ]))
    transitions.append(fac.Transition(st_4, [
         ]))
    st_3._set_transitionSet(transitions)
    transitions = []
    transitions.append(fac.Transition(st_0, [
        fac.UpdateInstruction(cc_0, True),
        fac.UpdateInstruction(cc_2, False) ]))
    transitions.append(fac.Transition(st_4, [
        fac.UpdateInstruction(cc_2, True) ]))
    st_4._set_transitionSet(transitions)
    return fac.Automaton(states, counters, True, containing_state=None)
CTD_ANON_3._Automaton = _BuildAutomaton_6()

