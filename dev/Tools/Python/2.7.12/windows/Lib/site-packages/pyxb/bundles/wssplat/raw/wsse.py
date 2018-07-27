# ./pyxb/bundles/wssplat/raw/wsse.py
# -*- coding: utf-8 -*-
# PyXB bindings for NM:533be3d902dc7f54d5027ddd5917639d584e9d38
# Generated 2014-10-19 06:24:58.398872 by PyXB version 1.2.4 using Python 2.7.3.final.0
# Namespace http://docs.oasis-open.org/wss/2004/01/oasis-200401-wss-wssecurity-secext-1.0.xsd

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
_GenerationUID = pyxb.utils.utility.UniqueIdentifier('urn:uuid:8ef390dc-5782-11e4-9f7f-c8600024e903')

# Version of PyXB used to generate the bindings
_PyXBVersion = '1.2.4'
# Generated bindings are not compatible across PyXB versions
if pyxb.__version__ != _PyXBVersion:
    raise pyxb.PyXBVersionError(_PyXBVersion)

# Import bindings for namespaces imported into schema
import pyxb.binding.datatypes
import pyxb.bundles.wssplat.wsu

# NOTE: All namespace declarations are reserved within the binding
Namespace = pyxb.namespace.NamespaceForURI('http://docs.oasis-open.org/wss/2004/01/oasis-200401-wss-wssecurity-secext-1.0.xsd', create_if_missing=True)
Namespace.configureCategories(['typeBinding', 'elementBinding'])
_Namespace_wsu = pyxb.bundles.wssplat.wsu.Namespace
_Namespace_wsu.configureCategories(['typeBinding', 'elementBinding'])

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


# List simple type: {http://docs.oasis-open.org/wss/2004/01/oasis-200401-wss-wssecurity-secext-1.0.xsd}tUsage
# superclasses pyxb.binding.datatypes.anySimpleType
class tUsage (pyxb.binding.basis.STD_list):

    """Typedef to allow a list of usages (as URIs)."""

    _ExpandedName = pyxb.namespace.ExpandedName(Namespace, 'tUsage')
    _XSDLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wsse.xsd', 76, 1)
    _Documentation = 'Typedef to allow a list of usages (as URIs).'

    _ItemType = pyxb.binding.datatypes.anyURI
tUsage._InitializeFacetMap()
Namespace.addCategoryObject('typeBinding', 'tUsage', tUsage)

# Atomic simple type: {http://docs.oasis-open.org/wss/2004/01/oasis-200401-wss-wssecurity-secext-1.0.xsd}FaultcodeEnum
class FaultcodeEnum (pyxb.binding.datatypes.QName, pyxb.binding.basis.enumeration_mixin):

    """An atomic simple type."""

    _ExpandedName = pyxb.namespace.ExpandedName(Namespace, 'FaultcodeEnum')
    _XSDLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wsse.xsd', 184, 1)
    _Documentation = None
FaultcodeEnum._CF_enumeration = pyxb.binding.facets.CF_enumeration(value_datatype=FaultcodeEnum, enum_prefix=None)
FaultcodeEnum._CF_enumeration.addEnumeration(value=pyxb.namespace.ExpandedName(Namespace, 'UnsupportedSecurityToken'), tag=None)
FaultcodeEnum._CF_enumeration.addEnumeration(value=pyxb.namespace.ExpandedName(Namespace, 'UnsupportedAlgorithm'), tag=None)
FaultcodeEnum._CF_enumeration.addEnumeration(value=pyxb.namespace.ExpandedName(Namespace, 'InvalidSecurity'), tag=None)
FaultcodeEnum._CF_enumeration.addEnumeration(value=pyxb.namespace.ExpandedName(Namespace, 'InvalidSecurityToken'), tag=None)
FaultcodeEnum._CF_enumeration.addEnumeration(value=pyxb.namespace.ExpandedName(Namespace, 'FailedAuthentication'), tag=None)
FaultcodeEnum._CF_enumeration.addEnumeration(value=pyxb.namespace.ExpandedName(Namespace, 'FailedCheck'), tag=None)
FaultcodeEnum._CF_enumeration.addEnumeration(value=pyxb.namespace.ExpandedName(Namespace, 'SecurityTokenUnavailable'), tag=None)
FaultcodeEnum._InitializeFacetMap(FaultcodeEnum._CF_enumeration)
Namespace.addCategoryObject('typeBinding', 'FaultcodeEnum', FaultcodeEnum)

# Complex type {http://docs.oasis-open.org/wss/2004/01/oasis-200401-wss-wssecurity-secext-1.0.xsd}AttributedString with content type SIMPLE
class AttributedString (pyxb.binding.basis.complexTypeDefinition):
    """This type represents an element with arbitrary attributes."""
    _TypeDefinition = pyxb.binding.datatypes.string
    _ContentTypeTag = pyxb.binding.basis.complexTypeDefinition._CT_SIMPLE
    _Abstract = False
    _ExpandedName = pyxb.namespace.ExpandedName(Namespace, 'AttributedString')
    _XSDLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wsse.xsd', 14, 1)
    _ElementMap = {}
    _AttributeMap = {}
    # Base type is pyxb.binding.datatypes.string
    
    # Attribute {http://docs.oasis-open.org/wss/2004/01/oasis-200401-wss-wssecurity-utility-1.0.xsd}Id uses Python identifier Id
    __Id = pyxb.binding.content.AttributeUse(pyxb.namespace.ExpandedName(_Namespace_wsu, 'Id'), 'Id', '__httpdocs_oasis_open_orgwss200401oasis_200401_wss_wssecurity_secext_1_0_xsd_AttributedString_httpdocs_oasis_open_orgwss200401oasis_200401_wss_wssecurity_utility_1_0_xsdId', pyxb.binding.datatypes.ID)
    __Id._DeclarationLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wsu.xsd', 28, 1)
    __Id._UseLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wsse.xsd', 20, 4)
    
    Id = property(__Id.value, __Id.set, None, '\nThis global attribute supports annotating arbitrary elements with an ID.\n          ')

    _AttributeWildcard = pyxb.binding.content.Wildcard(process_contents=pyxb.binding.content.Wildcard.PC_lax, namespace_constraint=(pyxb.binding.content.Wildcard.NC_not, 'http://docs.oasis-open.org/wss/2004/01/oasis-200401-wss-wssecurity-secext-1.0.xsd'))
    _ElementMap.update({
        
    })
    _AttributeMap.update({
        __Id.name() : __Id
    })
Namespace.addCategoryObject('typeBinding', 'AttributedString', AttributedString)


# Complex type {http://docs.oasis-open.org/wss/2004/01/oasis-200401-wss-wssecurity-secext-1.0.xsd}UsernameTokenType with content type ELEMENT_ONLY
class UsernameTokenType (pyxb.binding.basis.complexTypeDefinition):
    """This type represents a username token per Section 4.1"""
    _TypeDefinition = None
    _ContentTypeTag = pyxb.binding.basis.complexTypeDefinition._CT_ELEMENT_ONLY
    _Abstract = False
    _ExpandedName = pyxb.namespace.ExpandedName(Namespace, 'UsernameTokenType')
    _XSDLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wsse.xsd', 45, 1)
    _ElementMap = {}
    _AttributeMap = {}
    # Base type is pyxb.binding.datatypes.anyType
    
    # Element {http://docs.oasis-open.org/wss/2004/01/oasis-200401-wss-wssecurity-secext-1.0.xsd}Username uses Python identifier Username
    __Username = pyxb.binding.content.ElementDeclaration(pyxb.namespace.ExpandedName(Namespace, 'Username'), 'Username', '__httpdocs_oasis_open_orgwss200401oasis_200401_wss_wssecurity_secext_1_0_xsd_UsernameTokenType_httpdocs_oasis_open_orgwss200401oasis_200401_wss_wssecurity_secext_1_0_xsdUsername', False, pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wsse.xsd', 50, 3), )

    
    Username = property(__Username.value, __Username.set, None, None)

    
    # Attribute {http://docs.oasis-open.org/wss/2004/01/oasis-200401-wss-wssecurity-utility-1.0.xsd}Id uses Python identifier Id
    __Id = pyxb.binding.content.AttributeUse(pyxb.namespace.ExpandedName(_Namespace_wsu, 'Id'), 'Id', '__httpdocs_oasis_open_orgwss200401oasis_200401_wss_wssecurity_secext_1_0_xsd_UsernameTokenType_httpdocs_oasis_open_orgwss200401oasis_200401_wss_wssecurity_utility_1_0_xsdId', pyxb.binding.datatypes.ID)
    __Id._DeclarationLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wsu.xsd', 28, 1)
    __Id._UseLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wsse.xsd', 53, 2)
    
    Id = property(__Id.value, __Id.set, None, '\nThis global attribute supports annotating arbitrary elements with an ID.\n          ')

    _AttributeWildcard = pyxb.binding.content.Wildcard(process_contents=pyxb.binding.content.Wildcard.PC_lax, namespace_constraint=(pyxb.binding.content.Wildcard.NC_not, 'http://docs.oasis-open.org/wss/2004/01/oasis-200401-wss-wssecurity-secext-1.0.xsd'))
    _HasWildcardElement = True
    _ElementMap.update({
        __Username.name() : __Username
    })
    _AttributeMap.update({
        __Id.name() : __Id
    })
Namespace.addCategoryObject('typeBinding', 'UsernameTokenType', UsernameTokenType)


# Complex type {http://docs.oasis-open.org/wss/2004/01/oasis-200401-wss-wssecurity-secext-1.0.xsd}ReferenceType with content type EMPTY
class ReferenceType (pyxb.binding.basis.complexTypeDefinition):
    """This type represents a reference to an external security token."""
    _TypeDefinition = None
    _ContentTypeTag = pyxb.binding.basis.complexTypeDefinition._CT_EMPTY
    _Abstract = False
    _ExpandedName = pyxb.namespace.ExpandedName(Namespace, 'ReferenceType')
    _XSDLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wsse.xsd', 87, 1)
    _ElementMap = {}
    _AttributeMap = {}
    # Base type is pyxb.binding.datatypes.anyType
    
    # Attribute URI uses Python identifier URI
    __URI = pyxb.binding.content.AttributeUse(pyxb.namespace.ExpandedName(None, 'URI'), 'URI', '__httpdocs_oasis_open_orgwss200401oasis_200401_wss_wssecurity_secext_1_0_xsd_ReferenceType_URI', pyxb.binding.datatypes.anyURI)
    __URI._DeclarationLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wsse.xsd', 91, 2)
    __URI._UseLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wsse.xsd', 91, 2)
    
    URI = property(__URI.value, __URI.set, None, None)

    
    # Attribute ValueType uses Python identifier ValueType
    __ValueType = pyxb.binding.content.AttributeUse(pyxb.namespace.ExpandedName(None, 'ValueType'), 'ValueType', '__httpdocs_oasis_open_orgwss200401oasis_200401_wss_wssecurity_secext_1_0_xsd_ReferenceType_ValueType', pyxb.binding.datatypes.anyURI)
    __ValueType._DeclarationLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wsse.xsd', 92, 2)
    __ValueType._UseLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wsse.xsd', 92, 2)
    
    ValueType = property(__ValueType.value, __ValueType.set, None, None)

    _AttributeWildcard = pyxb.binding.content.Wildcard(process_contents=pyxb.binding.content.Wildcard.PC_lax, namespace_constraint=(pyxb.binding.content.Wildcard.NC_not, 'http://docs.oasis-open.org/wss/2004/01/oasis-200401-wss-wssecurity-secext-1.0.xsd'))
    _ElementMap.update({
        
    })
    _AttributeMap.update({
        __URI.name() : __URI,
        __ValueType.name() : __ValueType
    })
Namespace.addCategoryObject('typeBinding', 'ReferenceType', ReferenceType)


# Complex type {http://docs.oasis-open.org/wss/2004/01/oasis-200401-wss-wssecurity-secext-1.0.xsd}EmbeddedType with content type ELEMENT_ONLY
class EmbeddedType (pyxb.binding.basis.complexTypeDefinition):
    """This type represents a reference to an embedded security token."""
    _TypeDefinition = None
    _ContentTypeTag = pyxb.binding.basis.complexTypeDefinition._CT_ELEMENT_ONLY
    _Abstract = False
    _ExpandedName = pyxb.namespace.ExpandedName(Namespace, 'EmbeddedType')
    _XSDLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wsse.xsd', 95, 1)
    _ElementMap = {}
    _AttributeMap = {}
    # Base type is pyxb.binding.datatypes.anyType
    
    # Attribute ValueType uses Python identifier ValueType
    __ValueType = pyxb.binding.content.AttributeUse(pyxb.namespace.ExpandedName(None, 'ValueType'), 'ValueType', '__httpdocs_oasis_open_orgwss200401oasis_200401_wss_wssecurity_secext_1_0_xsd_EmbeddedType_ValueType', pyxb.binding.datatypes.anyURI)
    __ValueType._DeclarationLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wsse.xsd', 102, 2)
    __ValueType._UseLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wsse.xsd', 102, 2)
    
    ValueType = property(__ValueType.value, __ValueType.set, None, None)

    _AttributeWildcard = pyxb.binding.content.Wildcard(process_contents=pyxb.binding.content.Wildcard.PC_lax, namespace_constraint=(pyxb.binding.content.Wildcard.NC_not, 'http://docs.oasis-open.org/wss/2004/01/oasis-200401-wss-wssecurity-secext-1.0.xsd'))
    _HasWildcardElement = True
    _ElementMap.update({
        
    })
    _AttributeMap.update({
        __ValueType.name() : __ValueType
    })
Namespace.addCategoryObject('typeBinding', 'EmbeddedType', EmbeddedType)


# Complex type {http://docs.oasis-open.org/wss/2004/01/oasis-200401-wss-wssecurity-secext-1.0.xsd}SecurityHeaderType with content type ELEMENT_ONLY
class SecurityHeaderType (pyxb.binding.basis.complexTypeDefinition):
    """This complexType defines header block to use for security-relevant data directed at a specific SOAP actor."""
    _TypeDefinition = None
    _ContentTypeTag = pyxb.binding.basis.complexTypeDefinition._CT_ELEMENT_ONLY
    _Abstract = False
    _ExpandedName = pyxb.namespace.ExpandedName(Namespace, 'SecurityHeaderType')
    _XSDLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wsse.xsd', 116, 1)
    _ElementMap = {}
    _AttributeMap = {}
    # Base type is pyxb.binding.datatypes.anyType
    _AttributeWildcard = pyxb.binding.content.Wildcard(process_contents=pyxb.binding.content.Wildcard.PC_lax, namespace_constraint=(pyxb.binding.content.Wildcard.NC_not, 'http://docs.oasis-open.org/wss/2004/01/oasis-200401-wss-wssecurity-secext-1.0.xsd'))
    _HasWildcardElement = True
    _ElementMap.update({
        
    })
    _AttributeMap.update({
        
    })
Namespace.addCategoryObject('typeBinding', 'SecurityHeaderType', SecurityHeaderType)


# Complex type {http://docs.oasis-open.org/wss/2004/01/oasis-200401-wss-wssecurity-secext-1.0.xsd}TransformationParametersType with content type ELEMENT_ONLY
class TransformationParametersType (pyxb.binding.basis.complexTypeDefinition):
    """This complexType defines a container for elements to be specified from any namespace as properties/parameters of a DSIG transformation."""
    _TypeDefinition = None
    _ContentTypeTag = pyxb.binding.basis.complexTypeDefinition._CT_ELEMENT_ONLY
    _Abstract = False
    _ExpandedName = pyxb.namespace.ExpandedName(Namespace, 'TransformationParametersType')
    _XSDLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wsse.xsd', 129, 1)
    _ElementMap = {}
    _AttributeMap = {}
    # Base type is pyxb.binding.datatypes.anyType
    _AttributeWildcard = pyxb.binding.content.Wildcard(process_contents=pyxb.binding.content.Wildcard.PC_lax, namespace_constraint=(pyxb.binding.content.Wildcard.NC_not, 'http://docs.oasis-open.org/wss/2004/01/oasis-200401-wss-wssecurity-secext-1.0.xsd'))
    _HasWildcardElement = True
    _ElementMap.update({
        
    })
    _AttributeMap.update({
        
    })
Namespace.addCategoryObject('typeBinding', 'TransformationParametersType', TransformationParametersType)


# Complex type {http://docs.oasis-open.org/wss/2004/01/oasis-200401-wss-wssecurity-secext-1.0.xsd}PasswordString with content type SIMPLE
class PasswordString (AttributedString):
    """This type is used for password elements per Section 4.1."""
    _TypeDefinition = pyxb.binding.datatypes.string
    _ContentTypeTag = pyxb.binding.basis.complexTypeDefinition._CT_SIMPLE
    _Abstract = False
    _ExpandedName = pyxb.namespace.ExpandedName(Namespace, 'PasswordString')
    _XSDLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wsse.xsd', 25, 1)
    _ElementMap = AttributedString._ElementMap.copy()
    _AttributeMap = AttributedString._AttributeMap.copy()
    # Base type is AttributedString
    
    # Attribute Type uses Python identifier Type
    __Type = pyxb.binding.content.AttributeUse(pyxb.namespace.ExpandedName(None, 'Type'), 'Type', '__httpdocs_oasis_open_orgwss200401oasis_200401_wss_wssecurity_secext_1_0_xsd_PasswordString_Type', pyxb.binding.datatypes.anyURI)
    __Type._DeclarationLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wsse.xsd', 31, 4)
    __Type._UseLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wsse.xsd', 31, 4)
    
    Type = property(__Type.value, __Type.set, None, None)

    
    # Attribute Id inherited from {http://docs.oasis-open.org/wss/2004/01/oasis-200401-wss-wssecurity-secext-1.0.xsd}AttributedString
    _AttributeWildcard = pyxb.binding.content.Wildcard(process_contents=pyxb.binding.content.Wildcard.PC_lax, namespace_constraint=(pyxb.binding.content.Wildcard.NC_not, 'http://docs.oasis-open.org/wss/2004/01/oasis-200401-wss-wssecurity-secext-1.0.xsd'))
    _ElementMap.update({
        
    })
    _AttributeMap.update({
        __Type.name() : __Type
    })
Namespace.addCategoryObject('typeBinding', 'PasswordString', PasswordString)


# Complex type {http://docs.oasis-open.org/wss/2004/01/oasis-200401-wss-wssecurity-secext-1.0.xsd}EncodedString with content type SIMPLE
class EncodedString (AttributedString):
    """This type is used for elements containing stringified binary data."""
    _TypeDefinition = pyxb.binding.datatypes.string
    _ContentTypeTag = pyxb.binding.basis.complexTypeDefinition._CT_SIMPLE
    _Abstract = False
    _ExpandedName = pyxb.namespace.ExpandedName(Namespace, 'EncodedString')
    _XSDLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wsse.xsd', 35, 1)
    _ElementMap = AttributedString._ElementMap.copy()
    _AttributeMap = AttributedString._AttributeMap.copy()
    # Base type is AttributedString
    
    # Attribute EncodingType uses Python identifier EncodingType
    __EncodingType = pyxb.binding.content.AttributeUse(pyxb.namespace.ExpandedName(None, 'EncodingType'), 'EncodingType', '__httpdocs_oasis_open_orgwss200401oasis_200401_wss_wssecurity_secext_1_0_xsd_EncodedString_EncodingType', pyxb.binding.datatypes.anyURI)
    __EncodingType._DeclarationLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wsse.xsd', 41, 4)
    __EncodingType._UseLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wsse.xsd', 41, 4)
    
    EncodingType = property(__EncodingType.value, __EncodingType.set, None, None)

    
    # Attribute Id inherited from {http://docs.oasis-open.org/wss/2004/01/oasis-200401-wss-wssecurity-secext-1.0.xsd}AttributedString
    _AttributeWildcard = pyxb.binding.content.Wildcard(process_contents=pyxb.binding.content.Wildcard.PC_lax, namespace_constraint=(pyxb.binding.content.Wildcard.NC_not, 'http://docs.oasis-open.org/wss/2004/01/oasis-200401-wss-wssecurity-secext-1.0.xsd'))
    _ElementMap.update({
        
    })
    _AttributeMap.update({
        __EncodingType.name() : __EncodingType
    })
Namespace.addCategoryObject('typeBinding', 'EncodedString', EncodedString)


# Complex type {http://docs.oasis-open.org/wss/2004/01/oasis-200401-wss-wssecurity-secext-1.0.xsd}SecurityTokenReferenceType with content type ELEMENT_ONLY
class SecurityTokenReferenceType (pyxb.binding.basis.complexTypeDefinition):
    """This type is used reference a security token."""
    _TypeDefinition = None
    _ContentTypeTag = pyxb.binding.basis.complexTypeDefinition._CT_ELEMENT_ONLY
    _Abstract = False
    _ExpandedName = pyxb.namespace.ExpandedName(Namespace, 'SecurityTokenReferenceType')
    _XSDLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wsse.xsd', 105, 1)
    _ElementMap = {}
    _AttributeMap = {}
    # Base type is pyxb.binding.datatypes.anyType
    
    # Attribute {http://docs.oasis-open.org/wss/2004/01/oasis-200401-wss-wssecurity-secext-1.0.xsd}Usage uses Python identifier Usage
    __Usage = pyxb.binding.content.AttributeUse(pyxb.namespace.ExpandedName(Namespace, 'Usage'), 'Usage', '__httpdocs_oasis_open_orgwss200401oasis_200401_wss_wssecurity_secext_1_0_xsd_SecurityTokenReferenceType_httpdocs_oasis_open_orgwss200401oasis_200401_wss_wssecurity_secext_1_0_xsdUsage', tUsage)
    __Usage._DeclarationLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wsse.xsd', 82, 1)
    __Usage._UseLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wsse.xsd', 113, 2)
    
    Usage = property(__Usage.value, __Usage.set, None, 'This global attribute is used to indicate the usage of a referenced or indicated token within the containing context')

    
    # Attribute {http://docs.oasis-open.org/wss/2004/01/oasis-200401-wss-wssecurity-utility-1.0.xsd}Id uses Python identifier Id
    __Id = pyxb.binding.content.AttributeUse(pyxb.namespace.ExpandedName(_Namespace_wsu, 'Id'), 'Id', '__httpdocs_oasis_open_orgwss200401oasis_200401_wss_wssecurity_secext_1_0_xsd_SecurityTokenReferenceType_httpdocs_oasis_open_orgwss200401oasis_200401_wss_wssecurity_utility_1_0_xsdId', pyxb.binding.datatypes.ID)
    __Id._DeclarationLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wsu.xsd', 28, 1)
    __Id._UseLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wsse.xsd', 112, 2)
    
    Id = property(__Id.value, __Id.set, None, '\nThis global attribute supports annotating arbitrary elements with an ID.\n          ')

    _AttributeWildcard = pyxb.binding.content.Wildcard(process_contents=pyxb.binding.content.Wildcard.PC_lax, namespace_constraint=(pyxb.binding.content.Wildcard.NC_not, 'http://docs.oasis-open.org/wss/2004/01/oasis-200401-wss-wssecurity-secext-1.0.xsd'))
    _HasWildcardElement = True
    _ElementMap.update({
        
    })
    _AttributeMap.update({
        __Usage.name() : __Usage,
        __Id.name() : __Id
    })
Namespace.addCategoryObject('typeBinding', 'SecurityTokenReferenceType', SecurityTokenReferenceType)


# Complex type {http://docs.oasis-open.org/wss/2004/01/oasis-200401-wss-wssecurity-secext-1.0.xsd}BinarySecurityTokenType with content type SIMPLE
class BinarySecurityTokenType (EncodedString):
    """A security token that is encoded in binary"""
    _TypeDefinition = pyxb.binding.datatypes.string
    _ContentTypeTag = pyxb.binding.basis.complexTypeDefinition._CT_SIMPLE
    _Abstract = False
    _ExpandedName = pyxb.namespace.ExpandedName(Namespace, 'BinarySecurityTokenType')
    _XSDLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wsse.xsd', 56, 1)
    _ElementMap = EncodedString._ElementMap.copy()
    _AttributeMap = EncodedString._AttributeMap.copy()
    # Base type is EncodedString
    
    # Attribute EncodingType inherited from {http://docs.oasis-open.org/wss/2004/01/oasis-200401-wss-wssecurity-secext-1.0.xsd}EncodedString
    
    # Attribute ValueType uses Python identifier ValueType
    __ValueType = pyxb.binding.content.AttributeUse(pyxb.namespace.ExpandedName(None, 'ValueType'), 'ValueType', '__httpdocs_oasis_open_orgwss200401oasis_200401_wss_wssecurity_secext_1_0_xsd_BinarySecurityTokenType_ValueType', pyxb.binding.datatypes.anyURI)
    __ValueType._DeclarationLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wsse.xsd', 62, 4)
    __ValueType._UseLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wsse.xsd', 62, 4)
    
    ValueType = property(__ValueType.value, __ValueType.set, None, None)

    
    # Attribute Id inherited from {http://docs.oasis-open.org/wss/2004/01/oasis-200401-wss-wssecurity-secext-1.0.xsd}AttributedString
    _AttributeWildcard = pyxb.binding.content.Wildcard(process_contents=pyxb.binding.content.Wildcard.PC_lax, namespace_constraint=(pyxb.binding.content.Wildcard.NC_not, 'http://docs.oasis-open.org/wss/2004/01/oasis-200401-wss-wssecurity-secext-1.0.xsd'))
    _ElementMap.update({
        
    })
    _AttributeMap.update({
        __ValueType.name() : __ValueType
    })
Namespace.addCategoryObject('typeBinding', 'BinarySecurityTokenType', BinarySecurityTokenType)


# Complex type {http://docs.oasis-open.org/wss/2004/01/oasis-200401-wss-wssecurity-secext-1.0.xsd}KeyIdentifierType with content type SIMPLE
class KeyIdentifierType (EncodedString):
    """A security token key identifier"""
    _TypeDefinition = pyxb.binding.datatypes.string
    _ContentTypeTag = pyxb.binding.basis.complexTypeDefinition._CT_SIMPLE
    _Abstract = False
    _ExpandedName = pyxb.namespace.ExpandedName(Namespace, 'KeyIdentifierType')
    _XSDLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wsse.xsd', 66, 1)
    _ElementMap = EncodedString._ElementMap.copy()
    _AttributeMap = EncodedString._AttributeMap.copy()
    # Base type is EncodedString
    
    # Attribute EncodingType inherited from {http://docs.oasis-open.org/wss/2004/01/oasis-200401-wss-wssecurity-secext-1.0.xsd}EncodedString
    
    # Attribute ValueType uses Python identifier ValueType
    __ValueType = pyxb.binding.content.AttributeUse(pyxb.namespace.ExpandedName(None, 'ValueType'), 'ValueType', '__httpdocs_oasis_open_orgwss200401oasis_200401_wss_wssecurity_secext_1_0_xsd_KeyIdentifierType_ValueType', pyxb.binding.datatypes.anyURI)
    __ValueType._DeclarationLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wsse.xsd', 72, 4)
    __ValueType._UseLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wsse.xsd', 72, 4)
    
    ValueType = property(__ValueType.value, __ValueType.set, None, None)

    
    # Attribute Id inherited from {http://docs.oasis-open.org/wss/2004/01/oasis-200401-wss-wssecurity-secext-1.0.xsd}AttributedString
    _AttributeWildcard = pyxb.binding.content.Wildcard(process_contents=pyxb.binding.content.Wildcard.PC_lax, namespace_constraint=(pyxb.binding.content.Wildcard.NC_not, 'http://docs.oasis-open.org/wss/2004/01/oasis-200401-wss-wssecurity-secext-1.0.xsd'))
    _ElementMap.update({
        
    })
    _AttributeMap.update({
        __ValueType.name() : __ValueType
    })
Namespace.addCategoryObject('typeBinding', 'KeyIdentifierType', KeyIdentifierType)


UsernameToken = pyxb.binding.basis.element(pyxb.namespace.ExpandedName(Namespace, 'UsernameToken'), UsernameTokenType, documentation='This element defines the wsse:UsernameToken element per Section 4.1.', location=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wsse.xsd', 142, 1))
Namespace.addCategoryObject('elementBinding', UsernameToken.name().localName(), UsernameToken)

Reference = pyxb.binding.basis.element(pyxb.namespace.ExpandedName(Namespace, 'Reference'), ReferenceType, documentation='This element defines a security token reference', location=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wsse.xsd', 152, 1))
Namespace.addCategoryObject('elementBinding', Reference.name().localName(), Reference)

Embedded = pyxb.binding.basis.element(pyxb.namespace.ExpandedName(Namespace, 'Embedded'), EmbeddedType, documentation='This element defines a security token embedded reference', location=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wsse.xsd', 157, 1))
Namespace.addCategoryObject('elementBinding', Embedded.name().localName(), Embedded)

Security = pyxb.binding.basis.element(pyxb.namespace.ExpandedName(Namespace, 'Security'), SecurityHeaderType, documentation='This element defines the wsse:Security SOAP header element per Section 4.', location=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wsse.xsd', 172, 1))
Namespace.addCategoryObject('elementBinding', Security.name().localName(), Security)

TransformationParameters = pyxb.binding.basis.element(pyxb.namespace.ExpandedName(Namespace, 'TransformationParameters'), TransformationParametersType, documentation='This element contains properties for transformations from any namespace, including DSIG.', location=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wsse.xsd', 177, 1))
Namespace.addCategoryObject('elementBinding', TransformationParameters.name().localName(), TransformationParameters)

SecurityTokenReference = pyxb.binding.basis.element(pyxb.namespace.ExpandedName(Namespace, 'SecurityTokenReference'), SecurityTokenReferenceType, documentation='This element defines the wsse:SecurityTokenReference per Section 4.3.', location=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wsse.xsd', 167, 1))
Namespace.addCategoryObject('elementBinding', SecurityTokenReference.name().localName(), SecurityTokenReference)

Password = pyxb.binding.basis.element(pyxb.namespace.ExpandedName(Namespace, 'Password'), PasswordString, location=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wsse.xsd', 182, 1))
Namespace.addCategoryObject('elementBinding', Password.name().localName(), Password)

Nonce = pyxb.binding.basis.element(pyxb.namespace.ExpandedName(Namespace, 'Nonce'), EncodedString, location=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wsse.xsd', 183, 1))
Namespace.addCategoryObject('elementBinding', Nonce.name().localName(), Nonce)

BinarySecurityToken = pyxb.binding.basis.element(pyxb.namespace.ExpandedName(Namespace, 'BinarySecurityToken'), BinarySecurityTokenType, documentation='This element defines the wsse:BinarySecurityToken element per Section 4.2.', location=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wsse.xsd', 147, 1))
Namespace.addCategoryObject('elementBinding', BinarySecurityToken.name().localName(), BinarySecurityToken)

KeyIdentifier = pyxb.binding.basis.element(pyxb.namespace.ExpandedName(Namespace, 'KeyIdentifier'), KeyIdentifierType, documentation='This element defines a key identifier reference', location=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wsse.xsd', 162, 1))
Namespace.addCategoryObject('elementBinding', KeyIdentifier.name().localName(), KeyIdentifier)



UsernameTokenType._AddElement(pyxb.binding.basis.element(pyxb.namespace.ExpandedName(Namespace, 'Username'), AttributedString, scope=UsernameTokenType, location=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wsse.xsd', 50, 3)))

def _BuildAutomaton ():
    # Remove this helper function from the namespace after it is invoked
    global _BuildAutomaton
    del _BuildAutomaton
    import pyxb.utils.fac as fac

    counters = set()
    cc_0 = fac.CounterCondition(min=0, max=None, metadata=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wsse.xsd', 51, 3))
    counters.add(cc_0)
    states = []
    final_update = set()
    symbol = pyxb.binding.content.ElementUse(UsernameTokenType._UseForTag(pyxb.namespace.ExpandedName(Namespace, 'Username')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wsse.xsd', 50, 3))
    st_0 = fac.State(symbol, is_initial=True, final_update=final_update, is_unordered_catenation=False)
    states.append(st_0)
    final_update = set()
    final_update.add(fac.UpdateInstruction(cc_0, False))
    symbol = pyxb.binding.content.WildcardUse(pyxb.binding.content.Wildcard(process_contents=pyxb.binding.content.Wildcard.PC_lax, namespace_constraint=pyxb.binding.content.Wildcard.NC_any), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wsse.xsd', 51, 3))
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
UsernameTokenType._Automaton = _BuildAutomaton()




def _BuildAutomaton_ ():
    # Remove this helper function from the namespace after it is invoked
    global _BuildAutomaton_
    del _BuildAutomaton_
    import pyxb.utils.fac as fac

    counters = set()
    cc_0 = fac.CounterCondition(min=0, max=None, metadata=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wsse.xsd', 99, 2))
    counters.add(cc_0)
    states = []
    final_update = set()
    final_update.add(fac.UpdateInstruction(cc_0, False))
    symbol = pyxb.binding.content.WildcardUse(pyxb.binding.content.Wildcard(process_contents=pyxb.binding.content.Wildcard.PC_lax, namespace_constraint=pyxb.binding.content.Wildcard.NC_any), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wsse.xsd', 100, 3))
    st_0 = fac.State(symbol, is_initial=True, final_update=final_update, is_unordered_catenation=False)
    states.append(st_0)
    transitions = []
    transitions.append(fac.Transition(st_0, [
        fac.UpdateInstruction(cc_0, True) ]))
    st_0._set_transitionSet(transitions)
    return fac.Automaton(states, counters, True, containing_state=None)
EmbeddedType._Automaton = _BuildAutomaton_()




def _BuildAutomaton_2 ():
    # Remove this helper function from the namespace after it is invoked
    global _BuildAutomaton_2
    del _BuildAutomaton_2
    import pyxb.utils.fac as fac

    counters = set()
    cc_0 = fac.CounterCondition(min=0, max=None, metadata=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wsse.xsd', 121, 3))
    counters.add(cc_0)
    states = []
    final_update = set()
    final_update.add(fac.UpdateInstruction(cc_0, False))
    symbol = pyxb.binding.content.WildcardUse(pyxb.binding.content.Wildcard(process_contents=pyxb.binding.content.Wildcard.PC_lax, namespace_constraint=pyxb.binding.content.Wildcard.NC_any), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wsse.xsd', 121, 3))
    st_0 = fac.State(symbol, is_initial=True, final_update=final_update, is_unordered_catenation=False)
    states.append(st_0)
    transitions = []
    transitions.append(fac.Transition(st_0, [
        fac.UpdateInstruction(cc_0, True) ]))
    st_0._set_transitionSet(transitions)
    return fac.Automaton(states, counters, True, containing_state=None)
SecurityHeaderType._Automaton = _BuildAutomaton_2()




def _BuildAutomaton_3 ():
    # Remove this helper function from the namespace after it is invoked
    global _BuildAutomaton_3
    del _BuildAutomaton_3
    import pyxb.utils.fac as fac

    counters = set()
    cc_0 = fac.CounterCondition(min=0, max=None, metadata=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wsse.xsd', 134, 3))
    counters.add(cc_0)
    states = []
    final_update = set()
    final_update.add(fac.UpdateInstruction(cc_0, False))
    symbol = pyxb.binding.content.WildcardUse(pyxb.binding.content.Wildcard(process_contents=pyxb.binding.content.Wildcard.PC_lax, namespace_constraint=pyxb.binding.content.Wildcard.NC_any), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wsse.xsd', 134, 3))
    st_0 = fac.State(symbol, is_initial=True, final_update=final_update, is_unordered_catenation=False)
    states.append(st_0)
    transitions = []
    transitions.append(fac.Transition(st_0, [
        fac.UpdateInstruction(cc_0, True) ]))
    st_0._set_transitionSet(transitions)
    return fac.Automaton(states, counters, True, containing_state=None)
TransformationParametersType._Automaton = _BuildAutomaton_3()




def _BuildAutomaton_4 ():
    # Remove this helper function from the namespace after it is invoked
    global _BuildAutomaton_4
    del _BuildAutomaton_4
    import pyxb.utils.fac as fac

    counters = set()
    cc_0 = fac.CounterCondition(min=0, max=None, metadata=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wsse.xsd', 109, 2))
    counters.add(cc_0)
    states = []
    final_update = set()
    final_update.add(fac.UpdateInstruction(cc_0, False))
    symbol = pyxb.binding.content.WildcardUse(pyxb.binding.content.Wildcard(process_contents=pyxb.binding.content.Wildcard.PC_lax, namespace_constraint=pyxb.binding.content.Wildcard.NC_any), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wsse.xsd', 110, 3))
    st_0 = fac.State(symbol, is_initial=True, final_update=final_update, is_unordered_catenation=False)
    states.append(st_0)
    transitions = []
    transitions.append(fac.Transition(st_0, [
        fac.UpdateInstruction(cc_0, True) ]))
    st_0._set_transitionSet(transitions)
    return fac.Automaton(states, counters, True, containing_state=None)
SecurityTokenReferenceType._Automaton = _BuildAutomaton_4()

