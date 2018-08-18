# ./pyxb/bundles/wssplat/raw/soapbind12.py
# -*- coding: utf-8 -*-
# PyXB bindings for NM:fe6dee6f975d5202c12c3e364c8df804f68deab8
# Generated 2014-10-19 06:24:57.422267 by PyXB version 1.2.4 using Python 2.7.3.final.0
# Namespace http://schemas.xmlsoap.org/wsdl/soap12/

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
_GenerationUID = pyxb.utils.utility.UniqueIdentifier('urn:uuid:8e5d58ec-5782-11e4-9c84-c8600024e903')

# Version of PyXB used to generate the bindings
_PyXBVersion = '1.2.4'
# Generated bindings are not compatible across PyXB versions
if pyxb.__version__ != _PyXBVersion:
    raise pyxb.PyXBVersionError(_PyXBVersion)

# Import bindings for namespaces imported into schema
import pyxb.bundles.wssplat.wsdl11
import pyxb.binding.datatypes

# NOTE: All namespace declarations are reserved within the binding
Namespace = pyxb.namespace.NamespaceForURI('http://schemas.xmlsoap.org/wsdl/soap12/', create_if_missing=True)
Namespace.configureCategories(['typeBinding', 'elementBinding'])
_Namespace_wsdl = pyxb.bundles.wssplat.wsdl11.Namespace
_Namespace_wsdl.configureCategories(['typeBinding', 'elementBinding'])

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


# Atomic simple type: {http://schemas.xmlsoap.org/wsdl/soap12/}tStyleChoice
class tStyleChoice (pyxb.binding.datatypes.string, pyxb.binding.basis.enumeration_mixin):

    """An atomic simple type."""

    _ExpandedName = pyxb.namespace.ExpandedName(Namespace, 'tStyleChoice')
    _XSDLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/soapbind12.xsd', 46, 2)
    _Documentation = None
tStyleChoice._CF_enumeration = pyxb.binding.facets.CF_enumeration(value_datatype=tStyleChoice, enum_prefix=None)
tStyleChoice.rpc = tStyleChoice._CF_enumeration.addEnumeration(unicode_value='rpc', tag='rpc')
tStyleChoice.document = tStyleChoice._CF_enumeration.addEnumeration(unicode_value='document', tag='document')
tStyleChoice._InitializeFacetMap(tStyleChoice._CF_enumeration)
Namespace.addCategoryObject('typeBinding', 'tStyleChoice', tStyleChoice)

# List simple type: {http://schemas.xmlsoap.org/wsdl/soap12/}tParts
# superclasses pyxb.binding.datatypes.anySimpleType
class tParts (pyxb.binding.basis.STD_list):

    """Simple type that is a list of pyxb.binding.datatypes.NMTOKEN."""

    _ExpandedName = pyxb.namespace.ExpandedName(Namespace, 'tParts')
    _XSDLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/soapbind12.xsd', 71, 2)
    _Documentation = None

    _ItemType = pyxb.binding.datatypes.NMTOKEN
tParts._InitializeFacetMap()
Namespace.addCategoryObject('typeBinding', 'tParts', tParts)

# Atomic simple type: {http://schemas.xmlsoap.org/wsdl/soap12/}useChoice
class useChoice (pyxb.binding.datatypes.string, pyxb.binding.basis.enumeration_mixin):

    """An atomic simple type."""

    _ExpandedName = pyxb.namespace.ExpandedName(Namespace, 'useChoice')
    _XSDLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/soapbind12.xsd', 83, 2)
    _Documentation = None
useChoice._CF_enumeration = pyxb.binding.facets.CF_enumeration(value_datatype=useChoice, enum_prefix=None)
useChoice.literal = useChoice._CF_enumeration.addEnumeration(unicode_value='literal', tag='literal')
useChoice.encoded = useChoice._CF_enumeration.addEnumeration(unicode_value='encoded', tag='encoded')
useChoice._InitializeFacetMap(useChoice._CF_enumeration)
Namespace.addCategoryObject('typeBinding', 'useChoice', useChoice)

# Complex type {http://schemas.xmlsoap.org/wsdl/soap12/}tExtensibilityElementOpenAttrs with content type EMPTY
class tExtensibilityElementOpenAttrs (pyxb.bundles.wssplat.wsdl11.tExtensibilityElement):
    """Complex type {http://schemas.xmlsoap.org/wsdl/soap12/}tExtensibilityElementOpenAttrs with content type EMPTY"""
    _TypeDefinition = None
    _ContentTypeTag = pyxb.binding.basis.complexTypeDefinition._CT_EMPTY
    _Abstract = False
    _ExpandedName = pyxb.namespace.ExpandedName(Namespace, 'tExtensibilityElementOpenAttrs')
    _XSDLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/soapbind12.xsd', 28, 2)
    _ElementMap = pyxb.bundles.wssplat.wsdl11.tExtensibilityElement._ElementMap.copy()
    _AttributeMap = pyxb.bundles.wssplat.wsdl11.tExtensibilityElement._AttributeMap.copy()
    # Base type is pyxb.bundles.wssplat.wsdl11.tExtensibilityElement
    
    # Attribute required inherited from {http://schemas.xmlsoap.org/wsdl/}tExtensibilityElement
    _AttributeWildcard = pyxb.binding.content.Wildcard(process_contents=pyxb.binding.content.Wildcard.PC_lax, namespace_constraint=(pyxb.binding.content.Wildcard.NC_not, 'http://schemas.xmlsoap.org/wsdl/soap12/'))
    _ElementMap.update({
        
    })
    _AttributeMap.update({
        
    })
Namespace.addCategoryObject('typeBinding', 'tExtensibilityElementOpenAttrs', tExtensibilityElementOpenAttrs)


# Complex type {http://schemas.xmlsoap.org/wsdl/soap12/}tBinding with content type EMPTY
class tBinding (tExtensibilityElementOpenAttrs):
    """Complex type {http://schemas.xmlsoap.org/wsdl/soap12/}tBinding with content type EMPTY"""
    _TypeDefinition = None
    _ContentTypeTag = pyxb.binding.basis.complexTypeDefinition._CT_EMPTY
    _Abstract = False
    _ExpandedName = pyxb.namespace.ExpandedName(Namespace, 'tBinding')
    _XSDLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/soapbind12.xsd', 37, 2)
    _ElementMap = tExtensibilityElementOpenAttrs._ElementMap.copy()
    _AttributeMap = tExtensibilityElementOpenAttrs._AttributeMap.copy()
    # Base type is tExtensibilityElementOpenAttrs
    
    # Attribute required inherited from {http://schemas.xmlsoap.org/wsdl/}tExtensibilityElement
    
    # Attribute transport uses Python identifier transport
    __transport = pyxb.binding.content.AttributeUse(pyxb.namespace.ExpandedName(None, 'transport'), 'transport', '__httpschemas_xmlsoap_orgwsdlsoap12_tBinding_transport', pyxb.binding.datatypes.anyURI, required=True)
    __transport._DeclarationLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/soapbind12.xsd', 40, 8)
    __transport._UseLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/soapbind12.xsd', 40, 8)
    
    transport = property(__transport.value, __transport.set, None, None)

    
    # Attribute style uses Python identifier style
    __style = pyxb.binding.content.AttributeUse(pyxb.namespace.ExpandedName(None, 'style'), 'style', '__httpschemas_xmlsoap_orgwsdlsoap12_tBinding_style', tStyleChoice)
    __style._DeclarationLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/soapbind12.xsd', 41, 8)
    __style._UseLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/soapbind12.xsd', 41, 8)
    
    style = property(__style.value, __style.set, None, None)

    _AttributeWildcard = pyxb.binding.content.Wildcard(process_contents=pyxb.binding.content.Wildcard.PC_lax, namespace_constraint=(pyxb.binding.content.Wildcard.NC_not, 'http://schemas.xmlsoap.org/wsdl/soap12/'))
    _ElementMap.update({
        
    })
    _AttributeMap.update({
        __transport.name() : __transport,
        __style.name() : __style
    })
Namespace.addCategoryObject('typeBinding', 'tBinding', tBinding)


# Complex type {http://schemas.xmlsoap.org/wsdl/soap12/}tOperation with content type EMPTY
class tOperation (tExtensibilityElementOpenAttrs):
    """Complex type {http://schemas.xmlsoap.org/wsdl/soap12/}tOperation with content type EMPTY"""
    _TypeDefinition = None
    _ContentTypeTag = pyxb.binding.basis.complexTypeDefinition._CT_EMPTY
    _Abstract = False
    _ExpandedName = pyxb.namespace.ExpandedName(Namespace, 'tOperation')
    _XSDLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/soapbind12.xsd', 54, 2)
    _ElementMap = tExtensibilityElementOpenAttrs._ElementMap.copy()
    _AttributeMap = tExtensibilityElementOpenAttrs._AttributeMap.copy()
    # Base type is tExtensibilityElementOpenAttrs
    
    # Attribute required inherited from {http://schemas.xmlsoap.org/wsdl/}tExtensibilityElement
    
    # Attribute soapAction uses Python identifier soapAction
    __soapAction = pyxb.binding.content.AttributeUse(pyxb.namespace.ExpandedName(None, 'soapAction'), 'soapAction', '__httpschemas_xmlsoap_orgwsdlsoap12_tOperation_soapAction', pyxb.binding.datatypes.anyURI)
    __soapAction._DeclarationLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/soapbind12.xsd', 57, 8)
    __soapAction._UseLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/soapbind12.xsd', 57, 8)
    
    soapAction = property(__soapAction.value, __soapAction.set, None, None)

    
    # Attribute soapActionRequired uses Python identifier soapActionRequired
    __soapActionRequired = pyxb.binding.content.AttributeUse(pyxb.namespace.ExpandedName(None, 'soapActionRequired'), 'soapActionRequired', '__httpschemas_xmlsoap_orgwsdlsoap12_tOperation_soapActionRequired', pyxb.binding.datatypes.boolean)
    __soapActionRequired._DeclarationLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/soapbind12.xsd', 58, 8)
    __soapActionRequired._UseLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/soapbind12.xsd', 58, 8)
    
    soapActionRequired = property(__soapActionRequired.value, __soapActionRequired.set, None, None)

    
    # Attribute style uses Python identifier style
    __style = pyxb.binding.content.AttributeUse(pyxb.namespace.ExpandedName(None, 'style'), 'style', '__httpschemas_xmlsoap_orgwsdlsoap12_tOperation_style', tStyleChoice)
    __style._DeclarationLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/soapbind12.xsd', 59, 8)
    __style._UseLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/soapbind12.xsd', 59, 8)
    
    style = property(__style.value, __style.set, None, None)

    _AttributeWildcard = pyxb.binding.content.Wildcard(process_contents=pyxb.binding.content.Wildcard.PC_lax, namespace_constraint=(pyxb.binding.content.Wildcard.NC_not, 'http://schemas.xmlsoap.org/wsdl/soap12/'))
    _ElementMap.update({
        
    })
    _AttributeMap.update({
        __soapAction.name() : __soapAction,
        __soapActionRequired.name() : __soapActionRequired,
        __style.name() : __style
    })
Namespace.addCategoryObject('typeBinding', 'tOperation', tOperation)


# Complex type {http://schemas.xmlsoap.org/wsdl/soap12/}tBody with content type EMPTY
class tBody (tExtensibilityElementOpenAttrs):
    """Complex type {http://schemas.xmlsoap.org/wsdl/soap12/}tBody with content type EMPTY"""
    _TypeDefinition = None
    _ContentTypeTag = pyxb.binding.basis.complexTypeDefinition._CT_EMPTY
    _Abstract = False
    _ExpandedName = pyxb.namespace.ExpandedName(Namespace, 'tBody')
    _XSDLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/soapbind12.xsd', 74, 2)
    _ElementMap = tExtensibilityElementOpenAttrs._ElementMap.copy()
    _AttributeMap = tExtensibilityElementOpenAttrs._AttributeMap.copy()
    # Base type is tExtensibilityElementOpenAttrs
    
    # Attribute required inherited from {http://schemas.xmlsoap.org/wsdl/}tExtensibilityElement
    
    # Attribute encodingStyle uses Python identifier encodingStyle
    __encodingStyle = pyxb.binding.content.AttributeUse(pyxb.namespace.ExpandedName(None, 'encodingStyle'), 'encodingStyle', '__httpschemas_xmlsoap_orgwsdlsoap12_tBody_encodingStyle', pyxb.binding.datatypes.anyURI)
    __encodingStyle._DeclarationLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/soapbind12.xsd', 67, 4)
    __encodingStyle._UseLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/soapbind12.xsd', 67, 4)
    
    encodingStyle = property(__encodingStyle.value, __encodingStyle.set, None, None)

    
    # Attribute use uses Python identifier use
    __use = pyxb.binding.content.AttributeUse(pyxb.namespace.ExpandedName(None, 'use'), 'use', '__httpschemas_xmlsoap_orgwsdlsoap12_tBody_use', useChoice)
    __use._DeclarationLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/soapbind12.xsd', 68, 4)
    __use._UseLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/soapbind12.xsd', 68, 4)
    
    use = property(__use.value, __use.set, None, None)

    
    # Attribute namespace uses Python identifier namespace
    __namespace = pyxb.binding.content.AttributeUse(pyxb.namespace.ExpandedName(None, 'namespace'), 'namespace', '__httpschemas_xmlsoap_orgwsdlsoap12_tBody_namespace', pyxb.binding.datatypes.anyURI)
    __namespace._DeclarationLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/soapbind12.xsd', 69, 4)
    __namespace._UseLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/soapbind12.xsd', 69, 4)
    
    namespace = property(__namespace.value, __namespace.set, None, None)

    
    # Attribute parts uses Python identifier parts
    __parts = pyxb.binding.content.AttributeUse(pyxb.namespace.ExpandedName(None, 'parts'), 'parts', '__httpschemas_xmlsoap_orgwsdlsoap12_tBody_parts', tParts)
    __parts._DeclarationLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/soapbind12.xsd', 77, 8)
    __parts._UseLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/soapbind12.xsd', 77, 8)
    
    parts = property(__parts.value, __parts.set, None, None)

    _AttributeWildcard = pyxb.binding.content.Wildcard(process_contents=pyxb.binding.content.Wildcard.PC_lax, namespace_constraint=(pyxb.binding.content.Wildcard.NC_not, 'http://schemas.xmlsoap.org/wsdl/soap12/'))
    _ElementMap.update({
        
    })
    _AttributeMap.update({
        __encodingStyle.name() : __encodingStyle,
        __use.name() : __use,
        __namespace.name() : __namespace,
        __parts.name() : __parts
    })
Namespace.addCategoryObject('typeBinding', 'tBody', tBody)


# Complex type {http://schemas.xmlsoap.org/wsdl/soap12/}tHeader with content type ELEMENT_ONLY
class tHeader (tExtensibilityElementOpenAttrs):
    """Complex type {http://schemas.xmlsoap.org/wsdl/soap12/}tHeader with content type ELEMENT_ONLY"""
    _TypeDefinition = None
    _ContentTypeTag = pyxb.binding.basis.complexTypeDefinition._CT_ELEMENT_ONLY
    _Abstract = False
    _ExpandedName = pyxb.namespace.ExpandedName(Namespace, 'tHeader')
    _XSDLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/soapbind12.xsd', 117, 2)
    _ElementMap = tExtensibilityElementOpenAttrs._ElementMap.copy()
    _AttributeMap = tExtensibilityElementOpenAttrs._AttributeMap.copy()
    # Base type is tExtensibilityElementOpenAttrs
    
    # Element {http://schemas.xmlsoap.org/wsdl/soap12/}headerfault uses Python identifier headerfault
    __headerfault = pyxb.binding.content.ElementDeclaration(pyxb.namespace.ExpandedName(Namespace, 'headerfault'), 'headerfault', '__httpschemas_xmlsoap_orgwsdlsoap12_tHeader_httpschemas_xmlsoap_orgwsdlsoap12headerfault', True, pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/soapbind12.xsd', 128, 2), )

    
    headerfault = property(__headerfault.value, __headerfault.set, None, None)

    
    # Attribute required inherited from {http://schemas.xmlsoap.org/wsdl/}tExtensibilityElement
    
    # Attribute message uses Python identifier message
    __message = pyxb.binding.content.AttributeUse(pyxb.namespace.ExpandedName(None, 'message'), 'message', '__httpschemas_xmlsoap_orgwsdlsoap12_tHeader_message', pyxb.binding.datatypes.QName, required=True)
    __message._DeclarationLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/soapbind12.xsd', 111, 4)
    __message._UseLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/soapbind12.xsd', 111, 4)
    
    message = property(__message.value, __message.set, None, None)

    
    # Attribute part uses Python identifier part
    __part = pyxb.binding.content.AttributeUse(pyxb.namespace.ExpandedName(None, 'part'), 'part', '__httpschemas_xmlsoap_orgwsdlsoap12_tHeader_part', pyxb.binding.datatypes.NMTOKEN, required=True)
    __part._DeclarationLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/soapbind12.xsd', 112, 4)
    __part._UseLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/soapbind12.xsd', 112, 4)
    
    part = property(__part.value, __part.set, None, None)

    
    # Attribute use uses Python identifier use
    __use = pyxb.binding.content.AttributeUse(pyxb.namespace.ExpandedName(None, 'use'), 'use', '__httpschemas_xmlsoap_orgwsdlsoap12_tHeader_use', useChoice, required=True)
    __use._DeclarationLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/soapbind12.xsd', 113, 4)
    __use._UseLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/soapbind12.xsd', 113, 4)
    
    use = property(__use.value, __use.set, None, None)

    
    # Attribute encodingStyle uses Python identifier encodingStyle
    __encodingStyle = pyxb.binding.content.AttributeUse(pyxb.namespace.ExpandedName(None, 'encodingStyle'), 'encodingStyle', '__httpschemas_xmlsoap_orgwsdlsoap12_tHeader_encodingStyle', pyxb.binding.datatypes.anyURI)
    __encodingStyle._DeclarationLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/soapbind12.xsd', 114, 4)
    __encodingStyle._UseLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/soapbind12.xsd', 114, 4)
    
    encodingStyle = property(__encodingStyle.value, __encodingStyle.set, None, None)

    
    # Attribute namespace uses Python identifier namespace
    __namespace = pyxb.binding.content.AttributeUse(pyxb.namespace.ExpandedName(None, 'namespace'), 'namespace', '__httpschemas_xmlsoap_orgwsdlsoap12_tHeader_namespace', pyxb.binding.datatypes.anyURI)
    __namespace._DeclarationLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/soapbind12.xsd', 115, 4)
    __namespace._UseLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/soapbind12.xsd', 115, 4)
    
    namespace = property(__namespace.value, __namespace.set, None, None)

    _AttributeWildcard = pyxb.binding.content.Wildcard(process_contents=pyxb.binding.content.Wildcard.PC_lax, namespace_constraint=(pyxb.binding.content.Wildcard.NC_not, 'http://schemas.xmlsoap.org/wsdl/soap12/'))
    _ElementMap.update({
        __headerfault.name() : __headerfault
    })
    _AttributeMap.update({
        __message.name() : __message,
        __part.name() : __part,
        __use.name() : __use,
        __encodingStyle.name() : __encodingStyle,
        __namespace.name() : __namespace
    })
Namespace.addCategoryObject('typeBinding', 'tHeader', tHeader)


# Complex type {http://schemas.xmlsoap.org/wsdl/soap12/}tHeaderFault with content type EMPTY
class tHeaderFault (pyxb.binding.basis.complexTypeDefinition):
    """Complex type {http://schemas.xmlsoap.org/wsdl/soap12/}tHeaderFault with content type EMPTY"""
    _TypeDefinition = None
    _ContentTypeTag = pyxb.binding.basis.complexTypeDefinition._CT_EMPTY
    _Abstract = False
    _ExpandedName = pyxb.namespace.ExpandedName(Namespace, 'tHeaderFault')
    _XSDLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/soapbind12.xsd', 129, 2)
    _ElementMap = {}
    _AttributeMap = {}
    # Base type is pyxb.binding.datatypes.anyType
    
    # Attribute message uses Python identifier message
    __message = pyxb.binding.content.AttributeUse(pyxb.namespace.ExpandedName(None, 'message'), 'message', '__httpschemas_xmlsoap_orgwsdlsoap12_tHeaderFault_message', pyxb.binding.datatypes.QName, required=True)
    __message._DeclarationLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/soapbind12.xsd', 111, 4)
    __message._UseLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/soapbind12.xsd', 111, 4)
    
    message = property(__message.value, __message.set, None, None)

    
    # Attribute part uses Python identifier part
    __part = pyxb.binding.content.AttributeUse(pyxb.namespace.ExpandedName(None, 'part'), 'part', '__httpschemas_xmlsoap_orgwsdlsoap12_tHeaderFault_part', pyxb.binding.datatypes.NMTOKEN, required=True)
    __part._DeclarationLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/soapbind12.xsd', 112, 4)
    __part._UseLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/soapbind12.xsd', 112, 4)
    
    part = property(__part.value, __part.set, None, None)

    
    # Attribute use uses Python identifier use
    __use = pyxb.binding.content.AttributeUse(pyxb.namespace.ExpandedName(None, 'use'), 'use', '__httpschemas_xmlsoap_orgwsdlsoap12_tHeaderFault_use', useChoice, required=True)
    __use._DeclarationLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/soapbind12.xsd', 113, 4)
    __use._UseLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/soapbind12.xsd', 113, 4)
    
    use = property(__use.value, __use.set, None, None)

    
    # Attribute encodingStyle uses Python identifier encodingStyle
    __encodingStyle = pyxb.binding.content.AttributeUse(pyxb.namespace.ExpandedName(None, 'encodingStyle'), 'encodingStyle', '__httpschemas_xmlsoap_orgwsdlsoap12_tHeaderFault_encodingStyle', pyxb.binding.datatypes.anyURI)
    __encodingStyle._DeclarationLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/soapbind12.xsd', 114, 4)
    __encodingStyle._UseLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/soapbind12.xsd', 114, 4)
    
    encodingStyle = property(__encodingStyle.value, __encodingStyle.set, None, None)

    
    # Attribute namespace uses Python identifier namespace
    __namespace = pyxb.binding.content.AttributeUse(pyxb.namespace.ExpandedName(None, 'namespace'), 'namespace', '__httpschemas_xmlsoap_orgwsdlsoap12_tHeaderFault_namespace', pyxb.binding.datatypes.anyURI)
    __namespace._DeclarationLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/soapbind12.xsd', 115, 4)
    __namespace._UseLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/soapbind12.xsd', 115, 4)
    
    namespace = property(__namespace.value, __namespace.set, None, None)

    _AttributeWildcard = pyxb.binding.content.Wildcard(process_contents=pyxb.binding.content.Wildcard.PC_lax, namespace_constraint=(pyxb.binding.content.Wildcard.NC_not, 'http://schemas.xmlsoap.org/wsdl/soap12/'))
    _ElementMap.update({
        
    })
    _AttributeMap.update({
        __message.name() : __message,
        __part.name() : __part,
        __use.name() : __use,
        __encodingStyle.name() : __encodingStyle,
        __namespace.name() : __namespace
    })
Namespace.addCategoryObject('typeBinding', 'tHeaderFault', tHeaderFault)


# Complex type {http://schemas.xmlsoap.org/wsdl/soap12/}tAddress with content type EMPTY
class tAddress (tExtensibilityElementOpenAttrs):
    """Complex type {http://schemas.xmlsoap.org/wsdl/soap12/}tAddress with content type EMPTY"""
    _TypeDefinition = None
    _ContentTypeTag = pyxb.binding.basis.complexTypeDefinition._CT_EMPTY
    _Abstract = False
    _ExpandedName = pyxb.namespace.ExpandedName(Namespace, 'tAddress')
    _XSDLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/soapbind12.xsd', 135, 2)
    _ElementMap = tExtensibilityElementOpenAttrs._ElementMap.copy()
    _AttributeMap = tExtensibilityElementOpenAttrs._AttributeMap.copy()
    # Base type is tExtensibilityElementOpenAttrs
    
    # Attribute required inherited from {http://schemas.xmlsoap.org/wsdl/}tExtensibilityElement
    
    # Attribute location uses Python identifier location
    __location = pyxb.binding.content.AttributeUse(pyxb.namespace.ExpandedName(None, 'location'), 'location', '__httpschemas_xmlsoap_orgwsdlsoap12_tAddress_location', pyxb.binding.datatypes.anyURI, required=True)
    __location._DeclarationLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/soapbind12.xsd', 138, 8)
    __location._UseLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/soapbind12.xsd', 138, 8)
    
    location = property(__location.value, __location.set, None, None)

    _AttributeWildcard = pyxb.binding.content.Wildcard(process_contents=pyxb.binding.content.Wildcard.PC_lax, namespace_constraint=(pyxb.binding.content.Wildcard.NC_not, 'http://schemas.xmlsoap.org/wsdl/soap12/'))
    _ElementMap.update({
        
    })
    _AttributeMap.update({
        __location.name() : __location
    })
Namespace.addCategoryObject('typeBinding', 'tAddress', tAddress)


# Complex type {http://schemas.xmlsoap.org/wsdl/soap12/}tFaultRes with content type EMPTY
class tFaultRes (tBody):
    """Complex type {http://schemas.xmlsoap.org/wsdl/soap12/}tFaultRes with content type EMPTY"""
    _TypeDefinition = None
    _ContentTypeTag = pyxb.binding.basis.complexTypeDefinition._CT_EMPTY
    _Abstract = True
    _ExpandedName = pyxb.namespace.ExpandedName(Namespace, 'tFaultRes')
    _XSDLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/soapbind12.xsd', 91, 2)
    _ElementMap = tBody._ElementMap.copy()
    _AttributeMap = tBody._AttributeMap.copy()
    # Base type is tBody
    
    # Attribute required is restricted from parent
    
    # Attribute {http://schemas.xmlsoap.org/wsdl/}required uses Python identifier required
    __required = pyxb.binding.content.AttributeUse(pyxb.namespace.ExpandedName(_Namespace_wsdl, 'required'), 'required', '__httpschemas_xmlsoap_orgwsdl_tExtensibilityElement_httpschemas_xmlsoap_orgwsdlrequired', pyxb.binding.datatypes.boolean)
    __required._DeclarationLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wsdl11.xsd', 305, 2)
    __required._UseLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/soapbind12.xsd', 94, 5)
    
    required = property(__required.value, __required.set, None, None)

    
    # Attribute encodingStyle is restricted from parent
    
    # Attribute encodingStyle uses Python identifier encodingStyle
    __encodingStyle = pyxb.binding.content.AttributeUse(pyxb.namespace.ExpandedName(None, 'encodingStyle'), 'encodingStyle', '__httpschemas_xmlsoap_orgwsdlsoap12_tBody_encodingStyle', pyxb.binding.datatypes.anyURI)
    __encodingStyle._DeclarationLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/soapbind12.xsd', 67, 4)
    __encodingStyle._UseLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/soapbind12.xsd', 67, 4)
    
    encodingStyle = property(__encodingStyle.value, __encodingStyle.set, None, None)

    
    # Attribute use is restricted from parent
    
    # Attribute use uses Python identifier use
    __use = pyxb.binding.content.AttributeUse(pyxb.namespace.ExpandedName(None, 'use'), 'use', '__httpschemas_xmlsoap_orgwsdlsoap12_tBody_use', useChoice)
    __use._DeclarationLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/soapbind12.xsd', 68, 4)
    __use._UseLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/soapbind12.xsd', 68, 4)
    
    use = property(__use.value, __use.set, None, None)

    
    # Attribute namespace is restricted from parent
    
    # Attribute namespace uses Python identifier namespace
    __namespace = pyxb.binding.content.AttributeUse(pyxb.namespace.ExpandedName(None, 'namespace'), 'namespace', '__httpschemas_xmlsoap_orgwsdlsoap12_tBody_namespace', pyxb.binding.datatypes.anyURI)
    __namespace._DeclarationLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/soapbind12.xsd', 69, 4)
    __namespace._UseLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/soapbind12.xsd', 69, 4)
    
    namespace = property(__namespace.value, __namespace.set, None, None)

    
    # Attribute parts is restricted from parent
    
    # Attribute parts uses Python identifier parts
    __parts = pyxb.binding.content.AttributeUse(pyxb.namespace.ExpandedName(None, 'parts'), 'parts', '__httpschemas_xmlsoap_orgwsdlsoap12_tBody_parts', tParts, prohibited=True)
    __parts._DeclarationLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/soapbind12.xsd', 95, 8)
    __parts._UseLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/soapbind12.xsd', 95, 8)
    
    parts = property(__parts.value, __parts.set, None, None)

    _ElementMap.update({
        
    })
    _AttributeMap.update({
        __required.name() : __required,
        __encodingStyle.name() : __encodingStyle,
        __use.name() : __use,
        __namespace.name() : __namespace,
        __parts.name() : __parts
    })
Namespace.addCategoryObject('typeBinding', 'tFaultRes', tFaultRes)


# Complex type {http://schemas.xmlsoap.org/wsdl/soap12/}tFault with content type EMPTY
class tFault (tFaultRes):
    """Complex type {http://schemas.xmlsoap.org/wsdl/soap12/}tFault with content type EMPTY"""
    _TypeDefinition = None
    _ContentTypeTag = pyxb.binding.basis.complexTypeDefinition._CT_EMPTY
    _Abstract = False
    _ExpandedName = pyxb.namespace.ExpandedName(Namespace, 'tFault')
    _XSDLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/soapbind12.xsd', 100, 2)
    _ElementMap = tFaultRes._ElementMap.copy()
    _AttributeMap = tFaultRes._AttributeMap.copy()
    # Base type is tFaultRes
    
    # Attribute required_ inherited from {http://schemas.xmlsoap.org/wsdl/soap12/}tFaultRes
    
    # Attribute encodingStyle_ inherited from {http://schemas.xmlsoap.org/wsdl/soap12/}tFaultRes
    
    # Attribute use_ inherited from {http://schemas.xmlsoap.org/wsdl/soap12/}tFaultRes
    
    # Attribute namespace_ inherited from {http://schemas.xmlsoap.org/wsdl/soap12/}tFaultRes
    
    # Attribute parts_ inherited from {http://schemas.xmlsoap.org/wsdl/soap12/}tFaultRes
    
    # Attribute name uses Python identifier name
    __name = pyxb.binding.content.AttributeUse(pyxb.namespace.ExpandedName(None, 'name'), 'name', '__httpschemas_xmlsoap_orgwsdlsoap12_tFault_name', pyxb.binding.datatypes.NCName, required=True)
    __name._DeclarationLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/soapbind12.xsd', 103, 8)
    __name._UseLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/soapbind12.xsd', 103, 8)
    
    name = property(__name.value, __name.set, None, None)

    _ElementMap.update({
        
    })
    _AttributeMap.update({
        __name.name() : __name
    })
Namespace.addCategoryObject('typeBinding', 'tFault', tFault)


binding = pyxb.binding.basis.element(pyxb.namespace.ExpandedName(Namespace, 'binding'), tBinding, location=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/soapbind12.xsd', 36, 2))
Namespace.addCategoryObject('elementBinding', binding.name().localName(), binding)

operation = pyxb.binding.basis.element(pyxb.namespace.ExpandedName(Namespace, 'operation'), tOperation, location=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/soapbind12.xsd', 53, 2))
Namespace.addCategoryObject('elementBinding', operation.name().localName(), operation)

body = pyxb.binding.basis.element(pyxb.namespace.ExpandedName(Namespace, 'body'), tBody, location=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/soapbind12.xsd', 64, 2))
Namespace.addCategoryObject('elementBinding', body.name().localName(), body)

header = pyxb.binding.basis.element(pyxb.namespace.ExpandedName(Namespace, 'header'), tHeader, location=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/soapbind12.xsd', 109, 2))
Namespace.addCategoryObject('elementBinding', header.name().localName(), header)

headerfault = pyxb.binding.basis.element(pyxb.namespace.ExpandedName(Namespace, 'headerfault'), tHeaderFault, location=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/soapbind12.xsd', 128, 2))
Namespace.addCategoryObject('elementBinding', headerfault.name().localName(), headerfault)

address = pyxb.binding.basis.element(pyxb.namespace.ExpandedName(Namespace, 'address'), tAddress, location=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/soapbind12.xsd', 134, 2))
Namespace.addCategoryObject('elementBinding', address.name().localName(), address)

fault = pyxb.binding.basis.element(pyxb.namespace.ExpandedName(Namespace, 'fault'), tFault, location=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/soapbind12.xsd', 90, 2))
Namespace.addCategoryObject('elementBinding', fault.name().localName(), fault)



tHeader._AddElement(pyxb.binding.basis.element(pyxb.namespace.ExpandedName(Namespace, 'headerfault'), tHeaderFault, scope=tHeader, location=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/soapbind12.xsd', 128, 2)))

def _BuildAutomaton ():
    # Remove this helper function from the namespace after it is invoked
    global _BuildAutomaton
    del _BuildAutomaton
    import pyxb.utils.fac as fac

    counters = set()
    cc_0 = fac.CounterCondition(min=0, max=None, metadata=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/soapbind12.xsd', 121, 10))
    counters.add(cc_0)
    states = []
    final_update = set()
    final_update.add(fac.UpdateInstruction(cc_0, False))
    symbol = pyxb.binding.content.ElementUse(tHeader._UseForTag(pyxb.namespace.ExpandedName(Namespace, 'headerfault')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/soapbind12.xsd', 121, 10))
    st_0 = fac.State(symbol, is_initial=True, final_update=final_update, is_unordered_catenation=False)
    states.append(st_0)
    transitions = []
    transitions.append(fac.Transition(st_0, [
        fac.UpdateInstruction(cc_0, True) ]))
    st_0._set_transitionSet(transitions)
    return fac.Automaton(states, counters, True, containing_state=None)
tHeader._Automaton = _BuildAutomaton()

