# ./pyxb/bundles/wssplat/raw/wstop.py
# -*- coding: utf-8 -*-
# PyXB bindings for NM:4e8dbbb71a72b67c5dc95d408f0b903c3981ba86
# Generated 2014-10-19 06:25:00.198861 by PyXB version 1.2.4 using Python 2.7.3.final.0
# Namespace http://docs.oasis-open.org/wsn/t-1

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
_GenerationUID = pyxb.utils.utility.UniqueIdentifier('urn:uuid:900382de-5782-11e4-9aa6-c8600024e903')

# Version of PyXB used to generate the bindings
_PyXBVersion = '1.2.4'
# Generated bindings are not compatible across PyXB versions
if pyxb.__version__ != _PyXBVersion:
    raise pyxb.PyXBVersionError(_PyXBVersion)

# Import bindings for namespaces imported into schema
import pyxb.binding.datatypes

# NOTE: All namespace declarations are reserved within the binding
Namespace = pyxb.namespace.NamespaceForURI('http://docs.oasis-open.org/wsn/t-1', create_if_missing=True)
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
    _XSDLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wstop.xsd', 109, 10)
    _Documentation = None

    _ItemType = pyxb.binding.datatypes.QName
STD_ANON._InitializeFacetMap()

# Atomic simple type: {http://docs.oasis-open.org/wsn/t-1}FullTopicExpression
class FullTopicExpression (pyxb.binding.datatypes.token):

    """An atomic simple type."""

    _ExpandedName = pyxb.namespace.ExpandedName(Namespace, 'FullTopicExpression')
    _XSDLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wstop.xsd', 138, 2)
    _Documentation = None
FullTopicExpression._CF_pattern = pyxb.binding.facets.CF_pattern()
FullTopicExpression._CF_pattern.addPattern(pattern='([\\i-[:]][\\c-[:]]*:)?(//)?([\\i-[:]][\\c-[:]]*|\\*)((/|//)(([\\i-[:]][\\c-[:]]*:)?[\\i-[:]][\\c-[:]]*|\\*|[.]))*(\\|([\\i-[:]][\\c-[:]]*:)?(//)?([\\i-[:]][\\c-[:]]*|\\*)((/|//)(([\\i-[:]][\\c-[:]]*:)?[\\i-[:]][\\c-[:]]*|\\*|[.]))*)*')
FullTopicExpression._InitializeFacetMap(FullTopicExpression._CF_pattern)
Namespace.addCategoryObject('typeBinding', 'FullTopicExpression', FullTopicExpression)

# Atomic simple type: {http://docs.oasis-open.org/wsn/t-1}ConcreteTopicExpression
class ConcreteTopicExpression (pyxb.binding.datatypes.token):

    """An atomic simple type."""

    _ExpandedName = pyxb.namespace.ExpandedName(Namespace, 'ConcreteTopicExpression')
    _XSDLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wstop.xsd', 156, 2)
    _Documentation = None
ConcreteTopicExpression._CF_pattern = pyxb.binding.facets.CF_pattern()
ConcreteTopicExpression._CF_pattern.addPattern(pattern='(([\\i-[:]][\\c-[:]]*:)?[\\i-[:]][\\c-[:]]*)(/([\\i-[:]][\\c-[:]]*:)?[\\i-[:]][\\c-[:]]*)*')
ConcreteTopicExpression._InitializeFacetMap(ConcreteTopicExpression._CF_pattern)
Namespace.addCategoryObject('typeBinding', 'ConcreteTopicExpression', ConcreteTopicExpression)

# Atomic simple type: {http://docs.oasis-open.org/wsn/t-1}SimpleTopicExpression
class SimpleTopicExpression (pyxb.binding.datatypes.QName):

    """An atomic simple type."""

    _ExpandedName = pyxb.namespace.ExpandedName(Namespace, 'SimpleTopicExpression')
    _XSDLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wstop.xsd', 173, 2)
    _Documentation = None
SimpleTopicExpression._InitializeFacetMap()
Namespace.addCategoryObject('typeBinding', 'SimpleTopicExpression', SimpleTopicExpression)

# Complex type {http://docs.oasis-open.org/wsn/t-1}Documentation with content type MIXED
class Documentation (pyxb.binding.basis.complexTypeDefinition):
    """Complex type {http://docs.oasis-open.org/wsn/t-1}Documentation with content type MIXED"""
    _TypeDefinition = None
    _ContentTypeTag = pyxb.binding.basis.complexTypeDefinition._CT_MIXED
    _Abstract = False
    _ExpandedName = pyxb.namespace.ExpandedName(Namespace, 'Documentation')
    _XSDLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wstop.xsd', 26, 2)
    _ElementMap = {}
    _AttributeMap = {}
    # Base type is pyxb.binding.datatypes.anyType
    _HasWildcardElement = True
    _ElementMap.update({
        
    })
    _AttributeMap.update({
        
    })
Namespace.addCategoryObject('typeBinding', 'Documentation', Documentation)


# Complex type {http://docs.oasis-open.org/wsn/t-1}ExtensibleDocumented with content type ELEMENT_ONLY
class ExtensibleDocumented (pyxb.binding.basis.complexTypeDefinition):
    """Complex type {http://docs.oasis-open.org/wsn/t-1}ExtensibleDocumented with content type ELEMENT_ONLY"""
    _TypeDefinition = None
    _ContentTypeTag = pyxb.binding.basis.complexTypeDefinition._CT_ELEMENT_ONLY
    _Abstract = True
    _ExpandedName = pyxb.namespace.ExpandedName(Namespace, 'ExtensibleDocumented')
    _XSDLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wstop.xsd', 33, 2)
    _ElementMap = {}
    _AttributeMap = {}
    # Base type is pyxb.binding.datatypes.anyType
    
    # Element {http://docs.oasis-open.org/wsn/t-1}documentation uses Python identifier documentation
    __documentation = pyxb.binding.content.ElementDeclaration(pyxb.namespace.ExpandedName(Namespace, 'documentation'), 'documentation', '__httpdocs_oasis_open_orgwsnt_1_ExtensibleDocumented_httpdocs_oasis_open_orgwsnt_1documentation', False, pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wstop.xsd', 36, 6), )

    
    documentation = property(__documentation.value, __documentation.set, None, None)

    _AttributeWildcard = pyxb.binding.content.Wildcard(process_contents=pyxb.binding.content.Wildcard.PC_lax, namespace_constraint=(pyxb.binding.content.Wildcard.NC_not, 'http://docs.oasis-open.org/wsn/t-1'))
    _ElementMap.update({
        __documentation.name() : __documentation
    })
    _AttributeMap.update({
        
    })
Namespace.addCategoryObject('typeBinding', 'ExtensibleDocumented', ExtensibleDocumented)


# Complex type {http://docs.oasis-open.org/wsn/t-1}QueryExpressionType with content type MIXED
class QueryExpressionType (pyxb.binding.basis.complexTypeDefinition):
    """Complex type {http://docs.oasis-open.org/wsn/t-1}QueryExpressionType with content type MIXED"""
    _TypeDefinition = None
    _ContentTypeTag = pyxb.binding.basis.complexTypeDefinition._CT_MIXED
    _Abstract = False
    _ExpandedName = pyxb.namespace.ExpandedName(Namespace, 'QueryExpressionType')
    _XSDLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wstop.xsd', 42, 0)
    _ElementMap = {}
    _AttributeMap = {}
    # Base type is pyxb.binding.datatypes.anyType
    
    # Attribute Dialect uses Python identifier Dialect
    __Dialect = pyxb.binding.content.AttributeUse(pyxb.namespace.ExpandedName(None, 'Dialect'), 'Dialect', '__httpdocs_oasis_open_orgwsnt_1_QueryExpressionType_Dialect', pyxb.binding.datatypes.anyURI, required=True)
    __Dialect._DeclarationLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wstop.xsd', 46, 2)
    __Dialect._UseLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wstop.xsd', 46, 2)
    
    Dialect = property(__Dialect.value, __Dialect.set, None, None)

    _HasWildcardElement = True
    _ElementMap.update({
        
    })
    _AttributeMap.update({
        __Dialect.name() : __Dialect
    })
Namespace.addCategoryObject('typeBinding', 'QueryExpressionType', QueryExpressionType)


# Complex type {http://docs.oasis-open.org/wsn/t-1}TopicNamespaceType with content type ELEMENT_ONLY
class TopicNamespaceType (ExtensibleDocumented):
    """Complex type {http://docs.oasis-open.org/wsn/t-1}TopicNamespaceType with content type ELEMENT_ONLY"""
    _TypeDefinition = None
    _ContentTypeTag = pyxb.binding.basis.complexTypeDefinition._CT_ELEMENT_ONLY
    _Abstract = False
    _ExpandedName = pyxb.namespace.ExpandedName(Namespace, 'TopicNamespaceType')
    _XSDLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wstop.xsd', 50, 2)
    _ElementMap = ExtensibleDocumented._ElementMap.copy()
    _AttributeMap = ExtensibleDocumented._AttributeMap.copy()
    # Base type is ExtensibleDocumented
    
    # Element documentation ({http://docs.oasis-open.org/wsn/t-1}documentation) inherited from {http://docs.oasis-open.org/wsn/t-1}ExtensibleDocumented
    
    # Element {http://docs.oasis-open.org/wsn/t-1}Topic uses Python identifier Topic
    __Topic = pyxb.binding.content.ElementDeclaration(pyxb.namespace.ExpandedName(Namespace, 'Topic'), 'Topic', '__httpdocs_oasis_open_orgwsnt_1_TopicNamespaceType_httpdocs_oasis_open_orgwsnt_1Topic', True, pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wstop.xsd', 54, 11), )

    
    Topic = property(__Topic.value, __Topic.set, None, None)

    
    # Attribute name uses Python identifier name
    __name = pyxb.binding.content.AttributeUse(pyxb.namespace.ExpandedName(None, 'name'), 'name', '__httpdocs_oasis_open_orgwsnt_1_TopicNamespaceType_name', pyxb.binding.datatypes.NCName)
    __name._DeclarationLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wstop.xsd', 68, 9)
    __name._UseLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wstop.xsd', 68, 9)
    
    name = property(__name.value, __name.set, None, None)

    
    # Attribute targetNamespace uses Python identifier targetNamespace
    __targetNamespace = pyxb.binding.content.AttributeUse(pyxb.namespace.ExpandedName(None, 'targetNamespace'), 'targetNamespace', '__httpdocs_oasis_open_orgwsnt_1_TopicNamespaceType_targetNamespace', pyxb.binding.datatypes.anyURI, required=True)
    __targetNamespace._DeclarationLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wstop.xsd', 69, 9)
    __targetNamespace._UseLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wstop.xsd', 69, 9)
    
    targetNamespace = property(__targetNamespace.value, __targetNamespace.set, None, None)

    
    # Attribute final uses Python identifier final
    __final = pyxb.binding.content.AttributeUse(pyxb.namespace.ExpandedName(None, 'final'), 'final', '__httpdocs_oasis_open_orgwsnt_1_TopicNamespaceType_final', pyxb.binding.datatypes.boolean, unicode_default='false')
    __final._DeclarationLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wstop.xsd', 71, 9)
    __final._UseLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wstop.xsd', 71, 9)
    
    final = property(__final.value, __final.set, None, None)

    _AttributeWildcard = pyxb.binding.content.Wildcard(process_contents=pyxb.binding.content.Wildcard.PC_lax, namespace_constraint=(pyxb.binding.content.Wildcard.NC_not, 'http://docs.oasis-open.org/wsn/t-1'))
    _HasWildcardElement = True
    _ElementMap.update({
        __Topic.name() : __Topic
    })
    _AttributeMap.update({
        __name.name() : __name,
        __targetNamespace.name() : __targetNamespace,
        __final.name() : __final
    })
Namespace.addCategoryObject('typeBinding', 'TopicNamespaceType', TopicNamespaceType)


# Complex type {http://docs.oasis-open.org/wsn/t-1}TopicType with content type ELEMENT_ONLY
class TopicType (ExtensibleDocumented):
    """Complex type {http://docs.oasis-open.org/wsn/t-1}TopicType with content type ELEMENT_ONLY"""
    _TypeDefinition = None
    _ContentTypeTag = pyxb.binding.basis.complexTypeDefinition._CT_ELEMENT_ONLY
    _Abstract = False
    _ExpandedName = pyxb.namespace.ExpandedName(Namespace, 'TopicType')
    _XSDLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wstop.xsd', 90, 2)
    _ElementMap = ExtensibleDocumented._ElementMap.copy()
    _AttributeMap = ExtensibleDocumented._AttributeMap.copy()
    # Base type is ExtensibleDocumented
    
    # Element documentation ({http://docs.oasis-open.org/wsn/t-1}documentation) inherited from {http://docs.oasis-open.org/wsn/t-1}ExtensibleDocumented
    
    # Element {http://docs.oasis-open.org/wsn/t-1}MessagePattern uses Python identifier MessagePattern
    __MessagePattern = pyxb.binding.content.ElementDeclaration(pyxb.namespace.ExpandedName(Namespace, 'MessagePattern'), 'MessagePattern', '__httpdocs_oasis_open_orgwsnt_1_TopicType_httpdocs_oasis_open_orgwsnt_1MessagePattern', False, pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wstop.xsd', 94, 10), )

    
    MessagePattern = property(__MessagePattern.value, __MessagePattern.set, None, None)

    
    # Element {http://docs.oasis-open.org/wsn/t-1}Topic uses Python identifier Topic
    __Topic = pyxb.binding.content.ElementDeclaration(pyxb.namespace.ExpandedName(Namespace, 'Topic'), 'Topic', '__httpdocs_oasis_open_orgwsnt_1_TopicType_httpdocs_oasis_open_orgwsnt_1Topic', True, pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wstop.xsd', 97, 10), )

    
    Topic = property(__Topic.value, __Topic.set, None, None)

    
    # Attribute name uses Python identifier name
    __name = pyxb.binding.content.AttributeUse(pyxb.namespace.ExpandedName(None, 'name'), 'name', '__httpdocs_oasis_open_orgwsnt_1_TopicType_name', pyxb.binding.datatypes.NCName, required=True)
    __name._DeclarationLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wstop.xsd', 107, 8)
    __name._UseLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wstop.xsd', 107, 8)
    
    name = property(__name.value, __name.set, None, None)

    
    # Attribute messageTypes uses Python identifier messageTypes
    __messageTypes = pyxb.binding.content.AttributeUse(pyxb.namespace.ExpandedName(None, 'messageTypes'), 'messageTypes', '__httpdocs_oasis_open_orgwsnt_1_TopicType_messageTypes', STD_ANON)
    __messageTypes._DeclarationLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wstop.xsd', 108, 8)
    __messageTypes._UseLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wstop.xsd', 108, 8)
    
    messageTypes = property(__messageTypes.value, __messageTypes.set, None, None)

    
    # Attribute final uses Python identifier final
    __final = pyxb.binding.content.AttributeUse(pyxb.namespace.ExpandedName(None, 'final'), 'final', '__httpdocs_oasis_open_orgwsnt_1_TopicType_final', pyxb.binding.datatypes.boolean, unicode_default='false')
    __final._DeclarationLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wstop.xsd', 113, 8)
    __final._UseLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wstop.xsd', 113, 8)
    
    final = property(__final.value, __final.set, None, None)

    _AttributeWildcard = pyxb.binding.content.Wildcard(process_contents=pyxb.binding.content.Wildcard.PC_lax, namespace_constraint=(pyxb.binding.content.Wildcard.NC_not, 'http://docs.oasis-open.org/wsn/t-1'))
    _HasWildcardElement = True
    _ElementMap.update({
        __MessagePattern.name() : __MessagePattern,
        __Topic.name() : __Topic
    })
    _AttributeMap.update({
        __name.name() : __name,
        __messageTypes.name() : __messageTypes,
        __final.name() : __final
    })
Namespace.addCategoryObject('typeBinding', 'TopicType', TopicType)


# Complex type {http://docs.oasis-open.org/wsn/t-1}TopicSetType with content type ELEMENT_ONLY
class TopicSetType (ExtensibleDocumented):
    """Complex type {http://docs.oasis-open.org/wsn/t-1}TopicSetType with content type ELEMENT_ONLY"""
    _TypeDefinition = None
    _ContentTypeTag = pyxb.binding.basis.complexTypeDefinition._CT_ELEMENT_ONLY
    _Abstract = False
    _ExpandedName = pyxb.namespace.ExpandedName(Namespace, 'TopicSetType')
    _XSDLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wstop.xsd', 121, 2)
    _ElementMap = ExtensibleDocumented._ElementMap.copy()
    _AttributeMap = ExtensibleDocumented._AttributeMap.copy()
    # Base type is ExtensibleDocumented
    
    # Element documentation ({http://docs.oasis-open.org/wsn/t-1}documentation) inherited from {http://docs.oasis-open.org/wsn/t-1}ExtensibleDocumented
    _AttributeWildcard = pyxb.binding.content.Wildcard(process_contents=pyxb.binding.content.Wildcard.PC_lax, namespace_constraint=(pyxb.binding.content.Wildcard.NC_not, 'http://docs.oasis-open.org/wsn/t-1'))
    _HasWildcardElement = True
    _ElementMap.update({
        
    })
    _AttributeMap.update({
        
    })
Namespace.addCategoryObject('typeBinding', 'TopicSetType', TopicSetType)


# Complex type [anonymous] with content type ELEMENT_ONLY
class CTD_ANON (TopicType):
    """Complex type [anonymous] with content type ELEMENT_ONLY"""
    _TypeDefinition = None
    _ContentTypeTag = pyxb.binding.basis.complexTypeDefinition._CT_ELEMENT_ONLY
    _Abstract = False
    _ExpandedName = None
    _XSDLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wstop.xsd', 56, 14)
    _ElementMap = TopicType._ElementMap.copy()
    _AttributeMap = TopicType._AttributeMap.copy()
    # Base type is TopicType
    
    # Element documentation ({http://docs.oasis-open.org/wsn/t-1}documentation) inherited from {http://docs.oasis-open.org/wsn/t-1}ExtensibleDocumented
    
    # Element MessagePattern ({http://docs.oasis-open.org/wsn/t-1}MessagePattern) inherited from {http://docs.oasis-open.org/wsn/t-1}TopicType
    
    # Element Topic ({http://docs.oasis-open.org/wsn/t-1}Topic) inherited from {http://docs.oasis-open.org/wsn/t-1}TopicType
    
    # Attribute parent uses Python identifier parent
    __parent = pyxb.binding.content.AttributeUse(pyxb.namespace.ExpandedName(None, 'parent'), 'parent', '__httpdocs_oasis_open_orgwsnt_1_CTD_ANON_parent', ConcreteTopicExpression)
    __parent._DeclarationLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wstop.xsd', 59, 19)
    __parent._UseLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wstop.xsd', 59, 19)
    
    parent = property(__parent.value, __parent.set, None, None)

    
    # Attribute name inherited from {http://docs.oasis-open.org/wsn/t-1}TopicType
    
    # Attribute messageTypes inherited from {http://docs.oasis-open.org/wsn/t-1}TopicType
    
    # Attribute final inherited from {http://docs.oasis-open.org/wsn/t-1}TopicType
    _AttributeWildcard = pyxb.binding.content.Wildcard(process_contents=pyxb.binding.content.Wildcard.PC_lax, namespace_constraint=(pyxb.binding.content.Wildcard.NC_not, 'http://docs.oasis-open.org/wsn/t-1'))
    _HasWildcardElement = True
    _ElementMap.update({
        
    })
    _AttributeMap.update({
        __parent.name() : __parent
    })



TopicNamespace = pyxb.binding.basis.element(pyxb.namespace.ExpandedName(Namespace, 'TopicNamespace'), TopicNamespaceType, location=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wstop.xsd', 77, 2))
Namespace.addCategoryObject('elementBinding', TopicNamespace.name().localName(), TopicNamespace)

TopicSet = pyxb.binding.basis.element(pyxb.namespace.ExpandedName(Namespace, 'TopicSet'), TopicSetType, location=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wstop.xsd', 133, 2))
Namespace.addCategoryObject('elementBinding', TopicSet.name().localName(), TopicSet)



def _BuildAutomaton ():
    # Remove this helper function from the namespace after it is invoked
    global _BuildAutomaton
    del _BuildAutomaton
    import pyxb.utils.fac as fac

    counters = set()
    cc_0 = fac.CounterCondition(min=0, max=None, metadata=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wstop.xsd', 28, 6))
    counters.add(cc_0)
    states = []
    final_update = set()
    final_update.add(fac.UpdateInstruction(cc_0, False))
    symbol = pyxb.binding.content.WildcardUse(pyxb.binding.content.Wildcard(process_contents=pyxb.binding.content.Wildcard.PC_lax, namespace_constraint=pyxb.binding.content.Wildcard.NC_any), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wstop.xsd', 28, 6))
    st_0 = fac.State(symbol, is_initial=True, final_update=final_update, is_unordered_catenation=False)
    states.append(st_0)
    transitions = []
    transitions.append(fac.Transition(st_0, [
        fac.UpdateInstruction(cc_0, True) ]))
    st_0._set_transitionSet(transitions)
    return fac.Automaton(states, counters, True, containing_state=None)
Documentation._Automaton = _BuildAutomaton()




ExtensibleDocumented._AddElement(pyxb.binding.basis.element(pyxb.namespace.ExpandedName(Namespace, 'documentation'), Documentation, scope=ExtensibleDocumented, location=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wstop.xsd', 36, 6)))

def _BuildAutomaton_ ():
    # Remove this helper function from the namespace after it is invoked
    global _BuildAutomaton_
    del _BuildAutomaton_
    import pyxb.utils.fac as fac

    counters = set()
    cc_0 = fac.CounterCondition(min=0, max=1, metadata=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wstop.xsd', 36, 6))
    counters.add(cc_0)
    states = []
    final_update = set()
    final_update.add(fac.UpdateInstruction(cc_0, False))
    symbol = pyxb.binding.content.ElementUse(ExtensibleDocumented._UseForTag(pyxb.namespace.ExpandedName(Namespace, 'documentation')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wstop.xsd', 36, 6))
    st_0 = fac.State(symbol, is_initial=True, final_update=final_update, is_unordered_catenation=False)
    states.append(st_0)
    transitions = []
    transitions.append(fac.Transition(st_0, [
        fac.UpdateInstruction(cc_0, True) ]))
    st_0._set_transitionSet(transitions)
    return fac.Automaton(states, counters, True, containing_state=None)
ExtensibleDocumented._Automaton = _BuildAutomaton_()




def _BuildAutomaton_2 ():
    # Remove this helper function from the namespace after it is invoked
    global _BuildAutomaton_2
    del _BuildAutomaton_2
    import pyxb.utils.fac as fac

    counters = set()
    cc_0 = fac.CounterCondition(min=0, max=1, metadata=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wstop.xsd', 44, 4))
    counters.add(cc_0)
    states = []
    final_update = set()
    final_update.add(fac.UpdateInstruction(cc_0, False))
    symbol = pyxb.binding.content.WildcardUse(pyxb.binding.content.Wildcard(process_contents=pyxb.binding.content.Wildcard.PC_lax, namespace_constraint=pyxb.binding.content.Wildcard.NC_any), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wstop.xsd', 44, 4))
    st_0 = fac.State(symbol, is_initial=True, final_update=final_update, is_unordered_catenation=False)
    states.append(st_0)
    transitions = []
    transitions.append(fac.Transition(st_0, [
        fac.UpdateInstruction(cc_0, True) ]))
    st_0._set_transitionSet(transitions)
    return fac.Automaton(states, counters, True, containing_state=None)
QueryExpressionType._Automaton = _BuildAutomaton_2()




TopicNamespaceType._AddElement(pyxb.binding.basis.element(pyxb.namespace.ExpandedName(Namespace, 'Topic'), CTD_ANON, scope=TopicNamespaceType, location=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wstop.xsd', 54, 11)))

def _BuildAutomaton_3 ():
    # Remove this helper function from the namespace after it is invoked
    global _BuildAutomaton_3
    del _BuildAutomaton_3
    import pyxb.utils.fac as fac

    counters = set()
    cc_0 = fac.CounterCondition(min=0, max=1, metadata=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wstop.xsd', 36, 6))
    counters.add(cc_0)
    cc_1 = fac.CounterCondition(min=0, max=None, metadata=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wstop.xsd', 54, 11))
    counters.add(cc_1)
    cc_2 = fac.CounterCondition(min=0, max=None, metadata=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wstop.xsd', 64, 11))
    counters.add(cc_2)
    states = []
    final_update = set()
    final_update.add(fac.UpdateInstruction(cc_0, False))
    symbol = pyxb.binding.content.ElementUse(TopicNamespaceType._UseForTag(pyxb.namespace.ExpandedName(Namespace, 'documentation')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wstop.xsd', 36, 6))
    st_0 = fac.State(symbol, is_initial=True, final_update=final_update, is_unordered_catenation=False)
    states.append(st_0)
    final_update = set()
    final_update.add(fac.UpdateInstruction(cc_1, False))
    symbol = pyxb.binding.content.ElementUse(TopicNamespaceType._UseForTag(pyxb.namespace.ExpandedName(Namespace, 'Topic')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wstop.xsd', 54, 11))
    st_1 = fac.State(symbol, is_initial=True, final_update=final_update, is_unordered_catenation=False)
    states.append(st_1)
    final_update = set()
    final_update.add(fac.UpdateInstruction(cc_2, False))
    symbol = pyxb.binding.content.WildcardUse(pyxb.binding.content.Wildcard(process_contents=pyxb.binding.content.Wildcard.PC_lax, namespace_constraint=(pyxb.binding.content.Wildcard.NC_not, 'http://docs.oasis-open.org/wsn/t-1')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wstop.xsd', 64, 11))
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
TopicNamespaceType._Automaton = _BuildAutomaton_3()




TopicType._AddElement(pyxb.binding.basis.element(pyxb.namespace.ExpandedName(Namespace, 'MessagePattern'), QueryExpressionType, scope=TopicType, location=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wstop.xsd', 94, 10)))

TopicType._AddElement(pyxb.binding.basis.element(pyxb.namespace.ExpandedName(Namespace, 'Topic'), TopicType, scope=TopicType, location=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wstop.xsd', 97, 10)))

def _BuildAutomaton_4 ():
    # Remove this helper function from the namespace after it is invoked
    global _BuildAutomaton_4
    del _BuildAutomaton_4
    import pyxb.utils.fac as fac

    counters = set()
    cc_0 = fac.CounterCondition(min=0, max=1, metadata=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wstop.xsd', 36, 6))
    counters.add(cc_0)
    cc_1 = fac.CounterCondition(min=0, max=1, metadata=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wstop.xsd', 94, 10))
    counters.add(cc_1)
    cc_2 = fac.CounterCondition(min=0, max=None, metadata=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wstop.xsd', 97, 10))
    counters.add(cc_2)
    cc_3 = fac.CounterCondition(min=0, max=None, metadata=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wstop.xsd', 104, 10))
    counters.add(cc_3)
    states = []
    final_update = set()
    final_update.add(fac.UpdateInstruction(cc_0, False))
    symbol = pyxb.binding.content.ElementUse(TopicType._UseForTag(pyxb.namespace.ExpandedName(Namespace, 'documentation')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wstop.xsd', 36, 6))
    st_0 = fac.State(symbol, is_initial=True, final_update=final_update, is_unordered_catenation=False)
    states.append(st_0)
    final_update = set()
    final_update.add(fac.UpdateInstruction(cc_1, False))
    symbol = pyxb.binding.content.ElementUse(TopicType._UseForTag(pyxb.namespace.ExpandedName(Namespace, 'MessagePattern')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wstop.xsd', 94, 10))
    st_1 = fac.State(symbol, is_initial=True, final_update=final_update, is_unordered_catenation=False)
    states.append(st_1)
    final_update = set()
    final_update.add(fac.UpdateInstruction(cc_2, False))
    symbol = pyxb.binding.content.ElementUse(TopicType._UseForTag(pyxb.namespace.ExpandedName(Namespace, 'Topic')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wstop.xsd', 97, 10))
    st_2 = fac.State(symbol, is_initial=True, final_update=final_update, is_unordered_catenation=False)
    states.append(st_2)
    final_update = set()
    final_update.add(fac.UpdateInstruction(cc_3, False))
    symbol = pyxb.binding.content.WildcardUse(pyxb.binding.content.Wildcard(process_contents=pyxb.binding.content.Wildcard.PC_strict, namespace_constraint=(pyxb.binding.content.Wildcard.NC_not, 'http://docs.oasis-open.org/wsn/t-1')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wstop.xsd', 104, 10))
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
TopicType._Automaton = _BuildAutomaton_4()




def _BuildAutomaton_5 ():
    # Remove this helper function from the namespace after it is invoked
    global _BuildAutomaton_5
    del _BuildAutomaton_5
    import pyxb.utils.fac as fac

    counters = set()
    cc_0 = fac.CounterCondition(min=0, max=1, metadata=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wstop.xsd', 36, 6))
    counters.add(cc_0)
    cc_1 = fac.CounterCondition(min=0, max=None, metadata=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wstop.xsd', 125, 11))
    counters.add(cc_1)
    states = []
    final_update = set()
    final_update.add(fac.UpdateInstruction(cc_0, False))
    symbol = pyxb.binding.content.ElementUse(TopicSetType._UseForTag(pyxb.namespace.ExpandedName(Namespace, 'documentation')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wstop.xsd', 36, 6))
    st_0 = fac.State(symbol, is_initial=True, final_update=final_update, is_unordered_catenation=False)
    states.append(st_0)
    final_update = set()
    final_update.add(fac.UpdateInstruction(cc_1, False))
    symbol = pyxb.binding.content.WildcardUse(pyxb.binding.content.Wildcard(process_contents=pyxb.binding.content.Wildcard.PC_lax, namespace_constraint=(pyxb.binding.content.Wildcard.NC_not, 'http://docs.oasis-open.org/wsn/t-1')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wstop.xsd', 125, 11))
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
TopicSetType._Automaton = _BuildAutomaton_5()




def _BuildAutomaton_6 ():
    # Remove this helper function from the namespace after it is invoked
    global _BuildAutomaton_6
    del _BuildAutomaton_6
    import pyxb.utils.fac as fac

    counters = set()
    cc_0 = fac.CounterCondition(min=0, max=1, metadata=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wstop.xsd', 36, 6))
    counters.add(cc_0)
    cc_1 = fac.CounterCondition(min=0, max=1, metadata=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wstop.xsd', 94, 10))
    counters.add(cc_1)
    cc_2 = fac.CounterCondition(min=0, max=None, metadata=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wstop.xsd', 97, 10))
    counters.add(cc_2)
    cc_3 = fac.CounterCondition(min=0, max=None, metadata=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wstop.xsd', 104, 10))
    counters.add(cc_3)
    states = []
    final_update = set()
    final_update.add(fac.UpdateInstruction(cc_0, False))
    symbol = pyxb.binding.content.ElementUse(CTD_ANON._UseForTag(pyxb.namespace.ExpandedName(Namespace, 'documentation')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wstop.xsd', 36, 6))
    st_0 = fac.State(symbol, is_initial=True, final_update=final_update, is_unordered_catenation=False)
    states.append(st_0)
    final_update = set()
    final_update.add(fac.UpdateInstruction(cc_1, False))
    symbol = pyxb.binding.content.ElementUse(CTD_ANON._UseForTag(pyxb.namespace.ExpandedName(Namespace, 'MessagePattern')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wstop.xsd', 94, 10))
    st_1 = fac.State(symbol, is_initial=True, final_update=final_update, is_unordered_catenation=False)
    states.append(st_1)
    final_update = set()
    final_update.add(fac.UpdateInstruction(cc_2, False))
    symbol = pyxb.binding.content.ElementUse(CTD_ANON._UseForTag(pyxb.namespace.ExpandedName(Namespace, 'Topic')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wstop.xsd', 97, 10))
    st_2 = fac.State(symbol, is_initial=True, final_update=final_update, is_unordered_catenation=False)
    states.append(st_2)
    final_update = set()
    final_update.add(fac.UpdateInstruction(cc_3, False))
    symbol = pyxb.binding.content.WildcardUse(pyxb.binding.content.Wildcard(process_contents=pyxb.binding.content.Wildcard.PC_strict, namespace_constraint=(pyxb.binding.content.Wildcard.NC_not, 'http://docs.oasis-open.org/wsn/t-1')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wstop.xsd', 104, 10))
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
CTD_ANON._Automaton = _BuildAutomaton_6()

