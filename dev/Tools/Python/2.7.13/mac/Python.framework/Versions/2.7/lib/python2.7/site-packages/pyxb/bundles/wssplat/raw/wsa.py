# ./pyxb/bundles/wssplat/raw/wsa.py
# -*- coding: utf-8 -*-
# PyXB bindings for NM:0ecbd27a42302a2dbf33a51269e14ce6419c0738
# Generated 2014-10-19 06:24:58.939128 by PyXB version 1.2.4 using Python 2.7.3.final.0
# Namespace http://www.w3.org/2005/08/addressing

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
_GenerationUID = pyxb.utils.utility.UniqueIdentifier('urn:uuid:8f472436-5782-11e4-8766-c8600024e903')

# Version of PyXB used to generate the bindings
_PyXBVersion = '1.2.4'
# Generated bindings are not compatible across PyXB versions
if pyxb.__version__ != _PyXBVersion:
    raise pyxb.PyXBVersionError(_PyXBVersion)

# Import bindings for namespaces imported into schema
import pyxb.binding.datatypes

# NOTE: All namespace declarations are reserved within the binding
Namespace = pyxb.namespace.NamespaceForURI('http://www.w3.org/2005/08/addressing', create_if_missing=True)
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


# Atomic simple type: {http://www.w3.org/2005/08/addressing}RelationshipType
class RelationshipType (pyxb.binding.datatypes.anyURI, pyxb.binding.basis.enumeration_mixin):

    """An atomic simple type."""

    _ExpandedName = pyxb.namespace.ExpandedName(Namespace, 'RelationshipType')
    _XSDLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wsa.xsd', 64, 1)
    _Documentation = None
RelationshipType._CF_enumeration = pyxb.binding.facets.CF_enumeration(value_datatype=RelationshipType, enum_prefix=None)
RelationshipType.httpwww_w3_org200508addressingreply = RelationshipType._CF_enumeration.addEnumeration(unicode_value='http://www.w3.org/2005/08/addressing/reply', tag='httpwww_w3_org200508addressingreply')
RelationshipType._InitializeFacetMap(RelationshipType._CF_enumeration)
Namespace.addCategoryObject('typeBinding', 'RelationshipType', RelationshipType)

# Atomic simple type: {http://www.w3.org/2005/08/addressing}FaultCodesType
class FaultCodesType (pyxb.binding.datatypes.QName, pyxb.binding.basis.enumeration_mixin):

    """An atomic simple type."""

    _ExpandedName = pyxb.namespace.ExpandedName(Namespace, 'FaultCodesType')
    _XSDLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wsa.xsd', 92, 1)
    _Documentation = None
FaultCodesType._CF_enumeration = pyxb.binding.facets.CF_enumeration(value_datatype=FaultCodesType, enum_prefix=None)
FaultCodesType._CF_enumeration.addEnumeration(value=pyxb.namespace.ExpandedName(Namespace, 'InvalidAddressingHeader'), tag=None)
FaultCodesType._CF_enumeration.addEnumeration(value=pyxb.namespace.ExpandedName(Namespace, 'InvalidAddress'), tag=None)
FaultCodesType._CF_enumeration.addEnumeration(value=pyxb.namespace.ExpandedName(Namespace, 'InvalidEPR'), tag=None)
FaultCodesType._CF_enumeration.addEnumeration(value=pyxb.namespace.ExpandedName(Namespace, 'InvalidCardinality'), tag=None)
FaultCodesType._CF_enumeration.addEnumeration(value=pyxb.namespace.ExpandedName(Namespace, 'MissingAddressInEPR'), tag=None)
FaultCodesType._CF_enumeration.addEnumeration(value=pyxb.namespace.ExpandedName(Namespace, 'DuplicateMessageID'), tag=None)
FaultCodesType._CF_enumeration.addEnumeration(value=pyxb.namespace.ExpandedName(Namespace, 'ActionMismatch'), tag=None)
FaultCodesType._CF_enumeration.addEnumeration(value=pyxb.namespace.ExpandedName(Namespace, 'MessageAddressingHeaderRequired'), tag=None)
FaultCodesType._CF_enumeration.addEnumeration(value=pyxb.namespace.ExpandedName(Namespace, 'DestinationUnreachable'), tag=None)
FaultCodesType._CF_enumeration.addEnumeration(value=pyxb.namespace.ExpandedName(Namespace, 'ActionNotSupported'), tag=None)
FaultCodesType._CF_enumeration.addEnumeration(value=pyxb.namespace.ExpandedName(Namespace, 'EndpointUnavailable'), tag=None)
FaultCodesType._InitializeFacetMap(FaultCodesType._CF_enumeration)
Namespace.addCategoryObject('typeBinding', 'FaultCodesType', FaultCodesType)

# Union simple type: {http://www.w3.org/2005/08/addressing}RelationshipTypeOpenEnum
# superclasses pyxb.binding.datatypes.anySimpleType
class RelationshipTypeOpenEnum (pyxb.binding.basis.STD_union):

    """Simple type that is a union of RelationshipType, pyxb.binding.datatypes.anyURI."""

    _ExpandedName = pyxb.namespace.ExpandedName(Namespace, 'RelationshipTypeOpenEnum')
    _XSDLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wsa.xsd', 60, 1)
    _Documentation = None

    _MemberTypes = ( RelationshipType, pyxb.binding.datatypes.anyURI, )
RelationshipTypeOpenEnum._CF_enumeration = pyxb.binding.facets.CF_enumeration(value_datatype=RelationshipTypeOpenEnum)
RelationshipTypeOpenEnum._CF_pattern = pyxb.binding.facets.CF_pattern()
RelationshipTypeOpenEnum.httpwww_w3_org200508addressingreply = 'http://www.w3.org/2005/08/addressing/reply'# originally RelationshipType.httpwww_w3_org200508addressingreply
RelationshipTypeOpenEnum._InitializeFacetMap(RelationshipTypeOpenEnum._CF_enumeration,
   RelationshipTypeOpenEnum._CF_pattern)
Namespace.addCategoryObject('typeBinding', 'RelationshipTypeOpenEnum', RelationshipTypeOpenEnum)

# Union simple type: {http://www.w3.org/2005/08/addressing}FaultCodesOpenEnumType
# superclasses pyxb.binding.datatypes.anySimpleType
class FaultCodesOpenEnumType (pyxb.binding.basis.STD_union):

    """Simple type that is a union of FaultCodesType, pyxb.binding.datatypes.QName."""

    _ExpandedName = pyxb.namespace.ExpandedName(Namespace, 'FaultCodesOpenEnumType')
    _XSDLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wsa.xsd', 88, 1)
    _Documentation = None

    _MemberTypes = ( FaultCodesType, pyxb.binding.datatypes.QName, )
FaultCodesOpenEnumType._CF_enumeration = pyxb.binding.facets.CF_enumeration(value_datatype=FaultCodesOpenEnumType)
FaultCodesOpenEnumType._CF_pattern = pyxb.binding.facets.CF_pattern()
FaultCodesOpenEnumType._InitializeFacetMap(FaultCodesOpenEnumType._CF_enumeration,
   FaultCodesOpenEnumType._CF_pattern)
Namespace.addCategoryObject('typeBinding', 'FaultCodesOpenEnumType', FaultCodesOpenEnumType)

# Complex type {http://www.w3.org/2005/08/addressing}EndpointReferenceType with content type ELEMENT_ONLY
class EndpointReferenceType (pyxb.binding.basis.complexTypeDefinition):
    """Complex type {http://www.w3.org/2005/08/addressing}EndpointReferenceType with content type ELEMENT_ONLY"""
    _TypeDefinition = None
    _ContentTypeTag = pyxb.binding.basis.complexTypeDefinition._CT_ELEMENT_ONLY
    _Abstract = False
    _ExpandedName = pyxb.namespace.ExpandedName(Namespace, 'EndpointReferenceType')
    _XSDLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wsa.xsd', 23, 1)
    _ElementMap = {}
    _AttributeMap = {}
    # Base type is pyxb.binding.datatypes.anyType
    
    # Element {http://www.w3.org/2005/08/addressing}Address uses Python identifier Address
    __Address = pyxb.binding.content.ElementDeclaration(pyxb.namespace.ExpandedName(Namespace, 'Address'), 'Address', '__httpwww_w3_org200508addressing_EndpointReferenceType_httpwww_w3_org200508addressingAddress', False, pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wsa.xsd', 25, 3), )

    
    Address = property(__Address.value, __Address.set, None, None)

    
    # Element {http://www.w3.org/2005/08/addressing}ReferenceParameters uses Python identifier ReferenceParameters
    __ReferenceParameters = pyxb.binding.content.ElementDeclaration(pyxb.namespace.ExpandedName(Namespace, 'ReferenceParameters'), 'ReferenceParameters', '__httpwww_w3_org200508addressing_EndpointReferenceType_httpwww_w3_org200508addressingReferenceParameters', False, pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wsa.xsd', 33, 1), )

    
    ReferenceParameters = property(__ReferenceParameters.value, __ReferenceParameters.set, None, None)

    
    # Element {http://www.w3.org/2005/08/addressing}Metadata uses Python identifier Metadata
    __Metadata = pyxb.binding.content.ElementDeclaration(pyxb.namespace.ExpandedName(Namespace, 'Metadata'), 'Metadata', '__httpwww_w3_org200508addressing_EndpointReferenceType_httpwww_w3_org200508addressingMetadata', False, pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wsa.xsd', 41, 1), )

    
    Metadata = property(__Metadata.value, __Metadata.set, None, None)

    _AttributeWildcard = pyxb.binding.content.Wildcard(process_contents=pyxb.binding.content.Wildcard.PC_lax, namespace_constraint=(pyxb.binding.content.Wildcard.NC_not, 'http://www.w3.org/2005/08/addressing'))
    _HasWildcardElement = True
    _ElementMap.update({
        __Address.name() : __Address,
        __ReferenceParameters.name() : __ReferenceParameters,
        __Metadata.name() : __Metadata
    })
    _AttributeMap.update({
        
    })
Namespace.addCategoryObject('typeBinding', 'EndpointReferenceType', EndpointReferenceType)


# Complex type {http://www.w3.org/2005/08/addressing}ReferenceParametersType with content type ELEMENT_ONLY
class ReferenceParametersType (pyxb.binding.basis.complexTypeDefinition):
    """Complex type {http://www.w3.org/2005/08/addressing}ReferenceParametersType with content type ELEMENT_ONLY"""
    _TypeDefinition = None
    _ContentTypeTag = pyxb.binding.basis.complexTypeDefinition._CT_ELEMENT_ONLY
    _Abstract = False
    _ExpandedName = pyxb.namespace.ExpandedName(Namespace, 'ReferenceParametersType')
    _XSDLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wsa.xsd', 34, 1)
    _ElementMap = {}
    _AttributeMap = {}
    # Base type is pyxb.binding.datatypes.anyType
    _AttributeWildcard = pyxb.binding.content.Wildcard(process_contents=pyxb.binding.content.Wildcard.PC_lax, namespace_constraint=(pyxb.binding.content.Wildcard.NC_not, 'http://www.w3.org/2005/08/addressing'))
    _HasWildcardElement = True
    _ElementMap.update({
        
    })
    _AttributeMap.update({
        
    })
Namespace.addCategoryObject('typeBinding', 'ReferenceParametersType', ReferenceParametersType)


# Complex type {http://www.w3.org/2005/08/addressing}MetadataType with content type ELEMENT_ONLY
class MetadataType (pyxb.binding.basis.complexTypeDefinition):
    """Complex type {http://www.w3.org/2005/08/addressing}MetadataType with content type ELEMENT_ONLY"""
    _TypeDefinition = None
    _ContentTypeTag = pyxb.binding.basis.complexTypeDefinition._CT_ELEMENT_ONLY
    _Abstract = False
    _ExpandedName = pyxb.namespace.ExpandedName(Namespace, 'MetadataType')
    _XSDLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wsa.xsd', 42, 1)
    _ElementMap = {}
    _AttributeMap = {}
    # Base type is pyxb.binding.datatypes.anyType
    _AttributeWildcard = pyxb.binding.content.Wildcard(process_contents=pyxb.binding.content.Wildcard.PC_lax, namespace_constraint=(pyxb.binding.content.Wildcard.NC_not, 'http://www.w3.org/2005/08/addressing'))
    _HasWildcardElement = True
    _ElementMap.update({
        
    })
    _AttributeMap.update({
        
    })
Namespace.addCategoryObject('typeBinding', 'MetadataType', MetadataType)


# Complex type {http://www.w3.org/2005/08/addressing}AttributedURIType with content type SIMPLE
class AttributedURIType (pyxb.binding.basis.complexTypeDefinition):
    """Complex type {http://www.w3.org/2005/08/addressing}AttributedURIType with content type SIMPLE"""
    _TypeDefinition = pyxb.binding.datatypes.anyURI
    _ContentTypeTag = pyxb.binding.basis.complexTypeDefinition._CT_SIMPLE
    _Abstract = False
    _ExpandedName = pyxb.namespace.ExpandedName(Namespace, 'AttributedURIType')
    _XSDLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wsa.xsd', 76, 1)
    _ElementMap = {}
    _AttributeMap = {}
    # Base type is pyxb.binding.datatypes.anyURI
    _AttributeWildcard = pyxb.binding.content.Wildcard(process_contents=pyxb.binding.content.Wildcard.PC_lax, namespace_constraint=(pyxb.binding.content.Wildcard.NC_not, 'http://www.w3.org/2005/08/addressing'))
    _ElementMap.update({
        
    })
    _AttributeMap.update({
        
    })
Namespace.addCategoryObject('typeBinding', 'AttributedURIType', AttributedURIType)


# Complex type {http://www.w3.org/2005/08/addressing}AttributedUnsignedLongType with content type SIMPLE
class AttributedUnsignedLongType (pyxb.binding.basis.complexTypeDefinition):
    """Complex type {http://www.w3.org/2005/08/addressing}AttributedUnsignedLongType with content type SIMPLE"""
    _TypeDefinition = pyxb.binding.datatypes.unsignedLong
    _ContentTypeTag = pyxb.binding.basis.complexTypeDefinition._CT_SIMPLE
    _Abstract = False
    _ExpandedName = pyxb.namespace.ExpandedName(Namespace, 'AttributedUnsignedLongType')
    _XSDLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wsa.xsd', 109, 1)
    _ElementMap = {}
    _AttributeMap = {}
    # Base type is pyxb.binding.datatypes.unsignedLong
    _AttributeWildcard = pyxb.binding.content.Wildcard(process_contents=pyxb.binding.content.Wildcard.PC_lax, namespace_constraint=(pyxb.binding.content.Wildcard.NC_not, 'http://www.w3.org/2005/08/addressing'))
    _ElementMap.update({
        
    })
    _AttributeMap.update({
        
    })
Namespace.addCategoryObject('typeBinding', 'AttributedUnsignedLongType', AttributedUnsignedLongType)


# Complex type {http://www.w3.org/2005/08/addressing}AttributedQNameType with content type SIMPLE
class AttributedQNameType (pyxb.binding.basis.complexTypeDefinition):
    """Complex type {http://www.w3.org/2005/08/addressing}AttributedQNameType with content type SIMPLE"""
    _TypeDefinition = pyxb.binding.datatypes.QName
    _ContentTypeTag = pyxb.binding.basis.complexTypeDefinition._CT_SIMPLE
    _Abstract = False
    _ExpandedName = pyxb.namespace.ExpandedName(Namespace, 'AttributedQNameType')
    _XSDLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wsa.xsd', 118, 1)
    _ElementMap = {}
    _AttributeMap = {}
    # Base type is pyxb.binding.datatypes.QName
    _AttributeWildcard = pyxb.binding.content.Wildcard(process_contents=pyxb.binding.content.Wildcard.PC_lax, namespace_constraint=(pyxb.binding.content.Wildcard.NC_not, 'http://www.w3.org/2005/08/addressing'))
    _ElementMap.update({
        
    })
    _AttributeMap.update({
        
    })
Namespace.addCategoryObject('typeBinding', 'AttributedQNameType', AttributedQNameType)


# Complex type {http://www.w3.org/2005/08/addressing}ProblemActionType with content type ELEMENT_ONLY
class ProblemActionType (pyxb.binding.basis.complexTypeDefinition):
    """Complex type {http://www.w3.org/2005/08/addressing}ProblemActionType with content type ELEMENT_ONLY"""
    _TypeDefinition = None
    _ContentTypeTag = pyxb.binding.basis.complexTypeDefinition._CT_ELEMENT_ONLY
    _Abstract = False
    _ExpandedName = pyxb.namespace.ExpandedName(Namespace, 'ProblemActionType')
    _XSDLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wsa.xsd', 129, 1)
    _ElementMap = {}
    _AttributeMap = {}
    # Base type is pyxb.binding.datatypes.anyType
    
    # Element {http://www.w3.org/2005/08/addressing}Action uses Python identifier Action
    __Action = pyxb.binding.content.ElementDeclaration(pyxb.namespace.ExpandedName(Namespace, 'Action'), 'Action', '__httpwww_w3_org200508addressing_ProblemActionType_httpwww_w3_org200508addressingAction', False, pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wsa.xsd', 74, 1), )

    
    Action = property(__Action.value, __Action.set, None, None)

    
    # Element {http://www.w3.org/2005/08/addressing}SoapAction uses Python identifier SoapAction
    __SoapAction = pyxb.binding.content.ElementDeclaration(pyxb.namespace.ExpandedName(Namespace, 'SoapAction'), 'SoapAction', '__httpwww_w3_org200508addressing_ProblemActionType_httpwww_w3_org200508addressingSoapAction', False, pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wsa.xsd', 132, 3), )

    
    SoapAction = property(__SoapAction.value, __SoapAction.set, None, None)

    _AttributeWildcard = pyxb.binding.content.Wildcard(process_contents=pyxb.binding.content.Wildcard.PC_lax, namespace_constraint=(pyxb.binding.content.Wildcard.NC_not, 'http://www.w3.org/2005/08/addressing'))
    _ElementMap.update({
        __Action.name() : __Action,
        __SoapAction.name() : __SoapAction
    })
    _AttributeMap.update({
        
    })
Namespace.addCategoryObject('typeBinding', 'ProblemActionType', ProblemActionType)


# Complex type {http://www.w3.org/2005/08/addressing}RelatesToType with content type SIMPLE
class RelatesToType (pyxb.binding.basis.complexTypeDefinition):
    """Complex type {http://www.w3.org/2005/08/addressing}RelatesToType with content type SIMPLE"""
    _TypeDefinition = pyxb.binding.datatypes.anyURI
    _ContentTypeTag = pyxb.binding.basis.complexTypeDefinition._CT_SIMPLE
    _Abstract = False
    _ExpandedName = pyxb.namespace.ExpandedName(Namespace, 'RelatesToType')
    _XSDLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wsa.xsd', 51, 1)
    _ElementMap = {}
    _AttributeMap = {}
    # Base type is pyxb.binding.datatypes.anyURI
    
    # Attribute RelationshipType uses Python identifier RelationshipType
    __RelationshipType = pyxb.binding.content.AttributeUse(pyxb.namespace.ExpandedName(None, 'RelationshipType'), 'RelationshipType', '__httpwww_w3_org200508addressing_RelatesToType_RelationshipType', RelationshipTypeOpenEnum, unicode_default='http://www.w3.org/2005/08/addressing/reply')
    __RelationshipType._DeclarationLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wsa.xsd', 54, 4)
    __RelationshipType._UseLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wsa.xsd', 54, 4)
    
    RelationshipType = property(__RelationshipType.value, __RelationshipType.set, None, None)

    _AttributeWildcard = pyxb.binding.content.Wildcard(process_contents=pyxb.binding.content.Wildcard.PC_lax, namespace_constraint=(pyxb.binding.content.Wildcard.NC_not, 'http://www.w3.org/2005/08/addressing'))
    _ElementMap.update({
        
    })
    _AttributeMap.update({
        __RelationshipType.name() : __RelationshipType
    })
Namespace.addCategoryObject('typeBinding', 'RelatesToType', RelatesToType)


EndpointReference = pyxb.binding.basis.element(pyxb.namespace.ExpandedName(Namespace, 'EndpointReference'), EndpointReferenceType, location=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wsa.xsd', 22, 1))
Namespace.addCategoryObject('elementBinding', EndpointReference.name().localName(), EndpointReference)

ReferenceParameters = pyxb.binding.basis.element(pyxb.namespace.ExpandedName(Namespace, 'ReferenceParameters'), ReferenceParametersType, location=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wsa.xsd', 33, 1))
Namespace.addCategoryObject('elementBinding', ReferenceParameters.name().localName(), ReferenceParameters)

Metadata = pyxb.binding.basis.element(pyxb.namespace.ExpandedName(Namespace, 'Metadata'), MetadataType, location=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wsa.xsd', 41, 1))
Namespace.addCategoryObject('elementBinding', Metadata.name().localName(), Metadata)

MessageID = pyxb.binding.basis.element(pyxb.namespace.ExpandedName(Namespace, 'MessageID'), AttributedURIType, location=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wsa.xsd', 49, 1))
Namespace.addCategoryObject('elementBinding', MessageID.name().localName(), MessageID)

ReplyTo = pyxb.binding.basis.element(pyxb.namespace.ExpandedName(Namespace, 'ReplyTo'), EndpointReferenceType, location=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wsa.xsd', 70, 1))
Namespace.addCategoryObject('elementBinding', ReplyTo.name().localName(), ReplyTo)

From = pyxb.binding.basis.element(pyxb.namespace.ExpandedName(Namespace, 'From'), EndpointReferenceType, location=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wsa.xsd', 71, 1))
Namespace.addCategoryObject('elementBinding', From.name().localName(), From)

FaultTo = pyxb.binding.basis.element(pyxb.namespace.ExpandedName(Namespace, 'FaultTo'), EndpointReferenceType, location=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wsa.xsd', 72, 1))
Namespace.addCategoryObject('elementBinding', FaultTo.name().localName(), FaultTo)

To = pyxb.binding.basis.element(pyxb.namespace.ExpandedName(Namespace, 'To'), AttributedURIType, location=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wsa.xsd', 73, 1))
Namespace.addCategoryObject('elementBinding', To.name().localName(), To)

Action = pyxb.binding.basis.element(pyxb.namespace.ExpandedName(Namespace, 'Action'), AttributedURIType, location=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wsa.xsd', 74, 1))
Namespace.addCategoryObject('elementBinding', Action.name().localName(), Action)

RetryAfter = pyxb.binding.basis.element(pyxb.namespace.ExpandedName(Namespace, 'RetryAfter'), AttributedUnsignedLongType, location=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wsa.xsd', 108, 1))
Namespace.addCategoryObject('elementBinding', RetryAfter.name().localName(), RetryAfter)

ProblemHeaderQName = pyxb.binding.basis.element(pyxb.namespace.ExpandedName(Namespace, 'ProblemHeaderQName'), AttributedQNameType, location=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wsa.xsd', 117, 1))
Namespace.addCategoryObject('elementBinding', ProblemHeaderQName.name().localName(), ProblemHeaderQName)

ProblemIRI = pyxb.binding.basis.element(pyxb.namespace.ExpandedName(Namespace, 'ProblemIRI'), AttributedURIType, location=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wsa.xsd', 126, 1))
Namespace.addCategoryObject('elementBinding', ProblemIRI.name().localName(), ProblemIRI)

ProblemAction = pyxb.binding.basis.element(pyxb.namespace.ExpandedName(Namespace, 'ProblemAction'), ProblemActionType, location=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wsa.xsd', 128, 1))
Namespace.addCategoryObject('elementBinding', ProblemAction.name().localName(), ProblemAction)

RelatesTo = pyxb.binding.basis.element(pyxb.namespace.ExpandedName(Namespace, 'RelatesTo'), RelatesToType, location=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wsa.xsd', 50, 1))
Namespace.addCategoryObject('elementBinding', RelatesTo.name().localName(), RelatesTo)



EndpointReferenceType._AddElement(pyxb.binding.basis.element(pyxb.namespace.ExpandedName(Namespace, 'Address'), AttributedURIType, scope=EndpointReferenceType, location=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wsa.xsd', 25, 3)))

EndpointReferenceType._AddElement(pyxb.binding.basis.element(pyxb.namespace.ExpandedName(Namespace, 'ReferenceParameters'), ReferenceParametersType, scope=EndpointReferenceType, location=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wsa.xsd', 33, 1)))

EndpointReferenceType._AddElement(pyxb.binding.basis.element(pyxb.namespace.ExpandedName(Namespace, 'Metadata'), MetadataType, scope=EndpointReferenceType, location=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wsa.xsd', 41, 1)))

def _BuildAutomaton ():
    # Remove this helper function from the namespace after it is invoked
    global _BuildAutomaton
    del _BuildAutomaton
    import pyxb.utils.fac as fac

    counters = set()
    cc_0 = fac.CounterCondition(min=0, max=1, metadata=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wsa.xsd', 26, 3))
    counters.add(cc_0)
    cc_1 = fac.CounterCondition(min=0, max=1, metadata=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wsa.xsd', 27, 3))
    counters.add(cc_1)
    cc_2 = fac.CounterCondition(min=0, max=None, metadata=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wsa.xsd', 28, 3))
    counters.add(cc_2)
    states = []
    final_update = set()
    symbol = pyxb.binding.content.ElementUse(EndpointReferenceType._UseForTag(pyxb.namespace.ExpandedName(Namespace, 'Address')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wsa.xsd', 25, 3))
    st_0 = fac.State(symbol, is_initial=True, final_update=final_update, is_unordered_catenation=False)
    states.append(st_0)
    final_update = set()
    final_update.add(fac.UpdateInstruction(cc_0, False))
    symbol = pyxb.binding.content.ElementUse(EndpointReferenceType._UseForTag(pyxb.namespace.ExpandedName(Namespace, 'ReferenceParameters')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wsa.xsd', 26, 3))
    st_1 = fac.State(symbol, is_initial=False, final_update=final_update, is_unordered_catenation=False)
    states.append(st_1)
    final_update = set()
    final_update.add(fac.UpdateInstruction(cc_1, False))
    symbol = pyxb.binding.content.ElementUse(EndpointReferenceType._UseForTag(pyxb.namespace.ExpandedName(Namespace, 'Metadata')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wsa.xsd', 27, 3))
    st_2 = fac.State(symbol, is_initial=False, final_update=final_update, is_unordered_catenation=False)
    states.append(st_2)
    final_update = set()
    final_update.add(fac.UpdateInstruction(cc_2, False))
    symbol = pyxb.binding.content.WildcardUse(pyxb.binding.content.Wildcard(process_contents=pyxb.binding.content.Wildcard.PC_lax, namespace_constraint=(pyxb.binding.content.Wildcard.NC_not, 'http://www.w3.org/2005/08/addressing')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wsa.xsd', 28, 3))
    st_3 = fac.State(symbol, is_initial=False, final_update=final_update, is_unordered_catenation=False)
    states.append(st_3)
    transitions = []
    transitions.append(fac.Transition(st_1, [
         ]))
    transitions.append(fac.Transition(st_2, [
         ]))
    transitions.append(fac.Transition(st_3, [
         ]))
    st_0._set_transitionSet(transitions)
    transitions = []
    transitions.append(fac.Transition(st_1, [
        fac.UpdateInstruction(cc_0, True) ]))
    transitions.append(fac.Transition(st_2, [
        fac.UpdateInstruction(cc_0, False) ]))
    transitions.append(fac.Transition(st_3, [
        fac.UpdateInstruction(cc_0, False) ]))
    st_1._set_transitionSet(transitions)
    transitions = []
    transitions.append(fac.Transition(st_2, [
        fac.UpdateInstruction(cc_1, True) ]))
    transitions.append(fac.Transition(st_3, [
        fac.UpdateInstruction(cc_1, False) ]))
    st_2._set_transitionSet(transitions)
    transitions = []
    transitions.append(fac.Transition(st_3, [
        fac.UpdateInstruction(cc_2, True) ]))
    st_3._set_transitionSet(transitions)
    return fac.Automaton(states, counters, False, containing_state=None)
EndpointReferenceType._Automaton = _BuildAutomaton()




def _BuildAutomaton_ ():
    # Remove this helper function from the namespace after it is invoked
    global _BuildAutomaton_
    del _BuildAutomaton_
    import pyxb.utils.fac as fac

    counters = set()
    cc_0 = fac.CounterCondition(min=0, max=None, metadata=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wsa.xsd', 36, 3))
    counters.add(cc_0)
    states = []
    final_update = set()
    final_update.add(fac.UpdateInstruction(cc_0, False))
    symbol = pyxb.binding.content.WildcardUse(pyxb.binding.content.Wildcard(process_contents=pyxb.binding.content.Wildcard.PC_lax, namespace_constraint=pyxb.binding.content.Wildcard.NC_any), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wsa.xsd', 36, 3))
    st_0 = fac.State(symbol, is_initial=True, final_update=final_update, is_unordered_catenation=False)
    states.append(st_0)
    transitions = []
    transitions.append(fac.Transition(st_0, [
        fac.UpdateInstruction(cc_0, True) ]))
    st_0._set_transitionSet(transitions)
    return fac.Automaton(states, counters, True, containing_state=None)
ReferenceParametersType._Automaton = _BuildAutomaton_()




def _BuildAutomaton_2 ():
    # Remove this helper function from the namespace after it is invoked
    global _BuildAutomaton_2
    del _BuildAutomaton_2
    import pyxb.utils.fac as fac

    counters = set()
    cc_0 = fac.CounterCondition(min=0, max=None, metadata=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wsa.xsd', 44, 3))
    counters.add(cc_0)
    states = []
    final_update = set()
    final_update.add(fac.UpdateInstruction(cc_0, False))
    symbol = pyxb.binding.content.WildcardUse(pyxb.binding.content.Wildcard(process_contents=pyxb.binding.content.Wildcard.PC_lax, namespace_constraint=pyxb.binding.content.Wildcard.NC_any), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wsa.xsd', 44, 3))
    st_0 = fac.State(symbol, is_initial=True, final_update=final_update, is_unordered_catenation=False)
    states.append(st_0)
    transitions = []
    transitions.append(fac.Transition(st_0, [
        fac.UpdateInstruction(cc_0, True) ]))
    st_0._set_transitionSet(transitions)
    return fac.Automaton(states, counters, True, containing_state=None)
MetadataType._Automaton = _BuildAutomaton_2()




ProblemActionType._AddElement(pyxb.binding.basis.element(pyxb.namespace.ExpandedName(Namespace, 'Action'), AttributedURIType, scope=ProblemActionType, location=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wsa.xsd', 74, 1)))

ProblemActionType._AddElement(pyxb.binding.basis.element(pyxb.namespace.ExpandedName(Namespace, 'SoapAction'), pyxb.binding.datatypes.anyURI, scope=ProblemActionType, location=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wsa.xsd', 132, 3)))

def _BuildAutomaton_3 ():
    # Remove this helper function from the namespace after it is invoked
    global _BuildAutomaton_3
    del _BuildAutomaton_3
    import pyxb.utils.fac as fac

    counters = set()
    cc_0 = fac.CounterCondition(min=0, max=1, metadata=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wsa.xsd', 131, 3))
    counters.add(cc_0)
    cc_1 = fac.CounterCondition(min=0, max=1, metadata=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wsa.xsd', 132, 3))
    counters.add(cc_1)
    states = []
    final_update = set()
    final_update.add(fac.UpdateInstruction(cc_0, False))
    symbol = pyxb.binding.content.ElementUse(ProblemActionType._UseForTag(pyxb.namespace.ExpandedName(Namespace, 'Action')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wsa.xsd', 131, 3))
    st_0 = fac.State(symbol, is_initial=True, final_update=final_update, is_unordered_catenation=False)
    states.append(st_0)
    final_update = set()
    final_update.add(fac.UpdateInstruction(cc_1, False))
    symbol = pyxb.binding.content.ElementUse(ProblemActionType._UseForTag(pyxb.namespace.ExpandedName(Namespace, 'SoapAction')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wsa.xsd', 132, 3))
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
        fac.UpdateInstruction(cc_1, True) ]))
    st_1._set_transitionSet(transitions)
    return fac.Automaton(states, counters, True, containing_state=None)
ProblemActionType._Automaton = _BuildAutomaton_3()

