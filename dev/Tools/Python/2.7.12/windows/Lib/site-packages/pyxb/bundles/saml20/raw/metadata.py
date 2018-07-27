# ./pyxb/bundles/saml20/raw/metadata.py
# -*- coding: utf-8 -*-
# PyXB bindings for NM:f9d428fdbafcf137fd6f3c6128735cdf8829c2c0
# Generated 2014-10-19 06:25:01.632075 by PyXB version 1.2.4 using Python 2.7.3.final.0
# Namespace urn:oasis:names:tc:SAML:2.0:metadata

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
_GenerationUID = pyxb.utils.utility.UniqueIdentifier('urn:uuid:90d71932-5782-11e4-b242-c8600024e903')

# Version of PyXB used to generate the bindings
_PyXBVersion = '1.2.4'
# Generated bindings are not compatible across PyXB versions
if pyxb.__version__ != _PyXBVersion:
    raise pyxb.PyXBVersionError(_PyXBVersion)

# Import bindings for namespaces imported into schema
import pyxb.bundles.wssplat.ds
import pyxb.bundles.wssplat.xenc
import pyxb.bundles.saml20.assertion
import pyxb.binding.datatypes
import pyxb.binding.xml_

# NOTE: All namespace declarations are reserved within the binding
Namespace = pyxb.namespace.NamespaceForURI('urn:oasis:names:tc:SAML:2.0:metadata', create_if_missing=True)
Namespace.configureCategories(['typeBinding', 'elementBinding'])
_Namespace_ds = pyxb.bundles.wssplat.ds.Namespace
_Namespace_ds.configureCategories(['typeBinding', 'elementBinding'])
_Namespace_saml = pyxb.bundles.saml20.assertion.Namespace
_Namespace_saml.configureCategories(['typeBinding', 'elementBinding'])

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


# Atomic simple type: {urn:oasis:names:tc:SAML:2.0:metadata}entityIDType
class entityIDType (pyxb.binding.datatypes.anyURI):

    """An atomic simple type."""

    _ExpandedName = pyxb.namespace.ExpandedName(Namespace, 'entityIDType')
    _XSDLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/saml20/schemas/metadata.xsd', 31, 4)
    _Documentation = None
entityIDType._CF_maxLength = pyxb.binding.facets.CF_maxLength(value=pyxb.binding.datatypes.nonNegativeInteger(1024))
entityIDType._InitializeFacetMap(entityIDType._CF_maxLength)
Namespace.addCategoryObject('typeBinding', 'entityIDType', entityIDType)

# Atomic simple type: {urn:oasis:names:tc:SAML:2.0:metadata}ContactTypeType
class ContactTypeType (pyxb.binding.datatypes.string, pyxb.binding.basis.enumeration_mixin):

    """An atomic simple type."""

    _ExpandedName = pyxb.namespace.ExpandedName(Namespace, 'ContactTypeType')
    _XSDLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/saml20/schemas/metadata.xsd', 151, 4)
    _Documentation = None
ContactTypeType._CF_enumeration = pyxb.binding.facets.CF_enumeration(value_datatype=ContactTypeType, enum_prefix=None)
ContactTypeType.technical = ContactTypeType._CF_enumeration.addEnumeration(unicode_value='technical', tag='technical')
ContactTypeType.support = ContactTypeType._CF_enumeration.addEnumeration(unicode_value='support', tag='support')
ContactTypeType.administrative = ContactTypeType._CF_enumeration.addEnumeration(unicode_value='administrative', tag='administrative')
ContactTypeType.billing = ContactTypeType._CF_enumeration.addEnumeration(unicode_value='billing', tag='billing')
ContactTypeType.other = ContactTypeType._CF_enumeration.addEnumeration(unicode_value='other', tag='other')
ContactTypeType._InitializeFacetMap(ContactTypeType._CF_enumeration)
Namespace.addCategoryObject('typeBinding', 'ContactTypeType', ContactTypeType)

# List simple type: {urn:oasis:names:tc:SAML:2.0:metadata}anyURIListType
# superclasses pyxb.binding.datatypes.anySimpleType
class anyURIListType (pyxb.binding.basis.STD_list):

    """Simple type that is a list of pyxb.binding.datatypes.anyURI."""

    _ExpandedName = pyxb.namespace.ExpandedName(Namespace, 'anyURIListType')
    _XSDLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/saml20/schemas/metadata.xsd', 186, 4)
    _Documentation = None

    _ItemType = pyxb.binding.datatypes.anyURI
anyURIListType._InitializeFacetMap()
Namespace.addCategoryObject('typeBinding', 'anyURIListType', anyURIListType)

# Atomic simple type: {urn:oasis:names:tc:SAML:2.0:metadata}KeyTypes
class KeyTypes (pyxb.binding.datatypes.string, pyxb.binding.basis.enumeration_mixin):

    """An atomic simple type."""

    _ExpandedName = pyxb.namespace.ExpandedName(Namespace, 'KeyTypes')
    _XSDLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/saml20/schemas/metadata.xsd', 198, 4)
    _Documentation = None
KeyTypes._CF_enumeration = pyxb.binding.facets.CF_enumeration(value_datatype=KeyTypes, enum_prefix=None)
KeyTypes.encryption = KeyTypes._CF_enumeration.addEnumeration(unicode_value='encryption', tag='encryption')
KeyTypes.signing = KeyTypes._CF_enumeration.addEnumeration(unicode_value='signing', tag='signing')
KeyTypes._InitializeFacetMap(KeyTypes._CF_enumeration)
Namespace.addCategoryObject('typeBinding', 'KeyTypes', KeyTypes)

# Complex type {urn:oasis:names:tc:SAML:2.0:metadata}localizedNameType with content type SIMPLE
class localizedNameType (pyxb.binding.basis.complexTypeDefinition):
    """Complex type {urn:oasis:names:tc:SAML:2.0:metadata}localizedNameType with content type SIMPLE"""
    _TypeDefinition = pyxb.binding.datatypes.string
    _ContentTypeTag = pyxb.binding.basis.complexTypeDefinition._CT_SIMPLE
    _Abstract = False
    _ExpandedName = pyxb.namespace.ExpandedName(Namespace, 'localizedNameType')
    _XSDLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/saml20/schemas/metadata.xsd', 36, 4)
    _ElementMap = {}
    _AttributeMap = {}
    # Base type is pyxb.binding.datatypes.string
    
    # Attribute {http://www.w3.org/XML/1998/namespace}lang uses Python identifier lang
    __lang = pyxb.binding.content.AttributeUse(pyxb.namespace.ExpandedName(pyxb.namespace.XML, 'lang'), 'lang', '__urnoasisnamestcSAML2_0metadata_localizedNameType_httpwww_w3_orgXML1998namespacelang', pyxb.binding.xml_.STD_ANON_lang, required=True)
    __lang._DeclarationLocation = None
    __lang._UseLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/saml20/schemas/metadata.xsd', 39, 16)
    
    lang = property(__lang.value, __lang.set, None, None)

    _ElementMap.update({
        
    })
    _AttributeMap.update({
        __lang.name() : __lang
    })
Namespace.addCategoryObject('typeBinding', 'localizedNameType', localizedNameType)


# Complex type {urn:oasis:names:tc:SAML:2.0:metadata}localizedURIType with content type SIMPLE
class localizedURIType (pyxb.binding.basis.complexTypeDefinition):
    """Complex type {urn:oasis:names:tc:SAML:2.0:metadata}localizedURIType with content type SIMPLE"""
    _TypeDefinition = pyxb.binding.datatypes.anyURI
    _ContentTypeTag = pyxb.binding.basis.complexTypeDefinition._CT_SIMPLE
    _Abstract = False
    _ExpandedName = pyxb.namespace.ExpandedName(Namespace, 'localizedURIType')
    _XSDLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/saml20/schemas/metadata.xsd', 43, 4)
    _ElementMap = {}
    _AttributeMap = {}
    # Base type is pyxb.binding.datatypes.anyURI
    
    # Attribute {http://www.w3.org/XML/1998/namespace}lang uses Python identifier lang
    __lang = pyxb.binding.content.AttributeUse(pyxb.namespace.ExpandedName(pyxb.namespace.XML, 'lang'), 'lang', '__urnoasisnamestcSAML2_0metadata_localizedURIType_httpwww_w3_orgXML1998namespacelang', pyxb.binding.xml_.STD_ANON_lang, required=True)
    __lang._DeclarationLocation = None
    __lang._UseLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/saml20/schemas/metadata.xsd', 46, 16)
    
    lang = property(__lang.value, __lang.set, None, None)

    _ElementMap.update({
        
    })
    _AttributeMap.update({
        __lang.name() : __lang
    })
Namespace.addCategoryObject('typeBinding', 'localizedURIType', localizedURIType)


# Complex type {urn:oasis:names:tc:SAML:2.0:metadata}ExtensionsType with content type ELEMENT_ONLY
class ExtensionsType (pyxb.binding.basis.complexTypeDefinition):
    """Complex type {urn:oasis:names:tc:SAML:2.0:metadata}ExtensionsType with content type ELEMENT_ONLY"""
    _TypeDefinition = None
    _ContentTypeTag = pyxb.binding.basis.complexTypeDefinition._CT_ELEMENT_ONLY
    _Abstract = False
    _ExpandedName = pyxb.namespace.ExpandedName(Namespace, 'ExtensionsType')
    _XSDLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/saml20/schemas/metadata.xsd', 52, 4)
    _ElementMap = {}
    _AttributeMap = {}
    # Base type is pyxb.binding.datatypes.anyType
    _HasWildcardElement = True
    _ElementMap.update({
        
    })
    _AttributeMap.update({
        
    })
Namespace.addCategoryObject('typeBinding', 'ExtensionsType', ExtensionsType)


# Complex type {urn:oasis:names:tc:SAML:2.0:metadata}EndpointType with content type ELEMENT_ONLY
class EndpointType (pyxb.binding.basis.complexTypeDefinition):
    """Complex type {urn:oasis:names:tc:SAML:2.0:metadata}EndpointType with content type ELEMENT_ONLY"""
    _TypeDefinition = None
    _ContentTypeTag = pyxb.binding.basis.complexTypeDefinition._CT_ELEMENT_ONLY
    _Abstract = False
    _ExpandedName = pyxb.namespace.ExpandedName(Namespace, 'EndpointType')
    _XSDLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/saml20/schemas/metadata.xsd', 58, 4)
    _ElementMap = {}
    _AttributeMap = {}
    # Base type is pyxb.binding.datatypes.anyType
    
    # Attribute Binding uses Python identifier Binding
    __Binding = pyxb.binding.content.AttributeUse(pyxb.namespace.ExpandedName(None, 'Binding'), 'Binding', '__urnoasisnamestcSAML2_0metadata_EndpointType_Binding', pyxb.binding.datatypes.anyURI, required=True)
    __Binding._DeclarationLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/saml20/schemas/metadata.xsd', 62, 8)
    __Binding._UseLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/saml20/schemas/metadata.xsd', 62, 8)
    
    Binding = property(__Binding.value, __Binding.set, None, None)

    
    # Attribute Location uses Python identifier Location
    __Location = pyxb.binding.content.AttributeUse(pyxb.namespace.ExpandedName(None, 'Location'), 'Location', '__urnoasisnamestcSAML2_0metadata_EndpointType_Location', pyxb.binding.datatypes.anyURI, required=True)
    __Location._DeclarationLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/saml20/schemas/metadata.xsd', 63, 8)
    __Location._UseLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/saml20/schemas/metadata.xsd', 63, 8)
    
    Location = property(__Location.value, __Location.set, None, None)

    
    # Attribute ResponseLocation uses Python identifier ResponseLocation
    __ResponseLocation = pyxb.binding.content.AttributeUse(pyxb.namespace.ExpandedName(None, 'ResponseLocation'), 'ResponseLocation', '__urnoasisnamestcSAML2_0metadata_EndpointType_ResponseLocation', pyxb.binding.datatypes.anyURI)
    __ResponseLocation._DeclarationLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/saml20/schemas/metadata.xsd', 64, 8)
    __ResponseLocation._UseLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/saml20/schemas/metadata.xsd', 64, 8)
    
    ResponseLocation = property(__ResponseLocation.value, __ResponseLocation.set, None, None)

    _AttributeWildcard = pyxb.binding.content.Wildcard(process_contents=pyxb.binding.content.Wildcard.PC_lax, namespace_constraint=(pyxb.binding.content.Wildcard.NC_not, 'urn:oasis:names:tc:SAML:2.0:metadata'))
    _HasWildcardElement = True
    _ElementMap.update({
        
    })
    _AttributeMap.update({
        __Binding.name() : __Binding,
        __Location.name() : __Location,
        __ResponseLocation.name() : __ResponseLocation
    })
Namespace.addCategoryObject('typeBinding', 'EndpointType', EndpointType)


# Complex type {urn:oasis:names:tc:SAML:2.0:metadata}EntitiesDescriptorType with content type ELEMENT_ONLY
class EntitiesDescriptorType (pyxb.binding.basis.complexTypeDefinition):
    """Complex type {urn:oasis:names:tc:SAML:2.0:metadata}EntitiesDescriptorType with content type ELEMENT_ONLY"""
    _TypeDefinition = None
    _ContentTypeTag = pyxb.binding.basis.complexTypeDefinition._CT_ELEMENT_ONLY
    _Abstract = False
    _ExpandedName = pyxb.namespace.ExpandedName(Namespace, 'EntitiesDescriptorType')
    _XSDLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/saml20/schemas/metadata.xsd', 78, 4)
    _ElementMap = {}
    _AttributeMap = {}
    # Base type is pyxb.binding.datatypes.anyType
    
    # Element {http://www.w3.org/2000/09/xmldsig#}Signature uses Python identifier Signature
    __Signature = pyxb.binding.content.ElementDeclaration(pyxb.namespace.ExpandedName(_Namespace_ds, 'Signature'), 'Signature', '__urnoasisnamestcSAML2_0metadata_EntitiesDescriptorType_httpwww_w3_org200009xmldsigSignature', False, pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/ds.xsd', 43, 0), )

    
    Signature = property(__Signature.value, __Signature.set, None, None)

    
    # Element {urn:oasis:names:tc:SAML:2.0:metadata}Extensions uses Python identifier Extensions
    __Extensions = pyxb.binding.content.ElementDeclaration(pyxb.namespace.ExpandedName(Namespace, 'Extensions'), 'Extensions', '__urnoasisnamestcSAML2_0metadata_EntitiesDescriptorType_urnoasisnamestcSAML2_0metadataExtensions', False, pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/saml20/schemas/metadata.xsd', 51, 4), )

    
    Extensions = property(__Extensions.value, __Extensions.set, None, None)

    
    # Element {urn:oasis:names:tc:SAML:2.0:metadata}EntitiesDescriptor uses Python identifier EntitiesDescriptor
    __EntitiesDescriptor = pyxb.binding.content.ElementDeclaration(pyxb.namespace.ExpandedName(Namespace, 'EntitiesDescriptor'), 'EntitiesDescriptor', '__urnoasisnamestcSAML2_0metadata_EntitiesDescriptorType_urnoasisnamestcSAML2_0metadataEntitiesDescriptor', True, pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/saml20/schemas/metadata.xsd', 77, 4), )

    
    EntitiesDescriptor = property(__EntitiesDescriptor.value, __EntitiesDescriptor.set, None, None)

    
    # Element {urn:oasis:names:tc:SAML:2.0:metadata}EntityDescriptor uses Python identifier EntityDescriptor
    __EntityDescriptor = pyxb.binding.content.ElementDeclaration(pyxb.namespace.ExpandedName(Namespace, 'EntityDescriptor'), 'EntityDescriptor', '__urnoasisnamestcSAML2_0metadata_EntitiesDescriptorType_urnoasisnamestcSAML2_0metadataEntityDescriptor', True, pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/saml20/schemas/metadata.xsd', 93, 4), )

    
    EntityDescriptor = property(__EntityDescriptor.value, __EntityDescriptor.set, None, None)

    
    # Attribute validUntil uses Python identifier validUntil
    __validUntil = pyxb.binding.content.AttributeUse(pyxb.namespace.ExpandedName(None, 'validUntil'), 'validUntil', '__urnoasisnamestcSAML2_0metadata_EntitiesDescriptorType_validUntil', pyxb.binding.datatypes.dateTime)
    __validUntil._DeclarationLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/saml20/schemas/metadata.xsd', 87, 8)
    __validUntil._UseLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/saml20/schemas/metadata.xsd', 87, 8)
    
    validUntil = property(__validUntil.value, __validUntil.set, None, None)

    
    # Attribute cacheDuration uses Python identifier cacheDuration
    __cacheDuration = pyxb.binding.content.AttributeUse(pyxb.namespace.ExpandedName(None, 'cacheDuration'), 'cacheDuration', '__urnoasisnamestcSAML2_0metadata_EntitiesDescriptorType_cacheDuration', pyxb.binding.datatypes.duration)
    __cacheDuration._DeclarationLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/saml20/schemas/metadata.xsd', 88, 8)
    __cacheDuration._UseLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/saml20/schemas/metadata.xsd', 88, 8)
    
    cacheDuration = property(__cacheDuration.value, __cacheDuration.set, None, None)

    
    # Attribute ID uses Python identifier ID
    __ID = pyxb.binding.content.AttributeUse(pyxb.namespace.ExpandedName(None, 'ID'), 'ID', '__urnoasisnamestcSAML2_0metadata_EntitiesDescriptorType_ID', pyxb.binding.datatypes.ID)
    __ID._DeclarationLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/saml20/schemas/metadata.xsd', 89, 8)
    __ID._UseLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/saml20/schemas/metadata.xsd', 89, 8)
    
    ID = property(__ID.value, __ID.set, None, None)

    
    # Attribute Name uses Python identifier Name
    __Name = pyxb.binding.content.AttributeUse(pyxb.namespace.ExpandedName(None, 'Name'), 'Name', '__urnoasisnamestcSAML2_0metadata_EntitiesDescriptorType_Name', pyxb.binding.datatypes.string)
    __Name._DeclarationLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/saml20/schemas/metadata.xsd', 90, 8)
    __Name._UseLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/saml20/schemas/metadata.xsd', 90, 8)
    
    Name = property(__Name.value, __Name.set, None, None)

    _ElementMap.update({
        __Signature.name() : __Signature,
        __Extensions.name() : __Extensions,
        __EntitiesDescriptor.name() : __EntitiesDescriptor,
        __EntityDescriptor.name() : __EntityDescriptor
    })
    _AttributeMap.update({
        __validUntil.name() : __validUntil,
        __cacheDuration.name() : __cacheDuration,
        __ID.name() : __ID,
        __Name.name() : __Name
    })
Namespace.addCategoryObject('typeBinding', 'EntitiesDescriptorType', EntitiesDescriptorType)


# Complex type {urn:oasis:names:tc:SAML:2.0:metadata}OrganizationType with content type ELEMENT_ONLY
class OrganizationType (pyxb.binding.basis.complexTypeDefinition):
    """Complex type {urn:oasis:names:tc:SAML:2.0:metadata}OrganizationType with content type ELEMENT_ONLY"""
    _TypeDefinition = None
    _ContentTypeTag = pyxb.binding.basis.complexTypeDefinition._CT_ELEMENT_ONLY
    _Abstract = False
    _ExpandedName = pyxb.namespace.ExpandedName(Namespace, 'OrganizationType')
    _XSDLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/saml20/schemas/metadata.xsd', 121, 4)
    _ElementMap = {}
    _AttributeMap = {}
    # Base type is pyxb.binding.datatypes.anyType
    
    # Element {urn:oasis:names:tc:SAML:2.0:metadata}Extensions uses Python identifier Extensions
    __Extensions = pyxb.binding.content.ElementDeclaration(pyxb.namespace.ExpandedName(Namespace, 'Extensions'), 'Extensions', '__urnoasisnamestcSAML2_0metadata_OrganizationType_urnoasisnamestcSAML2_0metadataExtensions', False, pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/saml20/schemas/metadata.xsd', 51, 4), )

    
    Extensions = property(__Extensions.value, __Extensions.set, None, None)

    
    # Element {urn:oasis:names:tc:SAML:2.0:metadata}OrganizationName uses Python identifier OrganizationName
    __OrganizationName = pyxb.binding.content.ElementDeclaration(pyxb.namespace.ExpandedName(Namespace, 'OrganizationName'), 'OrganizationName', '__urnoasisnamestcSAML2_0metadata_OrganizationType_urnoasisnamestcSAML2_0metadataOrganizationName', True, pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/saml20/schemas/metadata.xsd', 130, 4), )

    
    OrganizationName = property(__OrganizationName.value, __OrganizationName.set, None, None)

    
    # Element {urn:oasis:names:tc:SAML:2.0:metadata}OrganizationDisplayName uses Python identifier OrganizationDisplayName
    __OrganizationDisplayName = pyxb.binding.content.ElementDeclaration(pyxb.namespace.ExpandedName(Namespace, 'OrganizationDisplayName'), 'OrganizationDisplayName', '__urnoasisnamestcSAML2_0metadata_OrganizationType_urnoasisnamestcSAML2_0metadataOrganizationDisplayName', True, pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/saml20/schemas/metadata.xsd', 131, 4), )

    
    OrganizationDisplayName = property(__OrganizationDisplayName.value, __OrganizationDisplayName.set, None, None)

    
    # Element {urn:oasis:names:tc:SAML:2.0:metadata}OrganizationURL uses Python identifier OrganizationURL
    __OrganizationURL = pyxb.binding.content.ElementDeclaration(pyxb.namespace.ExpandedName(Namespace, 'OrganizationURL'), 'OrganizationURL', '__urnoasisnamestcSAML2_0metadata_OrganizationType_urnoasisnamestcSAML2_0metadataOrganizationURL', True, pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/saml20/schemas/metadata.xsd', 132, 4), )

    
    OrganizationURL = property(__OrganizationURL.value, __OrganizationURL.set, None, None)

    _AttributeWildcard = pyxb.binding.content.Wildcard(process_contents=pyxb.binding.content.Wildcard.PC_lax, namespace_constraint=(pyxb.binding.content.Wildcard.NC_not, 'urn:oasis:names:tc:SAML:2.0:metadata'))
    _ElementMap.update({
        __Extensions.name() : __Extensions,
        __OrganizationName.name() : __OrganizationName,
        __OrganizationDisplayName.name() : __OrganizationDisplayName,
        __OrganizationURL.name() : __OrganizationURL
    })
    _AttributeMap.update({
        
    })
Namespace.addCategoryObject('typeBinding', 'OrganizationType', OrganizationType)


# Complex type {urn:oasis:names:tc:SAML:2.0:metadata}AdditionalMetadataLocationType with content type SIMPLE
class AdditionalMetadataLocationType (pyxb.binding.basis.complexTypeDefinition):
    """Complex type {urn:oasis:names:tc:SAML:2.0:metadata}AdditionalMetadataLocationType with content type SIMPLE"""
    _TypeDefinition = pyxb.binding.datatypes.anyURI
    _ContentTypeTag = pyxb.binding.basis.complexTypeDefinition._CT_SIMPLE
    _Abstract = False
    _ExpandedName = pyxb.namespace.ExpandedName(Namespace, 'AdditionalMetadataLocationType')
    _XSDLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/saml20/schemas/metadata.xsd', 162, 4)
    _ElementMap = {}
    _AttributeMap = {}
    # Base type is pyxb.binding.datatypes.anyURI
    
    # Attribute namespace uses Python identifier namespace
    __namespace = pyxb.binding.content.AttributeUse(pyxb.namespace.ExpandedName(None, 'namespace'), 'namespace', '__urnoasisnamestcSAML2_0metadata_AdditionalMetadataLocationType_namespace', pyxb.binding.datatypes.anyURI, required=True)
    __namespace._DeclarationLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/saml20/schemas/metadata.xsd', 165, 16)
    __namespace._UseLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/saml20/schemas/metadata.xsd', 165, 16)
    
    namespace = property(__namespace.value, __namespace.set, None, None)

    _ElementMap.update({
        
    })
    _AttributeMap.update({
        __namespace.name() : __namespace
    })
Namespace.addCategoryObject('typeBinding', 'AdditionalMetadataLocationType', AdditionalMetadataLocationType)


# Complex type {urn:oasis:names:tc:SAML:2.0:metadata}AttributeConsumingServiceType with content type ELEMENT_ONLY
class AttributeConsumingServiceType (pyxb.binding.basis.complexTypeDefinition):
    """Complex type {urn:oasis:names:tc:SAML:2.0:metadata}AttributeConsumingServiceType with content type ELEMENT_ONLY"""
    _TypeDefinition = None
    _ContentTypeTag = pyxb.binding.basis.complexTypeDefinition._CT_ELEMENT_ONLY
    _Abstract = False
    _ExpandedName = pyxb.namespace.ExpandedName(Namespace, 'AttributeConsumingServiceType')
    _XSDLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/saml20/schemas/metadata.xsd', 258, 4)
    _ElementMap = {}
    _AttributeMap = {}
    # Base type is pyxb.binding.datatypes.anyType
    
    # Element {urn:oasis:names:tc:SAML:2.0:metadata}ServiceName uses Python identifier ServiceName
    __ServiceName = pyxb.binding.content.ElementDeclaration(pyxb.namespace.ExpandedName(Namespace, 'ServiceName'), 'ServiceName', '__urnoasisnamestcSAML2_0metadata_AttributeConsumingServiceType_urnoasisnamestcSAML2_0metadataServiceName', True, pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/saml20/schemas/metadata.xsd', 267, 4), )

    
    ServiceName = property(__ServiceName.value, __ServiceName.set, None, None)

    
    # Element {urn:oasis:names:tc:SAML:2.0:metadata}ServiceDescription uses Python identifier ServiceDescription
    __ServiceDescription = pyxb.binding.content.ElementDeclaration(pyxb.namespace.ExpandedName(Namespace, 'ServiceDescription'), 'ServiceDescription', '__urnoasisnamestcSAML2_0metadata_AttributeConsumingServiceType_urnoasisnamestcSAML2_0metadataServiceDescription', True, pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/saml20/schemas/metadata.xsd', 268, 4), )

    
    ServiceDescription = property(__ServiceDescription.value, __ServiceDescription.set, None, None)

    
    # Element {urn:oasis:names:tc:SAML:2.0:metadata}RequestedAttribute uses Python identifier RequestedAttribute
    __RequestedAttribute = pyxb.binding.content.ElementDeclaration(pyxb.namespace.ExpandedName(Namespace, 'RequestedAttribute'), 'RequestedAttribute', '__urnoasisnamestcSAML2_0metadata_AttributeConsumingServiceType_urnoasisnamestcSAML2_0metadataRequestedAttribute', True, pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/saml20/schemas/metadata.xsd', 269, 4), )

    
    RequestedAttribute = property(__RequestedAttribute.value, __RequestedAttribute.set, None, None)

    
    # Attribute index uses Python identifier index
    __index = pyxb.binding.content.AttributeUse(pyxb.namespace.ExpandedName(None, 'index'), 'index', '__urnoasisnamestcSAML2_0metadata_AttributeConsumingServiceType_index', pyxb.binding.datatypes.unsignedShort, required=True)
    __index._DeclarationLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/saml20/schemas/metadata.xsd', 264, 8)
    __index._UseLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/saml20/schemas/metadata.xsd', 264, 8)
    
    index = property(__index.value, __index.set, None, None)

    
    # Attribute isDefault uses Python identifier isDefault
    __isDefault = pyxb.binding.content.AttributeUse(pyxb.namespace.ExpandedName(None, 'isDefault'), 'isDefault', '__urnoasisnamestcSAML2_0metadata_AttributeConsumingServiceType_isDefault', pyxb.binding.datatypes.boolean)
    __isDefault._DeclarationLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/saml20/schemas/metadata.xsd', 265, 8)
    __isDefault._UseLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/saml20/schemas/metadata.xsd', 265, 8)
    
    isDefault = property(__isDefault.value, __isDefault.set, None, None)

    _ElementMap.update({
        __ServiceName.name() : __ServiceName,
        __ServiceDescription.name() : __ServiceDescription,
        __RequestedAttribute.name() : __RequestedAttribute
    })
    _AttributeMap.update({
        __index.name() : __index,
        __isDefault.name() : __isDefault
    })
Namespace.addCategoryObject('typeBinding', 'AttributeConsumingServiceType', AttributeConsumingServiceType)


# Complex type {urn:oasis:names:tc:SAML:2.0:metadata}RequestedAttributeType with content type ELEMENT_ONLY
class RequestedAttributeType (pyxb.bundles.saml20.assertion.AttributeType):
    """Complex type {urn:oasis:names:tc:SAML:2.0:metadata}RequestedAttributeType with content type ELEMENT_ONLY"""
    _TypeDefinition = None
    _ContentTypeTag = pyxb.binding.basis.complexTypeDefinition._CT_ELEMENT_ONLY
    _Abstract = False
    _ExpandedName = pyxb.namespace.ExpandedName(Namespace, 'RequestedAttributeType')
    _XSDLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/saml20/schemas/metadata.xsd', 270, 4)
    _ElementMap = pyxb.bundles.saml20.assertion.AttributeType._ElementMap.copy()
    _AttributeMap = pyxb.bundles.saml20.assertion.AttributeType._AttributeMap.copy()
    # Base type is pyxb.bundles.saml20.assertion.AttributeType
    
    # Element AttributeValue ({urn:oasis:names:tc:SAML:2.0:assertion}AttributeValue) inherited from {urn:oasis:names:tc:SAML:2.0:assertion}AttributeType
    
    # Attribute Name inherited from {urn:oasis:names:tc:SAML:2.0:assertion}AttributeType
    
    # Attribute NameFormat inherited from {urn:oasis:names:tc:SAML:2.0:assertion}AttributeType
    
    # Attribute FriendlyName inherited from {urn:oasis:names:tc:SAML:2.0:assertion}AttributeType
    
    # Attribute isRequired uses Python identifier isRequired
    __isRequired = pyxb.binding.content.AttributeUse(pyxb.namespace.ExpandedName(None, 'isRequired'), 'isRequired', '__urnoasisnamestcSAML2_0metadata_RequestedAttributeType_isRequired', pyxb.binding.datatypes.boolean)
    __isRequired._DeclarationLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/saml20/schemas/metadata.xsd', 273, 16)
    __isRequired._UseLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/saml20/schemas/metadata.xsd', 273, 16)
    
    isRequired = property(__isRequired.value, __isRequired.set, None, None)

    _AttributeWildcard = pyxb.binding.content.Wildcard(process_contents=pyxb.binding.content.Wildcard.PC_lax, namespace_constraint=(pyxb.binding.content.Wildcard.NC_not, 'urn:oasis:names:tc:SAML:2.0:assertion'))
    _ElementMap.update({
        
    })
    _AttributeMap.update({
        __isRequired.name() : __isRequired
    })
Namespace.addCategoryObject('typeBinding', 'RequestedAttributeType', RequestedAttributeType)


# Complex type {urn:oasis:names:tc:SAML:2.0:metadata}IndexedEndpointType with content type ELEMENT_ONLY
class IndexedEndpointType (EndpointType):
    """Complex type {urn:oasis:names:tc:SAML:2.0:metadata}IndexedEndpointType with content type ELEMENT_ONLY"""
    _TypeDefinition = None
    _ContentTypeTag = pyxb.binding.basis.complexTypeDefinition._CT_ELEMENT_ONLY
    _Abstract = False
    _ExpandedName = pyxb.namespace.ExpandedName(Namespace, 'IndexedEndpointType')
    _XSDLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/saml20/schemas/metadata.xsd', 68, 4)
    _ElementMap = EndpointType._ElementMap.copy()
    _AttributeMap = EndpointType._AttributeMap.copy()
    # Base type is EndpointType
    
    # Attribute Binding inherited from {urn:oasis:names:tc:SAML:2.0:metadata}EndpointType
    
    # Attribute Location inherited from {urn:oasis:names:tc:SAML:2.0:metadata}EndpointType
    
    # Attribute ResponseLocation inherited from {urn:oasis:names:tc:SAML:2.0:metadata}EndpointType
    
    # Attribute index uses Python identifier index
    __index = pyxb.binding.content.AttributeUse(pyxb.namespace.ExpandedName(None, 'index'), 'index', '__urnoasisnamestcSAML2_0metadata_IndexedEndpointType_index', pyxb.binding.datatypes.unsignedShort, required=True)
    __index._DeclarationLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/saml20/schemas/metadata.xsd', 71, 16)
    __index._UseLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/saml20/schemas/metadata.xsd', 71, 16)
    
    index = property(__index.value, __index.set, None, None)

    
    # Attribute isDefault uses Python identifier isDefault
    __isDefault = pyxb.binding.content.AttributeUse(pyxb.namespace.ExpandedName(None, 'isDefault'), 'isDefault', '__urnoasisnamestcSAML2_0metadata_IndexedEndpointType_isDefault', pyxb.binding.datatypes.boolean)
    __isDefault._DeclarationLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/saml20/schemas/metadata.xsd', 72, 16)
    __isDefault._UseLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/saml20/schemas/metadata.xsd', 72, 16)
    
    isDefault = property(__isDefault.value, __isDefault.set, None, None)

    _AttributeWildcard = pyxb.binding.content.Wildcard(process_contents=pyxb.binding.content.Wildcard.PC_lax, namespace_constraint=(pyxb.binding.content.Wildcard.NC_not, 'urn:oasis:names:tc:SAML:2.0:metadata'))
    _HasWildcardElement = True
    _ElementMap.update({
        
    })
    _AttributeMap.update({
        __index.name() : __index,
        __isDefault.name() : __isDefault
    })
Namespace.addCategoryObject('typeBinding', 'IndexedEndpointType', IndexedEndpointType)


# Complex type {urn:oasis:names:tc:SAML:2.0:metadata}EntityDescriptorType with content type ELEMENT_ONLY
class EntityDescriptorType (pyxb.binding.basis.complexTypeDefinition):
    """Complex type {urn:oasis:names:tc:SAML:2.0:metadata}EntityDescriptorType with content type ELEMENT_ONLY"""
    _TypeDefinition = None
    _ContentTypeTag = pyxb.binding.basis.complexTypeDefinition._CT_ELEMENT_ONLY
    _Abstract = False
    _ExpandedName = pyxb.namespace.ExpandedName(Namespace, 'EntityDescriptorType')
    _XSDLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/saml20/schemas/metadata.xsd', 94, 4)
    _ElementMap = {}
    _AttributeMap = {}
    # Base type is pyxb.binding.datatypes.anyType
    
    # Element {http://www.w3.org/2000/09/xmldsig#}Signature uses Python identifier Signature
    __Signature = pyxb.binding.content.ElementDeclaration(pyxb.namespace.ExpandedName(_Namespace_ds, 'Signature'), 'Signature', '__urnoasisnamestcSAML2_0metadata_EntityDescriptorType_httpwww_w3_org200009xmldsigSignature', False, pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/ds.xsd', 43, 0), )

    
    Signature = property(__Signature.value, __Signature.set, None, None)

    
    # Element {urn:oasis:names:tc:SAML:2.0:metadata}Extensions uses Python identifier Extensions
    __Extensions = pyxb.binding.content.ElementDeclaration(pyxb.namespace.ExpandedName(Namespace, 'Extensions'), 'Extensions', '__urnoasisnamestcSAML2_0metadata_EntityDescriptorType_urnoasisnamestcSAML2_0metadataExtensions', False, pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/saml20/schemas/metadata.xsd', 51, 4), )

    
    Extensions = property(__Extensions.value, __Extensions.set, None, None)

    
    # Element {urn:oasis:names:tc:SAML:2.0:metadata}Organization uses Python identifier Organization
    __Organization = pyxb.binding.content.ElementDeclaration(pyxb.namespace.ExpandedName(Namespace, 'Organization'), 'Organization', '__urnoasisnamestcSAML2_0metadata_EntityDescriptorType_urnoasisnamestcSAML2_0metadataOrganization', False, pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/saml20/schemas/metadata.xsd', 120, 4), )

    
    Organization = property(__Organization.value, __Organization.set, None, None)

    
    # Element {urn:oasis:names:tc:SAML:2.0:metadata}ContactPerson uses Python identifier ContactPerson
    __ContactPerson = pyxb.binding.content.ElementDeclaration(pyxb.namespace.ExpandedName(Namespace, 'ContactPerson'), 'ContactPerson', '__urnoasisnamestcSAML2_0metadata_EntityDescriptorType_urnoasisnamestcSAML2_0metadataContactPerson', True, pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/saml20/schemas/metadata.xsd', 133, 4), )

    
    ContactPerson = property(__ContactPerson.value, __ContactPerson.set, None, None)

    
    # Element {urn:oasis:names:tc:SAML:2.0:metadata}AdditionalMetadataLocation uses Python identifier AdditionalMetadataLocation
    __AdditionalMetadataLocation = pyxb.binding.content.ElementDeclaration(pyxb.namespace.ExpandedName(Namespace, 'AdditionalMetadataLocation'), 'AdditionalMetadataLocation', '__urnoasisnamestcSAML2_0metadata_EntityDescriptorType_urnoasisnamestcSAML2_0metadataAdditionalMetadataLocation', True, pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/saml20/schemas/metadata.xsd', 161, 4), )

    
    AdditionalMetadataLocation = property(__AdditionalMetadataLocation.value, __AdditionalMetadataLocation.set, None, None)

    
    # Element {urn:oasis:names:tc:SAML:2.0:metadata}RoleDescriptor uses Python identifier RoleDescriptor
    __RoleDescriptor = pyxb.binding.content.ElementDeclaration(pyxb.namespace.ExpandedName(Namespace, 'RoleDescriptor'), 'RoleDescriptor', '__urnoasisnamestcSAML2_0metadata_EntityDescriptorType_urnoasisnamestcSAML2_0metadataRoleDescriptor', True, pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/saml20/schemas/metadata.xsd', 170, 4), )

    
    RoleDescriptor = property(__RoleDescriptor.value, __RoleDescriptor.set, None, None)

    
    # Element {urn:oasis:names:tc:SAML:2.0:metadata}IDPSSODescriptor uses Python identifier IDPSSODescriptor
    __IDPSSODescriptor = pyxb.binding.content.ElementDeclaration(pyxb.namespace.ExpandedName(Namespace, 'IDPSSODescriptor'), 'IDPSSODescriptor', '__urnoasisnamestcSAML2_0metadata_EntityDescriptorType_urnoasisnamestcSAML2_0metadataIDPSSODescriptor', True, pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/saml20/schemas/metadata.xsd', 223, 4), )

    
    IDPSSODescriptor = property(__IDPSSODescriptor.value, __IDPSSODescriptor.set, None, None)

    
    # Element {urn:oasis:names:tc:SAML:2.0:metadata}SPSSODescriptor uses Python identifier SPSSODescriptor
    __SPSSODescriptor = pyxb.binding.content.ElementDeclaration(pyxb.namespace.ExpandedName(Namespace, 'SPSSODescriptor'), 'SPSSODescriptor', '__urnoasisnamestcSAML2_0metadata_EntityDescriptorType_urnoasisnamestcSAML2_0metadataSPSSODescriptor', True, pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/saml20/schemas/metadata.xsd', 243, 4), )

    
    SPSSODescriptor = property(__SPSSODescriptor.value, __SPSSODescriptor.set, None, None)

    
    # Element {urn:oasis:names:tc:SAML:2.0:metadata}AuthnAuthorityDescriptor uses Python identifier AuthnAuthorityDescriptor
    __AuthnAuthorityDescriptor = pyxb.binding.content.ElementDeclaration(pyxb.namespace.ExpandedName(Namespace, 'AuthnAuthorityDescriptor'), 'AuthnAuthorityDescriptor', '__urnoasisnamestcSAML2_0metadata_EntityDescriptorType_urnoasisnamestcSAML2_0metadataAuthnAuthorityDescriptor', True, pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/saml20/schemas/metadata.xsd', 278, 4), )

    
    AuthnAuthorityDescriptor = property(__AuthnAuthorityDescriptor.value, __AuthnAuthorityDescriptor.set, None, None)

    
    # Element {urn:oasis:names:tc:SAML:2.0:metadata}PDPDescriptor uses Python identifier PDPDescriptor
    __PDPDescriptor = pyxb.binding.content.ElementDeclaration(pyxb.namespace.ExpandedName(Namespace, 'PDPDescriptor'), 'PDPDescriptor', '__urnoasisnamestcSAML2_0metadata_EntityDescriptorType_urnoasisnamestcSAML2_0metadataPDPDescriptor', True, pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/saml20/schemas/metadata.xsd', 292, 4), )

    
    PDPDescriptor = property(__PDPDescriptor.value, __PDPDescriptor.set, None, None)

    
    # Element {urn:oasis:names:tc:SAML:2.0:metadata}AttributeAuthorityDescriptor uses Python identifier AttributeAuthorityDescriptor
    __AttributeAuthorityDescriptor = pyxb.binding.content.ElementDeclaration(pyxb.namespace.ExpandedName(Namespace, 'AttributeAuthorityDescriptor'), 'AttributeAuthorityDescriptor', '__urnoasisnamestcSAML2_0metadata_EntityDescriptorType_urnoasisnamestcSAML2_0metadataAttributeAuthorityDescriptor', True, pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/saml20/schemas/metadata.xsd', 306, 4), )

    
    AttributeAuthorityDescriptor = property(__AttributeAuthorityDescriptor.value, __AttributeAuthorityDescriptor.set, None, None)

    
    # Element {urn:oasis:names:tc:SAML:2.0:metadata}AffiliationDescriptor uses Python identifier AffiliationDescriptor
    __AffiliationDescriptor = pyxb.binding.content.ElementDeclaration(pyxb.namespace.ExpandedName(Namespace, 'AffiliationDescriptor'), 'AffiliationDescriptor', '__urnoasisnamestcSAML2_0metadata_EntityDescriptorType_urnoasisnamestcSAML2_0metadataAffiliationDescriptor', False, pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/saml20/schemas/metadata.xsd', 322, 4), )

    
    AffiliationDescriptor = property(__AffiliationDescriptor.value, __AffiliationDescriptor.set, None, None)

    
    # Attribute entityID uses Python identifier entityID
    __entityID = pyxb.binding.content.AttributeUse(pyxb.namespace.ExpandedName(None, 'entityID'), 'entityID', '__urnoasisnamestcSAML2_0metadata_EntityDescriptorType_entityID', entityIDType, required=True)
    __entityID._DeclarationLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/saml20/schemas/metadata.xsd', 113, 8)
    __entityID._UseLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/saml20/schemas/metadata.xsd', 113, 8)
    
    entityID = property(__entityID.value, __entityID.set, None, None)

    
    # Attribute validUntil uses Python identifier validUntil
    __validUntil = pyxb.binding.content.AttributeUse(pyxb.namespace.ExpandedName(None, 'validUntil'), 'validUntil', '__urnoasisnamestcSAML2_0metadata_EntityDescriptorType_validUntil', pyxb.binding.datatypes.dateTime)
    __validUntil._DeclarationLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/saml20/schemas/metadata.xsd', 114, 8)
    __validUntil._UseLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/saml20/schemas/metadata.xsd', 114, 8)
    
    validUntil = property(__validUntil.value, __validUntil.set, None, None)

    
    # Attribute cacheDuration uses Python identifier cacheDuration
    __cacheDuration = pyxb.binding.content.AttributeUse(pyxb.namespace.ExpandedName(None, 'cacheDuration'), 'cacheDuration', '__urnoasisnamestcSAML2_0metadata_EntityDescriptorType_cacheDuration', pyxb.binding.datatypes.duration)
    __cacheDuration._DeclarationLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/saml20/schemas/metadata.xsd', 115, 8)
    __cacheDuration._UseLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/saml20/schemas/metadata.xsd', 115, 8)
    
    cacheDuration = property(__cacheDuration.value, __cacheDuration.set, None, None)

    
    # Attribute ID uses Python identifier ID
    __ID = pyxb.binding.content.AttributeUse(pyxb.namespace.ExpandedName(None, 'ID'), 'ID', '__urnoasisnamestcSAML2_0metadata_EntityDescriptorType_ID', pyxb.binding.datatypes.ID)
    __ID._DeclarationLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/saml20/schemas/metadata.xsd', 116, 8)
    __ID._UseLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/saml20/schemas/metadata.xsd', 116, 8)
    
    ID = property(__ID.value, __ID.set, None, None)

    _AttributeWildcard = pyxb.binding.content.Wildcard(process_contents=pyxb.binding.content.Wildcard.PC_lax, namespace_constraint=(pyxb.binding.content.Wildcard.NC_not, 'urn:oasis:names:tc:SAML:2.0:metadata'))
    _ElementMap.update({
        __Signature.name() : __Signature,
        __Extensions.name() : __Extensions,
        __Organization.name() : __Organization,
        __ContactPerson.name() : __ContactPerson,
        __AdditionalMetadataLocation.name() : __AdditionalMetadataLocation,
        __RoleDescriptor.name() : __RoleDescriptor,
        __IDPSSODescriptor.name() : __IDPSSODescriptor,
        __SPSSODescriptor.name() : __SPSSODescriptor,
        __AuthnAuthorityDescriptor.name() : __AuthnAuthorityDescriptor,
        __PDPDescriptor.name() : __PDPDescriptor,
        __AttributeAuthorityDescriptor.name() : __AttributeAuthorityDescriptor,
        __AffiliationDescriptor.name() : __AffiliationDescriptor
    })
    _AttributeMap.update({
        __entityID.name() : __entityID,
        __validUntil.name() : __validUntil,
        __cacheDuration.name() : __cacheDuration,
        __ID.name() : __ID
    })
Namespace.addCategoryObject('typeBinding', 'EntityDescriptorType', EntityDescriptorType)


# Complex type {urn:oasis:names:tc:SAML:2.0:metadata}ContactType with content type ELEMENT_ONLY
class ContactType (pyxb.binding.basis.complexTypeDefinition):
    """Complex type {urn:oasis:names:tc:SAML:2.0:metadata}ContactType with content type ELEMENT_ONLY"""
    _TypeDefinition = None
    _ContentTypeTag = pyxb.binding.basis.complexTypeDefinition._CT_ELEMENT_ONLY
    _Abstract = False
    _ExpandedName = pyxb.namespace.ExpandedName(Namespace, 'ContactType')
    _XSDLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/saml20/schemas/metadata.xsd', 134, 4)
    _ElementMap = {}
    _AttributeMap = {}
    # Base type is pyxb.binding.datatypes.anyType
    
    # Element {urn:oasis:names:tc:SAML:2.0:metadata}Extensions uses Python identifier Extensions
    __Extensions = pyxb.binding.content.ElementDeclaration(pyxb.namespace.ExpandedName(Namespace, 'Extensions'), 'Extensions', '__urnoasisnamestcSAML2_0metadata_ContactType_urnoasisnamestcSAML2_0metadataExtensions', False, pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/saml20/schemas/metadata.xsd', 51, 4), )

    
    Extensions = property(__Extensions.value, __Extensions.set, None, None)

    
    # Element {urn:oasis:names:tc:SAML:2.0:metadata}Company uses Python identifier Company
    __Company = pyxb.binding.content.ElementDeclaration(pyxb.namespace.ExpandedName(Namespace, 'Company'), 'Company', '__urnoasisnamestcSAML2_0metadata_ContactType_urnoasisnamestcSAML2_0metadataCompany', False, pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/saml20/schemas/metadata.xsd', 146, 4), )

    
    Company = property(__Company.value, __Company.set, None, None)

    
    # Element {urn:oasis:names:tc:SAML:2.0:metadata}GivenName uses Python identifier GivenName
    __GivenName = pyxb.binding.content.ElementDeclaration(pyxb.namespace.ExpandedName(Namespace, 'GivenName'), 'GivenName', '__urnoasisnamestcSAML2_0metadata_ContactType_urnoasisnamestcSAML2_0metadataGivenName', False, pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/saml20/schemas/metadata.xsd', 147, 4), )

    
    GivenName = property(__GivenName.value, __GivenName.set, None, None)

    
    # Element {urn:oasis:names:tc:SAML:2.0:metadata}SurName uses Python identifier SurName
    __SurName = pyxb.binding.content.ElementDeclaration(pyxb.namespace.ExpandedName(Namespace, 'SurName'), 'SurName', '__urnoasisnamestcSAML2_0metadata_ContactType_urnoasisnamestcSAML2_0metadataSurName', False, pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/saml20/schemas/metadata.xsd', 148, 4), )

    
    SurName = property(__SurName.value, __SurName.set, None, None)

    
    # Element {urn:oasis:names:tc:SAML:2.0:metadata}EmailAddress uses Python identifier EmailAddress
    __EmailAddress = pyxb.binding.content.ElementDeclaration(pyxb.namespace.ExpandedName(Namespace, 'EmailAddress'), 'EmailAddress', '__urnoasisnamestcSAML2_0metadata_ContactType_urnoasisnamestcSAML2_0metadataEmailAddress', True, pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/saml20/schemas/metadata.xsd', 149, 4), )

    
    EmailAddress = property(__EmailAddress.value, __EmailAddress.set, None, None)

    
    # Element {urn:oasis:names:tc:SAML:2.0:metadata}TelephoneNumber uses Python identifier TelephoneNumber
    __TelephoneNumber = pyxb.binding.content.ElementDeclaration(pyxb.namespace.ExpandedName(Namespace, 'TelephoneNumber'), 'TelephoneNumber', '__urnoasisnamestcSAML2_0metadata_ContactType_urnoasisnamestcSAML2_0metadataTelephoneNumber', True, pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/saml20/schemas/metadata.xsd', 150, 4), )

    
    TelephoneNumber = property(__TelephoneNumber.value, __TelephoneNumber.set, None, None)

    
    # Attribute contactType uses Python identifier contactType
    __contactType = pyxb.binding.content.AttributeUse(pyxb.namespace.ExpandedName(None, 'contactType'), 'contactType', '__urnoasisnamestcSAML2_0metadata_ContactType_contactType', ContactTypeType, required=True)
    __contactType._DeclarationLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/saml20/schemas/metadata.xsd', 143, 8)
    __contactType._UseLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/saml20/schemas/metadata.xsd', 143, 8)
    
    contactType = property(__contactType.value, __contactType.set, None, None)

    _AttributeWildcard = pyxb.binding.content.Wildcard(process_contents=pyxb.binding.content.Wildcard.PC_lax, namespace_constraint=(pyxb.binding.content.Wildcard.NC_not, 'urn:oasis:names:tc:SAML:2.0:metadata'))
    _ElementMap.update({
        __Extensions.name() : __Extensions,
        __Company.name() : __Company,
        __GivenName.name() : __GivenName,
        __SurName.name() : __SurName,
        __EmailAddress.name() : __EmailAddress,
        __TelephoneNumber.name() : __TelephoneNumber
    })
    _AttributeMap.update({
        __contactType.name() : __contactType
    })
Namespace.addCategoryObject('typeBinding', 'ContactType', ContactType)


# Complex type {urn:oasis:names:tc:SAML:2.0:metadata}RoleDescriptorType with content type ELEMENT_ONLY
class RoleDescriptorType (pyxb.binding.basis.complexTypeDefinition):
    """Complex type {urn:oasis:names:tc:SAML:2.0:metadata}RoleDescriptorType with content type ELEMENT_ONLY"""
    _TypeDefinition = None
    _ContentTypeTag = pyxb.binding.basis.complexTypeDefinition._CT_ELEMENT_ONLY
    _Abstract = True
    _ExpandedName = pyxb.namespace.ExpandedName(Namespace, 'RoleDescriptorType')
    _XSDLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/saml20/schemas/metadata.xsd', 171, 4)
    _ElementMap = {}
    _AttributeMap = {}
    # Base type is pyxb.binding.datatypes.anyType
    
    # Element {http://www.w3.org/2000/09/xmldsig#}Signature uses Python identifier Signature
    __Signature = pyxb.binding.content.ElementDeclaration(pyxb.namespace.ExpandedName(_Namespace_ds, 'Signature'), 'Signature', '__urnoasisnamestcSAML2_0metadata_RoleDescriptorType_httpwww_w3_org200009xmldsigSignature', False, pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/ds.xsd', 43, 0), )

    
    Signature = property(__Signature.value, __Signature.set, None, None)

    
    # Element {urn:oasis:names:tc:SAML:2.0:metadata}Extensions uses Python identifier Extensions
    __Extensions = pyxb.binding.content.ElementDeclaration(pyxb.namespace.ExpandedName(Namespace, 'Extensions'), 'Extensions', '__urnoasisnamestcSAML2_0metadata_RoleDescriptorType_urnoasisnamestcSAML2_0metadataExtensions', False, pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/saml20/schemas/metadata.xsd', 51, 4), )

    
    Extensions = property(__Extensions.value, __Extensions.set, None, None)

    
    # Element {urn:oasis:names:tc:SAML:2.0:metadata}Organization uses Python identifier Organization
    __Organization = pyxb.binding.content.ElementDeclaration(pyxb.namespace.ExpandedName(Namespace, 'Organization'), 'Organization', '__urnoasisnamestcSAML2_0metadata_RoleDescriptorType_urnoasisnamestcSAML2_0metadataOrganization', False, pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/saml20/schemas/metadata.xsd', 120, 4), )

    
    Organization = property(__Organization.value, __Organization.set, None, None)

    
    # Element {urn:oasis:names:tc:SAML:2.0:metadata}ContactPerson uses Python identifier ContactPerson
    __ContactPerson = pyxb.binding.content.ElementDeclaration(pyxb.namespace.ExpandedName(Namespace, 'ContactPerson'), 'ContactPerson', '__urnoasisnamestcSAML2_0metadata_RoleDescriptorType_urnoasisnamestcSAML2_0metadataContactPerson', True, pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/saml20/schemas/metadata.xsd', 133, 4), )

    
    ContactPerson = property(__ContactPerson.value, __ContactPerson.set, None, None)

    
    # Element {urn:oasis:names:tc:SAML:2.0:metadata}KeyDescriptor uses Python identifier KeyDescriptor
    __KeyDescriptor = pyxb.binding.content.ElementDeclaration(pyxb.namespace.ExpandedName(Namespace, 'KeyDescriptor'), 'KeyDescriptor', '__urnoasisnamestcSAML2_0metadata_RoleDescriptorType_urnoasisnamestcSAML2_0metadataKeyDescriptor', True, pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/saml20/schemas/metadata.xsd', 190, 4), )

    
    KeyDescriptor = property(__KeyDescriptor.value, __KeyDescriptor.set, None, None)

    
    # Attribute ID uses Python identifier ID
    __ID = pyxb.binding.content.AttributeUse(pyxb.namespace.ExpandedName(None, 'ID'), 'ID', '__urnoasisnamestcSAML2_0metadata_RoleDescriptorType_ID', pyxb.binding.datatypes.ID)
    __ID._DeclarationLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/saml20/schemas/metadata.xsd', 179, 8)
    __ID._UseLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/saml20/schemas/metadata.xsd', 179, 8)
    
    ID = property(__ID.value, __ID.set, None, None)

    
    # Attribute validUntil uses Python identifier validUntil
    __validUntil = pyxb.binding.content.AttributeUse(pyxb.namespace.ExpandedName(None, 'validUntil'), 'validUntil', '__urnoasisnamestcSAML2_0metadata_RoleDescriptorType_validUntil', pyxb.binding.datatypes.dateTime)
    __validUntil._DeclarationLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/saml20/schemas/metadata.xsd', 180, 8)
    __validUntil._UseLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/saml20/schemas/metadata.xsd', 180, 8)
    
    validUntil = property(__validUntil.value, __validUntil.set, None, None)

    
    # Attribute cacheDuration uses Python identifier cacheDuration
    __cacheDuration = pyxb.binding.content.AttributeUse(pyxb.namespace.ExpandedName(None, 'cacheDuration'), 'cacheDuration', '__urnoasisnamestcSAML2_0metadata_RoleDescriptorType_cacheDuration', pyxb.binding.datatypes.duration)
    __cacheDuration._DeclarationLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/saml20/schemas/metadata.xsd', 181, 8)
    __cacheDuration._UseLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/saml20/schemas/metadata.xsd', 181, 8)
    
    cacheDuration = property(__cacheDuration.value, __cacheDuration.set, None, None)

    
    # Attribute protocolSupportEnumeration uses Python identifier protocolSupportEnumeration
    __protocolSupportEnumeration = pyxb.binding.content.AttributeUse(pyxb.namespace.ExpandedName(None, 'protocolSupportEnumeration'), 'protocolSupportEnumeration', '__urnoasisnamestcSAML2_0metadata_RoleDescriptorType_protocolSupportEnumeration', anyURIListType, required=True)
    __protocolSupportEnumeration._DeclarationLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/saml20/schemas/metadata.xsd', 182, 8)
    __protocolSupportEnumeration._UseLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/saml20/schemas/metadata.xsd', 182, 8)
    
    protocolSupportEnumeration = property(__protocolSupportEnumeration.value, __protocolSupportEnumeration.set, None, None)

    
    # Attribute errorURL uses Python identifier errorURL
    __errorURL = pyxb.binding.content.AttributeUse(pyxb.namespace.ExpandedName(None, 'errorURL'), 'errorURL', '__urnoasisnamestcSAML2_0metadata_RoleDescriptorType_errorURL', pyxb.binding.datatypes.anyURI)
    __errorURL._DeclarationLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/saml20/schemas/metadata.xsd', 183, 8)
    __errorURL._UseLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/saml20/schemas/metadata.xsd', 183, 8)
    
    errorURL = property(__errorURL.value, __errorURL.set, None, None)

    _AttributeWildcard = pyxb.binding.content.Wildcard(process_contents=pyxb.binding.content.Wildcard.PC_lax, namespace_constraint=(pyxb.binding.content.Wildcard.NC_not, 'urn:oasis:names:tc:SAML:2.0:metadata'))
    _ElementMap.update({
        __Signature.name() : __Signature,
        __Extensions.name() : __Extensions,
        __Organization.name() : __Organization,
        __ContactPerson.name() : __ContactPerson,
        __KeyDescriptor.name() : __KeyDescriptor
    })
    _AttributeMap.update({
        __ID.name() : __ID,
        __validUntil.name() : __validUntil,
        __cacheDuration.name() : __cacheDuration,
        __protocolSupportEnumeration.name() : __protocolSupportEnumeration,
        __errorURL.name() : __errorURL
    })
Namespace.addCategoryObject('typeBinding', 'RoleDescriptorType', RoleDescriptorType)


# Complex type {urn:oasis:names:tc:SAML:2.0:metadata}KeyDescriptorType with content type ELEMENT_ONLY
class KeyDescriptorType (pyxb.binding.basis.complexTypeDefinition):
    """Complex type {urn:oasis:names:tc:SAML:2.0:metadata}KeyDescriptorType with content type ELEMENT_ONLY"""
    _TypeDefinition = None
    _ContentTypeTag = pyxb.binding.basis.complexTypeDefinition._CT_ELEMENT_ONLY
    _Abstract = False
    _ExpandedName = pyxb.namespace.ExpandedName(Namespace, 'KeyDescriptorType')
    _XSDLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/saml20/schemas/metadata.xsd', 191, 4)
    _ElementMap = {}
    _AttributeMap = {}
    # Base type is pyxb.binding.datatypes.anyType
    
    # Element {http://www.w3.org/2000/09/xmldsig#}KeyInfo uses Python identifier KeyInfo
    __KeyInfo = pyxb.binding.content.ElementDeclaration(pyxb.namespace.ExpandedName(_Namespace_ds, 'KeyInfo'), 'KeyInfo', '__urnoasisnamestcSAML2_0metadata_KeyDescriptorType_httpwww_w3_org200009xmldsigKeyInfo', False, pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/ds.xsd', 144, 0), )

    
    KeyInfo = property(__KeyInfo.value, __KeyInfo.set, None, None)

    
    # Element {urn:oasis:names:tc:SAML:2.0:metadata}EncryptionMethod uses Python identifier EncryptionMethod
    __EncryptionMethod = pyxb.binding.content.ElementDeclaration(pyxb.namespace.ExpandedName(Namespace, 'EncryptionMethod'), 'EncryptionMethod', '__urnoasisnamestcSAML2_0metadata_KeyDescriptorType_urnoasisnamestcSAML2_0metadataEncryptionMethod', True, pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/saml20/schemas/metadata.xsd', 204, 4), )

    
    EncryptionMethod = property(__EncryptionMethod.value, __EncryptionMethod.set, None, None)

    
    # Attribute use uses Python identifier use
    __use = pyxb.binding.content.AttributeUse(pyxb.namespace.ExpandedName(None, 'use'), 'use', '__urnoasisnamestcSAML2_0metadata_KeyDescriptorType_use', KeyTypes)
    __use._DeclarationLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/saml20/schemas/metadata.xsd', 196, 8)
    __use._UseLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/saml20/schemas/metadata.xsd', 196, 8)
    
    use = property(__use.value, __use.set, None, None)

    _ElementMap.update({
        __KeyInfo.name() : __KeyInfo,
        __EncryptionMethod.name() : __EncryptionMethod
    })
    _AttributeMap.update({
        __use.name() : __use
    })
Namespace.addCategoryObject('typeBinding', 'KeyDescriptorType', KeyDescriptorType)


# Complex type {urn:oasis:names:tc:SAML:2.0:metadata}AffiliationDescriptorType with content type ELEMENT_ONLY
class AffiliationDescriptorType (pyxb.binding.basis.complexTypeDefinition):
    """Complex type {urn:oasis:names:tc:SAML:2.0:metadata}AffiliationDescriptorType with content type ELEMENT_ONLY"""
    _TypeDefinition = None
    _ContentTypeTag = pyxb.binding.basis.complexTypeDefinition._CT_ELEMENT_ONLY
    _Abstract = False
    _ExpandedName = pyxb.namespace.ExpandedName(Namespace, 'AffiliationDescriptorType')
    _XSDLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/saml20/schemas/metadata.xsd', 323, 4)
    _ElementMap = {}
    _AttributeMap = {}
    # Base type is pyxb.binding.datatypes.anyType
    
    # Element {http://www.w3.org/2000/09/xmldsig#}Signature uses Python identifier Signature
    __Signature = pyxb.binding.content.ElementDeclaration(pyxb.namespace.ExpandedName(_Namespace_ds, 'Signature'), 'Signature', '__urnoasisnamestcSAML2_0metadata_AffiliationDescriptorType_httpwww_w3_org200009xmldsigSignature', False, pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/ds.xsd', 43, 0), )

    
    Signature = property(__Signature.value, __Signature.set, None, None)

    
    # Element {urn:oasis:names:tc:SAML:2.0:metadata}Extensions uses Python identifier Extensions
    __Extensions = pyxb.binding.content.ElementDeclaration(pyxb.namespace.ExpandedName(Namespace, 'Extensions'), 'Extensions', '__urnoasisnamestcSAML2_0metadata_AffiliationDescriptorType_urnoasisnamestcSAML2_0metadataExtensions', False, pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/saml20/schemas/metadata.xsd', 51, 4), )

    
    Extensions = property(__Extensions.value, __Extensions.set, None, None)

    
    # Element {urn:oasis:names:tc:SAML:2.0:metadata}KeyDescriptor uses Python identifier KeyDescriptor
    __KeyDescriptor = pyxb.binding.content.ElementDeclaration(pyxb.namespace.ExpandedName(Namespace, 'KeyDescriptor'), 'KeyDescriptor', '__urnoasisnamestcSAML2_0metadata_AffiliationDescriptorType_urnoasisnamestcSAML2_0metadataKeyDescriptor', True, pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/saml20/schemas/metadata.xsd', 190, 4), )

    
    KeyDescriptor = property(__KeyDescriptor.value, __KeyDescriptor.set, None, None)

    
    # Element {urn:oasis:names:tc:SAML:2.0:metadata}AffiliateMember uses Python identifier AffiliateMember
    __AffiliateMember = pyxb.binding.content.ElementDeclaration(pyxb.namespace.ExpandedName(Namespace, 'AffiliateMember'), 'AffiliateMember', '__urnoasisnamestcSAML2_0metadata_AffiliationDescriptorType_urnoasisnamestcSAML2_0metadataAffiliateMember', True, pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/saml20/schemas/metadata.xsd', 336, 4), )

    
    AffiliateMember = property(__AffiliateMember.value, __AffiliateMember.set, None, None)

    
    # Attribute affiliationOwnerID uses Python identifier affiliationOwnerID
    __affiliationOwnerID = pyxb.binding.content.AttributeUse(pyxb.namespace.ExpandedName(None, 'affiliationOwnerID'), 'affiliationOwnerID', '__urnoasisnamestcSAML2_0metadata_AffiliationDescriptorType_affiliationOwnerID', entityIDType, required=True)
    __affiliationOwnerID._DeclarationLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/saml20/schemas/metadata.xsd', 330, 8)
    __affiliationOwnerID._UseLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/saml20/schemas/metadata.xsd', 330, 8)
    
    affiliationOwnerID = property(__affiliationOwnerID.value, __affiliationOwnerID.set, None, None)

    
    # Attribute validUntil uses Python identifier validUntil
    __validUntil = pyxb.binding.content.AttributeUse(pyxb.namespace.ExpandedName(None, 'validUntil'), 'validUntil', '__urnoasisnamestcSAML2_0metadata_AffiliationDescriptorType_validUntil', pyxb.binding.datatypes.dateTime)
    __validUntil._DeclarationLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/saml20/schemas/metadata.xsd', 331, 8)
    __validUntil._UseLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/saml20/schemas/metadata.xsd', 331, 8)
    
    validUntil = property(__validUntil.value, __validUntil.set, None, None)

    
    # Attribute cacheDuration uses Python identifier cacheDuration
    __cacheDuration = pyxb.binding.content.AttributeUse(pyxb.namespace.ExpandedName(None, 'cacheDuration'), 'cacheDuration', '__urnoasisnamestcSAML2_0metadata_AffiliationDescriptorType_cacheDuration', pyxb.binding.datatypes.duration)
    __cacheDuration._DeclarationLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/saml20/schemas/metadata.xsd', 332, 8)
    __cacheDuration._UseLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/saml20/schemas/metadata.xsd', 332, 8)
    
    cacheDuration = property(__cacheDuration.value, __cacheDuration.set, None, None)

    
    # Attribute ID uses Python identifier ID
    __ID = pyxb.binding.content.AttributeUse(pyxb.namespace.ExpandedName(None, 'ID'), 'ID', '__urnoasisnamestcSAML2_0metadata_AffiliationDescriptorType_ID', pyxb.binding.datatypes.ID)
    __ID._DeclarationLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/saml20/schemas/metadata.xsd', 333, 8)
    __ID._UseLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/saml20/schemas/metadata.xsd', 333, 8)
    
    ID = property(__ID.value, __ID.set, None, None)

    _AttributeWildcard = pyxb.binding.content.Wildcard(process_contents=pyxb.binding.content.Wildcard.PC_lax, namespace_constraint=(pyxb.binding.content.Wildcard.NC_not, 'urn:oasis:names:tc:SAML:2.0:metadata'))
    _ElementMap.update({
        __Signature.name() : __Signature,
        __Extensions.name() : __Extensions,
        __KeyDescriptor.name() : __KeyDescriptor,
        __AffiliateMember.name() : __AffiliateMember
    })
    _AttributeMap.update({
        __affiliationOwnerID.name() : __affiliationOwnerID,
        __validUntil.name() : __validUntil,
        __cacheDuration.name() : __cacheDuration,
        __ID.name() : __ID
    })
Namespace.addCategoryObject('typeBinding', 'AffiliationDescriptorType', AffiliationDescriptorType)


# Complex type {urn:oasis:names:tc:SAML:2.0:metadata}SSODescriptorType with content type ELEMENT_ONLY
class SSODescriptorType (RoleDescriptorType):
    """Complex type {urn:oasis:names:tc:SAML:2.0:metadata}SSODescriptorType with content type ELEMENT_ONLY"""
    _TypeDefinition = None
    _ContentTypeTag = pyxb.binding.basis.complexTypeDefinition._CT_ELEMENT_ONLY
    _Abstract = True
    _ExpandedName = pyxb.namespace.ExpandedName(Namespace, 'SSODescriptorType')
    _XSDLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/saml20/schemas/metadata.xsd', 206, 4)
    _ElementMap = RoleDescriptorType._ElementMap.copy()
    _AttributeMap = RoleDescriptorType._AttributeMap.copy()
    # Base type is RoleDescriptorType
    
    # Element Signature ({http://www.w3.org/2000/09/xmldsig#}Signature) inherited from {urn:oasis:names:tc:SAML:2.0:metadata}RoleDescriptorType
    
    # Element Extensions ({urn:oasis:names:tc:SAML:2.0:metadata}Extensions) inherited from {urn:oasis:names:tc:SAML:2.0:metadata}RoleDescriptorType
    
    # Element Organization ({urn:oasis:names:tc:SAML:2.0:metadata}Organization) inherited from {urn:oasis:names:tc:SAML:2.0:metadata}RoleDescriptorType
    
    # Element ContactPerson ({urn:oasis:names:tc:SAML:2.0:metadata}ContactPerson) inherited from {urn:oasis:names:tc:SAML:2.0:metadata}RoleDescriptorType
    
    # Element KeyDescriptor ({urn:oasis:names:tc:SAML:2.0:metadata}KeyDescriptor) inherited from {urn:oasis:names:tc:SAML:2.0:metadata}RoleDescriptorType
    
    # Element {urn:oasis:names:tc:SAML:2.0:metadata}ArtifactResolutionService uses Python identifier ArtifactResolutionService
    __ArtifactResolutionService = pyxb.binding.content.ElementDeclaration(pyxb.namespace.ExpandedName(Namespace, 'ArtifactResolutionService'), 'ArtifactResolutionService', '__urnoasisnamestcSAML2_0metadata_SSODescriptorType_urnoasisnamestcSAML2_0metadataArtifactResolutionService', True, pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/saml20/schemas/metadata.xsd', 218, 4), )

    
    ArtifactResolutionService = property(__ArtifactResolutionService.value, __ArtifactResolutionService.set, None, None)

    
    # Element {urn:oasis:names:tc:SAML:2.0:metadata}SingleLogoutService uses Python identifier SingleLogoutService
    __SingleLogoutService = pyxb.binding.content.ElementDeclaration(pyxb.namespace.ExpandedName(Namespace, 'SingleLogoutService'), 'SingleLogoutService', '__urnoasisnamestcSAML2_0metadata_SSODescriptorType_urnoasisnamestcSAML2_0metadataSingleLogoutService', True, pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/saml20/schemas/metadata.xsd', 219, 4), )

    
    SingleLogoutService = property(__SingleLogoutService.value, __SingleLogoutService.set, None, None)

    
    # Element {urn:oasis:names:tc:SAML:2.0:metadata}ManageNameIDService uses Python identifier ManageNameIDService
    __ManageNameIDService = pyxb.binding.content.ElementDeclaration(pyxb.namespace.ExpandedName(Namespace, 'ManageNameIDService'), 'ManageNameIDService', '__urnoasisnamestcSAML2_0metadata_SSODescriptorType_urnoasisnamestcSAML2_0metadataManageNameIDService', True, pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/saml20/schemas/metadata.xsd', 220, 4), )

    
    ManageNameIDService = property(__ManageNameIDService.value, __ManageNameIDService.set, None, None)

    
    # Element {urn:oasis:names:tc:SAML:2.0:metadata}NameIDFormat uses Python identifier NameIDFormat
    __NameIDFormat = pyxb.binding.content.ElementDeclaration(pyxb.namespace.ExpandedName(Namespace, 'NameIDFormat'), 'NameIDFormat', '__urnoasisnamestcSAML2_0metadata_SSODescriptorType_urnoasisnamestcSAML2_0metadataNameIDFormat', True, pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/saml20/schemas/metadata.xsd', 221, 4), )

    
    NameIDFormat = property(__NameIDFormat.value, __NameIDFormat.set, None, None)

    
    # Attribute ID inherited from {urn:oasis:names:tc:SAML:2.0:metadata}RoleDescriptorType
    
    # Attribute validUntil inherited from {urn:oasis:names:tc:SAML:2.0:metadata}RoleDescriptorType
    
    # Attribute cacheDuration inherited from {urn:oasis:names:tc:SAML:2.0:metadata}RoleDescriptorType
    
    # Attribute protocolSupportEnumeration inherited from {urn:oasis:names:tc:SAML:2.0:metadata}RoleDescriptorType
    
    # Attribute errorURL inherited from {urn:oasis:names:tc:SAML:2.0:metadata}RoleDescriptorType
    _AttributeWildcard = pyxb.binding.content.Wildcard(process_contents=pyxb.binding.content.Wildcard.PC_lax, namespace_constraint=(pyxb.binding.content.Wildcard.NC_not, 'urn:oasis:names:tc:SAML:2.0:metadata'))
    _ElementMap.update({
        __ArtifactResolutionService.name() : __ArtifactResolutionService,
        __SingleLogoutService.name() : __SingleLogoutService,
        __ManageNameIDService.name() : __ManageNameIDService,
        __NameIDFormat.name() : __NameIDFormat
    })
    _AttributeMap.update({
        
    })
Namespace.addCategoryObject('typeBinding', 'SSODescriptorType', SSODescriptorType)


# Complex type {urn:oasis:names:tc:SAML:2.0:metadata}AuthnAuthorityDescriptorType with content type ELEMENT_ONLY
class AuthnAuthorityDescriptorType (RoleDescriptorType):
    """Complex type {urn:oasis:names:tc:SAML:2.0:metadata}AuthnAuthorityDescriptorType with content type ELEMENT_ONLY"""
    _TypeDefinition = None
    _ContentTypeTag = pyxb.binding.basis.complexTypeDefinition._CT_ELEMENT_ONLY
    _Abstract = False
    _ExpandedName = pyxb.namespace.ExpandedName(Namespace, 'AuthnAuthorityDescriptorType')
    _XSDLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/saml20/schemas/metadata.xsd', 279, 4)
    _ElementMap = RoleDescriptorType._ElementMap.copy()
    _AttributeMap = RoleDescriptorType._AttributeMap.copy()
    # Base type is RoleDescriptorType
    
    # Element Signature ({http://www.w3.org/2000/09/xmldsig#}Signature) inherited from {urn:oasis:names:tc:SAML:2.0:metadata}RoleDescriptorType
    
    # Element Extensions ({urn:oasis:names:tc:SAML:2.0:metadata}Extensions) inherited from {urn:oasis:names:tc:SAML:2.0:metadata}RoleDescriptorType
    
    # Element Organization ({urn:oasis:names:tc:SAML:2.0:metadata}Organization) inherited from {urn:oasis:names:tc:SAML:2.0:metadata}RoleDescriptorType
    
    # Element ContactPerson ({urn:oasis:names:tc:SAML:2.0:metadata}ContactPerson) inherited from {urn:oasis:names:tc:SAML:2.0:metadata}RoleDescriptorType
    
    # Element KeyDescriptor ({urn:oasis:names:tc:SAML:2.0:metadata}KeyDescriptor) inherited from {urn:oasis:names:tc:SAML:2.0:metadata}RoleDescriptorType
    
    # Element {urn:oasis:names:tc:SAML:2.0:metadata}NameIDFormat uses Python identifier NameIDFormat
    __NameIDFormat = pyxb.binding.content.ElementDeclaration(pyxb.namespace.ExpandedName(Namespace, 'NameIDFormat'), 'NameIDFormat', '__urnoasisnamestcSAML2_0metadata_AuthnAuthorityDescriptorType_urnoasisnamestcSAML2_0metadataNameIDFormat', True, pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/saml20/schemas/metadata.xsd', 221, 4), )

    
    NameIDFormat = property(__NameIDFormat.value, __NameIDFormat.set, None, None)

    
    # Element {urn:oasis:names:tc:SAML:2.0:metadata}AssertionIDRequestService uses Python identifier AssertionIDRequestService
    __AssertionIDRequestService = pyxb.binding.content.ElementDeclaration(pyxb.namespace.ExpandedName(Namespace, 'AssertionIDRequestService'), 'AssertionIDRequestService', '__urnoasisnamestcSAML2_0metadata_AuthnAuthorityDescriptorType_urnoasisnamestcSAML2_0metadataAssertionIDRequestService', True, pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/saml20/schemas/metadata.xsd', 240, 4), )

    
    AssertionIDRequestService = property(__AssertionIDRequestService.value, __AssertionIDRequestService.set, None, None)

    
    # Element {urn:oasis:names:tc:SAML:2.0:metadata}AuthnQueryService uses Python identifier AuthnQueryService
    __AuthnQueryService = pyxb.binding.content.ElementDeclaration(pyxb.namespace.ExpandedName(Namespace, 'AuthnQueryService'), 'AuthnQueryService', '__urnoasisnamestcSAML2_0metadata_AuthnAuthorityDescriptorType_urnoasisnamestcSAML2_0metadataAuthnQueryService', True, pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/saml20/schemas/metadata.xsd', 290, 4), )

    
    AuthnQueryService = property(__AuthnQueryService.value, __AuthnQueryService.set, None, None)

    
    # Attribute ID inherited from {urn:oasis:names:tc:SAML:2.0:metadata}RoleDescriptorType
    
    # Attribute validUntil inherited from {urn:oasis:names:tc:SAML:2.0:metadata}RoleDescriptorType
    
    # Attribute cacheDuration inherited from {urn:oasis:names:tc:SAML:2.0:metadata}RoleDescriptorType
    
    # Attribute protocolSupportEnumeration inherited from {urn:oasis:names:tc:SAML:2.0:metadata}RoleDescriptorType
    
    # Attribute errorURL inherited from {urn:oasis:names:tc:SAML:2.0:metadata}RoleDescriptorType
    _AttributeWildcard = pyxb.binding.content.Wildcard(process_contents=pyxb.binding.content.Wildcard.PC_lax, namespace_constraint=(pyxb.binding.content.Wildcard.NC_not, 'urn:oasis:names:tc:SAML:2.0:metadata'))
    _ElementMap.update({
        __NameIDFormat.name() : __NameIDFormat,
        __AssertionIDRequestService.name() : __AssertionIDRequestService,
        __AuthnQueryService.name() : __AuthnQueryService
    })
    _AttributeMap.update({
        
    })
Namespace.addCategoryObject('typeBinding', 'AuthnAuthorityDescriptorType', AuthnAuthorityDescriptorType)


# Complex type {urn:oasis:names:tc:SAML:2.0:metadata}PDPDescriptorType with content type ELEMENT_ONLY
class PDPDescriptorType (RoleDescriptorType):
    """Complex type {urn:oasis:names:tc:SAML:2.0:metadata}PDPDescriptorType with content type ELEMENT_ONLY"""
    _TypeDefinition = None
    _ContentTypeTag = pyxb.binding.basis.complexTypeDefinition._CT_ELEMENT_ONLY
    _Abstract = False
    _ExpandedName = pyxb.namespace.ExpandedName(Namespace, 'PDPDescriptorType')
    _XSDLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/saml20/schemas/metadata.xsd', 293, 4)
    _ElementMap = RoleDescriptorType._ElementMap.copy()
    _AttributeMap = RoleDescriptorType._AttributeMap.copy()
    # Base type is RoleDescriptorType
    
    # Element Signature ({http://www.w3.org/2000/09/xmldsig#}Signature) inherited from {urn:oasis:names:tc:SAML:2.0:metadata}RoleDescriptorType
    
    # Element Extensions ({urn:oasis:names:tc:SAML:2.0:metadata}Extensions) inherited from {urn:oasis:names:tc:SAML:2.0:metadata}RoleDescriptorType
    
    # Element Organization ({urn:oasis:names:tc:SAML:2.0:metadata}Organization) inherited from {urn:oasis:names:tc:SAML:2.0:metadata}RoleDescriptorType
    
    # Element ContactPerson ({urn:oasis:names:tc:SAML:2.0:metadata}ContactPerson) inherited from {urn:oasis:names:tc:SAML:2.0:metadata}RoleDescriptorType
    
    # Element KeyDescriptor ({urn:oasis:names:tc:SAML:2.0:metadata}KeyDescriptor) inherited from {urn:oasis:names:tc:SAML:2.0:metadata}RoleDescriptorType
    
    # Element {urn:oasis:names:tc:SAML:2.0:metadata}NameIDFormat uses Python identifier NameIDFormat
    __NameIDFormat = pyxb.binding.content.ElementDeclaration(pyxb.namespace.ExpandedName(Namespace, 'NameIDFormat'), 'NameIDFormat', '__urnoasisnamestcSAML2_0metadata_PDPDescriptorType_urnoasisnamestcSAML2_0metadataNameIDFormat', True, pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/saml20/schemas/metadata.xsd', 221, 4), )

    
    NameIDFormat = property(__NameIDFormat.value, __NameIDFormat.set, None, None)

    
    # Element {urn:oasis:names:tc:SAML:2.0:metadata}AssertionIDRequestService uses Python identifier AssertionIDRequestService
    __AssertionIDRequestService = pyxb.binding.content.ElementDeclaration(pyxb.namespace.ExpandedName(Namespace, 'AssertionIDRequestService'), 'AssertionIDRequestService', '__urnoasisnamestcSAML2_0metadata_PDPDescriptorType_urnoasisnamestcSAML2_0metadataAssertionIDRequestService', True, pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/saml20/schemas/metadata.xsd', 240, 4), )

    
    AssertionIDRequestService = property(__AssertionIDRequestService.value, __AssertionIDRequestService.set, None, None)

    
    # Element {urn:oasis:names:tc:SAML:2.0:metadata}AuthzService uses Python identifier AuthzService
    __AuthzService = pyxb.binding.content.ElementDeclaration(pyxb.namespace.ExpandedName(Namespace, 'AuthzService'), 'AuthzService', '__urnoasisnamestcSAML2_0metadata_PDPDescriptorType_urnoasisnamestcSAML2_0metadataAuthzService', True, pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/saml20/schemas/metadata.xsd', 304, 4), )

    
    AuthzService = property(__AuthzService.value, __AuthzService.set, None, None)

    
    # Attribute ID inherited from {urn:oasis:names:tc:SAML:2.0:metadata}RoleDescriptorType
    
    # Attribute validUntil inherited from {urn:oasis:names:tc:SAML:2.0:metadata}RoleDescriptorType
    
    # Attribute cacheDuration inherited from {urn:oasis:names:tc:SAML:2.0:metadata}RoleDescriptorType
    
    # Attribute protocolSupportEnumeration inherited from {urn:oasis:names:tc:SAML:2.0:metadata}RoleDescriptorType
    
    # Attribute errorURL inherited from {urn:oasis:names:tc:SAML:2.0:metadata}RoleDescriptorType
    _AttributeWildcard = pyxb.binding.content.Wildcard(process_contents=pyxb.binding.content.Wildcard.PC_lax, namespace_constraint=(pyxb.binding.content.Wildcard.NC_not, 'urn:oasis:names:tc:SAML:2.0:metadata'))
    _ElementMap.update({
        __NameIDFormat.name() : __NameIDFormat,
        __AssertionIDRequestService.name() : __AssertionIDRequestService,
        __AuthzService.name() : __AuthzService
    })
    _AttributeMap.update({
        
    })
Namespace.addCategoryObject('typeBinding', 'PDPDescriptorType', PDPDescriptorType)


# Complex type {urn:oasis:names:tc:SAML:2.0:metadata}AttributeAuthorityDescriptorType with content type ELEMENT_ONLY
class AttributeAuthorityDescriptorType (RoleDescriptorType):
    """Complex type {urn:oasis:names:tc:SAML:2.0:metadata}AttributeAuthorityDescriptorType with content type ELEMENT_ONLY"""
    _TypeDefinition = None
    _ContentTypeTag = pyxb.binding.basis.complexTypeDefinition._CT_ELEMENT_ONLY
    _Abstract = False
    _ExpandedName = pyxb.namespace.ExpandedName(Namespace, 'AttributeAuthorityDescriptorType')
    _XSDLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/saml20/schemas/metadata.xsd', 307, 4)
    _ElementMap = RoleDescriptorType._ElementMap.copy()
    _AttributeMap = RoleDescriptorType._AttributeMap.copy()
    # Base type is RoleDescriptorType
    
    # Element Signature ({http://www.w3.org/2000/09/xmldsig#}Signature) inherited from {urn:oasis:names:tc:SAML:2.0:metadata}RoleDescriptorType
    
    # Element {urn:oasis:names:tc:SAML:2.0:assertion}Attribute uses Python identifier Attribute
    __Attribute = pyxb.binding.content.ElementDeclaration(pyxb.namespace.ExpandedName(_Namespace_saml, 'Attribute'), 'Attribute', '__urnoasisnamestcSAML2_0metadata_AttributeAuthorityDescriptorType_urnoasisnamestcSAML2_0assertionAttribute', True, pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/saml20/schemas/assertion.xsd', 271, 4), )

    
    Attribute = property(__Attribute.value, __Attribute.set, None, None)

    
    # Element Extensions ({urn:oasis:names:tc:SAML:2.0:metadata}Extensions) inherited from {urn:oasis:names:tc:SAML:2.0:metadata}RoleDescriptorType
    
    # Element Organization ({urn:oasis:names:tc:SAML:2.0:metadata}Organization) inherited from {urn:oasis:names:tc:SAML:2.0:metadata}RoleDescriptorType
    
    # Element ContactPerson ({urn:oasis:names:tc:SAML:2.0:metadata}ContactPerson) inherited from {urn:oasis:names:tc:SAML:2.0:metadata}RoleDescriptorType
    
    # Element KeyDescriptor ({urn:oasis:names:tc:SAML:2.0:metadata}KeyDescriptor) inherited from {urn:oasis:names:tc:SAML:2.0:metadata}RoleDescriptorType
    
    # Element {urn:oasis:names:tc:SAML:2.0:metadata}NameIDFormat uses Python identifier NameIDFormat
    __NameIDFormat = pyxb.binding.content.ElementDeclaration(pyxb.namespace.ExpandedName(Namespace, 'NameIDFormat'), 'NameIDFormat', '__urnoasisnamestcSAML2_0metadata_AttributeAuthorityDescriptorType_urnoasisnamestcSAML2_0metadataNameIDFormat', True, pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/saml20/schemas/metadata.xsd', 221, 4), )

    
    NameIDFormat = property(__NameIDFormat.value, __NameIDFormat.set, None, None)

    
    # Element {urn:oasis:names:tc:SAML:2.0:metadata}AssertionIDRequestService uses Python identifier AssertionIDRequestService
    __AssertionIDRequestService = pyxb.binding.content.ElementDeclaration(pyxb.namespace.ExpandedName(Namespace, 'AssertionIDRequestService'), 'AssertionIDRequestService', '__urnoasisnamestcSAML2_0metadata_AttributeAuthorityDescriptorType_urnoasisnamestcSAML2_0metadataAssertionIDRequestService', True, pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/saml20/schemas/metadata.xsd', 240, 4), )

    
    AssertionIDRequestService = property(__AssertionIDRequestService.value, __AssertionIDRequestService.set, None, None)

    
    # Element {urn:oasis:names:tc:SAML:2.0:metadata}AttributeProfile uses Python identifier AttributeProfile
    __AttributeProfile = pyxb.binding.content.ElementDeclaration(pyxb.namespace.ExpandedName(Namespace, 'AttributeProfile'), 'AttributeProfile', '__urnoasisnamestcSAML2_0metadata_AttributeAuthorityDescriptorType_urnoasisnamestcSAML2_0metadataAttributeProfile', True, pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/saml20/schemas/metadata.xsd', 241, 4), )

    
    AttributeProfile = property(__AttributeProfile.value, __AttributeProfile.set, None, None)

    
    # Element {urn:oasis:names:tc:SAML:2.0:metadata}AttributeService uses Python identifier AttributeService
    __AttributeService = pyxb.binding.content.ElementDeclaration(pyxb.namespace.ExpandedName(Namespace, 'AttributeService'), 'AttributeService', '__urnoasisnamestcSAML2_0metadata_AttributeAuthorityDescriptorType_urnoasisnamestcSAML2_0metadataAttributeService', True, pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/saml20/schemas/metadata.xsd', 320, 4), )

    
    AttributeService = property(__AttributeService.value, __AttributeService.set, None, None)

    
    # Attribute ID inherited from {urn:oasis:names:tc:SAML:2.0:metadata}RoleDescriptorType
    
    # Attribute validUntil inherited from {urn:oasis:names:tc:SAML:2.0:metadata}RoleDescriptorType
    
    # Attribute cacheDuration inherited from {urn:oasis:names:tc:SAML:2.0:metadata}RoleDescriptorType
    
    # Attribute protocolSupportEnumeration inherited from {urn:oasis:names:tc:SAML:2.0:metadata}RoleDescriptorType
    
    # Attribute errorURL inherited from {urn:oasis:names:tc:SAML:2.0:metadata}RoleDescriptorType
    _AttributeWildcard = pyxb.binding.content.Wildcard(process_contents=pyxb.binding.content.Wildcard.PC_lax, namespace_constraint=(pyxb.binding.content.Wildcard.NC_not, 'urn:oasis:names:tc:SAML:2.0:metadata'))
    _ElementMap.update({
        __Attribute.name() : __Attribute,
        __NameIDFormat.name() : __NameIDFormat,
        __AssertionIDRequestService.name() : __AssertionIDRequestService,
        __AttributeProfile.name() : __AttributeProfile,
        __AttributeService.name() : __AttributeService
    })
    _AttributeMap.update({
        
    })
Namespace.addCategoryObject('typeBinding', 'AttributeAuthorityDescriptorType', AttributeAuthorityDescriptorType)


# Complex type {urn:oasis:names:tc:SAML:2.0:metadata}IDPSSODescriptorType with content type ELEMENT_ONLY
class IDPSSODescriptorType (SSODescriptorType):
    """Complex type {urn:oasis:names:tc:SAML:2.0:metadata}IDPSSODescriptorType with content type ELEMENT_ONLY"""
    _TypeDefinition = None
    _ContentTypeTag = pyxb.binding.basis.complexTypeDefinition._CT_ELEMENT_ONLY
    _Abstract = False
    _ExpandedName = pyxb.namespace.ExpandedName(Namespace, 'IDPSSODescriptorType')
    _XSDLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/saml20/schemas/metadata.xsd', 224, 4)
    _ElementMap = SSODescriptorType._ElementMap.copy()
    _AttributeMap = SSODescriptorType._AttributeMap.copy()
    # Base type is SSODescriptorType
    
    # Element Signature ({http://www.w3.org/2000/09/xmldsig#}Signature) inherited from {urn:oasis:names:tc:SAML:2.0:metadata}RoleDescriptorType
    
    # Element {urn:oasis:names:tc:SAML:2.0:assertion}Attribute uses Python identifier Attribute
    __Attribute = pyxb.binding.content.ElementDeclaration(pyxb.namespace.ExpandedName(_Namespace_saml, 'Attribute'), 'Attribute', '__urnoasisnamestcSAML2_0metadata_IDPSSODescriptorType_urnoasisnamestcSAML2_0assertionAttribute', True, pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/saml20/schemas/assertion.xsd', 271, 4), )

    
    Attribute = property(__Attribute.value, __Attribute.set, None, None)

    
    # Element Extensions ({urn:oasis:names:tc:SAML:2.0:metadata}Extensions) inherited from {urn:oasis:names:tc:SAML:2.0:metadata}RoleDescriptorType
    
    # Element Organization ({urn:oasis:names:tc:SAML:2.0:metadata}Organization) inherited from {urn:oasis:names:tc:SAML:2.0:metadata}RoleDescriptorType
    
    # Element ContactPerson ({urn:oasis:names:tc:SAML:2.0:metadata}ContactPerson) inherited from {urn:oasis:names:tc:SAML:2.0:metadata}RoleDescriptorType
    
    # Element KeyDescriptor ({urn:oasis:names:tc:SAML:2.0:metadata}KeyDescriptor) inherited from {urn:oasis:names:tc:SAML:2.0:metadata}RoleDescriptorType
    
    # Element ArtifactResolutionService ({urn:oasis:names:tc:SAML:2.0:metadata}ArtifactResolutionService) inherited from {urn:oasis:names:tc:SAML:2.0:metadata}SSODescriptorType
    
    # Element SingleLogoutService ({urn:oasis:names:tc:SAML:2.0:metadata}SingleLogoutService) inherited from {urn:oasis:names:tc:SAML:2.0:metadata}SSODescriptorType
    
    # Element ManageNameIDService ({urn:oasis:names:tc:SAML:2.0:metadata}ManageNameIDService) inherited from {urn:oasis:names:tc:SAML:2.0:metadata}SSODescriptorType
    
    # Element NameIDFormat ({urn:oasis:names:tc:SAML:2.0:metadata}NameIDFormat) inherited from {urn:oasis:names:tc:SAML:2.0:metadata}SSODescriptorType
    
    # Element {urn:oasis:names:tc:SAML:2.0:metadata}SingleSignOnService uses Python identifier SingleSignOnService
    __SingleSignOnService = pyxb.binding.content.ElementDeclaration(pyxb.namespace.ExpandedName(Namespace, 'SingleSignOnService'), 'SingleSignOnService', '__urnoasisnamestcSAML2_0metadata_IDPSSODescriptorType_urnoasisnamestcSAML2_0metadataSingleSignOnService', True, pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/saml20/schemas/metadata.xsd', 238, 4), )

    
    SingleSignOnService = property(__SingleSignOnService.value, __SingleSignOnService.set, None, None)

    
    # Element {urn:oasis:names:tc:SAML:2.0:metadata}NameIDMappingService uses Python identifier NameIDMappingService
    __NameIDMappingService = pyxb.binding.content.ElementDeclaration(pyxb.namespace.ExpandedName(Namespace, 'NameIDMappingService'), 'NameIDMappingService', '__urnoasisnamestcSAML2_0metadata_IDPSSODescriptorType_urnoasisnamestcSAML2_0metadataNameIDMappingService', True, pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/saml20/schemas/metadata.xsd', 239, 4), )

    
    NameIDMappingService = property(__NameIDMappingService.value, __NameIDMappingService.set, None, None)

    
    # Element {urn:oasis:names:tc:SAML:2.0:metadata}AssertionIDRequestService uses Python identifier AssertionIDRequestService
    __AssertionIDRequestService = pyxb.binding.content.ElementDeclaration(pyxb.namespace.ExpandedName(Namespace, 'AssertionIDRequestService'), 'AssertionIDRequestService', '__urnoasisnamestcSAML2_0metadata_IDPSSODescriptorType_urnoasisnamestcSAML2_0metadataAssertionIDRequestService', True, pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/saml20/schemas/metadata.xsd', 240, 4), )

    
    AssertionIDRequestService = property(__AssertionIDRequestService.value, __AssertionIDRequestService.set, None, None)

    
    # Element {urn:oasis:names:tc:SAML:2.0:metadata}AttributeProfile uses Python identifier AttributeProfile
    __AttributeProfile = pyxb.binding.content.ElementDeclaration(pyxb.namespace.ExpandedName(Namespace, 'AttributeProfile'), 'AttributeProfile', '__urnoasisnamestcSAML2_0metadata_IDPSSODescriptorType_urnoasisnamestcSAML2_0metadataAttributeProfile', True, pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/saml20/schemas/metadata.xsd', 241, 4), )

    
    AttributeProfile = property(__AttributeProfile.value, __AttributeProfile.set, None, None)

    
    # Attribute ID inherited from {urn:oasis:names:tc:SAML:2.0:metadata}RoleDescriptorType
    
    # Attribute validUntil inherited from {urn:oasis:names:tc:SAML:2.0:metadata}RoleDescriptorType
    
    # Attribute cacheDuration inherited from {urn:oasis:names:tc:SAML:2.0:metadata}RoleDescriptorType
    
    # Attribute protocolSupportEnumeration inherited from {urn:oasis:names:tc:SAML:2.0:metadata}RoleDescriptorType
    
    # Attribute errorURL inherited from {urn:oasis:names:tc:SAML:2.0:metadata}RoleDescriptorType
    
    # Attribute WantAuthnRequestsSigned uses Python identifier WantAuthnRequestsSigned
    __WantAuthnRequestsSigned = pyxb.binding.content.AttributeUse(pyxb.namespace.ExpandedName(None, 'WantAuthnRequestsSigned'), 'WantAuthnRequestsSigned', '__urnoasisnamestcSAML2_0metadata_IDPSSODescriptorType_WantAuthnRequestsSigned', pyxb.binding.datatypes.boolean)
    __WantAuthnRequestsSigned._DeclarationLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/saml20/schemas/metadata.xsd', 234, 16)
    __WantAuthnRequestsSigned._UseLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/saml20/schemas/metadata.xsd', 234, 16)
    
    WantAuthnRequestsSigned = property(__WantAuthnRequestsSigned.value, __WantAuthnRequestsSigned.set, None, None)

    _AttributeWildcard = pyxb.binding.content.Wildcard(process_contents=pyxb.binding.content.Wildcard.PC_lax, namespace_constraint=(pyxb.binding.content.Wildcard.NC_not, 'urn:oasis:names:tc:SAML:2.0:metadata'))
    _ElementMap.update({
        __Attribute.name() : __Attribute,
        __SingleSignOnService.name() : __SingleSignOnService,
        __NameIDMappingService.name() : __NameIDMappingService,
        __AssertionIDRequestService.name() : __AssertionIDRequestService,
        __AttributeProfile.name() : __AttributeProfile
    })
    _AttributeMap.update({
        __WantAuthnRequestsSigned.name() : __WantAuthnRequestsSigned
    })
Namespace.addCategoryObject('typeBinding', 'IDPSSODescriptorType', IDPSSODescriptorType)


# Complex type {urn:oasis:names:tc:SAML:2.0:metadata}SPSSODescriptorType with content type ELEMENT_ONLY
class SPSSODescriptorType (SSODescriptorType):
    """Complex type {urn:oasis:names:tc:SAML:2.0:metadata}SPSSODescriptorType with content type ELEMENT_ONLY"""
    _TypeDefinition = None
    _ContentTypeTag = pyxb.binding.basis.complexTypeDefinition._CT_ELEMENT_ONLY
    _Abstract = False
    _ExpandedName = pyxb.namespace.ExpandedName(Namespace, 'SPSSODescriptorType')
    _XSDLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/saml20/schemas/metadata.xsd', 244, 4)
    _ElementMap = SSODescriptorType._ElementMap.copy()
    _AttributeMap = SSODescriptorType._AttributeMap.copy()
    # Base type is SSODescriptorType
    
    # Element Signature ({http://www.w3.org/2000/09/xmldsig#}Signature) inherited from {urn:oasis:names:tc:SAML:2.0:metadata}RoleDescriptorType
    
    # Element Extensions ({urn:oasis:names:tc:SAML:2.0:metadata}Extensions) inherited from {urn:oasis:names:tc:SAML:2.0:metadata}RoleDescriptorType
    
    # Element Organization ({urn:oasis:names:tc:SAML:2.0:metadata}Organization) inherited from {urn:oasis:names:tc:SAML:2.0:metadata}RoleDescriptorType
    
    # Element ContactPerson ({urn:oasis:names:tc:SAML:2.0:metadata}ContactPerson) inherited from {urn:oasis:names:tc:SAML:2.0:metadata}RoleDescriptorType
    
    # Element KeyDescriptor ({urn:oasis:names:tc:SAML:2.0:metadata}KeyDescriptor) inherited from {urn:oasis:names:tc:SAML:2.0:metadata}RoleDescriptorType
    
    # Element ArtifactResolutionService ({urn:oasis:names:tc:SAML:2.0:metadata}ArtifactResolutionService) inherited from {urn:oasis:names:tc:SAML:2.0:metadata}SSODescriptorType
    
    # Element SingleLogoutService ({urn:oasis:names:tc:SAML:2.0:metadata}SingleLogoutService) inherited from {urn:oasis:names:tc:SAML:2.0:metadata}SSODescriptorType
    
    # Element ManageNameIDService ({urn:oasis:names:tc:SAML:2.0:metadata}ManageNameIDService) inherited from {urn:oasis:names:tc:SAML:2.0:metadata}SSODescriptorType
    
    # Element NameIDFormat ({urn:oasis:names:tc:SAML:2.0:metadata}NameIDFormat) inherited from {urn:oasis:names:tc:SAML:2.0:metadata}SSODescriptorType
    
    # Element {urn:oasis:names:tc:SAML:2.0:metadata}AssertionConsumerService uses Python identifier AssertionConsumerService
    __AssertionConsumerService = pyxb.binding.content.ElementDeclaration(pyxb.namespace.ExpandedName(Namespace, 'AssertionConsumerService'), 'AssertionConsumerService', '__urnoasisnamestcSAML2_0metadata_SPSSODescriptorType_urnoasisnamestcSAML2_0metadataAssertionConsumerService', True, pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/saml20/schemas/metadata.xsd', 256, 4), )

    
    AssertionConsumerService = property(__AssertionConsumerService.value, __AssertionConsumerService.set, None, None)

    
    # Element {urn:oasis:names:tc:SAML:2.0:metadata}AttributeConsumingService uses Python identifier AttributeConsumingService
    __AttributeConsumingService = pyxb.binding.content.ElementDeclaration(pyxb.namespace.ExpandedName(Namespace, 'AttributeConsumingService'), 'AttributeConsumingService', '__urnoasisnamestcSAML2_0metadata_SPSSODescriptorType_urnoasisnamestcSAML2_0metadataAttributeConsumingService', True, pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/saml20/schemas/metadata.xsd', 257, 4), )

    
    AttributeConsumingService = property(__AttributeConsumingService.value, __AttributeConsumingService.set, None, None)

    
    # Attribute ID inherited from {urn:oasis:names:tc:SAML:2.0:metadata}RoleDescriptorType
    
    # Attribute validUntil inherited from {urn:oasis:names:tc:SAML:2.0:metadata}RoleDescriptorType
    
    # Attribute cacheDuration inherited from {urn:oasis:names:tc:SAML:2.0:metadata}RoleDescriptorType
    
    # Attribute protocolSupportEnumeration inherited from {urn:oasis:names:tc:SAML:2.0:metadata}RoleDescriptorType
    
    # Attribute errorURL inherited from {urn:oasis:names:tc:SAML:2.0:metadata}RoleDescriptorType
    
    # Attribute AuthnRequestsSigned uses Python identifier AuthnRequestsSigned
    __AuthnRequestsSigned = pyxb.binding.content.AttributeUse(pyxb.namespace.ExpandedName(None, 'AuthnRequestsSigned'), 'AuthnRequestsSigned', '__urnoasisnamestcSAML2_0metadata_SPSSODescriptorType_AuthnRequestsSigned', pyxb.binding.datatypes.boolean)
    __AuthnRequestsSigned._DeclarationLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/saml20/schemas/metadata.xsd', 251, 16)
    __AuthnRequestsSigned._UseLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/saml20/schemas/metadata.xsd', 251, 16)
    
    AuthnRequestsSigned = property(__AuthnRequestsSigned.value, __AuthnRequestsSigned.set, None, None)

    
    # Attribute WantAssertionsSigned uses Python identifier WantAssertionsSigned
    __WantAssertionsSigned = pyxb.binding.content.AttributeUse(pyxb.namespace.ExpandedName(None, 'WantAssertionsSigned'), 'WantAssertionsSigned', '__urnoasisnamestcSAML2_0metadata_SPSSODescriptorType_WantAssertionsSigned', pyxb.binding.datatypes.boolean)
    __WantAssertionsSigned._DeclarationLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/saml20/schemas/metadata.xsd', 252, 16)
    __WantAssertionsSigned._UseLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/saml20/schemas/metadata.xsd', 252, 16)
    
    WantAssertionsSigned = property(__WantAssertionsSigned.value, __WantAssertionsSigned.set, None, None)

    _AttributeWildcard = pyxb.binding.content.Wildcard(process_contents=pyxb.binding.content.Wildcard.PC_lax, namespace_constraint=(pyxb.binding.content.Wildcard.NC_not, 'urn:oasis:names:tc:SAML:2.0:metadata'))
    _ElementMap.update({
        __AssertionConsumerService.name() : __AssertionConsumerService,
        __AttributeConsumingService.name() : __AttributeConsumingService
    })
    _AttributeMap.update({
        __AuthnRequestsSigned.name() : __AuthnRequestsSigned,
        __WantAssertionsSigned.name() : __WantAssertionsSigned
    })
Namespace.addCategoryObject('typeBinding', 'SPSSODescriptorType', SPSSODescriptorType)


Company = pyxb.binding.basis.element(pyxb.namespace.ExpandedName(Namespace, 'Company'), pyxb.binding.datatypes.string, location=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/saml20/schemas/metadata.xsd', 146, 4))
Namespace.addCategoryObject('elementBinding', Company.name().localName(), Company)

GivenName = pyxb.binding.basis.element(pyxb.namespace.ExpandedName(Namespace, 'GivenName'), pyxb.binding.datatypes.string, location=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/saml20/schemas/metadata.xsd', 147, 4))
Namespace.addCategoryObject('elementBinding', GivenName.name().localName(), GivenName)

SurName = pyxb.binding.basis.element(pyxb.namespace.ExpandedName(Namespace, 'SurName'), pyxb.binding.datatypes.string, location=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/saml20/schemas/metadata.xsd', 148, 4))
Namespace.addCategoryObject('elementBinding', SurName.name().localName(), SurName)

EmailAddress = pyxb.binding.basis.element(pyxb.namespace.ExpandedName(Namespace, 'EmailAddress'), pyxb.binding.datatypes.anyURI, location=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/saml20/schemas/metadata.xsd', 149, 4))
Namespace.addCategoryObject('elementBinding', EmailAddress.name().localName(), EmailAddress)

TelephoneNumber = pyxb.binding.basis.element(pyxb.namespace.ExpandedName(Namespace, 'TelephoneNumber'), pyxb.binding.datatypes.string, location=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/saml20/schemas/metadata.xsd', 150, 4))
Namespace.addCategoryObject('elementBinding', TelephoneNumber.name().localName(), TelephoneNumber)

EncryptionMethod = pyxb.binding.basis.element(pyxb.namespace.ExpandedName(Namespace, 'EncryptionMethod'), pyxb.bundles.wssplat.xenc.EncryptionMethodType, location=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/saml20/schemas/metadata.xsd', 204, 4))
Namespace.addCategoryObject('elementBinding', EncryptionMethod.name().localName(), EncryptionMethod)

NameIDFormat = pyxb.binding.basis.element(pyxb.namespace.ExpandedName(Namespace, 'NameIDFormat'), pyxb.binding.datatypes.anyURI, location=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/saml20/schemas/metadata.xsd', 221, 4))
Namespace.addCategoryObject('elementBinding', NameIDFormat.name().localName(), NameIDFormat)

AttributeProfile = pyxb.binding.basis.element(pyxb.namespace.ExpandedName(Namespace, 'AttributeProfile'), pyxb.binding.datatypes.anyURI, location=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/saml20/schemas/metadata.xsd', 241, 4))
Namespace.addCategoryObject('elementBinding', AttributeProfile.name().localName(), AttributeProfile)

Extensions = pyxb.binding.basis.element(pyxb.namespace.ExpandedName(Namespace, 'Extensions'), ExtensionsType, location=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/saml20/schemas/metadata.xsd', 51, 4))
Namespace.addCategoryObject('elementBinding', Extensions.name().localName(), Extensions)

EntitiesDescriptor = pyxb.binding.basis.element(pyxb.namespace.ExpandedName(Namespace, 'EntitiesDescriptor'), EntitiesDescriptorType, location=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/saml20/schemas/metadata.xsd', 77, 4))
Namespace.addCategoryObject('elementBinding', EntitiesDescriptor.name().localName(), EntitiesDescriptor)

Organization = pyxb.binding.basis.element(pyxb.namespace.ExpandedName(Namespace, 'Organization'), OrganizationType, location=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/saml20/schemas/metadata.xsd', 120, 4))
Namespace.addCategoryObject('elementBinding', Organization.name().localName(), Organization)

OrganizationName = pyxb.binding.basis.element(pyxb.namespace.ExpandedName(Namespace, 'OrganizationName'), localizedNameType, location=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/saml20/schemas/metadata.xsd', 130, 4))
Namespace.addCategoryObject('elementBinding', OrganizationName.name().localName(), OrganizationName)

OrganizationDisplayName = pyxb.binding.basis.element(pyxb.namespace.ExpandedName(Namespace, 'OrganizationDisplayName'), localizedNameType, location=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/saml20/schemas/metadata.xsd', 131, 4))
Namespace.addCategoryObject('elementBinding', OrganizationDisplayName.name().localName(), OrganizationDisplayName)

OrganizationURL = pyxb.binding.basis.element(pyxb.namespace.ExpandedName(Namespace, 'OrganizationURL'), localizedURIType, location=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/saml20/schemas/metadata.xsd', 132, 4))
Namespace.addCategoryObject('elementBinding', OrganizationURL.name().localName(), OrganizationURL)

AdditionalMetadataLocation = pyxb.binding.basis.element(pyxb.namespace.ExpandedName(Namespace, 'AdditionalMetadataLocation'), AdditionalMetadataLocationType, location=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/saml20/schemas/metadata.xsd', 161, 4))
Namespace.addCategoryObject('elementBinding', AdditionalMetadataLocation.name().localName(), AdditionalMetadataLocation)

SingleLogoutService = pyxb.binding.basis.element(pyxb.namespace.ExpandedName(Namespace, 'SingleLogoutService'), EndpointType, location=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/saml20/schemas/metadata.xsd', 219, 4))
Namespace.addCategoryObject('elementBinding', SingleLogoutService.name().localName(), SingleLogoutService)

ManageNameIDService = pyxb.binding.basis.element(pyxb.namespace.ExpandedName(Namespace, 'ManageNameIDService'), EndpointType, location=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/saml20/schemas/metadata.xsd', 220, 4))
Namespace.addCategoryObject('elementBinding', ManageNameIDService.name().localName(), ManageNameIDService)

SingleSignOnService = pyxb.binding.basis.element(pyxb.namespace.ExpandedName(Namespace, 'SingleSignOnService'), EndpointType, location=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/saml20/schemas/metadata.xsd', 238, 4))
Namespace.addCategoryObject('elementBinding', SingleSignOnService.name().localName(), SingleSignOnService)

NameIDMappingService = pyxb.binding.basis.element(pyxb.namespace.ExpandedName(Namespace, 'NameIDMappingService'), EndpointType, location=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/saml20/schemas/metadata.xsd', 239, 4))
Namespace.addCategoryObject('elementBinding', NameIDMappingService.name().localName(), NameIDMappingService)

AssertionIDRequestService = pyxb.binding.basis.element(pyxb.namespace.ExpandedName(Namespace, 'AssertionIDRequestService'), EndpointType, location=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/saml20/schemas/metadata.xsd', 240, 4))
Namespace.addCategoryObject('elementBinding', AssertionIDRequestService.name().localName(), AssertionIDRequestService)

AttributeConsumingService = pyxb.binding.basis.element(pyxb.namespace.ExpandedName(Namespace, 'AttributeConsumingService'), AttributeConsumingServiceType, location=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/saml20/schemas/metadata.xsd', 257, 4))
Namespace.addCategoryObject('elementBinding', AttributeConsumingService.name().localName(), AttributeConsumingService)

ServiceName = pyxb.binding.basis.element(pyxb.namespace.ExpandedName(Namespace, 'ServiceName'), localizedNameType, location=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/saml20/schemas/metadata.xsd', 267, 4))
Namespace.addCategoryObject('elementBinding', ServiceName.name().localName(), ServiceName)

ServiceDescription = pyxb.binding.basis.element(pyxb.namespace.ExpandedName(Namespace, 'ServiceDescription'), localizedNameType, location=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/saml20/schemas/metadata.xsd', 268, 4))
Namespace.addCategoryObject('elementBinding', ServiceDescription.name().localName(), ServiceDescription)

RequestedAttribute = pyxb.binding.basis.element(pyxb.namespace.ExpandedName(Namespace, 'RequestedAttribute'), RequestedAttributeType, location=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/saml20/schemas/metadata.xsd', 269, 4))
Namespace.addCategoryObject('elementBinding', RequestedAttribute.name().localName(), RequestedAttribute)

AuthnQueryService = pyxb.binding.basis.element(pyxb.namespace.ExpandedName(Namespace, 'AuthnQueryService'), EndpointType, location=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/saml20/schemas/metadata.xsd', 290, 4))
Namespace.addCategoryObject('elementBinding', AuthnQueryService.name().localName(), AuthnQueryService)

AuthzService = pyxb.binding.basis.element(pyxb.namespace.ExpandedName(Namespace, 'AuthzService'), EndpointType, location=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/saml20/schemas/metadata.xsd', 304, 4))
Namespace.addCategoryObject('elementBinding', AuthzService.name().localName(), AuthzService)

AttributeService = pyxb.binding.basis.element(pyxb.namespace.ExpandedName(Namespace, 'AttributeService'), EndpointType, location=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/saml20/schemas/metadata.xsd', 320, 4))
Namespace.addCategoryObject('elementBinding', AttributeService.name().localName(), AttributeService)

AffiliateMember = pyxb.binding.basis.element(pyxb.namespace.ExpandedName(Namespace, 'AffiliateMember'), entityIDType, location=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/saml20/schemas/metadata.xsd', 336, 4))
Namespace.addCategoryObject('elementBinding', AffiliateMember.name().localName(), AffiliateMember)

EntityDescriptor = pyxb.binding.basis.element(pyxb.namespace.ExpandedName(Namespace, 'EntityDescriptor'), EntityDescriptorType, location=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/saml20/schemas/metadata.xsd', 93, 4))
Namespace.addCategoryObject('elementBinding', EntityDescriptor.name().localName(), EntityDescriptor)

ContactPerson = pyxb.binding.basis.element(pyxb.namespace.ExpandedName(Namespace, 'ContactPerson'), ContactType, location=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/saml20/schemas/metadata.xsd', 133, 4))
Namespace.addCategoryObject('elementBinding', ContactPerson.name().localName(), ContactPerson)

RoleDescriptor = pyxb.binding.basis.element(pyxb.namespace.ExpandedName(Namespace, 'RoleDescriptor'), RoleDescriptorType, location=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/saml20/schemas/metadata.xsd', 170, 4))
Namespace.addCategoryObject('elementBinding', RoleDescriptor.name().localName(), RoleDescriptor)

KeyDescriptor = pyxb.binding.basis.element(pyxb.namespace.ExpandedName(Namespace, 'KeyDescriptor'), KeyDescriptorType, location=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/saml20/schemas/metadata.xsd', 190, 4))
Namespace.addCategoryObject('elementBinding', KeyDescriptor.name().localName(), KeyDescriptor)

ArtifactResolutionService = pyxb.binding.basis.element(pyxb.namespace.ExpandedName(Namespace, 'ArtifactResolutionService'), IndexedEndpointType, location=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/saml20/schemas/metadata.xsd', 218, 4))
Namespace.addCategoryObject('elementBinding', ArtifactResolutionService.name().localName(), ArtifactResolutionService)

AssertionConsumerService = pyxb.binding.basis.element(pyxb.namespace.ExpandedName(Namespace, 'AssertionConsumerService'), IndexedEndpointType, location=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/saml20/schemas/metadata.xsd', 256, 4))
Namespace.addCategoryObject('elementBinding', AssertionConsumerService.name().localName(), AssertionConsumerService)

AffiliationDescriptor = pyxb.binding.basis.element(pyxb.namespace.ExpandedName(Namespace, 'AffiliationDescriptor'), AffiliationDescriptorType, location=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/saml20/schemas/metadata.xsd', 322, 4))
Namespace.addCategoryObject('elementBinding', AffiliationDescriptor.name().localName(), AffiliationDescriptor)

AuthnAuthorityDescriptor = pyxb.binding.basis.element(pyxb.namespace.ExpandedName(Namespace, 'AuthnAuthorityDescriptor'), AuthnAuthorityDescriptorType, location=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/saml20/schemas/metadata.xsd', 278, 4))
Namespace.addCategoryObject('elementBinding', AuthnAuthorityDescriptor.name().localName(), AuthnAuthorityDescriptor)

PDPDescriptor = pyxb.binding.basis.element(pyxb.namespace.ExpandedName(Namespace, 'PDPDescriptor'), PDPDescriptorType, location=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/saml20/schemas/metadata.xsd', 292, 4))
Namespace.addCategoryObject('elementBinding', PDPDescriptor.name().localName(), PDPDescriptor)

AttributeAuthorityDescriptor = pyxb.binding.basis.element(pyxb.namespace.ExpandedName(Namespace, 'AttributeAuthorityDescriptor'), AttributeAuthorityDescriptorType, location=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/saml20/schemas/metadata.xsd', 306, 4))
Namespace.addCategoryObject('elementBinding', AttributeAuthorityDescriptor.name().localName(), AttributeAuthorityDescriptor)

IDPSSODescriptor = pyxb.binding.basis.element(pyxb.namespace.ExpandedName(Namespace, 'IDPSSODescriptor'), IDPSSODescriptorType, location=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/saml20/schemas/metadata.xsd', 223, 4))
Namespace.addCategoryObject('elementBinding', IDPSSODescriptor.name().localName(), IDPSSODescriptor)

SPSSODescriptor = pyxb.binding.basis.element(pyxb.namespace.ExpandedName(Namespace, 'SPSSODescriptor'), SPSSODescriptorType, location=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/saml20/schemas/metadata.xsd', 243, 4))
Namespace.addCategoryObject('elementBinding', SPSSODescriptor.name().localName(), SPSSODescriptor)



def _BuildAutomaton ():
    # Remove this helper function from the namespace after it is invoked
    global _BuildAutomaton
    del _BuildAutomaton
    import pyxb.utils.fac as fac

    counters = set()
    states = []
    final_update = set()
    symbol = pyxb.binding.content.WildcardUse(pyxb.binding.content.Wildcard(process_contents=pyxb.binding.content.Wildcard.PC_lax, namespace_constraint=(pyxb.binding.content.Wildcard.NC_not, 'urn:oasis:names:tc:SAML:2.0:metadata')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/saml20/schemas/metadata.xsd', 54, 12))
    st_0 = fac.State(symbol, is_initial=True, final_update=final_update, is_unordered_catenation=False)
    states.append(st_0)
    transitions = []
    transitions.append(fac.Transition(st_0, [
         ]))
    st_0._set_transitionSet(transitions)
    return fac.Automaton(states, counters, False, containing_state=None)
ExtensionsType._Automaton = _BuildAutomaton()




def _BuildAutomaton_ ():
    # Remove this helper function from the namespace after it is invoked
    global _BuildAutomaton_
    del _BuildAutomaton_
    import pyxb.utils.fac as fac

    counters = set()
    cc_0 = fac.CounterCondition(min=0, max=None, metadata=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/saml20/schemas/metadata.xsd', 60, 12))
    counters.add(cc_0)
    states = []
    final_update = set()
    final_update.add(fac.UpdateInstruction(cc_0, False))
    symbol = pyxb.binding.content.WildcardUse(pyxb.binding.content.Wildcard(process_contents=pyxb.binding.content.Wildcard.PC_lax, namespace_constraint=(pyxb.binding.content.Wildcard.NC_not, 'urn:oasis:names:tc:SAML:2.0:metadata')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/saml20/schemas/metadata.xsd', 60, 12))
    st_0 = fac.State(symbol, is_initial=True, final_update=final_update, is_unordered_catenation=False)
    states.append(st_0)
    transitions = []
    transitions.append(fac.Transition(st_0, [
        fac.UpdateInstruction(cc_0, True) ]))
    st_0._set_transitionSet(transitions)
    return fac.Automaton(states, counters, True, containing_state=None)
EndpointType._Automaton = _BuildAutomaton_()




EntitiesDescriptorType._AddElement(pyxb.binding.basis.element(pyxb.namespace.ExpandedName(_Namespace_ds, 'Signature'), pyxb.bundles.wssplat.ds.SignatureType, scope=EntitiesDescriptorType, location=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/ds.xsd', 43, 0)))

EntitiesDescriptorType._AddElement(pyxb.binding.basis.element(pyxb.namespace.ExpandedName(Namespace, 'Extensions'), ExtensionsType, scope=EntitiesDescriptorType, location=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/saml20/schemas/metadata.xsd', 51, 4)))

EntitiesDescriptorType._AddElement(pyxb.binding.basis.element(pyxb.namespace.ExpandedName(Namespace, 'EntitiesDescriptor'), EntitiesDescriptorType, scope=EntitiesDescriptorType, location=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/saml20/schemas/metadata.xsd', 77, 4)))

EntitiesDescriptorType._AddElement(pyxb.binding.basis.element(pyxb.namespace.ExpandedName(Namespace, 'EntityDescriptor'), EntityDescriptorType, scope=EntitiesDescriptorType, location=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/saml20/schemas/metadata.xsd', 93, 4)))

def _BuildAutomaton_2 ():
    # Remove this helper function from the namespace after it is invoked
    global _BuildAutomaton_2
    del _BuildAutomaton_2
    import pyxb.utils.fac as fac

    counters = set()
    cc_0 = fac.CounterCondition(min=0, max=1, metadata=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/saml20/schemas/metadata.xsd', 80, 12))
    counters.add(cc_0)
    cc_1 = fac.CounterCondition(min=0, max=1, metadata=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/saml20/schemas/metadata.xsd', 81, 12))
    counters.add(cc_1)
    states = []
    final_update = None
    symbol = pyxb.binding.content.ElementUse(EntitiesDescriptorType._UseForTag(pyxb.namespace.ExpandedName(_Namespace_ds, 'Signature')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/saml20/schemas/metadata.xsd', 80, 12))
    st_0 = fac.State(symbol, is_initial=True, final_update=final_update, is_unordered_catenation=False)
    states.append(st_0)
    final_update = None
    symbol = pyxb.binding.content.ElementUse(EntitiesDescriptorType._UseForTag(pyxb.namespace.ExpandedName(Namespace, 'Extensions')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/saml20/schemas/metadata.xsd', 81, 12))
    st_1 = fac.State(symbol, is_initial=True, final_update=final_update, is_unordered_catenation=False)
    states.append(st_1)
    final_update = set()
    symbol = pyxb.binding.content.ElementUse(EntitiesDescriptorType._UseForTag(pyxb.namespace.ExpandedName(Namespace, 'EntityDescriptor')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/saml20/schemas/metadata.xsd', 83, 16))
    st_2 = fac.State(symbol, is_initial=True, final_update=final_update, is_unordered_catenation=False)
    states.append(st_2)
    final_update = set()
    symbol = pyxb.binding.content.ElementUse(EntitiesDescriptorType._UseForTag(pyxb.namespace.ExpandedName(Namespace, 'EntitiesDescriptor')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/saml20/schemas/metadata.xsd', 84, 16))
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
         ]))
    transitions.append(fac.Transition(st_3, [
         ]))
    st_2._set_transitionSet(transitions)
    transitions = []
    transitions.append(fac.Transition(st_2, [
         ]))
    transitions.append(fac.Transition(st_3, [
         ]))
    st_3._set_transitionSet(transitions)
    return fac.Automaton(states, counters, False, containing_state=None)
EntitiesDescriptorType._Automaton = _BuildAutomaton_2()




OrganizationType._AddElement(pyxb.binding.basis.element(pyxb.namespace.ExpandedName(Namespace, 'Extensions'), ExtensionsType, scope=OrganizationType, location=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/saml20/schemas/metadata.xsd', 51, 4)))

OrganizationType._AddElement(pyxb.binding.basis.element(pyxb.namespace.ExpandedName(Namespace, 'OrganizationName'), localizedNameType, scope=OrganizationType, location=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/saml20/schemas/metadata.xsd', 130, 4)))

OrganizationType._AddElement(pyxb.binding.basis.element(pyxb.namespace.ExpandedName(Namespace, 'OrganizationDisplayName'), localizedNameType, scope=OrganizationType, location=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/saml20/schemas/metadata.xsd', 131, 4)))

OrganizationType._AddElement(pyxb.binding.basis.element(pyxb.namespace.ExpandedName(Namespace, 'OrganizationURL'), localizedURIType, scope=OrganizationType, location=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/saml20/schemas/metadata.xsd', 132, 4)))

def _BuildAutomaton_3 ():
    # Remove this helper function from the namespace after it is invoked
    global _BuildAutomaton_3
    del _BuildAutomaton_3
    import pyxb.utils.fac as fac

    counters = set()
    cc_0 = fac.CounterCondition(min=0, max=1, metadata=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/saml20/schemas/metadata.xsd', 123, 12))
    counters.add(cc_0)
    states = []
    final_update = None
    symbol = pyxb.binding.content.ElementUse(OrganizationType._UseForTag(pyxb.namespace.ExpandedName(Namespace, 'Extensions')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/saml20/schemas/metadata.xsd', 123, 12))
    st_0 = fac.State(symbol, is_initial=True, final_update=final_update, is_unordered_catenation=False)
    states.append(st_0)
    final_update = None
    symbol = pyxb.binding.content.ElementUse(OrganizationType._UseForTag(pyxb.namespace.ExpandedName(Namespace, 'OrganizationName')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/saml20/schemas/metadata.xsd', 124, 12))
    st_1 = fac.State(symbol, is_initial=True, final_update=final_update, is_unordered_catenation=False)
    states.append(st_1)
    final_update = None
    symbol = pyxb.binding.content.ElementUse(OrganizationType._UseForTag(pyxb.namespace.ExpandedName(Namespace, 'OrganizationDisplayName')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/saml20/schemas/metadata.xsd', 125, 12))
    st_2 = fac.State(symbol, is_initial=False, final_update=final_update, is_unordered_catenation=False)
    states.append(st_2)
    final_update = set()
    symbol = pyxb.binding.content.ElementUse(OrganizationType._UseForTag(pyxb.namespace.ExpandedName(Namespace, 'OrganizationURL')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/saml20/schemas/metadata.xsd', 126, 12))
    st_3 = fac.State(symbol, is_initial=False, final_update=final_update, is_unordered_catenation=False)
    states.append(st_3)
    transitions = []
    transitions.append(fac.Transition(st_0, [
        fac.UpdateInstruction(cc_0, True) ]))
    transitions.append(fac.Transition(st_1, [
        fac.UpdateInstruction(cc_0, False) ]))
    st_0._set_transitionSet(transitions)
    transitions = []
    transitions.append(fac.Transition(st_1, [
         ]))
    transitions.append(fac.Transition(st_2, [
         ]))
    st_1._set_transitionSet(transitions)
    transitions = []
    transitions.append(fac.Transition(st_2, [
         ]))
    transitions.append(fac.Transition(st_3, [
         ]))
    st_2._set_transitionSet(transitions)
    transitions = []
    transitions.append(fac.Transition(st_3, [
         ]))
    st_3._set_transitionSet(transitions)
    return fac.Automaton(states, counters, False, containing_state=None)
OrganizationType._Automaton = _BuildAutomaton_3()




AttributeConsumingServiceType._AddElement(pyxb.binding.basis.element(pyxb.namespace.ExpandedName(Namespace, 'ServiceName'), localizedNameType, scope=AttributeConsumingServiceType, location=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/saml20/schemas/metadata.xsd', 267, 4)))

AttributeConsumingServiceType._AddElement(pyxb.binding.basis.element(pyxb.namespace.ExpandedName(Namespace, 'ServiceDescription'), localizedNameType, scope=AttributeConsumingServiceType, location=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/saml20/schemas/metadata.xsd', 268, 4)))

AttributeConsumingServiceType._AddElement(pyxb.binding.basis.element(pyxb.namespace.ExpandedName(Namespace, 'RequestedAttribute'), RequestedAttributeType, scope=AttributeConsumingServiceType, location=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/saml20/schemas/metadata.xsd', 269, 4)))

def _BuildAutomaton_4 ():
    # Remove this helper function from the namespace after it is invoked
    global _BuildAutomaton_4
    del _BuildAutomaton_4
    import pyxb.utils.fac as fac

    counters = set()
    cc_0 = fac.CounterCondition(min=0, max=None, metadata=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/saml20/schemas/metadata.xsd', 261, 12))
    counters.add(cc_0)
    states = []
    final_update = None
    symbol = pyxb.binding.content.ElementUse(AttributeConsumingServiceType._UseForTag(pyxb.namespace.ExpandedName(Namespace, 'ServiceName')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/saml20/schemas/metadata.xsd', 260, 12))
    st_0 = fac.State(symbol, is_initial=True, final_update=final_update, is_unordered_catenation=False)
    states.append(st_0)
    final_update = None
    symbol = pyxb.binding.content.ElementUse(AttributeConsumingServiceType._UseForTag(pyxb.namespace.ExpandedName(Namespace, 'ServiceDescription')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/saml20/schemas/metadata.xsd', 261, 12))
    st_1 = fac.State(symbol, is_initial=False, final_update=final_update, is_unordered_catenation=False)
    states.append(st_1)
    final_update = set()
    symbol = pyxb.binding.content.ElementUse(AttributeConsumingServiceType._UseForTag(pyxb.namespace.ExpandedName(Namespace, 'RequestedAttribute')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/saml20/schemas/metadata.xsd', 262, 12))
    st_2 = fac.State(symbol, is_initial=False, final_update=final_update, is_unordered_catenation=False)
    states.append(st_2)
    transitions = []
    transitions.append(fac.Transition(st_0, [
         ]))
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
         ]))
    st_2._set_transitionSet(transitions)
    return fac.Automaton(states, counters, False, containing_state=None)
AttributeConsumingServiceType._Automaton = _BuildAutomaton_4()




def _BuildAutomaton_5 ():
    # Remove this helper function from the namespace after it is invoked
    global _BuildAutomaton_5
    del _BuildAutomaton_5
    import pyxb.utils.fac as fac

    counters = set()
    cc_0 = fac.CounterCondition(min=0, max=None, metadata=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/saml20/schemas/assertion.xsd', 274, 12))
    counters.add(cc_0)
    states = []
    final_update = set()
    final_update.add(fac.UpdateInstruction(cc_0, False))
    symbol = pyxb.binding.content.ElementUse(RequestedAttributeType._UseForTag(pyxb.namespace.ExpandedName(_Namespace_saml, 'AttributeValue')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/saml20/schemas/assertion.xsd', 274, 12))
    st_0 = fac.State(symbol, is_initial=True, final_update=final_update, is_unordered_catenation=False)
    states.append(st_0)
    transitions = []
    transitions.append(fac.Transition(st_0, [
        fac.UpdateInstruction(cc_0, True) ]))
    st_0._set_transitionSet(transitions)
    return fac.Automaton(states, counters, True, containing_state=None)
RequestedAttributeType._Automaton = _BuildAutomaton_5()




def _BuildAutomaton_6 ():
    # Remove this helper function from the namespace after it is invoked
    global _BuildAutomaton_6
    del _BuildAutomaton_6
    import pyxb.utils.fac as fac

    counters = set()
    cc_0 = fac.CounterCondition(min=0, max=None, metadata=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/saml20/schemas/metadata.xsd', 60, 12))
    counters.add(cc_0)
    states = []
    final_update = set()
    final_update.add(fac.UpdateInstruction(cc_0, False))
    symbol = pyxb.binding.content.WildcardUse(pyxb.binding.content.Wildcard(process_contents=pyxb.binding.content.Wildcard.PC_lax, namespace_constraint=(pyxb.binding.content.Wildcard.NC_not, 'urn:oasis:names:tc:SAML:2.0:metadata')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/saml20/schemas/metadata.xsd', 60, 12))
    st_0 = fac.State(symbol, is_initial=True, final_update=final_update, is_unordered_catenation=False)
    states.append(st_0)
    transitions = []
    transitions.append(fac.Transition(st_0, [
        fac.UpdateInstruction(cc_0, True) ]))
    st_0._set_transitionSet(transitions)
    return fac.Automaton(states, counters, True, containing_state=None)
IndexedEndpointType._Automaton = _BuildAutomaton_6()




EntityDescriptorType._AddElement(pyxb.binding.basis.element(pyxb.namespace.ExpandedName(_Namespace_ds, 'Signature'), pyxb.bundles.wssplat.ds.SignatureType, scope=EntityDescriptorType, location=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/ds.xsd', 43, 0)))

EntityDescriptorType._AddElement(pyxb.binding.basis.element(pyxb.namespace.ExpandedName(Namespace, 'Extensions'), ExtensionsType, scope=EntityDescriptorType, location=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/saml20/schemas/metadata.xsd', 51, 4)))

EntityDescriptorType._AddElement(pyxb.binding.basis.element(pyxb.namespace.ExpandedName(Namespace, 'Organization'), OrganizationType, scope=EntityDescriptorType, location=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/saml20/schemas/metadata.xsd', 120, 4)))

EntityDescriptorType._AddElement(pyxb.binding.basis.element(pyxb.namespace.ExpandedName(Namespace, 'ContactPerson'), ContactType, scope=EntityDescriptorType, location=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/saml20/schemas/metadata.xsd', 133, 4)))

EntityDescriptorType._AddElement(pyxb.binding.basis.element(pyxb.namespace.ExpandedName(Namespace, 'AdditionalMetadataLocation'), AdditionalMetadataLocationType, scope=EntityDescriptorType, location=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/saml20/schemas/metadata.xsd', 161, 4)))

EntityDescriptorType._AddElement(pyxb.binding.basis.element(pyxb.namespace.ExpandedName(Namespace, 'RoleDescriptor'), RoleDescriptorType, scope=EntityDescriptorType, location=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/saml20/schemas/metadata.xsd', 170, 4)))

EntityDescriptorType._AddElement(pyxb.binding.basis.element(pyxb.namespace.ExpandedName(Namespace, 'IDPSSODescriptor'), IDPSSODescriptorType, scope=EntityDescriptorType, location=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/saml20/schemas/metadata.xsd', 223, 4)))

EntityDescriptorType._AddElement(pyxb.binding.basis.element(pyxb.namespace.ExpandedName(Namespace, 'SPSSODescriptor'), SPSSODescriptorType, scope=EntityDescriptorType, location=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/saml20/schemas/metadata.xsd', 243, 4)))

EntityDescriptorType._AddElement(pyxb.binding.basis.element(pyxb.namespace.ExpandedName(Namespace, 'AuthnAuthorityDescriptor'), AuthnAuthorityDescriptorType, scope=EntityDescriptorType, location=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/saml20/schemas/metadata.xsd', 278, 4)))

EntityDescriptorType._AddElement(pyxb.binding.basis.element(pyxb.namespace.ExpandedName(Namespace, 'PDPDescriptor'), PDPDescriptorType, scope=EntityDescriptorType, location=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/saml20/schemas/metadata.xsd', 292, 4)))

EntityDescriptorType._AddElement(pyxb.binding.basis.element(pyxb.namespace.ExpandedName(Namespace, 'AttributeAuthorityDescriptor'), AttributeAuthorityDescriptorType, scope=EntityDescriptorType, location=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/saml20/schemas/metadata.xsd', 306, 4)))

EntityDescriptorType._AddElement(pyxb.binding.basis.element(pyxb.namespace.ExpandedName(Namespace, 'AffiliationDescriptor'), AffiliationDescriptorType, scope=EntityDescriptorType, location=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/saml20/schemas/metadata.xsd', 322, 4)))

def _BuildAutomaton_7 ():
    # Remove this helper function from the namespace after it is invoked
    global _BuildAutomaton_7
    del _BuildAutomaton_7
    import pyxb.utils.fac as fac

    counters = set()
    cc_0 = fac.CounterCondition(min=0, max=1, metadata=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/saml20/schemas/metadata.xsd', 96, 12))
    counters.add(cc_0)
    cc_1 = fac.CounterCondition(min=0, max=1, metadata=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/saml20/schemas/metadata.xsd', 97, 12))
    counters.add(cc_1)
    cc_2 = fac.CounterCondition(min=0, max=1, metadata=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/saml20/schemas/metadata.xsd', 109, 12))
    counters.add(cc_2)
    cc_3 = fac.CounterCondition(min=0, max=None, metadata=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/saml20/schemas/metadata.xsd', 110, 12))
    counters.add(cc_3)
    cc_4 = fac.CounterCondition(min=0, max=None, metadata=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/saml20/schemas/metadata.xsd', 111, 12))
    counters.add(cc_4)
    states = []
    final_update = None
    symbol = pyxb.binding.content.ElementUse(EntityDescriptorType._UseForTag(pyxb.namespace.ExpandedName(_Namespace_ds, 'Signature')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/saml20/schemas/metadata.xsd', 96, 12))
    st_0 = fac.State(symbol, is_initial=True, final_update=final_update, is_unordered_catenation=False)
    states.append(st_0)
    final_update = None
    symbol = pyxb.binding.content.ElementUse(EntityDescriptorType._UseForTag(pyxb.namespace.ExpandedName(Namespace, 'Extensions')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/saml20/schemas/metadata.xsd', 97, 12))
    st_1 = fac.State(symbol, is_initial=True, final_update=final_update, is_unordered_catenation=False)
    states.append(st_1)
    final_update = set()
    symbol = pyxb.binding.content.ElementUse(EntityDescriptorType._UseForTag(pyxb.namespace.ExpandedName(Namespace, 'RoleDescriptor')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/saml20/schemas/metadata.xsd', 100, 20))
    st_2 = fac.State(symbol, is_initial=True, final_update=final_update, is_unordered_catenation=False)
    states.append(st_2)
    final_update = set()
    symbol = pyxb.binding.content.ElementUse(EntityDescriptorType._UseForTag(pyxb.namespace.ExpandedName(Namespace, 'IDPSSODescriptor')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/saml20/schemas/metadata.xsd', 101, 20))
    st_3 = fac.State(symbol, is_initial=True, final_update=final_update, is_unordered_catenation=False)
    states.append(st_3)
    final_update = set()
    symbol = pyxb.binding.content.ElementUse(EntityDescriptorType._UseForTag(pyxb.namespace.ExpandedName(Namespace, 'SPSSODescriptor')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/saml20/schemas/metadata.xsd', 102, 20))
    st_4 = fac.State(symbol, is_initial=True, final_update=final_update, is_unordered_catenation=False)
    states.append(st_4)
    final_update = set()
    symbol = pyxb.binding.content.ElementUse(EntityDescriptorType._UseForTag(pyxb.namespace.ExpandedName(Namespace, 'AuthnAuthorityDescriptor')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/saml20/schemas/metadata.xsd', 103, 20))
    st_5 = fac.State(symbol, is_initial=True, final_update=final_update, is_unordered_catenation=False)
    states.append(st_5)
    final_update = set()
    symbol = pyxb.binding.content.ElementUse(EntityDescriptorType._UseForTag(pyxb.namespace.ExpandedName(Namespace, 'AttributeAuthorityDescriptor')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/saml20/schemas/metadata.xsd', 104, 20))
    st_6 = fac.State(symbol, is_initial=True, final_update=final_update, is_unordered_catenation=False)
    states.append(st_6)
    final_update = set()
    symbol = pyxb.binding.content.ElementUse(EntityDescriptorType._UseForTag(pyxb.namespace.ExpandedName(Namespace, 'PDPDescriptor')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/saml20/schemas/metadata.xsd', 105, 20))
    st_7 = fac.State(symbol, is_initial=True, final_update=final_update, is_unordered_catenation=False)
    states.append(st_7)
    final_update = set()
    symbol = pyxb.binding.content.ElementUse(EntityDescriptorType._UseForTag(pyxb.namespace.ExpandedName(Namespace, 'AffiliationDescriptor')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/saml20/schemas/metadata.xsd', 107, 16))
    st_8 = fac.State(symbol, is_initial=True, final_update=final_update, is_unordered_catenation=False)
    states.append(st_8)
    final_update = set()
    final_update.add(fac.UpdateInstruction(cc_2, False))
    symbol = pyxb.binding.content.ElementUse(EntityDescriptorType._UseForTag(pyxb.namespace.ExpandedName(Namespace, 'Organization')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/saml20/schemas/metadata.xsd', 109, 12))
    st_9 = fac.State(symbol, is_initial=False, final_update=final_update, is_unordered_catenation=False)
    states.append(st_9)
    final_update = set()
    final_update.add(fac.UpdateInstruction(cc_3, False))
    symbol = pyxb.binding.content.ElementUse(EntityDescriptorType._UseForTag(pyxb.namespace.ExpandedName(Namespace, 'ContactPerson')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/saml20/schemas/metadata.xsd', 110, 12))
    st_10 = fac.State(symbol, is_initial=False, final_update=final_update, is_unordered_catenation=False)
    states.append(st_10)
    final_update = set()
    final_update.add(fac.UpdateInstruction(cc_4, False))
    symbol = pyxb.binding.content.ElementUse(EntityDescriptorType._UseForTag(pyxb.namespace.ExpandedName(Namespace, 'AdditionalMetadataLocation')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/saml20/schemas/metadata.xsd', 111, 12))
    st_11 = fac.State(symbol, is_initial=False, final_update=final_update, is_unordered_catenation=False)
    states.append(st_11)
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
    transitions.append(fac.Transition(st_8, [
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
    transitions.append(fac.Transition(st_5, [
        fac.UpdateInstruction(cc_1, False) ]))
    transitions.append(fac.Transition(st_6, [
        fac.UpdateInstruction(cc_1, False) ]))
    transitions.append(fac.Transition(st_7, [
        fac.UpdateInstruction(cc_1, False) ]))
    transitions.append(fac.Transition(st_8, [
        fac.UpdateInstruction(cc_1, False) ]))
    st_1._set_transitionSet(transitions)
    transitions = []
    transitions.append(fac.Transition(st_2, [
         ]))
    transitions.append(fac.Transition(st_3, [
         ]))
    transitions.append(fac.Transition(st_4, [
         ]))
    transitions.append(fac.Transition(st_5, [
         ]))
    transitions.append(fac.Transition(st_6, [
         ]))
    transitions.append(fac.Transition(st_7, [
         ]))
    transitions.append(fac.Transition(st_9, [
         ]))
    transitions.append(fac.Transition(st_10, [
         ]))
    transitions.append(fac.Transition(st_11, [
         ]))
    st_2._set_transitionSet(transitions)
    transitions = []
    transitions.append(fac.Transition(st_2, [
         ]))
    transitions.append(fac.Transition(st_3, [
         ]))
    transitions.append(fac.Transition(st_4, [
         ]))
    transitions.append(fac.Transition(st_5, [
         ]))
    transitions.append(fac.Transition(st_6, [
         ]))
    transitions.append(fac.Transition(st_7, [
         ]))
    transitions.append(fac.Transition(st_9, [
         ]))
    transitions.append(fac.Transition(st_10, [
         ]))
    transitions.append(fac.Transition(st_11, [
         ]))
    st_3._set_transitionSet(transitions)
    transitions = []
    transitions.append(fac.Transition(st_2, [
         ]))
    transitions.append(fac.Transition(st_3, [
         ]))
    transitions.append(fac.Transition(st_4, [
         ]))
    transitions.append(fac.Transition(st_5, [
         ]))
    transitions.append(fac.Transition(st_6, [
         ]))
    transitions.append(fac.Transition(st_7, [
         ]))
    transitions.append(fac.Transition(st_9, [
         ]))
    transitions.append(fac.Transition(st_10, [
         ]))
    transitions.append(fac.Transition(st_11, [
         ]))
    st_4._set_transitionSet(transitions)
    transitions = []
    transitions.append(fac.Transition(st_2, [
         ]))
    transitions.append(fac.Transition(st_3, [
         ]))
    transitions.append(fac.Transition(st_4, [
         ]))
    transitions.append(fac.Transition(st_5, [
         ]))
    transitions.append(fac.Transition(st_6, [
         ]))
    transitions.append(fac.Transition(st_7, [
         ]))
    transitions.append(fac.Transition(st_9, [
         ]))
    transitions.append(fac.Transition(st_10, [
         ]))
    transitions.append(fac.Transition(st_11, [
         ]))
    st_5._set_transitionSet(transitions)
    transitions = []
    transitions.append(fac.Transition(st_2, [
         ]))
    transitions.append(fac.Transition(st_3, [
         ]))
    transitions.append(fac.Transition(st_4, [
         ]))
    transitions.append(fac.Transition(st_5, [
         ]))
    transitions.append(fac.Transition(st_6, [
         ]))
    transitions.append(fac.Transition(st_7, [
         ]))
    transitions.append(fac.Transition(st_9, [
         ]))
    transitions.append(fac.Transition(st_10, [
         ]))
    transitions.append(fac.Transition(st_11, [
         ]))
    st_6._set_transitionSet(transitions)
    transitions = []
    transitions.append(fac.Transition(st_2, [
         ]))
    transitions.append(fac.Transition(st_3, [
         ]))
    transitions.append(fac.Transition(st_4, [
         ]))
    transitions.append(fac.Transition(st_5, [
         ]))
    transitions.append(fac.Transition(st_6, [
         ]))
    transitions.append(fac.Transition(st_7, [
         ]))
    transitions.append(fac.Transition(st_9, [
         ]))
    transitions.append(fac.Transition(st_10, [
         ]))
    transitions.append(fac.Transition(st_11, [
         ]))
    st_7._set_transitionSet(transitions)
    transitions = []
    transitions.append(fac.Transition(st_9, [
         ]))
    transitions.append(fac.Transition(st_10, [
         ]))
    transitions.append(fac.Transition(st_11, [
         ]))
    st_8._set_transitionSet(transitions)
    transitions = []
    transitions.append(fac.Transition(st_9, [
        fac.UpdateInstruction(cc_2, True) ]))
    transitions.append(fac.Transition(st_10, [
        fac.UpdateInstruction(cc_2, False) ]))
    transitions.append(fac.Transition(st_11, [
        fac.UpdateInstruction(cc_2, False) ]))
    st_9._set_transitionSet(transitions)
    transitions = []
    transitions.append(fac.Transition(st_10, [
        fac.UpdateInstruction(cc_3, True) ]))
    transitions.append(fac.Transition(st_11, [
        fac.UpdateInstruction(cc_3, False) ]))
    st_10._set_transitionSet(transitions)
    transitions = []
    transitions.append(fac.Transition(st_11, [
        fac.UpdateInstruction(cc_4, True) ]))
    st_11._set_transitionSet(transitions)
    return fac.Automaton(states, counters, False, containing_state=None)
EntityDescriptorType._Automaton = _BuildAutomaton_7()




ContactType._AddElement(pyxb.binding.basis.element(pyxb.namespace.ExpandedName(Namespace, 'Extensions'), ExtensionsType, scope=ContactType, location=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/saml20/schemas/metadata.xsd', 51, 4)))

ContactType._AddElement(pyxb.binding.basis.element(pyxb.namespace.ExpandedName(Namespace, 'Company'), pyxb.binding.datatypes.string, scope=ContactType, location=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/saml20/schemas/metadata.xsd', 146, 4)))

ContactType._AddElement(pyxb.binding.basis.element(pyxb.namespace.ExpandedName(Namespace, 'GivenName'), pyxb.binding.datatypes.string, scope=ContactType, location=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/saml20/schemas/metadata.xsd', 147, 4)))

ContactType._AddElement(pyxb.binding.basis.element(pyxb.namespace.ExpandedName(Namespace, 'SurName'), pyxb.binding.datatypes.string, scope=ContactType, location=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/saml20/schemas/metadata.xsd', 148, 4)))

ContactType._AddElement(pyxb.binding.basis.element(pyxb.namespace.ExpandedName(Namespace, 'EmailAddress'), pyxb.binding.datatypes.anyURI, scope=ContactType, location=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/saml20/schemas/metadata.xsd', 149, 4)))

ContactType._AddElement(pyxb.binding.basis.element(pyxb.namespace.ExpandedName(Namespace, 'TelephoneNumber'), pyxb.binding.datatypes.string, scope=ContactType, location=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/saml20/schemas/metadata.xsd', 150, 4)))

def _BuildAutomaton_8 ():
    # Remove this helper function from the namespace after it is invoked
    global _BuildAutomaton_8
    del _BuildAutomaton_8
    import pyxb.utils.fac as fac

    counters = set()
    cc_0 = fac.CounterCondition(min=0, max=1, metadata=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/saml20/schemas/metadata.xsd', 136, 12))
    counters.add(cc_0)
    cc_1 = fac.CounterCondition(min=0, max=1, metadata=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/saml20/schemas/metadata.xsd', 137, 12))
    counters.add(cc_1)
    cc_2 = fac.CounterCondition(min=0, max=1, metadata=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/saml20/schemas/metadata.xsd', 138, 12))
    counters.add(cc_2)
    cc_3 = fac.CounterCondition(min=0, max=1, metadata=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/saml20/schemas/metadata.xsd', 139, 12))
    counters.add(cc_3)
    cc_4 = fac.CounterCondition(min=0, max=None, metadata=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/saml20/schemas/metadata.xsd', 140, 12))
    counters.add(cc_4)
    cc_5 = fac.CounterCondition(min=0, max=None, metadata=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/saml20/schemas/metadata.xsd', 141, 12))
    counters.add(cc_5)
    states = []
    final_update = set()
    final_update.add(fac.UpdateInstruction(cc_0, False))
    symbol = pyxb.binding.content.ElementUse(ContactType._UseForTag(pyxb.namespace.ExpandedName(Namespace, 'Extensions')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/saml20/schemas/metadata.xsd', 136, 12))
    st_0 = fac.State(symbol, is_initial=True, final_update=final_update, is_unordered_catenation=False)
    states.append(st_0)
    final_update = set()
    final_update.add(fac.UpdateInstruction(cc_1, False))
    symbol = pyxb.binding.content.ElementUse(ContactType._UseForTag(pyxb.namespace.ExpandedName(Namespace, 'Company')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/saml20/schemas/metadata.xsd', 137, 12))
    st_1 = fac.State(symbol, is_initial=True, final_update=final_update, is_unordered_catenation=False)
    states.append(st_1)
    final_update = set()
    final_update.add(fac.UpdateInstruction(cc_2, False))
    symbol = pyxb.binding.content.ElementUse(ContactType._UseForTag(pyxb.namespace.ExpandedName(Namespace, 'GivenName')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/saml20/schemas/metadata.xsd', 138, 12))
    st_2 = fac.State(symbol, is_initial=True, final_update=final_update, is_unordered_catenation=False)
    states.append(st_2)
    final_update = set()
    final_update.add(fac.UpdateInstruction(cc_3, False))
    symbol = pyxb.binding.content.ElementUse(ContactType._UseForTag(pyxb.namespace.ExpandedName(Namespace, 'SurName')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/saml20/schemas/metadata.xsd', 139, 12))
    st_3 = fac.State(symbol, is_initial=True, final_update=final_update, is_unordered_catenation=False)
    states.append(st_3)
    final_update = set()
    final_update.add(fac.UpdateInstruction(cc_4, False))
    symbol = pyxb.binding.content.ElementUse(ContactType._UseForTag(pyxb.namespace.ExpandedName(Namespace, 'EmailAddress')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/saml20/schemas/metadata.xsd', 140, 12))
    st_4 = fac.State(symbol, is_initial=True, final_update=final_update, is_unordered_catenation=False)
    states.append(st_4)
    final_update = set()
    final_update.add(fac.UpdateInstruction(cc_5, False))
    symbol = pyxb.binding.content.ElementUse(ContactType._UseForTag(pyxb.namespace.ExpandedName(Namespace, 'TelephoneNumber')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/saml20/schemas/metadata.xsd', 141, 12))
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
        fac.UpdateInstruction(cc_1, False) ]))
    transitions.append(fac.Transition(st_3, [
        fac.UpdateInstruction(cc_1, False) ]))
    transitions.append(fac.Transition(st_4, [
        fac.UpdateInstruction(cc_1, False) ]))
    transitions.append(fac.Transition(st_5, [
        fac.UpdateInstruction(cc_1, False) ]))
    st_1._set_transitionSet(transitions)
    transitions = []
    transitions.append(fac.Transition(st_2, [
        fac.UpdateInstruction(cc_2, True) ]))
    transitions.append(fac.Transition(st_3, [
        fac.UpdateInstruction(cc_2, False) ]))
    transitions.append(fac.Transition(st_4, [
        fac.UpdateInstruction(cc_2, False) ]))
    transitions.append(fac.Transition(st_5, [
        fac.UpdateInstruction(cc_2, False) ]))
    st_2._set_transitionSet(transitions)
    transitions = []
    transitions.append(fac.Transition(st_3, [
        fac.UpdateInstruction(cc_3, True) ]))
    transitions.append(fac.Transition(st_4, [
        fac.UpdateInstruction(cc_3, False) ]))
    transitions.append(fac.Transition(st_5, [
        fac.UpdateInstruction(cc_3, False) ]))
    st_3._set_transitionSet(transitions)
    transitions = []
    transitions.append(fac.Transition(st_4, [
        fac.UpdateInstruction(cc_4, True) ]))
    transitions.append(fac.Transition(st_5, [
        fac.UpdateInstruction(cc_4, False) ]))
    st_4._set_transitionSet(transitions)
    transitions = []
    transitions.append(fac.Transition(st_5, [
        fac.UpdateInstruction(cc_5, True) ]))
    st_5._set_transitionSet(transitions)
    return fac.Automaton(states, counters, True, containing_state=None)
ContactType._Automaton = _BuildAutomaton_8()




RoleDescriptorType._AddElement(pyxb.binding.basis.element(pyxb.namespace.ExpandedName(_Namespace_ds, 'Signature'), pyxb.bundles.wssplat.ds.SignatureType, scope=RoleDescriptorType, location=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/ds.xsd', 43, 0)))

RoleDescriptorType._AddElement(pyxb.binding.basis.element(pyxb.namespace.ExpandedName(Namespace, 'Extensions'), ExtensionsType, scope=RoleDescriptorType, location=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/saml20/schemas/metadata.xsd', 51, 4)))

RoleDescriptorType._AddElement(pyxb.binding.basis.element(pyxb.namespace.ExpandedName(Namespace, 'Organization'), OrganizationType, scope=RoleDescriptorType, location=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/saml20/schemas/metadata.xsd', 120, 4)))

RoleDescriptorType._AddElement(pyxb.binding.basis.element(pyxb.namespace.ExpandedName(Namespace, 'ContactPerson'), ContactType, scope=RoleDescriptorType, location=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/saml20/schemas/metadata.xsd', 133, 4)))

RoleDescriptorType._AddElement(pyxb.binding.basis.element(pyxb.namespace.ExpandedName(Namespace, 'KeyDescriptor'), KeyDescriptorType, scope=RoleDescriptorType, location=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/saml20/schemas/metadata.xsd', 190, 4)))

def _BuildAutomaton_9 ():
    # Remove this helper function from the namespace after it is invoked
    global _BuildAutomaton_9
    del _BuildAutomaton_9
    import pyxb.utils.fac as fac

    counters = set()
    cc_0 = fac.CounterCondition(min=0, max=1, metadata=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/saml20/schemas/metadata.xsd', 173, 12))
    counters.add(cc_0)
    cc_1 = fac.CounterCondition(min=0, max=1, metadata=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/saml20/schemas/metadata.xsd', 174, 12))
    counters.add(cc_1)
    cc_2 = fac.CounterCondition(min=0, max=None, metadata=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/saml20/schemas/metadata.xsd', 175, 12))
    counters.add(cc_2)
    cc_3 = fac.CounterCondition(min=0, max=1, metadata=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/saml20/schemas/metadata.xsd', 176, 12))
    counters.add(cc_3)
    cc_4 = fac.CounterCondition(min=0, max=None, metadata=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/saml20/schemas/metadata.xsd', 177, 12))
    counters.add(cc_4)
    states = []
    final_update = set()
    final_update.add(fac.UpdateInstruction(cc_0, False))
    symbol = pyxb.binding.content.ElementUse(RoleDescriptorType._UseForTag(pyxb.namespace.ExpandedName(_Namespace_ds, 'Signature')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/saml20/schemas/metadata.xsd', 173, 12))
    st_0 = fac.State(symbol, is_initial=True, final_update=final_update, is_unordered_catenation=False)
    states.append(st_0)
    final_update = set()
    final_update.add(fac.UpdateInstruction(cc_1, False))
    symbol = pyxb.binding.content.ElementUse(RoleDescriptorType._UseForTag(pyxb.namespace.ExpandedName(Namespace, 'Extensions')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/saml20/schemas/metadata.xsd', 174, 12))
    st_1 = fac.State(symbol, is_initial=True, final_update=final_update, is_unordered_catenation=False)
    states.append(st_1)
    final_update = set()
    final_update.add(fac.UpdateInstruction(cc_2, False))
    symbol = pyxb.binding.content.ElementUse(RoleDescriptorType._UseForTag(pyxb.namespace.ExpandedName(Namespace, 'KeyDescriptor')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/saml20/schemas/metadata.xsd', 175, 12))
    st_2 = fac.State(symbol, is_initial=True, final_update=final_update, is_unordered_catenation=False)
    states.append(st_2)
    final_update = set()
    final_update.add(fac.UpdateInstruction(cc_3, False))
    symbol = pyxb.binding.content.ElementUse(RoleDescriptorType._UseForTag(pyxb.namespace.ExpandedName(Namespace, 'Organization')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/saml20/schemas/metadata.xsd', 176, 12))
    st_3 = fac.State(symbol, is_initial=True, final_update=final_update, is_unordered_catenation=False)
    states.append(st_3)
    final_update = set()
    final_update.add(fac.UpdateInstruction(cc_4, False))
    symbol = pyxb.binding.content.ElementUse(RoleDescriptorType._UseForTag(pyxb.namespace.ExpandedName(Namespace, 'ContactPerson')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/saml20/schemas/metadata.xsd', 177, 12))
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
RoleDescriptorType._Automaton = _BuildAutomaton_9()




KeyDescriptorType._AddElement(pyxb.binding.basis.element(pyxb.namespace.ExpandedName(_Namespace_ds, 'KeyInfo'), pyxb.bundles.wssplat.ds.KeyInfoType, scope=KeyDescriptorType, location=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/ds.xsd', 144, 0)))

KeyDescriptorType._AddElement(pyxb.binding.basis.element(pyxb.namespace.ExpandedName(Namespace, 'EncryptionMethod'), pyxb.bundles.wssplat.xenc.EncryptionMethodType, scope=KeyDescriptorType, location=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/saml20/schemas/metadata.xsd', 204, 4)))

def _BuildAutomaton_10 ():
    # Remove this helper function from the namespace after it is invoked
    global _BuildAutomaton_10
    del _BuildAutomaton_10
    import pyxb.utils.fac as fac

    counters = set()
    cc_0 = fac.CounterCondition(min=0, max=None, metadata=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/saml20/schemas/metadata.xsd', 194, 12))
    counters.add(cc_0)
    states = []
    final_update = set()
    symbol = pyxb.binding.content.ElementUse(KeyDescriptorType._UseForTag(pyxb.namespace.ExpandedName(_Namespace_ds, 'KeyInfo')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/saml20/schemas/metadata.xsd', 193, 12))
    st_0 = fac.State(symbol, is_initial=True, final_update=final_update, is_unordered_catenation=False)
    states.append(st_0)
    final_update = set()
    final_update.add(fac.UpdateInstruction(cc_0, False))
    symbol = pyxb.binding.content.ElementUse(KeyDescriptorType._UseForTag(pyxb.namespace.ExpandedName(Namespace, 'EncryptionMethod')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/saml20/schemas/metadata.xsd', 194, 12))
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
KeyDescriptorType._Automaton = _BuildAutomaton_10()




AffiliationDescriptorType._AddElement(pyxb.binding.basis.element(pyxb.namespace.ExpandedName(_Namespace_ds, 'Signature'), pyxb.bundles.wssplat.ds.SignatureType, scope=AffiliationDescriptorType, location=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/ds.xsd', 43, 0)))

AffiliationDescriptorType._AddElement(pyxb.binding.basis.element(pyxb.namespace.ExpandedName(Namespace, 'Extensions'), ExtensionsType, scope=AffiliationDescriptorType, location=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/saml20/schemas/metadata.xsd', 51, 4)))

AffiliationDescriptorType._AddElement(pyxb.binding.basis.element(pyxb.namespace.ExpandedName(Namespace, 'KeyDescriptor'), KeyDescriptorType, scope=AffiliationDescriptorType, location=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/saml20/schemas/metadata.xsd', 190, 4)))

AffiliationDescriptorType._AddElement(pyxb.binding.basis.element(pyxb.namespace.ExpandedName(Namespace, 'AffiliateMember'), entityIDType, scope=AffiliationDescriptorType, location=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/saml20/schemas/metadata.xsd', 336, 4)))

def _BuildAutomaton_11 ():
    # Remove this helper function from the namespace after it is invoked
    global _BuildAutomaton_11
    del _BuildAutomaton_11
    import pyxb.utils.fac as fac

    counters = set()
    cc_0 = fac.CounterCondition(min=0, max=1, metadata=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/saml20/schemas/metadata.xsd', 325, 12))
    counters.add(cc_0)
    cc_1 = fac.CounterCondition(min=0, max=1, metadata=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/saml20/schemas/metadata.xsd', 326, 12))
    counters.add(cc_1)
    cc_2 = fac.CounterCondition(min=0, max=None, metadata=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/saml20/schemas/metadata.xsd', 328, 12))
    counters.add(cc_2)
    states = []
    final_update = None
    symbol = pyxb.binding.content.ElementUse(AffiliationDescriptorType._UseForTag(pyxb.namespace.ExpandedName(_Namespace_ds, 'Signature')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/saml20/schemas/metadata.xsd', 325, 12))
    st_0 = fac.State(symbol, is_initial=True, final_update=final_update, is_unordered_catenation=False)
    states.append(st_0)
    final_update = None
    symbol = pyxb.binding.content.ElementUse(AffiliationDescriptorType._UseForTag(pyxb.namespace.ExpandedName(Namespace, 'Extensions')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/saml20/schemas/metadata.xsd', 326, 12))
    st_1 = fac.State(symbol, is_initial=True, final_update=final_update, is_unordered_catenation=False)
    states.append(st_1)
    final_update = set()
    symbol = pyxb.binding.content.ElementUse(AffiliationDescriptorType._UseForTag(pyxb.namespace.ExpandedName(Namespace, 'AffiliateMember')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/saml20/schemas/metadata.xsd', 327, 12))
    st_2 = fac.State(symbol, is_initial=True, final_update=final_update, is_unordered_catenation=False)
    states.append(st_2)
    final_update = set()
    final_update.add(fac.UpdateInstruction(cc_2, False))
    symbol = pyxb.binding.content.ElementUse(AffiliationDescriptorType._UseForTag(pyxb.namespace.ExpandedName(Namespace, 'KeyDescriptor')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/saml20/schemas/metadata.xsd', 328, 12))
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
    transitions.append(fac.Transition(st_2, [
         ]))
    transitions.append(fac.Transition(st_3, [
         ]))
    st_2._set_transitionSet(transitions)
    transitions = []
    transitions.append(fac.Transition(st_3, [
        fac.UpdateInstruction(cc_2, True) ]))
    st_3._set_transitionSet(transitions)
    return fac.Automaton(states, counters, False, containing_state=None)
AffiliationDescriptorType._Automaton = _BuildAutomaton_11()




SSODescriptorType._AddElement(pyxb.binding.basis.element(pyxb.namespace.ExpandedName(Namespace, 'ArtifactResolutionService'), IndexedEndpointType, scope=SSODescriptorType, location=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/saml20/schemas/metadata.xsd', 218, 4)))

SSODescriptorType._AddElement(pyxb.binding.basis.element(pyxb.namespace.ExpandedName(Namespace, 'SingleLogoutService'), EndpointType, scope=SSODescriptorType, location=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/saml20/schemas/metadata.xsd', 219, 4)))

SSODescriptorType._AddElement(pyxb.binding.basis.element(pyxb.namespace.ExpandedName(Namespace, 'ManageNameIDService'), EndpointType, scope=SSODescriptorType, location=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/saml20/schemas/metadata.xsd', 220, 4)))

SSODescriptorType._AddElement(pyxb.binding.basis.element(pyxb.namespace.ExpandedName(Namespace, 'NameIDFormat'), pyxb.binding.datatypes.anyURI, scope=SSODescriptorType, location=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/saml20/schemas/metadata.xsd', 221, 4)))

def _BuildAutomaton_12 ():
    # Remove this helper function from the namespace after it is invoked
    global _BuildAutomaton_12
    del _BuildAutomaton_12
    import pyxb.utils.fac as fac

    counters = set()
    cc_0 = fac.CounterCondition(min=0, max=1, metadata=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/saml20/schemas/metadata.xsd', 173, 12))
    counters.add(cc_0)
    cc_1 = fac.CounterCondition(min=0, max=1, metadata=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/saml20/schemas/metadata.xsd', 174, 12))
    counters.add(cc_1)
    cc_2 = fac.CounterCondition(min=0, max=None, metadata=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/saml20/schemas/metadata.xsd', 175, 12))
    counters.add(cc_2)
    cc_3 = fac.CounterCondition(min=0, max=1, metadata=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/saml20/schemas/metadata.xsd', 176, 12))
    counters.add(cc_3)
    cc_4 = fac.CounterCondition(min=0, max=None, metadata=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/saml20/schemas/metadata.xsd', 177, 12))
    counters.add(cc_4)
    cc_5 = fac.CounterCondition(min=0, max=None, metadata=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/saml20/schemas/metadata.xsd', 210, 20))
    counters.add(cc_5)
    cc_6 = fac.CounterCondition(min=0, max=None, metadata=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/saml20/schemas/metadata.xsd', 211, 20))
    counters.add(cc_6)
    cc_7 = fac.CounterCondition(min=0, max=None, metadata=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/saml20/schemas/metadata.xsd', 212, 20))
    counters.add(cc_7)
    cc_8 = fac.CounterCondition(min=0, max=None, metadata=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/saml20/schemas/metadata.xsd', 213, 20))
    counters.add(cc_8)
    states = []
    final_update = set()
    final_update.add(fac.UpdateInstruction(cc_0, False))
    symbol = pyxb.binding.content.ElementUse(SSODescriptorType._UseForTag(pyxb.namespace.ExpandedName(_Namespace_ds, 'Signature')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/saml20/schemas/metadata.xsd', 173, 12))
    st_0 = fac.State(symbol, is_initial=True, final_update=final_update, is_unordered_catenation=False)
    states.append(st_0)
    final_update = set()
    final_update.add(fac.UpdateInstruction(cc_1, False))
    symbol = pyxb.binding.content.ElementUse(SSODescriptorType._UseForTag(pyxb.namespace.ExpandedName(Namespace, 'Extensions')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/saml20/schemas/metadata.xsd', 174, 12))
    st_1 = fac.State(symbol, is_initial=True, final_update=final_update, is_unordered_catenation=False)
    states.append(st_1)
    final_update = set()
    final_update.add(fac.UpdateInstruction(cc_2, False))
    symbol = pyxb.binding.content.ElementUse(SSODescriptorType._UseForTag(pyxb.namespace.ExpandedName(Namespace, 'KeyDescriptor')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/saml20/schemas/metadata.xsd', 175, 12))
    st_2 = fac.State(symbol, is_initial=True, final_update=final_update, is_unordered_catenation=False)
    states.append(st_2)
    final_update = set()
    final_update.add(fac.UpdateInstruction(cc_3, False))
    symbol = pyxb.binding.content.ElementUse(SSODescriptorType._UseForTag(pyxb.namespace.ExpandedName(Namespace, 'Organization')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/saml20/schemas/metadata.xsd', 176, 12))
    st_3 = fac.State(symbol, is_initial=True, final_update=final_update, is_unordered_catenation=False)
    states.append(st_3)
    final_update = set()
    final_update.add(fac.UpdateInstruction(cc_4, False))
    symbol = pyxb.binding.content.ElementUse(SSODescriptorType._UseForTag(pyxb.namespace.ExpandedName(Namespace, 'ContactPerson')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/saml20/schemas/metadata.xsd', 177, 12))
    st_4 = fac.State(symbol, is_initial=True, final_update=final_update, is_unordered_catenation=False)
    states.append(st_4)
    final_update = set()
    final_update.add(fac.UpdateInstruction(cc_5, False))
    symbol = pyxb.binding.content.ElementUse(SSODescriptorType._UseForTag(pyxb.namespace.ExpandedName(Namespace, 'ArtifactResolutionService')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/saml20/schemas/metadata.xsd', 210, 20))
    st_5 = fac.State(symbol, is_initial=True, final_update=final_update, is_unordered_catenation=False)
    states.append(st_5)
    final_update = set()
    final_update.add(fac.UpdateInstruction(cc_6, False))
    symbol = pyxb.binding.content.ElementUse(SSODescriptorType._UseForTag(pyxb.namespace.ExpandedName(Namespace, 'SingleLogoutService')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/saml20/schemas/metadata.xsd', 211, 20))
    st_6 = fac.State(symbol, is_initial=True, final_update=final_update, is_unordered_catenation=False)
    states.append(st_6)
    final_update = set()
    final_update.add(fac.UpdateInstruction(cc_7, False))
    symbol = pyxb.binding.content.ElementUse(SSODescriptorType._UseForTag(pyxb.namespace.ExpandedName(Namespace, 'ManageNameIDService')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/saml20/schemas/metadata.xsd', 212, 20))
    st_7 = fac.State(symbol, is_initial=True, final_update=final_update, is_unordered_catenation=False)
    states.append(st_7)
    final_update = set()
    final_update.add(fac.UpdateInstruction(cc_8, False))
    symbol = pyxb.binding.content.ElementUse(SSODescriptorType._UseForTag(pyxb.namespace.ExpandedName(Namespace, 'NameIDFormat')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/saml20/schemas/metadata.xsd', 213, 20))
    st_8 = fac.State(symbol, is_initial=True, final_update=final_update, is_unordered_catenation=False)
    states.append(st_8)
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
    transitions.append(fac.Transition(st_8, [
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
    transitions.append(fac.Transition(st_5, [
        fac.UpdateInstruction(cc_1, False) ]))
    transitions.append(fac.Transition(st_6, [
        fac.UpdateInstruction(cc_1, False) ]))
    transitions.append(fac.Transition(st_7, [
        fac.UpdateInstruction(cc_1, False) ]))
    transitions.append(fac.Transition(st_8, [
        fac.UpdateInstruction(cc_1, False) ]))
    st_1._set_transitionSet(transitions)
    transitions = []
    transitions.append(fac.Transition(st_2, [
        fac.UpdateInstruction(cc_2, True) ]))
    transitions.append(fac.Transition(st_3, [
        fac.UpdateInstruction(cc_2, False) ]))
    transitions.append(fac.Transition(st_4, [
        fac.UpdateInstruction(cc_2, False) ]))
    transitions.append(fac.Transition(st_5, [
        fac.UpdateInstruction(cc_2, False) ]))
    transitions.append(fac.Transition(st_6, [
        fac.UpdateInstruction(cc_2, False) ]))
    transitions.append(fac.Transition(st_7, [
        fac.UpdateInstruction(cc_2, False) ]))
    transitions.append(fac.Transition(st_8, [
        fac.UpdateInstruction(cc_2, False) ]))
    st_2._set_transitionSet(transitions)
    transitions = []
    transitions.append(fac.Transition(st_3, [
        fac.UpdateInstruction(cc_3, True) ]))
    transitions.append(fac.Transition(st_4, [
        fac.UpdateInstruction(cc_3, False) ]))
    transitions.append(fac.Transition(st_5, [
        fac.UpdateInstruction(cc_3, False) ]))
    transitions.append(fac.Transition(st_6, [
        fac.UpdateInstruction(cc_3, False) ]))
    transitions.append(fac.Transition(st_7, [
        fac.UpdateInstruction(cc_3, False) ]))
    transitions.append(fac.Transition(st_8, [
        fac.UpdateInstruction(cc_3, False) ]))
    st_3._set_transitionSet(transitions)
    transitions = []
    transitions.append(fac.Transition(st_4, [
        fac.UpdateInstruction(cc_4, True) ]))
    transitions.append(fac.Transition(st_5, [
        fac.UpdateInstruction(cc_4, False) ]))
    transitions.append(fac.Transition(st_6, [
        fac.UpdateInstruction(cc_4, False) ]))
    transitions.append(fac.Transition(st_7, [
        fac.UpdateInstruction(cc_4, False) ]))
    transitions.append(fac.Transition(st_8, [
        fac.UpdateInstruction(cc_4, False) ]))
    st_4._set_transitionSet(transitions)
    transitions = []
    transitions.append(fac.Transition(st_5, [
        fac.UpdateInstruction(cc_5, True) ]))
    transitions.append(fac.Transition(st_6, [
        fac.UpdateInstruction(cc_5, False) ]))
    transitions.append(fac.Transition(st_7, [
        fac.UpdateInstruction(cc_5, False) ]))
    transitions.append(fac.Transition(st_8, [
        fac.UpdateInstruction(cc_5, False) ]))
    st_5._set_transitionSet(transitions)
    transitions = []
    transitions.append(fac.Transition(st_6, [
        fac.UpdateInstruction(cc_6, True) ]))
    transitions.append(fac.Transition(st_7, [
        fac.UpdateInstruction(cc_6, False) ]))
    transitions.append(fac.Transition(st_8, [
        fac.UpdateInstruction(cc_6, False) ]))
    st_6._set_transitionSet(transitions)
    transitions = []
    transitions.append(fac.Transition(st_7, [
        fac.UpdateInstruction(cc_7, True) ]))
    transitions.append(fac.Transition(st_8, [
        fac.UpdateInstruction(cc_7, False) ]))
    st_7._set_transitionSet(transitions)
    transitions = []
    transitions.append(fac.Transition(st_8, [
        fac.UpdateInstruction(cc_8, True) ]))
    st_8._set_transitionSet(transitions)
    return fac.Automaton(states, counters, True, containing_state=None)
SSODescriptorType._Automaton = _BuildAutomaton_12()




AuthnAuthorityDescriptorType._AddElement(pyxb.binding.basis.element(pyxb.namespace.ExpandedName(Namespace, 'NameIDFormat'), pyxb.binding.datatypes.anyURI, scope=AuthnAuthorityDescriptorType, location=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/saml20/schemas/metadata.xsd', 221, 4)))

AuthnAuthorityDescriptorType._AddElement(pyxb.binding.basis.element(pyxb.namespace.ExpandedName(Namespace, 'AssertionIDRequestService'), EndpointType, scope=AuthnAuthorityDescriptorType, location=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/saml20/schemas/metadata.xsd', 240, 4)))

AuthnAuthorityDescriptorType._AddElement(pyxb.binding.basis.element(pyxb.namespace.ExpandedName(Namespace, 'AuthnQueryService'), EndpointType, scope=AuthnAuthorityDescriptorType, location=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/saml20/schemas/metadata.xsd', 290, 4)))

def _BuildAutomaton_13 ():
    # Remove this helper function from the namespace after it is invoked
    global _BuildAutomaton_13
    del _BuildAutomaton_13
    import pyxb.utils.fac as fac

    counters = set()
    cc_0 = fac.CounterCondition(min=0, max=1, metadata=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/saml20/schemas/metadata.xsd', 173, 12))
    counters.add(cc_0)
    cc_1 = fac.CounterCondition(min=0, max=1, metadata=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/saml20/schemas/metadata.xsd', 174, 12))
    counters.add(cc_1)
    cc_2 = fac.CounterCondition(min=0, max=None, metadata=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/saml20/schemas/metadata.xsd', 175, 12))
    counters.add(cc_2)
    cc_3 = fac.CounterCondition(min=0, max=1, metadata=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/saml20/schemas/metadata.xsd', 176, 12))
    counters.add(cc_3)
    cc_4 = fac.CounterCondition(min=0, max=None, metadata=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/saml20/schemas/metadata.xsd', 177, 12))
    counters.add(cc_4)
    cc_5 = fac.CounterCondition(min=0, max=None, metadata=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/saml20/schemas/metadata.xsd', 284, 20))
    counters.add(cc_5)
    cc_6 = fac.CounterCondition(min=0, max=None, metadata=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/saml20/schemas/metadata.xsd', 285, 20))
    counters.add(cc_6)
    states = []
    final_update = None
    symbol = pyxb.binding.content.ElementUse(AuthnAuthorityDescriptorType._UseForTag(pyxb.namespace.ExpandedName(_Namespace_ds, 'Signature')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/saml20/schemas/metadata.xsd', 173, 12))
    st_0 = fac.State(symbol, is_initial=True, final_update=final_update, is_unordered_catenation=False)
    states.append(st_0)
    final_update = None
    symbol = pyxb.binding.content.ElementUse(AuthnAuthorityDescriptorType._UseForTag(pyxb.namespace.ExpandedName(Namespace, 'Extensions')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/saml20/schemas/metadata.xsd', 174, 12))
    st_1 = fac.State(symbol, is_initial=True, final_update=final_update, is_unordered_catenation=False)
    states.append(st_1)
    final_update = None
    symbol = pyxb.binding.content.ElementUse(AuthnAuthorityDescriptorType._UseForTag(pyxb.namespace.ExpandedName(Namespace, 'KeyDescriptor')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/saml20/schemas/metadata.xsd', 175, 12))
    st_2 = fac.State(symbol, is_initial=True, final_update=final_update, is_unordered_catenation=False)
    states.append(st_2)
    final_update = None
    symbol = pyxb.binding.content.ElementUse(AuthnAuthorityDescriptorType._UseForTag(pyxb.namespace.ExpandedName(Namespace, 'Organization')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/saml20/schemas/metadata.xsd', 176, 12))
    st_3 = fac.State(symbol, is_initial=True, final_update=final_update, is_unordered_catenation=False)
    states.append(st_3)
    final_update = None
    symbol = pyxb.binding.content.ElementUse(AuthnAuthorityDescriptorType._UseForTag(pyxb.namespace.ExpandedName(Namespace, 'ContactPerson')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/saml20/schemas/metadata.xsd', 177, 12))
    st_4 = fac.State(symbol, is_initial=True, final_update=final_update, is_unordered_catenation=False)
    states.append(st_4)
    final_update = set()
    symbol = pyxb.binding.content.ElementUse(AuthnAuthorityDescriptorType._UseForTag(pyxb.namespace.ExpandedName(Namespace, 'AuthnQueryService')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/saml20/schemas/metadata.xsd', 283, 20))
    st_5 = fac.State(symbol, is_initial=True, final_update=final_update, is_unordered_catenation=False)
    states.append(st_5)
    final_update = set()
    final_update.add(fac.UpdateInstruction(cc_5, False))
    symbol = pyxb.binding.content.ElementUse(AuthnAuthorityDescriptorType._UseForTag(pyxb.namespace.ExpandedName(Namespace, 'AssertionIDRequestService')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/saml20/schemas/metadata.xsd', 284, 20))
    st_6 = fac.State(symbol, is_initial=False, final_update=final_update, is_unordered_catenation=False)
    states.append(st_6)
    final_update = set()
    final_update.add(fac.UpdateInstruction(cc_6, False))
    symbol = pyxb.binding.content.ElementUse(AuthnAuthorityDescriptorType._UseForTag(pyxb.namespace.ExpandedName(Namespace, 'NameIDFormat')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/saml20/schemas/metadata.xsd', 285, 20))
    st_7 = fac.State(symbol, is_initial=False, final_update=final_update, is_unordered_catenation=False)
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
    transitions.append(fac.Transition(st_5, [
        fac.UpdateInstruction(cc_1, False) ]))
    st_1._set_transitionSet(transitions)
    transitions = []
    transitions.append(fac.Transition(st_2, [
        fac.UpdateInstruction(cc_2, True) ]))
    transitions.append(fac.Transition(st_3, [
        fac.UpdateInstruction(cc_2, False) ]))
    transitions.append(fac.Transition(st_4, [
        fac.UpdateInstruction(cc_2, False) ]))
    transitions.append(fac.Transition(st_5, [
        fac.UpdateInstruction(cc_2, False) ]))
    st_2._set_transitionSet(transitions)
    transitions = []
    transitions.append(fac.Transition(st_3, [
        fac.UpdateInstruction(cc_3, True) ]))
    transitions.append(fac.Transition(st_4, [
        fac.UpdateInstruction(cc_3, False) ]))
    transitions.append(fac.Transition(st_5, [
        fac.UpdateInstruction(cc_3, False) ]))
    st_3._set_transitionSet(transitions)
    transitions = []
    transitions.append(fac.Transition(st_4, [
        fac.UpdateInstruction(cc_4, True) ]))
    transitions.append(fac.Transition(st_5, [
        fac.UpdateInstruction(cc_4, False) ]))
    st_4._set_transitionSet(transitions)
    transitions = []
    transitions.append(fac.Transition(st_5, [
         ]))
    transitions.append(fac.Transition(st_6, [
         ]))
    transitions.append(fac.Transition(st_7, [
         ]))
    st_5._set_transitionSet(transitions)
    transitions = []
    transitions.append(fac.Transition(st_6, [
        fac.UpdateInstruction(cc_5, True) ]))
    transitions.append(fac.Transition(st_7, [
        fac.UpdateInstruction(cc_5, False) ]))
    st_6._set_transitionSet(transitions)
    transitions = []
    transitions.append(fac.Transition(st_7, [
        fac.UpdateInstruction(cc_6, True) ]))
    st_7._set_transitionSet(transitions)
    return fac.Automaton(states, counters, False, containing_state=None)
AuthnAuthorityDescriptorType._Automaton = _BuildAutomaton_13()




PDPDescriptorType._AddElement(pyxb.binding.basis.element(pyxb.namespace.ExpandedName(Namespace, 'NameIDFormat'), pyxb.binding.datatypes.anyURI, scope=PDPDescriptorType, location=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/saml20/schemas/metadata.xsd', 221, 4)))

PDPDescriptorType._AddElement(pyxb.binding.basis.element(pyxb.namespace.ExpandedName(Namespace, 'AssertionIDRequestService'), EndpointType, scope=PDPDescriptorType, location=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/saml20/schemas/metadata.xsd', 240, 4)))

PDPDescriptorType._AddElement(pyxb.binding.basis.element(pyxb.namespace.ExpandedName(Namespace, 'AuthzService'), EndpointType, scope=PDPDescriptorType, location=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/saml20/schemas/metadata.xsd', 304, 4)))

def _BuildAutomaton_14 ():
    # Remove this helper function from the namespace after it is invoked
    global _BuildAutomaton_14
    del _BuildAutomaton_14
    import pyxb.utils.fac as fac

    counters = set()
    cc_0 = fac.CounterCondition(min=0, max=1, metadata=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/saml20/schemas/metadata.xsd', 173, 12))
    counters.add(cc_0)
    cc_1 = fac.CounterCondition(min=0, max=1, metadata=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/saml20/schemas/metadata.xsd', 174, 12))
    counters.add(cc_1)
    cc_2 = fac.CounterCondition(min=0, max=None, metadata=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/saml20/schemas/metadata.xsd', 175, 12))
    counters.add(cc_2)
    cc_3 = fac.CounterCondition(min=0, max=1, metadata=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/saml20/schemas/metadata.xsd', 176, 12))
    counters.add(cc_3)
    cc_4 = fac.CounterCondition(min=0, max=None, metadata=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/saml20/schemas/metadata.xsd', 177, 12))
    counters.add(cc_4)
    cc_5 = fac.CounterCondition(min=0, max=None, metadata=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/saml20/schemas/metadata.xsd', 298, 20))
    counters.add(cc_5)
    cc_6 = fac.CounterCondition(min=0, max=None, metadata=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/saml20/schemas/metadata.xsd', 299, 20))
    counters.add(cc_6)
    states = []
    final_update = None
    symbol = pyxb.binding.content.ElementUse(PDPDescriptorType._UseForTag(pyxb.namespace.ExpandedName(_Namespace_ds, 'Signature')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/saml20/schemas/metadata.xsd', 173, 12))
    st_0 = fac.State(symbol, is_initial=True, final_update=final_update, is_unordered_catenation=False)
    states.append(st_0)
    final_update = None
    symbol = pyxb.binding.content.ElementUse(PDPDescriptorType._UseForTag(pyxb.namespace.ExpandedName(Namespace, 'Extensions')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/saml20/schemas/metadata.xsd', 174, 12))
    st_1 = fac.State(symbol, is_initial=True, final_update=final_update, is_unordered_catenation=False)
    states.append(st_1)
    final_update = None
    symbol = pyxb.binding.content.ElementUse(PDPDescriptorType._UseForTag(pyxb.namespace.ExpandedName(Namespace, 'KeyDescriptor')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/saml20/schemas/metadata.xsd', 175, 12))
    st_2 = fac.State(symbol, is_initial=True, final_update=final_update, is_unordered_catenation=False)
    states.append(st_2)
    final_update = None
    symbol = pyxb.binding.content.ElementUse(PDPDescriptorType._UseForTag(pyxb.namespace.ExpandedName(Namespace, 'Organization')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/saml20/schemas/metadata.xsd', 176, 12))
    st_3 = fac.State(symbol, is_initial=True, final_update=final_update, is_unordered_catenation=False)
    states.append(st_3)
    final_update = None
    symbol = pyxb.binding.content.ElementUse(PDPDescriptorType._UseForTag(pyxb.namespace.ExpandedName(Namespace, 'ContactPerson')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/saml20/schemas/metadata.xsd', 177, 12))
    st_4 = fac.State(symbol, is_initial=True, final_update=final_update, is_unordered_catenation=False)
    states.append(st_4)
    final_update = set()
    symbol = pyxb.binding.content.ElementUse(PDPDescriptorType._UseForTag(pyxb.namespace.ExpandedName(Namespace, 'AuthzService')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/saml20/schemas/metadata.xsd', 297, 20))
    st_5 = fac.State(symbol, is_initial=True, final_update=final_update, is_unordered_catenation=False)
    states.append(st_5)
    final_update = set()
    final_update.add(fac.UpdateInstruction(cc_5, False))
    symbol = pyxb.binding.content.ElementUse(PDPDescriptorType._UseForTag(pyxb.namespace.ExpandedName(Namespace, 'AssertionIDRequestService')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/saml20/schemas/metadata.xsd', 298, 20))
    st_6 = fac.State(symbol, is_initial=False, final_update=final_update, is_unordered_catenation=False)
    states.append(st_6)
    final_update = set()
    final_update.add(fac.UpdateInstruction(cc_6, False))
    symbol = pyxb.binding.content.ElementUse(PDPDescriptorType._UseForTag(pyxb.namespace.ExpandedName(Namespace, 'NameIDFormat')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/saml20/schemas/metadata.xsd', 299, 20))
    st_7 = fac.State(symbol, is_initial=False, final_update=final_update, is_unordered_catenation=False)
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
    transitions.append(fac.Transition(st_5, [
        fac.UpdateInstruction(cc_1, False) ]))
    st_1._set_transitionSet(transitions)
    transitions = []
    transitions.append(fac.Transition(st_2, [
        fac.UpdateInstruction(cc_2, True) ]))
    transitions.append(fac.Transition(st_3, [
        fac.UpdateInstruction(cc_2, False) ]))
    transitions.append(fac.Transition(st_4, [
        fac.UpdateInstruction(cc_2, False) ]))
    transitions.append(fac.Transition(st_5, [
        fac.UpdateInstruction(cc_2, False) ]))
    st_2._set_transitionSet(transitions)
    transitions = []
    transitions.append(fac.Transition(st_3, [
        fac.UpdateInstruction(cc_3, True) ]))
    transitions.append(fac.Transition(st_4, [
        fac.UpdateInstruction(cc_3, False) ]))
    transitions.append(fac.Transition(st_5, [
        fac.UpdateInstruction(cc_3, False) ]))
    st_3._set_transitionSet(transitions)
    transitions = []
    transitions.append(fac.Transition(st_4, [
        fac.UpdateInstruction(cc_4, True) ]))
    transitions.append(fac.Transition(st_5, [
        fac.UpdateInstruction(cc_4, False) ]))
    st_4._set_transitionSet(transitions)
    transitions = []
    transitions.append(fac.Transition(st_5, [
         ]))
    transitions.append(fac.Transition(st_6, [
         ]))
    transitions.append(fac.Transition(st_7, [
         ]))
    st_5._set_transitionSet(transitions)
    transitions = []
    transitions.append(fac.Transition(st_6, [
        fac.UpdateInstruction(cc_5, True) ]))
    transitions.append(fac.Transition(st_7, [
        fac.UpdateInstruction(cc_5, False) ]))
    st_6._set_transitionSet(transitions)
    transitions = []
    transitions.append(fac.Transition(st_7, [
        fac.UpdateInstruction(cc_6, True) ]))
    st_7._set_transitionSet(transitions)
    return fac.Automaton(states, counters, False, containing_state=None)
PDPDescriptorType._Automaton = _BuildAutomaton_14()




AttributeAuthorityDescriptorType._AddElement(pyxb.binding.basis.element(pyxb.namespace.ExpandedName(_Namespace_saml, 'Attribute'), pyxb.bundles.saml20.assertion.AttributeType, scope=AttributeAuthorityDescriptorType, location=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/saml20/schemas/assertion.xsd', 271, 4)))

AttributeAuthorityDescriptorType._AddElement(pyxb.binding.basis.element(pyxb.namespace.ExpandedName(Namespace, 'NameIDFormat'), pyxb.binding.datatypes.anyURI, scope=AttributeAuthorityDescriptorType, location=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/saml20/schemas/metadata.xsd', 221, 4)))

AttributeAuthorityDescriptorType._AddElement(pyxb.binding.basis.element(pyxb.namespace.ExpandedName(Namespace, 'AssertionIDRequestService'), EndpointType, scope=AttributeAuthorityDescriptorType, location=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/saml20/schemas/metadata.xsd', 240, 4)))

AttributeAuthorityDescriptorType._AddElement(pyxb.binding.basis.element(pyxb.namespace.ExpandedName(Namespace, 'AttributeProfile'), pyxb.binding.datatypes.anyURI, scope=AttributeAuthorityDescriptorType, location=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/saml20/schemas/metadata.xsd', 241, 4)))

AttributeAuthorityDescriptorType._AddElement(pyxb.binding.basis.element(pyxb.namespace.ExpandedName(Namespace, 'AttributeService'), EndpointType, scope=AttributeAuthorityDescriptorType, location=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/saml20/schemas/metadata.xsd', 320, 4)))

def _BuildAutomaton_15 ():
    # Remove this helper function from the namespace after it is invoked
    global _BuildAutomaton_15
    del _BuildAutomaton_15
    import pyxb.utils.fac as fac

    counters = set()
    cc_0 = fac.CounterCondition(min=0, max=1, metadata=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/saml20/schemas/metadata.xsd', 173, 12))
    counters.add(cc_0)
    cc_1 = fac.CounterCondition(min=0, max=1, metadata=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/saml20/schemas/metadata.xsd', 174, 12))
    counters.add(cc_1)
    cc_2 = fac.CounterCondition(min=0, max=None, metadata=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/saml20/schemas/metadata.xsd', 175, 12))
    counters.add(cc_2)
    cc_3 = fac.CounterCondition(min=0, max=1, metadata=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/saml20/schemas/metadata.xsd', 176, 12))
    counters.add(cc_3)
    cc_4 = fac.CounterCondition(min=0, max=None, metadata=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/saml20/schemas/metadata.xsd', 177, 12))
    counters.add(cc_4)
    cc_5 = fac.CounterCondition(min=0, max=None, metadata=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/saml20/schemas/metadata.xsd', 312, 20))
    counters.add(cc_5)
    cc_6 = fac.CounterCondition(min=0, max=None, metadata=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/saml20/schemas/metadata.xsd', 313, 20))
    counters.add(cc_6)
    cc_7 = fac.CounterCondition(min=0, max=None, metadata=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/saml20/schemas/metadata.xsd', 314, 20))
    counters.add(cc_7)
    cc_8 = fac.CounterCondition(min=0, max=None, metadata=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/saml20/schemas/metadata.xsd', 315, 20))
    counters.add(cc_8)
    states = []
    final_update = None
    symbol = pyxb.binding.content.ElementUse(AttributeAuthorityDescriptorType._UseForTag(pyxb.namespace.ExpandedName(_Namespace_ds, 'Signature')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/saml20/schemas/metadata.xsd', 173, 12))
    st_0 = fac.State(symbol, is_initial=True, final_update=final_update, is_unordered_catenation=False)
    states.append(st_0)
    final_update = None
    symbol = pyxb.binding.content.ElementUse(AttributeAuthorityDescriptorType._UseForTag(pyxb.namespace.ExpandedName(Namespace, 'Extensions')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/saml20/schemas/metadata.xsd', 174, 12))
    st_1 = fac.State(symbol, is_initial=True, final_update=final_update, is_unordered_catenation=False)
    states.append(st_1)
    final_update = None
    symbol = pyxb.binding.content.ElementUse(AttributeAuthorityDescriptorType._UseForTag(pyxb.namespace.ExpandedName(Namespace, 'KeyDescriptor')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/saml20/schemas/metadata.xsd', 175, 12))
    st_2 = fac.State(symbol, is_initial=True, final_update=final_update, is_unordered_catenation=False)
    states.append(st_2)
    final_update = None
    symbol = pyxb.binding.content.ElementUse(AttributeAuthorityDescriptorType._UseForTag(pyxb.namespace.ExpandedName(Namespace, 'Organization')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/saml20/schemas/metadata.xsd', 176, 12))
    st_3 = fac.State(symbol, is_initial=True, final_update=final_update, is_unordered_catenation=False)
    states.append(st_3)
    final_update = None
    symbol = pyxb.binding.content.ElementUse(AttributeAuthorityDescriptorType._UseForTag(pyxb.namespace.ExpandedName(Namespace, 'ContactPerson')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/saml20/schemas/metadata.xsd', 177, 12))
    st_4 = fac.State(symbol, is_initial=True, final_update=final_update, is_unordered_catenation=False)
    states.append(st_4)
    final_update = set()
    symbol = pyxb.binding.content.ElementUse(AttributeAuthorityDescriptorType._UseForTag(pyxb.namespace.ExpandedName(Namespace, 'AttributeService')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/saml20/schemas/metadata.xsd', 311, 20))
    st_5 = fac.State(symbol, is_initial=True, final_update=final_update, is_unordered_catenation=False)
    states.append(st_5)
    final_update = set()
    final_update.add(fac.UpdateInstruction(cc_5, False))
    symbol = pyxb.binding.content.ElementUse(AttributeAuthorityDescriptorType._UseForTag(pyxb.namespace.ExpandedName(Namespace, 'AssertionIDRequestService')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/saml20/schemas/metadata.xsd', 312, 20))
    st_6 = fac.State(symbol, is_initial=False, final_update=final_update, is_unordered_catenation=False)
    states.append(st_6)
    final_update = set()
    final_update.add(fac.UpdateInstruction(cc_6, False))
    symbol = pyxb.binding.content.ElementUse(AttributeAuthorityDescriptorType._UseForTag(pyxb.namespace.ExpandedName(Namespace, 'NameIDFormat')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/saml20/schemas/metadata.xsd', 313, 20))
    st_7 = fac.State(symbol, is_initial=False, final_update=final_update, is_unordered_catenation=False)
    states.append(st_7)
    final_update = set()
    final_update.add(fac.UpdateInstruction(cc_7, False))
    symbol = pyxb.binding.content.ElementUse(AttributeAuthorityDescriptorType._UseForTag(pyxb.namespace.ExpandedName(Namespace, 'AttributeProfile')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/saml20/schemas/metadata.xsd', 314, 20))
    st_8 = fac.State(symbol, is_initial=False, final_update=final_update, is_unordered_catenation=False)
    states.append(st_8)
    final_update = set()
    final_update.add(fac.UpdateInstruction(cc_8, False))
    symbol = pyxb.binding.content.ElementUse(AttributeAuthorityDescriptorType._UseForTag(pyxb.namespace.ExpandedName(_Namespace_saml, 'Attribute')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/saml20/schemas/metadata.xsd', 315, 20))
    st_9 = fac.State(symbol, is_initial=False, final_update=final_update, is_unordered_catenation=False)
    states.append(st_9)
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
        fac.UpdateInstruction(cc_1, False) ]))
    transitions.append(fac.Transition(st_3, [
        fac.UpdateInstruction(cc_1, False) ]))
    transitions.append(fac.Transition(st_4, [
        fac.UpdateInstruction(cc_1, False) ]))
    transitions.append(fac.Transition(st_5, [
        fac.UpdateInstruction(cc_1, False) ]))
    st_1._set_transitionSet(transitions)
    transitions = []
    transitions.append(fac.Transition(st_2, [
        fac.UpdateInstruction(cc_2, True) ]))
    transitions.append(fac.Transition(st_3, [
        fac.UpdateInstruction(cc_2, False) ]))
    transitions.append(fac.Transition(st_4, [
        fac.UpdateInstruction(cc_2, False) ]))
    transitions.append(fac.Transition(st_5, [
        fac.UpdateInstruction(cc_2, False) ]))
    st_2._set_transitionSet(transitions)
    transitions = []
    transitions.append(fac.Transition(st_3, [
        fac.UpdateInstruction(cc_3, True) ]))
    transitions.append(fac.Transition(st_4, [
        fac.UpdateInstruction(cc_3, False) ]))
    transitions.append(fac.Transition(st_5, [
        fac.UpdateInstruction(cc_3, False) ]))
    st_3._set_transitionSet(transitions)
    transitions = []
    transitions.append(fac.Transition(st_4, [
        fac.UpdateInstruction(cc_4, True) ]))
    transitions.append(fac.Transition(st_5, [
        fac.UpdateInstruction(cc_4, False) ]))
    st_4._set_transitionSet(transitions)
    transitions = []
    transitions.append(fac.Transition(st_5, [
         ]))
    transitions.append(fac.Transition(st_6, [
         ]))
    transitions.append(fac.Transition(st_7, [
         ]))
    transitions.append(fac.Transition(st_8, [
         ]))
    transitions.append(fac.Transition(st_9, [
         ]))
    st_5._set_transitionSet(transitions)
    transitions = []
    transitions.append(fac.Transition(st_6, [
        fac.UpdateInstruction(cc_5, True) ]))
    transitions.append(fac.Transition(st_7, [
        fac.UpdateInstruction(cc_5, False) ]))
    transitions.append(fac.Transition(st_8, [
        fac.UpdateInstruction(cc_5, False) ]))
    transitions.append(fac.Transition(st_9, [
        fac.UpdateInstruction(cc_5, False) ]))
    st_6._set_transitionSet(transitions)
    transitions = []
    transitions.append(fac.Transition(st_7, [
        fac.UpdateInstruction(cc_6, True) ]))
    transitions.append(fac.Transition(st_8, [
        fac.UpdateInstruction(cc_6, False) ]))
    transitions.append(fac.Transition(st_9, [
        fac.UpdateInstruction(cc_6, False) ]))
    st_7._set_transitionSet(transitions)
    transitions = []
    transitions.append(fac.Transition(st_8, [
        fac.UpdateInstruction(cc_7, True) ]))
    transitions.append(fac.Transition(st_9, [
        fac.UpdateInstruction(cc_7, False) ]))
    st_8._set_transitionSet(transitions)
    transitions = []
    transitions.append(fac.Transition(st_9, [
        fac.UpdateInstruction(cc_8, True) ]))
    st_9._set_transitionSet(transitions)
    return fac.Automaton(states, counters, False, containing_state=None)
AttributeAuthorityDescriptorType._Automaton = _BuildAutomaton_15()




IDPSSODescriptorType._AddElement(pyxb.binding.basis.element(pyxb.namespace.ExpandedName(_Namespace_saml, 'Attribute'), pyxb.bundles.saml20.assertion.AttributeType, scope=IDPSSODescriptorType, location=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/saml20/schemas/assertion.xsd', 271, 4)))

IDPSSODescriptorType._AddElement(pyxb.binding.basis.element(pyxb.namespace.ExpandedName(Namespace, 'SingleSignOnService'), EndpointType, scope=IDPSSODescriptorType, location=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/saml20/schemas/metadata.xsd', 238, 4)))

IDPSSODescriptorType._AddElement(pyxb.binding.basis.element(pyxb.namespace.ExpandedName(Namespace, 'NameIDMappingService'), EndpointType, scope=IDPSSODescriptorType, location=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/saml20/schemas/metadata.xsd', 239, 4)))

IDPSSODescriptorType._AddElement(pyxb.binding.basis.element(pyxb.namespace.ExpandedName(Namespace, 'AssertionIDRequestService'), EndpointType, scope=IDPSSODescriptorType, location=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/saml20/schemas/metadata.xsd', 240, 4)))

IDPSSODescriptorType._AddElement(pyxb.binding.basis.element(pyxb.namespace.ExpandedName(Namespace, 'AttributeProfile'), pyxb.binding.datatypes.anyURI, scope=IDPSSODescriptorType, location=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/saml20/schemas/metadata.xsd', 241, 4)))

def _BuildAutomaton_16 ():
    # Remove this helper function from the namespace after it is invoked
    global _BuildAutomaton_16
    del _BuildAutomaton_16
    import pyxb.utils.fac as fac

    counters = set()
    cc_0 = fac.CounterCondition(min=0, max=1, metadata=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/saml20/schemas/metadata.xsd', 173, 12))
    counters.add(cc_0)
    cc_1 = fac.CounterCondition(min=0, max=1, metadata=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/saml20/schemas/metadata.xsd', 174, 12))
    counters.add(cc_1)
    cc_2 = fac.CounterCondition(min=0, max=None, metadata=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/saml20/schemas/metadata.xsd', 175, 12))
    counters.add(cc_2)
    cc_3 = fac.CounterCondition(min=0, max=1, metadata=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/saml20/schemas/metadata.xsd', 176, 12))
    counters.add(cc_3)
    cc_4 = fac.CounterCondition(min=0, max=None, metadata=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/saml20/schemas/metadata.xsd', 177, 12))
    counters.add(cc_4)
    cc_5 = fac.CounterCondition(min=0, max=None, metadata=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/saml20/schemas/metadata.xsd', 210, 20))
    counters.add(cc_5)
    cc_6 = fac.CounterCondition(min=0, max=None, metadata=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/saml20/schemas/metadata.xsd', 211, 20))
    counters.add(cc_6)
    cc_7 = fac.CounterCondition(min=0, max=None, metadata=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/saml20/schemas/metadata.xsd', 212, 20))
    counters.add(cc_7)
    cc_8 = fac.CounterCondition(min=0, max=None, metadata=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/saml20/schemas/metadata.xsd', 213, 20))
    counters.add(cc_8)
    cc_9 = fac.CounterCondition(min=0, max=None, metadata=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/saml20/schemas/metadata.xsd', 229, 20))
    counters.add(cc_9)
    cc_10 = fac.CounterCondition(min=0, max=None, metadata=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/saml20/schemas/metadata.xsd', 230, 20))
    counters.add(cc_10)
    cc_11 = fac.CounterCondition(min=0, max=None, metadata=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/saml20/schemas/metadata.xsd', 231, 20))
    counters.add(cc_11)
    cc_12 = fac.CounterCondition(min=0, max=None, metadata=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/saml20/schemas/metadata.xsd', 232, 20))
    counters.add(cc_12)
    states = []
    final_update = None
    symbol = pyxb.binding.content.ElementUse(IDPSSODescriptorType._UseForTag(pyxb.namespace.ExpandedName(_Namespace_ds, 'Signature')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/saml20/schemas/metadata.xsd', 173, 12))
    st_0 = fac.State(symbol, is_initial=True, final_update=final_update, is_unordered_catenation=False)
    states.append(st_0)
    final_update = None
    symbol = pyxb.binding.content.ElementUse(IDPSSODescriptorType._UseForTag(pyxb.namespace.ExpandedName(Namespace, 'Extensions')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/saml20/schemas/metadata.xsd', 174, 12))
    st_1 = fac.State(symbol, is_initial=True, final_update=final_update, is_unordered_catenation=False)
    states.append(st_1)
    final_update = None
    symbol = pyxb.binding.content.ElementUse(IDPSSODescriptorType._UseForTag(pyxb.namespace.ExpandedName(Namespace, 'KeyDescriptor')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/saml20/schemas/metadata.xsd', 175, 12))
    st_2 = fac.State(symbol, is_initial=True, final_update=final_update, is_unordered_catenation=False)
    states.append(st_2)
    final_update = None
    symbol = pyxb.binding.content.ElementUse(IDPSSODescriptorType._UseForTag(pyxb.namespace.ExpandedName(Namespace, 'Organization')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/saml20/schemas/metadata.xsd', 176, 12))
    st_3 = fac.State(symbol, is_initial=True, final_update=final_update, is_unordered_catenation=False)
    states.append(st_3)
    final_update = None
    symbol = pyxb.binding.content.ElementUse(IDPSSODescriptorType._UseForTag(pyxb.namespace.ExpandedName(Namespace, 'ContactPerson')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/saml20/schemas/metadata.xsd', 177, 12))
    st_4 = fac.State(symbol, is_initial=True, final_update=final_update, is_unordered_catenation=False)
    states.append(st_4)
    final_update = None
    symbol = pyxb.binding.content.ElementUse(IDPSSODescriptorType._UseForTag(pyxb.namespace.ExpandedName(Namespace, 'ArtifactResolutionService')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/saml20/schemas/metadata.xsd', 210, 20))
    st_5 = fac.State(symbol, is_initial=True, final_update=final_update, is_unordered_catenation=False)
    states.append(st_5)
    final_update = None
    symbol = pyxb.binding.content.ElementUse(IDPSSODescriptorType._UseForTag(pyxb.namespace.ExpandedName(Namespace, 'SingleLogoutService')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/saml20/schemas/metadata.xsd', 211, 20))
    st_6 = fac.State(symbol, is_initial=True, final_update=final_update, is_unordered_catenation=False)
    states.append(st_6)
    final_update = None
    symbol = pyxb.binding.content.ElementUse(IDPSSODescriptorType._UseForTag(pyxb.namespace.ExpandedName(Namespace, 'ManageNameIDService')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/saml20/schemas/metadata.xsd', 212, 20))
    st_7 = fac.State(symbol, is_initial=True, final_update=final_update, is_unordered_catenation=False)
    states.append(st_7)
    final_update = None
    symbol = pyxb.binding.content.ElementUse(IDPSSODescriptorType._UseForTag(pyxb.namespace.ExpandedName(Namespace, 'NameIDFormat')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/saml20/schemas/metadata.xsd', 213, 20))
    st_8 = fac.State(symbol, is_initial=True, final_update=final_update, is_unordered_catenation=False)
    states.append(st_8)
    final_update = set()
    symbol = pyxb.binding.content.ElementUse(IDPSSODescriptorType._UseForTag(pyxb.namespace.ExpandedName(Namespace, 'SingleSignOnService')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/saml20/schemas/metadata.xsd', 228, 20))
    st_9 = fac.State(symbol, is_initial=True, final_update=final_update, is_unordered_catenation=False)
    states.append(st_9)
    final_update = set()
    final_update.add(fac.UpdateInstruction(cc_9, False))
    symbol = pyxb.binding.content.ElementUse(IDPSSODescriptorType._UseForTag(pyxb.namespace.ExpandedName(Namespace, 'NameIDMappingService')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/saml20/schemas/metadata.xsd', 229, 20))
    st_10 = fac.State(symbol, is_initial=False, final_update=final_update, is_unordered_catenation=False)
    states.append(st_10)
    final_update = set()
    final_update.add(fac.UpdateInstruction(cc_10, False))
    symbol = pyxb.binding.content.ElementUse(IDPSSODescriptorType._UseForTag(pyxb.namespace.ExpandedName(Namespace, 'AssertionIDRequestService')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/saml20/schemas/metadata.xsd', 230, 20))
    st_11 = fac.State(symbol, is_initial=False, final_update=final_update, is_unordered_catenation=False)
    states.append(st_11)
    final_update = set()
    final_update.add(fac.UpdateInstruction(cc_11, False))
    symbol = pyxb.binding.content.ElementUse(IDPSSODescriptorType._UseForTag(pyxb.namespace.ExpandedName(Namespace, 'AttributeProfile')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/saml20/schemas/metadata.xsd', 231, 20))
    st_12 = fac.State(symbol, is_initial=False, final_update=final_update, is_unordered_catenation=False)
    states.append(st_12)
    final_update = set()
    final_update.add(fac.UpdateInstruction(cc_12, False))
    symbol = pyxb.binding.content.ElementUse(IDPSSODescriptorType._UseForTag(pyxb.namespace.ExpandedName(_Namespace_saml, 'Attribute')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/saml20/schemas/metadata.xsd', 232, 20))
    st_13 = fac.State(symbol, is_initial=False, final_update=final_update, is_unordered_catenation=False)
    states.append(st_13)
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
    transitions.append(fac.Transition(st_8, [
        fac.UpdateInstruction(cc_0, False) ]))
    transitions.append(fac.Transition(st_9, [
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
    transitions.append(fac.Transition(st_5, [
        fac.UpdateInstruction(cc_1, False) ]))
    transitions.append(fac.Transition(st_6, [
        fac.UpdateInstruction(cc_1, False) ]))
    transitions.append(fac.Transition(st_7, [
        fac.UpdateInstruction(cc_1, False) ]))
    transitions.append(fac.Transition(st_8, [
        fac.UpdateInstruction(cc_1, False) ]))
    transitions.append(fac.Transition(st_9, [
        fac.UpdateInstruction(cc_1, False) ]))
    st_1._set_transitionSet(transitions)
    transitions = []
    transitions.append(fac.Transition(st_2, [
        fac.UpdateInstruction(cc_2, True) ]))
    transitions.append(fac.Transition(st_3, [
        fac.UpdateInstruction(cc_2, False) ]))
    transitions.append(fac.Transition(st_4, [
        fac.UpdateInstruction(cc_2, False) ]))
    transitions.append(fac.Transition(st_5, [
        fac.UpdateInstruction(cc_2, False) ]))
    transitions.append(fac.Transition(st_6, [
        fac.UpdateInstruction(cc_2, False) ]))
    transitions.append(fac.Transition(st_7, [
        fac.UpdateInstruction(cc_2, False) ]))
    transitions.append(fac.Transition(st_8, [
        fac.UpdateInstruction(cc_2, False) ]))
    transitions.append(fac.Transition(st_9, [
        fac.UpdateInstruction(cc_2, False) ]))
    st_2._set_transitionSet(transitions)
    transitions = []
    transitions.append(fac.Transition(st_3, [
        fac.UpdateInstruction(cc_3, True) ]))
    transitions.append(fac.Transition(st_4, [
        fac.UpdateInstruction(cc_3, False) ]))
    transitions.append(fac.Transition(st_5, [
        fac.UpdateInstruction(cc_3, False) ]))
    transitions.append(fac.Transition(st_6, [
        fac.UpdateInstruction(cc_3, False) ]))
    transitions.append(fac.Transition(st_7, [
        fac.UpdateInstruction(cc_3, False) ]))
    transitions.append(fac.Transition(st_8, [
        fac.UpdateInstruction(cc_3, False) ]))
    transitions.append(fac.Transition(st_9, [
        fac.UpdateInstruction(cc_3, False) ]))
    st_3._set_transitionSet(transitions)
    transitions = []
    transitions.append(fac.Transition(st_4, [
        fac.UpdateInstruction(cc_4, True) ]))
    transitions.append(fac.Transition(st_5, [
        fac.UpdateInstruction(cc_4, False) ]))
    transitions.append(fac.Transition(st_6, [
        fac.UpdateInstruction(cc_4, False) ]))
    transitions.append(fac.Transition(st_7, [
        fac.UpdateInstruction(cc_4, False) ]))
    transitions.append(fac.Transition(st_8, [
        fac.UpdateInstruction(cc_4, False) ]))
    transitions.append(fac.Transition(st_9, [
        fac.UpdateInstruction(cc_4, False) ]))
    st_4._set_transitionSet(transitions)
    transitions = []
    transitions.append(fac.Transition(st_5, [
        fac.UpdateInstruction(cc_5, True) ]))
    transitions.append(fac.Transition(st_6, [
        fac.UpdateInstruction(cc_5, False) ]))
    transitions.append(fac.Transition(st_7, [
        fac.UpdateInstruction(cc_5, False) ]))
    transitions.append(fac.Transition(st_8, [
        fac.UpdateInstruction(cc_5, False) ]))
    transitions.append(fac.Transition(st_9, [
        fac.UpdateInstruction(cc_5, False) ]))
    st_5._set_transitionSet(transitions)
    transitions = []
    transitions.append(fac.Transition(st_6, [
        fac.UpdateInstruction(cc_6, True) ]))
    transitions.append(fac.Transition(st_7, [
        fac.UpdateInstruction(cc_6, False) ]))
    transitions.append(fac.Transition(st_8, [
        fac.UpdateInstruction(cc_6, False) ]))
    transitions.append(fac.Transition(st_9, [
        fac.UpdateInstruction(cc_6, False) ]))
    st_6._set_transitionSet(transitions)
    transitions = []
    transitions.append(fac.Transition(st_7, [
        fac.UpdateInstruction(cc_7, True) ]))
    transitions.append(fac.Transition(st_8, [
        fac.UpdateInstruction(cc_7, False) ]))
    transitions.append(fac.Transition(st_9, [
        fac.UpdateInstruction(cc_7, False) ]))
    st_7._set_transitionSet(transitions)
    transitions = []
    transitions.append(fac.Transition(st_8, [
        fac.UpdateInstruction(cc_8, True) ]))
    transitions.append(fac.Transition(st_9, [
        fac.UpdateInstruction(cc_8, False) ]))
    st_8._set_transitionSet(transitions)
    transitions = []
    transitions.append(fac.Transition(st_9, [
         ]))
    transitions.append(fac.Transition(st_10, [
         ]))
    transitions.append(fac.Transition(st_11, [
         ]))
    transitions.append(fac.Transition(st_12, [
         ]))
    transitions.append(fac.Transition(st_13, [
         ]))
    st_9._set_transitionSet(transitions)
    transitions = []
    transitions.append(fac.Transition(st_10, [
        fac.UpdateInstruction(cc_9, True) ]))
    transitions.append(fac.Transition(st_11, [
        fac.UpdateInstruction(cc_9, False) ]))
    transitions.append(fac.Transition(st_12, [
        fac.UpdateInstruction(cc_9, False) ]))
    transitions.append(fac.Transition(st_13, [
        fac.UpdateInstruction(cc_9, False) ]))
    st_10._set_transitionSet(transitions)
    transitions = []
    transitions.append(fac.Transition(st_11, [
        fac.UpdateInstruction(cc_10, True) ]))
    transitions.append(fac.Transition(st_12, [
        fac.UpdateInstruction(cc_10, False) ]))
    transitions.append(fac.Transition(st_13, [
        fac.UpdateInstruction(cc_10, False) ]))
    st_11._set_transitionSet(transitions)
    transitions = []
    transitions.append(fac.Transition(st_12, [
        fac.UpdateInstruction(cc_11, True) ]))
    transitions.append(fac.Transition(st_13, [
        fac.UpdateInstruction(cc_11, False) ]))
    st_12._set_transitionSet(transitions)
    transitions = []
    transitions.append(fac.Transition(st_13, [
        fac.UpdateInstruction(cc_12, True) ]))
    st_13._set_transitionSet(transitions)
    return fac.Automaton(states, counters, False, containing_state=None)
IDPSSODescriptorType._Automaton = _BuildAutomaton_16()




SPSSODescriptorType._AddElement(pyxb.binding.basis.element(pyxb.namespace.ExpandedName(Namespace, 'AssertionConsumerService'), IndexedEndpointType, scope=SPSSODescriptorType, location=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/saml20/schemas/metadata.xsd', 256, 4)))

SPSSODescriptorType._AddElement(pyxb.binding.basis.element(pyxb.namespace.ExpandedName(Namespace, 'AttributeConsumingService'), AttributeConsumingServiceType, scope=SPSSODescriptorType, location=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/saml20/schemas/metadata.xsd', 257, 4)))

def _BuildAutomaton_17 ():
    # Remove this helper function from the namespace after it is invoked
    global _BuildAutomaton_17
    del _BuildAutomaton_17
    import pyxb.utils.fac as fac

    counters = set()
    cc_0 = fac.CounterCondition(min=0, max=1, metadata=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/saml20/schemas/metadata.xsd', 173, 12))
    counters.add(cc_0)
    cc_1 = fac.CounterCondition(min=0, max=1, metadata=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/saml20/schemas/metadata.xsd', 174, 12))
    counters.add(cc_1)
    cc_2 = fac.CounterCondition(min=0, max=None, metadata=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/saml20/schemas/metadata.xsd', 175, 12))
    counters.add(cc_2)
    cc_3 = fac.CounterCondition(min=0, max=1, metadata=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/saml20/schemas/metadata.xsd', 176, 12))
    counters.add(cc_3)
    cc_4 = fac.CounterCondition(min=0, max=None, metadata=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/saml20/schemas/metadata.xsd', 177, 12))
    counters.add(cc_4)
    cc_5 = fac.CounterCondition(min=0, max=None, metadata=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/saml20/schemas/metadata.xsd', 210, 20))
    counters.add(cc_5)
    cc_6 = fac.CounterCondition(min=0, max=None, metadata=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/saml20/schemas/metadata.xsd', 211, 20))
    counters.add(cc_6)
    cc_7 = fac.CounterCondition(min=0, max=None, metadata=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/saml20/schemas/metadata.xsd', 212, 20))
    counters.add(cc_7)
    cc_8 = fac.CounterCondition(min=0, max=None, metadata=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/saml20/schemas/metadata.xsd', 213, 20))
    counters.add(cc_8)
    cc_9 = fac.CounterCondition(min=0, max=None, metadata=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/saml20/schemas/metadata.xsd', 249, 20))
    counters.add(cc_9)
    states = []
    final_update = None
    symbol = pyxb.binding.content.ElementUse(SPSSODescriptorType._UseForTag(pyxb.namespace.ExpandedName(_Namespace_ds, 'Signature')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/saml20/schemas/metadata.xsd', 173, 12))
    st_0 = fac.State(symbol, is_initial=True, final_update=final_update, is_unordered_catenation=False)
    states.append(st_0)
    final_update = None
    symbol = pyxb.binding.content.ElementUse(SPSSODescriptorType._UseForTag(pyxb.namespace.ExpandedName(Namespace, 'Extensions')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/saml20/schemas/metadata.xsd', 174, 12))
    st_1 = fac.State(symbol, is_initial=True, final_update=final_update, is_unordered_catenation=False)
    states.append(st_1)
    final_update = None
    symbol = pyxb.binding.content.ElementUse(SPSSODescriptorType._UseForTag(pyxb.namespace.ExpandedName(Namespace, 'KeyDescriptor')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/saml20/schemas/metadata.xsd', 175, 12))
    st_2 = fac.State(symbol, is_initial=True, final_update=final_update, is_unordered_catenation=False)
    states.append(st_2)
    final_update = None
    symbol = pyxb.binding.content.ElementUse(SPSSODescriptorType._UseForTag(pyxb.namespace.ExpandedName(Namespace, 'Organization')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/saml20/schemas/metadata.xsd', 176, 12))
    st_3 = fac.State(symbol, is_initial=True, final_update=final_update, is_unordered_catenation=False)
    states.append(st_3)
    final_update = None
    symbol = pyxb.binding.content.ElementUse(SPSSODescriptorType._UseForTag(pyxb.namespace.ExpandedName(Namespace, 'ContactPerson')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/saml20/schemas/metadata.xsd', 177, 12))
    st_4 = fac.State(symbol, is_initial=True, final_update=final_update, is_unordered_catenation=False)
    states.append(st_4)
    final_update = None
    symbol = pyxb.binding.content.ElementUse(SPSSODescriptorType._UseForTag(pyxb.namespace.ExpandedName(Namespace, 'ArtifactResolutionService')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/saml20/schemas/metadata.xsd', 210, 20))
    st_5 = fac.State(symbol, is_initial=True, final_update=final_update, is_unordered_catenation=False)
    states.append(st_5)
    final_update = None
    symbol = pyxb.binding.content.ElementUse(SPSSODescriptorType._UseForTag(pyxb.namespace.ExpandedName(Namespace, 'SingleLogoutService')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/saml20/schemas/metadata.xsd', 211, 20))
    st_6 = fac.State(symbol, is_initial=True, final_update=final_update, is_unordered_catenation=False)
    states.append(st_6)
    final_update = None
    symbol = pyxb.binding.content.ElementUse(SPSSODescriptorType._UseForTag(pyxb.namespace.ExpandedName(Namespace, 'ManageNameIDService')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/saml20/schemas/metadata.xsd', 212, 20))
    st_7 = fac.State(symbol, is_initial=True, final_update=final_update, is_unordered_catenation=False)
    states.append(st_7)
    final_update = None
    symbol = pyxb.binding.content.ElementUse(SPSSODescriptorType._UseForTag(pyxb.namespace.ExpandedName(Namespace, 'NameIDFormat')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/saml20/schemas/metadata.xsd', 213, 20))
    st_8 = fac.State(symbol, is_initial=True, final_update=final_update, is_unordered_catenation=False)
    states.append(st_8)
    final_update = set()
    symbol = pyxb.binding.content.ElementUse(SPSSODescriptorType._UseForTag(pyxb.namespace.ExpandedName(Namespace, 'AssertionConsumerService')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/saml20/schemas/metadata.xsd', 248, 20))
    st_9 = fac.State(symbol, is_initial=True, final_update=final_update, is_unordered_catenation=False)
    states.append(st_9)
    final_update = set()
    final_update.add(fac.UpdateInstruction(cc_9, False))
    symbol = pyxb.binding.content.ElementUse(SPSSODescriptorType._UseForTag(pyxb.namespace.ExpandedName(Namespace, 'AttributeConsumingService')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/saml20/schemas/metadata.xsd', 249, 20))
    st_10 = fac.State(symbol, is_initial=False, final_update=final_update, is_unordered_catenation=False)
    states.append(st_10)
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
    transitions.append(fac.Transition(st_8, [
        fac.UpdateInstruction(cc_0, False) ]))
    transitions.append(fac.Transition(st_9, [
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
    transitions.append(fac.Transition(st_5, [
        fac.UpdateInstruction(cc_1, False) ]))
    transitions.append(fac.Transition(st_6, [
        fac.UpdateInstruction(cc_1, False) ]))
    transitions.append(fac.Transition(st_7, [
        fac.UpdateInstruction(cc_1, False) ]))
    transitions.append(fac.Transition(st_8, [
        fac.UpdateInstruction(cc_1, False) ]))
    transitions.append(fac.Transition(st_9, [
        fac.UpdateInstruction(cc_1, False) ]))
    st_1._set_transitionSet(transitions)
    transitions = []
    transitions.append(fac.Transition(st_2, [
        fac.UpdateInstruction(cc_2, True) ]))
    transitions.append(fac.Transition(st_3, [
        fac.UpdateInstruction(cc_2, False) ]))
    transitions.append(fac.Transition(st_4, [
        fac.UpdateInstruction(cc_2, False) ]))
    transitions.append(fac.Transition(st_5, [
        fac.UpdateInstruction(cc_2, False) ]))
    transitions.append(fac.Transition(st_6, [
        fac.UpdateInstruction(cc_2, False) ]))
    transitions.append(fac.Transition(st_7, [
        fac.UpdateInstruction(cc_2, False) ]))
    transitions.append(fac.Transition(st_8, [
        fac.UpdateInstruction(cc_2, False) ]))
    transitions.append(fac.Transition(st_9, [
        fac.UpdateInstruction(cc_2, False) ]))
    st_2._set_transitionSet(transitions)
    transitions = []
    transitions.append(fac.Transition(st_3, [
        fac.UpdateInstruction(cc_3, True) ]))
    transitions.append(fac.Transition(st_4, [
        fac.UpdateInstruction(cc_3, False) ]))
    transitions.append(fac.Transition(st_5, [
        fac.UpdateInstruction(cc_3, False) ]))
    transitions.append(fac.Transition(st_6, [
        fac.UpdateInstruction(cc_3, False) ]))
    transitions.append(fac.Transition(st_7, [
        fac.UpdateInstruction(cc_3, False) ]))
    transitions.append(fac.Transition(st_8, [
        fac.UpdateInstruction(cc_3, False) ]))
    transitions.append(fac.Transition(st_9, [
        fac.UpdateInstruction(cc_3, False) ]))
    st_3._set_transitionSet(transitions)
    transitions = []
    transitions.append(fac.Transition(st_4, [
        fac.UpdateInstruction(cc_4, True) ]))
    transitions.append(fac.Transition(st_5, [
        fac.UpdateInstruction(cc_4, False) ]))
    transitions.append(fac.Transition(st_6, [
        fac.UpdateInstruction(cc_4, False) ]))
    transitions.append(fac.Transition(st_7, [
        fac.UpdateInstruction(cc_4, False) ]))
    transitions.append(fac.Transition(st_8, [
        fac.UpdateInstruction(cc_4, False) ]))
    transitions.append(fac.Transition(st_9, [
        fac.UpdateInstruction(cc_4, False) ]))
    st_4._set_transitionSet(transitions)
    transitions = []
    transitions.append(fac.Transition(st_5, [
        fac.UpdateInstruction(cc_5, True) ]))
    transitions.append(fac.Transition(st_6, [
        fac.UpdateInstruction(cc_5, False) ]))
    transitions.append(fac.Transition(st_7, [
        fac.UpdateInstruction(cc_5, False) ]))
    transitions.append(fac.Transition(st_8, [
        fac.UpdateInstruction(cc_5, False) ]))
    transitions.append(fac.Transition(st_9, [
        fac.UpdateInstruction(cc_5, False) ]))
    st_5._set_transitionSet(transitions)
    transitions = []
    transitions.append(fac.Transition(st_6, [
        fac.UpdateInstruction(cc_6, True) ]))
    transitions.append(fac.Transition(st_7, [
        fac.UpdateInstruction(cc_6, False) ]))
    transitions.append(fac.Transition(st_8, [
        fac.UpdateInstruction(cc_6, False) ]))
    transitions.append(fac.Transition(st_9, [
        fac.UpdateInstruction(cc_6, False) ]))
    st_6._set_transitionSet(transitions)
    transitions = []
    transitions.append(fac.Transition(st_7, [
        fac.UpdateInstruction(cc_7, True) ]))
    transitions.append(fac.Transition(st_8, [
        fac.UpdateInstruction(cc_7, False) ]))
    transitions.append(fac.Transition(st_9, [
        fac.UpdateInstruction(cc_7, False) ]))
    st_7._set_transitionSet(transitions)
    transitions = []
    transitions.append(fac.Transition(st_8, [
        fac.UpdateInstruction(cc_8, True) ]))
    transitions.append(fac.Transition(st_9, [
        fac.UpdateInstruction(cc_8, False) ]))
    st_8._set_transitionSet(transitions)
    transitions = []
    transitions.append(fac.Transition(st_9, [
         ]))
    transitions.append(fac.Transition(st_10, [
         ]))
    st_9._set_transitionSet(transitions)
    transitions = []
    transitions.append(fac.Transition(st_10, [
        fac.UpdateInstruction(cc_9, True) ]))
    st_10._set_transitionSet(transitions)
    return fac.Automaton(states, counters, False, containing_state=None)
SPSSODescriptorType._Automaton = _BuildAutomaton_17()

