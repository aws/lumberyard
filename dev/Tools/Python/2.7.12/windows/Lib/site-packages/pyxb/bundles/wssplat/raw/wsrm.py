# ./pyxb/bundles/wssplat/raw/wsrm.py
# -*- coding: utf-8 -*-
# PyXB bindings for NM:51117966240f7c6b98d95f1b3ed6125f819a832c
# Generated 2014-10-19 06:24:59.959448 by PyXB version 1.2.4 using Python 2.7.3.final.0
# Namespace http://docs.oasis-open.org/ws-rx/wsrm/200702

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
_GenerationUID = pyxb.utils.utility.UniqueIdentifier('urn:uuid:8fdf6778-5782-11e4-a2b7-c8600024e903')

# Version of PyXB used to generate the bindings
_PyXBVersion = '1.2.4'
# Generated bindings are not compatible across PyXB versions
if pyxb.__version__ != _PyXBVersion:
    raise pyxb.PyXBVersionError(_PyXBVersion)

# Import bindings for namespaces imported into schema
import pyxb.bundles.wssplat.wsa
import pyxb.binding.datatypes

# NOTE: All namespace declarations are reserved within the binding
Namespace = pyxb.namespace.NamespaceForURI('http://docs.oasis-open.org/ws-rx/wsrm/200702', create_if_missing=True)
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


# Atomic simple type: {http://docs.oasis-open.org/ws-rx/wsrm/200702}MessageNumberType
class MessageNumberType (pyxb.binding.datatypes.unsignedLong):

    """An atomic simple type."""

    _ExpandedName = pyxb.namespace.ExpandedName(Namespace, 'MessageNumberType')
    _XSDLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wsrm.xsd', 81, 2)
    _Documentation = None
MessageNumberType._CF_maxInclusive = pyxb.binding.facets.CF_maxInclusive(value_datatype=MessageNumberType, value=pyxb.binding.datatypes.unsignedLong(9223372036854775807))
MessageNumberType._CF_minInclusive = pyxb.binding.facets.CF_minInclusive(value_datatype=MessageNumberType, value=pyxb.binding.datatypes.unsignedLong(1))
MessageNumberType._InitializeFacetMap(MessageNumberType._CF_maxInclusive,
   MessageNumberType._CF_minInclusive)
Namespace.addCategoryObject('typeBinding', 'MessageNumberType', MessageNumberType)

# Atomic simple type: {http://docs.oasis-open.org/ws-rx/wsrm/200702}FaultCodes
class FaultCodes (pyxb.binding.datatypes.QName, pyxb.binding.basis.enumeration_mixin):

    """An atomic simple type."""

    _ExpandedName = pyxb.namespace.ExpandedName(Namespace, 'FaultCodes')
    _XSDLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wsrm.xsd', 88, 2)
    _Documentation = None
FaultCodes._CF_enumeration = pyxb.binding.facets.CF_enumeration(value_datatype=FaultCodes, enum_prefix=None)
FaultCodes._CF_enumeration.addEnumeration(value=pyxb.namespace.ExpandedName(Namespace, 'SequenceTerminated'), tag=None)
FaultCodes._CF_enumeration.addEnumeration(value=pyxb.namespace.ExpandedName(Namespace, 'UnknownSequence'), tag=None)
FaultCodes._CF_enumeration.addEnumeration(value=pyxb.namespace.ExpandedName(Namespace, 'InvalidAcknowledgement'), tag=None)
FaultCodes._CF_enumeration.addEnumeration(value=pyxb.namespace.ExpandedName(Namespace, 'MessageNumberRollover'), tag=None)
FaultCodes._CF_enumeration.addEnumeration(value=pyxb.namespace.ExpandedName(Namespace, 'CreateSequenceRefused'), tag=None)
FaultCodes._CF_enumeration.addEnumeration(value=pyxb.namespace.ExpandedName(Namespace, 'SequenceClosed'), tag=None)
FaultCodes._CF_enumeration.addEnumeration(value=pyxb.namespace.ExpandedName(Namespace, 'WSRMRequired'), tag=None)
FaultCodes._InitializeFacetMap(FaultCodes._CF_enumeration)
Namespace.addCategoryObject('typeBinding', 'FaultCodes', FaultCodes)

# Atomic simple type: {http://docs.oasis-open.org/ws-rx/wsrm/200702}IncompleteSequenceBehaviorType
class IncompleteSequenceBehaviorType (pyxb.binding.datatypes.string, pyxb.binding.basis.enumeration_mixin):

    """An atomic simple type."""

    _ExpandedName = pyxb.namespace.ExpandedName(Namespace, 'IncompleteSequenceBehaviorType')
    _XSDLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wsrm.xsd', 202, 2)
    _Documentation = None
IncompleteSequenceBehaviorType._CF_enumeration = pyxb.binding.facets.CF_enumeration(value_datatype=IncompleteSequenceBehaviorType, enum_prefix=None)
IncompleteSequenceBehaviorType.DiscardEntireSequence = IncompleteSequenceBehaviorType._CF_enumeration.addEnumeration(unicode_value='DiscardEntireSequence', tag='DiscardEntireSequence')
IncompleteSequenceBehaviorType.DiscardFollowingFirstGap = IncompleteSequenceBehaviorType._CF_enumeration.addEnumeration(unicode_value='DiscardFollowingFirstGap', tag='DiscardFollowingFirstGap')
IncompleteSequenceBehaviorType.NoDiscard = IncompleteSequenceBehaviorType._CF_enumeration.addEnumeration(unicode_value='NoDiscard', tag='NoDiscard')
IncompleteSequenceBehaviorType._InitializeFacetMap(IncompleteSequenceBehaviorType._CF_enumeration)
Namespace.addCategoryObject('typeBinding', 'IncompleteSequenceBehaviorType', IncompleteSequenceBehaviorType)

# Atomic simple type: [anonymous]
class STD_ANON (pyxb.binding.datatypes.QName):

    """An atomic simple type."""

    _ExpandedName = None
    _XSDLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wsrm.xsd', 222, 4)
    _Documentation = None
STD_ANON._InitializeFacetMap()

# Complex type {http://docs.oasis-open.org/ws-rx/wsrm/200702}SequenceType with content type ELEMENT_ONLY
class SequenceType (pyxb.binding.basis.complexTypeDefinition):
    """Complex type {http://docs.oasis-open.org/ws-rx/wsrm/200702}SequenceType with content type ELEMENT_ONLY"""
    _TypeDefinition = None
    _ContentTypeTag = pyxb.binding.basis.complexTypeDefinition._CT_ELEMENT_ONLY
    _Abstract = False
    _ExpandedName = pyxb.namespace.ExpandedName(Namespace, 'SequenceType')
    _XSDLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wsrm.xsd', 7, 2)
    _ElementMap = {}
    _AttributeMap = {}
    # Base type is pyxb.binding.datatypes.anyType
    
    # Element {http://docs.oasis-open.org/ws-rx/wsrm/200702}MessageNumber uses Python identifier MessageNumber
    __MessageNumber = pyxb.binding.content.ElementDeclaration(pyxb.namespace.ExpandedName(Namespace, 'MessageNumber'), 'MessageNumber', '__httpdocs_oasis_open_orgws_rxwsrm200702_SequenceType_httpdocs_oasis_open_orgws_rxwsrm200702MessageNumber', False, pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wsrm.xsd', 10, 6), )

    
    MessageNumber = property(__MessageNumber.value, __MessageNumber.set, None, None)

    
    # Element {http://docs.oasis-open.org/ws-rx/wsrm/200702}Identifier uses Python identifier Identifier
    __Identifier = pyxb.binding.content.ElementDeclaration(pyxb.namespace.ExpandedName(Namespace, 'Identifier'), 'Identifier', '__httpdocs_oasis_open_orgws_rxwsrm200702_SequenceType_httpdocs_oasis_open_orgws_rxwsrm200702Identifier', False, pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wsrm.xsd', 58, 2), )

    
    Identifier = property(__Identifier.value, __Identifier.set, None, None)

    _AttributeWildcard = pyxb.binding.content.Wildcard(process_contents=pyxb.binding.content.Wildcard.PC_lax, namespace_constraint=(pyxb.binding.content.Wildcard.NC_not, 'http://docs.oasis-open.org/ws-rx/wsrm/200702'))
    _HasWildcardElement = True
    _ElementMap.update({
        __MessageNumber.name() : __MessageNumber,
        __Identifier.name() : __Identifier
    })
    _AttributeMap.update({
        
    })
Namespace.addCategoryObject('typeBinding', 'SequenceType', SequenceType)


# Complex type [anonymous] with content type ELEMENT_ONLY
class CTD_ANON (pyxb.binding.basis.complexTypeDefinition):
    """Complex type [anonymous] with content type ELEMENT_ONLY"""
    _TypeDefinition = None
    _ContentTypeTag = pyxb.binding.basis.complexTypeDefinition._CT_ELEMENT_ONLY
    _Abstract = False
    _ExpandedName = None
    _XSDLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wsrm.xsd', 17, 4)
    _ElementMap = {}
    _AttributeMap = {}
    # Base type is pyxb.binding.datatypes.anyType
    
    # Element {http://docs.oasis-open.org/ws-rx/wsrm/200702}AcknowledgementRange uses Python identifier AcknowledgementRange
    __AcknowledgementRange = pyxb.binding.content.ElementDeclaration(pyxb.namespace.ExpandedName(Namespace, 'AcknowledgementRange'), 'AcknowledgementRange', '__httpdocs_oasis_open_orgws_rxwsrm200702_CTD_ANON_httpdocs_oasis_open_orgws_rxwsrm200702AcknowledgementRange', True, pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wsrm.xsd', 23, 14), )

    
    AcknowledgementRange = property(__AcknowledgementRange.value, __AcknowledgementRange.set, None, None)

    
    # Element {http://docs.oasis-open.org/ws-rx/wsrm/200702}None uses Python identifier None_
    __None = pyxb.binding.content.ElementDeclaration(pyxb.namespace.ExpandedName(Namespace, 'None'), 'None_', '__httpdocs_oasis_open_orgws_rxwsrm200702_CTD_ANON_httpdocs_oasis_open_orgws_rxwsrm200702None', False, pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wsrm.xsd', 31, 14), )

    
    None_ = property(__None.value, __None.set, None, None)

    
    # Element {http://docs.oasis-open.org/ws-rx/wsrm/200702}Final uses Python identifier Final
    __Final = pyxb.binding.content.ElementDeclaration(pyxb.namespace.ExpandedName(Namespace, 'Final'), 'Final', '__httpdocs_oasis_open_orgws_rxwsrm200702_CTD_ANON_httpdocs_oasis_open_orgws_rxwsrm200702Final', False, pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wsrm.xsd', 37, 12), )

    
    Final = property(__Final.value, __Final.set, None, None)

    
    # Element {http://docs.oasis-open.org/ws-rx/wsrm/200702}Nack uses Python identifier Nack
    __Nack = pyxb.binding.content.ElementDeclaration(pyxb.namespace.ExpandedName(Namespace, 'Nack'), 'Nack', '__httpdocs_oasis_open_orgws_rxwsrm200702_CTD_ANON_httpdocs_oasis_open_orgws_rxwsrm200702Nack', True, pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wsrm.xsd', 43, 10), )

    
    Nack = property(__Nack.value, __Nack.set, None, None)

    
    # Element {http://docs.oasis-open.org/ws-rx/wsrm/200702}Identifier uses Python identifier Identifier
    __Identifier = pyxb.binding.content.ElementDeclaration(pyxb.namespace.ExpandedName(Namespace, 'Identifier'), 'Identifier', '__httpdocs_oasis_open_orgws_rxwsrm200702_CTD_ANON_httpdocs_oasis_open_orgws_rxwsrm200702Identifier', False, pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wsrm.xsd', 58, 2), )

    
    Identifier = property(__Identifier.value, __Identifier.set, None, None)

    _AttributeWildcard = pyxb.binding.content.Wildcard(process_contents=pyxb.binding.content.Wildcard.PC_lax, namespace_constraint=(pyxb.binding.content.Wildcard.NC_not, 'http://docs.oasis-open.org/ws-rx/wsrm/200702'))
    _HasWildcardElement = True
    _ElementMap.update({
        __AcknowledgementRange.name() : __AcknowledgementRange,
        __None.name() : __None,
        __Final.name() : __Final,
        __Nack.name() : __Nack,
        __Identifier.name() : __Identifier
    })
    _AttributeMap.update({
        
    })



# Complex type [anonymous] with content type EMPTY
class CTD_ANON_ (pyxb.binding.basis.complexTypeDefinition):
    """Complex type [anonymous] with content type EMPTY"""
    _TypeDefinition = None
    _ContentTypeTag = pyxb.binding.basis.complexTypeDefinition._CT_EMPTY
    _Abstract = False
    _ExpandedName = None
    _XSDLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wsrm.xsd', 24, 16)
    _ElementMap = {}
    _AttributeMap = {}
    # Base type is pyxb.binding.datatypes.anyType
    
    # Attribute Upper uses Python identifier Upper
    __Upper = pyxb.binding.content.AttributeUse(pyxb.namespace.ExpandedName(None, 'Upper'), 'Upper', '__httpdocs_oasis_open_orgws_rxwsrm200702_CTD_ANON__Upper', pyxb.binding.datatypes.unsignedLong, required=True)
    __Upper._DeclarationLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wsrm.xsd', 26, 18)
    __Upper._UseLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wsrm.xsd', 26, 18)
    
    Upper = property(__Upper.value, __Upper.set, None, None)

    
    # Attribute Lower uses Python identifier Lower
    __Lower = pyxb.binding.content.AttributeUse(pyxb.namespace.ExpandedName(None, 'Lower'), 'Lower', '__httpdocs_oasis_open_orgws_rxwsrm200702_CTD_ANON__Lower', pyxb.binding.datatypes.unsignedLong, required=True)
    __Lower._DeclarationLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wsrm.xsd', 27, 18)
    __Lower._UseLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wsrm.xsd', 27, 18)
    
    Lower = property(__Lower.value, __Lower.set, None, None)

    _AttributeWildcard = pyxb.binding.content.Wildcard(process_contents=pyxb.binding.content.Wildcard.PC_lax, namespace_constraint=(pyxb.binding.content.Wildcard.NC_not, 'http://docs.oasis-open.org/ws-rx/wsrm/200702'))
    _ElementMap.update({
        
    })
    _AttributeMap.update({
        __Upper.name() : __Upper,
        __Lower.name() : __Lower
    })



# Complex type [anonymous] with content type EMPTY
class CTD_ANON_2 (pyxb.binding.basis.complexTypeDefinition):
    """Complex type [anonymous] with content type EMPTY"""
    _TypeDefinition = None
    _ContentTypeTag = pyxb.binding.basis.complexTypeDefinition._CT_EMPTY
    _Abstract = False
    _ExpandedName = None
    _XSDLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wsrm.xsd', 32, 16)
    _ElementMap = {}
    _AttributeMap = {}
    # Base type is pyxb.binding.datatypes.anyType
    _ElementMap.update({
        
    })
    _AttributeMap.update({
        
    })



# Complex type [anonymous] with content type EMPTY
class CTD_ANON_3 (pyxb.binding.basis.complexTypeDefinition):
    """Complex type [anonymous] with content type EMPTY"""
    _TypeDefinition = None
    _ContentTypeTag = pyxb.binding.basis.complexTypeDefinition._CT_EMPTY
    _Abstract = False
    _ExpandedName = None
    _XSDLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wsrm.xsd', 38, 14)
    _ElementMap = {}
    _AttributeMap = {}
    # Base type is pyxb.binding.datatypes.anyType
    _ElementMap.update({
        
    })
    _AttributeMap.update({
        
    })



# Complex type {http://docs.oasis-open.org/ws-rx/wsrm/200702}AckRequestedType with content type ELEMENT_ONLY
class AckRequestedType (pyxb.binding.basis.complexTypeDefinition):
    """Complex type {http://docs.oasis-open.org/ws-rx/wsrm/200702}AckRequestedType with content type ELEMENT_ONLY"""
    _TypeDefinition = None
    _ContentTypeTag = pyxb.binding.basis.complexTypeDefinition._CT_ELEMENT_ONLY
    _Abstract = False
    _ExpandedName = pyxb.namespace.ExpandedName(Namespace, 'AckRequestedType')
    _XSDLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wsrm.xsd', 50, 2)
    _ElementMap = {}
    _AttributeMap = {}
    # Base type is pyxb.binding.datatypes.anyType
    
    # Element {http://docs.oasis-open.org/ws-rx/wsrm/200702}Identifier uses Python identifier Identifier
    __Identifier = pyxb.binding.content.ElementDeclaration(pyxb.namespace.ExpandedName(Namespace, 'Identifier'), 'Identifier', '__httpdocs_oasis_open_orgws_rxwsrm200702_AckRequestedType_httpdocs_oasis_open_orgws_rxwsrm200702Identifier', False, pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wsrm.xsd', 58, 2), )

    
    Identifier = property(__Identifier.value, __Identifier.set, None, None)

    _AttributeWildcard = pyxb.binding.content.Wildcard(process_contents=pyxb.binding.content.Wildcard.PC_lax, namespace_constraint=(pyxb.binding.content.Wildcard.NC_not, 'http://docs.oasis-open.org/ws-rx/wsrm/200702'))
    _HasWildcardElement = True
    _ElementMap.update({
        __Identifier.name() : __Identifier
    })
    _AttributeMap.update({
        
    })
Namespace.addCategoryObject('typeBinding', 'AckRequestedType', AckRequestedType)


# Complex type [anonymous] with content type SIMPLE
class CTD_ANON_4 (pyxb.binding.basis.complexTypeDefinition):
    """
          This type is for elements whose [children] is an anyURI and can have arbitrary attributes.
        """
    _TypeDefinition = pyxb.binding.datatypes.anyURI
    _ContentTypeTag = pyxb.binding.basis.complexTypeDefinition._CT_SIMPLE
    _Abstract = False
    _ExpandedName = None
    _XSDLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wsrm.xsd', 59, 4)
    _ElementMap = {}
    _AttributeMap = {}
    # Base type is pyxb.binding.datatypes.anyURI
    _AttributeWildcard = pyxb.binding.content.Wildcard(process_contents=pyxb.binding.content.Wildcard.PC_lax, namespace_constraint=(pyxb.binding.content.Wildcard.NC_not, 'http://docs.oasis-open.org/ws-rx/wsrm/200702'))
    _ElementMap.update({
        
    })
    _AttributeMap.update({
        
    })



# Complex type [anonymous] with content type SIMPLE
class CTD_ANON_5 (pyxb.binding.basis.complexTypeDefinition):
    """Complex type [anonymous] with content type SIMPLE"""
    _TypeDefinition = pyxb.binding.datatypes.anyURI
    _ContentTypeTag = pyxb.binding.basis.complexTypeDefinition._CT_SIMPLE
    _Abstract = False
    _ExpandedName = None
    _XSDLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wsrm.xsd', 73, 4)
    _ElementMap = {}
    _AttributeMap = {}
    # Base type is pyxb.binding.datatypes.anyURI
    _AttributeWildcard = pyxb.binding.content.Wildcard(process_contents=pyxb.binding.content.Wildcard.PC_lax, namespace_constraint=(pyxb.binding.content.Wildcard.NC_not, 'http://docs.oasis-open.org/ws-rx/wsrm/200702'))
    _ElementMap.update({
        
    })
    _AttributeMap.update({
        
    })



# Complex type {http://docs.oasis-open.org/ws-rx/wsrm/200702}SequenceFaultType with content type ELEMENT_ONLY
class SequenceFaultType (pyxb.binding.basis.complexTypeDefinition):
    """Complex type {http://docs.oasis-open.org/ws-rx/wsrm/200702}SequenceFaultType with content type ELEMENT_ONLY"""
    _TypeDefinition = None
    _ContentTypeTag = pyxb.binding.basis.complexTypeDefinition._CT_ELEMENT_ONLY
    _Abstract = False
    _ExpandedName = pyxb.namespace.ExpandedName(Namespace, 'SequenceFaultType')
    _XSDLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wsrm.xsd', 99, 2)
    _ElementMap = {}
    _AttributeMap = {}
    # Base type is pyxb.binding.datatypes.anyType
    
    # Element {http://docs.oasis-open.org/ws-rx/wsrm/200702}FaultCode uses Python identifier FaultCode
    __FaultCode = pyxb.binding.content.ElementDeclaration(pyxb.namespace.ExpandedName(Namespace, 'FaultCode'), 'FaultCode', '__httpdocs_oasis_open_orgws_rxwsrm200702_SequenceFaultType_httpdocs_oasis_open_orgws_rxwsrm200702FaultCode', False, pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wsrm.xsd', 101, 6), )

    
    FaultCode = property(__FaultCode.value, __FaultCode.set, None, None)

    
    # Element {http://docs.oasis-open.org/ws-rx/wsrm/200702}Detail uses Python identifier Detail
    __Detail = pyxb.binding.content.ElementDeclaration(pyxb.namespace.ExpandedName(Namespace, 'Detail'), 'Detail', '__httpdocs_oasis_open_orgws_rxwsrm200702_SequenceFaultType_httpdocs_oasis_open_orgws_rxwsrm200702Detail', False, pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wsrm.xsd', 102, 6), )

    
    Detail = property(__Detail.value, __Detail.set, None, None)

    _AttributeWildcard = pyxb.binding.content.Wildcard(process_contents=pyxb.binding.content.Wildcard.PC_lax, namespace_constraint=(pyxb.binding.content.Wildcard.NC_not, 'http://docs.oasis-open.org/ws-rx/wsrm/200702'))
    _HasWildcardElement = True
    _ElementMap.update({
        __FaultCode.name() : __FaultCode,
        __Detail.name() : __Detail
    })
    _AttributeMap.update({
        
    })
Namespace.addCategoryObject('typeBinding', 'SequenceFaultType', SequenceFaultType)


# Complex type {http://docs.oasis-open.org/ws-rx/wsrm/200702}DetailType with content type ELEMENT_ONLY
class DetailType (pyxb.binding.basis.complexTypeDefinition):
    """Complex type {http://docs.oasis-open.org/ws-rx/wsrm/200702}DetailType with content type ELEMENT_ONLY"""
    _TypeDefinition = None
    _ContentTypeTag = pyxb.binding.basis.complexTypeDefinition._CT_ELEMENT_ONLY
    _Abstract = False
    _ExpandedName = pyxb.namespace.ExpandedName(Namespace, 'DetailType')
    _XSDLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wsrm.xsd', 107, 2)
    _ElementMap = {}
    _AttributeMap = {}
    # Base type is pyxb.binding.datatypes.anyType
    _AttributeWildcard = pyxb.binding.content.Wildcard(process_contents=pyxb.binding.content.Wildcard.PC_lax, namespace_constraint=(pyxb.binding.content.Wildcard.NC_not, 'http://docs.oasis-open.org/ws-rx/wsrm/200702'))
    _HasWildcardElement = True
    _ElementMap.update({
        
    })
    _AttributeMap.update({
        
    })
Namespace.addCategoryObject('typeBinding', 'DetailType', DetailType)


# Complex type {http://docs.oasis-open.org/ws-rx/wsrm/200702}CreateSequenceType with content type ELEMENT_ONLY
class CreateSequenceType (pyxb.binding.basis.complexTypeDefinition):
    """Complex type {http://docs.oasis-open.org/ws-rx/wsrm/200702}CreateSequenceType with content type ELEMENT_ONLY"""
    _TypeDefinition = None
    _ContentTypeTag = pyxb.binding.basis.complexTypeDefinition._CT_ELEMENT_ONLY
    _Abstract = False
    _ExpandedName = pyxb.namespace.ExpandedName(Namespace, 'CreateSequenceType')
    _XSDLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wsrm.xsd', 120, 2)
    _ElementMap = {}
    _AttributeMap = {}
    # Base type is pyxb.binding.datatypes.anyType
    
    # Element {http://docs.oasis-open.org/ws-rx/wsrm/200702}Offer uses Python identifier Offer
    __Offer = pyxb.binding.content.ElementDeclaration(pyxb.namespace.ExpandedName(Namespace, 'Offer'), 'Offer', '__httpdocs_oasis_open_orgws_rxwsrm200702_CreateSequenceType_httpdocs_oasis_open_orgws_rxwsrm200702Offer', False, pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wsrm.xsd', 124, 6), )

    
    Offer = property(__Offer.value, __Offer.set, None, None)

    
    # Element {http://docs.oasis-open.org/ws-rx/wsrm/200702}AcksTo uses Python identifier AcksTo
    __AcksTo = pyxb.binding.content.ElementDeclaration(pyxb.namespace.ExpandedName(Namespace, 'AcksTo'), 'AcksTo', '__httpdocs_oasis_open_orgws_rxwsrm200702_CreateSequenceType_httpdocs_oasis_open_orgws_rxwsrm200702AcksTo', False, pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wsrm.xsd', 175, 2), )

    
    AcksTo = property(__AcksTo.value, __AcksTo.set, None, None)

    
    # Element {http://docs.oasis-open.org/ws-rx/wsrm/200702}Expires uses Python identifier Expires
    __Expires = pyxb.binding.content.ElementDeclaration(pyxb.namespace.ExpandedName(Namespace, 'Expires'), 'Expires', '__httpdocs_oasis_open_orgws_rxwsrm200702_CreateSequenceType_httpdocs_oasis_open_orgws_rxwsrm200702Expires', False, pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wsrm.xsd', 193, 2), )

    
    Expires = property(__Expires.value, __Expires.set, None, None)

    _AttributeWildcard = pyxb.binding.content.Wildcard(process_contents=pyxb.binding.content.Wildcard.PC_lax, namespace_constraint=(pyxb.binding.content.Wildcard.NC_not, 'http://docs.oasis-open.org/ws-rx/wsrm/200702'))
    _HasWildcardElement = True
    _ElementMap.update({
        __Offer.name() : __Offer,
        __AcksTo.name() : __AcksTo,
        __Expires.name() : __Expires
    })
    _AttributeMap.update({
        
    })
Namespace.addCategoryObject('typeBinding', 'CreateSequenceType', CreateSequenceType)


# Complex type {http://docs.oasis-open.org/ws-rx/wsrm/200702}CreateSequenceResponseType with content type ELEMENT_ONLY
class CreateSequenceResponseType (pyxb.binding.basis.complexTypeDefinition):
    """Complex type {http://docs.oasis-open.org/ws-rx/wsrm/200702}CreateSequenceResponseType with content type ELEMENT_ONLY"""
    _TypeDefinition = None
    _ContentTypeTag = pyxb.binding.basis.complexTypeDefinition._CT_ELEMENT_ONLY
    _Abstract = False
    _ExpandedName = pyxb.namespace.ExpandedName(Namespace, 'CreateSequenceResponseType')
    _XSDLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wsrm.xsd', 135, 2)
    _ElementMap = {}
    _AttributeMap = {}
    # Base type is pyxb.binding.datatypes.anyType
    
    # Element {http://docs.oasis-open.org/ws-rx/wsrm/200702}Identifier uses Python identifier Identifier
    __Identifier = pyxb.binding.content.ElementDeclaration(pyxb.namespace.ExpandedName(Namespace, 'Identifier'), 'Identifier', '__httpdocs_oasis_open_orgws_rxwsrm200702_CreateSequenceResponseType_httpdocs_oasis_open_orgws_rxwsrm200702Identifier', False, pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wsrm.xsd', 58, 2), )

    
    Identifier = property(__Identifier.value, __Identifier.set, None, None)

    
    # Element {http://docs.oasis-open.org/ws-rx/wsrm/200702}IncompleteSequenceBehavior uses Python identifier IncompleteSequenceBehavior
    __IncompleteSequenceBehavior = pyxb.binding.content.ElementDeclaration(pyxb.namespace.ExpandedName(Namespace, 'IncompleteSequenceBehavior'), 'IncompleteSequenceBehavior', '__httpdocs_oasis_open_orgws_rxwsrm200702_CreateSequenceResponseType_httpdocs_oasis_open_orgws_rxwsrm200702IncompleteSequenceBehavior', False, pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wsrm.xsd', 139, 6), )

    
    IncompleteSequenceBehavior = property(__IncompleteSequenceBehavior.value, __IncompleteSequenceBehavior.set, None, None)

    
    # Element {http://docs.oasis-open.org/ws-rx/wsrm/200702}Accept uses Python identifier Accept
    __Accept = pyxb.binding.content.ElementDeclaration(pyxb.namespace.ExpandedName(Namespace, 'Accept'), 'Accept', '__httpdocs_oasis_open_orgws_rxwsrm200702_CreateSequenceResponseType_httpdocs_oasis_open_orgws_rxwsrm200702Accept', False, pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wsrm.xsd', 140, 6), )

    
    Accept = property(__Accept.value, __Accept.set, None, None)

    
    # Element {http://docs.oasis-open.org/ws-rx/wsrm/200702}Expires uses Python identifier Expires
    __Expires = pyxb.binding.content.ElementDeclaration(pyxb.namespace.ExpandedName(Namespace, 'Expires'), 'Expires', '__httpdocs_oasis_open_orgws_rxwsrm200702_CreateSequenceResponseType_httpdocs_oasis_open_orgws_rxwsrm200702Expires', False, pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wsrm.xsd', 193, 2), )

    
    Expires = property(__Expires.value, __Expires.set, None, None)

    _AttributeWildcard = pyxb.binding.content.Wildcard(process_contents=pyxb.binding.content.Wildcard.PC_lax, namespace_constraint=(pyxb.binding.content.Wildcard.NC_not, 'http://docs.oasis-open.org/ws-rx/wsrm/200702'))
    _HasWildcardElement = True
    _ElementMap.update({
        __Identifier.name() : __Identifier,
        __IncompleteSequenceBehavior.name() : __IncompleteSequenceBehavior,
        __Accept.name() : __Accept,
        __Expires.name() : __Expires
    })
    _AttributeMap.update({
        
    })
Namespace.addCategoryObject('typeBinding', 'CreateSequenceResponseType', CreateSequenceResponseType)


# Complex type {http://docs.oasis-open.org/ws-rx/wsrm/200702}CloseSequenceType with content type ELEMENT_ONLY
class CloseSequenceType (pyxb.binding.basis.complexTypeDefinition):
    """Complex type {http://docs.oasis-open.org/ws-rx/wsrm/200702}CloseSequenceType with content type ELEMENT_ONLY"""
    _TypeDefinition = None
    _ContentTypeTag = pyxb.binding.basis.complexTypeDefinition._CT_ELEMENT_ONLY
    _Abstract = False
    _ExpandedName = pyxb.namespace.ExpandedName(Namespace, 'CloseSequenceType')
    _XSDLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wsrm.xsd', 145, 2)
    _ElementMap = {}
    _AttributeMap = {}
    # Base type is pyxb.binding.datatypes.anyType
    
    # Element {http://docs.oasis-open.org/ws-rx/wsrm/200702}Identifier uses Python identifier Identifier
    __Identifier = pyxb.binding.content.ElementDeclaration(pyxb.namespace.ExpandedName(Namespace, 'Identifier'), 'Identifier', '__httpdocs_oasis_open_orgws_rxwsrm200702_CloseSequenceType_httpdocs_oasis_open_orgws_rxwsrm200702Identifier', False, pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wsrm.xsd', 58, 2), )

    
    Identifier = property(__Identifier.value, __Identifier.set, None, None)

    
    # Element {http://docs.oasis-open.org/ws-rx/wsrm/200702}LastMsgNumber uses Python identifier LastMsgNumber
    __LastMsgNumber = pyxb.binding.content.ElementDeclaration(pyxb.namespace.ExpandedName(Namespace, 'LastMsgNumber'), 'LastMsgNumber', '__httpdocs_oasis_open_orgws_rxwsrm200702_CloseSequenceType_httpdocs_oasis_open_orgws_rxwsrm200702LastMsgNumber', False, pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wsrm.xsd', 148, 6), )

    
    LastMsgNumber = property(__LastMsgNumber.value, __LastMsgNumber.set, None, None)

    _AttributeWildcard = pyxb.binding.content.Wildcard(process_contents=pyxb.binding.content.Wildcard.PC_lax, namespace_constraint=(pyxb.binding.content.Wildcard.NC_not, 'http://docs.oasis-open.org/ws-rx/wsrm/200702'))
    _HasWildcardElement = True
    _ElementMap.update({
        __Identifier.name() : __Identifier,
        __LastMsgNumber.name() : __LastMsgNumber
    })
    _AttributeMap.update({
        
    })
Namespace.addCategoryObject('typeBinding', 'CloseSequenceType', CloseSequenceType)


# Complex type {http://docs.oasis-open.org/ws-rx/wsrm/200702}CloseSequenceResponseType with content type ELEMENT_ONLY
class CloseSequenceResponseType (pyxb.binding.basis.complexTypeDefinition):
    """Complex type {http://docs.oasis-open.org/ws-rx/wsrm/200702}CloseSequenceResponseType with content type ELEMENT_ONLY"""
    _TypeDefinition = None
    _ContentTypeTag = pyxb.binding.basis.complexTypeDefinition._CT_ELEMENT_ONLY
    _Abstract = False
    _ExpandedName = pyxb.namespace.ExpandedName(Namespace, 'CloseSequenceResponseType')
    _XSDLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wsrm.xsd', 153, 2)
    _ElementMap = {}
    _AttributeMap = {}
    # Base type is pyxb.binding.datatypes.anyType
    
    # Element {http://docs.oasis-open.org/ws-rx/wsrm/200702}Identifier uses Python identifier Identifier
    __Identifier = pyxb.binding.content.ElementDeclaration(pyxb.namespace.ExpandedName(Namespace, 'Identifier'), 'Identifier', '__httpdocs_oasis_open_orgws_rxwsrm200702_CloseSequenceResponseType_httpdocs_oasis_open_orgws_rxwsrm200702Identifier', False, pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wsrm.xsd', 58, 2), )

    
    Identifier = property(__Identifier.value, __Identifier.set, None, None)

    _AttributeWildcard = pyxb.binding.content.Wildcard(process_contents=pyxb.binding.content.Wildcard.PC_lax, namespace_constraint=(pyxb.binding.content.Wildcard.NC_not, 'http://docs.oasis-open.org/ws-rx/wsrm/200702'))
    _HasWildcardElement = True
    _ElementMap.update({
        __Identifier.name() : __Identifier
    })
    _AttributeMap.update({
        
    })
Namespace.addCategoryObject('typeBinding', 'CloseSequenceResponseType', CloseSequenceResponseType)


# Complex type {http://docs.oasis-open.org/ws-rx/wsrm/200702}TerminateSequenceType with content type ELEMENT_ONLY
class TerminateSequenceType (pyxb.binding.basis.complexTypeDefinition):
    """Complex type {http://docs.oasis-open.org/ws-rx/wsrm/200702}TerminateSequenceType with content type ELEMENT_ONLY"""
    _TypeDefinition = None
    _ContentTypeTag = pyxb.binding.basis.complexTypeDefinition._CT_ELEMENT_ONLY
    _Abstract = False
    _ExpandedName = pyxb.namespace.ExpandedName(Namespace, 'TerminateSequenceType')
    _XSDLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wsrm.xsd', 160, 2)
    _ElementMap = {}
    _AttributeMap = {}
    # Base type is pyxb.binding.datatypes.anyType
    
    # Element {http://docs.oasis-open.org/ws-rx/wsrm/200702}Identifier uses Python identifier Identifier
    __Identifier = pyxb.binding.content.ElementDeclaration(pyxb.namespace.ExpandedName(Namespace, 'Identifier'), 'Identifier', '__httpdocs_oasis_open_orgws_rxwsrm200702_TerminateSequenceType_httpdocs_oasis_open_orgws_rxwsrm200702Identifier', False, pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wsrm.xsd', 58, 2), )

    
    Identifier = property(__Identifier.value, __Identifier.set, None, None)

    
    # Element {http://docs.oasis-open.org/ws-rx/wsrm/200702}LastMsgNumber uses Python identifier LastMsgNumber
    __LastMsgNumber = pyxb.binding.content.ElementDeclaration(pyxb.namespace.ExpandedName(Namespace, 'LastMsgNumber'), 'LastMsgNumber', '__httpdocs_oasis_open_orgws_rxwsrm200702_TerminateSequenceType_httpdocs_oasis_open_orgws_rxwsrm200702LastMsgNumber', False, pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wsrm.xsd', 163, 6), )

    
    LastMsgNumber = property(__LastMsgNumber.value, __LastMsgNumber.set, None, None)

    _AttributeWildcard = pyxb.binding.content.Wildcard(process_contents=pyxb.binding.content.Wildcard.PC_lax, namespace_constraint=(pyxb.binding.content.Wildcard.NC_not, 'http://docs.oasis-open.org/ws-rx/wsrm/200702'))
    _HasWildcardElement = True
    _ElementMap.update({
        __Identifier.name() : __Identifier,
        __LastMsgNumber.name() : __LastMsgNumber
    })
    _AttributeMap.update({
        
    })
Namespace.addCategoryObject('typeBinding', 'TerminateSequenceType', TerminateSequenceType)


# Complex type {http://docs.oasis-open.org/ws-rx/wsrm/200702}TerminateSequenceResponseType with content type ELEMENT_ONLY
class TerminateSequenceResponseType (pyxb.binding.basis.complexTypeDefinition):
    """Complex type {http://docs.oasis-open.org/ws-rx/wsrm/200702}TerminateSequenceResponseType with content type ELEMENT_ONLY"""
    _TypeDefinition = None
    _ContentTypeTag = pyxb.binding.basis.complexTypeDefinition._CT_ELEMENT_ONLY
    _Abstract = False
    _ExpandedName = pyxb.namespace.ExpandedName(Namespace, 'TerminateSequenceResponseType')
    _XSDLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wsrm.xsd', 168, 2)
    _ElementMap = {}
    _AttributeMap = {}
    # Base type is pyxb.binding.datatypes.anyType
    
    # Element {http://docs.oasis-open.org/ws-rx/wsrm/200702}Identifier uses Python identifier Identifier
    __Identifier = pyxb.binding.content.ElementDeclaration(pyxb.namespace.ExpandedName(Namespace, 'Identifier'), 'Identifier', '__httpdocs_oasis_open_orgws_rxwsrm200702_TerminateSequenceResponseType_httpdocs_oasis_open_orgws_rxwsrm200702Identifier', False, pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wsrm.xsd', 58, 2), )

    
    Identifier = property(__Identifier.value, __Identifier.set, None, None)

    _AttributeWildcard = pyxb.binding.content.Wildcard(process_contents=pyxb.binding.content.Wildcard.PC_lax, namespace_constraint=(pyxb.binding.content.Wildcard.NC_not, 'http://docs.oasis-open.org/ws-rx/wsrm/200702'))
    _HasWildcardElement = True
    _ElementMap.update({
        __Identifier.name() : __Identifier
    })
    _AttributeMap.update({
        
    })
Namespace.addCategoryObject('typeBinding', 'TerminateSequenceResponseType', TerminateSequenceResponseType)


# Complex type {http://docs.oasis-open.org/ws-rx/wsrm/200702}OfferType with content type ELEMENT_ONLY
class OfferType (pyxb.binding.basis.complexTypeDefinition):
    """Complex type {http://docs.oasis-open.org/ws-rx/wsrm/200702}OfferType with content type ELEMENT_ONLY"""
    _TypeDefinition = None
    _ContentTypeTag = pyxb.binding.basis.complexTypeDefinition._CT_ELEMENT_ONLY
    _Abstract = False
    _ExpandedName = pyxb.namespace.ExpandedName(Namespace, 'OfferType')
    _XSDLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wsrm.xsd', 176, 2)
    _ElementMap = {}
    _AttributeMap = {}
    # Base type is pyxb.binding.datatypes.anyType
    
    # Element {http://docs.oasis-open.org/ws-rx/wsrm/200702}Identifier uses Python identifier Identifier
    __Identifier = pyxb.binding.content.ElementDeclaration(pyxb.namespace.ExpandedName(Namespace, 'Identifier'), 'Identifier', '__httpdocs_oasis_open_orgws_rxwsrm200702_OfferType_httpdocs_oasis_open_orgws_rxwsrm200702Identifier', False, pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wsrm.xsd', 58, 2), )

    
    Identifier = property(__Identifier.value, __Identifier.set, None, None)

    
    # Element {http://docs.oasis-open.org/ws-rx/wsrm/200702}Endpoint uses Python identifier Endpoint
    __Endpoint = pyxb.binding.content.ElementDeclaration(pyxb.namespace.ExpandedName(Namespace, 'Endpoint'), 'Endpoint', '__httpdocs_oasis_open_orgws_rxwsrm200702_OfferType_httpdocs_oasis_open_orgws_rxwsrm200702Endpoint', False, pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wsrm.xsd', 179, 6), )

    
    Endpoint = property(__Endpoint.value, __Endpoint.set, None, None)

    
    # Element {http://docs.oasis-open.org/ws-rx/wsrm/200702}IncompleteSequenceBehavior uses Python identifier IncompleteSequenceBehavior
    __IncompleteSequenceBehavior = pyxb.binding.content.ElementDeclaration(pyxb.namespace.ExpandedName(Namespace, 'IncompleteSequenceBehavior'), 'IncompleteSequenceBehavior', '__httpdocs_oasis_open_orgws_rxwsrm200702_OfferType_httpdocs_oasis_open_orgws_rxwsrm200702IncompleteSequenceBehavior', False, pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wsrm.xsd', 181, 6), )

    
    IncompleteSequenceBehavior = property(__IncompleteSequenceBehavior.value, __IncompleteSequenceBehavior.set, None, None)

    
    # Element {http://docs.oasis-open.org/ws-rx/wsrm/200702}Expires uses Python identifier Expires
    __Expires = pyxb.binding.content.ElementDeclaration(pyxb.namespace.ExpandedName(Namespace, 'Expires'), 'Expires', '__httpdocs_oasis_open_orgws_rxwsrm200702_OfferType_httpdocs_oasis_open_orgws_rxwsrm200702Expires', False, pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wsrm.xsd', 193, 2), )

    
    Expires = property(__Expires.value, __Expires.set, None, None)

    _AttributeWildcard = pyxb.binding.content.Wildcard(process_contents=pyxb.binding.content.Wildcard.PC_lax, namespace_constraint=(pyxb.binding.content.Wildcard.NC_not, 'http://docs.oasis-open.org/ws-rx/wsrm/200702'))
    _HasWildcardElement = True
    _ElementMap.update({
        __Identifier.name() : __Identifier,
        __Endpoint.name() : __Endpoint,
        __IncompleteSequenceBehavior.name() : __IncompleteSequenceBehavior,
        __Expires.name() : __Expires
    })
    _AttributeMap.update({
        
    })
Namespace.addCategoryObject('typeBinding', 'OfferType', OfferType)


# Complex type {http://docs.oasis-open.org/ws-rx/wsrm/200702}AcceptType with content type ELEMENT_ONLY
class AcceptType (pyxb.binding.basis.complexTypeDefinition):
    """Complex type {http://docs.oasis-open.org/ws-rx/wsrm/200702}AcceptType with content type ELEMENT_ONLY"""
    _TypeDefinition = None
    _ContentTypeTag = pyxb.binding.basis.complexTypeDefinition._CT_ELEMENT_ONLY
    _Abstract = False
    _ExpandedName = pyxb.namespace.ExpandedName(Namespace, 'AcceptType')
    _XSDLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wsrm.xsd', 186, 2)
    _ElementMap = {}
    _AttributeMap = {}
    # Base type is pyxb.binding.datatypes.anyType
    
    # Element {http://docs.oasis-open.org/ws-rx/wsrm/200702}AcksTo uses Python identifier AcksTo
    __AcksTo = pyxb.binding.content.ElementDeclaration(pyxb.namespace.ExpandedName(Namespace, 'AcksTo'), 'AcksTo', '__httpdocs_oasis_open_orgws_rxwsrm200702_AcceptType_httpdocs_oasis_open_orgws_rxwsrm200702AcksTo', False, pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wsrm.xsd', 175, 2), )

    
    AcksTo = property(__AcksTo.value, __AcksTo.set, None, None)

    _AttributeWildcard = pyxb.binding.content.Wildcard(process_contents=pyxb.binding.content.Wildcard.PC_lax, namespace_constraint=(pyxb.binding.content.Wildcard.NC_not, 'http://docs.oasis-open.org/ws-rx/wsrm/200702'))
    _HasWildcardElement = True
    _ElementMap.update({
        __AcksTo.name() : __AcksTo
    })
    _AttributeMap.update({
        
    })
Namespace.addCategoryObject('typeBinding', 'AcceptType', AcceptType)


# Complex type [anonymous] with content type SIMPLE
class CTD_ANON_6 (pyxb.binding.basis.complexTypeDefinition):
    """Complex type [anonymous] with content type SIMPLE"""
    _TypeDefinition = pyxb.binding.datatypes.duration
    _ContentTypeTag = pyxb.binding.basis.complexTypeDefinition._CT_SIMPLE
    _Abstract = False
    _ExpandedName = None
    _XSDLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wsrm.xsd', 194, 4)
    _ElementMap = {}
    _AttributeMap = {}
    # Base type is pyxb.binding.datatypes.duration
    _AttributeWildcard = pyxb.binding.content.Wildcard(process_contents=pyxb.binding.content.Wildcard.PC_lax, namespace_constraint=(pyxb.binding.content.Wildcard.NC_not, 'http://docs.oasis-open.org/ws-rx/wsrm/200702'))
    _ElementMap.update({
        
    })
    _AttributeMap.update({
        
    })



# Complex type [anonymous] with content type EMPTY
class CTD_ANON_7 (pyxb.binding.basis.complexTypeDefinition):
    """Complex type [anonymous] with content type EMPTY"""
    _TypeDefinition = None
    _ContentTypeTag = pyxb.binding.basis.complexTypeDefinition._CT_EMPTY
    _Abstract = False
    _ExpandedName = None
    _XSDLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wsrm.xsd', 210, 4)
    _ElementMap = {}
    _AttributeMap = {}
    # Base type is pyxb.binding.datatypes.anyType
    _AttributeWildcard = pyxb.binding.content.Wildcard(process_contents=pyxb.binding.content.Wildcard.PC_lax, namespace_constraint=(pyxb.binding.content.Wildcard.NC_not, 'http://docs.oasis-open.org/ws-rx/wsrm/200702'))
    _ElementMap.update({
        
    })
    _AttributeMap.update({
        
    })



# Complex type [anonymous] with content type EMPTY
class CTD_ANON_8 (pyxb.binding.basis.complexTypeDefinition):
    """Complex type [anonymous] with content type EMPTY"""
    _TypeDefinition = None
    _ContentTypeTag = pyxb.binding.basis.complexTypeDefinition._CT_EMPTY
    _Abstract = False
    _ExpandedName = None
    _XSDLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wsrm.xsd', 216, 4)
    _ElementMap = {}
    _AttributeMap = {}
    # Base type is pyxb.binding.datatypes.anyType
    _AttributeWildcard = pyxb.binding.content.Wildcard(process_contents=pyxb.binding.content.Wildcard.PC_lax, namespace_constraint=(pyxb.binding.content.Wildcard.NC_not, 'http://docs.oasis-open.org/ws-rx/wsrm/200702'))
    _ElementMap.update({
        
    })
    _AttributeMap.update({
        
    })



AcksTo = pyxb.binding.basis.element(pyxb.namespace.ExpandedName(Namespace, 'AcksTo'), pyxb.bundles.wssplat.wsa.EndpointReferenceType, location=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wsrm.xsd', 175, 2))
Namespace.addCategoryObject('elementBinding', AcksTo.name().localName(), AcksTo)

Sequence = pyxb.binding.basis.element(pyxb.namespace.ExpandedName(Namespace, 'Sequence'), SequenceType, location=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wsrm.xsd', 15, 2))
Namespace.addCategoryObject('elementBinding', Sequence.name().localName(), Sequence)

SequenceAcknowledgement = pyxb.binding.basis.element(pyxb.namespace.ExpandedName(Namespace, 'SequenceAcknowledgement'), CTD_ANON, location=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wsrm.xsd', 16, 2))
Namespace.addCategoryObject('elementBinding', SequenceAcknowledgement.name().localName(), SequenceAcknowledgement)

AckRequested = pyxb.binding.basis.element(pyxb.namespace.ExpandedName(Namespace, 'AckRequested'), AckRequestedType, location=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wsrm.xsd', 57, 2))
Namespace.addCategoryObject('elementBinding', AckRequested.name().localName(), AckRequested)

Identifier = pyxb.binding.basis.element(pyxb.namespace.ExpandedName(Namespace, 'Identifier'), CTD_ANON_4, location=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wsrm.xsd', 58, 2))
Namespace.addCategoryObject('elementBinding', Identifier.name().localName(), Identifier)

Address = pyxb.binding.basis.element(pyxb.namespace.ExpandedName(Namespace, 'Address'), CTD_ANON_5, location=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wsrm.xsd', 72, 2))
Namespace.addCategoryObject('elementBinding', Address.name().localName(), Address)

SequenceFault = pyxb.binding.basis.element(pyxb.namespace.ExpandedName(Namespace, 'SequenceFault'), SequenceFaultType, location=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wsrm.xsd', 113, 2))
Namespace.addCategoryObject('elementBinding', SequenceFault.name().localName(), SequenceFault)

CreateSequence = pyxb.binding.basis.element(pyxb.namespace.ExpandedName(Namespace, 'CreateSequence'), CreateSequenceType, location=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wsrm.xsd', 114, 2))
Namespace.addCategoryObject('elementBinding', CreateSequence.name().localName(), CreateSequence)

CreateSequenceResponse = pyxb.binding.basis.element(pyxb.namespace.ExpandedName(Namespace, 'CreateSequenceResponse'), CreateSequenceResponseType, location=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wsrm.xsd', 115, 2))
Namespace.addCategoryObject('elementBinding', CreateSequenceResponse.name().localName(), CreateSequenceResponse)

CloseSequence = pyxb.binding.basis.element(pyxb.namespace.ExpandedName(Namespace, 'CloseSequence'), CloseSequenceType, location=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wsrm.xsd', 116, 2))
Namespace.addCategoryObject('elementBinding', CloseSequence.name().localName(), CloseSequence)

CloseSequenceResponse = pyxb.binding.basis.element(pyxb.namespace.ExpandedName(Namespace, 'CloseSequenceResponse'), CloseSequenceResponseType, location=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wsrm.xsd', 117, 2))
Namespace.addCategoryObject('elementBinding', CloseSequenceResponse.name().localName(), CloseSequenceResponse)

TerminateSequence = pyxb.binding.basis.element(pyxb.namespace.ExpandedName(Namespace, 'TerminateSequence'), TerminateSequenceType, location=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wsrm.xsd', 118, 2))
Namespace.addCategoryObject('elementBinding', TerminateSequence.name().localName(), TerminateSequence)

TerminateSequenceResponse = pyxb.binding.basis.element(pyxb.namespace.ExpandedName(Namespace, 'TerminateSequenceResponse'), TerminateSequenceResponseType, location=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wsrm.xsd', 119, 2))
Namespace.addCategoryObject('elementBinding', TerminateSequenceResponse.name().localName(), TerminateSequenceResponse)

Expires = pyxb.binding.basis.element(pyxb.namespace.ExpandedName(Namespace, 'Expires'), CTD_ANON_6, location=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wsrm.xsd', 193, 2))
Namespace.addCategoryObject('elementBinding', Expires.name().localName(), Expires)

UsesSequenceSTR = pyxb.binding.basis.element(pyxb.namespace.ExpandedName(Namespace, 'UsesSequenceSTR'), CTD_ANON_7, location=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wsrm.xsd', 209, 2))
Namespace.addCategoryObject('elementBinding', UsesSequenceSTR.name().localName(), UsesSequenceSTR)

UsesSequenceSSL = pyxb.binding.basis.element(pyxb.namespace.ExpandedName(Namespace, 'UsesSequenceSSL'), CTD_ANON_8, location=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wsrm.xsd', 215, 2))
Namespace.addCategoryObject('elementBinding', UsesSequenceSSL.name().localName(), UsesSequenceSSL)

UnsupportedElement = pyxb.binding.basis.element(pyxb.namespace.ExpandedName(Namespace, 'UnsupportedElement'), STD_ANON, location=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wsrm.xsd', 221, 2))
Namespace.addCategoryObject('elementBinding', UnsupportedElement.name().localName(), UnsupportedElement)



SequenceType._AddElement(pyxb.binding.basis.element(pyxb.namespace.ExpandedName(Namespace, 'MessageNumber'), MessageNumberType, scope=SequenceType, location=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wsrm.xsd', 10, 6)))

SequenceType._AddElement(pyxb.binding.basis.element(pyxb.namespace.ExpandedName(Namespace, 'Identifier'), CTD_ANON_4, scope=SequenceType, location=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wsrm.xsd', 58, 2)))

def _BuildAutomaton ():
    # Remove this helper function from the namespace after it is invoked
    global _BuildAutomaton
    del _BuildAutomaton
    import pyxb.utils.fac as fac

    counters = set()
    cc_0 = fac.CounterCondition(min=0, max=None, metadata=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wsrm.xsd', 11, 6))
    counters.add(cc_0)
    states = []
    final_update = None
    symbol = pyxb.binding.content.ElementUse(SequenceType._UseForTag(pyxb.namespace.ExpandedName(Namespace, 'Identifier')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wsrm.xsd', 9, 6))
    st_0 = fac.State(symbol, is_initial=True, final_update=final_update, is_unordered_catenation=False)
    states.append(st_0)
    final_update = set()
    symbol = pyxb.binding.content.ElementUse(SequenceType._UseForTag(pyxb.namespace.ExpandedName(Namespace, 'MessageNumber')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wsrm.xsd', 10, 6))
    st_1 = fac.State(symbol, is_initial=False, final_update=final_update, is_unordered_catenation=False)
    states.append(st_1)
    final_update = set()
    final_update.add(fac.UpdateInstruction(cc_0, False))
    symbol = pyxb.binding.content.WildcardUse(pyxb.binding.content.Wildcard(process_contents=pyxb.binding.content.Wildcard.PC_lax, namespace_constraint=(pyxb.binding.content.Wildcard.NC_not, 'http://docs.oasis-open.org/ws-rx/wsrm/200702')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wsrm.xsd', 11, 6))
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
SequenceType._Automaton = _BuildAutomaton()




CTD_ANON._AddElement(pyxb.binding.basis.element(pyxb.namespace.ExpandedName(Namespace, 'AcknowledgementRange'), CTD_ANON_, scope=CTD_ANON, location=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wsrm.xsd', 23, 14)))

CTD_ANON._AddElement(pyxb.binding.basis.element(pyxb.namespace.ExpandedName(Namespace, 'None'), CTD_ANON_2, scope=CTD_ANON, location=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wsrm.xsd', 31, 14)))

CTD_ANON._AddElement(pyxb.binding.basis.element(pyxb.namespace.ExpandedName(Namespace, 'Final'), CTD_ANON_3, scope=CTD_ANON, location=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wsrm.xsd', 37, 12)))

CTD_ANON._AddElement(pyxb.binding.basis.element(pyxb.namespace.ExpandedName(Namespace, 'Nack'), pyxb.binding.datatypes.unsignedLong, scope=CTD_ANON, location=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wsrm.xsd', 43, 10)))

CTD_ANON._AddElement(pyxb.binding.basis.element(pyxb.namespace.ExpandedName(Namespace, 'Identifier'), CTD_ANON_4, scope=CTD_ANON, location=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wsrm.xsd', 58, 2)))

def _BuildAutomaton_ ():
    # Remove this helper function from the namespace after it is invoked
    global _BuildAutomaton_
    del _BuildAutomaton_
    import pyxb.utils.fac as fac

    counters = set()
    cc_0 = fac.CounterCondition(min=0, max=1, metadata=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wsrm.xsd', 37, 12))
    counters.add(cc_0)
    cc_1 = fac.CounterCondition(min=0, max=None, metadata=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wsrm.xsd', 45, 8))
    counters.add(cc_1)
    states = []
    final_update = None
    symbol = pyxb.binding.content.ElementUse(CTD_ANON._UseForTag(pyxb.namespace.ExpandedName(Namespace, 'Identifier')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wsrm.xsd', 19, 8))
    st_0 = fac.State(symbol, is_initial=True, final_update=final_update, is_unordered_catenation=False)
    states.append(st_0)
    final_update = set()
    symbol = pyxb.binding.content.ElementUse(CTD_ANON._UseForTag(pyxb.namespace.ExpandedName(Namespace, 'AcknowledgementRange')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wsrm.xsd', 23, 14))
    st_1 = fac.State(symbol, is_initial=False, final_update=final_update, is_unordered_catenation=False)
    states.append(st_1)
    final_update = set()
    symbol = pyxb.binding.content.ElementUse(CTD_ANON._UseForTag(pyxb.namespace.ExpandedName(Namespace, 'None')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wsrm.xsd', 31, 14))
    st_2 = fac.State(symbol, is_initial=False, final_update=final_update, is_unordered_catenation=False)
    states.append(st_2)
    final_update = set()
    final_update.add(fac.UpdateInstruction(cc_0, False))
    symbol = pyxb.binding.content.ElementUse(CTD_ANON._UseForTag(pyxb.namespace.ExpandedName(Namespace, 'Final')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wsrm.xsd', 37, 12))
    st_3 = fac.State(symbol, is_initial=False, final_update=final_update, is_unordered_catenation=False)
    states.append(st_3)
    final_update = set()
    symbol = pyxb.binding.content.ElementUse(CTD_ANON._UseForTag(pyxb.namespace.ExpandedName(Namespace, 'Nack')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wsrm.xsd', 43, 10))
    st_4 = fac.State(symbol, is_initial=False, final_update=final_update, is_unordered_catenation=False)
    states.append(st_4)
    final_update = set()
    final_update.add(fac.UpdateInstruction(cc_1, False))
    symbol = pyxb.binding.content.WildcardUse(pyxb.binding.content.Wildcard(process_contents=pyxb.binding.content.Wildcard.PC_lax, namespace_constraint=(pyxb.binding.content.Wildcard.NC_not, 'http://docs.oasis-open.org/ws-rx/wsrm/200702')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wsrm.xsd', 45, 8))
    st_5 = fac.State(symbol, is_initial=False, final_update=final_update, is_unordered_catenation=False)
    states.append(st_5)
    transitions = []
    transitions.append(fac.Transition(st_1, [
         ]))
    transitions.append(fac.Transition(st_2, [
         ]))
    transitions.append(fac.Transition(st_4, [
         ]))
    st_0._set_transitionSet(transitions)
    transitions = []
    transitions.append(fac.Transition(st_1, [
         ]))
    transitions.append(fac.Transition(st_3, [
         ]))
    transitions.append(fac.Transition(st_5, [
         ]))
    st_1._set_transitionSet(transitions)
    transitions = []
    transitions.append(fac.Transition(st_3, [
         ]))
    transitions.append(fac.Transition(st_5, [
         ]))
    st_2._set_transitionSet(transitions)
    transitions = []
    transitions.append(fac.Transition(st_3, [
        fac.UpdateInstruction(cc_0, True) ]))
    transitions.append(fac.Transition(st_5, [
        fac.UpdateInstruction(cc_0, False) ]))
    st_3._set_transitionSet(transitions)
    transitions = []
    transitions.append(fac.Transition(st_4, [
         ]))
    transitions.append(fac.Transition(st_5, [
         ]))
    st_4._set_transitionSet(transitions)
    transitions = []
    transitions.append(fac.Transition(st_5, [
        fac.UpdateInstruction(cc_1, True) ]))
    st_5._set_transitionSet(transitions)
    return fac.Automaton(states, counters, False, containing_state=None)
CTD_ANON._Automaton = _BuildAutomaton_()




AckRequestedType._AddElement(pyxb.binding.basis.element(pyxb.namespace.ExpandedName(Namespace, 'Identifier'), CTD_ANON_4, scope=AckRequestedType, location=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wsrm.xsd', 58, 2)))

def _BuildAutomaton_2 ():
    # Remove this helper function from the namespace after it is invoked
    global _BuildAutomaton_2
    del _BuildAutomaton_2
    import pyxb.utils.fac as fac

    counters = set()
    cc_0 = fac.CounterCondition(min=0, max=None, metadata=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wsrm.xsd', 53, 6))
    counters.add(cc_0)
    states = []
    final_update = set()
    symbol = pyxb.binding.content.ElementUse(AckRequestedType._UseForTag(pyxb.namespace.ExpandedName(Namespace, 'Identifier')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wsrm.xsd', 52, 6))
    st_0 = fac.State(symbol, is_initial=True, final_update=final_update, is_unordered_catenation=False)
    states.append(st_0)
    final_update = set()
    final_update.add(fac.UpdateInstruction(cc_0, False))
    symbol = pyxb.binding.content.WildcardUse(pyxb.binding.content.Wildcard(process_contents=pyxb.binding.content.Wildcard.PC_lax, namespace_constraint=(pyxb.binding.content.Wildcard.NC_not, 'http://docs.oasis-open.org/ws-rx/wsrm/200702')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wsrm.xsd', 53, 6))
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
AckRequestedType._Automaton = _BuildAutomaton_2()




SequenceFaultType._AddElement(pyxb.binding.basis.element(pyxb.namespace.ExpandedName(Namespace, 'FaultCode'), FaultCodes, scope=SequenceFaultType, location=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wsrm.xsd', 101, 6)))

SequenceFaultType._AddElement(pyxb.binding.basis.element(pyxb.namespace.ExpandedName(Namespace, 'Detail'), DetailType, scope=SequenceFaultType, location=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wsrm.xsd', 102, 6)))

def _BuildAutomaton_3 ():
    # Remove this helper function from the namespace after it is invoked
    global _BuildAutomaton_3
    del _BuildAutomaton_3
    import pyxb.utils.fac as fac

    counters = set()
    cc_0 = fac.CounterCondition(min=0, max=1, metadata=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wsrm.xsd', 102, 6))
    counters.add(cc_0)
    cc_1 = fac.CounterCondition(min=0, max=None, metadata=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wsrm.xsd', 103, 6))
    counters.add(cc_1)
    states = []
    final_update = set()
    symbol = pyxb.binding.content.ElementUse(SequenceFaultType._UseForTag(pyxb.namespace.ExpandedName(Namespace, 'FaultCode')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wsrm.xsd', 101, 6))
    st_0 = fac.State(symbol, is_initial=True, final_update=final_update, is_unordered_catenation=False)
    states.append(st_0)
    final_update = set()
    final_update.add(fac.UpdateInstruction(cc_0, False))
    symbol = pyxb.binding.content.ElementUse(SequenceFaultType._UseForTag(pyxb.namespace.ExpandedName(Namespace, 'Detail')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wsrm.xsd', 102, 6))
    st_1 = fac.State(symbol, is_initial=False, final_update=final_update, is_unordered_catenation=False)
    states.append(st_1)
    final_update = set()
    final_update.add(fac.UpdateInstruction(cc_1, False))
    symbol = pyxb.binding.content.WildcardUse(pyxb.binding.content.Wildcard(process_contents=pyxb.binding.content.Wildcard.PC_lax, namespace_constraint=(pyxb.binding.content.Wildcard.NC_not, 'http://docs.oasis-open.org/ws-rx/wsrm/200702')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wsrm.xsd', 103, 6))
    st_2 = fac.State(symbol, is_initial=False, final_update=final_update, is_unordered_catenation=False)
    states.append(st_2)
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
    transitions.append(fac.Transition(st_2, [
        fac.UpdateInstruction(cc_1, True) ]))
    st_2._set_transitionSet(transitions)
    return fac.Automaton(states, counters, False, containing_state=None)
SequenceFaultType._Automaton = _BuildAutomaton_3()




def _BuildAutomaton_4 ():
    # Remove this helper function from the namespace after it is invoked
    global _BuildAutomaton_4
    del _BuildAutomaton_4
    import pyxb.utils.fac as fac

    counters = set()
    cc_0 = fac.CounterCondition(min=0, max=None, metadata=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wsrm.xsd', 109, 6))
    counters.add(cc_0)
    states = []
    final_update = set()
    final_update.add(fac.UpdateInstruction(cc_0, False))
    symbol = pyxb.binding.content.WildcardUse(pyxb.binding.content.Wildcard(process_contents=pyxb.binding.content.Wildcard.PC_lax, namespace_constraint=(pyxb.binding.content.Wildcard.NC_not, 'http://docs.oasis-open.org/ws-rx/wsrm/200702')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wsrm.xsd', 109, 6))
    st_0 = fac.State(symbol, is_initial=True, final_update=final_update, is_unordered_catenation=False)
    states.append(st_0)
    transitions = []
    transitions.append(fac.Transition(st_0, [
        fac.UpdateInstruction(cc_0, True) ]))
    st_0._set_transitionSet(transitions)
    return fac.Automaton(states, counters, True, containing_state=None)
DetailType._Automaton = _BuildAutomaton_4()




CreateSequenceType._AddElement(pyxb.binding.basis.element(pyxb.namespace.ExpandedName(Namespace, 'Offer'), OfferType, scope=CreateSequenceType, location=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wsrm.xsd', 124, 6)))

CreateSequenceType._AddElement(pyxb.binding.basis.element(pyxb.namespace.ExpandedName(Namespace, 'AcksTo'), pyxb.bundles.wssplat.wsa.EndpointReferenceType, scope=CreateSequenceType, location=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wsrm.xsd', 175, 2)))

CreateSequenceType._AddElement(pyxb.binding.basis.element(pyxb.namespace.ExpandedName(Namespace, 'Expires'), CTD_ANON_6, scope=CreateSequenceType, location=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wsrm.xsd', 193, 2)))

def _BuildAutomaton_5 ():
    # Remove this helper function from the namespace after it is invoked
    global _BuildAutomaton_5
    del _BuildAutomaton_5
    import pyxb.utils.fac as fac

    counters = set()
    cc_0 = fac.CounterCondition(min=0, max=1, metadata=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wsrm.xsd', 123, 6))
    counters.add(cc_0)
    cc_1 = fac.CounterCondition(min=0, max=1, metadata=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wsrm.xsd', 124, 6))
    counters.add(cc_1)
    cc_2 = fac.CounterCondition(min=0, max=None, metadata=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wsrm.xsd', 125, 6))
    counters.add(cc_2)
    states = []
    final_update = set()
    symbol = pyxb.binding.content.ElementUse(CreateSequenceType._UseForTag(pyxb.namespace.ExpandedName(Namespace, 'AcksTo')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wsrm.xsd', 122, 6))
    st_0 = fac.State(symbol, is_initial=True, final_update=final_update, is_unordered_catenation=False)
    states.append(st_0)
    final_update = set()
    final_update.add(fac.UpdateInstruction(cc_0, False))
    symbol = pyxb.binding.content.ElementUse(CreateSequenceType._UseForTag(pyxb.namespace.ExpandedName(Namespace, 'Expires')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wsrm.xsd', 123, 6))
    st_1 = fac.State(symbol, is_initial=False, final_update=final_update, is_unordered_catenation=False)
    states.append(st_1)
    final_update = set()
    final_update.add(fac.UpdateInstruction(cc_1, False))
    symbol = pyxb.binding.content.ElementUse(CreateSequenceType._UseForTag(pyxb.namespace.ExpandedName(Namespace, 'Offer')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wsrm.xsd', 124, 6))
    st_2 = fac.State(symbol, is_initial=False, final_update=final_update, is_unordered_catenation=False)
    states.append(st_2)
    final_update = set()
    final_update.add(fac.UpdateInstruction(cc_2, False))
    symbol = pyxb.binding.content.WildcardUse(pyxb.binding.content.Wildcard(process_contents=pyxb.binding.content.Wildcard.PC_lax, namespace_constraint=(pyxb.binding.content.Wildcard.NC_not, 'http://docs.oasis-open.org/ws-rx/wsrm/200702')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wsrm.xsd', 125, 6))
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
CreateSequenceType._Automaton = _BuildAutomaton_5()




CreateSequenceResponseType._AddElement(pyxb.binding.basis.element(pyxb.namespace.ExpandedName(Namespace, 'Identifier'), CTD_ANON_4, scope=CreateSequenceResponseType, location=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wsrm.xsd', 58, 2)))

CreateSequenceResponseType._AddElement(pyxb.binding.basis.element(pyxb.namespace.ExpandedName(Namespace, 'IncompleteSequenceBehavior'), IncompleteSequenceBehaviorType, scope=CreateSequenceResponseType, location=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wsrm.xsd', 139, 6)))

CreateSequenceResponseType._AddElement(pyxb.binding.basis.element(pyxb.namespace.ExpandedName(Namespace, 'Accept'), AcceptType, scope=CreateSequenceResponseType, location=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wsrm.xsd', 140, 6)))

CreateSequenceResponseType._AddElement(pyxb.binding.basis.element(pyxb.namespace.ExpandedName(Namespace, 'Expires'), CTD_ANON_6, scope=CreateSequenceResponseType, location=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wsrm.xsd', 193, 2)))

def _BuildAutomaton_6 ():
    # Remove this helper function from the namespace after it is invoked
    global _BuildAutomaton_6
    del _BuildAutomaton_6
    import pyxb.utils.fac as fac

    counters = set()
    cc_0 = fac.CounterCondition(min=0, max=1, metadata=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wsrm.xsd', 138, 6))
    counters.add(cc_0)
    cc_1 = fac.CounterCondition(min=0, max=1, metadata=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wsrm.xsd', 139, 6))
    counters.add(cc_1)
    cc_2 = fac.CounterCondition(min=0, max=1, metadata=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wsrm.xsd', 140, 6))
    counters.add(cc_2)
    cc_3 = fac.CounterCondition(min=0, max=None, metadata=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wsrm.xsd', 141, 6))
    counters.add(cc_3)
    states = []
    final_update = set()
    symbol = pyxb.binding.content.ElementUse(CreateSequenceResponseType._UseForTag(pyxb.namespace.ExpandedName(Namespace, 'Identifier')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wsrm.xsd', 137, 6))
    st_0 = fac.State(symbol, is_initial=True, final_update=final_update, is_unordered_catenation=False)
    states.append(st_0)
    final_update = set()
    final_update.add(fac.UpdateInstruction(cc_0, False))
    symbol = pyxb.binding.content.ElementUse(CreateSequenceResponseType._UseForTag(pyxb.namespace.ExpandedName(Namespace, 'Expires')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wsrm.xsd', 138, 6))
    st_1 = fac.State(symbol, is_initial=False, final_update=final_update, is_unordered_catenation=False)
    states.append(st_1)
    final_update = set()
    final_update.add(fac.UpdateInstruction(cc_1, False))
    symbol = pyxb.binding.content.ElementUse(CreateSequenceResponseType._UseForTag(pyxb.namespace.ExpandedName(Namespace, 'IncompleteSequenceBehavior')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wsrm.xsd', 139, 6))
    st_2 = fac.State(symbol, is_initial=False, final_update=final_update, is_unordered_catenation=False)
    states.append(st_2)
    final_update = set()
    final_update.add(fac.UpdateInstruction(cc_2, False))
    symbol = pyxb.binding.content.ElementUse(CreateSequenceResponseType._UseForTag(pyxb.namespace.ExpandedName(Namespace, 'Accept')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wsrm.xsd', 140, 6))
    st_3 = fac.State(symbol, is_initial=False, final_update=final_update, is_unordered_catenation=False)
    states.append(st_3)
    final_update = set()
    final_update.add(fac.UpdateInstruction(cc_3, False))
    symbol = pyxb.binding.content.WildcardUse(pyxb.binding.content.Wildcard(process_contents=pyxb.binding.content.Wildcard.PC_lax, namespace_constraint=(pyxb.binding.content.Wildcard.NC_not, 'http://docs.oasis-open.org/ws-rx/wsrm/200702')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wsrm.xsd', 141, 6))
    st_4 = fac.State(symbol, is_initial=False, final_update=final_update, is_unordered_catenation=False)
    states.append(st_4)
    transitions = []
    transitions.append(fac.Transition(st_1, [
         ]))
    transitions.append(fac.Transition(st_2, [
         ]))
    transitions.append(fac.Transition(st_3, [
         ]))
    transitions.append(fac.Transition(st_4, [
         ]))
    st_0._set_transitionSet(transitions)
    transitions = []
    transitions.append(fac.Transition(st_1, [
        fac.UpdateInstruction(cc_0, True) ]))
    transitions.append(fac.Transition(st_2, [
        fac.UpdateInstruction(cc_0, False) ]))
    transitions.append(fac.Transition(st_3, [
        fac.UpdateInstruction(cc_0, False) ]))
    transitions.append(fac.Transition(st_4, [
        fac.UpdateInstruction(cc_0, False) ]))
    st_1._set_transitionSet(transitions)
    transitions = []
    transitions.append(fac.Transition(st_2, [
        fac.UpdateInstruction(cc_1, True) ]))
    transitions.append(fac.Transition(st_3, [
        fac.UpdateInstruction(cc_1, False) ]))
    transitions.append(fac.Transition(st_4, [
        fac.UpdateInstruction(cc_1, False) ]))
    st_2._set_transitionSet(transitions)
    transitions = []
    transitions.append(fac.Transition(st_3, [
        fac.UpdateInstruction(cc_2, True) ]))
    transitions.append(fac.Transition(st_4, [
        fac.UpdateInstruction(cc_2, False) ]))
    st_3._set_transitionSet(transitions)
    transitions = []
    transitions.append(fac.Transition(st_4, [
        fac.UpdateInstruction(cc_3, True) ]))
    st_4._set_transitionSet(transitions)
    return fac.Automaton(states, counters, False, containing_state=None)
CreateSequenceResponseType._Automaton = _BuildAutomaton_6()




CloseSequenceType._AddElement(pyxb.binding.basis.element(pyxb.namespace.ExpandedName(Namespace, 'Identifier'), CTD_ANON_4, scope=CloseSequenceType, location=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wsrm.xsd', 58, 2)))

CloseSequenceType._AddElement(pyxb.binding.basis.element(pyxb.namespace.ExpandedName(Namespace, 'LastMsgNumber'), MessageNumberType, scope=CloseSequenceType, location=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wsrm.xsd', 148, 6)))

def _BuildAutomaton_7 ():
    # Remove this helper function from the namespace after it is invoked
    global _BuildAutomaton_7
    del _BuildAutomaton_7
    import pyxb.utils.fac as fac

    counters = set()
    cc_0 = fac.CounterCondition(min=0, max=1, metadata=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wsrm.xsd', 148, 6))
    counters.add(cc_0)
    cc_1 = fac.CounterCondition(min=0, max=None, metadata=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wsrm.xsd', 149, 6))
    counters.add(cc_1)
    states = []
    final_update = set()
    symbol = pyxb.binding.content.ElementUse(CloseSequenceType._UseForTag(pyxb.namespace.ExpandedName(Namespace, 'Identifier')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wsrm.xsd', 147, 6))
    st_0 = fac.State(symbol, is_initial=True, final_update=final_update, is_unordered_catenation=False)
    states.append(st_0)
    final_update = set()
    final_update.add(fac.UpdateInstruction(cc_0, False))
    symbol = pyxb.binding.content.ElementUse(CloseSequenceType._UseForTag(pyxb.namespace.ExpandedName(Namespace, 'LastMsgNumber')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wsrm.xsd', 148, 6))
    st_1 = fac.State(symbol, is_initial=False, final_update=final_update, is_unordered_catenation=False)
    states.append(st_1)
    final_update = set()
    final_update.add(fac.UpdateInstruction(cc_1, False))
    symbol = pyxb.binding.content.WildcardUse(pyxb.binding.content.Wildcard(process_contents=pyxb.binding.content.Wildcard.PC_lax, namespace_constraint=(pyxb.binding.content.Wildcard.NC_not, 'http://docs.oasis-open.org/ws-rx/wsrm/200702')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wsrm.xsd', 149, 6))
    st_2 = fac.State(symbol, is_initial=False, final_update=final_update, is_unordered_catenation=False)
    states.append(st_2)
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
    transitions.append(fac.Transition(st_2, [
        fac.UpdateInstruction(cc_1, True) ]))
    st_2._set_transitionSet(transitions)
    return fac.Automaton(states, counters, False, containing_state=None)
CloseSequenceType._Automaton = _BuildAutomaton_7()




CloseSequenceResponseType._AddElement(pyxb.binding.basis.element(pyxb.namespace.ExpandedName(Namespace, 'Identifier'), CTD_ANON_4, scope=CloseSequenceResponseType, location=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wsrm.xsd', 58, 2)))

def _BuildAutomaton_8 ():
    # Remove this helper function from the namespace after it is invoked
    global _BuildAutomaton_8
    del _BuildAutomaton_8
    import pyxb.utils.fac as fac

    counters = set()
    cc_0 = fac.CounterCondition(min=0, max=None, metadata=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wsrm.xsd', 156, 6))
    counters.add(cc_0)
    states = []
    final_update = set()
    symbol = pyxb.binding.content.ElementUse(CloseSequenceResponseType._UseForTag(pyxb.namespace.ExpandedName(Namespace, 'Identifier')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wsrm.xsd', 155, 6))
    st_0 = fac.State(symbol, is_initial=True, final_update=final_update, is_unordered_catenation=False)
    states.append(st_0)
    final_update = set()
    final_update.add(fac.UpdateInstruction(cc_0, False))
    symbol = pyxb.binding.content.WildcardUse(pyxb.binding.content.Wildcard(process_contents=pyxb.binding.content.Wildcard.PC_lax, namespace_constraint=(pyxb.binding.content.Wildcard.NC_not, 'http://docs.oasis-open.org/ws-rx/wsrm/200702')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wsrm.xsd', 156, 6))
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
CloseSequenceResponseType._Automaton = _BuildAutomaton_8()




TerminateSequenceType._AddElement(pyxb.binding.basis.element(pyxb.namespace.ExpandedName(Namespace, 'Identifier'), CTD_ANON_4, scope=TerminateSequenceType, location=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wsrm.xsd', 58, 2)))

TerminateSequenceType._AddElement(pyxb.binding.basis.element(pyxb.namespace.ExpandedName(Namespace, 'LastMsgNumber'), MessageNumberType, scope=TerminateSequenceType, location=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wsrm.xsd', 163, 6)))

def _BuildAutomaton_9 ():
    # Remove this helper function from the namespace after it is invoked
    global _BuildAutomaton_9
    del _BuildAutomaton_9
    import pyxb.utils.fac as fac

    counters = set()
    cc_0 = fac.CounterCondition(min=0, max=1, metadata=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wsrm.xsd', 163, 6))
    counters.add(cc_0)
    cc_1 = fac.CounterCondition(min=0, max=None, metadata=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wsrm.xsd', 164, 6))
    counters.add(cc_1)
    states = []
    final_update = set()
    symbol = pyxb.binding.content.ElementUse(TerminateSequenceType._UseForTag(pyxb.namespace.ExpandedName(Namespace, 'Identifier')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wsrm.xsd', 162, 6))
    st_0 = fac.State(symbol, is_initial=True, final_update=final_update, is_unordered_catenation=False)
    states.append(st_0)
    final_update = set()
    final_update.add(fac.UpdateInstruction(cc_0, False))
    symbol = pyxb.binding.content.ElementUse(TerminateSequenceType._UseForTag(pyxb.namespace.ExpandedName(Namespace, 'LastMsgNumber')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wsrm.xsd', 163, 6))
    st_1 = fac.State(symbol, is_initial=False, final_update=final_update, is_unordered_catenation=False)
    states.append(st_1)
    final_update = set()
    final_update.add(fac.UpdateInstruction(cc_1, False))
    symbol = pyxb.binding.content.WildcardUse(pyxb.binding.content.Wildcard(process_contents=pyxb.binding.content.Wildcard.PC_lax, namespace_constraint=(pyxb.binding.content.Wildcard.NC_not, 'http://docs.oasis-open.org/ws-rx/wsrm/200702')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wsrm.xsd', 164, 6))
    st_2 = fac.State(symbol, is_initial=False, final_update=final_update, is_unordered_catenation=False)
    states.append(st_2)
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
    transitions.append(fac.Transition(st_2, [
        fac.UpdateInstruction(cc_1, True) ]))
    st_2._set_transitionSet(transitions)
    return fac.Automaton(states, counters, False, containing_state=None)
TerminateSequenceType._Automaton = _BuildAutomaton_9()




TerminateSequenceResponseType._AddElement(pyxb.binding.basis.element(pyxb.namespace.ExpandedName(Namespace, 'Identifier'), CTD_ANON_4, scope=TerminateSequenceResponseType, location=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wsrm.xsd', 58, 2)))

def _BuildAutomaton_10 ():
    # Remove this helper function from the namespace after it is invoked
    global _BuildAutomaton_10
    del _BuildAutomaton_10
    import pyxb.utils.fac as fac

    counters = set()
    cc_0 = fac.CounterCondition(min=0, max=None, metadata=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wsrm.xsd', 171, 6))
    counters.add(cc_0)
    states = []
    final_update = set()
    symbol = pyxb.binding.content.ElementUse(TerminateSequenceResponseType._UseForTag(pyxb.namespace.ExpandedName(Namespace, 'Identifier')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wsrm.xsd', 170, 6))
    st_0 = fac.State(symbol, is_initial=True, final_update=final_update, is_unordered_catenation=False)
    states.append(st_0)
    final_update = set()
    final_update.add(fac.UpdateInstruction(cc_0, False))
    symbol = pyxb.binding.content.WildcardUse(pyxb.binding.content.Wildcard(process_contents=pyxb.binding.content.Wildcard.PC_lax, namespace_constraint=(pyxb.binding.content.Wildcard.NC_not, 'http://docs.oasis-open.org/ws-rx/wsrm/200702')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wsrm.xsd', 171, 6))
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
TerminateSequenceResponseType._Automaton = _BuildAutomaton_10()




OfferType._AddElement(pyxb.binding.basis.element(pyxb.namespace.ExpandedName(Namespace, 'Identifier'), CTD_ANON_4, scope=OfferType, location=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wsrm.xsd', 58, 2)))

OfferType._AddElement(pyxb.binding.basis.element(pyxb.namespace.ExpandedName(Namespace, 'Endpoint'), pyxb.bundles.wssplat.wsa.EndpointReferenceType, scope=OfferType, location=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wsrm.xsd', 179, 6)))

OfferType._AddElement(pyxb.binding.basis.element(pyxb.namespace.ExpandedName(Namespace, 'IncompleteSequenceBehavior'), IncompleteSequenceBehaviorType, scope=OfferType, location=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wsrm.xsd', 181, 6)))

OfferType._AddElement(pyxb.binding.basis.element(pyxb.namespace.ExpandedName(Namespace, 'Expires'), CTD_ANON_6, scope=OfferType, location=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wsrm.xsd', 193, 2)))

def _BuildAutomaton_11 ():
    # Remove this helper function from the namespace after it is invoked
    global _BuildAutomaton_11
    del _BuildAutomaton_11
    import pyxb.utils.fac as fac

    counters = set()
    cc_0 = fac.CounterCondition(min=0, max=1, metadata=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wsrm.xsd', 180, 6))
    counters.add(cc_0)
    cc_1 = fac.CounterCondition(min=0, max=1, metadata=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wsrm.xsd', 181, 6))
    counters.add(cc_1)
    cc_2 = fac.CounterCondition(min=0, max=None, metadata=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wsrm.xsd', 182, 6))
    counters.add(cc_2)
    states = []
    final_update = None
    symbol = pyxb.binding.content.ElementUse(OfferType._UseForTag(pyxb.namespace.ExpandedName(Namespace, 'Identifier')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wsrm.xsd', 178, 6))
    st_0 = fac.State(symbol, is_initial=True, final_update=final_update, is_unordered_catenation=False)
    states.append(st_0)
    final_update = set()
    symbol = pyxb.binding.content.ElementUse(OfferType._UseForTag(pyxb.namespace.ExpandedName(Namespace, 'Endpoint')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wsrm.xsd', 179, 6))
    st_1 = fac.State(symbol, is_initial=False, final_update=final_update, is_unordered_catenation=False)
    states.append(st_1)
    final_update = set()
    final_update.add(fac.UpdateInstruction(cc_0, False))
    symbol = pyxb.binding.content.ElementUse(OfferType._UseForTag(pyxb.namespace.ExpandedName(Namespace, 'Expires')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wsrm.xsd', 180, 6))
    st_2 = fac.State(symbol, is_initial=False, final_update=final_update, is_unordered_catenation=False)
    states.append(st_2)
    final_update = set()
    final_update.add(fac.UpdateInstruction(cc_1, False))
    symbol = pyxb.binding.content.ElementUse(OfferType._UseForTag(pyxb.namespace.ExpandedName(Namespace, 'IncompleteSequenceBehavior')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wsrm.xsd', 181, 6))
    st_3 = fac.State(symbol, is_initial=False, final_update=final_update, is_unordered_catenation=False)
    states.append(st_3)
    final_update = set()
    final_update.add(fac.UpdateInstruction(cc_2, False))
    symbol = pyxb.binding.content.WildcardUse(pyxb.binding.content.Wildcard(process_contents=pyxb.binding.content.Wildcard.PC_lax, namespace_constraint=(pyxb.binding.content.Wildcard.NC_not, 'http://docs.oasis-open.org/ws-rx/wsrm/200702')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wsrm.xsd', 182, 6))
    st_4 = fac.State(symbol, is_initial=False, final_update=final_update, is_unordered_catenation=False)
    states.append(st_4)
    transitions = []
    transitions.append(fac.Transition(st_1, [
         ]))
    st_0._set_transitionSet(transitions)
    transitions = []
    transitions.append(fac.Transition(st_2, [
         ]))
    transitions.append(fac.Transition(st_3, [
         ]))
    transitions.append(fac.Transition(st_4, [
         ]))
    st_1._set_transitionSet(transitions)
    transitions = []
    transitions.append(fac.Transition(st_2, [
        fac.UpdateInstruction(cc_0, True) ]))
    transitions.append(fac.Transition(st_3, [
        fac.UpdateInstruction(cc_0, False) ]))
    transitions.append(fac.Transition(st_4, [
        fac.UpdateInstruction(cc_0, False) ]))
    st_2._set_transitionSet(transitions)
    transitions = []
    transitions.append(fac.Transition(st_3, [
        fac.UpdateInstruction(cc_1, True) ]))
    transitions.append(fac.Transition(st_4, [
        fac.UpdateInstruction(cc_1, False) ]))
    st_3._set_transitionSet(transitions)
    transitions = []
    transitions.append(fac.Transition(st_4, [
        fac.UpdateInstruction(cc_2, True) ]))
    st_4._set_transitionSet(transitions)
    return fac.Automaton(states, counters, False, containing_state=None)
OfferType._Automaton = _BuildAutomaton_11()




AcceptType._AddElement(pyxb.binding.basis.element(pyxb.namespace.ExpandedName(Namespace, 'AcksTo'), pyxb.bundles.wssplat.wsa.EndpointReferenceType, scope=AcceptType, location=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wsrm.xsd', 175, 2)))

def _BuildAutomaton_12 ():
    # Remove this helper function from the namespace after it is invoked
    global _BuildAutomaton_12
    del _BuildAutomaton_12
    import pyxb.utils.fac as fac

    counters = set()
    cc_0 = fac.CounterCondition(min=0, max=None, metadata=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wsrm.xsd', 189, 6))
    counters.add(cc_0)
    states = []
    final_update = set()
    symbol = pyxb.binding.content.ElementUse(AcceptType._UseForTag(pyxb.namespace.ExpandedName(Namespace, 'AcksTo')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wsrm.xsd', 188, 6))
    st_0 = fac.State(symbol, is_initial=True, final_update=final_update, is_unordered_catenation=False)
    states.append(st_0)
    final_update = set()
    final_update.add(fac.UpdateInstruction(cc_0, False))
    symbol = pyxb.binding.content.WildcardUse(pyxb.binding.content.Wildcard(process_contents=pyxb.binding.content.Wildcard.PC_lax, namespace_constraint=(pyxb.binding.content.Wildcard.NC_not, 'http://docs.oasis-open.org/ws-rx/wsrm/200702')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wsrm.xsd', 189, 6))
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
AcceptType._Automaton = _BuildAutomaton_12()

