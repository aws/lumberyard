# ./pyxb/bundles/wssplat/raw/wsrf_br.py
# -*- coding: utf-8 -*-
# PyXB bindings for NM:dd5653a4ef6cf46e4740ac1eaece204a915200aa
# Generated 2014-10-19 06:25:00.905939 by PyXB version 1.2.4 using Python 2.7.3.final.0
# Namespace http://docs.oasis-open.org/wsn/br-2

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
_GenerationUID = pyxb.utils.utility.UniqueIdentifier('urn:uuid:906eb6e4-5782-11e4-b088-c8600024e903')

# Version of PyXB used to generate the bindings
_PyXBVersion = '1.2.4'
# Generated bindings are not compatible across PyXB versions
if pyxb.__version__ != _PyXBVersion:
    raise pyxb.PyXBVersionError(_PyXBVersion)

# Import bindings for namespaces imported into schema
import pyxb.bundles.wssplat.wsa
import pyxb.bundles.wssplat.wstop
import pyxb.binding.datatypes
import pyxb.bundles.wssplat.wsrf_bf
import pyxb.bundles.wssplat.wsnt

# NOTE: All namespace declarations are reserved within the binding
Namespace = pyxb.namespace.NamespaceForURI('http://docs.oasis-open.org/wsn/br-2', create_if_missing=True)
Namespace.configureCategories(['typeBinding', 'elementBinding'])
_Namespace_wsn_b = pyxb.bundles.wssplat.wsnt.Namespace
_Namespace_wsn_b.configureCategories(['typeBinding', 'elementBinding'])
_Namespace_wstop = pyxb.bundles.wssplat.wstop.Namespace
_Namespace_wstop.configureCategories(['typeBinding', 'elementBinding'])
_Namespace_wsrf_bf = pyxb.bundles.wssplat.wsrf_bf.Namespace
_Namespace_wsrf_bf.configureCategories(['typeBinding', 'elementBinding'])

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


# Complex type [anonymous] with content type ELEMENT_ONLY
class CTD_ANON (pyxb.binding.basis.complexTypeDefinition):
    """Complex type [anonymous] with content type ELEMENT_ONLY"""
    _TypeDefinition = None
    _ContentTypeTag = pyxb.binding.basis.complexTypeDefinition._CT_ELEMENT_ONLY
    _Abstract = False
    _ExpandedName = None
    _XSDLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wsrf_br.xsd', 57, 12)
    _ElementMap = {}
    _AttributeMap = {}
    # Base type is pyxb.binding.datatypes.anyType
    
    # Element {http://docs.oasis-open.org/wsn/b-2}TopicExpression uses Python identifier TopicExpression
    __TopicExpression = pyxb.binding.content.ElementDeclaration(pyxb.namespace.ExpandedName(_Namespace_wsn_b, 'TopicExpression'), 'TopicExpression', '__httpdocs_oasis_open_orgwsnbr_2_CTD_ANON_httpdocs_oasis_open_orgwsnb_2TopicExpression', True, pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wsnt.xsd', 71, 2), )

    
    TopicExpression = property(__TopicExpression.value, __TopicExpression.set, None, None)

    
    # Element {http://docs.oasis-open.org/wsn/b-2}FixedTopicSet uses Python identifier FixedTopicSet
    __FixedTopicSet = pyxb.binding.content.ElementDeclaration(pyxb.namespace.ExpandedName(_Namespace_wsn_b, 'FixedTopicSet'), 'FixedTopicSet', '__httpdocs_oasis_open_orgwsnbr_2_CTD_ANON_httpdocs_oasis_open_orgwsnb_2FixedTopicSet', False, pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wsnt.xsd', 72, 2), )

    
    FixedTopicSet = property(__FixedTopicSet.value, __FixedTopicSet.set, None, None)

    
    # Element {http://docs.oasis-open.org/wsn/b-2}TopicExpressionDialect uses Python identifier TopicExpressionDialect
    __TopicExpressionDialect = pyxb.binding.content.ElementDeclaration(pyxb.namespace.ExpandedName(_Namespace_wsn_b, 'TopicExpressionDialect'), 'TopicExpressionDialect', '__httpdocs_oasis_open_orgwsnbr_2_CTD_ANON_httpdocs_oasis_open_orgwsnb_2TopicExpressionDialect', True, pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wsnt.xsd', 73, 2), )

    
    TopicExpressionDialect = property(__TopicExpressionDialect.value, __TopicExpressionDialect.set, None, None)

    
    # Element {http://docs.oasis-open.org/wsn/br-2}RequiresRegistration uses Python identifier RequiresRegistration
    __RequiresRegistration = pyxb.binding.content.ElementDeclaration(pyxb.namespace.ExpandedName(Namespace, 'RequiresRegistration'), 'RequiresRegistration', '__httpdocs_oasis_open_orgwsnbr_2_CTD_ANON_httpdocs_oasis_open_orgwsnbr_2RequiresRegistration', False, pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wsrf_br.xsd', 43, 3), )

    
    RequiresRegistration = property(__RequiresRegistration.value, __RequiresRegistration.set, None, None)

    
    # Element {http://docs.oasis-open.org/wsn/t-1}TopicSet uses Python identifier TopicSet
    __TopicSet = pyxb.binding.content.ElementDeclaration(pyxb.namespace.ExpandedName(_Namespace_wstop, 'TopicSet'), 'TopicSet', '__httpdocs_oasis_open_orgwsnbr_2_CTD_ANON_httpdocs_oasis_open_orgwsnt_1TopicSet', False, pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wstop.xsd', 133, 2), )

    
    TopicSet = property(__TopicSet.value, __TopicSet.set, None, None)

    _ElementMap.update({
        __TopicExpression.name() : __TopicExpression,
        __FixedTopicSet.name() : __FixedTopicSet,
        __TopicExpressionDialect.name() : __TopicExpressionDialect,
        __RequiresRegistration.name() : __RequiresRegistration,
        __TopicSet.name() : __TopicSet
    })
    _AttributeMap.update({
        
    })



# Complex type [anonymous] with content type ELEMENT_ONLY
class CTD_ANON_ (pyxb.binding.basis.complexTypeDefinition):
    """Complex type [anonymous] with content type ELEMENT_ONLY"""
    _TypeDefinition = None
    _ContentTypeTag = pyxb.binding.basis.complexTypeDefinition._CT_ELEMENT_ONLY
    _Abstract = False
    _ExpandedName = None
    _XSDLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wsrf_br.xsd', 77, 12)
    _ElementMap = {}
    _AttributeMap = {}
    # Base type is pyxb.binding.datatypes.anyType
    
    # Element {http://docs.oasis-open.org/wsn/br-2}PublisherReference uses Python identifier PublisherReference
    __PublisherReference = pyxb.binding.content.ElementDeclaration(pyxb.namespace.ExpandedName(Namespace, 'PublisherReference'), 'PublisherReference', '__httpdocs_oasis_open_orgwsnbr_2_CTD_ANON__httpdocs_oasis_open_orgwsnbr_2PublisherReference', False, pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wsrf_br.xsd', 46, 3), )

    
    PublisherReference = property(__PublisherReference.value, __PublisherReference.set, None, None)

    
    # Element {http://docs.oasis-open.org/wsn/br-2}Topic uses Python identifier Topic
    __Topic = pyxb.binding.content.ElementDeclaration(pyxb.namespace.ExpandedName(Namespace, 'Topic'), 'Topic', '__httpdocs_oasis_open_orgwsnbr_2_CTD_ANON__httpdocs_oasis_open_orgwsnbr_2Topic', True, pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wsrf_br.xsd', 50, 3), )

    
    Topic = property(__Topic.value, __Topic.set, None, None)

    
    # Element {http://docs.oasis-open.org/wsn/br-2}Demand uses Python identifier Demand
    __Demand = pyxb.binding.content.ElementDeclaration(pyxb.namespace.ExpandedName(Namespace, 'Demand'), 'Demand', '__httpdocs_oasis_open_orgwsnbr_2_CTD_ANON__httpdocs_oasis_open_orgwsnbr_2Demand', False, pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wsrf_br.xsd', 52, 3), )

    
    Demand = property(__Demand.value, __Demand.set, None, None)

    
    # Element {http://docs.oasis-open.org/wsn/br-2}CreationTime uses Python identifier CreationTime
    __CreationTime = pyxb.binding.content.ElementDeclaration(pyxb.namespace.ExpandedName(Namespace, 'CreationTime'), 'CreationTime', '__httpdocs_oasis_open_orgwsnbr_2_CTD_ANON__httpdocs_oasis_open_orgwsnbr_2CreationTime', False, pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wsrf_br.xsd', 54, 3), )

    
    CreationTime = property(__CreationTime.value, __CreationTime.set, None, None)

    _ElementMap.update({
        __PublisherReference.name() : __PublisherReference,
        __Topic.name() : __Topic,
        __Demand.name() : __Demand,
        __CreationTime.name() : __CreationTime
    })
    _AttributeMap.update({
        
    })



# Complex type [anonymous] with content type ELEMENT_ONLY
class CTD_ANON_2 (pyxb.binding.basis.complexTypeDefinition):
    """Complex type [anonymous] with content type ELEMENT_ONLY"""
    _TypeDefinition = None
    _ContentTypeTag = pyxb.binding.basis.complexTypeDefinition._CT_ELEMENT_ONLY
    _Abstract = False
    _ExpandedName = None
    _XSDLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wsrf_br.xsd', 93, 12)
    _ElementMap = {}
    _AttributeMap = {}
    # Base type is pyxb.binding.datatypes.anyType
    
    # Element {http://docs.oasis-open.org/wsn/br-2}PublisherReference uses Python identifier PublisherReference
    __PublisherReference = pyxb.binding.content.ElementDeclaration(pyxb.namespace.ExpandedName(Namespace, 'PublisherReference'), 'PublisherReference', '__httpdocs_oasis_open_orgwsnbr_2_CTD_ANON_2_httpdocs_oasis_open_orgwsnbr_2PublisherReference', False, pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wsrf_br.xsd', 95, 18), )

    
    PublisherReference = property(__PublisherReference.value, __PublisherReference.set, None, None)

    
    # Element {http://docs.oasis-open.org/wsn/br-2}Topic uses Python identifier Topic
    __Topic = pyxb.binding.content.ElementDeclaration(pyxb.namespace.ExpandedName(Namespace, 'Topic'), 'Topic', '__httpdocs_oasis_open_orgwsnbr_2_CTD_ANON_2_httpdocs_oasis_open_orgwsnbr_2Topic', True, pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wsrf_br.xsd', 98, 18), )

    
    Topic = property(__Topic.value, __Topic.set, None, None)

    
    # Element {http://docs.oasis-open.org/wsn/br-2}Demand uses Python identifier Demand
    __Demand = pyxb.binding.content.ElementDeclaration(pyxb.namespace.ExpandedName(Namespace, 'Demand'), 'Demand', '__httpdocs_oasis_open_orgwsnbr_2_CTD_ANON_2_httpdocs_oasis_open_orgwsnbr_2Demand', False, pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wsrf_br.xsd', 101, 18), )

    
    Demand = property(__Demand.value, __Demand.set, None, None)

    
    # Element {http://docs.oasis-open.org/wsn/br-2}InitialTerminationTime uses Python identifier InitialTerminationTime
    __InitialTerminationTime = pyxb.binding.content.ElementDeclaration(pyxb.namespace.ExpandedName(Namespace, 'InitialTerminationTime'), 'InitialTerminationTime', '__httpdocs_oasis_open_orgwsnbr_2_CTD_ANON_2_httpdocs_oasis_open_orgwsnbr_2InitialTerminationTime', False, pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wsrf_br.xsd', 104, 18), )

    
    InitialTerminationTime = property(__InitialTerminationTime.value, __InitialTerminationTime.set, None, None)

    _HasWildcardElement = True
    _ElementMap.update({
        __PublisherReference.name() : __PublisherReference,
        __Topic.name() : __Topic,
        __Demand.name() : __Demand,
        __InitialTerminationTime.name() : __InitialTerminationTime
    })
    _AttributeMap.update({
        
    })



# Complex type [anonymous] with content type ELEMENT_ONLY
class CTD_ANON_3 (pyxb.binding.basis.complexTypeDefinition):
    """Complex type [anonymous] with content type ELEMENT_ONLY"""
    _TypeDefinition = None
    _ContentTypeTag = pyxb.binding.basis.complexTypeDefinition._CT_ELEMENT_ONLY
    _Abstract = False
    _ExpandedName = None
    _XSDLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wsrf_br.xsd', 114, 12)
    _ElementMap = {}
    _AttributeMap = {}
    # Base type is pyxb.binding.datatypes.anyType
    
    # Element {http://docs.oasis-open.org/wsn/br-2}PublisherRegistrationReference uses Python identifier PublisherRegistrationReference
    __PublisherRegistrationReference = pyxb.binding.content.ElementDeclaration(pyxb.namespace.ExpandedName(Namespace, 'PublisherRegistrationReference'), 'PublisherRegistrationReference', '__httpdocs_oasis_open_orgwsnbr_2_CTD_ANON_3_httpdocs_oasis_open_orgwsnbr_2PublisherRegistrationReference', False, pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wsrf_br.xsd', 116, 18), )

    
    PublisherRegistrationReference = property(__PublisherRegistrationReference.value, __PublisherRegistrationReference.set, None, None)

    
    # Element {http://docs.oasis-open.org/wsn/br-2}ConsumerReference uses Python identifier ConsumerReference
    __ConsumerReference = pyxb.binding.content.ElementDeclaration(pyxb.namespace.ExpandedName(Namespace, 'ConsumerReference'), 'ConsumerReference', '__httpdocs_oasis_open_orgwsnbr_2_CTD_ANON_3_httpdocs_oasis_open_orgwsnbr_2ConsumerReference', False, pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wsrf_br.xsd', 119, 5), )

    
    ConsumerReference = property(__ConsumerReference.value, __ConsumerReference.set, None, None)

    _ElementMap.update({
        __PublisherRegistrationReference.name() : __PublisherRegistrationReference,
        __ConsumerReference.name() : __ConsumerReference
    })
    _AttributeMap.update({
        
    })



# Complex type {http://docs.oasis-open.org/wsn/br-2}PublisherRegistrationRejectedFaultType with content type ELEMENT_ONLY
class PublisherRegistrationRejectedFaultType (pyxb.bundles.wssplat.wsrf_bf.BaseFaultType):
    """Complex type {http://docs.oasis-open.org/wsn/br-2}PublisherRegistrationRejectedFaultType with content type ELEMENT_ONLY"""
    _TypeDefinition = None
    _ContentTypeTag = pyxb.binding.basis.complexTypeDefinition._CT_ELEMENT_ONLY
    _Abstract = False
    _ExpandedName = pyxb.namespace.ExpandedName(Namespace, 'PublisherRegistrationRejectedFaultType')
    _XSDLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wsrf_br.xsd', 127, 3)
    _ElementMap = pyxb.bundles.wssplat.wsrf_bf.BaseFaultType._ElementMap.copy()
    _AttributeMap = pyxb.bundles.wssplat.wsrf_bf.BaseFaultType._AttributeMap.copy()
    # Base type is pyxb.bundles.wssplat.wsrf_bf.BaseFaultType
    
    # Element Timestamp ({http://docs.oasis-open.org/wsrf/bf-2}Timestamp) inherited from {http://docs.oasis-open.org/wsrf/bf-2}BaseFaultType
    
    # Element Originator ({http://docs.oasis-open.org/wsrf/bf-2}Originator) inherited from {http://docs.oasis-open.org/wsrf/bf-2}BaseFaultType
    
    # Element ErrorCode ({http://docs.oasis-open.org/wsrf/bf-2}ErrorCode) inherited from {http://docs.oasis-open.org/wsrf/bf-2}BaseFaultType
    
    # Element Description ({http://docs.oasis-open.org/wsrf/bf-2}Description) inherited from {http://docs.oasis-open.org/wsrf/bf-2}BaseFaultType
    
    # Element FaultCause ({http://docs.oasis-open.org/wsrf/bf-2}FaultCause) inherited from {http://docs.oasis-open.org/wsrf/bf-2}BaseFaultType
    _AttributeWildcard = pyxb.binding.content.Wildcard(process_contents=pyxb.binding.content.Wildcard.PC_lax, namespace_constraint=(pyxb.binding.content.Wildcard.NC_not, 'http://docs.oasis-open.org/wsrf/bf-2'))
    _HasWildcardElement = True
    _ElementMap.update({
        
    })
    _AttributeMap.update({
        
    })
Namespace.addCategoryObject('typeBinding', 'PublisherRegistrationRejectedFaultType', PublisherRegistrationRejectedFaultType)


# Complex type {http://docs.oasis-open.org/wsn/br-2}PublisherRegistrationFailedFaultType with content type ELEMENT_ONLY
class PublisherRegistrationFailedFaultType (pyxb.bundles.wssplat.wsrf_bf.BaseFaultType):
    """Complex type {http://docs.oasis-open.org/wsn/br-2}PublisherRegistrationFailedFaultType with content type ELEMENT_ONLY"""
    _TypeDefinition = None
    _ContentTypeTag = pyxb.binding.basis.complexTypeDefinition._CT_ELEMENT_ONLY
    _Abstract = False
    _ExpandedName = pyxb.namespace.ExpandedName(Namespace, 'PublisherRegistrationFailedFaultType')
    _XSDLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wsrf_br.xsd', 135, 3)
    _ElementMap = pyxb.bundles.wssplat.wsrf_bf.BaseFaultType._ElementMap.copy()
    _AttributeMap = pyxb.bundles.wssplat.wsrf_bf.BaseFaultType._AttributeMap.copy()
    # Base type is pyxb.bundles.wssplat.wsrf_bf.BaseFaultType
    
    # Element Timestamp ({http://docs.oasis-open.org/wsrf/bf-2}Timestamp) inherited from {http://docs.oasis-open.org/wsrf/bf-2}BaseFaultType
    
    # Element Originator ({http://docs.oasis-open.org/wsrf/bf-2}Originator) inherited from {http://docs.oasis-open.org/wsrf/bf-2}BaseFaultType
    
    # Element ErrorCode ({http://docs.oasis-open.org/wsrf/bf-2}ErrorCode) inherited from {http://docs.oasis-open.org/wsrf/bf-2}BaseFaultType
    
    # Element Description ({http://docs.oasis-open.org/wsrf/bf-2}Description) inherited from {http://docs.oasis-open.org/wsrf/bf-2}BaseFaultType
    
    # Element FaultCause ({http://docs.oasis-open.org/wsrf/bf-2}FaultCause) inherited from {http://docs.oasis-open.org/wsrf/bf-2}BaseFaultType
    _AttributeWildcard = pyxb.binding.content.Wildcard(process_contents=pyxb.binding.content.Wildcard.PC_lax, namespace_constraint=(pyxb.binding.content.Wildcard.NC_not, 'http://docs.oasis-open.org/wsrf/bf-2'))
    _HasWildcardElement = True
    _ElementMap.update({
        
    })
    _AttributeMap.update({
        
    })
Namespace.addCategoryObject('typeBinding', 'PublisherRegistrationFailedFaultType', PublisherRegistrationFailedFaultType)


# Complex type [anonymous] with content type ELEMENT_ONLY
class CTD_ANON_4 (pyxb.binding.basis.complexTypeDefinition):
    """Complex type [anonymous] with content type ELEMENT_ONLY"""
    _TypeDefinition = None
    _ContentTypeTag = pyxb.binding.basis.complexTypeDefinition._CT_ELEMENT_ONLY
    _Abstract = False
    _ExpandedName = None
    _XSDLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wsrf_br.xsd', 146, 4)
    _ElementMap = {}
    _AttributeMap = {}
    # Base type is pyxb.binding.datatypes.anyType
    _AttributeWildcard = pyxb.binding.content.Wildcard(process_contents=pyxb.binding.content.Wildcard.PC_strict, namespace_constraint=pyxb.binding.content.Wildcard.NC_any)
    _HasWildcardElement = True
    _ElementMap.update({
        
    })
    _AttributeMap.update({
        
    })



# Complex type [anonymous] with content type ELEMENT_ONLY
class CTD_ANON_5 (pyxb.binding.basis.complexTypeDefinition):
    """Complex type [anonymous] with content type ELEMENT_ONLY"""
    _TypeDefinition = None
    _ContentTypeTag = pyxb.binding.basis.complexTypeDefinition._CT_ELEMENT_ONLY
    _Abstract = False
    _ExpandedName = None
    _XSDLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wsrf_br.xsd', 156, 4)
    _ElementMap = {}
    _AttributeMap = {}
    # Base type is pyxb.binding.datatypes.anyType
    _AttributeWildcard = pyxb.binding.content.Wildcard(process_contents=pyxb.binding.content.Wildcard.PC_strict, namespace_constraint=pyxb.binding.content.Wildcard.NC_any)
    _HasWildcardElement = True
    _ElementMap.update({
        
    })
    _AttributeMap.update({
        
    })



# Complex type {http://docs.oasis-open.org/wsn/br-2}ResourceNotDestroyedFaultType with content type ELEMENT_ONLY
class ResourceNotDestroyedFaultType (pyxb.bundles.wssplat.wsrf_bf.BaseFaultType):
    """Complex type {http://docs.oasis-open.org/wsn/br-2}ResourceNotDestroyedFaultType with content type ELEMENT_ONLY"""
    _TypeDefinition = None
    _ContentTypeTag = pyxb.binding.basis.complexTypeDefinition._CT_ELEMENT_ONLY
    _Abstract = False
    _ExpandedName = pyxb.namespace.ExpandedName(Namespace, 'ResourceNotDestroyedFaultType')
    _XSDLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wsrf_br.xsd', 165, 2)
    _ElementMap = pyxb.bundles.wssplat.wsrf_bf.BaseFaultType._ElementMap.copy()
    _AttributeMap = pyxb.bundles.wssplat.wsrf_bf.BaseFaultType._AttributeMap.copy()
    # Base type is pyxb.bundles.wssplat.wsrf_bf.BaseFaultType
    
    # Element Timestamp ({http://docs.oasis-open.org/wsrf/bf-2}Timestamp) inherited from {http://docs.oasis-open.org/wsrf/bf-2}BaseFaultType
    
    # Element Originator ({http://docs.oasis-open.org/wsrf/bf-2}Originator) inherited from {http://docs.oasis-open.org/wsrf/bf-2}BaseFaultType
    
    # Element ErrorCode ({http://docs.oasis-open.org/wsrf/bf-2}ErrorCode) inherited from {http://docs.oasis-open.org/wsrf/bf-2}BaseFaultType
    
    # Element Description ({http://docs.oasis-open.org/wsrf/bf-2}Description) inherited from {http://docs.oasis-open.org/wsrf/bf-2}BaseFaultType
    
    # Element FaultCause ({http://docs.oasis-open.org/wsrf/bf-2}FaultCause) inherited from {http://docs.oasis-open.org/wsrf/bf-2}BaseFaultType
    _AttributeWildcard = pyxb.binding.content.Wildcard(process_contents=pyxb.binding.content.Wildcard.PC_lax, namespace_constraint=(pyxb.binding.content.Wildcard.NC_not, 'http://docs.oasis-open.org/wsrf/bf-2'))
    _HasWildcardElement = True
    _ElementMap.update({
        
    })
    _AttributeMap.update({
        
    })
Namespace.addCategoryObject('typeBinding', 'ResourceNotDestroyedFaultType', ResourceNotDestroyedFaultType)


RequiresRegistration = pyxb.binding.basis.element(pyxb.namespace.ExpandedName(Namespace, 'RequiresRegistration'), pyxb.binding.datatypes.boolean, location=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wsrf_br.xsd', 43, 3))
Namespace.addCategoryObject('elementBinding', RequiresRegistration.name().localName(), RequiresRegistration)

PublisherReference = pyxb.binding.basis.element(pyxb.namespace.ExpandedName(Namespace, 'PublisherReference'), pyxb.bundles.wssplat.wsa.EndpointReferenceType, location=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wsrf_br.xsd', 46, 3))
Namespace.addCategoryObject('elementBinding', PublisherReference.name().localName(), PublisherReference)

ConsumerReference = pyxb.binding.basis.element(pyxb.namespace.ExpandedName(Namespace, 'ConsumerReference'), pyxb.bundles.wssplat.wsa.EndpointReferenceType, location=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wsrf_br.xsd', 48, 3))
Namespace.addCategoryObject('elementBinding', ConsumerReference.name().localName(), ConsumerReference)

Topic = pyxb.binding.basis.element(pyxb.namespace.ExpandedName(Namespace, 'Topic'), pyxb.bundles.wssplat.wsnt.TopicExpressionType, location=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wsrf_br.xsd', 50, 3))
Namespace.addCategoryObject('elementBinding', Topic.name().localName(), Topic)

Demand = pyxb.binding.basis.element(pyxb.namespace.ExpandedName(Namespace, 'Demand'), pyxb.binding.datatypes.boolean, location=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wsrf_br.xsd', 52, 3))
Namespace.addCategoryObject('elementBinding', Demand.name().localName(), Demand)

CreationTime = pyxb.binding.basis.element(pyxb.namespace.ExpandedName(Namespace, 'CreationTime'), pyxb.binding.datatypes.dateTime, location=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wsrf_br.xsd', 54, 3))
Namespace.addCategoryObject('elementBinding', CreationTime.name().localName(), CreationTime)

NotificationBrokerRP = pyxb.binding.basis.element(pyxb.namespace.ExpandedName(Namespace, 'NotificationBrokerRP'), CTD_ANON, location=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wsrf_br.xsd', 56, 3))
Namespace.addCategoryObject('elementBinding', NotificationBrokerRP.name().localName(), NotificationBrokerRP)

PublisherRegistrationRP = pyxb.binding.basis.element(pyxb.namespace.ExpandedName(Namespace, 'PublisherRegistrationRP'), CTD_ANON_, location=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wsrf_br.xsd', 76, 3))
Namespace.addCategoryObject('elementBinding', PublisherRegistrationRP.name().localName(), PublisherRegistrationRP)

RegisterPublisher = pyxb.binding.basis.element(pyxb.namespace.ExpandedName(Namespace, 'RegisterPublisher'), CTD_ANON_2, location=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wsrf_br.xsd', 92, 3))
Namespace.addCategoryObject('elementBinding', RegisterPublisher.name().localName(), RegisterPublisher)

RegisterPublisherResponse = pyxb.binding.basis.element(pyxb.namespace.ExpandedName(Namespace, 'RegisterPublisherResponse'), CTD_ANON_3, location=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wsrf_br.xsd', 113, 3))
Namespace.addCategoryObject('elementBinding', RegisterPublisherResponse.name().localName(), RegisterPublisherResponse)

PublisherRegistrationRejectedFault = pyxb.binding.basis.element(pyxb.namespace.ExpandedName(Namespace, 'PublisherRegistrationRejectedFault'), PublisherRegistrationRejectedFaultType, location=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wsrf_br.xsd', 132, 3))
Namespace.addCategoryObject('elementBinding', PublisherRegistrationRejectedFault.name().localName(), PublisherRegistrationRejectedFault)

PublisherRegistrationFailedFault = pyxb.binding.basis.element(pyxb.namespace.ExpandedName(Namespace, 'PublisherRegistrationFailedFault'), PublisherRegistrationFailedFaultType, location=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wsrf_br.xsd', 140, 3))
Namespace.addCategoryObject('elementBinding', PublisherRegistrationFailedFault.name().localName(), PublisherRegistrationFailedFault)

DestroyRegistration = pyxb.binding.basis.element(pyxb.namespace.ExpandedName(Namespace, 'DestroyRegistration'), CTD_ANON_4, location=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wsrf_br.xsd', 145, 3))
Namespace.addCategoryObject('elementBinding', DestroyRegistration.name().localName(), DestroyRegistration)

DestroyRegistrationResponse = pyxb.binding.basis.element(pyxb.namespace.ExpandedName(Namespace, 'DestroyRegistrationResponse'), CTD_ANON_5, location=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wsrf_br.xsd', 155, 2))
Namespace.addCategoryObject('elementBinding', DestroyRegistrationResponse.name().localName(), DestroyRegistrationResponse)

ResourceNotDestroyedFault = pyxb.binding.basis.element(pyxb.namespace.ExpandedName(Namespace, 'ResourceNotDestroyedFault'), ResourceNotDestroyedFaultType, location=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wsrf_br.xsd', 170, 2))
Namespace.addCategoryObject('elementBinding', ResourceNotDestroyedFault.name().localName(), ResourceNotDestroyedFault)



CTD_ANON._AddElement(pyxb.binding.basis.element(pyxb.namespace.ExpandedName(_Namespace_wsn_b, 'TopicExpression'), pyxb.bundles.wssplat.wsnt.TopicExpressionType, scope=CTD_ANON, location=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wsnt.xsd', 71, 2)))

CTD_ANON._AddElement(pyxb.binding.basis.element(pyxb.namespace.ExpandedName(_Namespace_wsn_b, 'FixedTopicSet'), pyxb.binding.datatypes.boolean, scope=CTD_ANON, location=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wsnt.xsd', 72, 2), unicode_default='true'))

CTD_ANON._AddElement(pyxb.binding.basis.element(pyxb.namespace.ExpandedName(_Namespace_wsn_b, 'TopicExpressionDialect'), pyxb.binding.datatypes.anyURI, scope=CTD_ANON, location=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wsnt.xsd', 73, 2)))

CTD_ANON._AddElement(pyxb.binding.basis.element(pyxb.namespace.ExpandedName(Namespace, 'RequiresRegistration'), pyxb.binding.datatypes.boolean, scope=CTD_ANON, location=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wsrf_br.xsd', 43, 3)))

CTD_ANON._AddElement(pyxb.binding.basis.element(pyxb.namespace.ExpandedName(_Namespace_wstop, 'TopicSet'), pyxb.bundles.wssplat.wstop.TopicSetType, scope=CTD_ANON, location=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wstop.xsd', 133, 2)))

def _BuildAutomaton ():
    # Remove this helper function from the namespace after it is invoked
    global _BuildAutomaton
    del _BuildAutomaton
    import pyxb.utils.fac as fac

    counters = set()
    cc_0 = fac.CounterCondition(min=0, max=None, metadata=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wsrf_br.xsd', 60, 18))
    counters.add(cc_0)
    cc_1 = fac.CounterCondition(min=0, max=1, metadata=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wsrf_br.xsd', 62, 12))
    counters.add(cc_1)
    cc_2 = fac.CounterCondition(min=0, max=None, metadata=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wsrf_br.xsd', 64, 12))
    counters.add(cc_2)
    cc_3 = fac.CounterCondition(min=0, max=1, metadata=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wsrf_br.xsd', 66, 18))
    counters.add(cc_3)
    states = []
    final_update = None
    symbol = pyxb.binding.content.ElementUse(CTD_ANON._UseForTag(pyxb.namespace.ExpandedName(_Namespace_wsn_b, 'TopicExpression')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wsrf_br.xsd', 60, 18))
    st_0 = fac.State(symbol, is_initial=True, final_update=final_update, is_unordered_catenation=False)
    states.append(st_0)
    final_update = None
    symbol = pyxb.binding.content.ElementUse(CTD_ANON._UseForTag(pyxb.namespace.ExpandedName(_Namespace_wsn_b, 'FixedTopicSet')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wsrf_br.xsd', 62, 12))
    st_1 = fac.State(symbol, is_initial=True, final_update=final_update, is_unordered_catenation=False)
    states.append(st_1)
    final_update = None
    symbol = pyxb.binding.content.ElementUse(CTD_ANON._UseForTag(pyxb.namespace.ExpandedName(_Namespace_wsn_b, 'TopicExpressionDialect')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wsrf_br.xsd', 64, 12))
    st_2 = fac.State(symbol, is_initial=True, final_update=final_update, is_unordered_catenation=False)
    states.append(st_2)
    final_update = None
    symbol = pyxb.binding.content.ElementUse(CTD_ANON._UseForTag(pyxb.namespace.ExpandedName(_Namespace_wstop, 'TopicSet')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wsrf_br.xsd', 66, 18))
    st_3 = fac.State(symbol, is_initial=True, final_update=final_update, is_unordered_catenation=False)
    states.append(st_3)
    final_update = set()
    symbol = pyxb.binding.content.ElementUse(CTD_ANON._UseForTag(pyxb.namespace.ExpandedName(Namespace, 'RequiresRegistration')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wsrf_br.xsd', 69, 18))
    st_4 = fac.State(symbol, is_initial=True, final_update=final_update, is_unordered_catenation=False)
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
    transitions.append(fac.Transition(st_4, [
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
    st_1._set_transitionSet(transitions)
    transitions = []
    transitions.append(fac.Transition(st_2, [
        fac.UpdateInstruction(cc_2, True) ]))
    transitions.append(fac.Transition(st_3, [
        fac.UpdateInstruction(cc_2, False) ]))
    transitions.append(fac.Transition(st_4, [
        fac.UpdateInstruction(cc_2, False) ]))
    st_2._set_transitionSet(transitions)
    transitions = []
    transitions.append(fac.Transition(st_3, [
        fac.UpdateInstruction(cc_3, True) ]))
    transitions.append(fac.Transition(st_4, [
        fac.UpdateInstruction(cc_3, False) ]))
    st_3._set_transitionSet(transitions)
    transitions = []
    st_4._set_transitionSet(transitions)
    return fac.Automaton(states, counters, False, containing_state=None)
CTD_ANON._Automaton = _BuildAutomaton()




CTD_ANON_._AddElement(pyxb.binding.basis.element(pyxb.namespace.ExpandedName(Namespace, 'PublisherReference'), pyxb.bundles.wssplat.wsa.EndpointReferenceType, scope=CTD_ANON_, location=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wsrf_br.xsd', 46, 3)))

CTD_ANON_._AddElement(pyxb.binding.basis.element(pyxb.namespace.ExpandedName(Namespace, 'Topic'), pyxb.bundles.wssplat.wsnt.TopicExpressionType, scope=CTD_ANON_, location=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wsrf_br.xsd', 50, 3)))

CTD_ANON_._AddElement(pyxb.binding.basis.element(pyxb.namespace.ExpandedName(Namespace, 'Demand'), pyxb.binding.datatypes.boolean, scope=CTD_ANON_, location=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wsrf_br.xsd', 52, 3)))

CTD_ANON_._AddElement(pyxb.binding.basis.element(pyxb.namespace.ExpandedName(Namespace, 'CreationTime'), pyxb.binding.datatypes.dateTime, scope=CTD_ANON_, location=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wsrf_br.xsd', 54, 3)))

def _BuildAutomaton_ ():
    # Remove this helper function from the namespace after it is invoked
    global _BuildAutomaton_
    del _BuildAutomaton_
    import pyxb.utils.fac as fac

    counters = set()
    cc_0 = fac.CounterCondition(min=0, max=1, metadata=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wsrf_br.xsd', 79, 18))
    counters.add(cc_0)
    cc_1 = fac.CounterCondition(min=0, max=None, metadata=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wsrf_br.xsd', 81, 18))
    counters.add(cc_1)
    cc_2 = fac.CounterCondition(min=0, max=1, metadata=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wsrf_br.xsd', 85, 18))
    counters.add(cc_2)
    states = []
    final_update = None
    symbol = pyxb.binding.content.ElementUse(CTD_ANON_._UseForTag(pyxb.namespace.ExpandedName(Namespace, 'PublisherReference')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wsrf_br.xsd', 79, 18))
    st_0 = fac.State(symbol, is_initial=True, final_update=final_update, is_unordered_catenation=False)
    states.append(st_0)
    final_update = None
    symbol = pyxb.binding.content.ElementUse(CTD_ANON_._UseForTag(pyxb.namespace.ExpandedName(Namespace, 'Topic')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wsrf_br.xsd', 81, 18))
    st_1 = fac.State(symbol, is_initial=True, final_update=final_update, is_unordered_catenation=False)
    states.append(st_1)
    final_update = set()
    symbol = pyxb.binding.content.ElementUse(CTD_ANON_._UseForTag(pyxb.namespace.ExpandedName(Namespace, 'Demand')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wsrf_br.xsd', 83, 18))
    st_2 = fac.State(symbol, is_initial=True, final_update=final_update, is_unordered_catenation=False)
    states.append(st_2)
    final_update = set()
    final_update.add(fac.UpdateInstruction(cc_2, False))
    symbol = pyxb.binding.content.ElementUse(CTD_ANON_._UseForTag(pyxb.namespace.ExpandedName(Namespace, 'CreationTime')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wsrf_br.xsd', 85, 18))
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
CTD_ANON_._Automaton = _BuildAutomaton_()




CTD_ANON_2._AddElement(pyxb.binding.basis.element(pyxb.namespace.ExpandedName(Namespace, 'PublisherReference'), pyxb.bundles.wssplat.wsa.EndpointReferenceType, scope=CTD_ANON_2, location=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wsrf_br.xsd', 95, 18)))

CTD_ANON_2._AddElement(pyxb.binding.basis.element(pyxb.namespace.ExpandedName(Namespace, 'Topic'), pyxb.bundles.wssplat.wsnt.TopicExpressionType, scope=CTD_ANON_2, location=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wsrf_br.xsd', 98, 18)))

CTD_ANON_2._AddElement(pyxb.binding.basis.element(pyxb.namespace.ExpandedName(Namespace, 'Demand'), pyxb.binding.datatypes.boolean, scope=CTD_ANON_2, location=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wsrf_br.xsd', 101, 18), unicode_default='false'))

CTD_ANON_2._AddElement(pyxb.binding.basis.element(pyxb.namespace.ExpandedName(Namespace, 'InitialTerminationTime'), pyxb.binding.datatypes.dateTime, scope=CTD_ANON_2, location=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wsrf_br.xsd', 104, 18)))

def _BuildAutomaton_2 ():
    # Remove this helper function from the namespace after it is invoked
    global _BuildAutomaton_2
    del _BuildAutomaton_2
    import pyxb.utils.fac as fac

    counters = set()
    cc_0 = fac.CounterCondition(min=0, max=1, metadata=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wsrf_br.xsd', 95, 18))
    counters.add(cc_0)
    cc_1 = fac.CounterCondition(min=0, max=None, metadata=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wsrf_br.xsd', 98, 18))
    counters.add(cc_1)
    cc_2 = fac.CounterCondition(min=0, max=1, metadata=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wsrf_br.xsd', 101, 18))
    counters.add(cc_2)
    cc_3 = fac.CounterCondition(min=0, max=1, metadata=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wsrf_br.xsd', 104, 18))
    counters.add(cc_3)
    cc_4 = fac.CounterCondition(min=0, max=None, metadata=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wsrf_br.xsd', 107, 18))
    counters.add(cc_4)
    states = []
    final_update = set()
    final_update.add(fac.UpdateInstruction(cc_0, False))
    symbol = pyxb.binding.content.ElementUse(CTD_ANON_2._UseForTag(pyxb.namespace.ExpandedName(Namespace, 'PublisherReference')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wsrf_br.xsd', 95, 18))
    st_0 = fac.State(symbol, is_initial=True, final_update=final_update, is_unordered_catenation=False)
    states.append(st_0)
    final_update = set()
    final_update.add(fac.UpdateInstruction(cc_1, False))
    symbol = pyxb.binding.content.ElementUse(CTD_ANON_2._UseForTag(pyxb.namespace.ExpandedName(Namespace, 'Topic')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wsrf_br.xsd', 98, 18))
    st_1 = fac.State(symbol, is_initial=True, final_update=final_update, is_unordered_catenation=False)
    states.append(st_1)
    final_update = set()
    final_update.add(fac.UpdateInstruction(cc_2, False))
    symbol = pyxb.binding.content.ElementUse(CTD_ANON_2._UseForTag(pyxb.namespace.ExpandedName(Namespace, 'Demand')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wsrf_br.xsd', 101, 18))
    st_2 = fac.State(symbol, is_initial=True, final_update=final_update, is_unordered_catenation=False)
    states.append(st_2)
    final_update = set()
    final_update.add(fac.UpdateInstruction(cc_3, False))
    symbol = pyxb.binding.content.ElementUse(CTD_ANON_2._UseForTag(pyxb.namespace.ExpandedName(Namespace, 'InitialTerminationTime')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wsrf_br.xsd', 104, 18))
    st_3 = fac.State(symbol, is_initial=True, final_update=final_update, is_unordered_catenation=False)
    states.append(st_3)
    final_update = set()
    final_update.add(fac.UpdateInstruction(cc_4, False))
    symbol = pyxb.binding.content.WildcardUse(pyxb.binding.content.Wildcard(process_contents=pyxb.binding.content.Wildcard.PC_lax, namespace_constraint=(pyxb.binding.content.Wildcard.NC_not, 'http://docs.oasis-open.org/wsn/br-2')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wsrf_br.xsd', 107, 18))
    st_4 = fac.State(symbol, is_initial=True, final_update=final_update, is_unordered_catenation=False)
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
    transitions.append(fac.Transition(st_4, [
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
    st_1._set_transitionSet(transitions)
    transitions = []
    transitions.append(fac.Transition(st_2, [
        fac.UpdateInstruction(cc_2, True) ]))
    transitions.append(fac.Transition(st_3, [
        fac.UpdateInstruction(cc_2, False) ]))
    transitions.append(fac.Transition(st_4, [
        fac.UpdateInstruction(cc_2, False) ]))
    st_2._set_transitionSet(transitions)
    transitions = []
    transitions.append(fac.Transition(st_3, [
        fac.UpdateInstruction(cc_3, True) ]))
    transitions.append(fac.Transition(st_4, [
        fac.UpdateInstruction(cc_3, False) ]))
    st_3._set_transitionSet(transitions)
    transitions = []
    transitions.append(fac.Transition(st_4, [
        fac.UpdateInstruction(cc_4, True) ]))
    st_4._set_transitionSet(transitions)
    return fac.Automaton(states, counters, True, containing_state=None)
CTD_ANON_2._Automaton = _BuildAutomaton_2()




CTD_ANON_3._AddElement(pyxb.binding.basis.element(pyxb.namespace.ExpandedName(Namespace, 'PublisherRegistrationReference'), pyxb.bundles.wssplat.wsa.EndpointReferenceType, scope=CTD_ANON_3, location=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wsrf_br.xsd', 116, 18)))

CTD_ANON_3._AddElement(pyxb.binding.basis.element(pyxb.namespace.ExpandedName(Namespace, 'ConsumerReference'), pyxb.bundles.wssplat.wsa.EndpointReferenceType, scope=CTD_ANON_3, location=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wsrf_br.xsd', 119, 5)))

def _BuildAutomaton_3 ():
    # Remove this helper function from the namespace after it is invoked
    global _BuildAutomaton_3
    del _BuildAutomaton_3
    import pyxb.utils.fac as fac

    counters = set()
    cc_0 = fac.CounterCondition(min=0, max=1, metadata=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wsrf_br.xsd', 119, 5))
    counters.add(cc_0)
    states = []
    final_update = set()
    symbol = pyxb.binding.content.ElementUse(CTD_ANON_3._UseForTag(pyxb.namespace.ExpandedName(Namespace, 'PublisherRegistrationReference')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wsrf_br.xsd', 116, 18))
    st_0 = fac.State(symbol, is_initial=True, final_update=final_update, is_unordered_catenation=False)
    states.append(st_0)
    final_update = set()
    final_update.add(fac.UpdateInstruction(cc_0, False))
    symbol = pyxb.binding.content.ElementUse(CTD_ANON_3._UseForTag(pyxb.namespace.ExpandedName(Namespace, 'ConsumerReference')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wsrf_br.xsd', 119, 5))
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
CTD_ANON_3._Automaton = _BuildAutomaton_3()




def _BuildAutomaton_4 ():
    # Remove this helper function from the namespace after it is invoked
    global _BuildAutomaton_4
    del _BuildAutomaton_4
    import pyxb.utils.fac as fac

    counters = set()
    cc_0 = fac.CounterCondition(min=0, max=None, metadata=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wsrf_bf.xsd', 46, 6))
    counters.add(cc_0)
    cc_1 = fac.CounterCondition(min=0, max=1, metadata=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wsrf_bf.xsd', 50, 6))
    counters.add(cc_1)
    cc_2 = fac.CounterCondition(min=0, max=1, metadata=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wsrf_bf.xsd', 52, 6))
    counters.add(cc_2)
    cc_3 = fac.CounterCondition(min=0, max=None, metadata=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wsrf_bf.xsd', 64, 6))
    counters.add(cc_3)
    cc_4 = fac.CounterCondition(min=0, max=1, metadata=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wsrf_bf.xsd', 74, 6))
    counters.add(cc_4)
    states = []
    final_update = None
    symbol = pyxb.binding.content.WildcardUse(pyxb.binding.content.Wildcard(process_contents=pyxb.binding.content.Wildcard.PC_lax, namespace_constraint=(pyxb.binding.content.Wildcard.NC_not, 'http://docs.oasis-open.org/wsrf/bf-2')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wsrf_bf.xsd', 46, 6))
    st_0 = fac.State(symbol, is_initial=True, final_update=final_update, is_unordered_catenation=False)
    states.append(st_0)
    final_update = set()
    symbol = pyxb.binding.content.ElementUse(PublisherRegistrationRejectedFaultType._UseForTag(pyxb.namespace.ExpandedName(_Namespace_wsrf_bf, 'Timestamp')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wsrf_bf.xsd', 48, 6))
    st_1 = fac.State(symbol, is_initial=True, final_update=final_update, is_unordered_catenation=False)
    states.append(st_1)
    final_update = set()
    final_update.add(fac.UpdateInstruction(cc_1, False))
    symbol = pyxb.binding.content.ElementUse(PublisherRegistrationRejectedFaultType._UseForTag(pyxb.namespace.ExpandedName(_Namespace_wsrf_bf, 'Originator')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wsrf_bf.xsd', 50, 6))
    st_2 = fac.State(symbol, is_initial=False, final_update=final_update, is_unordered_catenation=False)
    states.append(st_2)
    final_update = set()
    final_update.add(fac.UpdateInstruction(cc_2, False))
    symbol = pyxb.binding.content.ElementUse(PublisherRegistrationRejectedFaultType._UseForTag(pyxb.namespace.ExpandedName(_Namespace_wsrf_bf, 'ErrorCode')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wsrf_bf.xsd', 52, 6))
    st_3 = fac.State(symbol, is_initial=False, final_update=final_update, is_unordered_catenation=False)
    states.append(st_3)
    final_update = set()
    final_update.add(fac.UpdateInstruction(cc_3, False))
    symbol = pyxb.binding.content.ElementUse(PublisherRegistrationRejectedFaultType._UseForTag(pyxb.namespace.ExpandedName(_Namespace_wsrf_bf, 'Description')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wsrf_bf.xsd', 64, 6))
    st_4 = fac.State(symbol, is_initial=False, final_update=final_update, is_unordered_catenation=False)
    states.append(st_4)
    final_update = set()
    final_update.add(fac.UpdateInstruction(cc_4, False))
    symbol = pyxb.binding.content.ElementUse(PublisherRegistrationRejectedFaultType._UseForTag(pyxb.namespace.ExpandedName(_Namespace_wsrf_bf, 'FaultCause')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wsrf_bf.xsd', 74, 6))
    st_5 = fac.State(symbol, is_initial=False, final_update=final_update, is_unordered_catenation=False)
    states.append(st_5)
    transitions = []
    transitions.append(fac.Transition(st_0, [
        fac.UpdateInstruction(cc_0, True) ]))
    transitions.append(fac.Transition(st_1, [
        fac.UpdateInstruction(cc_0, False) ]))
    st_0._set_transitionSet(transitions)
    transitions = []
    transitions.append(fac.Transition(st_2, [
         ]))
    transitions.append(fac.Transition(st_3, [
         ]))
    transitions.append(fac.Transition(st_4, [
         ]))
    transitions.append(fac.Transition(st_5, [
         ]))
    st_1._set_transitionSet(transitions)
    transitions = []
    transitions.append(fac.Transition(st_2, [
        fac.UpdateInstruction(cc_1, True) ]))
    transitions.append(fac.Transition(st_3, [
        fac.UpdateInstruction(cc_1, False) ]))
    transitions.append(fac.Transition(st_4, [
        fac.UpdateInstruction(cc_1, False) ]))
    transitions.append(fac.Transition(st_5, [
        fac.UpdateInstruction(cc_1, False) ]))
    st_2._set_transitionSet(transitions)
    transitions = []
    transitions.append(fac.Transition(st_3, [
        fac.UpdateInstruction(cc_2, True) ]))
    transitions.append(fac.Transition(st_4, [
        fac.UpdateInstruction(cc_2, False) ]))
    transitions.append(fac.Transition(st_5, [
        fac.UpdateInstruction(cc_2, False) ]))
    st_3._set_transitionSet(transitions)
    transitions = []
    transitions.append(fac.Transition(st_4, [
        fac.UpdateInstruction(cc_3, True) ]))
    transitions.append(fac.Transition(st_5, [
        fac.UpdateInstruction(cc_3, False) ]))
    st_4._set_transitionSet(transitions)
    transitions = []
    transitions.append(fac.Transition(st_5, [
        fac.UpdateInstruction(cc_4, True) ]))
    st_5._set_transitionSet(transitions)
    return fac.Automaton(states, counters, False, containing_state=None)
PublisherRegistrationRejectedFaultType._Automaton = _BuildAutomaton_4()




def _BuildAutomaton_5 ():
    # Remove this helper function from the namespace after it is invoked
    global _BuildAutomaton_5
    del _BuildAutomaton_5
    import pyxb.utils.fac as fac

    counters = set()
    cc_0 = fac.CounterCondition(min=0, max=None, metadata=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wsrf_bf.xsd', 46, 6))
    counters.add(cc_0)
    cc_1 = fac.CounterCondition(min=0, max=1, metadata=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wsrf_bf.xsd', 50, 6))
    counters.add(cc_1)
    cc_2 = fac.CounterCondition(min=0, max=1, metadata=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wsrf_bf.xsd', 52, 6))
    counters.add(cc_2)
    cc_3 = fac.CounterCondition(min=0, max=None, metadata=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wsrf_bf.xsd', 64, 6))
    counters.add(cc_3)
    cc_4 = fac.CounterCondition(min=0, max=1, metadata=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wsrf_bf.xsd', 74, 6))
    counters.add(cc_4)
    states = []
    final_update = None
    symbol = pyxb.binding.content.WildcardUse(pyxb.binding.content.Wildcard(process_contents=pyxb.binding.content.Wildcard.PC_lax, namespace_constraint=(pyxb.binding.content.Wildcard.NC_not, 'http://docs.oasis-open.org/wsrf/bf-2')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wsrf_bf.xsd', 46, 6))
    st_0 = fac.State(symbol, is_initial=True, final_update=final_update, is_unordered_catenation=False)
    states.append(st_0)
    final_update = set()
    symbol = pyxb.binding.content.ElementUse(PublisherRegistrationFailedFaultType._UseForTag(pyxb.namespace.ExpandedName(_Namespace_wsrf_bf, 'Timestamp')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wsrf_bf.xsd', 48, 6))
    st_1 = fac.State(symbol, is_initial=True, final_update=final_update, is_unordered_catenation=False)
    states.append(st_1)
    final_update = set()
    final_update.add(fac.UpdateInstruction(cc_1, False))
    symbol = pyxb.binding.content.ElementUse(PublisherRegistrationFailedFaultType._UseForTag(pyxb.namespace.ExpandedName(_Namespace_wsrf_bf, 'Originator')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wsrf_bf.xsd', 50, 6))
    st_2 = fac.State(symbol, is_initial=False, final_update=final_update, is_unordered_catenation=False)
    states.append(st_2)
    final_update = set()
    final_update.add(fac.UpdateInstruction(cc_2, False))
    symbol = pyxb.binding.content.ElementUse(PublisherRegistrationFailedFaultType._UseForTag(pyxb.namespace.ExpandedName(_Namespace_wsrf_bf, 'ErrorCode')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wsrf_bf.xsd', 52, 6))
    st_3 = fac.State(symbol, is_initial=False, final_update=final_update, is_unordered_catenation=False)
    states.append(st_3)
    final_update = set()
    final_update.add(fac.UpdateInstruction(cc_3, False))
    symbol = pyxb.binding.content.ElementUse(PublisherRegistrationFailedFaultType._UseForTag(pyxb.namespace.ExpandedName(_Namespace_wsrf_bf, 'Description')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wsrf_bf.xsd', 64, 6))
    st_4 = fac.State(symbol, is_initial=False, final_update=final_update, is_unordered_catenation=False)
    states.append(st_4)
    final_update = set()
    final_update.add(fac.UpdateInstruction(cc_4, False))
    symbol = pyxb.binding.content.ElementUse(PublisherRegistrationFailedFaultType._UseForTag(pyxb.namespace.ExpandedName(_Namespace_wsrf_bf, 'FaultCause')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wsrf_bf.xsd', 74, 6))
    st_5 = fac.State(symbol, is_initial=False, final_update=final_update, is_unordered_catenation=False)
    states.append(st_5)
    transitions = []
    transitions.append(fac.Transition(st_0, [
        fac.UpdateInstruction(cc_0, True) ]))
    transitions.append(fac.Transition(st_1, [
        fac.UpdateInstruction(cc_0, False) ]))
    st_0._set_transitionSet(transitions)
    transitions = []
    transitions.append(fac.Transition(st_2, [
         ]))
    transitions.append(fac.Transition(st_3, [
         ]))
    transitions.append(fac.Transition(st_4, [
         ]))
    transitions.append(fac.Transition(st_5, [
         ]))
    st_1._set_transitionSet(transitions)
    transitions = []
    transitions.append(fac.Transition(st_2, [
        fac.UpdateInstruction(cc_1, True) ]))
    transitions.append(fac.Transition(st_3, [
        fac.UpdateInstruction(cc_1, False) ]))
    transitions.append(fac.Transition(st_4, [
        fac.UpdateInstruction(cc_1, False) ]))
    transitions.append(fac.Transition(st_5, [
        fac.UpdateInstruction(cc_1, False) ]))
    st_2._set_transitionSet(transitions)
    transitions = []
    transitions.append(fac.Transition(st_3, [
        fac.UpdateInstruction(cc_2, True) ]))
    transitions.append(fac.Transition(st_4, [
        fac.UpdateInstruction(cc_2, False) ]))
    transitions.append(fac.Transition(st_5, [
        fac.UpdateInstruction(cc_2, False) ]))
    st_3._set_transitionSet(transitions)
    transitions = []
    transitions.append(fac.Transition(st_4, [
        fac.UpdateInstruction(cc_3, True) ]))
    transitions.append(fac.Transition(st_5, [
        fac.UpdateInstruction(cc_3, False) ]))
    st_4._set_transitionSet(transitions)
    transitions = []
    transitions.append(fac.Transition(st_5, [
        fac.UpdateInstruction(cc_4, True) ]))
    st_5._set_transitionSet(transitions)
    return fac.Automaton(states, counters, False, containing_state=None)
PublisherRegistrationFailedFaultType._Automaton = _BuildAutomaton_5()




def _BuildAutomaton_6 ():
    # Remove this helper function from the namespace after it is invoked
    global _BuildAutomaton_6
    del _BuildAutomaton_6
    import pyxb.utils.fac as fac

    counters = set()
    cc_0 = fac.CounterCondition(min=0, max=None, metadata=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wsrf_br.xsd', 148, 8))
    counters.add(cc_0)
    states = []
    final_update = set()
    final_update.add(fac.UpdateInstruction(cc_0, False))
    symbol = pyxb.binding.content.WildcardUse(pyxb.binding.content.Wildcard(process_contents=pyxb.binding.content.Wildcard.PC_lax, namespace_constraint=(pyxb.binding.content.Wildcard.NC_not, 'http://docs.oasis-open.org/wsn/br-2')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wsrf_br.xsd', 148, 8))
    st_0 = fac.State(symbol, is_initial=True, final_update=final_update, is_unordered_catenation=False)
    states.append(st_0)
    transitions = []
    transitions.append(fac.Transition(st_0, [
        fac.UpdateInstruction(cc_0, True) ]))
    st_0._set_transitionSet(transitions)
    return fac.Automaton(states, counters, True, containing_state=None)
CTD_ANON_4._Automaton = _BuildAutomaton_6()




def _BuildAutomaton_7 ():
    # Remove this helper function from the namespace after it is invoked
    global _BuildAutomaton_7
    del _BuildAutomaton_7
    import pyxb.utils.fac as fac

    counters = set()
    cc_0 = fac.CounterCondition(min=0, max=None, metadata=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wsrf_br.xsd', 158, 8))
    counters.add(cc_0)
    states = []
    final_update = set()
    final_update.add(fac.UpdateInstruction(cc_0, False))
    symbol = pyxb.binding.content.WildcardUse(pyxb.binding.content.Wildcard(process_contents=pyxb.binding.content.Wildcard.PC_lax, namespace_constraint=(pyxb.binding.content.Wildcard.NC_not, 'http://docs.oasis-open.org/wsn/br-2')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wsrf_br.xsd', 158, 8))
    st_0 = fac.State(symbol, is_initial=True, final_update=final_update, is_unordered_catenation=False)
    states.append(st_0)
    transitions = []
    transitions.append(fac.Transition(st_0, [
        fac.UpdateInstruction(cc_0, True) ]))
    st_0._set_transitionSet(transitions)
    return fac.Automaton(states, counters, True, containing_state=None)
CTD_ANON_5._Automaton = _BuildAutomaton_7()




def _BuildAutomaton_8 ():
    # Remove this helper function from the namespace after it is invoked
    global _BuildAutomaton_8
    del _BuildAutomaton_8
    import pyxb.utils.fac as fac

    counters = set()
    cc_0 = fac.CounterCondition(min=0, max=None, metadata=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wsrf_bf.xsd', 46, 6))
    counters.add(cc_0)
    cc_1 = fac.CounterCondition(min=0, max=1, metadata=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wsrf_bf.xsd', 50, 6))
    counters.add(cc_1)
    cc_2 = fac.CounterCondition(min=0, max=1, metadata=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wsrf_bf.xsd', 52, 6))
    counters.add(cc_2)
    cc_3 = fac.CounterCondition(min=0, max=None, metadata=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wsrf_bf.xsd', 64, 6))
    counters.add(cc_3)
    cc_4 = fac.CounterCondition(min=0, max=1, metadata=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wsrf_bf.xsd', 74, 6))
    counters.add(cc_4)
    states = []
    final_update = None
    symbol = pyxb.binding.content.WildcardUse(pyxb.binding.content.Wildcard(process_contents=pyxb.binding.content.Wildcard.PC_lax, namespace_constraint=(pyxb.binding.content.Wildcard.NC_not, 'http://docs.oasis-open.org/wsrf/bf-2')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wsrf_bf.xsd', 46, 6))
    st_0 = fac.State(symbol, is_initial=True, final_update=final_update, is_unordered_catenation=False)
    states.append(st_0)
    final_update = set()
    symbol = pyxb.binding.content.ElementUse(ResourceNotDestroyedFaultType._UseForTag(pyxb.namespace.ExpandedName(_Namespace_wsrf_bf, 'Timestamp')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wsrf_bf.xsd', 48, 6))
    st_1 = fac.State(symbol, is_initial=True, final_update=final_update, is_unordered_catenation=False)
    states.append(st_1)
    final_update = set()
    final_update.add(fac.UpdateInstruction(cc_1, False))
    symbol = pyxb.binding.content.ElementUse(ResourceNotDestroyedFaultType._UseForTag(pyxb.namespace.ExpandedName(_Namespace_wsrf_bf, 'Originator')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wsrf_bf.xsd', 50, 6))
    st_2 = fac.State(symbol, is_initial=False, final_update=final_update, is_unordered_catenation=False)
    states.append(st_2)
    final_update = set()
    final_update.add(fac.UpdateInstruction(cc_2, False))
    symbol = pyxb.binding.content.ElementUse(ResourceNotDestroyedFaultType._UseForTag(pyxb.namespace.ExpandedName(_Namespace_wsrf_bf, 'ErrorCode')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wsrf_bf.xsd', 52, 6))
    st_3 = fac.State(symbol, is_initial=False, final_update=final_update, is_unordered_catenation=False)
    states.append(st_3)
    final_update = set()
    final_update.add(fac.UpdateInstruction(cc_3, False))
    symbol = pyxb.binding.content.ElementUse(ResourceNotDestroyedFaultType._UseForTag(pyxb.namespace.ExpandedName(_Namespace_wsrf_bf, 'Description')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wsrf_bf.xsd', 64, 6))
    st_4 = fac.State(symbol, is_initial=False, final_update=final_update, is_unordered_catenation=False)
    states.append(st_4)
    final_update = set()
    final_update.add(fac.UpdateInstruction(cc_4, False))
    symbol = pyxb.binding.content.ElementUse(ResourceNotDestroyedFaultType._UseForTag(pyxb.namespace.ExpandedName(_Namespace_wsrf_bf, 'FaultCause')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wsrf_bf.xsd', 74, 6))
    st_5 = fac.State(symbol, is_initial=False, final_update=final_update, is_unordered_catenation=False)
    states.append(st_5)
    transitions = []
    transitions.append(fac.Transition(st_0, [
        fac.UpdateInstruction(cc_0, True) ]))
    transitions.append(fac.Transition(st_1, [
        fac.UpdateInstruction(cc_0, False) ]))
    st_0._set_transitionSet(transitions)
    transitions = []
    transitions.append(fac.Transition(st_2, [
         ]))
    transitions.append(fac.Transition(st_3, [
         ]))
    transitions.append(fac.Transition(st_4, [
         ]))
    transitions.append(fac.Transition(st_5, [
         ]))
    st_1._set_transitionSet(transitions)
    transitions = []
    transitions.append(fac.Transition(st_2, [
        fac.UpdateInstruction(cc_1, True) ]))
    transitions.append(fac.Transition(st_3, [
        fac.UpdateInstruction(cc_1, False) ]))
    transitions.append(fac.Transition(st_4, [
        fac.UpdateInstruction(cc_1, False) ]))
    transitions.append(fac.Transition(st_5, [
        fac.UpdateInstruction(cc_1, False) ]))
    st_2._set_transitionSet(transitions)
    transitions = []
    transitions.append(fac.Transition(st_3, [
        fac.UpdateInstruction(cc_2, True) ]))
    transitions.append(fac.Transition(st_4, [
        fac.UpdateInstruction(cc_2, False) ]))
    transitions.append(fac.Transition(st_5, [
        fac.UpdateInstruction(cc_2, False) ]))
    st_3._set_transitionSet(transitions)
    transitions = []
    transitions.append(fac.Transition(st_4, [
        fac.UpdateInstruction(cc_3, True) ]))
    transitions.append(fac.Transition(st_5, [
        fac.UpdateInstruction(cc_3, False) ]))
    st_4._set_transitionSet(transitions)
    transitions = []
    transitions.append(fac.Transition(st_5, [
        fac.UpdateInstruction(cc_4, True) ]))
    st_5._set_transitionSet(transitions)
    return fac.Automaton(states, counters, False, containing_state=None)
ResourceNotDestroyedFaultType._Automaton = _BuildAutomaton_8()

