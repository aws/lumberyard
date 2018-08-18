# ./pyxb/bundles/wssplat/raw/wsdl20.py
# -*- coding: utf-8 -*-
# PyXB bindings for NM:19bbf5816d83775dc94d85cfc7c87689406810ec
# Generated 2014-10-19 06:24:55.511233 by PyXB version 1.2.4 using Python 2.7.3.final.0
# Namespace http://www.w3.org/ns/wsdl

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
_GenerationUID = pyxb.utils.utility.UniqueIdentifier('urn:uuid:8d378866-5782-11e4-9b4c-c8600024e903')

# Version of PyXB used to generate the bindings
_PyXBVersion = '1.2.4'
# Generated bindings are not compatible across PyXB versions
if pyxb.__version__ != _PyXBVersion:
    raise pyxb.PyXBVersionError(_PyXBVersion)

# Import bindings for namespaces imported into schema
import pyxb.binding.datatypes

# NOTE: All namespace declarations are reserved within the binding
Namespace = pyxb.namespace.NamespaceForURI('http://www.w3.org/ns/wsdl', create_if_missing=True)
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
    _XSDLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wsdl20.xsd', 162, 4)
    _Documentation = None

    _ItemType = pyxb.binding.datatypes.QName
STD_ANON._InitializeFacetMap()

# List simple type: [anonymous]
# superclasses pyxb.binding.datatypes.anySimpleType
class STD_ANON_ (pyxb.binding.basis.STD_list):

    """Simple type that is a list of pyxb.binding.datatypes.anyURI."""

    _ExpandedName = None
    _XSDLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wsdl20.xsd', 167, 4)
    _Documentation = None

    _ItemType = pyxb.binding.datatypes.anyURI
STD_ANON_._InitializeFacetMap()

# Atomic simple type: [anonymous]
class STD_ANON_2 (pyxb.binding.datatypes.token, pyxb.binding.basis.enumeration_mixin):

    """An atomic simple type."""

    _ExpandedName = None
    _XSDLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wsdl20.xsd', 215, 6)
    _Documentation = None
STD_ANON_2._CF_enumeration = pyxb.binding.facets.CF_enumeration(value_datatype=STD_ANON_2, enum_prefix=None)
STD_ANON_2.any = STD_ANON_2._CF_enumeration.addEnumeration(unicode_value='#any', tag='any')
STD_ANON_2.none = STD_ANON_2._CF_enumeration.addEnumeration(unicode_value='#none', tag='none')
STD_ANON_2.other = STD_ANON_2._CF_enumeration.addEnumeration(unicode_value='#other', tag='other')
STD_ANON_2._InitializeFacetMap(STD_ANON_2._CF_enumeration)

# Union simple type: {http://www.w3.org/ns/wsdl}ElementReferenceType
# superclasses pyxb.binding.datatypes.anySimpleType
class ElementReferenceType (pyxb.binding.basis.STD_union):

    """
      Use the QName of a GED that describes the content, 
      #any for any content, 
      #none for empty content, or 
      #other for content described by some other extension attribute that references a declaration in a non-XML extension type system.
      """

    _ExpandedName = pyxb.namespace.ExpandedName(Namespace, 'ElementReferenceType')
    _XSDLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wsdl20.xsd', 205, 2)
    _Documentation = '\n      Use the QName of a GED that describes the content, \n      #any for any content, \n      #none for empty content, or \n      #other for content described by some other extension attribute that references a declaration in a non-XML extension type system.\n      '

    _MemberTypes = ( pyxb.binding.datatypes.QName, STD_ANON_2, )
ElementReferenceType._CF_enumeration = pyxb.binding.facets.CF_enumeration(value_datatype=ElementReferenceType)
ElementReferenceType._CF_pattern = pyxb.binding.facets.CF_pattern()
ElementReferenceType.any = '#any'                 # originally STD_ANON_2.any
ElementReferenceType.none = '#none'               # originally STD_ANON_2.none
ElementReferenceType.other = '#other'             # originally STD_ANON_2.other
ElementReferenceType._InitializeFacetMap(ElementReferenceType._CF_enumeration,
   ElementReferenceType._CF_pattern)
Namespace.addCategoryObject('typeBinding', 'ElementReferenceType', ElementReferenceType)

# Complex type {http://www.w3.org/ns/wsdl}DocumentationType with content type MIXED
class DocumentationType (pyxb.binding.basis.complexTypeDefinition):
    """Complex type {http://www.w3.org/ns/wsdl}DocumentationType with content type MIXED"""
    _TypeDefinition = None
    _ContentTypeTag = pyxb.binding.basis.complexTypeDefinition._CT_MIXED
    _Abstract = False
    _ExpandedName = pyxb.namespace.ExpandedName(Namespace, 'DocumentationType')
    _XSDLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wsdl20.xsd', 23, 2)
    _ElementMap = {}
    _AttributeMap = {}
    # Base type is pyxb.binding.datatypes.anyType
    _AttributeWildcard = pyxb.binding.content.Wildcard(process_contents=pyxb.binding.content.Wildcard.PC_lax, namespace_constraint=(pyxb.binding.content.Wildcard.NC_not, 'http://www.w3.org/ns/wsdl'))
    _HasWildcardElement = True
    _ElementMap.update({
        
    })
    _AttributeMap.update({
        
    })
Namespace.addCategoryObject('typeBinding', 'DocumentationType', DocumentationType)


# Complex type {http://www.w3.org/ns/wsdl}DocumentedType with content type ELEMENT_ONLY
class DocumentedType (pyxb.binding.basis.complexTypeDefinition):
    """
      This type is extended by component types to allow them to be documented.
      """
    _TypeDefinition = None
    _ContentTypeTag = pyxb.binding.basis.complexTypeDefinition._CT_ELEMENT_ONLY
    _Abstract = False
    _ExpandedName = pyxb.namespace.ExpandedName(Namespace, 'DocumentedType')
    _XSDLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wsdl20.xsd', 30, 2)
    _ElementMap = {}
    _AttributeMap = {}
    # Base type is pyxb.binding.datatypes.anyType
    
    # Element {http://www.w3.org/ns/wsdl}documentation uses Python identifier documentation
    __documentation = pyxb.binding.content.ElementDeclaration(pyxb.namespace.ExpandedName(Namespace, 'documentation'), 'documentation', '__httpwww_w3_orgnswsdl_DocumentedType_httpwww_w3_orgnswsdldocumentation', True, pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wsdl20.xsd', 22, 2), )

    
    documentation = property(__documentation.value, __documentation.set, None, None)

    _ElementMap.update({
        __documentation.name() : __documentation
    })
    _AttributeMap.update({
        
    })
Namespace.addCategoryObject('typeBinding', 'DocumentedType', DocumentedType)


# Complex type {http://www.w3.org/ns/wsdl}ExtensionElement with content type EMPTY
class ExtensionElement (pyxb.binding.basis.complexTypeDefinition):
    """
	  This abstract type is intended to serve as the base type for
      extension elements. It includes the wsdl:required attribute
      which it is anticipated will be used by most extension elements
	  """
    _TypeDefinition = None
    _ContentTypeTag = pyxb.binding.basis.complexTypeDefinition._CT_EMPTY
    _Abstract = True
    _ExpandedName = pyxb.namespace.ExpandedName(Namespace, 'ExtensionElement')
    _XSDLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wsdl20.xsd', 351, 2)
    _ElementMap = {}
    _AttributeMap = {}
    # Base type is pyxb.binding.datatypes.anyType
    
    # Attribute {http://www.w3.org/ns/wsdl}required uses Python identifier required
    __required = pyxb.binding.content.AttributeUse(pyxb.namespace.ExpandedName(Namespace, 'required'), 'required', '__httpwww_w3_orgnswsdl_ExtensionElement_httpwww_w3_orgnswsdlrequired', pyxb.binding.datatypes.boolean)
    __required._DeclarationLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wsdl20.xsd', 349, 2)
    __required._UseLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wsdl20.xsd', 359, 4)
    
    required = property(__required.value, __required.set, None, None)

    _ElementMap.update({
        
    })
    _AttributeMap.update({
        __required.name() : __required
    })
Namespace.addCategoryObject('typeBinding', 'ExtensionElement', ExtensionElement)


# Complex type {http://www.w3.org/ns/wsdl}ExtensibleDocumentedType with content type ELEMENT_ONLY
class ExtensibleDocumentedType (DocumentedType):
    """
      This type is extended by component types to allow 
      attributes from other namespaces to be added.
      """
    _TypeDefinition = None
    _ContentTypeTag = pyxb.binding.basis.complexTypeDefinition._CT_ELEMENT_ONLY
    _Abstract = True
    _ExpandedName = pyxb.namespace.ExpandedName(Namespace, 'ExtensibleDocumentedType')
    _XSDLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wsdl20.xsd', 41, 2)
    _ElementMap = DocumentedType._ElementMap.copy()
    _AttributeMap = DocumentedType._AttributeMap.copy()
    # Base type is DocumentedType
    
    # Element documentation ({http://www.w3.org/ns/wsdl}documentation) inherited from {http://www.w3.org/ns/wsdl}DocumentedType
    _AttributeWildcard = pyxb.binding.content.Wildcard(process_contents=pyxb.binding.content.Wildcard.PC_lax, namespace_constraint=(pyxb.binding.content.Wildcard.NC_not, 'http://www.w3.org/ns/wsdl'))
    _ElementMap.update({
        
    })
    _AttributeMap.update({
        
    })
Namespace.addCategoryObject('typeBinding', 'ExtensibleDocumentedType', ExtensibleDocumentedType)


# Complex type {http://www.w3.org/ns/wsdl}DescriptionType with content type ELEMENT_ONLY
class DescriptionType (ExtensibleDocumentedType):
    """
        Although correct, this type declaration does not capture
        all the constraints on the contents of the wsdl:description
        element as defined by the WSDL 2.0 specification.

        In particular, the ordering constraints wrt elements preceding
        and following the wsdl:types child element are not captured, as
        attempts to incorporate such restrictions in the schema
        ran afoul of the UPA (Unique Particle Attribution) rule
        in the XML Schema language.

        Please refer to the WSDL 2.0 specification for
	additional information on the contents of this type.
      """
    _TypeDefinition = None
    _ContentTypeTag = pyxb.binding.basis.complexTypeDefinition._CT_ELEMENT_ONLY
    _Abstract = False
    _ExpandedName = pyxb.namespace.ExpandedName(Namespace, 'DescriptionType')
    _XSDLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wsdl20.xsd', 71, 2)
    _ElementMap = ExtensibleDocumentedType._ElementMap.copy()
    _AttributeMap = ExtensibleDocumentedType._AttributeMap.copy()
    # Base type is ExtensibleDocumentedType
    
    # Element documentation ({http://www.w3.org/ns/wsdl}documentation) inherited from {http://www.w3.org/ns/wsdl}DocumentedType
    
    # Element {http://www.w3.org/ns/wsdl}import uses Python identifier import_
    __import = pyxb.binding.content.ElementDeclaration(pyxb.namespace.ExpandedName(Namespace, 'import'), 'import_', '__httpwww_w3_orgnswsdl_DescriptionType_httpwww_w3_orgnswsdlimport', True, pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wsdl20.xsd', 105, 2), )

    
    import_ = property(__import.value, __import.set, None, None)

    
    # Element {http://www.w3.org/ns/wsdl}include uses Python identifier include
    __include = pyxb.binding.content.ElementDeclaration(pyxb.namespace.ExpandedName(Namespace, 'include'), 'include', '__httpwww_w3_orgnswsdl_DescriptionType_httpwww_w3_orgnswsdlinclude', True, pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wsdl20.xsd', 118, 2), )

    
    include = property(__include.value, __include.set, None, None)

    
    # Element {http://www.w3.org/ns/wsdl}types uses Python identifier types
    __types = pyxb.binding.content.ElementDeclaration(pyxb.namespace.ExpandedName(Namespace, 'types'), 'types', '__httpwww_w3_orgnswsdl_DescriptionType_httpwww_w3_orgnswsdltypes', True, pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wsdl20.xsd', 130, 2), )

    
    types = property(__types.value, __types.set, None, None)

    
    # Element {http://www.w3.org/ns/wsdl}interface uses Python identifier interface
    __interface = pyxb.binding.content.ElementDeclaration(pyxb.namespace.ExpandedName(Namespace, 'interface'), 'interface', '__httpwww_w3_orgnswsdl_DescriptionType_httpwww_w3_orgnswsdlinterface', True, pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wsdl20.xsd', 142, 2), )

    
    interface = property(__interface.value, __interface.set, None, None)

    
    # Element {http://www.w3.org/ns/wsdl}binding uses Python identifier binding
    __binding = pyxb.binding.content.ElementDeclaration(pyxb.namespace.ExpandedName(Namespace, 'binding'), 'binding', '__httpwww_w3_orgnswsdl_DescriptionType_httpwww_w3_orgnswsdlbinding', True, pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wsdl20.xsd', 250, 2), )

    
    binding = property(__binding.value, __binding.set, None, None)

    
    # Element {http://www.w3.org/ns/wsdl}service uses Python identifier service
    __service = pyxb.binding.content.ElementDeclaration(pyxb.namespace.ExpandedName(Namespace, 'service'), 'service', '__httpwww_w3_orgnswsdl_DescriptionType_httpwww_w3_orgnswsdlservice', True, pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wsdl20.xsd', 316, 2), )

    
    service = property(__service.value, __service.set, None, None)

    
    # Attribute targetNamespace uses Python identifier targetNamespace
    __targetNamespace = pyxb.binding.content.AttributeUse(pyxb.namespace.ExpandedName(None, 'targetNamespace'), 'targetNamespace', '__httpwww_w3_orgnswsdl_DescriptionType_targetNamespace', pyxb.binding.datatypes.anyURI, required=True)
    __targetNamespace._DeclarationLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wsdl20.xsd', 99, 1)
    __targetNamespace._UseLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wsdl20.xsd', 99, 1)
    
    targetNamespace = property(__targetNamespace.value, __targetNamespace.set, None, None)

    _AttributeWildcard = pyxb.binding.content.Wildcard(process_contents=pyxb.binding.content.Wildcard.PC_lax, namespace_constraint=(pyxb.binding.content.Wildcard.NC_not, 'http://www.w3.org/ns/wsdl'))
    _HasWildcardElement = True
    _ElementMap.update({
        __import.name() : __import,
        __include.name() : __include,
        __types.name() : __types,
        __interface.name() : __interface,
        __binding.name() : __binding,
        __service.name() : __service
    })
    _AttributeMap.update({
        __targetNamespace.name() : __targetNamespace
    })
Namespace.addCategoryObject('typeBinding', 'DescriptionType', DescriptionType)


# Complex type {http://www.w3.org/ns/wsdl}ImportType with content type ELEMENT_ONLY
class ImportType (ExtensibleDocumentedType):
    """Complex type {http://www.w3.org/ns/wsdl}ImportType with content type ELEMENT_ONLY"""
    _TypeDefinition = None
    _ContentTypeTag = pyxb.binding.basis.complexTypeDefinition._CT_ELEMENT_ONLY
    _Abstract = False
    _ExpandedName = pyxb.namespace.ExpandedName(Namespace, 'ImportType')
    _XSDLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wsdl20.xsd', 106, 2)
    _ElementMap = ExtensibleDocumentedType._ElementMap.copy()
    _AttributeMap = ExtensibleDocumentedType._AttributeMap.copy()
    # Base type is ExtensibleDocumentedType
    
    # Element documentation ({http://www.w3.org/ns/wsdl}documentation) inherited from {http://www.w3.org/ns/wsdl}DocumentedType
    
    # Attribute namespace uses Python identifier namespace
    __namespace = pyxb.binding.content.AttributeUse(pyxb.namespace.ExpandedName(None, 'namespace'), 'namespace', '__httpwww_w3_orgnswsdl_ImportType_namespace', pyxb.binding.datatypes.anyURI, required=True)
    __namespace._DeclarationLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wsdl20.xsd', 112, 8)
    __namespace._UseLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wsdl20.xsd', 112, 8)
    
    namespace = property(__namespace.value, __namespace.set, None, None)

    
    # Attribute location uses Python identifier location
    __location = pyxb.binding.content.AttributeUse(pyxb.namespace.ExpandedName(None, 'location'), 'location', '__httpwww_w3_orgnswsdl_ImportType_location', pyxb.binding.datatypes.anyURI)
    __location._DeclarationLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wsdl20.xsd', 113, 8)
    __location._UseLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wsdl20.xsd', 113, 8)
    
    location = property(__location.value, __location.set, None, None)

    _AttributeWildcard = pyxb.binding.content.Wildcard(process_contents=pyxb.binding.content.Wildcard.PC_lax, namespace_constraint=(pyxb.binding.content.Wildcard.NC_not, 'http://www.w3.org/ns/wsdl'))
    _HasWildcardElement = True
    _ElementMap.update({
        
    })
    _AttributeMap.update({
        __namespace.name() : __namespace,
        __location.name() : __location
    })
Namespace.addCategoryObject('typeBinding', 'ImportType', ImportType)


# Complex type {http://www.w3.org/ns/wsdl}IncludeType with content type ELEMENT_ONLY
class IncludeType (ExtensibleDocumentedType):
    """Complex type {http://www.w3.org/ns/wsdl}IncludeType with content type ELEMENT_ONLY"""
    _TypeDefinition = None
    _ContentTypeTag = pyxb.binding.basis.complexTypeDefinition._CT_ELEMENT_ONLY
    _Abstract = False
    _ExpandedName = pyxb.namespace.ExpandedName(Namespace, 'IncludeType')
    _XSDLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wsdl20.xsd', 119, 2)
    _ElementMap = ExtensibleDocumentedType._ElementMap.copy()
    _AttributeMap = ExtensibleDocumentedType._AttributeMap.copy()
    # Base type is ExtensibleDocumentedType
    
    # Element documentation ({http://www.w3.org/ns/wsdl}documentation) inherited from {http://www.w3.org/ns/wsdl}DocumentedType
    
    # Attribute location uses Python identifier location
    __location = pyxb.binding.content.AttributeUse(pyxb.namespace.ExpandedName(None, 'location'), 'location', '__httpwww_w3_orgnswsdl_IncludeType_location', pyxb.binding.datatypes.anyURI, required=True)
    __location._DeclarationLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wsdl20.xsd', 125, 8)
    __location._UseLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wsdl20.xsd', 125, 8)
    
    location = property(__location.value, __location.set, None, None)

    _AttributeWildcard = pyxb.binding.content.Wildcard(process_contents=pyxb.binding.content.Wildcard.PC_lax, namespace_constraint=(pyxb.binding.content.Wildcard.NC_not, 'http://www.w3.org/ns/wsdl'))
    _HasWildcardElement = True
    _ElementMap.update({
        
    })
    _AttributeMap.update({
        __location.name() : __location
    })
Namespace.addCategoryObject('typeBinding', 'IncludeType', IncludeType)


# Complex type {http://www.w3.org/ns/wsdl}TypesType with content type ELEMENT_ONLY
class TypesType (ExtensibleDocumentedType):
    """Complex type {http://www.w3.org/ns/wsdl}TypesType with content type ELEMENT_ONLY"""
    _TypeDefinition = None
    _ContentTypeTag = pyxb.binding.basis.complexTypeDefinition._CT_ELEMENT_ONLY
    _Abstract = False
    _ExpandedName = pyxb.namespace.ExpandedName(Namespace, 'TypesType')
    _XSDLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wsdl20.xsd', 131, 2)
    _ElementMap = ExtensibleDocumentedType._ElementMap.copy()
    _AttributeMap = ExtensibleDocumentedType._AttributeMap.copy()
    # Base type is ExtensibleDocumentedType
    
    # Element documentation ({http://www.w3.org/ns/wsdl}documentation) inherited from {http://www.w3.org/ns/wsdl}DocumentedType
    _AttributeWildcard = pyxb.binding.content.Wildcard(process_contents=pyxb.binding.content.Wildcard.PC_lax, namespace_constraint=(pyxb.binding.content.Wildcard.NC_not, 'http://www.w3.org/ns/wsdl'))
    _HasWildcardElement = True
    _ElementMap.update({
        
    })
    _AttributeMap.update({
        
    })
Namespace.addCategoryObject('typeBinding', 'TypesType', TypesType)


# Complex type {http://www.w3.org/ns/wsdl}InterfaceType with content type ELEMENT_ONLY
class InterfaceType (ExtensibleDocumentedType):
    """Complex type {http://www.w3.org/ns/wsdl}InterfaceType with content type ELEMENT_ONLY"""
    _TypeDefinition = None
    _ContentTypeTag = pyxb.binding.basis.complexTypeDefinition._CT_ELEMENT_ONLY
    _Abstract = False
    _ExpandedName = pyxb.namespace.ExpandedName(Namespace, 'InterfaceType')
    _XSDLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wsdl20.xsd', 152, 2)
    _ElementMap = ExtensibleDocumentedType._ElementMap.copy()
    _AttributeMap = ExtensibleDocumentedType._AttributeMap.copy()
    # Base type is ExtensibleDocumentedType
    
    # Element documentation ({http://www.w3.org/ns/wsdl}documentation) inherited from {http://www.w3.org/ns/wsdl}DocumentedType
    
    # Element {http://www.w3.org/ns/wsdl}operation uses Python identifier operation
    __operation = pyxb.binding.content.ElementDeclaration(pyxb.namespace.ExpandedName(Namespace, 'operation'), 'operation', '__httpwww_w3_orgnswsdl_InterfaceType_httpwww_w3_orgnswsdloperation', True, pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wsdl20.xsd', 156, 10), )

    
    operation = property(__operation.value, __operation.set, None, None)

    
    # Element {http://www.w3.org/ns/wsdl}fault uses Python identifier fault
    __fault = pyxb.binding.content.ElementDeclaration(pyxb.namespace.ExpandedName(Namespace, 'fault'), 'fault', '__httpwww_w3_orgnswsdl_InterfaceType_httpwww_w3_orgnswsdlfault', True, pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wsdl20.xsd', 157, 10), )

    
    fault = property(__fault.value, __fault.set, None, None)

    
    # Attribute name uses Python identifier name
    __name = pyxb.binding.content.AttributeUse(pyxb.namespace.ExpandedName(None, 'name'), 'name', '__httpwww_w3_orgnswsdl_InterfaceType_name', pyxb.binding.datatypes.NCName, required=True)
    __name._DeclarationLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wsdl20.xsd', 160, 8)
    __name._UseLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wsdl20.xsd', 160, 8)
    
    name = property(__name.value, __name.set, None, None)

    
    # Attribute extends uses Python identifier extends
    __extends = pyxb.binding.content.AttributeUse(pyxb.namespace.ExpandedName(None, 'extends'), 'extends', '__httpwww_w3_orgnswsdl_InterfaceType_extends', STD_ANON)
    __extends._DeclarationLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wsdl20.xsd', 161, 8)
    __extends._UseLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wsdl20.xsd', 161, 8)
    
    extends = property(__extends.value, __extends.set, None, None)

    
    # Attribute styleDefault uses Python identifier styleDefault
    __styleDefault = pyxb.binding.content.AttributeUse(pyxb.namespace.ExpandedName(None, 'styleDefault'), 'styleDefault', '__httpwww_w3_orgnswsdl_InterfaceType_styleDefault', STD_ANON_)
    __styleDefault._DeclarationLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wsdl20.xsd', 166, 8)
    __styleDefault._UseLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wsdl20.xsd', 166, 8)
    
    styleDefault = property(__styleDefault.value, __styleDefault.set, None, None)

    _AttributeWildcard = pyxb.binding.content.Wildcard(process_contents=pyxb.binding.content.Wildcard.PC_lax, namespace_constraint=(pyxb.binding.content.Wildcard.NC_not, 'http://www.w3.org/ns/wsdl'))
    _HasWildcardElement = True
    _ElementMap.update({
        __operation.name() : __operation,
        __fault.name() : __fault
    })
    _AttributeMap.update({
        __name.name() : __name,
        __extends.name() : __extends,
        __styleDefault.name() : __styleDefault
    })
Namespace.addCategoryObject('typeBinding', 'InterfaceType', InterfaceType)


# Complex type {http://www.w3.org/ns/wsdl}InterfaceOperationType with content type ELEMENT_ONLY
class InterfaceOperationType (ExtensibleDocumentedType):
    """Complex type {http://www.w3.org/ns/wsdl}InterfaceOperationType with content type ELEMENT_ONLY"""
    _TypeDefinition = None
    _ContentTypeTag = pyxb.binding.basis.complexTypeDefinition._CT_ELEMENT_ONLY
    _Abstract = False
    _ExpandedName = pyxb.namespace.ExpandedName(Namespace, 'InterfaceOperationType')
    _XSDLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wsdl20.xsd', 175, 2)
    _ElementMap = ExtensibleDocumentedType._ElementMap.copy()
    _AttributeMap = ExtensibleDocumentedType._AttributeMap.copy()
    # Base type is ExtensibleDocumentedType
    
    # Element documentation ({http://www.w3.org/ns/wsdl}documentation) inherited from {http://www.w3.org/ns/wsdl}DocumentedType
    
    # Element {http://www.w3.org/ns/wsdl}input uses Python identifier input
    __input = pyxb.binding.content.ElementDeclaration(pyxb.namespace.ExpandedName(Namespace, 'input'), 'input', '__httpwww_w3_orgnswsdl_InterfaceOperationType_httpwww_w3_orgnswsdlinput', True, pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wsdl20.xsd', 179, 10), )

    
    input = property(__input.value, __input.set, None, None)

    
    # Element {http://www.w3.org/ns/wsdl}output uses Python identifier output
    __output = pyxb.binding.content.ElementDeclaration(pyxb.namespace.ExpandedName(Namespace, 'output'), 'output', '__httpwww_w3_orgnswsdl_InterfaceOperationType_httpwww_w3_orgnswsdloutput', True, pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wsdl20.xsd', 180, 10), )

    
    output = property(__output.value, __output.set, None, None)

    
    # Element {http://www.w3.org/ns/wsdl}infault uses Python identifier infault
    __infault = pyxb.binding.content.ElementDeclaration(pyxb.namespace.ExpandedName(Namespace, 'infault'), 'infault', '__httpwww_w3_orgnswsdl_InterfaceOperationType_httpwww_w3_orgnswsdlinfault', True, pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wsdl20.xsd', 181, 10), )

    
    infault = property(__infault.value, __infault.set, None, None)

    
    # Element {http://www.w3.org/ns/wsdl}outfault uses Python identifier outfault
    __outfault = pyxb.binding.content.ElementDeclaration(pyxb.namespace.ExpandedName(Namespace, 'outfault'), 'outfault', '__httpwww_w3_orgnswsdl_InterfaceOperationType_httpwww_w3_orgnswsdloutfault', True, pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wsdl20.xsd', 182, 10), )

    
    outfault = property(__outfault.value, __outfault.set, None, None)

    
    # Attribute name uses Python identifier name
    __name = pyxb.binding.content.AttributeUse(pyxb.namespace.ExpandedName(None, 'name'), 'name', '__httpwww_w3_orgnswsdl_InterfaceOperationType_name', pyxb.binding.datatypes.NCName, required=True)
    __name._DeclarationLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wsdl20.xsd', 185, 8)
    __name._UseLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wsdl20.xsd', 185, 8)
    
    name = property(__name.value, __name.set, None, None)

    
    # Attribute pattern uses Python identifier pattern
    __pattern = pyxb.binding.content.AttributeUse(pyxb.namespace.ExpandedName(None, 'pattern'), 'pattern', '__httpwww_w3_orgnswsdl_InterfaceOperationType_pattern', pyxb.binding.datatypes.anyURI)
    __pattern._DeclarationLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wsdl20.xsd', 186, 8)
    __pattern._UseLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wsdl20.xsd', 186, 8)
    
    pattern = property(__pattern.value, __pattern.set, None, None)

    
    # Attribute safe uses Python identifier safe
    __safe = pyxb.binding.content.AttributeUse(pyxb.namespace.ExpandedName(None, 'safe'), 'safe', '__httpwww_w3_orgnswsdl_InterfaceOperationType_safe', pyxb.binding.datatypes.boolean)
    __safe._DeclarationLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wsdl20.xsd', 187, 8)
    __safe._UseLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wsdl20.xsd', 187, 8)
    
    safe = property(__safe.value, __safe.set, None, None)

    
    # Attribute style uses Python identifier style
    __style = pyxb.binding.content.AttributeUse(pyxb.namespace.ExpandedName(None, 'style'), 'style', '__httpwww_w3_orgnswsdl_InterfaceOperationType_style', pyxb.binding.datatypes.anyURI)
    __style._DeclarationLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wsdl20.xsd', 188, 8)
    __style._UseLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wsdl20.xsd', 188, 8)
    
    style = property(__style.value, __style.set, None, None)

    _AttributeWildcard = pyxb.binding.content.Wildcard(process_contents=pyxb.binding.content.Wildcard.PC_lax, namespace_constraint=(pyxb.binding.content.Wildcard.NC_not, 'http://www.w3.org/ns/wsdl'))
    _HasWildcardElement = True
    _ElementMap.update({
        __input.name() : __input,
        __output.name() : __output,
        __infault.name() : __infault,
        __outfault.name() : __outfault
    })
    _AttributeMap.update({
        __name.name() : __name,
        __pattern.name() : __pattern,
        __safe.name() : __safe,
        __style.name() : __style
    })
Namespace.addCategoryObject('typeBinding', 'InterfaceOperationType', InterfaceOperationType)


# Complex type {http://www.w3.org/ns/wsdl}MessageRefType with content type ELEMENT_ONLY
class MessageRefType (ExtensibleDocumentedType):
    """Complex type {http://www.w3.org/ns/wsdl}MessageRefType with content type ELEMENT_ONLY"""
    _TypeDefinition = None
    _ContentTypeTag = pyxb.binding.basis.complexTypeDefinition._CT_ELEMENT_ONLY
    _Abstract = False
    _ExpandedName = pyxb.namespace.ExpandedName(Namespace, 'MessageRefType')
    _XSDLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wsdl20.xsd', 193, 2)
    _ElementMap = ExtensibleDocumentedType._ElementMap.copy()
    _AttributeMap = ExtensibleDocumentedType._AttributeMap.copy()
    # Base type is ExtensibleDocumentedType
    
    # Element documentation ({http://www.w3.org/ns/wsdl}documentation) inherited from {http://www.w3.org/ns/wsdl}DocumentedType
    
    # Attribute messageLabel uses Python identifier messageLabel
    __messageLabel = pyxb.binding.content.AttributeUse(pyxb.namespace.ExpandedName(None, 'messageLabel'), 'messageLabel', '__httpwww_w3_orgnswsdl_MessageRefType_messageLabel', pyxb.binding.datatypes.NCName)
    __messageLabel._DeclarationLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wsdl20.xsd', 199, 8)
    __messageLabel._UseLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wsdl20.xsd', 199, 8)
    
    messageLabel = property(__messageLabel.value, __messageLabel.set, None, None)

    
    # Attribute element uses Python identifier element
    __element = pyxb.binding.content.AttributeUse(pyxb.namespace.ExpandedName(None, 'element'), 'element', '__httpwww_w3_orgnswsdl_MessageRefType_element', ElementReferenceType)
    __element._DeclarationLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wsdl20.xsd', 200, 8)
    __element._UseLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wsdl20.xsd', 200, 8)
    
    element = property(__element.value, __element.set, None, None)

    _AttributeWildcard = pyxb.binding.content.Wildcard(process_contents=pyxb.binding.content.Wildcard.PC_lax, namespace_constraint=(pyxb.binding.content.Wildcard.NC_not, 'http://www.w3.org/ns/wsdl'))
    _HasWildcardElement = True
    _ElementMap.update({
        
    })
    _AttributeMap.update({
        __messageLabel.name() : __messageLabel,
        __element.name() : __element
    })
Namespace.addCategoryObject('typeBinding', 'MessageRefType', MessageRefType)


# Complex type {http://www.w3.org/ns/wsdl}MessageRefFaultType with content type ELEMENT_ONLY
class MessageRefFaultType (ExtensibleDocumentedType):
    """Complex type {http://www.w3.org/ns/wsdl}MessageRefFaultType with content type ELEMENT_ONLY"""
    _TypeDefinition = None
    _ContentTypeTag = pyxb.binding.basis.complexTypeDefinition._CT_ELEMENT_ONLY
    _Abstract = False
    _ExpandedName = pyxb.namespace.ExpandedName(Namespace, 'MessageRefFaultType')
    _XSDLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wsdl20.xsd', 225, 2)
    _ElementMap = ExtensibleDocumentedType._ElementMap.copy()
    _AttributeMap = ExtensibleDocumentedType._AttributeMap.copy()
    # Base type is ExtensibleDocumentedType
    
    # Element documentation ({http://www.w3.org/ns/wsdl}documentation) inherited from {http://www.w3.org/ns/wsdl}DocumentedType
    
    # Attribute ref uses Python identifier ref
    __ref = pyxb.binding.content.AttributeUse(pyxb.namespace.ExpandedName(None, 'ref'), 'ref', '__httpwww_w3_orgnswsdl_MessageRefFaultType_ref', pyxb.binding.datatypes.QName, required=True)
    __ref._DeclarationLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wsdl20.xsd', 231, 8)
    __ref._UseLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wsdl20.xsd', 231, 8)
    
    ref = property(__ref.value, __ref.set, None, None)

    
    # Attribute messageLabel uses Python identifier messageLabel
    __messageLabel = pyxb.binding.content.AttributeUse(pyxb.namespace.ExpandedName(None, 'messageLabel'), 'messageLabel', '__httpwww_w3_orgnswsdl_MessageRefFaultType_messageLabel', pyxb.binding.datatypes.NCName)
    __messageLabel._DeclarationLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wsdl20.xsd', 232, 8)
    __messageLabel._UseLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wsdl20.xsd', 232, 8)
    
    messageLabel = property(__messageLabel.value, __messageLabel.set, None, None)

    _AttributeWildcard = pyxb.binding.content.Wildcard(process_contents=pyxb.binding.content.Wildcard.PC_lax, namespace_constraint=(pyxb.binding.content.Wildcard.NC_not, 'http://www.w3.org/ns/wsdl'))
    _HasWildcardElement = True
    _ElementMap.update({
        
    })
    _AttributeMap.update({
        __ref.name() : __ref,
        __messageLabel.name() : __messageLabel
    })
Namespace.addCategoryObject('typeBinding', 'MessageRefFaultType', MessageRefFaultType)


# Complex type {http://www.w3.org/ns/wsdl}InterfaceFaultType with content type ELEMENT_ONLY
class InterfaceFaultType (ExtensibleDocumentedType):
    """Complex type {http://www.w3.org/ns/wsdl}InterfaceFaultType with content type ELEMENT_ONLY"""
    _TypeDefinition = None
    _ContentTypeTag = pyxb.binding.basis.complexTypeDefinition._CT_ELEMENT_ONLY
    _Abstract = False
    _ExpandedName = pyxb.namespace.ExpandedName(Namespace, 'InterfaceFaultType')
    _XSDLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wsdl20.xsd', 237, 2)
    _ElementMap = ExtensibleDocumentedType._ElementMap.copy()
    _AttributeMap = ExtensibleDocumentedType._AttributeMap.copy()
    # Base type is ExtensibleDocumentedType
    
    # Element documentation ({http://www.w3.org/ns/wsdl}documentation) inherited from {http://www.w3.org/ns/wsdl}DocumentedType
    
    # Attribute name uses Python identifier name
    __name = pyxb.binding.content.AttributeUse(pyxb.namespace.ExpandedName(None, 'name'), 'name', '__httpwww_w3_orgnswsdl_InterfaceFaultType_name', pyxb.binding.datatypes.NCName, required=True)
    __name._DeclarationLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wsdl20.xsd', 243, 8)
    __name._UseLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wsdl20.xsd', 243, 8)
    
    name = property(__name.value, __name.set, None, None)

    
    # Attribute element uses Python identifier element
    __element = pyxb.binding.content.AttributeUse(pyxb.namespace.ExpandedName(None, 'element'), 'element', '__httpwww_w3_orgnswsdl_InterfaceFaultType_element', ElementReferenceType)
    __element._DeclarationLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wsdl20.xsd', 244, 8)
    __element._UseLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wsdl20.xsd', 244, 8)
    
    element = property(__element.value, __element.set, None, None)

    _AttributeWildcard = pyxb.binding.content.Wildcard(process_contents=pyxb.binding.content.Wildcard.PC_lax, namespace_constraint=(pyxb.binding.content.Wildcard.NC_not, 'http://www.w3.org/ns/wsdl'))
    _HasWildcardElement = True
    _ElementMap.update({
        
    })
    _AttributeMap.update({
        __name.name() : __name,
        __element.name() : __element
    })
Namespace.addCategoryObject('typeBinding', 'InterfaceFaultType', InterfaceFaultType)


# Complex type {http://www.w3.org/ns/wsdl}BindingType with content type ELEMENT_ONLY
class BindingType (ExtensibleDocumentedType):
    """Complex type {http://www.w3.org/ns/wsdl}BindingType with content type ELEMENT_ONLY"""
    _TypeDefinition = None
    _ContentTypeTag = pyxb.binding.basis.complexTypeDefinition._CT_ELEMENT_ONLY
    _Abstract = False
    _ExpandedName = pyxb.namespace.ExpandedName(Namespace, 'BindingType')
    _XSDLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wsdl20.xsd', 251, 2)
    _ElementMap = ExtensibleDocumentedType._ElementMap.copy()
    _AttributeMap = ExtensibleDocumentedType._AttributeMap.copy()
    # Base type is ExtensibleDocumentedType
    
    # Element documentation ({http://www.w3.org/ns/wsdl}documentation) inherited from {http://www.w3.org/ns/wsdl}DocumentedType
    
    # Element {http://www.w3.org/ns/wsdl}operation uses Python identifier operation
    __operation = pyxb.binding.content.ElementDeclaration(pyxb.namespace.ExpandedName(Namespace, 'operation'), 'operation', '__httpwww_w3_orgnswsdl_BindingType_httpwww_w3_orgnswsdloperation', True, pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wsdl20.xsd', 255, 10), )

    
    operation = property(__operation.value, __operation.set, None, None)

    
    # Element {http://www.w3.org/ns/wsdl}fault uses Python identifier fault
    __fault = pyxb.binding.content.ElementDeclaration(pyxb.namespace.ExpandedName(Namespace, 'fault'), 'fault', '__httpwww_w3_orgnswsdl_BindingType_httpwww_w3_orgnswsdlfault', True, pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wsdl20.xsd', 256, 10), )

    
    fault = property(__fault.value, __fault.set, None, None)

    
    # Attribute name uses Python identifier name
    __name = pyxb.binding.content.AttributeUse(pyxb.namespace.ExpandedName(None, 'name'), 'name', '__httpwww_w3_orgnswsdl_BindingType_name', pyxb.binding.datatypes.NCName, required=True)
    __name._DeclarationLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wsdl20.xsd', 259, 8)
    __name._UseLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wsdl20.xsd', 259, 8)
    
    name = property(__name.value, __name.set, None, None)

    
    # Attribute type uses Python identifier type
    __type = pyxb.binding.content.AttributeUse(pyxb.namespace.ExpandedName(None, 'type'), 'type', '__httpwww_w3_orgnswsdl_BindingType_type', pyxb.binding.datatypes.anyURI, required=True)
    __type._DeclarationLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wsdl20.xsd', 260, 8)
    __type._UseLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wsdl20.xsd', 260, 8)
    
    type = property(__type.value, __type.set, None, None)

    
    # Attribute interface uses Python identifier interface
    __interface = pyxb.binding.content.AttributeUse(pyxb.namespace.ExpandedName(None, 'interface'), 'interface', '__httpwww_w3_orgnswsdl_BindingType_interface', pyxb.binding.datatypes.QName)
    __interface._DeclarationLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wsdl20.xsd', 261, 8)
    __interface._UseLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wsdl20.xsd', 261, 8)
    
    interface = property(__interface.value, __interface.set, None, None)

    _AttributeWildcard = pyxb.binding.content.Wildcard(process_contents=pyxb.binding.content.Wildcard.PC_lax, namespace_constraint=(pyxb.binding.content.Wildcard.NC_not, 'http://www.w3.org/ns/wsdl'))
    _HasWildcardElement = True
    _ElementMap.update({
        __operation.name() : __operation,
        __fault.name() : __fault
    })
    _AttributeMap.update({
        __name.name() : __name,
        __type.name() : __type,
        __interface.name() : __interface
    })
Namespace.addCategoryObject('typeBinding', 'BindingType', BindingType)


# Complex type {http://www.w3.org/ns/wsdl}BindingOperationType with content type ELEMENT_ONLY
class BindingOperationType (ExtensibleDocumentedType):
    """Complex type {http://www.w3.org/ns/wsdl}BindingOperationType with content type ELEMENT_ONLY"""
    _TypeDefinition = None
    _ContentTypeTag = pyxb.binding.basis.complexTypeDefinition._CT_ELEMENT_ONLY
    _Abstract = False
    _ExpandedName = pyxb.namespace.ExpandedName(Namespace, 'BindingOperationType')
    _XSDLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wsdl20.xsd', 266, 2)
    _ElementMap = ExtensibleDocumentedType._ElementMap.copy()
    _AttributeMap = ExtensibleDocumentedType._AttributeMap.copy()
    # Base type is ExtensibleDocumentedType
    
    # Element documentation ({http://www.w3.org/ns/wsdl}documentation) inherited from {http://www.w3.org/ns/wsdl}DocumentedType
    
    # Element {http://www.w3.org/ns/wsdl}input uses Python identifier input
    __input = pyxb.binding.content.ElementDeclaration(pyxb.namespace.ExpandedName(Namespace, 'input'), 'input', '__httpwww_w3_orgnswsdl_BindingOperationType_httpwww_w3_orgnswsdlinput', True, pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wsdl20.xsd', 270, 10), )

    
    input = property(__input.value, __input.set, None, None)

    
    # Element {http://www.w3.org/ns/wsdl}output uses Python identifier output
    __output = pyxb.binding.content.ElementDeclaration(pyxb.namespace.ExpandedName(Namespace, 'output'), 'output', '__httpwww_w3_orgnswsdl_BindingOperationType_httpwww_w3_orgnswsdloutput', True, pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wsdl20.xsd', 271, 10), )

    
    output = property(__output.value, __output.set, None, None)

    
    # Element {http://www.w3.org/ns/wsdl}infault uses Python identifier infault
    __infault = pyxb.binding.content.ElementDeclaration(pyxb.namespace.ExpandedName(Namespace, 'infault'), 'infault', '__httpwww_w3_orgnswsdl_BindingOperationType_httpwww_w3_orgnswsdlinfault', True, pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wsdl20.xsd', 272, 10), )

    
    infault = property(__infault.value, __infault.set, None, None)

    
    # Element {http://www.w3.org/ns/wsdl}outfault uses Python identifier outfault
    __outfault = pyxb.binding.content.ElementDeclaration(pyxb.namespace.ExpandedName(Namespace, 'outfault'), 'outfault', '__httpwww_w3_orgnswsdl_BindingOperationType_httpwww_w3_orgnswsdloutfault', True, pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wsdl20.xsd', 273, 10), )

    
    outfault = property(__outfault.value, __outfault.set, None, None)

    
    # Attribute ref uses Python identifier ref
    __ref = pyxb.binding.content.AttributeUse(pyxb.namespace.ExpandedName(None, 'ref'), 'ref', '__httpwww_w3_orgnswsdl_BindingOperationType_ref', pyxb.binding.datatypes.QName, required=True)
    __ref._DeclarationLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wsdl20.xsd', 276, 8)
    __ref._UseLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wsdl20.xsd', 276, 8)
    
    ref = property(__ref.value, __ref.set, None, None)

    _AttributeWildcard = pyxb.binding.content.Wildcard(process_contents=pyxb.binding.content.Wildcard.PC_lax, namespace_constraint=(pyxb.binding.content.Wildcard.NC_not, 'http://www.w3.org/ns/wsdl'))
    _HasWildcardElement = True
    _ElementMap.update({
        __input.name() : __input,
        __output.name() : __output,
        __infault.name() : __infault,
        __outfault.name() : __outfault
    })
    _AttributeMap.update({
        __ref.name() : __ref
    })
Namespace.addCategoryObject('typeBinding', 'BindingOperationType', BindingOperationType)


# Complex type {http://www.w3.org/ns/wsdl}BindingOperationMessageType with content type ELEMENT_ONLY
class BindingOperationMessageType (ExtensibleDocumentedType):
    """Complex type {http://www.w3.org/ns/wsdl}BindingOperationMessageType with content type ELEMENT_ONLY"""
    _TypeDefinition = None
    _ContentTypeTag = pyxb.binding.basis.complexTypeDefinition._CT_ELEMENT_ONLY
    _Abstract = False
    _ExpandedName = pyxb.namespace.ExpandedName(Namespace, 'BindingOperationMessageType')
    _XSDLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wsdl20.xsd', 281, 2)
    _ElementMap = ExtensibleDocumentedType._ElementMap.copy()
    _AttributeMap = ExtensibleDocumentedType._AttributeMap.copy()
    # Base type is ExtensibleDocumentedType
    
    # Element documentation ({http://www.w3.org/ns/wsdl}documentation) inherited from {http://www.w3.org/ns/wsdl}DocumentedType
    
    # Attribute messageLabel uses Python identifier messageLabel
    __messageLabel = pyxb.binding.content.AttributeUse(pyxb.namespace.ExpandedName(None, 'messageLabel'), 'messageLabel', '__httpwww_w3_orgnswsdl_BindingOperationMessageType_messageLabel', pyxb.binding.datatypes.NCName)
    __messageLabel._DeclarationLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wsdl20.xsd', 287, 8)
    __messageLabel._UseLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wsdl20.xsd', 287, 8)
    
    messageLabel = property(__messageLabel.value, __messageLabel.set, None, None)

    _AttributeWildcard = pyxb.binding.content.Wildcard(process_contents=pyxb.binding.content.Wildcard.PC_lax, namespace_constraint=(pyxb.binding.content.Wildcard.NC_not, 'http://www.w3.org/ns/wsdl'))
    _HasWildcardElement = True
    _ElementMap.update({
        
    })
    _AttributeMap.update({
        __messageLabel.name() : __messageLabel
    })
Namespace.addCategoryObject('typeBinding', 'BindingOperationMessageType', BindingOperationMessageType)


# Complex type {http://www.w3.org/ns/wsdl}BindingOperationFaultType with content type ELEMENT_ONLY
class BindingOperationFaultType (ExtensibleDocumentedType):
    """Complex type {http://www.w3.org/ns/wsdl}BindingOperationFaultType with content type ELEMENT_ONLY"""
    _TypeDefinition = None
    _ContentTypeTag = pyxb.binding.basis.complexTypeDefinition._CT_ELEMENT_ONLY
    _Abstract = False
    _ExpandedName = pyxb.namespace.ExpandedName(Namespace, 'BindingOperationFaultType')
    _XSDLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wsdl20.xsd', 292, 2)
    _ElementMap = ExtensibleDocumentedType._ElementMap.copy()
    _AttributeMap = ExtensibleDocumentedType._AttributeMap.copy()
    # Base type is ExtensibleDocumentedType
    
    # Element documentation ({http://www.w3.org/ns/wsdl}documentation) inherited from {http://www.w3.org/ns/wsdl}DocumentedType
    
    # Attribute ref uses Python identifier ref
    __ref = pyxb.binding.content.AttributeUse(pyxb.namespace.ExpandedName(None, 'ref'), 'ref', '__httpwww_w3_orgnswsdl_BindingOperationFaultType_ref', pyxb.binding.datatypes.QName, required=True)
    __ref._DeclarationLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wsdl20.xsd', 298, 8)
    __ref._UseLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wsdl20.xsd', 298, 8)
    
    ref = property(__ref.value, __ref.set, None, None)

    
    # Attribute messageLabel uses Python identifier messageLabel
    __messageLabel = pyxb.binding.content.AttributeUse(pyxb.namespace.ExpandedName(None, 'messageLabel'), 'messageLabel', '__httpwww_w3_orgnswsdl_BindingOperationFaultType_messageLabel', pyxb.binding.datatypes.NCName)
    __messageLabel._DeclarationLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wsdl20.xsd', 299, 8)
    __messageLabel._UseLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wsdl20.xsd', 299, 8)
    
    messageLabel = property(__messageLabel.value, __messageLabel.set, None, None)

    _AttributeWildcard = pyxb.binding.content.Wildcard(process_contents=pyxb.binding.content.Wildcard.PC_lax, namespace_constraint=(pyxb.binding.content.Wildcard.NC_not, 'http://www.w3.org/ns/wsdl'))
    _HasWildcardElement = True
    _ElementMap.update({
        
    })
    _AttributeMap.update({
        __ref.name() : __ref,
        __messageLabel.name() : __messageLabel
    })
Namespace.addCategoryObject('typeBinding', 'BindingOperationFaultType', BindingOperationFaultType)


# Complex type {http://www.w3.org/ns/wsdl}BindingFaultType with content type ELEMENT_ONLY
class BindingFaultType (ExtensibleDocumentedType):
    """Complex type {http://www.w3.org/ns/wsdl}BindingFaultType with content type ELEMENT_ONLY"""
    _TypeDefinition = None
    _ContentTypeTag = pyxb.binding.basis.complexTypeDefinition._CT_ELEMENT_ONLY
    _Abstract = False
    _ExpandedName = pyxb.namespace.ExpandedName(Namespace, 'BindingFaultType')
    _XSDLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wsdl20.xsd', 304, 2)
    _ElementMap = ExtensibleDocumentedType._ElementMap.copy()
    _AttributeMap = ExtensibleDocumentedType._AttributeMap.copy()
    # Base type is ExtensibleDocumentedType
    
    # Element documentation ({http://www.w3.org/ns/wsdl}documentation) inherited from {http://www.w3.org/ns/wsdl}DocumentedType
    
    # Attribute ref uses Python identifier ref
    __ref = pyxb.binding.content.AttributeUse(pyxb.namespace.ExpandedName(None, 'ref'), 'ref', '__httpwww_w3_orgnswsdl_BindingFaultType_ref', pyxb.binding.datatypes.QName, required=True)
    __ref._DeclarationLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wsdl20.xsd', 310, 8)
    __ref._UseLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wsdl20.xsd', 310, 8)
    
    ref = property(__ref.value, __ref.set, None, None)

    _AttributeWildcard = pyxb.binding.content.Wildcard(process_contents=pyxb.binding.content.Wildcard.PC_lax, namespace_constraint=(pyxb.binding.content.Wildcard.NC_not, 'http://www.w3.org/ns/wsdl'))
    _HasWildcardElement = True
    _ElementMap.update({
        
    })
    _AttributeMap.update({
        __ref.name() : __ref
    })
Namespace.addCategoryObject('typeBinding', 'BindingFaultType', BindingFaultType)


# Complex type {http://www.w3.org/ns/wsdl}ServiceType with content type ELEMENT_ONLY
class ServiceType (ExtensibleDocumentedType):
    """Complex type {http://www.w3.org/ns/wsdl}ServiceType with content type ELEMENT_ONLY"""
    _TypeDefinition = None
    _ContentTypeTag = pyxb.binding.basis.complexTypeDefinition._CT_ELEMENT_ONLY
    _Abstract = False
    _ExpandedName = pyxb.namespace.ExpandedName(Namespace, 'ServiceType')
    _XSDLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wsdl20.xsd', 322, 2)
    _ElementMap = ExtensibleDocumentedType._ElementMap.copy()
    _AttributeMap = ExtensibleDocumentedType._AttributeMap.copy()
    # Base type is ExtensibleDocumentedType
    
    # Element documentation ({http://www.w3.org/ns/wsdl}documentation) inherited from {http://www.w3.org/ns/wsdl}DocumentedType
    
    # Element {http://www.w3.org/ns/wsdl}endpoint uses Python identifier endpoint
    __endpoint = pyxb.binding.content.ElementDeclaration(pyxb.namespace.ExpandedName(Namespace, 'endpoint'), 'endpoint', '__httpwww_w3_orgnswsdl_ServiceType_httpwww_w3_orgnswsdlendpoint', True, pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wsdl20.xsd', 335, 2), )

    
    endpoint = property(__endpoint.value, __endpoint.set, None, None)

    
    # Attribute name uses Python identifier name
    __name = pyxb.binding.content.AttributeUse(pyxb.namespace.ExpandedName(None, 'name'), 'name', '__httpwww_w3_orgnswsdl_ServiceType_name', pyxb.binding.datatypes.NCName, required=True)
    __name._DeclarationLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wsdl20.xsd', 329, 8)
    __name._UseLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wsdl20.xsd', 329, 8)
    
    name = property(__name.value, __name.set, None, None)

    
    # Attribute interface uses Python identifier interface
    __interface = pyxb.binding.content.AttributeUse(pyxb.namespace.ExpandedName(None, 'interface'), 'interface', '__httpwww_w3_orgnswsdl_ServiceType_interface', pyxb.binding.datatypes.QName, required=True)
    __interface._DeclarationLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wsdl20.xsd', 330, 8)
    __interface._UseLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wsdl20.xsd', 330, 8)
    
    interface = property(__interface.value, __interface.set, None, None)

    _AttributeWildcard = pyxb.binding.content.Wildcard(process_contents=pyxb.binding.content.Wildcard.PC_lax, namespace_constraint=(pyxb.binding.content.Wildcard.NC_not, 'http://www.w3.org/ns/wsdl'))
    _HasWildcardElement = True
    _ElementMap.update({
        __endpoint.name() : __endpoint
    })
    _AttributeMap.update({
        __name.name() : __name,
        __interface.name() : __interface
    })
Namespace.addCategoryObject('typeBinding', 'ServiceType', ServiceType)


# Complex type {http://www.w3.org/ns/wsdl}EndpointType with content type ELEMENT_ONLY
class EndpointType (ExtensibleDocumentedType):
    """Complex type {http://www.w3.org/ns/wsdl}EndpointType with content type ELEMENT_ONLY"""
    _TypeDefinition = None
    _ContentTypeTag = pyxb.binding.basis.complexTypeDefinition._CT_ELEMENT_ONLY
    _Abstract = False
    _ExpandedName = pyxb.namespace.ExpandedName(Namespace, 'EndpointType')
    _XSDLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wsdl20.xsd', 336, 2)
    _ElementMap = ExtensibleDocumentedType._ElementMap.copy()
    _AttributeMap = ExtensibleDocumentedType._AttributeMap.copy()
    # Base type is ExtensibleDocumentedType
    
    # Element documentation ({http://www.w3.org/ns/wsdl}documentation) inherited from {http://www.w3.org/ns/wsdl}DocumentedType
    
    # Attribute name uses Python identifier name
    __name = pyxb.binding.content.AttributeUse(pyxb.namespace.ExpandedName(None, 'name'), 'name', '__httpwww_w3_orgnswsdl_EndpointType_name', pyxb.binding.datatypes.NCName, required=True)
    __name._DeclarationLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wsdl20.xsd', 342, 8)
    __name._UseLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wsdl20.xsd', 342, 8)
    
    name = property(__name.value, __name.set, None, None)

    
    # Attribute binding uses Python identifier binding
    __binding = pyxb.binding.content.AttributeUse(pyxb.namespace.ExpandedName(None, 'binding'), 'binding', '__httpwww_w3_orgnswsdl_EndpointType_binding', pyxb.binding.datatypes.QName, required=True)
    __binding._DeclarationLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wsdl20.xsd', 343, 8)
    __binding._UseLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wsdl20.xsd', 343, 8)
    
    binding = property(__binding.value, __binding.set, None, None)

    
    # Attribute address uses Python identifier address
    __address = pyxb.binding.content.AttributeUse(pyxb.namespace.ExpandedName(None, 'address'), 'address', '__httpwww_w3_orgnswsdl_EndpointType_address', pyxb.binding.datatypes.anyURI)
    __address._DeclarationLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wsdl20.xsd', 344, 8)
    __address._UseLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wsdl20.xsd', 344, 8)
    
    address = property(__address.value, __address.set, None, None)

    _AttributeWildcard = pyxb.binding.content.Wildcard(process_contents=pyxb.binding.content.Wildcard.PC_lax, namespace_constraint=(pyxb.binding.content.Wildcard.NC_not, 'http://www.w3.org/ns/wsdl'))
    _HasWildcardElement = True
    _ElementMap.update({
        
    })
    _AttributeMap.update({
        __name.name() : __name,
        __binding.name() : __binding,
        __address.name() : __address
    })
Namespace.addCategoryObject('typeBinding', 'EndpointType', EndpointType)


documentation = pyxb.binding.basis.element(pyxb.namespace.ExpandedName(Namespace, 'documentation'), DocumentationType, location=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wsdl20.xsd', 22, 2))
Namespace.addCategoryObject('elementBinding', documentation.name().localName(), documentation)

description = pyxb.binding.basis.element(pyxb.namespace.ExpandedName(Namespace, 'description'), DescriptionType, location=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wsdl20.xsd', 56, 2))
Namespace.addCategoryObject('elementBinding', description.name().localName(), description)

import_ = pyxb.binding.basis.element(pyxb.namespace.ExpandedName(Namespace, 'import'), ImportType, location=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wsdl20.xsd', 105, 2))
Namespace.addCategoryObject('elementBinding', import_.name().localName(), import_)

include = pyxb.binding.basis.element(pyxb.namespace.ExpandedName(Namespace, 'include'), IncludeType, location=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wsdl20.xsd', 118, 2))
Namespace.addCategoryObject('elementBinding', include.name().localName(), include)

types = pyxb.binding.basis.element(pyxb.namespace.ExpandedName(Namespace, 'types'), TypesType, location=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wsdl20.xsd', 130, 2))
Namespace.addCategoryObject('elementBinding', types.name().localName(), types)

interface = pyxb.binding.basis.element(pyxb.namespace.ExpandedName(Namespace, 'interface'), InterfaceType, location=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wsdl20.xsd', 142, 2))
Namespace.addCategoryObject('elementBinding', interface.name().localName(), interface)

binding = pyxb.binding.basis.element(pyxb.namespace.ExpandedName(Namespace, 'binding'), BindingType, location=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wsdl20.xsd', 250, 2))
Namespace.addCategoryObject('elementBinding', binding.name().localName(), binding)

service = pyxb.binding.basis.element(pyxb.namespace.ExpandedName(Namespace, 'service'), ServiceType, location=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wsdl20.xsd', 316, 2))
Namespace.addCategoryObject('elementBinding', service.name().localName(), service)

endpoint = pyxb.binding.basis.element(pyxb.namespace.ExpandedName(Namespace, 'endpoint'), EndpointType, location=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wsdl20.xsd', 335, 2))
Namespace.addCategoryObject('elementBinding', endpoint.name().localName(), endpoint)



def _BuildAutomaton ():
    # Remove this helper function from the namespace after it is invoked
    global _BuildAutomaton
    del _BuildAutomaton
    import pyxb.utils.fac as fac

    counters = set()
    cc_0 = fac.CounterCondition(min=0, max=None, metadata=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wsdl20.xsd', 25, 6))
    counters.add(cc_0)
    states = []
    final_update = set()
    final_update.add(fac.UpdateInstruction(cc_0, False))
    symbol = pyxb.binding.content.WildcardUse(pyxb.binding.content.Wildcard(process_contents=pyxb.binding.content.Wildcard.PC_lax, namespace_constraint=pyxb.binding.content.Wildcard.NC_any), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wsdl20.xsd', 25, 6))
    st_0 = fac.State(symbol, is_initial=True, final_update=final_update, is_unordered_catenation=False)
    states.append(st_0)
    transitions = []
    transitions.append(fac.Transition(st_0, [
        fac.UpdateInstruction(cc_0, True) ]))
    st_0._set_transitionSet(transitions)
    return fac.Automaton(states, counters, True, containing_state=None)
DocumentationType._Automaton = _BuildAutomaton()




DocumentedType._AddElement(pyxb.binding.basis.element(pyxb.namespace.ExpandedName(Namespace, 'documentation'), DocumentationType, scope=DocumentedType, location=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wsdl20.xsd', 22, 2)))

def _BuildAutomaton_ ():
    # Remove this helper function from the namespace after it is invoked
    global _BuildAutomaton_
    del _BuildAutomaton_
    import pyxb.utils.fac as fac

    counters = set()
    cc_0 = fac.CounterCondition(min=0, max=None, metadata=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wsdl20.xsd', 37, 6))
    counters.add(cc_0)
    states = []
    final_update = set()
    final_update.add(fac.UpdateInstruction(cc_0, False))
    symbol = pyxb.binding.content.ElementUse(DocumentedType._UseForTag(pyxb.namespace.ExpandedName(Namespace, 'documentation')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wsdl20.xsd', 37, 6))
    st_0 = fac.State(symbol, is_initial=True, final_update=final_update, is_unordered_catenation=False)
    states.append(st_0)
    transitions = []
    transitions.append(fac.Transition(st_0, [
        fac.UpdateInstruction(cc_0, True) ]))
    st_0._set_transitionSet(transitions)
    return fac.Automaton(states, counters, True, containing_state=None)
DocumentedType._Automaton = _BuildAutomaton_()




def _BuildAutomaton_2 ():
    # Remove this helper function from the namespace after it is invoked
    global _BuildAutomaton_2
    del _BuildAutomaton_2
    import pyxb.utils.fac as fac

    counters = set()
    cc_0 = fac.CounterCondition(min=0, max=None, metadata=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wsdl20.xsd', 37, 6))
    counters.add(cc_0)
    states = []
    final_update = set()
    final_update.add(fac.UpdateInstruction(cc_0, False))
    symbol = pyxb.binding.content.ElementUse(ExtensibleDocumentedType._UseForTag(pyxb.namespace.ExpandedName(Namespace, 'documentation')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wsdl20.xsd', 37, 6))
    st_0 = fac.State(symbol, is_initial=True, final_update=final_update, is_unordered_catenation=False)
    states.append(st_0)
    transitions = []
    transitions.append(fac.Transition(st_0, [
        fac.UpdateInstruction(cc_0, True) ]))
    st_0._set_transitionSet(transitions)
    return fac.Automaton(states, counters, True, containing_state=None)
ExtensibleDocumentedType._Automaton = _BuildAutomaton_2()




DescriptionType._AddElement(pyxb.binding.basis.element(pyxb.namespace.ExpandedName(Namespace, 'import'), ImportType, scope=DescriptionType, location=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wsdl20.xsd', 105, 2)))

DescriptionType._AddElement(pyxb.binding.basis.element(pyxb.namespace.ExpandedName(Namespace, 'include'), IncludeType, scope=DescriptionType, location=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wsdl20.xsd', 118, 2)))

DescriptionType._AddElement(pyxb.binding.basis.element(pyxb.namespace.ExpandedName(Namespace, 'types'), TypesType, scope=DescriptionType, location=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wsdl20.xsd', 130, 2)))

DescriptionType._AddElement(pyxb.binding.basis.element(pyxb.namespace.ExpandedName(Namespace, 'interface'), InterfaceType, scope=DescriptionType, location=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wsdl20.xsd', 142, 2)))

DescriptionType._AddElement(pyxb.binding.basis.element(pyxb.namespace.ExpandedName(Namespace, 'binding'), BindingType, scope=DescriptionType, location=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wsdl20.xsd', 250, 2)))

DescriptionType._AddElement(pyxb.binding.basis.element(pyxb.namespace.ExpandedName(Namespace, 'service'), ServiceType, scope=DescriptionType, location=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wsdl20.xsd', 316, 2)))

def _BuildAutomaton_3 ():
    # Remove this helper function from the namespace after it is invoked
    global _BuildAutomaton_3
    del _BuildAutomaton_3
    import pyxb.utils.fac as fac

    counters = set()
    cc_0 = fac.CounterCondition(min=0, max=None, metadata=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wsdl20.xsd', 37, 6))
    counters.add(cc_0)
    cc_1 = fac.CounterCondition(min=0, max=None, metadata=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wsdl20.xsd', 90, 8))
    counters.add(cc_1)
    states = []
    final_update = set()
    final_update.add(fac.UpdateInstruction(cc_0, False))
    symbol = pyxb.binding.content.ElementUse(DescriptionType._UseForTag(pyxb.namespace.ExpandedName(Namespace, 'documentation')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wsdl20.xsd', 37, 6))
    st_0 = fac.State(symbol, is_initial=True, final_update=final_update, is_unordered_catenation=False)
    states.append(st_0)
    final_update = set()
    final_update.add(fac.UpdateInstruction(cc_1, False))
    symbol = pyxb.binding.content.ElementUse(DescriptionType._UseForTag(pyxb.namespace.ExpandedName(Namespace, 'import')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wsdl20.xsd', 91, 10))
    st_1 = fac.State(symbol, is_initial=True, final_update=final_update, is_unordered_catenation=False)
    states.append(st_1)
    final_update = set()
    final_update.add(fac.UpdateInstruction(cc_1, False))
    symbol = pyxb.binding.content.ElementUse(DescriptionType._UseForTag(pyxb.namespace.ExpandedName(Namespace, 'include')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wsdl20.xsd', 92, 10))
    st_2 = fac.State(symbol, is_initial=True, final_update=final_update, is_unordered_catenation=False)
    states.append(st_2)
    final_update = set()
    final_update.add(fac.UpdateInstruction(cc_1, False))
    symbol = pyxb.binding.content.ElementUse(DescriptionType._UseForTag(pyxb.namespace.ExpandedName(Namespace, 'types')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wsdl20.xsd', 93, 10))
    st_3 = fac.State(symbol, is_initial=True, final_update=final_update, is_unordered_catenation=False)
    states.append(st_3)
    final_update = set()
    final_update.add(fac.UpdateInstruction(cc_1, False))
    symbol = pyxb.binding.content.ElementUse(DescriptionType._UseForTag(pyxb.namespace.ExpandedName(Namespace, 'interface')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wsdl20.xsd', 94, 10))
    st_4 = fac.State(symbol, is_initial=True, final_update=final_update, is_unordered_catenation=False)
    states.append(st_4)
    final_update = set()
    final_update.add(fac.UpdateInstruction(cc_1, False))
    symbol = pyxb.binding.content.ElementUse(DescriptionType._UseForTag(pyxb.namespace.ExpandedName(Namespace, 'binding')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wsdl20.xsd', 95, 7))
    st_5 = fac.State(symbol, is_initial=True, final_update=final_update, is_unordered_catenation=False)
    states.append(st_5)
    final_update = set()
    final_update.add(fac.UpdateInstruction(cc_1, False))
    symbol = pyxb.binding.content.ElementUse(DescriptionType._UseForTag(pyxb.namespace.ExpandedName(Namespace, 'service')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wsdl20.xsd', 96, 7))
    st_6 = fac.State(symbol, is_initial=True, final_update=final_update, is_unordered_catenation=False)
    states.append(st_6)
    final_update = set()
    final_update.add(fac.UpdateInstruction(cc_1, False))
    symbol = pyxb.binding.content.WildcardUse(pyxb.binding.content.Wildcard(process_contents=pyxb.binding.content.Wildcard.PC_lax, namespace_constraint=(pyxb.binding.content.Wildcard.NC_not, 'http://www.w3.org/ns/wsdl')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wsdl20.xsd', 97, 10))
    st_7 = fac.State(symbol, is_initial=True, final_update=final_update, is_unordered_catenation=False)
    states.append(st_7)
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
    st_0._set_transitionSet(transitions)
    transitions = []
    transitions.append(fac.Transition(st_1, [
        fac.UpdateInstruction(cc_1, True) ]))
    transitions.append(fac.Transition(st_2, [
        fac.UpdateInstruction(cc_1, True) ]))
    transitions.append(fac.Transition(st_3, [
        fac.UpdateInstruction(cc_1, True) ]))
    transitions.append(fac.Transition(st_4, [
        fac.UpdateInstruction(cc_1, True) ]))
    transitions.append(fac.Transition(st_5, [
        fac.UpdateInstruction(cc_1, True) ]))
    transitions.append(fac.Transition(st_6, [
        fac.UpdateInstruction(cc_1, True) ]))
    transitions.append(fac.Transition(st_7, [
        fac.UpdateInstruction(cc_1, True) ]))
    st_1._set_transitionSet(transitions)
    transitions = []
    transitions.append(fac.Transition(st_1, [
        fac.UpdateInstruction(cc_1, True) ]))
    transitions.append(fac.Transition(st_2, [
        fac.UpdateInstruction(cc_1, True) ]))
    transitions.append(fac.Transition(st_3, [
        fac.UpdateInstruction(cc_1, True) ]))
    transitions.append(fac.Transition(st_4, [
        fac.UpdateInstruction(cc_1, True) ]))
    transitions.append(fac.Transition(st_5, [
        fac.UpdateInstruction(cc_1, True) ]))
    transitions.append(fac.Transition(st_6, [
        fac.UpdateInstruction(cc_1, True) ]))
    transitions.append(fac.Transition(st_7, [
        fac.UpdateInstruction(cc_1, True) ]))
    st_2._set_transitionSet(transitions)
    transitions = []
    transitions.append(fac.Transition(st_1, [
        fac.UpdateInstruction(cc_1, True) ]))
    transitions.append(fac.Transition(st_2, [
        fac.UpdateInstruction(cc_1, True) ]))
    transitions.append(fac.Transition(st_3, [
        fac.UpdateInstruction(cc_1, True) ]))
    transitions.append(fac.Transition(st_4, [
        fac.UpdateInstruction(cc_1, True) ]))
    transitions.append(fac.Transition(st_5, [
        fac.UpdateInstruction(cc_1, True) ]))
    transitions.append(fac.Transition(st_6, [
        fac.UpdateInstruction(cc_1, True) ]))
    transitions.append(fac.Transition(st_7, [
        fac.UpdateInstruction(cc_1, True) ]))
    st_3._set_transitionSet(transitions)
    transitions = []
    transitions.append(fac.Transition(st_1, [
        fac.UpdateInstruction(cc_1, True) ]))
    transitions.append(fac.Transition(st_2, [
        fac.UpdateInstruction(cc_1, True) ]))
    transitions.append(fac.Transition(st_3, [
        fac.UpdateInstruction(cc_1, True) ]))
    transitions.append(fac.Transition(st_4, [
        fac.UpdateInstruction(cc_1, True) ]))
    transitions.append(fac.Transition(st_5, [
        fac.UpdateInstruction(cc_1, True) ]))
    transitions.append(fac.Transition(st_6, [
        fac.UpdateInstruction(cc_1, True) ]))
    transitions.append(fac.Transition(st_7, [
        fac.UpdateInstruction(cc_1, True) ]))
    st_4._set_transitionSet(transitions)
    transitions = []
    transitions.append(fac.Transition(st_1, [
        fac.UpdateInstruction(cc_1, True) ]))
    transitions.append(fac.Transition(st_2, [
        fac.UpdateInstruction(cc_1, True) ]))
    transitions.append(fac.Transition(st_3, [
        fac.UpdateInstruction(cc_1, True) ]))
    transitions.append(fac.Transition(st_4, [
        fac.UpdateInstruction(cc_1, True) ]))
    transitions.append(fac.Transition(st_5, [
        fac.UpdateInstruction(cc_1, True) ]))
    transitions.append(fac.Transition(st_6, [
        fac.UpdateInstruction(cc_1, True) ]))
    transitions.append(fac.Transition(st_7, [
        fac.UpdateInstruction(cc_1, True) ]))
    st_5._set_transitionSet(transitions)
    transitions = []
    transitions.append(fac.Transition(st_1, [
        fac.UpdateInstruction(cc_1, True) ]))
    transitions.append(fac.Transition(st_2, [
        fac.UpdateInstruction(cc_1, True) ]))
    transitions.append(fac.Transition(st_3, [
        fac.UpdateInstruction(cc_1, True) ]))
    transitions.append(fac.Transition(st_4, [
        fac.UpdateInstruction(cc_1, True) ]))
    transitions.append(fac.Transition(st_5, [
        fac.UpdateInstruction(cc_1, True) ]))
    transitions.append(fac.Transition(st_6, [
        fac.UpdateInstruction(cc_1, True) ]))
    transitions.append(fac.Transition(st_7, [
        fac.UpdateInstruction(cc_1, True) ]))
    st_6._set_transitionSet(transitions)
    transitions = []
    transitions.append(fac.Transition(st_1, [
        fac.UpdateInstruction(cc_1, True) ]))
    transitions.append(fac.Transition(st_2, [
        fac.UpdateInstruction(cc_1, True) ]))
    transitions.append(fac.Transition(st_3, [
        fac.UpdateInstruction(cc_1, True) ]))
    transitions.append(fac.Transition(st_4, [
        fac.UpdateInstruction(cc_1, True) ]))
    transitions.append(fac.Transition(st_5, [
        fac.UpdateInstruction(cc_1, True) ]))
    transitions.append(fac.Transition(st_6, [
        fac.UpdateInstruction(cc_1, True) ]))
    transitions.append(fac.Transition(st_7, [
        fac.UpdateInstruction(cc_1, True) ]))
    st_7._set_transitionSet(transitions)
    return fac.Automaton(states, counters, True, containing_state=None)
DescriptionType._Automaton = _BuildAutomaton_3()




def _BuildAutomaton_4 ():
    # Remove this helper function from the namespace after it is invoked
    global _BuildAutomaton_4
    del _BuildAutomaton_4
    import pyxb.utils.fac as fac

    counters = set()
    cc_0 = fac.CounterCondition(min=0, max=None, metadata=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wsdl20.xsd', 37, 6))
    counters.add(cc_0)
    cc_1 = fac.CounterCondition(min=0, max=None, metadata=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wsdl20.xsd', 110, 7))
    counters.add(cc_1)
    states = []
    final_update = set()
    final_update.add(fac.UpdateInstruction(cc_0, False))
    symbol = pyxb.binding.content.ElementUse(ImportType._UseForTag(pyxb.namespace.ExpandedName(Namespace, 'documentation')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wsdl20.xsd', 37, 6))
    st_0 = fac.State(symbol, is_initial=True, final_update=final_update, is_unordered_catenation=False)
    states.append(st_0)
    final_update = set()
    final_update.add(fac.UpdateInstruction(cc_1, False))
    symbol = pyxb.binding.content.WildcardUse(pyxb.binding.content.Wildcard(process_contents=pyxb.binding.content.Wildcard.PC_strict, namespace_constraint=(pyxb.binding.content.Wildcard.NC_not, 'http://www.w3.org/ns/wsdl')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wsdl20.xsd', 110, 7))
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
ImportType._Automaton = _BuildAutomaton_4()




def _BuildAutomaton_5 ():
    # Remove this helper function from the namespace after it is invoked
    global _BuildAutomaton_5
    del _BuildAutomaton_5
    import pyxb.utils.fac as fac

    counters = set()
    cc_0 = fac.CounterCondition(min=0, max=None, metadata=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wsdl20.xsd', 37, 6))
    counters.add(cc_0)
    cc_1 = fac.CounterCondition(min=0, max=None, metadata=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wsdl20.xsd', 123, 7))
    counters.add(cc_1)
    states = []
    final_update = set()
    final_update.add(fac.UpdateInstruction(cc_0, False))
    symbol = pyxb.binding.content.ElementUse(IncludeType._UseForTag(pyxb.namespace.ExpandedName(Namespace, 'documentation')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wsdl20.xsd', 37, 6))
    st_0 = fac.State(symbol, is_initial=True, final_update=final_update, is_unordered_catenation=False)
    states.append(st_0)
    final_update = set()
    final_update.add(fac.UpdateInstruction(cc_1, False))
    symbol = pyxb.binding.content.WildcardUse(pyxb.binding.content.Wildcard(process_contents=pyxb.binding.content.Wildcard.PC_strict, namespace_constraint=(pyxb.binding.content.Wildcard.NC_not, 'http://www.w3.org/ns/wsdl')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wsdl20.xsd', 123, 7))
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
IncludeType._Automaton = _BuildAutomaton_5()




def _BuildAutomaton_6 ():
    # Remove this helper function from the namespace after it is invoked
    global _BuildAutomaton_6
    del _BuildAutomaton_6
    import pyxb.utils.fac as fac

    counters = set()
    cc_0 = fac.CounterCondition(min=0, max=None, metadata=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wsdl20.xsd', 37, 6))
    counters.add(cc_0)
    cc_1 = fac.CounterCondition(min=0, max=None, metadata=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wsdl20.xsd', 135, 7))
    counters.add(cc_1)
    states = []
    final_update = set()
    final_update.add(fac.UpdateInstruction(cc_0, False))
    symbol = pyxb.binding.content.ElementUse(TypesType._UseForTag(pyxb.namespace.ExpandedName(Namespace, 'documentation')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wsdl20.xsd', 37, 6))
    st_0 = fac.State(symbol, is_initial=True, final_update=final_update, is_unordered_catenation=False)
    states.append(st_0)
    final_update = set()
    final_update.add(fac.UpdateInstruction(cc_1, False))
    symbol = pyxb.binding.content.WildcardUse(pyxb.binding.content.Wildcard(process_contents=pyxb.binding.content.Wildcard.PC_strict, namespace_constraint=(pyxb.binding.content.Wildcard.NC_not, 'http://www.w3.org/ns/wsdl')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wsdl20.xsd', 135, 7))
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
TypesType._Automaton = _BuildAutomaton_6()




InterfaceType._AddElement(pyxb.binding.basis.element(pyxb.namespace.ExpandedName(Namespace, 'operation'), InterfaceOperationType, scope=InterfaceType, location=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wsdl20.xsd', 156, 10)))

InterfaceType._AddElement(pyxb.binding.basis.element(pyxb.namespace.ExpandedName(Namespace, 'fault'), InterfaceFaultType, scope=InterfaceType, location=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wsdl20.xsd', 157, 10)))

def _BuildAutomaton_7 ():
    # Remove this helper function from the namespace after it is invoked
    global _BuildAutomaton_7
    del _BuildAutomaton_7
    import pyxb.utils.fac as fac

    counters = set()
    cc_0 = fac.CounterCondition(min=0, max=None, metadata=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wsdl20.xsd', 37, 6))
    counters.add(cc_0)
    cc_1 = fac.CounterCondition(min=0, max=None, metadata=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wsdl20.xsd', 155, 8))
    counters.add(cc_1)
    states = []
    final_update = set()
    final_update.add(fac.UpdateInstruction(cc_0, False))
    symbol = pyxb.binding.content.ElementUse(InterfaceType._UseForTag(pyxb.namespace.ExpandedName(Namespace, 'documentation')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wsdl20.xsd', 37, 6))
    st_0 = fac.State(symbol, is_initial=True, final_update=final_update, is_unordered_catenation=False)
    states.append(st_0)
    final_update = set()
    final_update.add(fac.UpdateInstruction(cc_1, False))
    symbol = pyxb.binding.content.ElementUse(InterfaceType._UseForTag(pyxb.namespace.ExpandedName(Namespace, 'operation')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wsdl20.xsd', 156, 10))
    st_1 = fac.State(symbol, is_initial=True, final_update=final_update, is_unordered_catenation=False)
    states.append(st_1)
    final_update = set()
    final_update.add(fac.UpdateInstruction(cc_1, False))
    symbol = pyxb.binding.content.ElementUse(InterfaceType._UseForTag(pyxb.namespace.ExpandedName(Namespace, 'fault')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wsdl20.xsd', 157, 10))
    st_2 = fac.State(symbol, is_initial=True, final_update=final_update, is_unordered_catenation=False)
    states.append(st_2)
    final_update = set()
    final_update.add(fac.UpdateInstruction(cc_1, False))
    symbol = pyxb.binding.content.WildcardUse(pyxb.binding.content.Wildcard(process_contents=pyxb.binding.content.Wildcard.PC_lax, namespace_constraint=(pyxb.binding.content.Wildcard.NC_not, 'http://www.w3.org/ns/wsdl')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wsdl20.xsd', 158, 10))
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
        fac.UpdateInstruction(cc_1, True) ]))
    transitions.append(fac.Transition(st_3, [
        fac.UpdateInstruction(cc_1, True) ]))
    st_1._set_transitionSet(transitions)
    transitions = []
    transitions.append(fac.Transition(st_1, [
        fac.UpdateInstruction(cc_1, True) ]))
    transitions.append(fac.Transition(st_2, [
        fac.UpdateInstruction(cc_1, True) ]))
    transitions.append(fac.Transition(st_3, [
        fac.UpdateInstruction(cc_1, True) ]))
    st_2._set_transitionSet(transitions)
    transitions = []
    transitions.append(fac.Transition(st_1, [
        fac.UpdateInstruction(cc_1, True) ]))
    transitions.append(fac.Transition(st_2, [
        fac.UpdateInstruction(cc_1, True) ]))
    transitions.append(fac.Transition(st_3, [
        fac.UpdateInstruction(cc_1, True) ]))
    st_3._set_transitionSet(transitions)
    return fac.Automaton(states, counters, True, containing_state=None)
InterfaceType._Automaton = _BuildAutomaton_7()




InterfaceOperationType._AddElement(pyxb.binding.basis.element(pyxb.namespace.ExpandedName(Namespace, 'input'), MessageRefType, scope=InterfaceOperationType, location=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wsdl20.xsd', 179, 10)))

InterfaceOperationType._AddElement(pyxb.binding.basis.element(pyxb.namespace.ExpandedName(Namespace, 'output'), MessageRefType, scope=InterfaceOperationType, location=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wsdl20.xsd', 180, 10)))

InterfaceOperationType._AddElement(pyxb.binding.basis.element(pyxb.namespace.ExpandedName(Namespace, 'infault'), MessageRefFaultType, scope=InterfaceOperationType, location=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wsdl20.xsd', 181, 10)))

InterfaceOperationType._AddElement(pyxb.binding.basis.element(pyxb.namespace.ExpandedName(Namespace, 'outfault'), MessageRefFaultType, scope=InterfaceOperationType, location=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wsdl20.xsd', 182, 10)))

def _BuildAutomaton_8 ():
    # Remove this helper function from the namespace after it is invoked
    global _BuildAutomaton_8
    del _BuildAutomaton_8
    import pyxb.utils.fac as fac

    counters = set()
    cc_0 = fac.CounterCondition(min=0, max=None, metadata=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wsdl20.xsd', 37, 6))
    counters.add(cc_0)
    cc_1 = fac.CounterCondition(min=0, max=None, metadata=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wsdl20.xsd', 178, 8))
    counters.add(cc_1)
    states = []
    final_update = set()
    final_update.add(fac.UpdateInstruction(cc_0, False))
    symbol = pyxb.binding.content.ElementUse(InterfaceOperationType._UseForTag(pyxb.namespace.ExpandedName(Namespace, 'documentation')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wsdl20.xsd', 37, 6))
    st_0 = fac.State(symbol, is_initial=True, final_update=final_update, is_unordered_catenation=False)
    states.append(st_0)
    final_update = set()
    final_update.add(fac.UpdateInstruction(cc_1, False))
    symbol = pyxb.binding.content.ElementUse(InterfaceOperationType._UseForTag(pyxb.namespace.ExpandedName(Namespace, 'input')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wsdl20.xsd', 179, 10))
    st_1 = fac.State(symbol, is_initial=True, final_update=final_update, is_unordered_catenation=False)
    states.append(st_1)
    final_update = set()
    final_update.add(fac.UpdateInstruction(cc_1, False))
    symbol = pyxb.binding.content.ElementUse(InterfaceOperationType._UseForTag(pyxb.namespace.ExpandedName(Namespace, 'output')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wsdl20.xsd', 180, 10))
    st_2 = fac.State(symbol, is_initial=True, final_update=final_update, is_unordered_catenation=False)
    states.append(st_2)
    final_update = set()
    final_update.add(fac.UpdateInstruction(cc_1, False))
    symbol = pyxb.binding.content.ElementUse(InterfaceOperationType._UseForTag(pyxb.namespace.ExpandedName(Namespace, 'infault')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wsdl20.xsd', 181, 10))
    st_3 = fac.State(symbol, is_initial=True, final_update=final_update, is_unordered_catenation=False)
    states.append(st_3)
    final_update = set()
    final_update.add(fac.UpdateInstruction(cc_1, False))
    symbol = pyxb.binding.content.ElementUse(InterfaceOperationType._UseForTag(pyxb.namespace.ExpandedName(Namespace, 'outfault')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wsdl20.xsd', 182, 10))
    st_4 = fac.State(symbol, is_initial=True, final_update=final_update, is_unordered_catenation=False)
    states.append(st_4)
    final_update = set()
    final_update.add(fac.UpdateInstruction(cc_1, False))
    symbol = pyxb.binding.content.WildcardUse(pyxb.binding.content.Wildcard(process_contents=pyxb.binding.content.Wildcard.PC_lax, namespace_constraint=(pyxb.binding.content.Wildcard.NC_not, 'http://www.w3.org/ns/wsdl')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wsdl20.xsd', 183, 10))
    st_5 = fac.State(symbol, is_initial=True, final_update=final_update, is_unordered_catenation=False)
    states.append(st_5)
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
    st_0._set_transitionSet(transitions)
    transitions = []
    transitions.append(fac.Transition(st_1, [
        fac.UpdateInstruction(cc_1, True) ]))
    transitions.append(fac.Transition(st_2, [
        fac.UpdateInstruction(cc_1, True) ]))
    transitions.append(fac.Transition(st_3, [
        fac.UpdateInstruction(cc_1, True) ]))
    transitions.append(fac.Transition(st_4, [
        fac.UpdateInstruction(cc_1, True) ]))
    transitions.append(fac.Transition(st_5, [
        fac.UpdateInstruction(cc_1, True) ]))
    st_1._set_transitionSet(transitions)
    transitions = []
    transitions.append(fac.Transition(st_1, [
        fac.UpdateInstruction(cc_1, True) ]))
    transitions.append(fac.Transition(st_2, [
        fac.UpdateInstruction(cc_1, True) ]))
    transitions.append(fac.Transition(st_3, [
        fac.UpdateInstruction(cc_1, True) ]))
    transitions.append(fac.Transition(st_4, [
        fac.UpdateInstruction(cc_1, True) ]))
    transitions.append(fac.Transition(st_5, [
        fac.UpdateInstruction(cc_1, True) ]))
    st_2._set_transitionSet(transitions)
    transitions = []
    transitions.append(fac.Transition(st_1, [
        fac.UpdateInstruction(cc_1, True) ]))
    transitions.append(fac.Transition(st_2, [
        fac.UpdateInstruction(cc_1, True) ]))
    transitions.append(fac.Transition(st_3, [
        fac.UpdateInstruction(cc_1, True) ]))
    transitions.append(fac.Transition(st_4, [
        fac.UpdateInstruction(cc_1, True) ]))
    transitions.append(fac.Transition(st_5, [
        fac.UpdateInstruction(cc_1, True) ]))
    st_3._set_transitionSet(transitions)
    transitions = []
    transitions.append(fac.Transition(st_1, [
        fac.UpdateInstruction(cc_1, True) ]))
    transitions.append(fac.Transition(st_2, [
        fac.UpdateInstruction(cc_1, True) ]))
    transitions.append(fac.Transition(st_3, [
        fac.UpdateInstruction(cc_1, True) ]))
    transitions.append(fac.Transition(st_4, [
        fac.UpdateInstruction(cc_1, True) ]))
    transitions.append(fac.Transition(st_5, [
        fac.UpdateInstruction(cc_1, True) ]))
    st_4._set_transitionSet(transitions)
    transitions = []
    transitions.append(fac.Transition(st_1, [
        fac.UpdateInstruction(cc_1, True) ]))
    transitions.append(fac.Transition(st_2, [
        fac.UpdateInstruction(cc_1, True) ]))
    transitions.append(fac.Transition(st_3, [
        fac.UpdateInstruction(cc_1, True) ]))
    transitions.append(fac.Transition(st_4, [
        fac.UpdateInstruction(cc_1, True) ]))
    transitions.append(fac.Transition(st_5, [
        fac.UpdateInstruction(cc_1, True) ]))
    st_5._set_transitionSet(transitions)
    return fac.Automaton(states, counters, True, containing_state=None)
InterfaceOperationType._Automaton = _BuildAutomaton_8()




def _BuildAutomaton_9 ():
    # Remove this helper function from the namespace after it is invoked
    global _BuildAutomaton_9
    del _BuildAutomaton_9
    import pyxb.utils.fac as fac

    counters = set()
    cc_0 = fac.CounterCondition(min=0, max=None, metadata=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wsdl20.xsd', 37, 6))
    counters.add(cc_0)
    cc_1 = fac.CounterCondition(min=0, max=None, metadata=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wsdl20.xsd', 196, 8))
    counters.add(cc_1)
    states = []
    final_update = set()
    final_update.add(fac.UpdateInstruction(cc_0, False))
    symbol = pyxb.binding.content.ElementUse(MessageRefType._UseForTag(pyxb.namespace.ExpandedName(Namespace, 'documentation')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wsdl20.xsd', 37, 6))
    st_0 = fac.State(symbol, is_initial=True, final_update=final_update, is_unordered_catenation=False)
    states.append(st_0)
    final_update = set()
    final_update.add(fac.UpdateInstruction(cc_1, False))
    symbol = pyxb.binding.content.WildcardUse(pyxb.binding.content.Wildcard(process_contents=pyxb.binding.content.Wildcard.PC_lax, namespace_constraint=(pyxb.binding.content.Wildcard.NC_not, 'http://www.w3.org/ns/wsdl')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wsdl20.xsd', 197, 10))
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
MessageRefType._Automaton = _BuildAutomaton_9()




def _BuildAutomaton_10 ():
    # Remove this helper function from the namespace after it is invoked
    global _BuildAutomaton_10
    del _BuildAutomaton_10
    import pyxb.utils.fac as fac

    counters = set()
    cc_0 = fac.CounterCondition(min=0, max=None, metadata=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wsdl20.xsd', 37, 6))
    counters.add(cc_0)
    cc_1 = fac.CounterCondition(min=0, max=None, metadata=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wsdl20.xsd', 228, 8))
    counters.add(cc_1)
    states = []
    final_update = set()
    final_update.add(fac.UpdateInstruction(cc_0, False))
    symbol = pyxb.binding.content.ElementUse(MessageRefFaultType._UseForTag(pyxb.namespace.ExpandedName(Namespace, 'documentation')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wsdl20.xsd', 37, 6))
    st_0 = fac.State(symbol, is_initial=True, final_update=final_update, is_unordered_catenation=False)
    states.append(st_0)
    final_update = set()
    final_update.add(fac.UpdateInstruction(cc_1, False))
    symbol = pyxb.binding.content.WildcardUse(pyxb.binding.content.Wildcard(process_contents=pyxb.binding.content.Wildcard.PC_lax, namespace_constraint=(pyxb.binding.content.Wildcard.NC_not, 'http://www.w3.org/ns/wsdl')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wsdl20.xsd', 229, 10))
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
MessageRefFaultType._Automaton = _BuildAutomaton_10()




def _BuildAutomaton_11 ():
    # Remove this helper function from the namespace after it is invoked
    global _BuildAutomaton_11
    del _BuildAutomaton_11
    import pyxb.utils.fac as fac

    counters = set()
    cc_0 = fac.CounterCondition(min=0, max=None, metadata=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wsdl20.xsd', 37, 6))
    counters.add(cc_0)
    cc_1 = fac.CounterCondition(min=0, max=None, metadata=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wsdl20.xsd', 240, 8))
    counters.add(cc_1)
    states = []
    final_update = set()
    final_update.add(fac.UpdateInstruction(cc_0, False))
    symbol = pyxb.binding.content.ElementUse(InterfaceFaultType._UseForTag(pyxb.namespace.ExpandedName(Namespace, 'documentation')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wsdl20.xsd', 37, 6))
    st_0 = fac.State(symbol, is_initial=True, final_update=final_update, is_unordered_catenation=False)
    states.append(st_0)
    final_update = set()
    final_update.add(fac.UpdateInstruction(cc_1, False))
    symbol = pyxb.binding.content.WildcardUse(pyxb.binding.content.Wildcard(process_contents=pyxb.binding.content.Wildcard.PC_lax, namespace_constraint=(pyxb.binding.content.Wildcard.NC_not, 'http://www.w3.org/ns/wsdl')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wsdl20.xsd', 241, 10))
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
InterfaceFaultType._Automaton = _BuildAutomaton_11()




BindingType._AddElement(pyxb.binding.basis.element(pyxb.namespace.ExpandedName(Namespace, 'operation'), BindingOperationType, scope=BindingType, location=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wsdl20.xsd', 255, 10)))

BindingType._AddElement(pyxb.binding.basis.element(pyxb.namespace.ExpandedName(Namespace, 'fault'), BindingFaultType, scope=BindingType, location=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wsdl20.xsd', 256, 10)))

def _BuildAutomaton_12 ():
    # Remove this helper function from the namespace after it is invoked
    global _BuildAutomaton_12
    del _BuildAutomaton_12
    import pyxb.utils.fac as fac

    counters = set()
    cc_0 = fac.CounterCondition(min=0, max=None, metadata=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wsdl20.xsd', 37, 6))
    counters.add(cc_0)
    cc_1 = fac.CounterCondition(min=0, max=None, metadata=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wsdl20.xsd', 254, 8))
    counters.add(cc_1)
    states = []
    final_update = set()
    final_update.add(fac.UpdateInstruction(cc_0, False))
    symbol = pyxb.binding.content.ElementUse(BindingType._UseForTag(pyxb.namespace.ExpandedName(Namespace, 'documentation')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wsdl20.xsd', 37, 6))
    st_0 = fac.State(symbol, is_initial=True, final_update=final_update, is_unordered_catenation=False)
    states.append(st_0)
    final_update = set()
    final_update.add(fac.UpdateInstruction(cc_1, False))
    symbol = pyxb.binding.content.ElementUse(BindingType._UseForTag(pyxb.namespace.ExpandedName(Namespace, 'operation')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wsdl20.xsd', 255, 10))
    st_1 = fac.State(symbol, is_initial=True, final_update=final_update, is_unordered_catenation=False)
    states.append(st_1)
    final_update = set()
    final_update.add(fac.UpdateInstruction(cc_1, False))
    symbol = pyxb.binding.content.ElementUse(BindingType._UseForTag(pyxb.namespace.ExpandedName(Namespace, 'fault')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wsdl20.xsd', 256, 10))
    st_2 = fac.State(symbol, is_initial=True, final_update=final_update, is_unordered_catenation=False)
    states.append(st_2)
    final_update = set()
    final_update.add(fac.UpdateInstruction(cc_1, False))
    symbol = pyxb.binding.content.WildcardUse(pyxb.binding.content.Wildcard(process_contents=pyxb.binding.content.Wildcard.PC_lax, namespace_constraint=(pyxb.binding.content.Wildcard.NC_not, 'http://www.w3.org/ns/wsdl')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wsdl20.xsd', 257, 10))
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
        fac.UpdateInstruction(cc_1, True) ]))
    transitions.append(fac.Transition(st_3, [
        fac.UpdateInstruction(cc_1, True) ]))
    st_1._set_transitionSet(transitions)
    transitions = []
    transitions.append(fac.Transition(st_1, [
        fac.UpdateInstruction(cc_1, True) ]))
    transitions.append(fac.Transition(st_2, [
        fac.UpdateInstruction(cc_1, True) ]))
    transitions.append(fac.Transition(st_3, [
        fac.UpdateInstruction(cc_1, True) ]))
    st_2._set_transitionSet(transitions)
    transitions = []
    transitions.append(fac.Transition(st_1, [
        fac.UpdateInstruction(cc_1, True) ]))
    transitions.append(fac.Transition(st_2, [
        fac.UpdateInstruction(cc_1, True) ]))
    transitions.append(fac.Transition(st_3, [
        fac.UpdateInstruction(cc_1, True) ]))
    st_3._set_transitionSet(transitions)
    return fac.Automaton(states, counters, True, containing_state=None)
BindingType._Automaton = _BuildAutomaton_12()




BindingOperationType._AddElement(pyxb.binding.basis.element(pyxb.namespace.ExpandedName(Namespace, 'input'), BindingOperationMessageType, scope=BindingOperationType, location=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wsdl20.xsd', 270, 10)))

BindingOperationType._AddElement(pyxb.binding.basis.element(pyxb.namespace.ExpandedName(Namespace, 'output'), BindingOperationMessageType, scope=BindingOperationType, location=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wsdl20.xsd', 271, 10)))

BindingOperationType._AddElement(pyxb.binding.basis.element(pyxb.namespace.ExpandedName(Namespace, 'infault'), BindingOperationFaultType, scope=BindingOperationType, location=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wsdl20.xsd', 272, 10)))

BindingOperationType._AddElement(pyxb.binding.basis.element(pyxb.namespace.ExpandedName(Namespace, 'outfault'), BindingOperationFaultType, scope=BindingOperationType, location=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wsdl20.xsd', 273, 10)))

def _BuildAutomaton_13 ():
    # Remove this helper function from the namespace after it is invoked
    global _BuildAutomaton_13
    del _BuildAutomaton_13
    import pyxb.utils.fac as fac

    counters = set()
    cc_0 = fac.CounterCondition(min=0, max=None, metadata=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wsdl20.xsd', 37, 6))
    counters.add(cc_0)
    cc_1 = fac.CounterCondition(min=0, max=None, metadata=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wsdl20.xsd', 269, 8))
    counters.add(cc_1)
    states = []
    final_update = set()
    final_update.add(fac.UpdateInstruction(cc_0, False))
    symbol = pyxb.binding.content.ElementUse(BindingOperationType._UseForTag(pyxb.namespace.ExpandedName(Namespace, 'documentation')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wsdl20.xsd', 37, 6))
    st_0 = fac.State(symbol, is_initial=True, final_update=final_update, is_unordered_catenation=False)
    states.append(st_0)
    final_update = set()
    final_update.add(fac.UpdateInstruction(cc_1, False))
    symbol = pyxb.binding.content.ElementUse(BindingOperationType._UseForTag(pyxb.namespace.ExpandedName(Namespace, 'input')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wsdl20.xsd', 270, 10))
    st_1 = fac.State(symbol, is_initial=True, final_update=final_update, is_unordered_catenation=False)
    states.append(st_1)
    final_update = set()
    final_update.add(fac.UpdateInstruction(cc_1, False))
    symbol = pyxb.binding.content.ElementUse(BindingOperationType._UseForTag(pyxb.namespace.ExpandedName(Namespace, 'output')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wsdl20.xsd', 271, 10))
    st_2 = fac.State(symbol, is_initial=True, final_update=final_update, is_unordered_catenation=False)
    states.append(st_2)
    final_update = set()
    final_update.add(fac.UpdateInstruction(cc_1, False))
    symbol = pyxb.binding.content.ElementUse(BindingOperationType._UseForTag(pyxb.namespace.ExpandedName(Namespace, 'infault')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wsdl20.xsd', 272, 10))
    st_3 = fac.State(symbol, is_initial=True, final_update=final_update, is_unordered_catenation=False)
    states.append(st_3)
    final_update = set()
    final_update.add(fac.UpdateInstruction(cc_1, False))
    symbol = pyxb.binding.content.ElementUse(BindingOperationType._UseForTag(pyxb.namespace.ExpandedName(Namespace, 'outfault')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wsdl20.xsd', 273, 10))
    st_4 = fac.State(symbol, is_initial=True, final_update=final_update, is_unordered_catenation=False)
    states.append(st_4)
    final_update = set()
    final_update.add(fac.UpdateInstruction(cc_1, False))
    symbol = pyxb.binding.content.WildcardUse(pyxb.binding.content.Wildcard(process_contents=pyxb.binding.content.Wildcard.PC_lax, namespace_constraint=(pyxb.binding.content.Wildcard.NC_not, 'http://www.w3.org/ns/wsdl')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wsdl20.xsd', 274, 10))
    st_5 = fac.State(symbol, is_initial=True, final_update=final_update, is_unordered_catenation=False)
    states.append(st_5)
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
    st_0._set_transitionSet(transitions)
    transitions = []
    transitions.append(fac.Transition(st_1, [
        fac.UpdateInstruction(cc_1, True) ]))
    transitions.append(fac.Transition(st_2, [
        fac.UpdateInstruction(cc_1, True) ]))
    transitions.append(fac.Transition(st_3, [
        fac.UpdateInstruction(cc_1, True) ]))
    transitions.append(fac.Transition(st_4, [
        fac.UpdateInstruction(cc_1, True) ]))
    transitions.append(fac.Transition(st_5, [
        fac.UpdateInstruction(cc_1, True) ]))
    st_1._set_transitionSet(transitions)
    transitions = []
    transitions.append(fac.Transition(st_1, [
        fac.UpdateInstruction(cc_1, True) ]))
    transitions.append(fac.Transition(st_2, [
        fac.UpdateInstruction(cc_1, True) ]))
    transitions.append(fac.Transition(st_3, [
        fac.UpdateInstruction(cc_1, True) ]))
    transitions.append(fac.Transition(st_4, [
        fac.UpdateInstruction(cc_1, True) ]))
    transitions.append(fac.Transition(st_5, [
        fac.UpdateInstruction(cc_1, True) ]))
    st_2._set_transitionSet(transitions)
    transitions = []
    transitions.append(fac.Transition(st_1, [
        fac.UpdateInstruction(cc_1, True) ]))
    transitions.append(fac.Transition(st_2, [
        fac.UpdateInstruction(cc_1, True) ]))
    transitions.append(fac.Transition(st_3, [
        fac.UpdateInstruction(cc_1, True) ]))
    transitions.append(fac.Transition(st_4, [
        fac.UpdateInstruction(cc_1, True) ]))
    transitions.append(fac.Transition(st_5, [
        fac.UpdateInstruction(cc_1, True) ]))
    st_3._set_transitionSet(transitions)
    transitions = []
    transitions.append(fac.Transition(st_1, [
        fac.UpdateInstruction(cc_1, True) ]))
    transitions.append(fac.Transition(st_2, [
        fac.UpdateInstruction(cc_1, True) ]))
    transitions.append(fac.Transition(st_3, [
        fac.UpdateInstruction(cc_1, True) ]))
    transitions.append(fac.Transition(st_4, [
        fac.UpdateInstruction(cc_1, True) ]))
    transitions.append(fac.Transition(st_5, [
        fac.UpdateInstruction(cc_1, True) ]))
    st_4._set_transitionSet(transitions)
    transitions = []
    transitions.append(fac.Transition(st_1, [
        fac.UpdateInstruction(cc_1, True) ]))
    transitions.append(fac.Transition(st_2, [
        fac.UpdateInstruction(cc_1, True) ]))
    transitions.append(fac.Transition(st_3, [
        fac.UpdateInstruction(cc_1, True) ]))
    transitions.append(fac.Transition(st_4, [
        fac.UpdateInstruction(cc_1, True) ]))
    transitions.append(fac.Transition(st_5, [
        fac.UpdateInstruction(cc_1, True) ]))
    st_5._set_transitionSet(transitions)
    return fac.Automaton(states, counters, True, containing_state=None)
BindingOperationType._Automaton = _BuildAutomaton_13()




def _BuildAutomaton_14 ():
    # Remove this helper function from the namespace after it is invoked
    global _BuildAutomaton_14
    del _BuildAutomaton_14
    import pyxb.utils.fac as fac

    counters = set()
    cc_0 = fac.CounterCondition(min=0, max=None, metadata=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wsdl20.xsd', 37, 6))
    counters.add(cc_0)
    cc_1 = fac.CounterCondition(min=0, max=None, metadata=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wsdl20.xsd', 284, 8))
    counters.add(cc_1)
    states = []
    final_update = set()
    final_update.add(fac.UpdateInstruction(cc_0, False))
    symbol = pyxb.binding.content.ElementUse(BindingOperationMessageType._UseForTag(pyxb.namespace.ExpandedName(Namespace, 'documentation')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wsdl20.xsd', 37, 6))
    st_0 = fac.State(symbol, is_initial=True, final_update=final_update, is_unordered_catenation=False)
    states.append(st_0)
    final_update = set()
    final_update.add(fac.UpdateInstruction(cc_1, False))
    symbol = pyxb.binding.content.WildcardUse(pyxb.binding.content.Wildcard(process_contents=pyxb.binding.content.Wildcard.PC_lax, namespace_constraint=(pyxb.binding.content.Wildcard.NC_not, 'http://www.w3.org/ns/wsdl')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wsdl20.xsd', 285, 10))
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
BindingOperationMessageType._Automaton = _BuildAutomaton_14()




def _BuildAutomaton_15 ():
    # Remove this helper function from the namespace after it is invoked
    global _BuildAutomaton_15
    del _BuildAutomaton_15
    import pyxb.utils.fac as fac

    counters = set()
    cc_0 = fac.CounterCondition(min=0, max=None, metadata=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wsdl20.xsd', 37, 6))
    counters.add(cc_0)
    cc_1 = fac.CounterCondition(min=0, max=None, metadata=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wsdl20.xsd', 295, 8))
    counters.add(cc_1)
    states = []
    final_update = set()
    final_update.add(fac.UpdateInstruction(cc_0, False))
    symbol = pyxb.binding.content.ElementUse(BindingOperationFaultType._UseForTag(pyxb.namespace.ExpandedName(Namespace, 'documentation')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wsdl20.xsd', 37, 6))
    st_0 = fac.State(symbol, is_initial=True, final_update=final_update, is_unordered_catenation=False)
    states.append(st_0)
    final_update = set()
    final_update.add(fac.UpdateInstruction(cc_1, False))
    symbol = pyxb.binding.content.WildcardUse(pyxb.binding.content.Wildcard(process_contents=pyxb.binding.content.Wildcard.PC_lax, namespace_constraint=(pyxb.binding.content.Wildcard.NC_not, 'http://www.w3.org/ns/wsdl')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wsdl20.xsd', 296, 10))
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
BindingOperationFaultType._Automaton = _BuildAutomaton_15()




def _BuildAutomaton_16 ():
    # Remove this helper function from the namespace after it is invoked
    global _BuildAutomaton_16
    del _BuildAutomaton_16
    import pyxb.utils.fac as fac

    counters = set()
    cc_0 = fac.CounterCondition(min=0, max=None, metadata=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wsdl20.xsd', 37, 6))
    counters.add(cc_0)
    cc_1 = fac.CounterCondition(min=0, max=None, metadata=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wsdl20.xsd', 307, 8))
    counters.add(cc_1)
    states = []
    final_update = set()
    final_update.add(fac.UpdateInstruction(cc_0, False))
    symbol = pyxb.binding.content.ElementUse(BindingFaultType._UseForTag(pyxb.namespace.ExpandedName(Namespace, 'documentation')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wsdl20.xsd', 37, 6))
    st_0 = fac.State(symbol, is_initial=True, final_update=final_update, is_unordered_catenation=False)
    states.append(st_0)
    final_update = set()
    final_update.add(fac.UpdateInstruction(cc_1, False))
    symbol = pyxb.binding.content.WildcardUse(pyxb.binding.content.Wildcard(process_contents=pyxb.binding.content.Wildcard.PC_lax, namespace_constraint=(pyxb.binding.content.Wildcard.NC_not, 'http://www.w3.org/ns/wsdl')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wsdl20.xsd', 308, 10))
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
BindingFaultType._Automaton = _BuildAutomaton_16()




ServiceType._AddElement(pyxb.binding.basis.element(pyxb.namespace.ExpandedName(Namespace, 'endpoint'), EndpointType, scope=ServiceType, location=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wsdl20.xsd', 335, 2)))

def _BuildAutomaton_17 ():
    # Remove this helper function from the namespace after it is invoked
    global _BuildAutomaton_17
    del _BuildAutomaton_17
    import pyxb.utils.fac as fac

    counters = set()
    cc_0 = fac.CounterCondition(min=0, max=None, metadata=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wsdl20.xsd', 37, 6))
    counters.add(cc_0)
    states = []
    final_update = None
    symbol = pyxb.binding.content.ElementUse(ServiceType._UseForTag(pyxb.namespace.ExpandedName(Namespace, 'documentation')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wsdl20.xsd', 37, 6))
    st_0 = fac.State(symbol, is_initial=True, final_update=final_update, is_unordered_catenation=False)
    states.append(st_0)
    final_update = set()
    symbol = pyxb.binding.content.ElementUse(ServiceType._UseForTag(pyxb.namespace.ExpandedName(Namespace, 'endpoint')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wsdl20.xsd', 326, 10))
    st_1 = fac.State(symbol, is_initial=True, final_update=final_update, is_unordered_catenation=False)
    states.append(st_1)
    final_update = set()
    symbol = pyxb.binding.content.WildcardUse(pyxb.binding.content.Wildcard(process_contents=pyxb.binding.content.Wildcard.PC_lax, namespace_constraint=(pyxb.binding.content.Wildcard.NC_not, 'http://www.w3.org/ns/wsdl')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wsdl20.xsd', 327, 10))
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
         ]))
    transitions.append(fac.Transition(st_2, [
         ]))
    st_1._set_transitionSet(transitions)
    transitions = []
    transitions.append(fac.Transition(st_1, [
         ]))
    transitions.append(fac.Transition(st_2, [
         ]))
    st_2._set_transitionSet(transitions)
    return fac.Automaton(states, counters, False, containing_state=None)
ServiceType._Automaton = _BuildAutomaton_17()




def _BuildAutomaton_18 ():
    # Remove this helper function from the namespace after it is invoked
    global _BuildAutomaton_18
    del _BuildAutomaton_18
    import pyxb.utils.fac as fac

    counters = set()
    cc_0 = fac.CounterCondition(min=0, max=None, metadata=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wsdl20.xsd', 37, 6))
    counters.add(cc_0)
    cc_1 = fac.CounterCondition(min=0, max=None, metadata=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wsdl20.xsd', 339, 8))
    counters.add(cc_1)
    states = []
    final_update = set()
    final_update.add(fac.UpdateInstruction(cc_0, False))
    symbol = pyxb.binding.content.ElementUse(EndpointType._UseForTag(pyxb.namespace.ExpandedName(Namespace, 'documentation')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wsdl20.xsd', 37, 6))
    st_0 = fac.State(symbol, is_initial=True, final_update=final_update, is_unordered_catenation=False)
    states.append(st_0)
    final_update = set()
    final_update.add(fac.UpdateInstruction(cc_1, False))
    symbol = pyxb.binding.content.WildcardUse(pyxb.binding.content.Wildcard(process_contents=pyxb.binding.content.Wildcard.PC_lax, namespace_constraint=(pyxb.binding.content.Wildcard.NC_not, 'http://www.w3.org/ns/wsdl')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wsdl20.xsd', 340, 10))
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
EndpointType._Automaton = _BuildAutomaton_18()

