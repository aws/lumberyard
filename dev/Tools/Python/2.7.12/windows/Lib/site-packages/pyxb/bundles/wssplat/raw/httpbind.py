# ./pyxb/bundles/wssplat/raw/httpbind.py
# -*- coding: utf-8 -*-
# PyXB bindings for NM:700d4ca8e0588f8b897f9c60d123e08fa4d56b36
# Generated 2014-10-19 06:24:56.250818 by PyXB version 1.2.4 using Python 2.7.3.final.0
# Namespace http://schemas.xmlsoap.org/wsdl/http/

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
_GenerationUID = pyxb.utils.utility.UniqueIdentifier('urn:uuid:8daca72c-5782-11e4-8649-c8600024e903')

# Version of PyXB used to generate the bindings
_PyXBVersion = '1.2.4'
# Generated bindings are not compatible across PyXB versions
if pyxb.__version__ != _PyXBVersion:
    raise pyxb.PyXBVersionError(_PyXBVersion)

# Import bindings for namespaces imported into schema
import pyxb.bundles.wssplat.wsdl11
import pyxb.binding.datatypes

# NOTE: All namespace declarations are reserved within the binding
Namespace = pyxb.namespace.NamespaceForURI('http://schemas.xmlsoap.org/wsdl/http/', create_if_missing=True)
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


# Complex type {http://schemas.xmlsoap.org/wsdl/http/}addressType with content type EMPTY
class addressType (pyxb.bundles.wssplat.wsdl11.tExtensibilityElement):
    """Complex type {http://schemas.xmlsoap.org/wsdl/http/}addressType with content type EMPTY"""
    _TypeDefinition = None
    _ContentTypeTag = pyxb.binding.basis.complexTypeDefinition._CT_EMPTY
    _Abstract = False
    _ExpandedName = pyxb.namespace.ExpandedName(Namespace, 'addressType')
    _XSDLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/httpbind.xsd', 42, 4)
    _ElementMap = pyxb.bundles.wssplat.wsdl11.tExtensibilityElement._ElementMap.copy()
    _AttributeMap = pyxb.bundles.wssplat.wsdl11.tExtensibilityElement._AttributeMap.copy()
    # Base type is pyxb.bundles.wssplat.wsdl11.tExtensibilityElement
    
    # Attribute required inherited from {http://schemas.xmlsoap.org/wsdl/}tExtensibilityElement
    
    # Attribute location uses Python identifier location
    __location = pyxb.binding.content.AttributeUse(pyxb.namespace.ExpandedName(None, 'location'), 'location', '__httpschemas_xmlsoap_orgwsdlhttp_addressType_location', pyxb.binding.datatypes.anyURI, required=True)
    __location._DeclarationLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/httpbind.xsd', 46, 6)
    __location._UseLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/httpbind.xsd', 46, 6)
    
    location = property(__location.value, __location.set, None, None)

    _ElementMap.update({
        
    })
    _AttributeMap.update({
        __location.name() : __location
    })
Namespace.addCategoryObject('typeBinding', 'addressType', addressType)


# Complex type {http://schemas.xmlsoap.org/wsdl/http/}bindingType with content type EMPTY
class bindingType (pyxb.bundles.wssplat.wsdl11.tExtensibilityElement):
    """Complex type {http://schemas.xmlsoap.org/wsdl/http/}bindingType with content type EMPTY"""
    _TypeDefinition = None
    _ContentTypeTag = pyxb.binding.basis.complexTypeDefinition._CT_EMPTY
    _Abstract = False
    _ExpandedName = pyxb.namespace.ExpandedName(Namespace, 'bindingType')
    _XSDLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/httpbind.xsd', 52, 4)
    _ElementMap = pyxb.bundles.wssplat.wsdl11.tExtensibilityElement._ElementMap.copy()
    _AttributeMap = pyxb.bundles.wssplat.wsdl11.tExtensibilityElement._AttributeMap.copy()
    # Base type is pyxb.bundles.wssplat.wsdl11.tExtensibilityElement
    
    # Attribute required inherited from {http://schemas.xmlsoap.org/wsdl/}tExtensibilityElement
    
    # Attribute verb uses Python identifier verb
    __verb = pyxb.binding.content.AttributeUse(pyxb.namespace.ExpandedName(None, 'verb'), 'verb', '__httpschemas_xmlsoap_orgwsdlhttp_bindingType_verb', pyxb.binding.datatypes.NMTOKEN, required=True)
    __verb._DeclarationLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/httpbind.xsd', 56, 9)
    __verb._UseLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/httpbind.xsd', 56, 9)
    
    verb = property(__verb.value, __verb.set, None, None)

    _ElementMap.update({
        
    })
    _AttributeMap.update({
        __verb.name() : __verb
    })
Namespace.addCategoryObject('typeBinding', 'bindingType', bindingType)


# Complex type {http://schemas.xmlsoap.org/wsdl/http/}operationType with content type EMPTY
class operationType (pyxb.bundles.wssplat.wsdl11.tExtensibilityElement):
    """Complex type {http://schemas.xmlsoap.org/wsdl/http/}operationType with content type EMPTY"""
    _TypeDefinition = None
    _ContentTypeTag = pyxb.binding.basis.complexTypeDefinition._CT_EMPTY
    _Abstract = False
    _ExpandedName = pyxb.namespace.ExpandedName(Namespace, 'operationType')
    _XSDLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/httpbind.xsd', 62, 4)
    _ElementMap = pyxb.bundles.wssplat.wsdl11.tExtensibilityElement._ElementMap.copy()
    _AttributeMap = pyxb.bundles.wssplat.wsdl11.tExtensibilityElement._AttributeMap.copy()
    # Base type is pyxb.bundles.wssplat.wsdl11.tExtensibilityElement
    
    # Attribute required inherited from {http://schemas.xmlsoap.org/wsdl/}tExtensibilityElement
    
    # Attribute location uses Python identifier location
    __location = pyxb.binding.content.AttributeUse(pyxb.namespace.ExpandedName(None, 'location'), 'location', '__httpschemas_xmlsoap_orgwsdlhttp_operationType_location', pyxb.binding.datatypes.anyURI, required=True)
    __location._DeclarationLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/httpbind.xsd', 66, 9)
    __location._UseLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/httpbind.xsd', 66, 9)
    
    location = property(__location.value, __location.set, None, None)

    _ElementMap.update({
        
    })
    _AttributeMap.update({
        __location.name() : __location
    })
Namespace.addCategoryObject('typeBinding', 'operationType', operationType)


# Complex type [anonymous] with content type EMPTY
class CTD_ANON (pyxb.binding.basis.complexTypeDefinition):
    """Complex type [anonymous] with content type EMPTY"""
    _TypeDefinition = None
    _ContentTypeTag = pyxb.binding.basis.complexTypeDefinition._CT_EMPTY
    _Abstract = False
    _ExpandedName = None
    _XSDLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/httpbind.xsd', 72, 8)
    _ElementMap = {}
    _AttributeMap = {}
    # Base type is pyxb.binding.datatypes.anyType
    _ElementMap.update({
        
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
    _XSDLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/httpbind.xsd', 75, 8)
    _ElementMap = {}
    _AttributeMap = {}
    # Base type is pyxb.binding.datatypes.anyType
    _ElementMap.update({
        
    })
    _AttributeMap.update({
        
    })



address = pyxb.binding.basis.element(pyxb.namespace.ExpandedName(Namespace, 'address'), addressType, location=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/httpbind.xsd', 40, 4))
Namespace.addCategoryObject('elementBinding', address.name().localName(), address)

binding = pyxb.binding.basis.element(pyxb.namespace.ExpandedName(Namespace, 'binding'), bindingType, location=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/httpbind.xsd', 51, 4))
Namespace.addCategoryObject('elementBinding', binding.name().localName(), binding)

operation = pyxb.binding.basis.element(pyxb.namespace.ExpandedName(Namespace, 'operation'), operationType, location=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/httpbind.xsd', 61, 4))
Namespace.addCategoryObject('elementBinding', operation.name().localName(), operation)

urlEncoded = pyxb.binding.basis.element(pyxb.namespace.ExpandedName(Namespace, 'urlEncoded'), CTD_ANON, location=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/httpbind.xsd', 71, 4))
Namespace.addCategoryObject('elementBinding', urlEncoded.name().localName(), urlEncoded)

urlReplacement = pyxb.binding.basis.element(pyxb.namespace.ExpandedName(Namespace, 'urlReplacement'), CTD_ANON_, location=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/httpbind.xsd', 74, 4))
Namespace.addCategoryObject('elementBinding', urlReplacement.name().localName(), urlReplacement)
