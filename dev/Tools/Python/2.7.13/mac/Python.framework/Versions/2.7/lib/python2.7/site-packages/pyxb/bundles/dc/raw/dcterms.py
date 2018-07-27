# ./pyxb/bundles/dc/raw/dcterms.py
# -*- coding: utf-8 -*-
# PyXB bindings for NM:62e52a6e1b0d23522982e9c2905e5cb67ad01951
# Generated 2014-10-19 06:25:02.925349 by PyXB version 1.2.4 using Python 2.7.3.final.0
# Namespace http://purl.org/dc/terms/

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
_GenerationUID = pyxb.utils.utility.UniqueIdentifier('urn:uuid:91a2ca00-5782-11e4-8790-c8600024e903')

# Version of PyXB used to generate the bindings
_PyXBVersion = '1.2.4'
# Generated bindings are not compatible across PyXB versions
if pyxb.__version__ != _PyXBVersion:
    raise pyxb.PyXBVersionError(_PyXBVersion)

# Import bindings for namespaces imported into schema
import pyxb.bundles.dc.dc
import pyxb.binding.datatypes
import pyxb.binding.xml_
import pyxb.bundles.dc.dcmitype

# NOTE: All namespace declarations are reserved within the binding
Namespace = pyxb.namespace.NamespaceForURI('http://purl.org/dc/terms/', create_if_missing=True)
Namespace.configureCategories(['typeBinding', 'elementBinding'])
_Namespace_dc = pyxb.bundles.dc.dc.Namespace
_Namespace_dc.configureCategories(['typeBinding', 'elementBinding'])

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


# Atomic simple type: [anonymous]
class STD_ANON (pyxb.binding.datatypes.string):

    """An atomic simple type."""

    _ExpandedName = None
    _XSDLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/dc/schemas/dcterms.xsd', 144, 8)
    _Documentation = None
STD_ANON._InitializeFacetMap()

# Atomic simple type: [anonymous]
class STD_ANON_ (pyxb.binding.datatypes.string):

    """An atomic simple type."""

    _ExpandedName = None
    _XSDLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/dc/schemas/dcterms.xsd', 155, 8)
    _Documentation = None
STD_ANON_._InitializeFacetMap()

# Atomic simple type: [anonymous]
class STD_ANON_2 (pyxb.binding.datatypes.string):

    """An atomic simple type."""

    _ExpandedName = None
    _XSDLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/dc/schemas/dcterms.xsd', 166, 8)
    _Documentation = None
STD_ANON_2._InitializeFacetMap()

# Atomic simple type: [anonymous]
class STD_ANON_3 (pyxb.binding.datatypes.string):

    """An atomic simple type."""

    _ExpandedName = None
    _XSDLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/dc/schemas/dcterms.xsd', 177, 8)
    _Documentation = None
STD_ANON_3._InitializeFacetMap()

# Atomic simple type: [anonymous]
class STD_ANON_4 (pyxb.binding.datatypes.string):

    """An atomic simple type."""

    _ExpandedName = None
    _XSDLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/dc/schemas/dcterms.xsd', 188, 8)
    _Documentation = None
STD_ANON_4._InitializeFacetMap()

# Atomic simple type: [anonymous]
class STD_ANON_5 (pyxb.binding.datatypes.string):

    """An atomic simple type."""

    _ExpandedName = None
    _XSDLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/dc/schemas/dcterms.xsd', 199, 8)
    _Documentation = None
STD_ANON_5._InitializeFacetMap()

# Union simple type: [anonymous]
# superclasses pyxb.binding.datatypes.anySimpleType
class STD_ANON_6 (pyxb.binding.basis.STD_union):

    """Simple type that is a union of pyxb.binding.datatypes.gYear, pyxb.binding.datatypes.gYearMonth, pyxb.binding.datatypes.date, pyxb.binding.datatypes.dateTime."""

    _ExpandedName = None
    _XSDLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/dc/schemas/dcterms.xsd', 210, 8)
    _Documentation = None

    _MemberTypes = ( pyxb.binding.datatypes.gYear, pyxb.binding.datatypes.gYearMonth, pyxb.binding.datatypes.date, pyxb.binding.datatypes.dateTime, )
STD_ANON_6._CF_pattern = pyxb.binding.facets.CF_pattern()
STD_ANON_6._CF_enumeration = pyxb.binding.facets.CF_enumeration(value_datatype=STD_ANON_6)
STD_ANON_6._InitializeFacetMap(STD_ANON_6._CF_pattern,
   STD_ANON_6._CF_enumeration)

# Atomic simple type: [anonymous]
class STD_ANON_7 (pyxb.binding.datatypes.string):

    """An atomic simple type."""

    _ExpandedName = None
    _XSDLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/dc/schemas/dcterms.xsd', 232, 8)
    _Documentation = None
STD_ANON_7._InitializeFacetMap()

# Atomic simple type: [anonymous]
class STD_ANON_8 (pyxb.binding.datatypes.anyURI):

    """An atomic simple type."""

    _ExpandedName = None
    _XSDLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/dc/schemas/dcterms.xsd', 243, 8)
    _Documentation = None
STD_ANON_8._InitializeFacetMap()

# Atomic simple type: [anonymous]
class STD_ANON_9 (pyxb.binding.datatypes.string):

    """An atomic simple type."""

    _ExpandedName = None
    _XSDLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/dc/schemas/dcterms.xsd', 254, 8)
    _Documentation = None
STD_ANON_9._InitializeFacetMap()

# Atomic simple type: [anonymous]
class STD_ANON_10 (pyxb.binding.datatypes.string):

    """An atomic simple type."""

    _ExpandedName = None
    _XSDLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/dc/schemas/dcterms.xsd', 265, 8)
    _Documentation = None
STD_ANON_10._InitializeFacetMap()

# Atomic simple type: [anonymous]
class STD_ANON_11 (pyxb.binding.datatypes.language):

    """An atomic simple type."""

    _ExpandedName = None
    _XSDLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/dc/schemas/dcterms.xsd', 276, 8)
    _Documentation = None
STD_ANON_11._InitializeFacetMap()

# Atomic simple type: [anonymous]
class STD_ANON_12 (pyxb.binding.datatypes.language):

    """An atomic simple type."""

    _ExpandedName = None
    _XSDLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/dc/schemas/dcterms.xsd', 287, 8)
    _Documentation = None
STD_ANON_12._InitializeFacetMap()

# Atomic simple type: [anonymous]
class STD_ANON_13 (pyxb.binding.datatypes.language):

    """An atomic simple type."""

    _ExpandedName = None
    _XSDLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/dc/schemas/dcterms.xsd', 298, 8)
    _Documentation = None
STD_ANON_13._InitializeFacetMap()

# Atomic simple type: [anonymous]
class STD_ANON_14 (pyxb.binding.datatypes.string):

    """An atomic simple type."""

    _ExpandedName = None
    _XSDLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/dc/schemas/dcterms.xsd', 309, 8)
    _Documentation = None
STD_ANON_14._InitializeFacetMap()

# Atomic simple type: [anonymous]
class STD_ANON_15 (pyxb.binding.datatypes.string):

    """An atomic simple type."""

    _ExpandedName = None
    _XSDLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/dc/schemas/dcterms.xsd', 320, 8)
    _Documentation = None
STD_ANON_15._InitializeFacetMap()

# Atomic simple type: [anonymous]
class STD_ANON_16 (pyxb.binding.datatypes.string):

    """An atomic simple type."""

    _ExpandedName = None
    _XSDLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/dc/schemas/dcterms.xsd', 331, 8)
    _Documentation = None
STD_ANON_16._InitializeFacetMap()

# Atomic simple type: [anonymous]
class STD_ANON_17 (pyxb.binding.datatypes.string):

    """An atomic simple type."""

    _ExpandedName = None
    _XSDLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/dc/schemas/dcterms.xsd', 342, 8)
    _Documentation = None
STD_ANON_17._InitializeFacetMap()

# Union simple type: [anonymous]
# superclasses pyxb.bundles.dc.dcmitype.DCMIType
class STD_ANON_18 (pyxb.binding.basis.STD_union):

    """Simple type that is a union of pyxb.bundles.dc.dcmitype.STD_ANON."""

    _ExpandedName = None
    _XSDLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/dc/schemas/dcterms.xsd', 221, 8)
    _Documentation = None

    _MemberTypes = ( pyxb.bundles.dc.dcmitype.STD_ANON, )
STD_ANON_18.Collection = 'Collection'             # originally pyxb.bundles.dc.dcmitype.STD_ANON.Collection
STD_ANON_18.Dataset = 'Dataset'                   # originally pyxb.bundles.dc.dcmitype.STD_ANON.Dataset
STD_ANON_18.Event = 'Event'                       # originally pyxb.bundles.dc.dcmitype.STD_ANON.Event
STD_ANON_18.Image = 'Image'                       # originally pyxb.bundles.dc.dcmitype.STD_ANON.Image
STD_ANON_18.MovingImage = 'MovingImage'           # originally pyxb.bundles.dc.dcmitype.STD_ANON.MovingImage
STD_ANON_18.StillImage = 'StillImage'             # originally pyxb.bundles.dc.dcmitype.STD_ANON.StillImage
STD_ANON_18.InteractiveResource = 'InteractiveResource'# originally pyxb.bundles.dc.dcmitype.STD_ANON.InteractiveResource
STD_ANON_18.Service = 'Service'                   # originally pyxb.bundles.dc.dcmitype.STD_ANON.Service
STD_ANON_18.Software = 'Software'                 # originally pyxb.bundles.dc.dcmitype.STD_ANON.Software
STD_ANON_18.Sound = 'Sound'                       # originally pyxb.bundles.dc.dcmitype.STD_ANON.Sound
STD_ANON_18.Text = 'Text'                         # originally pyxb.bundles.dc.dcmitype.STD_ANON.Text
STD_ANON_18.PhysicalObject = 'PhysicalObject'     # originally pyxb.bundles.dc.dcmitype.STD_ANON.PhysicalObject
STD_ANON_18._InitializeFacetMap()

# Complex type {http://purl.org/dc/terms/}elementOrRefinementContainer with content type ELEMENT_ONLY
class elementOrRefinementContainer (pyxb.binding.basis.complexTypeDefinition):
    """
    		This is included as a convenience for schema authors who need to define a root
    		or container element for all of the DC elements and element refinements.
    	"""
    _TypeDefinition = None
    _ContentTypeTag = pyxb.binding.basis.complexTypeDefinition._CT_ELEMENT_ONLY
    _Abstract = False
    _ExpandedName = pyxb.namespace.ExpandedName(Namespace, 'elementOrRefinementContainer')
    _XSDLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/dc/schemas/dcterms.xsd', 368, 2)
    _ElementMap = {}
    _AttributeMap = {}
    # Base type is pyxb.binding.datatypes.anyType
    
    # Element {http://purl.org/dc/elements/1.1/}any uses Python identifier any
    __any = pyxb.binding.content.ElementDeclaration(pyxb.namespace.ExpandedName(_Namespace_dc, 'any'), 'any', '__httppurl_orgdcterms_elementOrRefinementContainer_httppurl_orgdcelements1_1any', True, pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/dc/schemas/dc.xsd', 70, 2), )

    
    any = property(__any.value, __any.set, None, None)

    _ElementMap.update({
        __any.name() : __any
    })
    _AttributeMap.update({
        
    })
Namespace.addCategoryObject('typeBinding', 'elementOrRefinementContainer', elementOrRefinementContainer)


# Complex type {http://purl.org/dc/terms/}LCSH with content type SIMPLE
class LCSH (pyxb.bundles.dc.dc.SimpleLiteral):
    """Complex type {http://purl.org/dc/terms/}LCSH with content type SIMPLE"""
    _TypeDefinition = STD_ANON
    _ContentTypeTag = pyxb.binding.basis.complexTypeDefinition._CT_SIMPLE
    _Abstract = False
    _ExpandedName = pyxb.namespace.ExpandedName(Namespace, 'LCSH')
    _XSDLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/dc/schemas/dcterms.xsd', 141, 2)
    _ElementMap = pyxb.bundles.dc.dc.SimpleLiteral._ElementMap.copy()
    _AttributeMap = pyxb.bundles.dc.dc.SimpleLiteral._AttributeMap.copy()
    # Base type is pyxb.bundles.dc.dc.SimpleLiteral
    
    # Attribute lang is restricted from parent
    
    # Attribute {http://www.w3.org/XML/1998/namespace}lang uses Python identifier lang
    __lang = pyxb.binding.content.AttributeUse(pyxb.namespace.ExpandedName(pyxb.namespace.XML, 'lang'), 'lang', '__httppurl_orgdcelements1_1_SimpleLiteral_httpwww_w3_orgXML1998namespacelang', pyxb.binding.xml_.STD_ANON_lang, prohibited=True)
    __lang._DeclarationLocation = None
    __lang._UseLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/dc/schemas/dcterms.xsd', 147, 8)
    
    lang = property(__lang.value, __lang.set, None, None)

    _ElementMap.update({
        
    })
    _AttributeMap.update({
        __lang.name() : __lang
    })
Namespace.addCategoryObject('typeBinding', 'LCSH', LCSH)


# Complex type {http://purl.org/dc/terms/}MESH with content type SIMPLE
class MESH (pyxb.bundles.dc.dc.SimpleLiteral):
    """Complex type {http://purl.org/dc/terms/}MESH with content type SIMPLE"""
    _TypeDefinition = STD_ANON_
    _ContentTypeTag = pyxb.binding.basis.complexTypeDefinition._CT_SIMPLE
    _Abstract = False
    _ExpandedName = pyxb.namespace.ExpandedName(Namespace, 'MESH')
    _XSDLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/dc/schemas/dcterms.xsd', 152, 2)
    _ElementMap = pyxb.bundles.dc.dc.SimpleLiteral._ElementMap.copy()
    _AttributeMap = pyxb.bundles.dc.dc.SimpleLiteral._AttributeMap.copy()
    # Base type is pyxb.bundles.dc.dc.SimpleLiteral
    
    # Attribute lang is restricted from parent
    
    # Attribute {http://www.w3.org/XML/1998/namespace}lang uses Python identifier lang
    __lang = pyxb.binding.content.AttributeUse(pyxb.namespace.ExpandedName(pyxb.namespace.XML, 'lang'), 'lang', '__httppurl_orgdcelements1_1_SimpleLiteral_httpwww_w3_orgXML1998namespacelang', pyxb.binding.xml_.STD_ANON_lang, prohibited=True)
    __lang._DeclarationLocation = None
    __lang._UseLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/dc/schemas/dcterms.xsd', 158, 8)
    
    lang = property(__lang.value, __lang.set, None, None)

    _ElementMap.update({
        
    })
    _AttributeMap.update({
        __lang.name() : __lang
    })
Namespace.addCategoryObject('typeBinding', 'MESH', MESH)


# Complex type {http://purl.org/dc/terms/}DDC with content type SIMPLE
class DDC (pyxb.bundles.dc.dc.SimpleLiteral):
    """Complex type {http://purl.org/dc/terms/}DDC with content type SIMPLE"""
    _TypeDefinition = STD_ANON_2
    _ContentTypeTag = pyxb.binding.basis.complexTypeDefinition._CT_SIMPLE
    _Abstract = False
    _ExpandedName = pyxb.namespace.ExpandedName(Namespace, 'DDC')
    _XSDLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/dc/schemas/dcterms.xsd', 163, 2)
    _ElementMap = pyxb.bundles.dc.dc.SimpleLiteral._ElementMap.copy()
    _AttributeMap = pyxb.bundles.dc.dc.SimpleLiteral._AttributeMap.copy()
    # Base type is pyxb.bundles.dc.dc.SimpleLiteral
    
    # Attribute lang is restricted from parent
    
    # Attribute {http://www.w3.org/XML/1998/namespace}lang uses Python identifier lang
    __lang = pyxb.binding.content.AttributeUse(pyxb.namespace.ExpandedName(pyxb.namespace.XML, 'lang'), 'lang', '__httppurl_orgdcelements1_1_SimpleLiteral_httpwww_w3_orgXML1998namespacelang', pyxb.binding.xml_.STD_ANON_lang, prohibited=True)
    __lang._DeclarationLocation = None
    __lang._UseLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/dc/schemas/dcterms.xsd', 169, 8)
    
    lang = property(__lang.value, __lang.set, None, None)

    _ElementMap.update({
        
    })
    _AttributeMap.update({
        __lang.name() : __lang
    })
Namespace.addCategoryObject('typeBinding', 'DDC', DDC)


# Complex type {http://purl.org/dc/terms/}LCC with content type SIMPLE
class LCC (pyxb.bundles.dc.dc.SimpleLiteral):
    """Complex type {http://purl.org/dc/terms/}LCC with content type SIMPLE"""
    _TypeDefinition = STD_ANON_3
    _ContentTypeTag = pyxb.binding.basis.complexTypeDefinition._CT_SIMPLE
    _Abstract = False
    _ExpandedName = pyxb.namespace.ExpandedName(Namespace, 'LCC')
    _XSDLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/dc/schemas/dcterms.xsd', 174, 2)
    _ElementMap = pyxb.bundles.dc.dc.SimpleLiteral._ElementMap.copy()
    _AttributeMap = pyxb.bundles.dc.dc.SimpleLiteral._AttributeMap.copy()
    # Base type is pyxb.bundles.dc.dc.SimpleLiteral
    
    # Attribute lang is restricted from parent
    
    # Attribute {http://www.w3.org/XML/1998/namespace}lang uses Python identifier lang
    __lang = pyxb.binding.content.AttributeUse(pyxb.namespace.ExpandedName(pyxb.namespace.XML, 'lang'), 'lang', '__httppurl_orgdcelements1_1_SimpleLiteral_httpwww_w3_orgXML1998namespacelang', pyxb.binding.xml_.STD_ANON_lang, prohibited=True)
    __lang._DeclarationLocation = None
    __lang._UseLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/dc/schemas/dcterms.xsd', 180, 8)
    
    lang = property(__lang.value, __lang.set, None, None)

    _ElementMap.update({
        
    })
    _AttributeMap.update({
        __lang.name() : __lang
    })
Namespace.addCategoryObject('typeBinding', 'LCC', LCC)


# Complex type {http://purl.org/dc/terms/}UDC with content type SIMPLE
class UDC (pyxb.bundles.dc.dc.SimpleLiteral):
    """Complex type {http://purl.org/dc/terms/}UDC with content type SIMPLE"""
    _TypeDefinition = STD_ANON_4
    _ContentTypeTag = pyxb.binding.basis.complexTypeDefinition._CT_SIMPLE
    _Abstract = False
    _ExpandedName = pyxb.namespace.ExpandedName(Namespace, 'UDC')
    _XSDLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/dc/schemas/dcterms.xsd', 185, 2)
    _ElementMap = pyxb.bundles.dc.dc.SimpleLiteral._ElementMap.copy()
    _AttributeMap = pyxb.bundles.dc.dc.SimpleLiteral._AttributeMap.copy()
    # Base type is pyxb.bundles.dc.dc.SimpleLiteral
    
    # Attribute lang is restricted from parent
    
    # Attribute {http://www.w3.org/XML/1998/namespace}lang uses Python identifier lang
    __lang = pyxb.binding.content.AttributeUse(pyxb.namespace.ExpandedName(pyxb.namespace.XML, 'lang'), 'lang', '__httppurl_orgdcelements1_1_SimpleLiteral_httpwww_w3_orgXML1998namespacelang', pyxb.binding.xml_.STD_ANON_lang, prohibited=True)
    __lang._DeclarationLocation = None
    __lang._UseLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/dc/schemas/dcterms.xsd', 191, 8)
    
    lang = property(__lang.value, __lang.set, None, None)

    _ElementMap.update({
        
    })
    _AttributeMap.update({
        __lang.name() : __lang
    })
Namespace.addCategoryObject('typeBinding', 'UDC', UDC)


# Complex type {http://purl.org/dc/terms/}Period with content type SIMPLE
class Period (pyxb.bundles.dc.dc.SimpleLiteral):
    """Complex type {http://purl.org/dc/terms/}Period with content type SIMPLE"""
    _TypeDefinition = STD_ANON_5
    _ContentTypeTag = pyxb.binding.basis.complexTypeDefinition._CT_SIMPLE
    _Abstract = False
    _ExpandedName = pyxb.namespace.ExpandedName(Namespace, 'Period')
    _XSDLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/dc/schemas/dcterms.xsd', 196, 2)
    _ElementMap = pyxb.bundles.dc.dc.SimpleLiteral._ElementMap.copy()
    _AttributeMap = pyxb.bundles.dc.dc.SimpleLiteral._AttributeMap.copy()
    # Base type is pyxb.bundles.dc.dc.SimpleLiteral
    
    # Attribute lang is restricted from parent
    
    # Attribute {http://www.w3.org/XML/1998/namespace}lang uses Python identifier lang
    __lang = pyxb.binding.content.AttributeUse(pyxb.namespace.ExpandedName(pyxb.namespace.XML, 'lang'), 'lang', '__httppurl_orgdcelements1_1_SimpleLiteral_httpwww_w3_orgXML1998namespacelang', pyxb.binding.xml_.STD_ANON_lang, prohibited=True)
    __lang._DeclarationLocation = None
    __lang._UseLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/dc/schemas/dcterms.xsd', 202, 8)
    
    lang = property(__lang.value, __lang.set, None, None)

    _ElementMap.update({
        
    })
    _AttributeMap.update({
        __lang.name() : __lang
    })
Namespace.addCategoryObject('typeBinding', 'Period', Period)


# Complex type {http://purl.org/dc/terms/}W3CDTF with content type SIMPLE
class W3CDTF (pyxb.bundles.dc.dc.SimpleLiteral):
    """Complex type {http://purl.org/dc/terms/}W3CDTF with content type SIMPLE"""
    _TypeDefinition = STD_ANON_6
    _ContentTypeTag = pyxb.binding.basis.complexTypeDefinition._CT_SIMPLE
    _Abstract = False
    _ExpandedName = pyxb.namespace.ExpandedName(Namespace, 'W3CDTF')
    _XSDLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/dc/schemas/dcterms.xsd', 207, 2)
    _ElementMap = pyxb.bundles.dc.dc.SimpleLiteral._ElementMap.copy()
    _AttributeMap = pyxb.bundles.dc.dc.SimpleLiteral._AttributeMap.copy()
    # Base type is pyxb.bundles.dc.dc.SimpleLiteral
    
    # Attribute lang is restricted from parent
    
    # Attribute {http://www.w3.org/XML/1998/namespace}lang uses Python identifier lang
    __lang = pyxb.binding.content.AttributeUse(pyxb.namespace.ExpandedName(pyxb.namespace.XML, 'lang'), 'lang', '__httppurl_orgdcelements1_1_SimpleLiteral_httpwww_w3_orgXML1998namespacelang', pyxb.binding.xml_.STD_ANON_lang, prohibited=True)
    __lang._DeclarationLocation = None
    __lang._UseLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/dc/schemas/dcterms.xsd', 213, 8)
    
    lang = property(__lang.value, __lang.set, None, None)

    _ElementMap.update({
        
    })
    _AttributeMap.update({
        __lang.name() : __lang
    })
Namespace.addCategoryObject('typeBinding', 'W3CDTF', W3CDTF)


# Complex type {http://purl.org/dc/terms/}DCMIType with content type SIMPLE
class DCMIType (pyxb.bundles.dc.dc.SimpleLiteral):
    """Complex type {http://purl.org/dc/terms/}DCMIType with content type SIMPLE"""
    _TypeDefinition = STD_ANON_18
    _ContentTypeTag = pyxb.binding.basis.complexTypeDefinition._CT_SIMPLE
    _Abstract = False
    _ExpandedName = pyxb.namespace.ExpandedName(Namespace, 'DCMIType')
    _XSDLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/dc/schemas/dcterms.xsd', 218, 2)
    _ElementMap = pyxb.bundles.dc.dc.SimpleLiteral._ElementMap.copy()
    _AttributeMap = pyxb.bundles.dc.dc.SimpleLiteral._AttributeMap.copy()
    # Base type is pyxb.bundles.dc.dc.SimpleLiteral
    
    # Attribute lang is restricted from parent
    
    # Attribute {http://www.w3.org/XML/1998/namespace}lang uses Python identifier lang
    __lang = pyxb.binding.content.AttributeUse(pyxb.namespace.ExpandedName(pyxb.namespace.XML, 'lang'), 'lang', '__httppurl_orgdcelements1_1_SimpleLiteral_httpwww_w3_orgXML1998namespacelang', pyxb.binding.xml_.STD_ANON_lang, prohibited=True)
    __lang._DeclarationLocation = None
    __lang._UseLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/dc/schemas/dcterms.xsd', 224, 8)
    
    lang = property(__lang.value, __lang.set, None, None)

    _ElementMap.update({
        
    })
    _AttributeMap.update({
        __lang.name() : __lang
    })
Namespace.addCategoryObject('typeBinding', 'DCMIType', DCMIType)


# Complex type {http://purl.org/dc/terms/}IMT with content type SIMPLE
class IMT (pyxb.bundles.dc.dc.SimpleLiteral):
    """Complex type {http://purl.org/dc/terms/}IMT with content type SIMPLE"""
    _TypeDefinition = STD_ANON_7
    _ContentTypeTag = pyxb.binding.basis.complexTypeDefinition._CT_SIMPLE
    _Abstract = False
    _ExpandedName = pyxb.namespace.ExpandedName(Namespace, 'IMT')
    _XSDLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/dc/schemas/dcterms.xsd', 229, 2)
    _ElementMap = pyxb.bundles.dc.dc.SimpleLiteral._ElementMap.copy()
    _AttributeMap = pyxb.bundles.dc.dc.SimpleLiteral._AttributeMap.copy()
    # Base type is pyxb.bundles.dc.dc.SimpleLiteral
    
    # Attribute lang is restricted from parent
    
    # Attribute {http://www.w3.org/XML/1998/namespace}lang uses Python identifier lang
    __lang = pyxb.binding.content.AttributeUse(pyxb.namespace.ExpandedName(pyxb.namespace.XML, 'lang'), 'lang', '__httppurl_orgdcelements1_1_SimpleLiteral_httpwww_w3_orgXML1998namespacelang', pyxb.binding.xml_.STD_ANON_lang, prohibited=True)
    __lang._DeclarationLocation = None
    __lang._UseLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/dc/schemas/dcterms.xsd', 235, 8)
    
    lang = property(__lang.value, __lang.set, None, None)

    _ElementMap.update({
        
    })
    _AttributeMap.update({
        __lang.name() : __lang
    })
Namespace.addCategoryObject('typeBinding', 'IMT', IMT)


# Complex type {http://purl.org/dc/terms/}URI with content type SIMPLE
class URI (pyxb.bundles.dc.dc.SimpleLiteral):
    """Complex type {http://purl.org/dc/terms/}URI with content type SIMPLE"""
    _TypeDefinition = STD_ANON_8
    _ContentTypeTag = pyxb.binding.basis.complexTypeDefinition._CT_SIMPLE
    _Abstract = False
    _ExpandedName = pyxb.namespace.ExpandedName(Namespace, 'URI')
    _XSDLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/dc/schemas/dcterms.xsd', 240, 2)
    _ElementMap = pyxb.bundles.dc.dc.SimpleLiteral._ElementMap.copy()
    _AttributeMap = pyxb.bundles.dc.dc.SimpleLiteral._AttributeMap.copy()
    # Base type is pyxb.bundles.dc.dc.SimpleLiteral
    
    # Attribute lang is restricted from parent
    
    # Attribute {http://www.w3.org/XML/1998/namespace}lang uses Python identifier lang
    __lang = pyxb.binding.content.AttributeUse(pyxb.namespace.ExpandedName(pyxb.namespace.XML, 'lang'), 'lang', '__httppurl_orgdcelements1_1_SimpleLiteral_httpwww_w3_orgXML1998namespacelang', pyxb.binding.xml_.STD_ANON_lang, prohibited=True)
    __lang._DeclarationLocation = None
    __lang._UseLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/dc/schemas/dcterms.xsd', 246, 8)
    
    lang = property(__lang.value, __lang.set, None, None)

    _ElementMap.update({
        
    })
    _AttributeMap.update({
        __lang.name() : __lang
    })
Namespace.addCategoryObject('typeBinding', 'URI', URI)


# Complex type {http://purl.org/dc/terms/}ISO639-2 with content type SIMPLE
class ISO639_2 (pyxb.bundles.dc.dc.SimpleLiteral):
    """Complex type {http://purl.org/dc/terms/}ISO639-2 with content type SIMPLE"""
    _TypeDefinition = STD_ANON_9
    _ContentTypeTag = pyxb.binding.basis.complexTypeDefinition._CT_SIMPLE
    _Abstract = False
    _ExpandedName = pyxb.namespace.ExpandedName(Namespace, 'ISO639-2')
    _XSDLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/dc/schemas/dcterms.xsd', 251, 2)
    _ElementMap = pyxb.bundles.dc.dc.SimpleLiteral._ElementMap.copy()
    _AttributeMap = pyxb.bundles.dc.dc.SimpleLiteral._AttributeMap.copy()
    # Base type is pyxb.bundles.dc.dc.SimpleLiteral
    
    # Attribute lang is restricted from parent
    
    # Attribute {http://www.w3.org/XML/1998/namespace}lang uses Python identifier lang
    __lang = pyxb.binding.content.AttributeUse(pyxb.namespace.ExpandedName(pyxb.namespace.XML, 'lang'), 'lang', '__httppurl_orgdcelements1_1_SimpleLiteral_httpwww_w3_orgXML1998namespacelang', pyxb.binding.xml_.STD_ANON_lang, prohibited=True)
    __lang._DeclarationLocation = None
    __lang._UseLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/dc/schemas/dcterms.xsd', 257, 8)
    
    lang = property(__lang.value, __lang.set, None, None)

    _ElementMap.update({
        
    })
    _AttributeMap.update({
        __lang.name() : __lang
    })
Namespace.addCategoryObject('typeBinding', 'ISO639-2', ISO639_2)


# Complex type {http://purl.org/dc/terms/}ISO639-3 with content type SIMPLE
class ISO639_3 (pyxb.bundles.dc.dc.SimpleLiteral):
    """Complex type {http://purl.org/dc/terms/}ISO639-3 with content type SIMPLE"""
    _TypeDefinition = STD_ANON_10
    _ContentTypeTag = pyxb.binding.basis.complexTypeDefinition._CT_SIMPLE
    _Abstract = False
    _ExpandedName = pyxb.namespace.ExpandedName(Namespace, 'ISO639-3')
    _XSDLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/dc/schemas/dcterms.xsd', 262, 2)
    _ElementMap = pyxb.bundles.dc.dc.SimpleLiteral._ElementMap.copy()
    _AttributeMap = pyxb.bundles.dc.dc.SimpleLiteral._AttributeMap.copy()
    # Base type is pyxb.bundles.dc.dc.SimpleLiteral
    
    # Attribute lang is restricted from parent
    
    # Attribute {http://www.w3.org/XML/1998/namespace}lang uses Python identifier lang
    __lang = pyxb.binding.content.AttributeUse(pyxb.namespace.ExpandedName(pyxb.namespace.XML, 'lang'), 'lang', '__httppurl_orgdcelements1_1_SimpleLiteral_httpwww_w3_orgXML1998namespacelang', pyxb.binding.xml_.STD_ANON_lang, prohibited=True)
    __lang._DeclarationLocation = None
    __lang._UseLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/dc/schemas/dcterms.xsd', 268, 8)
    
    lang = property(__lang.value, __lang.set, None, None)

    _ElementMap.update({
        
    })
    _AttributeMap.update({
        __lang.name() : __lang
    })
Namespace.addCategoryObject('typeBinding', 'ISO639-3', ISO639_3)


# Complex type {http://purl.org/dc/terms/}RFC1766 with content type SIMPLE
class RFC1766 (pyxb.bundles.dc.dc.SimpleLiteral):
    """Complex type {http://purl.org/dc/terms/}RFC1766 with content type SIMPLE"""
    _TypeDefinition = STD_ANON_11
    _ContentTypeTag = pyxb.binding.basis.complexTypeDefinition._CT_SIMPLE
    _Abstract = False
    _ExpandedName = pyxb.namespace.ExpandedName(Namespace, 'RFC1766')
    _XSDLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/dc/schemas/dcterms.xsd', 273, 2)
    _ElementMap = pyxb.bundles.dc.dc.SimpleLiteral._ElementMap.copy()
    _AttributeMap = pyxb.bundles.dc.dc.SimpleLiteral._AttributeMap.copy()
    # Base type is pyxb.bundles.dc.dc.SimpleLiteral
    
    # Attribute lang is restricted from parent
    
    # Attribute {http://www.w3.org/XML/1998/namespace}lang uses Python identifier lang
    __lang = pyxb.binding.content.AttributeUse(pyxb.namespace.ExpandedName(pyxb.namespace.XML, 'lang'), 'lang', '__httppurl_orgdcelements1_1_SimpleLiteral_httpwww_w3_orgXML1998namespacelang', pyxb.binding.xml_.STD_ANON_lang, prohibited=True)
    __lang._DeclarationLocation = None
    __lang._UseLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/dc/schemas/dcterms.xsd', 279, 8)
    
    lang = property(__lang.value, __lang.set, None, None)

    _ElementMap.update({
        
    })
    _AttributeMap.update({
        __lang.name() : __lang
    })
Namespace.addCategoryObject('typeBinding', 'RFC1766', RFC1766)


# Complex type {http://purl.org/dc/terms/}RFC3066 with content type SIMPLE
class RFC3066 (pyxb.bundles.dc.dc.SimpleLiteral):
    """Complex type {http://purl.org/dc/terms/}RFC3066 with content type SIMPLE"""
    _TypeDefinition = STD_ANON_12
    _ContentTypeTag = pyxb.binding.basis.complexTypeDefinition._CT_SIMPLE
    _Abstract = False
    _ExpandedName = pyxb.namespace.ExpandedName(Namespace, 'RFC3066')
    _XSDLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/dc/schemas/dcterms.xsd', 284, 2)
    _ElementMap = pyxb.bundles.dc.dc.SimpleLiteral._ElementMap.copy()
    _AttributeMap = pyxb.bundles.dc.dc.SimpleLiteral._AttributeMap.copy()
    # Base type is pyxb.bundles.dc.dc.SimpleLiteral
    
    # Attribute lang is restricted from parent
    
    # Attribute {http://www.w3.org/XML/1998/namespace}lang uses Python identifier lang
    __lang = pyxb.binding.content.AttributeUse(pyxb.namespace.ExpandedName(pyxb.namespace.XML, 'lang'), 'lang', '__httppurl_orgdcelements1_1_SimpleLiteral_httpwww_w3_orgXML1998namespacelang', pyxb.binding.xml_.STD_ANON_lang, prohibited=True)
    __lang._DeclarationLocation = None
    __lang._UseLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/dc/schemas/dcterms.xsd', 290, 8)
    
    lang = property(__lang.value, __lang.set, None, None)

    _ElementMap.update({
        
    })
    _AttributeMap.update({
        __lang.name() : __lang
    })
Namespace.addCategoryObject('typeBinding', 'RFC3066', RFC3066)


# Complex type {http://purl.org/dc/terms/}RFC4646 with content type SIMPLE
class RFC4646 (pyxb.bundles.dc.dc.SimpleLiteral):
    """Complex type {http://purl.org/dc/terms/}RFC4646 with content type SIMPLE"""
    _TypeDefinition = STD_ANON_13
    _ContentTypeTag = pyxb.binding.basis.complexTypeDefinition._CT_SIMPLE
    _Abstract = False
    _ExpandedName = pyxb.namespace.ExpandedName(Namespace, 'RFC4646')
    _XSDLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/dc/schemas/dcterms.xsd', 295, 2)
    _ElementMap = pyxb.bundles.dc.dc.SimpleLiteral._ElementMap.copy()
    _AttributeMap = pyxb.bundles.dc.dc.SimpleLiteral._AttributeMap.copy()
    # Base type is pyxb.bundles.dc.dc.SimpleLiteral
    
    # Attribute lang is restricted from parent
    
    # Attribute {http://www.w3.org/XML/1998/namespace}lang uses Python identifier lang
    __lang = pyxb.binding.content.AttributeUse(pyxb.namespace.ExpandedName(pyxb.namespace.XML, 'lang'), 'lang', '__httppurl_orgdcelements1_1_SimpleLiteral_httpwww_w3_orgXML1998namespacelang', pyxb.binding.xml_.STD_ANON_lang, prohibited=True)
    __lang._DeclarationLocation = None
    __lang._UseLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/dc/schemas/dcterms.xsd', 301, 8)
    
    lang = property(__lang.value, __lang.set, None, None)

    _ElementMap.update({
        
    })
    _AttributeMap.update({
        __lang.name() : __lang
    })
Namespace.addCategoryObject('typeBinding', 'RFC4646', RFC4646)


# Complex type {http://purl.org/dc/terms/}Point with content type SIMPLE
class Point (pyxb.bundles.dc.dc.SimpleLiteral):
    """Complex type {http://purl.org/dc/terms/}Point with content type SIMPLE"""
    _TypeDefinition = STD_ANON_14
    _ContentTypeTag = pyxb.binding.basis.complexTypeDefinition._CT_SIMPLE
    _Abstract = False
    _ExpandedName = pyxb.namespace.ExpandedName(Namespace, 'Point')
    _XSDLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/dc/schemas/dcterms.xsd', 306, 2)
    _ElementMap = pyxb.bundles.dc.dc.SimpleLiteral._ElementMap.copy()
    _AttributeMap = pyxb.bundles.dc.dc.SimpleLiteral._AttributeMap.copy()
    # Base type is pyxb.bundles.dc.dc.SimpleLiteral
    
    # Attribute lang is restricted from parent
    
    # Attribute {http://www.w3.org/XML/1998/namespace}lang uses Python identifier lang
    __lang = pyxb.binding.content.AttributeUse(pyxb.namespace.ExpandedName(pyxb.namespace.XML, 'lang'), 'lang', '__httppurl_orgdcelements1_1_SimpleLiteral_httpwww_w3_orgXML1998namespacelang', pyxb.binding.xml_.STD_ANON_lang, prohibited=True)
    __lang._DeclarationLocation = None
    __lang._UseLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/dc/schemas/dcterms.xsd', 312, 8)
    
    lang = property(__lang.value, __lang.set, None, None)

    _ElementMap.update({
        
    })
    _AttributeMap.update({
        __lang.name() : __lang
    })
Namespace.addCategoryObject('typeBinding', 'Point', Point)


# Complex type {http://purl.org/dc/terms/}ISO3166 with content type SIMPLE
class ISO3166 (pyxb.bundles.dc.dc.SimpleLiteral):
    """Complex type {http://purl.org/dc/terms/}ISO3166 with content type SIMPLE"""
    _TypeDefinition = STD_ANON_15
    _ContentTypeTag = pyxb.binding.basis.complexTypeDefinition._CT_SIMPLE
    _Abstract = False
    _ExpandedName = pyxb.namespace.ExpandedName(Namespace, 'ISO3166')
    _XSDLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/dc/schemas/dcterms.xsd', 317, 2)
    _ElementMap = pyxb.bundles.dc.dc.SimpleLiteral._ElementMap.copy()
    _AttributeMap = pyxb.bundles.dc.dc.SimpleLiteral._AttributeMap.copy()
    # Base type is pyxb.bundles.dc.dc.SimpleLiteral
    
    # Attribute lang is restricted from parent
    
    # Attribute {http://www.w3.org/XML/1998/namespace}lang uses Python identifier lang
    __lang = pyxb.binding.content.AttributeUse(pyxb.namespace.ExpandedName(pyxb.namespace.XML, 'lang'), 'lang', '__httppurl_orgdcelements1_1_SimpleLiteral_httpwww_w3_orgXML1998namespacelang', pyxb.binding.xml_.STD_ANON_lang, prohibited=True)
    __lang._DeclarationLocation = None
    __lang._UseLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/dc/schemas/dcterms.xsd', 323, 8)
    
    lang = property(__lang.value, __lang.set, None, None)

    _ElementMap.update({
        
    })
    _AttributeMap.update({
        __lang.name() : __lang
    })
Namespace.addCategoryObject('typeBinding', 'ISO3166', ISO3166)


# Complex type {http://purl.org/dc/terms/}Box with content type SIMPLE
class Box (pyxb.bundles.dc.dc.SimpleLiteral):
    """Complex type {http://purl.org/dc/terms/}Box with content type SIMPLE"""
    _TypeDefinition = STD_ANON_16
    _ContentTypeTag = pyxb.binding.basis.complexTypeDefinition._CT_SIMPLE
    _Abstract = False
    _ExpandedName = pyxb.namespace.ExpandedName(Namespace, 'Box')
    _XSDLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/dc/schemas/dcterms.xsd', 328, 2)
    _ElementMap = pyxb.bundles.dc.dc.SimpleLiteral._ElementMap.copy()
    _AttributeMap = pyxb.bundles.dc.dc.SimpleLiteral._AttributeMap.copy()
    # Base type is pyxb.bundles.dc.dc.SimpleLiteral
    
    # Attribute lang is restricted from parent
    
    # Attribute {http://www.w3.org/XML/1998/namespace}lang uses Python identifier lang
    __lang = pyxb.binding.content.AttributeUse(pyxb.namespace.ExpandedName(pyxb.namespace.XML, 'lang'), 'lang', '__httppurl_orgdcelements1_1_SimpleLiteral_httpwww_w3_orgXML1998namespacelang', pyxb.binding.xml_.STD_ANON_lang, prohibited=True)
    __lang._DeclarationLocation = None
    __lang._UseLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/dc/schemas/dcterms.xsd', 334, 8)
    
    lang = property(__lang.value, __lang.set, None, None)

    _ElementMap.update({
        
    })
    _AttributeMap.update({
        __lang.name() : __lang
    })
Namespace.addCategoryObject('typeBinding', 'Box', Box)


# Complex type {http://purl.org/dc/terms/}TGN with content type SIMPLE
class TGN (pyxb.bundles.dc.dc.SimpleLiteral):
    """Complex type {http://purl.org/dc/terms/}TGN with content type SIMPLE"""
    _TypeDefinition = STD_ANON_17
    _ContentTypeTag = pyxb.binding.basis.complexTypeDefinition._CT_SIMPLE
    _Abstract = False
    _ExpandedName = pyxb.namespace.ExpandedName(Namespace, 'TGN')
    _XSDLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/dc/schemas/dcterms.xsd', 339, 2)
    _ElementMap = pyxb.bundles.dc.dc.SimpleLiteral._ElementMap.copy()
    _AttributeMap = pyxb.bundles.dc.dc.SimpleLiteral._AttributeMap.copy()
    # Base type is pyxb.bundles.dc.dc.SimpleLiteral
    
    # Attribute lang is restricted from parent
    
    # Attribute {http://www.w3.org/XML/1998/namespace}lang uses Python identifier lang
    __lang = pyxb.binding.content.AttributeUse(pyxb.namespace.ExpandedName(pyxb.namespace.XML, 'lang'), 'lang', '__httppurl_orgdcelements1_1_SimpleLiteral_httpwww_w3_orgXML1998namespacelang', pyxb.binding.xml_.STD_ANON_lang, prohibited=True)
    __lang._DeclarationLocation = None
    __lang._UseLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/dc/schemas/dcterms.xsd', 345, 8)
    
    lang = property(__lang.value, __lang.set, None, None)

    _ElementMap.update({
        
    })
    _AttributeMap.update({
        __lang.name() : __lang
    })
Namespace.addCategoryObject('typeBinding', 'TGN', TGN)


title = pyxb.binding.basis.element(pyxb.namespace.ExpandedName(Namespace, 'title'), pyxb.binding.datatypes.anyType, location=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/dc/schemas/dcterms.xsd', 75, 3))
Namespace.addCategoryObject('elementBinding', title.name().localName(), title)

creator = pyxb.binding.basis.element(pyxb.namespace.ExpandedName(Namespace, 'creator'), pyxb.binding.datatypes.anyType, location=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/dc/schemas/dcterms.xsd', 76, 3))
Namespace.addCategoryObject('elementBinding', creator.name().localName(), creator)

subject = pyxb.binding.basis.element(pyxb.namespace.ExpandedName(Namespace, 'subject'), pyxb.binding.datatypes.anyType, location=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/dc/schemas/dcterms.xsd', 77, 3))
Namespace.addCategoryObject('elementBinding', subject.name().localName(), subject)

description = pyxb.binding.basis.element(pyxb.namespace.ExpandedName(Namespace, 'description'), pyxb.binding.datatypes.anyType, location=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/dc/schemas/dcterms.xsd', 78, 3))
Namespace.addCategoryObject('elementBinding', description.name().localName(), description)

publisher = pyxb.binding.basis.element(pyxb.namespace.ExpandedName(Namespace, 'publisher'), pyxb.binding.datatypes.anyType, location=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/dc/schemas/dcterms.xsd', 79, 3))
Namespace.addCategoryObject('elementBinding', publisher.name().localName(), publisher)

contributor = pyxb.binding.basis.element(pyxb.namespace.ExpandedName(Namespace, 'contributor'), pyxb.binding.datatypes.anyType, location=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/dc/schemas/dcterms.xsd', 80, 3))
Namespace.addCategoryObject('elementBinding', contributor.name().localName(), contributor)

date = pyxb.binding.basis.element(pyxb.namespace.ExpandedName(Namespace, 'date'), pyxb.binding.datatypes.anyType, location=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/dc/schemas/dcterms.xsd', 81, 3))
Namespace.addCategoryObject('elementBinding', date.name().localName(), date)

type = pyxb.binding.basis.element(pyxb.namespace.ExpandedName(Namespace, 'type'), pyxb.binding.datatypes.anyType, location=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/dc/schemas/dcterms.xsd', 82, 3))
Namespace.addCategoryObject('elementBinding', type.name().localName(), type)

format = pyxb.binding.basis.element(pyxb.namespace.ExpandedName(Namespace, 'format'), pyxb.binding.datatypes.anyType, location=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/dc/schemas/dcterms.xsd', 83, 3))
Namespace.addCategoryObject('elementBinding', format.name().localName(), format)

identifier = pyxb.binding.basis.element(pyxb.namespace.ExpandedName(Namespace, 'identifier'), pyxb.binding.datatypes.anyType, location=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/dc/schemas/dcterms.xsd', 84, 3))
Namespace.addCategoryObject('elementBinding', identifier.name().localName(), identifier)

source = pyxb.binding.basis.element(pyxb.namespace.ExpandedName(Namespace, 'source'), pyxb.binding.datatypes.anyType, location=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/dc/schemas/dcterms.xsd', 85, 3))
Namespace.addCategoryObject('elementBinding', source.name().localName(), source)

language = pyxb.binding.basis.element(pyxb.namespace.ExpandedName(Namespace, 'language'), pyxb.binding.datatypes.anyType, location=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/dc/schemas/dcterms.xsd', 86, 3))
Namespace.addCategoryObject('elementBinding', language.name().localName(), language)

relation = pyxb.binding.basis.element(pyxb.namespace.ExpandedName(Namespace, 'relation'), pyxb.binding.datatypes.anyType, location=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/dc/schemas/dcterms.xsd', 87, 3))
Namespace.addCategoryObject('elementBinding', relation.name().localName(), relation)

coverage = pyxb.binding.basis.element(pyxb.namespace.ExpandedName(Namespace, 'coverage'), pyxb.binding.datatypes.anyType, location=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/dc/schemas/dcterms.xsd', 88, 3))
Namespace.addCategoryObject('elementBinding', coverage.name().localName(), coverage)

rights = pyxb.binding.basis.element(pyxb.namespace.ExpandedName(Namespace, 'rights'), pyxb.binding.datatypes.anyType, location=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/dc/schemas/dcterms.xsd', 89, 3))
Namespace.addCategoryObject('elementBinding', rights.name().localName(), rights)

alternative = pyxb.binding.basis.element(pyxb.namespace.ExpandedName(Namespace, 'alternative'), pyxb.binding.datatypes.anyType, location=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/dc/schemas/dcterms.xsd', 91, 3))
Namespace.addCategoryObject('elementBinding', alternative.name().localName(), alternative)

tableOfContents = pyxb.binding.basis.element(pyxb.namespace.ExpandedName(Namespace, 'tableOfContents'), pyxb.binding.datatypes.anyType, location=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/dc/schemas/dcterms.xsd', 93, 3))
Namespace.addCategoryObject('elementBinding', tableOfContents.name().localName(), tableOfContents)

abstract = pyxb.binding.basis.element(pyxb.namespace.ExpandedName(Namespace, 'abstract'), pyxb.binding.datatypes.anyType, location=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/dc/schemas/dcterms.xsd', 94, 3))
Namespace.addCategoryObject('elementBinding', abstract.name().localName(), abstract)

created = pyxb.binding.basis.element(pyxb.namespace.ExpandedName(Namespace, 'created'), pyxb.binding.datatypes.anyType, location=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/dc/schemas/dcterms.xsd', 96, 3))
Namespace.addCategoryObject('elementBinding', created.name().localName(), created)

valid = pyxb.binding.basis.element(pyxb.namespace.ExpandedName(Namespace, 'valid'), pyxb.binding.datatypes.anyType, location=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/dc/schemas/dcterms.xsd', 97, 3))
Namespace.addCategoryObject('elementBinding', valid.name().localName(), valid)

available = pyxb.binding.basis.element(pyxb.namespace.ExpandedName(Namespace, 'available'), pyxb.binding.datatypes.anyType, location=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/dc/schemas/dcterms.xsd', 98, 3))
Namespace.addCategoryObject('elementBinding', available.name().localName(), available)

issued = pyxb.binding.basis.element(pyxb.namespace.ExpandedName(Namespace, 'issued'), pyxb.binding.datatypes.anyType, location=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/dc/schemas/dcterms.xsd', 99, 3))
Namespace.addCategoryObject('elementBinding', issued.name().localName(), issued)

modified = pyxb.binding.basis.element(pyxb.namespace.ExpandedName(Namespace, 'modified'), pyxb.binding.datatypes.anyType, location=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/dc/schemas/dcterms.xsd', 100, 3))
Namespace.addCategoryObject('elementBinding', modified.name().localName(), modified)

dateAccepted = pyxb.binding.basis.element(pyxb.namespace.ExpandedName(Namespace, 'dateAccepted'), pyxb.binding.datatypes.anyType, location=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/dc/schemas/dcterms.xsd', 101, 3))
Namespace.addCategoryObject('elementBinding', dateAccepted.name().localName(), dateAccepted)

dateCopyrighted = pyxb.binding.basis.element(pyxb.namespace.ExpandedName(Namespace, 'dateCopyrighted'), pyxb.binding.datatypes.anyType, location=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/dc/schemas/dcterms.xsd', 102, 3))
Namespace.addCategoryObject('elementBinding', dateCopyrighted.name().localName(), dateCopyrighted)

dateSubmitted = pyxb.binding.basis.element(pyxb.namespace.ExpandedName(Namespace, 'dateSubmitted'), pyxb.binding.datatypes.anyType, location=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/dc/schemas/dcterms.xsd', 103, 3))
Namespace.addCategoryObject('elementBinding', dateSubmitted.name().localName(), dateSubmitted)

extent = pyxb.binding.basis.element(pyxb.namespace.ExpandedName(Namespace, 'extent'), pyxb.binding.datatypes.anyType, location=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/dc/schemas/dcterms.xsd', 105, 3))
Namespace.addCategoryObject('elementBinding', extent.name().localName(), extent)

medium = pyxb.binding.basis.element(pyxb.namespace.ExpandedName(Namespace, 'medium'), pyxb.binding.datatypes.anyType, location=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/dc/schemas/dcterms.xsd', 106, 3))
Namespace.addCategoryObject('elementBinding', medium.name().localName(), medium)

isVersionOf = pyxb.binding.basis.element(pyxb.namespace.ExpandedName(Namespace, 'isVersionOf'), pyxb.binding.datatypes.anyType, location=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/dc/schemas/dcterms.xsd', 108, 3))
Namespace.addCategoryObject('elementBinding', isVersionOf.name().localName(), isVersionOf)

hasVersion = pyxb.binding.basis.element(pyxb.namespace.ExpandedName(Namespace, 'hasVersion'), pyxb.binding.datatypes.anyType, location=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/dc/schemas/dcterms.xsd', 109, 3))
Namespace.addCategoryObject('elementBinding', hasVersion.name().localName(), hasVersion)

isReplacedBy = pyxb.binding.basis.element(pyxb.namespace.ExpandedName(Namespace, 'isReplacedBy'), pyxb.binding.datatypes.anyType, location=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/dc/schemas/dcterms.xsd', 110, 3))
Namespace.addCategoryObject('elementBinding', isReplacedBy.name().localName(), isReplacedBy)

replaces = pyxb.binding.basis.element(pyxb.namespace.ExpandedName(Namespace, 'replaces'), pyxb.binding.datatypes.anyType, location=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/dc/schemas/dcterms.xsd', 111, 3))
Namespace.addCategoryObject('elementBinding', replaces.name().localName(), replaces)

isRequiredBy = pyxb.binding.basis.element(pyxb.namespace.ExpandedName(Namespace, 'isRequiredBy'), pyxb.binding.datatypes.anyType, location=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/dc/schemas/dcterms.xsd', 112, 3))
Namespace.addCategoryObject('elementBinding', isRequiredBy.name().localName(), isRequiredBy)

requires = pyxb.binding.basis.element(pyxb.namespace.ExpandedName(Namespace, 'requires'), pyxb.binding.datatypes.anyType, location=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/dc/schemas/dcterms.xsd', 113, 3))
Namespace.addCategoryObject('elementBinding', requires.name().localName(), requires)

isPartOf = pyxb.binding.basis.element(pyxb.namespace.ExpandedName(Namespace, 'isPartOf'), pyxb.binding.datatypes.anyType, location=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/dc/schemas/dcterms.xsd', 114, 3))
Namespace.addCategoryObject('elementBinding', isPartOf.name().localName(), isPartOf)

hasPart = pyxb.binding.basis.element(pyxb.namespace.ExpandedName(Namespace, 'hasPart'), pyxb.binding.datatypes.anyType, location=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/dc/schemas/dcterms.xsd', 115, 3))
Namespace.addCategoryObject('elementBinding', hasPart.name().localName(), hasPart)

isReferencedBy = pyxb.binding.basis.element(pyxb.namespace.ExpandedName(Namespace, 'isReferencedBy'), pyxb.binding.datatypes.anyType, location=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/dc/schemas/dcterms.xsd', 116, 3))
Namespace.addCategoryObject('elementBinding', isReferencedBy.name().localName(), isReferencedBy)

references = pyxb.binding.basis.element(pyxb.namespace.ExpandedName(Namespace, 'references'), pyxb.binding.datatypes.anyType, location=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/dc/schemas/dcterms.xsd', 117, 3))
Namespace.addCategoryObject('elementBinding', references.name().localName(), references)

isFormatOf = pyxb.binding.basis.element(pyxb.namespace.ExpandedName(Namespace, 'isFormatOf'), pyxb.binding.datatypes.anyType, location=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/dc/schemas/dcterms.xsd', 118, 3))
Namespace.addCategoryObject('elementBinding', isFormatOf.name().localName(), isFormatOf)

hasFormat = pyxb.binding.basis.element(pyxb.namespace.ExpandedName(Namespace, 'hasFormat'), pyxb.binding.datatypes.anyType, location=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/dc/schemas/dcterms.xsd', 119, 3))
Namespace.addCategoryObject('elementBinding', hasFormat.name().localName(), hasFormat)

conformsTo = pyxb.binding.basis.element(pyxb.namespace.ExpandedName(Namespace, 'conformsTo'), pyxb.binding.datatypes.anyType, location=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/dc/schemas/dcterms.xsd', 120, 3))
Namespace.addCategoryObject('elementBinding', conformsTo.name().localName(), conformsTo)

spatial = pyxb.binding.basis.element(pyxb.namespace.ExpandedName(Namespace, 'spatial'), pyxb.binding.datatypes.anyType, location=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/dc/schemas/dcterms.xsd', 122, 3))
Namespace.addCategoryObject('elementBinding', spatial.name().localName(), spatial)

temporal = pyxb.binding.basis.element(pyxb.namespace.ExpandedName(Namespace, 'temporal'), pyxb.binding.datatypes.anyType, location=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/dc/schemas/dcterms.xsd', 123, 3))
Namespace.addCategoryObject('elementBinding', temporal.name().localName(), temporal)

audience = pyxb.binding.basis.element(pyxb.namespace.ExpandedName(Namespace, 'audience'), pyxb.binding.datatypes.anyType, location=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/dc/schemas/dcterms.xsd', 125, 3))
Namespace.addCategoryObject('elementBinding', audience.name().localName(), audience)

accrualMethod = pyxb.binding.basis.element(pyxb.namespace.ExpandedName(Namespace, 'accrualMethod'), pyxb.binding.datatypes.anyType, location=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/dc/schemas/dcterms.xsd', 126, 3))
Namespace.addCategoryObject('elementBinding', accrualMethod.name().localName(), accrualMethod)

accrualPeriodicity = pyxb.binding.basis.element(pyxb.namespace.ExpandedName(Namespace, 'accrualPeriodicity'), pyxb.binding.datatypes.anyType, location=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/dc/schemas/dcterms.xsd', 127, 3))
Namespace.addCategoryObject('elementBinding', accrualPeriodicity.name().localName(), accrualPeriodicity)

accrualPolicy = pyxb.binding.basis.element(pyxb.namespace.ExpandedName(Namespace, 'accrualPolicy'), pyxb.binding.datatypes.anyType, location=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/dc/schemas/dcterms.xsd', 128, 3))
Namespace.addCategoryObject('elementBinding', accrualPolicy.name().localName(), accrualPolicy)

instructionalMethod = pyxb.binding.basis.element(pyxb.namespace.ExpandedName(Namespace, 'instructionalMethod'), pyxb.binding.datatypes.anyType, location=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/dc/schemas/dcterms.xsd', 129, 3))
Namespace.addCategoryObject('elementBinding', instructionalMethod.name().localName(), instructionalMethod)

provenance = pyxb.binding.basis.element(pyxb.namespace.ExpandedName(Namespace, 'provenance'), pyxb.binding.datatypes.anyType, location=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/dc/schemas/dcterms.xsd', 130, 3))
Namespace.addCategoryObject('elementBinding', provenance.name().localName(), provenance)

rightsHolder = pyxb.binding.basis.element(pyxb.namespace.ExpandedName(Namespace, 'rightsHolder'), pyxb.binding.datatypes.anyType, location=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/dc/schemas/dcterms.xsd', 131, 3))
Namespace.addCategoryObject('elementBinding', rightsHolder.name().localName(), rightsHolder)

mediator = pyxb.binding.basis.element(pyxb.namespace.ExpandedName(Namespace, 'mediator'), pyxb.binding.datatypes.anyType, location=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/dc/schemas/dcterms.xsd', 133, 3))
Namespace.addCategoryObject('elementBinding', mediator.name().localName(), mediator)

educationLevel = pyxb.binding.basis.element(pyxb.namespace.ExpandedName(Namespace, 'educationLevel'), pyxb.binding.datatypes.anyType, location=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/dc/schemas/dcterms.xsd', 134, 3))
Namespace.addCategoryObject('elementBinding', educationLevel.name().localName(), educationLevel)

accessRights = pyxb.binding.basis.element(pyxb.namespace.ExpandedName(Namespace, 'accessRights'), pyxb.binding.datatypes.anyType, location=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/dc/schemas/dcterms.xsd', 136, 3))
Namespace.addCategoryObject('elementBinding', accessRights.name().localName(), accessRights)

license = pyxb.binding.basis.element(pyxb.namespace.ExpandedName(Namespace, 'license'), pyxb.binding.datatypes.anyType, location=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/dc/schemas/dcterms.xsd', 137, 3))
Namespace.addCategoryObject('elementBinding', license.name().localName(), license)

bibliographicCitation = pyxb.binding.basis.element(pyxb.namespace.ExpandedName(Namespace, 'bibliographicCitation'), pyxb.binding.datatypes.anyType, location=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/dc/schemas/dcterms.xsd', 139, 3))
Namespace.addCategoryObject('elementBinding', bibliographicCitation.name().localName(), bibliographicCitation)



elementOrRefinementContainer._AddElement(pyxb.binding.basis.element(pyxb.namespace.ExpandedName(_Namespace_dc, 'any'), pyxb.bundles.dc.dc.SimpleLiteral, abstract=pyxb.binding.datatypes.boolean(1), scope=elementOrRefinementContainer, location=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/dc/schemas/dc.xsd', 70, 2)))

def _BuildAutomaton ():
    # Remove this helper function from the namespace after it is invoked
    global _BuildAutomaton
    del _BuildAutomaton
    import pyxb.utils.fac as fac

    counters = set()
    cc_0 = fac.CounterCondition(min=0, max=None, metadata=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/dc/schemas/dcterms.xsd', 362, 4))
    counters.add(cc_0)
    states = []
    final_update = set()
    final_update.add(fac.UpdateInstruction(cc_0, False))
    symbol = pyxb.binding.content.ElementUse(elementOrRefinementContainer._UseForTag(pyxb.namespace.ExpandedName(_Namespace_dc, 'any')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/dc/schemas/dcterms.xsd', 363, 1))
    st_0 = fac.State(symbol, is_initial=True, final_update=final_update, is_unordered_catenation=False)
    states.append(st_0)
    transitions = []
    transitions.append(fac.Transition(st_0, [
        fac.UpdateInstruction(cc_0, True) ]))
    st_0._set_transitionSet(transitions)
    return fac.Automaton(states, counters, True, containing_state=None)
elementOrRefinementContainer._Automaton = _BuildAutomaton()


title._setSubstitutionGroup(pyxb.bundles.dc.dc.title)

creator._setSubstitutionGroup(pyxb.bundles.dc.dc.creator)

subject._setSubstitutionGroup(pyxb.bundles.dc.dc.subject)

description._setSubstitutionGroup(pyxb.bundles.dc.dc.description)

publisher._setSubstitutionGroup(pyxb.bundles.dc.dc.publisher)

contributor._setSubstitutionGroup(pyxb.bundles.dc.dc.contributor)

date._setSubstitutionGroup(pyxb.bundles.dc.dc.date)

type._setSubstitutionGroup(pyxb.bundles.dc.dc.type)

format._setSubstitutionGroup(pyxb.bundles.dc.dc.format)

identifier._setSubstitutionGroup(pyxb.bundles.dc.dc.identifier)

source._setSubstitutionGroup(pyxb.bundles.dc.dc.source)

language._setSubstitutionGroup(pyxb.bundles.dc.dc.language)

relation._setSubstitutionGroup(pyxb.bundles.dc.dc.relation)

coverage._setSubstitutionGroup(pyxb.bundles.dc.dc.coverage)

rights._setSubstitutionGroup(pyxb.bundles.dc.dc.rights)

alternative._setSubstitutionGroup(title)

tableOfContents._setSubstitutionGroup(description)

abstract._setSubstitutionGroup(description)

created._setSubstitutionGroup(date)

valid._setSubstitutionGroup(date)

available._setSubstitutionGroup(date)

issued._setSubstitutionGroup(date)

modified._setSubstitutionGroup(date)

dateAccepted._setSubstitutionGroup(date)

dateCopyrighted._setSubstitutionGroup(date)

dateSubmitted._setSubstitutionGroup(date)

extent._setSubstitutionGroup(format)

medium._setSubstitutionGroup(format)

isVersionOf._setSubstitutionGroup(relation)

hasVersion._setSubstitutionGroup(relation)

isReplacedBy._setSubstitutionGroup(relation)

replaces._setSubstitutionGroup(relation)

isRequiredBy._setSubstitutionGroup(relation)

requires._setSubstitutionGroup(relation)

isPartOf._setSubstitutionGroup(relation)

hasPart._setSubstitutionGroup(relation)

isReferencedBy._setSubstitutionGroup(relation)

references._setSubstitutionGroup(relation)

isFormatOf._setSubstitutionGroup(relation)

hasFormat._setSubstitutionGroup(relation)

conformsTo._setSubstitutionGroup(relation)

spatial._setSubstitutionGroup(coverage)

temporal._setSubstitutionGroup(coverage)

audience._setSubstitutionGroup(pyxb.bundles.dc.dc.any)

accrualMethod._setSubstitutionGroup(pyxb.bundles.dc.dc.any)

accrualPeriodicity._setSubstitutionGroup(pyxb.bundles.dc.dc.any)

accrualPolicy._setSubstitutionGroup(pyxb.bundles.dc.dc.any)

instructionalMethod._setSubstitutionGroup(pyxb.bundles.dc.dc.any)

provenance._setSubstitutionGroup(pyxb.bundles.dc.dc.any)

rightsHolder._setSubstitutionGroup(pyxb.bundles.dc.dc.any)

mediator._setSubstitutionGroup(audience)

educationLevel._setSubstitutionGroup(audience)

accessRights._setSubstitutionGroup(rights)

license._setSubstitutionGroup(rights)

bibliographicCitation._setSubstitutionGroup(identifier)
