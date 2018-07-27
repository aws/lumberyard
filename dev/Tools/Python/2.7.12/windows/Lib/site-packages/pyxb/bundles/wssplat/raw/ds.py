# ./pyxb/bundles/wssplat/raw/ds.py
# -*- coding: utf-8 -*-
# PyXB bindings for NM:f1c343a882e7a65fb879f4ee813309f8231f28c8
# Generated 2014-10-19 06:24:57.815001 by PyXB version 1.2.4 using Python 2.7.3.final.0
# Namespace http://www.w3.org/2000/09/xmldsig#

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
_GenerationUID = pyxb.utils.utility.UniqueIdentifier('urn:uuid:8e979af2-5782-11e4-bdc1-c8600024e903')

# Version of PyXB used to generate the bindings
_PyXBVersion = '1.2.4'
# Generated bindings are not compatible across PyXB versions
if pyxb.__version__ != _PyXBVersion:
    raise pyxb.PyXBVersionError(_PyXBVersion)

# Import bindings for namespaces imported into schema
import pyxb.binding.datatypes

# NOTE: All namespace declarations are reserved within the binding
Namespace = pyxb.namespace.NamespaceForURI('http://www.w3.org/2000/09/xmldsig#', create_if_missing=True)
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


# Atomic simple type: {http://www.w3.org/2000/09/xmldsig#}CryptoBinary
class CryptoBinary (pyxb.binding.datatypes.base64Binary):

    """An atomic simple type."""

    _ExpandedName = pyxb.namespace.ExpandedName(Namespace, 'CryptoBinary')
    _XSDLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/ds.xsd', 36, 0)
    _Documentation = None
CryptoBinary._InitializeFacetMap()
Namespace.addCategoryObject('typeBinding', 'CryptoBinary', CryptoBinary)

# Atomic simple type: {http://www.w3.org/2000/09/xmldsig#}DigestValueType
class DigestValueType (pyxb.binding.datatypes.base64Binary):

    """An atomic simple type."""

    _ExpandedName = pyxb.namespace.ExpandedName(Namespace, 'DigestValueType')
    _XSDLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/ds.xsd', 136, 0)
    _Documentation = None
DigestValueType._InitializeFacetMap()
Namespace.addCategoryObject('typeBinding', 'DigestValueType', DigestValueType)

# Atomic simple type: {http://www.w3.org/2000/09/xmldsig#}HMACOutputLengthType
class HMACOutputLengthType (pyxb.binding.datatypes.integer):

    """An atomic simple type."""

    _ExpandedName = pyxb.namespace.ExpandedName(Namespace, 'HMACOutputLengthType')
    _XSDLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/ds.xsd', 283, 0)
    _Documentation = None
HMACOutputLengthType._InitializeFacetMap()
Namespace.addCategoryObject('typeBinding', 'HMACOutputLengthType', HMACOutputLengthType)

# Complex type {http://www.w3.org/2000/09/xmldsig#}SignatureType with content type ELEMENT_ONLY
class SignatureType (pyxb.binding.basis.complexTypeDefinition):
    """Complex type {http://www.w3.org/2000/09/xmldsig#}SignatureType with content type ELEMENT_ONLY"""
    _TypeDefinition = None
    _ContentTypeTag = pyxb.binding.basis.complexTypeDefinition._CT_ELEMENT_ONLY
    _Abstract = False
    _ExpandedName = pyxb.namespace.ExpandedName(Namespace, 'SignatureType')
    _XSDLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/ds.xsd', 44, 0)
    _ElementMap = {}
    _AttributeMap = {}
    # Base type is pyxb.binding.datatypes.anyType
    
    # Element {http://www.w3.org/2000/09/xmldsig#}SignatureValue uses Python identifier SignatureValue
    __SignatureValue = pyxb.binding.content.ElementDeclaration(pyxb.namespace.ExpandedName(Namespace, 'SignatureValue'), 'SignatureValue', '__httpwww_w3_org200009xmldsig_SignatureType_httpwww_w3_org200009xmldsigSignatureValue', False, pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/ds.xsd', 54, 2), )

    
    SignatureValue = property(__SignatureValue.value, __SignatureValue.set, None, None)

    
    # Element {http://www.w3.org/2000/09/xmldsig#}SignedInfo uses Python identifier SignedInfo
    __SignedInfo = pyxb.binding.content.ElementDeclaration(pyxb.namespace.ExpandedName(Namespace, 'SignedInfo'), 'SignedInfo', '__httpwww_w3_org200009xmldsig_SignatureType_httpwww_w3_org200009xmldsigSignedInfo', False, pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/ds.xsd', 65, 0), )

    
    SignedInfo = property(__SignedInfo.value, __SignedInfo.set, None, None)

    
    # Element {http://www.w3.org/2000/09/xmldsig#}KeyInfo uses Python identifier KeyInfo
    __KeyInfo = pyxb.binding.content.ElementDeclaration(pyxb.namespace.ExpandedName(Namespace, 'KeyInfo'), 'KeyInfo', '__httpwww_w3_org200009xmldsig_SignatureType_httpwww_w3_org200009xmldsigKeyInfo', False, pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/ds.xsd', 144, 0), )

    
    KeyInfo = property(__KeyInfo.value, __KeyInfo.set, None, None)

    
    # Element {http://www.w3.org/2000/09/xmldsig#}Object uses Python identifier Object
    __Object = pyxb.binding.content.ElementDeclaration(pyxb.namespace.ExpandedName(Namespace, 'Object'), 'Object', '__httpwww_w3_org200009xmldsig_SignatureType_httpwww_w3_org200009xmldsigObject', True, pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/ds.xsd', 243, 0), )

    
    Object = property(__Object.value, __Object.set, None, None)

    
    # Attribute Id uses Python identifier Id
    __Id = pyxb.binding.content.AttributeUse(pyxb.namespace.ExpandedName(None, 'Id'), 'Id', '__httpwww_w3_org200009xmldsig_SignatureType_Id', pyxb.binding.datatypes.ID)
    __Id._DeclarationLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/ds.xsd', 51, 2)
    __Id._UseLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/ds.xsd', 51, 2)
    
    Id = property(__Id.value, __Id.set, None, None)

    _ElementMap.update({
        __SignatureValue.name() : __SignatureValue,
        __SignedInfo.name() : __SignedInfo,
        __KeyInfo.name() : __KeyInfo,
        __Object.name() : __Object
    })
    _AttributeMap.update({
        __Id.name() : __Id
    })
Namespace.addCategoryObject('typeBinding', 'SignatureType', SignatureType)


# Complex type {http://www.w3.org/2000/09/xmldsig#}SignatureValueType with content type SIMPLE
class SignatureValueType (pyxb.binding.basis.complexTypeDefinition):
    """Complex type {http://www.w3.org/2000/09/xmldsig#}SignatureValueType with content type SIMPLE"""
    _TypeDefinition = pyxb.binding.datatypes.base64Binary
    _ContentTypeTag = pyxb.binding.basis.complexTypeDefinition._CT_SIMPLE
    _Abstract = False
    _ExpandedName = pyxb.namespace.ExpandedName(Namespace, 'SignatureValueType')
    _XSDLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/ds.xsd', 55, 2)
    _ElementMap = {}
    _AttributeMap = {}
    # Base type is pyxb.binding.datatypes.base64Binary
    
    # Attribute Id uses Python identifier Id
    __Id = pyxb.binding.content.AttributeUse(pyxb.namespace.ExpandedName(None, 'Id'), 'Id', '__httpwww_w3_org200009xmldsig_SignatureValueType_Id', pyxb.binding.datatypes.ID)
    __Id._DeclarationLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/ds.xsd', 58, 8)
    __Id._UseLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/ds.xsd', 58, 8)
    
    Id = property(__Id.value, __Id.set, None, None)

    _ElementMap.update({
        
    })
    _AttributeMap.update({
        __Id.name() : __Id
    })
Namespace.addCategoryObject('typeBinding', 'SignatureValueType', SignatureValueType)


# Complex type {http://www.w3.org/2000/09/xmldsig#}SignedInfoType with content type ELEMENT_ONLY
class SignedInfoType (pyxb.binding.basis.complexTypeDefinition):
    """Complex type {http://www.w3.org/2000/09/xmldsig#}SignedInfoType with content type ELEMENT_ONLY"""
    _TypeDefinition = None
    _ContentTypeTag = pyxb.binding.basis.complexTypeDefinition._CT_ELEMENT_ONLY
    _Abstract = False
    _ExpandedName = pyxb.namespace.ExpandedName(Namespace, 'SignedInfoType')
    _XSDLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/ds.xsd', 66, 0)
    _ElementMap = {}
    _AttributeMap = {}
    # Base type is pyxb.binding.datatypes.anyType
    
    # Element {http://www.w3.org/2000/09/xmldsig#}CanonicalizationMethod uses Python identifier CanonicalizationMethod
    __CanonicalizationMethod = pyxb.binding.content.ElementDeclaration(pyxb.namespace.ExpandedName(Namespace, 'CanonicalizationMethod'), 'CanonicalizationMethod', '__httpwww_w3_org200009xmldsig_SignedInfoType_httpwww_w3_org200009xmldsigCanonicalizationMethod', False, pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/ds.xsd', 75, 2), )

    
    CanonicalizationMethod = property(__CanonicalizationMethod.value, __CanonicalizationMethod.set, None, None)

    
    # Element {http://www.w3.org/2000/09/xmldsig#}SignatureMethod uses Python identifier SignatureMethod
    __SignatureMethod = pyxb.binding.content.ElementDeclaration(pyxb.namespace.ExpandedName(Namespace, 'SignatureMethod'), 'SignatureMethod', '__httpwww_w3_org200009xmldsig_SignedInfoType_httpwww_w3_org200009xmldsigSignatureMethod', False, pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/ds.xsd', 84, 2), )

    
    SignatureMethod = property(__SignatureMethod.value, __SignatureMethod.set, None, None)

    
    # Element {http://www.w3.org/2000/09/xmldsig#}Reference uses Python identifier Reference
    __Reference = pyxb.binding.content.ElementDeclaration(pyxb.namespace.ExpandedName(Namespace, 'Reference'), 'Reference', '__httpwww_w3_org200009xmldsig_SignedInfoType_httpwww_w3_org200009xmldsigReference', True, pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/ds.xsd', 96, 0), )

    
    Reference = property(__Reference.value, __Reference.set, None, None)

    
    # Attribute Id uses Python identifier Id
    __Id = pyxb.binding.content.AttributeUse(pyxb.namespace.ExpandedName(None, 'Id'), 'Id', '__httpwww_w3_org200009xmldsig_SignedInfoType_Id', pyxb.binding.datatypes.ID)
    __Id._DeclarationLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/ds.xsd', 72, 2)
    __Id._UseLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/ds.xsd', 72, 2)
    
    Id = property(__Id.value, __Id.set, None, None)

    _ElementMap.update({
        __CanonicalizationMethod.name() : __CanonicalizationMethod,
        __SignatureMethod.name() : __SignatureMethod,
        __Reference.name() : __Reference
    })
    _AttributeMap.update({
        __Id.name() : __Id
    })
Namespace.addCategoryObject('typeBinding', 'SignedInfoType', SignedInfoType)


# Complex type {http://www.w3.org/2000/09/xmldsig#}CanonicalizationMethodType with content type MIXED
class CanonicalizationMethodType (pyxb.binding.basis.complexTypeDefinition):
    """Complex type {http://www.w3.org/2000/09/xmldsig#}CanonicalizationMethodType with content type MIXED"""
    _TypeDefinition = None
    _ContentTypeTag = pyxb.binding.basis.complexTypeDefinition._CT_MIXED
    _Abstract = False
    _ExpandedName = pyxb.namespace.ExpandedName(Namespace, 'CanonicalizationMethodType')
    _XSDLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/ds.xsd', 76, 2)
    _ElementMap = {}
    _AttributeMap = {}
    # Base type is pyxb.binding.datatypes.anyType
    
    # Attribute Algorithm uses Python identifier Algorithm
    __Algorithm = pyxb.binding.content.AttributeUse(pyxb.namespace.ExpandedName(None, 'Algorithm'), 'Algorithm', '__httpwww_w3_org200009xmldsig_CanonicalizationMethodType_Algorithm', pyxb.binding.datatypes.anyURI, required=True)
    __Algorithm._DeclarationLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/ds.xsd', 81, 4)
    __Algorithm._UseLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/ds.xsd', 81, 4)
    
    Algorithm = property(__Algorithm.value, __Algorithm.set, None, None)

    _HasWildcardElement = True
    _ElementMap.update({
        
    })
    _AttributeMap.update({
        __Algorithm.name() : __Algorithm
    })
Namespace.addCategoryObject('typeBinding', 'CanonicalizationMethodType', CanonicalizationMethodType)


# Complex type {http://www.w3.org/2000/09/xmldsig#}SignatureMethodType with content type MIXED
class SignatureMethodType (pyxb.binding.basis.complexTypeDefinition):
    """Complex type {http://www.w3.org/2000/09/xmldsig#}SignatureMethodType with content type MIXED"""
    _TypeDefinition = None
    _ContentTypeTag = pyxb.binding.basis.complexTypeDefinition._CT_MIXED
    _Abstract = False
    _ExpandedName = pyxb.namespace.ExpandedName(Namespace, 'SignatureMethodType')
    _XSDLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/ds.xsd', 85, 2)
    _ElementMap = {}
    _AttributeMap = {}
    # Base type is pyxb.binding.datatypes.anyType
    
    # Element {http://www.w3.org/2000/09/xmldsig#}HMACOutputLength uses Python identifier HMACOutputLength
    __HMACOutputLength = pyxb.binding.content.ElementDeclaration(pyxb.namespace.ExpandedName(Namespace, 'HMACOutputLength'), 'HMACOutputLength', '__httpwww_w3_org200009xmldsig_SignatureMethodType_httpwww_w3_org200009xmldsigHMACOutputLength', False, pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/ds.xsd', 87, 6), )

    
    HMACOutputLength = property(__HMACOutputLength.value, __HMACOutputLength.set, None, None)

    
    # Attribute Algorithm uses Python identifier Algorithm
    __Algorithm = pyxb.binding.content.AttributeUse(pyxb.namespace.ExpandedName(None, 'Algorithm'), 'Algorithm', '__httpwww_w3_org200009xmldsig_SignatureMethodType_Algorithm', pyxb.binding.datatypes.anyURI, required=True)
    __Algorithm._DeclarationLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/ds.xsd', 91, 4)
    __Algorithm._UseLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/ds.xsd', 91, 4)
    
    Algorithm = property(__Algorithm.value, __Algorithm.set, None, None)

    _HasWildcardElement = True
    _ElementMap.update({
        __HMACOutputLength.name() : __HMACOutputLength
    })
    _AttributeMap.update({
        __Algorithm.name() : __Algorithm
    })
Namespace.addCategoryObject('typeBinding', 'SignatureMethodType', SignatureMethodType)


# Complex type {http://www.w3.org/2000/09/xmldsig#}ReferenceType with content type ELEMENT_ONLY
class ReferenceType (pyxb.binding.basis.complexTypeDefinition):
    """Complex type {http://www.w3.org/2000/09/xmldsig#}ReferenceType with content type ELEMENT_ONLY"""
    _TypeDefinition = None
    _ContentTypeTag = pyxb.binding.basis.complexTypeDefinition._CT_ELEMENT_ONLY
    _Abstract = False
    _ExpandedName = pyxb.namespace.ExpandedName(Namespace, 'ReferenceType')
    _XSDLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/ds.xsd', 97, 0)
    _ElementMap = {}
    _AttributeMap = {}
    # Base type is pyxb.binding.datatypes.anyType
    
    # Element {http://www.w3.org/2000/09/xmldsig#}Transforms uses Python identifier Transforms
    __Transforms = pyxb.binding.content.ElementDeclaration(pyxb.namespace.ExpandedName(Namespace, 'Transforms'), 'Transforms', '__httpwww_w3_org200009xmldsig_ReferenceType_httpwww_w3_org200009xmldsigTransforms', False, pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/ds.xsd', 108, 2), )

    
    Transforms = property(__Transforms.value, __Transforms.set, None, None)

    
    # Element {http://www.w3.org/2000/09/xmldsig#}DigestMethod uses Python identifier DigestMethod
    __DigestMethod = pyxb.binding.content.ElementDeclaration(pyxb.namespace.ExpandedName(Namespace, 'DigestMethod'), 'DigestMethod', '__httpwww_w3_org200009xmldsig_ReferenceType_httpwww_w3_org200009xmldsigDigestMethod', False, pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/ds.xsd', 127, 0), )

    
    DigestMethod = property(__DigestMethod.value, __DigestMethod.set, None, None)

    
    # Element {http://www.w3.org/2000/09/xmldsig#}DigestValue uses Python identifier DigestValue
    __DigestValue = pyxb.binding.content.ElementDeclaration(pyxb.namespace.ExpandedName(Namespace, 'DigestValue'), 'DigestValue', '__httpwww_w3_org200009xmldsig_ReferenceType_httpwww_w3_org200009xmldsigDigestValue', False, pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/ds.xsd', 135, 0), )

    
    DigestValue = property(__DigestValue.value, __DigestValue.set, None, None)

    
    # Attribute Id uses Python identifier Id
    __Id = pyxb.binding.content.AttributeUse(pyxb.namespace.ExpandedName(None, 'Id'), 'Id', '__httpwww_w3_org200009xmldsig_ReferenceType_Id', pyxb.binding.datatypes.ID)
    __Id._DeclarationLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/ds.xsd', 103, 2)
    __Id._UseLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/ds.xsd', 103, 2)
    
    Id = property(__Id.value, __Id.set, None, None)

    
    # Attribute URI uses Python identifier URI
    __URI = pyxb.binding.content.AttributeUse(pyxb.namespace.ExpandedName(None, 'URI'), 'URI', '__httpwww_w3_org200009xmldsig_ReferenceType_URI', pyxb.binding.datatypes.anyURI)
    __URI._DeclarationLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/ds.xsd', 104, 2)
    __URI._UseLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/ds.xsd', 104, 2)
    
    URI = property(__URI.value, __URI.set, None, None)

    
    # Attribute Type uses Python identifier Type
    __Type = pyxb.binding.content.AttributeUse(pyxb.namespace.ExpandedName(None, 'Type'), 'Type', '__httpwww_w3_org200009xmldsig_ReferenceType_Type', pyxb.binding.datatypes.anyURI)
    __Type._DeclarationLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/ds.xsd', 105, 2)
    __Type._UseLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/ds.xsd', 105, 2)
    
    Type = property(__Type.value, __Type.set, None, None)

    _ElementMap.update({
        __Transforms.name() : __Transforms,
        __DigestMethod.name() : __DigestMethod,
        __DigestValue.name() : __DigestValue
    })
    _AttributeMap.update({
        __Id.name() : __Id,
        __URI.name() : __URI,
        __Type.name() : __Type
    })
Namespace.addCategoryObject('typeBinding', 'ReferenceType', ReferenceType)


# Complex type {http://www.w3.org/2000/09/xmldsig#}TransformsType with content type ELEMENT_ONLY
class TransformsType (pyxb.binding.basis.complexTypeDefinition):
    """Complex type {http://www.w3.org/2000/09/xmldsig#}TransformsType with content type ELEMENT_ONLY"""
    _TypeDefinition = None
    _ContentTypeTag = pyxb.binding.basis.complexTypeDefinition._CT_ELEMENT_ONLY
    _Abstract = False
    _ExpandedName = pyxb.namespace.ExpandedName(Namespace, 'TransformsType')
    _XSDLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/ds.xsd', 109, 2)
    _ElementMap = {}
    _AttributeMap = {}
    # Base type is pyxb.binding.datatypes.anyType
    
    # Element {http://www.w3.org/2000/09/xmldsig#}Transform uses Python identifier Transform
    __Transform = pyxb.binding.content.ElementDeclaration(pyxb.namespace.ExpandedName(Namespace, 'Transform'), 'Transform', '__httpwww_w3_org200009xmldsig_TransformsType_httpwww_w3_org200009xmldsigTransform', True, pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/ds.xsd', 115, 2), )

    
    Transform = property(__Transform.value, __Transform.set, None, None)

    _ElementMap.update({
        __Transform.name() : __Transform
    })
    _AttributeMap.update({
        
    })
Namespace.addCategoryObject('typeBinding', 'TransformsType', TransformsType)


# Complex type {http://www.w3.org/2000/09/xmldsig#}TransformType with content type MIXED
class TransformType (pyxb.binding.basis.complexTypeDefinition):
    """Complex type {http://www.w3.org/2000/09/xmldsig#}TransformType with content type MIXED"""
    _TypeDefinition = None
    _ContentTypeTag = pyxb.binding.basis.complexTypeDefinition._CT_MIXED
    _Abstract = False
    _ExpandedName = pyxb.namespace.ExpandedName(Namespace, 'TransformType')
    _XSDLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/ds.xsd', 116, 2)
    _ElementMap = {}
    _AttributeMap = {}
    # Base type is pyxb.binding.datatypes.anyType
    
    # Element {http://www.w3.org/2000/09/xmldsig#}XPath uses Python identifier XPath
    __XPath = pyxb.binding.content.ElementDeclaration(pyxb.namespace.ExpandedName(Namespace, 'XPath'), 'XPath', '__httpwww_w3_org200009xmldsig_TransformType_httpwww_w3_org200009xmldsigXPath', True, pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/ds.xsd', 120, 6), )

    
    XPath = property(__XPath.value, __XPath.set, None, None)

    
    # Attribute Algorithm uses Python identifier Algorithm
    __Algorithm = pyxb.binding.content.AttributeUse(pyxb.namespace.ExpandedName(None, 'Algorithm'), 'Algorithm', '__httpwww_w3_org200009xmldsig_TransformType_Algorithm', pyxb.binding.datatypes.anyURI, required=True)
    __Algorithm._DeclarationLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/ds.xsd', 122, 4)
    __Algorithm._UseLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/ds.xsd', 122, 4)
    
    Algorithm = property(__Algorithm.value, __Algorithm.set, None, None)

    _HasWildcardElement = True
    _ElementMap.update({
        __XPath.name() : __XPath
    })
    _AttributeMap.update({
        __Algorithm.name() : __Algorithm
    })
Namespace.addCategoryObject('typeBinding', 'TransformType', TransformType)


# Complex type {http://www.w3.org/2000/09/xmldsig#}DigestMethodType with content type MIXED
class DigestMethodType (pyxb.binding.basis.complexTypeDefinition):
    """Complex type {http://www.w3.org/2000/09/xmldsig#}DigestMethodType with content type MIXED"""
    _TypeDefinition = None
    _ContentTypeTag = pyxb.binding.basis.complexTypeDefinition._CT_MIXED
    _Abstract = False
    _ExpandedName = pyxb.namespace.ExpandedName(Namespace, 'DigestMethodType')
    _XSDLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/ds.xsd', 128, 0)
    _ElementMap = {}
    _AttributeMap = {}
    # Base type is pyxb.binding.datatypes.anyType
    
    # Attribute Algorithm uses Python identifier Algorithm
    __Algorithm = pyxb.binding.content.AttributeUse(pyxb.namespace.ExpandedName(None, 'Algorithm'), 'Algorithm', '__httpwww_w3_org200009xmldsig_DigestMethodType_Algorithm', pyxb.binding.datatypes.anyURI, required=True)
    __Algorithm._DeclarationLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/ds.xsd', 132, 2)
    __Algorithm._UseLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/ds.xsd', 132, 2)
    
    Algorithm = property(__Algorithm.value, __Algorithm.set, None, None)

    _HasWildcardElement = True
    _ElementMap.update({
        
    })
    _AttributeMap.update({
        __Algorithm.name() : __Algorithm
    })
Namespace.addCategoryObject('typeBinding', 'DigestMethodType', DigestMethodType)


# Complex type {http://www.w3.org/2000/09/xmldsig#}KeyInfoType with content type MIXED
class KeyInfoType (pyxb.binding.basis.complexTypeDefinition):
    """Complex type {http://www.w3.org/2000/09/xmldsig#}KeyInfoType with content type MIXED"""
    _TypeDefinition = None
    _ContentTypeTag = pyxb.binding.basis.complexTypeDefinition._CT_MIXED
    _Abstract = False
    _ExpandedName = pyxb.namespace.ExpandedName(Namespace, 'KeyInfoType')
    _XSDLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/ds.xsd', 145, 0)
    _ElementMap = {}
    _AttributeMap = {}
    # Base type is pyxb.binding.datatypes.anyType
    
    # Element {http://www.w3.org/2000/09/xmldsig#}KeyName uses Python identifier KeyName
    __KeyName = pyxb.binding.content.ElementDeclaration(pyxb.namespace.ExpandedName(Namespace, 'KeyName'), 'KeyName', '__httpwww_w3_org200009xmldsig_KeyInfoType_httpwww_w3_org200009xmldsigKeyName', True, pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/ds.xsd', 160, 2), )

    
    KeyName = property(__KeyName.value, __KeyName.set, None, None)

    
    # Element {http://www.w3.org/2000/09/xmldsig#}MgmtData uses Python identifier MgmtData
    __MgmtData = pyxb.binding.content.ElementDeclaration(pyxb.namespace.ExpandedName(Namespace, 'MgmtData'), 'MgmtData', '__httpwww_w3_org200009xmldsig_KeyInfoType_httpwww_w3_org200009xmldsigMgmtData', True, pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/ds.xsd', 161, 2), )

    
    MgmtData = property(__MgmtData.value, __MgmtData.set, None, None)

    
    # Element {http://www.w3.org/2000/09/xmldsig#}KeyValue uses Python identifier KeyValue
    __KeyValue = pyxb.binding.content.ElementDeclaration(pyxb.namespace.ExpandedName(Namespace, 'KeyValue'), 'KeyValue', '__httpwww_w3_org200009xmldsig_KeyInfoType_httpwww_w3_org200009xmldsigKeyValue', True, pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/ds.xsd', 163, 2), )

    
    KeyValue = property(__KeyValue.value, __KeyValue.set, None, None)

    
    # Element {http://www.w3.org/2000/09/xmldsig#}RetrievalMethod uses Python identifier RetrievalMethod
    __RetrievalMethod = pyxb.binding.content.ElementDeclaration(pyxb.namespace.ExpandedName(Namespace, 'RetrievalMethod'), 'RetrievalMethod', '__httpwww_w3_org200009xmldsig_KeyInfoType_httpwww_w3_org200009xmldsigRetrievalMethod', True, pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/ds.xsd', 172, 2), )

    
    RetrievalMethod = property(__RetrievalMethod.value, __RetrievalMethod.set, None, None)

    
    # Element {http://www.w3.org/2000/09/xmldsig#}X509Data uses Python identifier X509Data
    __X509Data = pyxb.binding.content.ElementDeclaration(pyxb.namespace.ExpandedName(Namespace, 'X509Data'), 'X509Data', '__httpwww_w3_org200009xmldsig_KeyInfoType_httpwww_w3_org200009xmldsigX509Data', True, pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/ds.xsd', 183, 0), )

    
    X509Data = property(__X509Data.value, __X509Data.set, None, None)

    
    # Element {http://www.w3.org/2000/09/xmldsig#}PGPData uses Python identifier PGPData
    __PGPData = pyxb.binding.content.ElementDeclaration(pyxb.namespace.ExpandedName(Namespace, 'PGPData'), 'PGPData', '__httpwww_w3_org200009xmldsig_KeyInfoType_httpwww_w3_org200009xmldsigPGPData', True, pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/ds.xsd', 208, 0), )

    
    PGPData = property(__PGPData.value, __PGPData.set, None, None)

    
    # Element {http://www.w3.org/2000/09/xmldsig#}SPKIData uses Python identifier SPKIData
    __SPKIData = pyxb.binding.content.ElementDeclaration(pyxb.namespace.ExpandedName(Namespace, 'SPKIData'), 'SPKIData', '__httpwww_w3_org200009xmldsig_KeyInfoType_httpwww_w3_org200009xmldsigSPKIData', True, pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/ds.xsd', 229, 0), )

    
    SPKIData = property(__SPKIData.value, __SPKIData.set, None, None)

    
    # Attribute Id uses Python identifier Id
    __Id = pyxb.binding.content.AttributeUse(pyxb.namespace.ExpandedName(None, 'Id'), 'Id', '__httpwww_w3_org200009xmldsig_KeyInfoType_Id', pyxb.binding.datatypes.ID)
    __Id._DeclarationLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/ds.xsd', 157, 2)
    __Id._UseLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/ds.xsd', 157, 2)
    
    Id = property(__Id.value, __Id.set, None, None)

    _HasWildcardElement = True
    _ElementMap.update({
        __KeyName.name() : __KeyName,
        __MgmtData.name() : __MgmtData,
        __KeyValue.name() : __KeyValue,
        __RetrievalMethod.name() : __RetrievalMethod,
        __X509Data.name() : __X509Data,
        __PGPData.name() : __PGPData,
        __SPKIData.name() : __SPKIData
    })
    _AttributeMap.update({
        __Id.name() : __Id
    })
Namespace.addCategoryObject('typeBinding', 'KeyInfoType', KeyInfoType)


# Complex type {http://www.w3.org/2000/09/xmldsig#}KeyValueType with content type MIXED
class KeyValueType (pyxb.binding.basis.complexTypeDefinition):
    """Complex type {http://www.w3.org/2000/09/xmldsig#}KeyValueType with content type MIXED"""
    _TypeDefinition = None
    _ContentTypeTag = pyxb.binding.basis.complexTypeDefinition._CT_MIXED
    _Abstract = False
    _ExpandedName = pyxb.namespace.ExpandedName(Namespace, 'KeyValueType')
    _XSDLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/ds.xsd', 164, 2)
    _ElementMap = {}
    _AttributeMap = {}
    # Base type is pyxb.binding.datatypes.anyType
    
    # Element {http://www.w3.org/2000/09/xmldsig#}DSAKeyValue uses Python identifier DSAKeyValue
    __DSAKeyValue = pyxb.binding.content.ElementDeclaration(pyxb.namespace.ExpandedName(Namespace, 'DSAKeyValue'), 'DSAKeyValue', '__httpwww_w3_org200009xmldsig_KeyValueType_httpwww_w3_org200009xmldsigDSAKeyValue', False, pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/ds.xsd', 289, 0), )

    
    DSAKeyValue = property(__DSAKeyValue.value, __DSAKeyValue.set, None, None)

    
    # Element {http://www.w3.org/2000/09/xmldsig#}RSAKeyValue uses Python identifier RSAKeyValue
    __RSAKeyValue = pyxb.binding.content.ElementDeclaration(pyxb.namespace.ExpandedName(Namespace, 'RSAKeyValue'), 'RSAKeyValue', '__httpwww_w3_org200009xmldsig_KeyValueType_httpwww_w3_org200009xmldsigRSAKeyValue', False, pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/ds.xsd', 306, 0), )

    
    RSAKeyValue = property(__RSAKeyValue.value, __RSAKeyValue.set, None, None)

    _HasWildcardElement = True
    _ElementMap.update({
        __DSAKeyValue.name() : __DSAKeyValue,
        __RSAKeyValue.name() : __RSAKeyValue
    })
    _AttributeMap.update({
        
    })
Namespace.addCategoryObject('typeBinding', 'KeyValueType', KeyValueType)


# Complex type {http://www.w3.org/2000/09/xmldsig#}RetrievalMethodType with content type ELEMENT_ONLY
class RetrievalMethodType (pyxb.binding.basis.complexTypeDefinition):
    """Complex type {http://www.w3.org/2000/09/xmldsig#}RetrievalMethodType with content type ELEMENT_ONLY"""
    _TypeDefinition = None
    _ContentTypeTag = pyxb.binding.basis.complexTypeDefinition._CT_ELEMENT_ONLY
    _Abstract = False
    _ExpandedName = pyxb.namespace.ExpandedName(Namespace, 'RetrievalMethodType')
    _XSDLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/ds.xsd', 173, 2)
    _ElementMap = {}
    _AttributeMap = {}
    # Base type is pyxb.binding.datatypes.anyType
    
    # Element {http://www.w3.org/2000/09/xmldsig#}Transforms uses Python identifier Transforms
    __Transforms = pyxb.binding.content.ElementDeclaration(pyxb.namespace.ExpandedName(Namespace, 'Transforms'), 'Transforms', '__httpwww_w3_org200009xmldsig_RetrievalMethodType_httpwww_w3_org200009xmldsigTransforms', False, pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/ds.xsd', 108, 2), )

    
    Transforms = property(__Transforms.value, __Transforms.set, None, None)

    
    # Attribute URI uses Python identifier URI
    __URI = pyxb.binding.content.AttributeUse(pyxb.namespace.ExpandedName(None, 'URI'), 'URI', '__httpwww_w3_org200009xmldsig_RetrievalMethodType_URI', pyxb.binding.datatypes.anyURI)
    __URI._DeclarationLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/ds.xsd', 177, 4)
    __URI._UseLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/ds.xsd', 177, 4)
    
    URI = property(__URI.value, __URI.set, None, None)

    
    # Attribute Type uses Python identifier Type
    __Type = pyxb.binding.content.AttributeUse(pyxb.namespace.ExpandedName(None, 'Type'), 'Type', '__httpwww_w3_org200009xmldsig_RetrievalMethodType_Type', pyxb.binding.datatypes.anyURI)
    __Type._DeclarationLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/ds.xsd', 178, 4)
    __Type._UseLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/ds.xsd', 178, 4)
    
    Type = property(__Type.value, __Type.set, None, None)

    _ElementMap.update({
        __Transforms.name() : __Transforms
    })
    _AttributeMap.update({
        __URI.name() : __URI,
        __Type.name() : __Type
    })
Namespace.addCategoryObject('typeBinding', 'RetrievalMethodType', RetrievalMethodType)


# Complex type {http://www.w3.org/2000/09/xmldsig#}X509DataType with content type ELEMENT_ONLY
class X509DataType (pyxb.binding.basis.complexTypeDefinition):
    """Complex type {http://www.w3.org/2000/09/xmldsig#}X509DataType with content type ELEMENT_ONLY"""
    _TypeDefinition = None
    _ContentTypeTag = pyxb.binding.basis.complexTypeDefinition._CT_ELEMENT_ONLY
    _Abstract = False
    _ExpandedName = pyxb.namespace.ExpandedName(Namespace, 'X509DataType')
    _XSDLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/ds.xsd', 184, 0)
    _ElementMap = {}
    _AttributeMap = {}
    # Base type is pyxb.binding.datatypes.anyType
    
    # Element {http://www.w3.org/2000/09/xmldsig#}X509IssuerSerial uses Python identifier X509IssuerSerial
    __X509IssuerSerial = pyxb.binding.content.ElementDeclaration(pyxb.namespace.ExpandedName(Namespace, 'X509IssuerSerial'), 'X509IssuerSerial', '__httpwww_w3_org200009xmldsig_X509DataType_httpwww_w3_org200009xmldsigX509IssuerSerial', True, pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/ds.xsd', 187, 6), )

    
    X509IssuerSerial = property(__X509IssuerSerial.value, __X509IssuerSerial.set, None, None)

    
    # Element {http://www.w3.org/2000/09/xmldsig#}X509SKI uses Python identifier X509SKI
    __X509SKI = pyxb.binding.content.ElementDeclaration(pyxb.namespace.ExpandedName(Namespace, 'X509SKI'), 'X509SKI', '__httpwww_w3_org200009xmldsig_X509DataType_httpwww_w3_org200009xmldsigX509SKI', True, pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/ds.xsd', 188, 6), )

    
    X509SKI = property(__X509SKI.value, __X509SKI.set, None, None)

    
    # Element {http://www.w3.org/2000/09/xmldsig#}X509SubjectName uses Python identifier X509SubjectName
    __X509SubjectName = pyxb.binding.content.ElementDeclaration(pyxb.namespace.ExpandedName(Namespace, 'X509SubjectName'), 'X509SubjectName', '__httpwww_w3_org200009xmldsig_X509DataType_httpwww_w3_org200009xmldsigX509SubjectName', True, pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/ds.xsd', 189, 6), )

    
    X509SubjectName = property(__X509SubjectName.value, __X509SubjectName.set, None, None)

    
    # Element {http://www.w3.org/2000/09/xmldsig#}X509Certificate uses Python identifier X509Certificate
    __X509Certificate = pyxb.binding.content.ElementDeclaration(pyxb.namespace.ExpandedName(Namespace, 'X509Certificate'), 'X509Certificate', '__httpwww_w3_org200009xmldsig_X509DataType_httpwww_w3_org200009xmldsigX509Certificate', True, pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/ds.xsd', 190, 6), )

    
    X509Certificate = property(__X509Certificate.value, __X509Certificate.set, None, None)

    
    # Element {http://www.w3.org/2000/09/xmldsig#}X509CRL uses Python identifier X509CRL
    __X509CRL = pyxb.binding.content.ElementDeclaration(pyxb.namespace.ExpandedName(Namespace, 'X509CRL'), 'X509CRL', '__httpwww_w3_org200009xmldsig_X509DataType_httpwww_w3_org200009xmldsigX509CRL', True, pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/ds.xsd', 191, 6), )

    
    X509CRL = property(__X509CRL.value, __X509CRL.set, None, None)

    _HasWildcardElement = True
    _ElementMap.update({
        __X509IssuerSerial.name() : __X509IssuerSerial,
        __X509SKI.name() : __X509SKI,
        __X509SubjectName.name() : __X509SubjectName,
        __X509Certificate.name() : __X509Certificate,
        __X509CRL.name() : __X509CRL
    })
    _AttributeMap.update({
        
    })
Namespace.addCategoryObject('typeBinding', 'X509DataType', X509DataType)


# Complex type {http://www.w3.org/2000/09/xmldsig#}X509IssuerSerialType with content type ELEMENT_ONLY
class X509IssuerSerialType (pyxb.binding.basis.complexTypeDefinition):
    """Complex type {http://www.w3.org/2000/09/xmldsig#}X509IssuerSerialType with content type ELEMENT_ONLY"""
    _TypeDefinition = None
    _ContentTypeTag = pyxb.binding.basis.complexTypeDefinition._CT_ELEMENT_ONLY
    _Abstract = False
    _ExpandedName = pyxb.namespace.ExpandedName(Namespace, 'X509IssuerSerialType')
    _XSDLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/ds.xsd', 197, 0)
    _ElementMap = {}
    _AttributeMap = {}
    # Base type is pyxb.binding.datatypes.anyType
    
    # Element {http://www.w3.org/2000/09/xmldsig#}X509IssuerName uses Python identifier X509IssuerName
    __X509IssuerName = pyxb.binding.content.ElementDeclaration(pyxb.namespace.ExpandedName(Namespace, 'X509IssuerName'), 'X509IssuerName', '__httpwww_w3_org200009xmldsig_X509IssuerSerialType_httpwww_w3_org200009xmldsigX509IssuerName', False, pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/ds.xsd', 199, 4), )

    
    X509IssuerName = property(__X509IssuerName.value, __X509IssuerName.set, None, None)

    
    # Element {http://www.w3.org/2000/09/xmldsig#}X509SerialNumber uses Python identifier X509SerialNumber
    __X509SerialNumber = pyxb.binding.content.ElementDeclaration(pyxb.namespace.ExpandedName(Namespace, 'X509SerialNumber'), 'X509SerialNumber', '__httpwww_w3_org200009xmldsig_X509IssuerSerialType_httpwww_w3_org200009xmldsigX509SerialNumber', False, pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/ds.xsd', 200, 4), )

    
    X509SerialNumber = property(__X509SerialNumber.value, __X509SerialNumber.set, None, None)

    _ElementMap.update({
        __X509IssuerName.name() : __X509IssuerName,
        __X509SerialNumber.name() : __X509SerialNumber
    })
    _AttributeMap.update({
        
    })
Namespace.addCategoryObject('typeBinding', 'X509IssuerSerialType', X509IssuerSerialType)


# Complex type {http://www.w3.org/2000/09/xmldsig#}PGPDataType with content type ELEMENT_ONLY
class PGPDataType (pyxb.binding.basis.complexTypeDefinition):
    """Complex type {http://www.w3.org/2000/09/xmldsig#}PGPDataType with content type ELEMENT_ONLY"""
    _TypeDefinition = None
    _ContentTypeTag = pyxb.binding.basis.complexTypeDefinition._CT_ELEMENT_ONLY
    _Abstract = False
    _ExpandedName = pyxb.namespace.ExpandedName(Namespace, 'PGPDataType')
    _XSDLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/ds.xsd', 209, 0)
    _ElementMap = {}
    _AttributeMap = {}
    # Base type is pyxb.binding.datatypes.anyType
    
    # Element {http://www.w3.org/2000/09/xmldsig#}PGPKeyID uses Python identifier PGPKeyID
    __PGPKeyID = pyxb.binding.content.ElementDeclaration(pyxb.namespace.ExpandedName(Namespace, 'PGPKeyID'), 'PGPKeyID', '__httpwww_w3_org200009xmldsig_PGPDataType_httpwww_w3_org200009xmldsigPGPKeyID', False, pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/ds.xsd', 212, 6), )

    
    PGPKeyID = property(__PGPKeyID.value, __PGPKeyID.set, None, None)

    
    # Element {http://www.w3.org/2000/09/xmldsig#}PGPKeyPacket uses Python identifier PGPKeyPacket
    __PGPKeyPacket = pyxb.binding.content.ElementDeclaration(pyxb.namespace.ExpandedName(Namespace, 'PGPKeyPacket'), 'PGPKeyPacket', '__httpwww_w3_org200009xmldsig_PGPDataType_httpwww_w3_org200009xmldsigPGPKeyPacket', False, pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/ds.xsd', 213, 6), )

    
    PGPKeyPacket = property(__PGPKeyPacket.value, __PGPKeyPacket.set, None, None)

    _HasWildcardElement = True
    _ElementMap.update({
        __PGPKeyID.name() : __PGPKeyID,
        __PGPKeyPacket.name() : __PGPKeyPacket
    })
    _AttributeMap.update({
        
    })
Namespace.addCategoryObject('typeBinding', 'PGPDataType', PGPDataType)


# Complex type {http://www.w3.org/2000/09/xmldsig#}SPKIDataType with content type ELEMENT_ONLY
class SPKIDataType (pyxb.binding.basis.complexTypeDefinition):
    """Complex type {http://www.w3.org/2000/09/xmldsig#}SPKIDataType with content type ELEMENT_ONLY"""
    _TypeDefinition = None
    _ContentTypeTag = pyxb.binding.basis.complexTypeDefinition._CT_ELEMENT_ONLY
    _Abstract = False
    _ExpandedName = pyxb.namespace.ExpandedName(Namespace, 'SPKIDataType')
    _XSDLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/ds.xsd', 230, 0)
    _ElementMap = {}
    _AttributeMap = {}
    # Base type is pyxb.binding.datatypes.anyType
    
    # Element {http://www.w3.org/2000/09/xmldsig#}SPKISexp uses Python identifier SPKISexp
    __SPKISexp = pyxb.binding.content.ElementDeclaration(pyxb.namespace.ExpandedName(Namespace, 'SPKISexp'), 'SPKISexp', '__httpwww_w3_org200009xmldsig_SPKIDataType_httpwww_w3_org200009xmldsigSPKISexp', True, pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/ds.xsd', 232, 4), )

    
    SPKISexp = property(__SPKISexp.value, __SPKISexp.set, None, None)

    _HasWildcardElement = True
    _ElementMap.update({
        __SPKISexp.name() : __SPKISexp
    })
    _AttributeMap.update({
        
    })
Namespace.addCategoryObject('typeBinding', 'SPKIDataType', SPKIDataType)


# Complex type {http://www.w3.org/2000/09/xmldsig#}ObjectType with content type MIXED
class ObjectType (pyxb.binding.basis.complexTypeDefinition):
    """Complex type {http://www.w3.org/2000/09/xmldsig#}ObjectType with content type MIXED"""
    _TypeDefinition = None
    _ContentTypeTag = pyxb.binding.basis.complexTypeDefinition._CT_MIXED
    _Abstract = False
    _ExpandedName = pyxb.namespace.ExpandedName(Namespace, 'ObjectType')
    _XSDLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/ds.xsd', 244, 0)
    _ElementMap = {}
    _AttributeMap = {}
    # Base type is pyxb.binding.datatypes.anyType
    
    # Attribute Id uses Python identifier Id
    __Id = pyxb.binding.content.AttributeUse(pyxb.namespace.ExpandedName(None, 'Id'), 'Id', '__httpwww_w3_org200009xmldsig_ObjectType_Id', pyxb.binding.datatypes.ID)
    __Id._DeclarationLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/ds.xsd', 248, 2)
    __Id._UseLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/ds.xsd', 248, 2)
    
    Id = property(__Id.value, __Id.set, None, None)

    
    # Attribute MimeType uses Python identifier MimeType
    __MimeType = pyxb.binding.content.AttributeUse(pyxb.namespace.ExpandedName(None, 'MimeType'), 'MimeType', '__httpwww_w3_org200009xmldsig_ObjectType_MimeType', pyxb.binding.datatypes.string)
    __MimeType._DeclarationLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/ds.xsd', 249, 2)
    __MimeType._UseLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/ds.xsd', 249, 2)
    
    MimeType = property(__MimeType.value, __MimeType.set, None, None)

    
    # Attribute Encoding uses Python identifier Encoding
    __Encoding = pyxb.binding.content.AttributeUse(pyxb.namespace.ExpandedName(None, 'Encoding'), 'Encoding', '__httpwww_w3_org200009xmldsig_ObjectType_Encoding', pyxb.binding.datatypes.anyURI)
    __Encoding._DeclarationLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/ds.xsd', 250, 2)
    __Encoding._UseLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/ds.xsd', 250, 2)
    
    Encoding = property(__Encoding.value, __Encoding.set, None, None)

    _HasWildcardElement = True
    _ElementMap.update({
        
    })
    _AttributeMap.update({
        __Id.name() : __Id,
        __MimeType.name() : __MimeType,
        __Encoding.name() : __Encoding
    })
Namespace.addCategoryObject('typeBinding', 'ObjectType', ObjectType)


# Complex type {http://www.w3.org/2000/09/xmldsig#}ManifestType with content type ELEMENT_ONLY
class ManifestType (pyxb.binding.basis.complexTypeDefinition):
    """Complex type {http://www.w3.org/2000/09/xmldsig#}ManifestType with content type ELEMENT_ONLY"""
    _TypeDefinition = None
    _ContentTypeTag = pyxb.binding.basis.complexTypeDefinition._CT_ELEMENT_ONLY
    _Abstract = False
    _ExpandedName = pyxb.namespace.ExpandedName(Namespace, 'ManifestType')
    _XSDLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/ds.xsd', 254, 0)
    _ElementMap = {}
    _AttributeMap = {}
    # Base type is pyxb.binding.datatypes.anyType
    
    # Element {http://www.w3.org/2000/09/xmldsig#}Reference uses Python identifier Reference
    __Reference = pyxb.binding.content.ElementDeclaration(pyxb.namespace.ExpandedName(Namespace, 'Reference'), 'Reference', '__httpwww_w3_org200009xmldsig_ManifestType_httpwww_w3_org200009xmldsigReference', True, pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/ds.xsd', 96, 0), )

    
    Reference = property(__Reference.value, __Reference.set, None, None)

    
    # Attribute Id uses Python identifier Id
    __Id = pyxb.binding.content.AttributeUse(pyxb.namespace.ExpandedName(None, 'Id'), 'Id', '__httpwww_w3_org200009xmldsig_ManifestType_Id', pyxb.binding.datatypes.ID)
    __Id._DeclarationLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/ds.xsd', 258, 2)
    __Id._UseLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/ds.xsd', 258, 2)
    
    Id = property(__Id.value, __Id.set, None, None)

    _ElementMap.update({
        __Reference.name() : __Reference
    })
    _AttributeMap.update({
        __Id.name() : __Id
    })
Namespace.addCategoryObject('typeBinding', 'ManifestType', ManifestType)


# Complex type {http://www.w3.org/2000/09/xmldsig#}SignaturePropertiesType with content type ELEMENT_ONLY
class SignaturePropertiesType (pyxb.binding.basis.complexTypeDefinition):
    """Complex type {http://www.w3.org/2000/09/xmldsig#}SignaturePropertiesType with content type ELEMENT_ONLY"""
    _TypeDefinition = None
    _ContentTypeTag = pyxb.binding.basis.complexTypeDefinition._CT_ELEMENT_ONLY
    _Abstract = False
    _ExpandedName = pyxb.namespace.ExpandedName(Namespace, 'SignaturePropertiesType')
    _XSDLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/ds.xsd', 262, 0)
    _ElementMap = {}
    _AttributeMap = {}
    # Base type is pyxb.binding.datatypes.anyType
    
    # Element {http://www.w3.org/2000/09/xmldsig#}SignatureProperty uses Python identifier SignatureProperty
    __SignatureProperty = pyxb.binding.content.ElementDeclaration(pyxb.namespace.ExpandedName(Namespace, 'SignatureProperty'), 'SignatureProperty', '__httpwww_w3_org200009xmldsig_SignaturePropertiesType_httpwww_w3_org200009xmldsigSignatureProperty', True, pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/ds.xsd', 269, 3), )

    
    SignatureProperty = property(__SignatureProperty.value, __SignatureProperty.set, None, None)

    
    # Attribute Id uses Python identifier Id
    __Id = pyxb.binding.content.AttributeUse(pyxb.namespace.ExpandedName(None, 'Id'), 'Id', '__httpwww_w3_org200009xmldsig_SignaturePropertiesType_Id', pyxb.binding.datatypes.ID)
    __Id._DeclarationLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/ds.xsd', 266, 2)
    __Id._UseLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/ds.xsd', 266, 2)
    
    Id = property(__Id.value, __Id.set, None, None)

    _ElementMap.update({
        __SignatureProperty.name() : __SignatureProperty
    })
    _AttributeMap.update({
        __Id.name() : __Id
    })
Namespace.addCategoryObject('typeBinding', 'SignaturePropertiesType', SignaturePropertiesType)


# Complex type {http://www.w3.org/2000/09/xmldsig#}SignaturePropertyType with content type MIXED
class SignaturePropertyType (pyxb.binding.basis.complexTypeDefinition):
    """Complex type {http://www.w3.org/2000/09/xmldsig#}SignaturePropertyType with content type MIXED"""
    _TypeDefinition = None
    _ContentTypeTag = pyxb.binding.basis.complexTypeDefinition._CT_MIXED
    _Abstract = False
    _ExpandedName = pyxb.namespace.ExpandedName(Namespace, 'SignaturePropertyType')
    _XSDLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/ds.xsd', 270, 3)
    _ElementMap = {}
    _AttributeMap = {}
    # Base type is pyxb.binding.datatypes.anyType
    
    # Attribute Target uses Python identifier Target
    __Target = pyxb.binding.content.AttributeUse(pyxb.namespace.ExpandedName(None, 'Target'), 'Target', '__httpwww_w3_org200009xmldsig_SignaturePropertyType_Target', pyxb.binding.datatypes.anyURI, required=True)
    __Target._DeclarationLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/ds.xsd', 275, 5)
    __Target._UseLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/ds.xsd', 275, 5)
    
    Target = property(__Target.value, __Target.set, None, None)

    
    # Attribute Id uses Python identifier Id
    __Id = pyxb.binding.content.AttributeUse(pyxb.namespace.ExpandedName(None, 'Id'), 'Id', '__httpwww_w3_org200009xmldsig_SignaturePropertyType_Id', pyxb.binding.datatypes.ID)
    __Id._DeclarationLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/ds.xsd', 276, 5)
    __Id._UseLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/ds.xsd', 276, 5)
    
    Id = property(__Id.value, __Id.set, None, None)

    _HasWildcardElement = True
    _ElementMap.update({
        
    })
    _AttributeMap.update({
        __Target.name() : __Target,
        __Id.name() : __Id
    })
Namespace.addCategoryObject('typeBinding', 'SignaturePropertyType', SignaturePropertyType)


# Complex type {http://www.w3.org/2000/09/xmldsig#}DSAKeyValueType with content type ELEMENT_ONLY
class DSAKeyValueType (pyxb.binding.basis.complexTypeDefinition):
    """Complex type {http://www.w3.org/2000/09/xmldsig#}DSAKeyValueType with content type ELEMENT_ONLY"""
    _TypeDefinition = None
    _ContentTypeTag = pyxb.binding.basis.complexTypeDefinition._CT_ELEMENT_ONLY
    _Abstract = False
    _ExpandedName = pyxb.namespace.ExpandedName(Namespace, 'DSAKeyValueType')
    _XSDLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/ds.xsd', 290, 0)
    _ElementMap = {}
    _AttributeMap = {}
    # Base type is pyxb.binding.datatypes.anyType
    
    # Element {http://www.w3.org/2000/09/xmldsig#}P uses Python identifier P
    __P = pyxb.binding.content.ElementDeclaration(pyxb.namespace.ExpandedName(Namespace, 'P'), 'P', '__httpwww_w3_org200009xmldsig_DSAKeyValueType_httpwww_w3_org200009xmldsigP', False, pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/ds.xsd', 293, 6), )

    
    P = property(__P.value, __P.set, None, None)

    
    # Element {http://www.w3.org/2000/09/xmldsig#}Q uses Python identifier Q
    __Q = pyxb.binding.content.ElementDeclaration(pyxb.namespace.ExpandedName(Namespace, 'Q'), 'Q', '__httpwww_w3_org200009xmldsig_DSAKeyValueType_httpwww_w3_org200009xmldsigQ', False, pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/ds.xsd', 294, 6), )

    
    Q = property(__Q.value, __Q.set, None, None)

    
    # Element {http://www.w3.org/2000/09/xmldsig#}G uses Python identifier G
    __G = pyxb.binding.content.ElementDeclaration(pyxb.namespace.ExpandedName(Namespace, 'G'), 'G', '__httpwww_w3_org200009xmldsig_DSAKeyValueType_httpwww_w3_org200009xmldsigG', False, pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/ds.xsd', 296, 4), )

    
    G = property(__G.value, __G.set, None, None)

    
    # Element {http://www.w3.org/2000/09/xmldsig#}Y uses Python identifier Y
    __Y = pyxb.binding.content.ElementDeclaration(pyxb.namespace.ExpandedName(Namespace, 'Y'), 'Y', '__httpwww_w3_org200009xmldsig_DSAKeyValueType_httpwww_w3_org200009xmldsigY', False, pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/ds.xsd', 297, 4), )

    
    Y = property(__Y.value, __Y.set, None, None)

    
    # Element {http://www.w3.org/2000/09/xmldsig#}J uses Python identifier J
    __J = pyxb.binding.content.ElementDeclaration(pyxb.namespace.ExpandedName(Namespace, 'J'), 'J', '__httpwww_w3_org200009xmldsig_DSAKeyValueType_httpwww_w3_org200009xmldsigJ', False, pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/ds.xsd', 298, 4), )

    
    J = property(__J.value, __J.set, None, None)

    
    # Element {http://www.w3.org/2000/09/xmldsig#}Seed uses Python identifier Seed
    __Seed = pyxb.binding.content.ElementDeclaration(pyxb.namespace.ExpandedName(Namespace, 'Seed'), 'Seed', '__httpwww_w3_org200009xmldsig_DSAKeyValueType_httpwww_w3_org200009xmldsigSeed', False, pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/ds.xsd', 300, 6), )

    
    Seed = property(__Seed.value, __Seed.set, None, None)

    
    # Element {http://www.w3.org/2000/09/xmldsig#}PgenCounter uses Python identifier PgenCounter
    __PgenCounter = pyxb.binding.content.ElementDeclaration(pyxb.namespace.ExpandedName(Namespace, 'PgenCounter'), 'PgenCounter', '__httpwww_w3_org200009xmldsig_DSAKeyValueType_httpwww_w3_org200009xmldsigPgenCounter', False, pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/ds.xsd', 301, 6), )

    
    PgenCounter = property(__PgenCounter.value, __PgenCounter.set, None, None)

    _ElementMap.update({
        __P.name() : __P,
        __Q.name() : __Q,
        __G.name() : __G,
        __Y.name() : __Y,
        __J.name() : __J,
        __Seed.name() : __Seed,
        __PgenCounter.name() : __PgenCounter
    })
    _AttributeMap.update({
        
    })
Namespace.addCategoryObject('typeBinding', 'DSAKeyValueType', DSAKeyValueType)


# Complex type {http://www.w3.org/2000/09/xmldsig#}RSAKeyValueType with content type ELEMENT_ONLY
class RSAKeyValueType (pyxb.binding.basis.complexTypeDefinition):
    """Complex type {http://www.w3.org/2000/09/xmldsig#}RSAKeyValueType with content type ELEMENT_ONLY"""
    _TypeDefinition = None
    _ContentTypeTag = pyxb.binding.basis.complexTypeDefinition._CT_ELEMENT_ONLY
    _Abstract = False
    _ExpandedName = pyxb.namespace.ExpandedName(Namespace, 'RSAKeyValueType')
    _XSDLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/ds.xsd', 307, 0)
    _ElementMap = {}
    _AttributeMap = {}
    # Base type is pyxb.binding.datatypes.anyType
    
    # Element {http://www.w3.org/2000/09/xmldsig#}Modulus uses Python identifier Modulus
    __Modulus = pyxb.binding.content.ElementDeclaration(pyxb.namespace.ExpandedName(Namespace, 'Modulus'), 'Modulus', '__httpwww_w3_org200009xmldsig_RSAKeyValueType_httpwww_w3_org200009xmldsigModulus', False, pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/ds.xsd', 309, 4), )

    
    Modulus = property(__Modulus.value, __Modulus.set, None, None)

    
    # Element {http://www.w3.org/2000/09/xmldsig#}Exponent uses Python identifier Exponent
    __Exponent = pyxb.binding.content.ElementDeclaration(pyxb.namespace.ExpandedName(Namespace, 'Exponent'), 'Exponent', '__httpwww_w3_org200009xmldsig_RSAKeyValueType_httpwww_w3_org200009xmldsigExponent', False, pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/ds.xsd', 310, 4), )

    
    Exponent = property(__Exponent.value, __Exponent.set, None, None)

    _ElementMap.update({
        __Modulus.name() : __Modulus,
        __Exponent.name() : __Exponent
    })
    _AttributeMap.update({
        
    })
Namespace.addCategoryObject('typeBinding', 'RSAKeyValueType', RSAKeyValueType)


KeyName = pyxb.binding.basis.element(pyxb.namespace.ExpandedName(Namespace, 'KeyName'), pyxb.binding.datatypes.string, location=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/ds.xsd', 160, 2))
Namespace.addCategoryObject('elementBinding', KeyName.name().localName(), KeyName)

MgmtData = pyxb.binding.basis.element(pyxb.namespace.ExpandedName(Namespace, 'MgmtData'), pyxb.binding.datatypes.string, location=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/ds.xsd', 161, 2))
Namespace.addCategoryObject('elementBinding', MgmtData.name().localName(), MgmtData)

Signature = pyxb.binding.basis.element(pyxb.namespace.ExpandedName(Namespace, 'Signature'), SignatureType, location=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/ds.xsd', 43, 0))
Namespace.addCategoryObject('elementBinding', Signature.name().localName(), Signature)

SignatureValue = pyxb.binding.basis.element(pyxb.namespace.ExpandedName(Namespace, 'SignatureValue'), SignatureValueType, location=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/ds.xsd', 54, 2))
Namespace.addCategoryObject('elementBinding', SignatureValue.name().localName(), SignatureValue)

SignedInfo = pyxb.binding.basis.element(pyxb.namespace.ExpandedName(Namespace, 'SignedInfo'), SignedInfoType, location=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/ds.xsd', 65, 0))
Namespace.addCategoryObject('elementBinding', SignedInfo.name().localName(), SignedInfo)

CanonicalizationMethod = pyxb.binding.basis.element(pyxb.namespace.ExpandedName(Namespace, 'CanonicalizationMethod'), CanonicalizationMethodType, location=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/ds.xsd', 75, 2))
Namespace.addCategoryObject('elementBinding', CanonicalizationMethod.name().localName(), CanonicalizationMethod)

SignatureMethod = pyxb.binding.basis.element(pyxb.namespace.ExpandedName(Namespace, 'SignatureMethod'), SignatureMethodType, location=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/ds.xsd', 84, 2))
Namespace.addCategoryObject('elementBinding', SignatureMethod.name().localName(), SignatureMethod)

Reference = pyxb.binding.basis.element(pyxb.namespace.ExpandedName(Namespace, 'Reference'), ReferenceType, location=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/ds.xsd', 96, 0))
Namespace.addCategoryObject('elementBinding', Reference.name().localName(), Reference)

Transforms = pyxb.binding.basis.element(pyxb.namespace.ExpandedName(Namespace, 'Transforms'), TransformsType, location=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/ds.xsd', 108, 2))
Namespace.addCategoryObject('elementBinding', Transforms.name().localName(), Transforms)

Transform = pyxb.binding.basis.element(pyxb.namespace.ExpandedName(Namespace, 'Transform'), TransformType, location=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/ds.xsd', 115, 2))
Namespace.addCategoryObject('elementBinding', Transform.name().localName(), Transform)

DigestMethod = pyxb.binding.basis.element(pyxb.namespace.ExpandedName(Namespace, 'DigestMethod'), DigestMethodType, location=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/ds.xsd', 127, 0))
Namespace.addCategoryObject('elementBinding', DigestMethod.name().localName(), DigestMethod)

DigestValue = pyxb.binding.basis.element(pyxb.namespace.ExpandedName(Namespace, 'DigestValue'), DigestValueType, location=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/ds.xsd', 135, 0))
Namespace.addCategoryObject('elementBinding', DigestValue.name().localName(), DigestValue)

KeyInfo = pyxb.binding.basis.element(pyxb.namespace.ExpandedName(Namespace, 'KeyInfo'), KeyInfoType, location=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/ds.xsd', 144, 0))
Namespace.addCategoryObject('elementBinding', KeyInfo.name().localName(), KeyInfo)

KeyValue = pyxb.binding.basis.element(pyxb.namespace.ExpandedName(Namespace, 'KeyValue'), KeyValueType, location=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/ds.xsd', 163, 2))
Namespace.addCategoryObject('elementBinding', KeyValue.name().localName(), KeyValue)

RetrievalMethod = pyxb.binding.basis.element(pyxb.namespace.ExpandedName(Namespace, 'RetrievalMethod'), RetrievalMethodType, location=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/ds.xsd', 172, 2))
Namespace.addCategoryObject('elementBinding', RetrievalMethod.name().localName(), RetrievalMethod)

X509Data = pyxb.binding.basis.element(pyxb.namespace.ExpandedName(Namespace, 'X509Data'), X509DataType, location=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/ds.xsd', 183, 0))
Namespace.addCategoryObject('elementBinding', X509Data.name().localName(), X509Data)

PGPData = pyxb.binding.basis.element(pyxb.namespace.ExpandedName(Namespace, 'PGPData'), PGPDataType, location=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/ds.xsd', 208, 0))
Namespace.addCategoryObject('elementBinding', PGPData.name().localName(), PGPData)

SPKIData = pyxb.binding.basis.element(pyxb.namespace.ExpandedName(Namespace, 'SPKIData'), SPKIDataType, location=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/ds.xsd', 229, 0))
Namespace.addCategoryObject('elementBinding', SPKIData.name().localName(), SPKIData)

Object = pyxb.binding.basis.element(pyxb.namespace.ExpandedName(Namespace, 'Object'), ObjectType, location=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/ds.xsd', 243, 0))
Namespace.addCategoryObject('elementBinding', Object.name().localName(), Object)

Manifest = pyxb.binding.basis.element(pyxb.namespace.ExpandedName(Namespace, 'Manifest'), ManifestType, location=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/ds.xsd', 253, 0))
Namespace.addCategoryObject('elementBinding', Manifest.name().localName(), Manifest)

SignatureProperties = pyxb.binding.basis.element(pyxb.namespace.ExpandedName(Namespace, 'SignatureProperties'), SignaturePropertiesType, location=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/ds.xsd', 261, 0))
Namespace.addCategoryObject('elementBinding', SignatureProperties.name().localName(), SignatureProperties)

SignatureProperty = pyxb.binding.basis.element(pyxb.namespace.ExpandedName(Namespace, 'SignatureProperty'), SignaturePropertyType, location=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/ds.xsd', 269, 3))
Namespace.addCategoryObject('elementBinding', SignatureProperty.name().localName(), SignatureProperty)

DSAKeyValue = pyxb.binding.basis.element(pyxb.namespace.ExpandedName(Namespace, 'DSAKeyValue'), DSAKeyValueType, location=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/ds.xsd', 289, 0))
Namespace.addCategoryObject('elementBinding', DSAKeyValue.name().localName(), DSAKeyValue)

RSAKeyValue = pyxb.binding.basis.element(pyxb.namespace.ExpandedName(Namespace, 'RSAKeyValue'), RSAKeyValueType, location=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/ds.xsd', 306, 0))
Namespace.addCategoryObject('elementBinding', RSAKeyValue.name().localName(), RSAKeyValue)



SignatureType._AddElement(pyxb.binding.basis.element(pyxb.namespace.ExpandedName(Namespace, 'SignatureValue'), SignatureValueType, scope=SignatureType, location=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/ds.xsd', 54, 2)))

SignatureType._AddElement(pyxb.binding.basis.element(pyxb.namespace.ExpandedName(Namespace, 'SignedInfo'), SignedInfoType, scope=SignatureType, location=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/ds.xsd', 65, 0)))

SignatureType._AddElement(pyxb.binding.basis.element(pyxb.namespace.ExpandedName(Namespace, 'KeyInfo'), KeyInfoType, scope=SignatureType, location=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/ds.xsd', 144, 0)))

SignatureType._AddElement(pyxb.binding.basis.element(pyxb.namespace.ExpandedName(Namespace, 'Object'), ObjectType, scope=SignatureType, location=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/ds.xsd', 243, 0)))

def _BuildAutomaton ():
    # Remove this helper function from the namespace after it is invoked
    global _BuildAutomaton
    del _BuildAutomaton
    import pyxb.utils.fac as fac

    counters = set()
    cc_0 = fac.CounterCondition(min=0, max=1, metadata=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/ds.xsd', 48, 4))
    counters.add(cc_0)
    cc_1 = fac.CounterCondition(min=0, max=None, metadata=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/ds.xsd', 49, 4))
    counters.add(cc_1)
    states = []
    final_update = None
    symbol = pyxb.binding.content.ElementUse(SignatureType._UseForTag(pyxb.namespace.ExpandedName(Namespace, 'SignedInfo')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/ds.xsd', 46, 4))
    st_0 = fac.State(symbol, is_initial=True, final_update=final_update, is_unordered_catenation=False)
    states.append(st_0)
    final_update = set()
    symbol = pyxb.binding.content.ElementUse(SignatureType._UseForTag(pyxb.namespace.ExpandedName(Namespace, 'SignatureValue')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/ds.xsd', 47, 4))
    st_1 = fac.State(symbol, is_initial=False, final_update=final_update, is_unordered_catenation=False)
    states.append(st_1)
    final_update = set()
    final_update.add(fac.UpdateInstruction(cc_0, False))
    symbol = pyxb.binding.content.ElementUse(SignatureType._UseForTag(pyxb.namespace.ExpandedName(Namespace, 'KeyInfo')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/ds.xsd', 48, 4))
    st_2 = fac.State(symbol, is_initial=False, final_update=final_update, is_unordered_catenation=False)
    states.append(st_2)
    final_update = set()
    final_update.add(fac.UpdateInstruction(cc_1, False))
    symbol = pyxb.binding.content.ElementUse(SignatureType._UseForTag(pyxb.namespace.ExpandedName(Namespace, 'Object')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/ds.xsd', 49, 4))
    st_3 = fac.State(symbol, is_initial=False, final_update=final_update, is_unordered_catenation=False)
    states.append(st_3)
    transitions = []
    transitions.append(fac.Transition(st_1, [
         ]))
    st_0._set_transitionSet(transitions)
    transitions = []
    transitions.append(fac.Transition(st_2, [
         ]))
    transitions.append(fac.Transition(st_3, [
         ]))
    st_1._set_transitionSet(transitions)
    transitions = []
    transitions.append(fac.Transition(st_2, [
        fac.UpdateInstruction(cc_0, True) ]))
    transitions.append(fac.Transition(st_3, [
        fac.UpdateInstruction(cc_0, False) ]))
    st_2._set_transitionSet(transitions)
    transitions = []
    transitions.append(fac.Transition(st_3, [
        fac.UpdateInstruction(cc_1, True) ]))
    st_3._set_transitionSet(transitions)
    return fac.Automaton(states, counters, False, containing_state=None)
SignatureType._Automaton = _BuildAutomaton()




SignedInfoType._AddElement(pyxb.binding.basis.element(pyxb.namespace.ExpandedName(Namespace, 'CanonicalizationMethod'), CanonicalizationMethodType, scope=SignedInfoType, location=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/ds.xsd', 75, 2)))

SignedInfoType._AddElement(pyxb.binding.basis.element(pyxb.namespace.ExpandedName(Namespace, 'SignatureMethod'), SignatureMethodType, scope=SignedInfoType, location=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/ds.xsd', 84, 2)))

SignedInfoType._AddElement(pyxb.binding.basis.element(pyxb.namespace.ExpandedName(Namespace, 'Reference'), ReferenceType, scope=SignedInfoType, location=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/ds.xsd', 96, 0)))

def _BuildAutomaton_ ():
    # Remove this helper function from the namespace after it is invoked
    global _BuildAutomaton_
    del _BuildAutomaton_
    import pyxb.utils.fac as fac

    counters = set()
    states = []
    final_update = None
    symbol = pyxb.binding.content.ElementUse(SignedInfoType._UseForTag(pyxb.namespace.ExpandedName(Namespace, 'CanonicalizationMethod')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/ds.xsd', 68, 4))
    st_0 = fac.State(symbol, is_initial=True, final_update=final_update, is_unordered_catenation=False)
    states.append(st_0)
    final_update = None
    symbol = pyxb.binding.content.ElementUse(SignedInfoType._UseForTag(pyxb.namespace.ExpandedName(Namespace, 'SignatureMethod')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/ds.xsd', 69, 4))
    st_1 = fac.State(symbol, is_initial=False, final_update=final_update, is_unordered_catenation=False)
    states.append(st_1)
    final_update = set()
    symbol = pyxb.binding.content.ElementUse(SignedInfoType._UseForTag(pyxb.namespace.ExpandedName(Namespace, 'Reference')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/ds.xsd', 70, 4))
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
         ]))
    st_2._set_transitionSet(transitions)
    return fac.Automaton(states, counters, False, containing_state=None)
SignedInfoType._Automaton = _BuildAutomaton_()




def _BuildAutomaton_2 ():
    # Remove this helper function from the namespace after it is invoked
    global _BuildAutomaton_2
    del _BuildAutomaton_2
    import pyxb.utils.fac as fac

    counters = set()
    cc_0 = fac.CounterCondition(min=0, max=None, metadata=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/ds.xsd', 78, 6))
    counters.add(cc_0)
    states = []
    final_update = set()
    final_update.add(fac.UpdateInstruction(cc_0, False))
    symbol = pyxb.binding.content.WildcardUse(pyxb.binding.content.Wildcard(process_contents=pyxb.binding.content.Wildcard.PC_strict, namespace_constraint=pyxb.binding.content.Wildcard.NC_any), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/ds.xsd', 78, 6))
    st_0 = fac.State(symbol, is_initial=True, final_update=final_update, is_unordered_catenation=False)
    states.append(st_0)
    transitions = []
    transitions.append(fac.Transition(st_0, [
        fac.UpdateInstruction(cc_0, True) ]))
    st_0._set_transitionSet(transitions)
    return fac.Automaton(states, counters, True, containing_state=None)
CanonicalizationMethodType._Automaton = _BuildAutomaton_2()




SignatureMethodType._AddElement(pyxb.binding.basis.element(pyxb.namespace.ExpandedName(Namespace, 'HMACOutputLength'), HMACOutputLengthType, scope=SignatureMethodType, location=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/ds.xsd', 87, 6)))

def _BuildAutomaton_3 ():
    # Remove this helper function from the namespace after it is invoked
    global _BuildAutomaton_3
    del _BuildAutomaton_3
    import pyxb.utils.fac as fac

    counters = set()
    cc_0 = fac.CounterCondition(min=0, max=1, metadata=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/ds.xsd', 87, 6))
    counters.add(cc_0)
    cc_1 = fac.CounterCondition(min=0, max=None, metadata=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/ds.xsd', 88, 6))
    counters.add(cc_1)
    states = []
    final_update = set()
    final_update.add(fac.UpdateInstruction(cc_0, False))
    symbol = pyxb.binding.content.ElementUse(SignatureMethodType._UseForTag(pyxb.namespace.ExpandedName(Namespace, 'HMACOutputLength')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/ds.xsd', 87, 6))
    st_0 = fac.State(symbol, is_initial=True, final_update=final_update, is_unordered_catenation=False)
    states.append(st_0)
    final_update = set()
    final_update.add(fac.UpdateInstruction(cc_1, False))
    symbol = pyxb.binding.content.WildcardUse(pyxb.binding.content.Wildcard(process_contents=pyxb.binding.content.Wildcard.PC_strict, namespace_constraint=(pyxb.binding.content.Wildcard.NC_not, 'http://www.w3.org/2000/09/xmldsig#')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/ds.xsd', 88, 6))
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
SignatureMethodType._Automaton = _BuildAutomaton_3()




ReferenceType._AddElement(pyxb.binding.basis.element(pyxb.namespace.ExpandedName(Namespace, 'Transforms'), TransformsType, scope=ReferenceType, location=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/ds.xsd', 108, 2)))

ReferenceType._AddElement(pyxb.binding.basis.element(pyxb.namespace.ExpandedName(Namespace, 'DigestMethod'), DigestMethodType, scope=ReferenceType, location=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/ds.xsd', 127, 0)))

ReferenceType._AddElement(pyxb.binding.basis.element(pyxb.namespace.ExpandedName(Namespace, 'DigestValue'), DigestValueType, scope=ReferenceType, location=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/ds.xsd', 135, 0)))

def _BuildAutomaton_4 ():
    # Remove this helper function from the namespace after it is invoked
    global _BuildAutomaton_4
    del _BuildAutomaton_4
    import pyxb.utils.fac as fac

    counters = set()
    cc_0 = fac.CounterCondition(min=0, max=1, metadata=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/ds.xsd', 99, 4))
    counters.add(cc_0)
    states = []
    final_update = None
    symbol = pyxb.binding.content.ElementUse(ReferenceType._UseForTag(pyxb.namespace.ExpandedName(Namespace, 'Transforms')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/ds.xsd', 99, 4))
    st_0 = fac.State(symbol, is_initial=True, final_update=final_update, is_unordered_catenation=False)
    states.append(st_0)
    final_update = None
    symbol = pyxb.binding.content.ElementUse(ReferenceType._UseForTag(pyxb.namespace.ExpandedName(Namespace, 'DigestMethod')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/ds.xsd', 100, 4))
    st_1 = fac.State(symbol, is_initial=True, final_update=final_update, is_unordered_catenation=False)
    states.append(st_1)
    final_update = set()
    symbol = pyxb.binding.content.ElementUse(ReferenceType._UseForTag(pyxb.namespace.ExpandedName(Namespace, 'DigestValue')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/ds.xsd', 101, 4))
    st_2 = fac.State(symbol, is_initial=False, final_update=final_update, is_unordered_catenation=False)
    states.append(st_2)
    transitions = []
    transitions.append(fac.Transition(st_0, [
        fac.UpdateInstruction(cc_0, True) ]))
    transitions.append(fac.Transition(st_1, [
        fac.UpdateInstruction(cc_0, False) ]))
    st_0._set_transitionSet(transitions)
    transitions = []
    transitions.append(fac.Transition(st_2, [
         ]))
    st_1._set_transitionSet(transitions)
    transitions = []
    st_2._set_transitionSet(transitions)
    return fac.Automaton(states, counters, False, containing_state=None)
ReferenceType._Automaton = _BuildAutomaton_4()




TransformsType._AddElement(pyxb.binding.basis.element(pyxb.namespace.ExpandedName(Namespace, 'Transform'), TransformType, scope=TransformsType, location=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/ds.xsd', 115, 2)))

def _BuildAutomaton_5 ():
    # Remove this helper function from the namespace after it is invoked
    global _BuildAutomaton_5
    del _BuildAutomaton_5
    import pyxb.utils.fac as fac

    counters = set()
    states = []
    final_update = set()
    symbol = pyxb.binding.content.ElementUse(TransformsType._UseForTag(pyxb.namespace.ExpandedName(Namespace, 'Transform')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/ds.xsd', 111, 6))
    st_0 = fac.State(symbol, is_initial=True, final_update=final_update, is_unordered_catenation=False)
    states.append(st_0)
    transitions = []
    transitions.append(fac.Transition(st_0, [
         ]))
    st_0._set_transitionSet(transitions)
    return fac.Automaton(states, counters, False, containing_state=None)
TransformsType._Automaton = _BuildAutomaton_5()




TransformType._AddElement(pyxb.binding.basis.element(pyxb.namespace.ExpandedName(Namespace, 'XPath'), pyxb.binding.datatypes.string, scope=TransformType, location=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/ds.xsd', 120, 6)))

def _BuildAutomaton_6 ():
    # Remove this helper function from the namespace after it is invoked
    global _BuildAutomaton_6
    del _BuildAutomaton_6
    import pyxb.utils.fac as fac

    counters = set()
    cc_0 = fac.CounterCondition(min=0, max=None, metadata=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/ds.xsd', 117, 4))
    counters.add(cc_0)
    states = []
    final_update = set()
    final_update.add(fac.UpdateInstruction(cc_0, False))
    symbol = pyxb.binding.content.WildcardUse(pyxb.binding.content.Wildcard(process_contents=pyxb.binding.content.Wildcard.PC_lax, namespace_constraint=(pyxb.binding.content.Wildcard.NC_not, 'http://www.w3.org/2000/09/xmldsig#')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/ds.xsd', 118, 6))
    st_0 = fac.State(symbol, is_initial=True, final_update=final_update, is_unordered_catenation=False)
    states.append(st_0)
    final_update = set()
    final_update.add(fac.UpdateInstruction(cc_0, False))
    symbol = pyxb.binding.content.ElementUse(TransformType._UseForTag(pyxb.namespace.ExpandedName(Namespace, 'XPath')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/ds.xsd', 120, 6))
    st_1 = fac.State(symbol, is_initial=True, final_update=final_update, is_unordered_catenation=False)
    states.append(st_1)
    transitions = []
    transitions.append(fac.Transition(st_0, [
        fac.UpdateInstruction(cc_0, True) ]))
    transitions.append(fac.Transition(st_1, [
        fac.UpdateInstruction(cc_0, True) ]))
    st_0._set_transitionSet(transitions)
    transitions = []
    transitions.append(fac.Transition(st_0, [
        fac.UpdateInstruction(cc_0, True) ]))
    transitions.append(fac.Transition(st_1, [
        fac.UpdateInstruction(cc_0, True) ]))
    st_1._set_transitionSet(transitions)
    return fac.Automaton(states, counters, True, containing_state=None)
TransformType._Automaton = _BuildAutomaton_6()




def _BuildAutomaton_7 ():
    # Remove this helper function from the namespace after it is invoked
    global _BuildAutomaton_7
    del _BuildAutomaton_7
    import pyxb.utils.fac as fac

    counters = set()
    cc_0 = fac.CounterCondition(min=0, max=None, metadata=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/ds.xsd', 130, 4))
    counters.add(cc_0)
    states = []
    final_update = set()
    final_update.add(fac.UpdateInstruction(cc_0, False))
    symbol = pyxb.binding.content.WildcardUse(pyxb.binding.content.Wildcard(process_contents=pyxb.binding.content.Wildcard.PC_lax, namespace_constraint=(pyxb.binding.content.Wildcard.NC_not, 'http://www.w3.org/2000/09/xmldsig#')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/ds.xsd', 130, 4))
    st_0 = fac.State(symbol, is_initial=True, final_update=final_update, is_unordered_catenation=False)
    states.append(st_0)
    transitions = []
    transitions.append(fac.Transition(st_0, [
        fac.UpdateInstruction(cc_0, True) ]))
    st_0._set_transitionSet(transitions)
    return fac.Automaton(states, counters, True, containing_state=None)
DigestMethodType._Automaton = _BuildAutomaton_7()




KeyInfoType._AddElement(pyxb.binding.basis.element(pyxb.namespace.ExpandedName(Namespace, 'KeyName'), pyxb.binding.datatypes.string, scope=KeyInfoType, location=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/ds.xsd', 160, 2)))

KeyInfoType._AddElement(pyxb.binding.basis.element(pyxb.namespace.ExpandedName(Namespace, 'MgmtData'), pyxb.binding.datatypes.string, scope=KeyInfoType, location=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/ds.xsd', 161, 2)))

KeyInfoType._AddElement(pyxb.binding.basis.element(pyxb.namespace.ExpandedName(Namespace, 'KeyValue'), KeyValueType, scope=KeyInfoType, location=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/ds.xsd', 163, 2)))

KeyInfoType._AddElement(pyxb.binding.basis.element(pyxb.namespace.ExpandedName(Namespace, 'RetrievalMethod'), RetrievalMethodType, scope=KeyInfoType, location=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/ds.xsd', 172, 2)))

KeyInfoType._AddElement(pyxb.binding.basis.element(pyxb.namespace.ExpandedName(Namespace, 'X509Data'), X509DataType, scope=KeyInfoType, location=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/ds.xsd', 183, 0)))

KeyInfoType._AddElement(pyxb.binding.basis.element(pyxb.namespace.ExpandedName(Namespace, 'PGPData'), PGPDataType, scope=KeyInfoType, location=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/ds.xsd', 208, 0)))

KeyInfoType._AddElement(pyxb.binding.basis.element(pyxb.namespace.ExpandedName(Namespace, 'SPKIData'), SPKIDataType, scope=KeyInfoType, location=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/ds.xsd', 229, 0)))

def _BuildAutomaton_8 ():
    # Remove this helper function from the namespace after it is invoked
    global _BuildAutomaton_8
    del _BuildAutomaton_8
    import pyxb.utils.fac as fac

    counters = set()
    states = []
    final_update = set()
    symbol = pyxb.binding.content.ElementUse(KeyInfoType._UseForTag(pyxb.namespace.ExpandedName(Namespace, 'KeyName')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/ds.xsd', 147, 4))
    st_0 = fac.State(symbol, is_initial=True, final_update=final_update, is_unordered_catenation=False)
    states.append(st_0)
    final_update = set()
    symbol = pyxb.binding.content.ElementUse(KeyInfoType._UseForTag(pyxb.namespace.ExpandedName(Namespace, 'KeyValue')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/ds.xsd', 148, 4))
    st_1 = fac.State(symbol, is_initial=True, final_update=final_update, is_unordered_catenation=False)
    states.append(st_1)
    final_update = set()
    symbol = pyxb.binding.content.ElementUse(KeyInfoType._UseForTag(pyxb.namespace.ExpandedName(Namespace, 'RetrievalMethod')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/ds.xsd', 149, 4))
    st_2 = fac.State(symbol, is_initial=True, final_update=final_update, is_unordered_catenation=False)
    states.append(st_2)
    final_update = set()
    symbol = pyxb.binding.content.ElementUse(KeyInfoType._UseForTag(pyxb.namespace.ExpandedName(Namespace, 'X509Data')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/ds.xsd', 150, 4))
    st_3 = fac.State(symbol, is_initial=True, final_update=final_update, is_unordered_catenation=False)
    states.append(st_3)
    final_update = set()
    symbol = pyxb.binding.content.ElementUse(KeyInfoType._UseForTag(pyxb.namespace.ExpandedName(Namespace, 'PGPData')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/ds.xsd', 151, 4))
    st_4 = fac.State(symbol, is_initial=True, final_update=final_update, is_unordered_catenation=False)
    states.append(st_4)
    final_update = set()
    symbol = pyxb.binding.content.ElementUse(KeyInfoType._UseForTag(pyxb.namespace.ExpandedName(Namespace, 'SPKIData')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/ds.xsd', 152, 4))
    st_5 = fac.State(symbol, is_initial=True, final_update=final_update, is_unordered_catenation=False)
    states.append(st_5)
    final_update = set()
    symbol = pyxb.binding.content.ElementUse(KeyInfoType._UseForTag(pyxb.namespace.ExpandedName(Namespace, 'MgmtData')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/ds.xsd', 153, 4))
    st_6 = fac.State(symbol, is_initial=True, final_update=final_update, is_unordered_catenation=False)
    states.append(st_6)
    final_update = set()
    symbol = pyxb.binding.content.WildcardUse(pyxb.binding.content.Wildcard(process_contents=pyxb.binding.content.Wildcard.PC_lax, namespace_constraint=(pyxb.binding.content.Wildcard.NC_not, 'http://www.w3.org/2000/09/xmldsig#')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/ds.xsd', 154, 4))
    st_7 = fac.State(symbol, is_initial=True, final_update=final_update, is_unordered_catenation=False)
    states.append(st_7)
    transitions = []
    transitions.append(fac.Transition(st_0, [
         ]))
    transitions.append(fac.Transition(st_1, [
         ]))
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
    st_0._set_transitionSet(transitions)
    transitions = []
    transitions.append(fac.Transition(st_0, [
         ]))
    transitions.append(fac.Transition(st_1, [
         ]))
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
    st_1._set_transitionSet(transitions)
    transitions = []
    transitions.append(fac.Transition(st_0, [
         ]))
    transitions.append(fac.Transition(st_1, [
         ]))
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
    st_2._set_transitionSet(transitions)
    transitions = []
    transitions.append(fac.Transition(st_0, [
         ]))
    transitions.append(fac.Transition(st_1, [
         ]))
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
    st_3._set_transitionSet(transitions)
    transitions = []
    transitions.append(fac.Transition(st_0, [
         ]))
    transitions.append(fac.Transition(st_1, [
         ]))
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
    st_4._set_transitionSet(transitions)
    transitions = []
    transitions.append(fac.Transition(st_0, [
         ]))
    transitions.append(fac.Transition(st_1, [
         ]))
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
    st_5._set_transitionSet(transitions)
    transitions = []
    transitions.append(fac.Transition(st_0, [
         ]))
    transitions.append(fac.Transition(st_1, [
         ]))
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
    st_6._set_transitionSet(transitions)
    transitions = []
    transitions.append(fac.Transition(st_0, [
         ]))
    transitions.append(fac.Transition(st_1, [
         ]))
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
    st_7._set_transitionSet(transitions)
    return fac.Automaton(states, counters, False, containing_state=None)
KeyInfoType._Automaton = _BuildAutomaton_8()




KeyValueType._AddElement(pyxb.binding.basis.element(pyxb.namespace.ExpandedName(Namespace, 'DSAKeyValue'), DSAKeyValueType, scope=KeyValueType, location=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/ds.xsd', 289, 0)))

KeyValueType._AddElement(pyxb.binding.basis.element(pyxb.namespace.ExpandedName(Namespace, 'RSAKeyValue'), RSAKeyValueType, scope=KeyValueType, location=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/ds.xsd', 306, 0)))

def _BuildAutomaton_9 ():
    # Remove this helper function from the namespace after it is invoked
    global _BuildAutomaton_9
    del _BuildAutomaton_9
    import pyxb.utils.fac as fac

    counters = set()
    states = []
    final_update = set()
    symbol = pyxb.binding.content.ElementUse(KeyValueType._UseForTag(pyxb.namespace.ExpandedName(Namespace, 'DSAKeyValue')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/ds.xsd', 166, 5))
    st_0 = fac.State(symbol, is_initial=True, final_update=final_update, is_unordered_catenation=False)
    states.append(st_0)
    final_update = set()
    symbol = pyxb.binding.content.ElementUse(KeyValueType._UseForTag(pyxb.namespace.ExpandedName(Namespace, 'RSAKeyValue')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/ds.xsd', 167, 5))
    st_1 = fac.State(symbol, is_initial=True, final_update=final_update, is_unordered_catenation=False)
    states.append(st_1)
    final_update = set()
    symbol = pyxb.binding.content.WildcardUse(pyxb.binding.content.Wildcard(process_contents=pyxb.binding.content.Wildcard.PC_lax, namespace_constraint=(pyxb.binding.content.Wildcard.NC_not, 'http://www.w3.org/2000/09/xmldsig#')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/ds.xsd', 168, 5))
    st_2 = fac.State(symbol, is_initial=True, final_update=final_update, is_unordered_catenation=False)
    states.append(st_2)
    transitions = []
    st_0._set_transitionSet(transitions)
    transitions = []
    st_1._set_transitionSet(transitions)
    transitions = []
    st_2._set_transitionSet(transitions)
    return fac.Automaton(states, counters, False, containing_state=None)
KeyValueType._Automaton = _BuildAutomaton_9()




RetrievalMethodType._AddElement(pyxb.binding.basis.element(pyxb.namespace.ExpandedName(Namespace, 'Transforms'), TransformsType, scope=RetrievalMethodType, location=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/ds.xsd', 108, 2)))

def _BuildAutomaton_10 ():
    # Remove this helper function from the namespace after it is invoked
    global _BuildAutomaton_10
    del _BuildAutomaton_10
    import pyxb.utils.fac as fac

    counters = set()
    cc_0 = fac.CounterCondition(min=0, max=1, metadata=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/ds.xsd', 175, 6))
    counters.add(cc_0)
    states = []
    final_update = set()
    final_update.add(fac.UpdateInstruction(cc_0, False))
    symbol = pyxb.binding.content.ElementUse(RetrievalMethodType._UseForTag(pyxb.namespace.ExpandedName(Namespace, 'Transforms')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/ds.xsd', 175, 6))
    st_0 = fac.State(symbol, is_initial=True, final_update=final_update, is_unordered_catenation=False)
    states.append(st_0)
    transitions = []
    transitions.append(fac.Transition(st_0, [
        fac.UpdateInstruction(cc_0, True) ]))
    st_0._set_transitionSet(transitions)
    return fac.Automaton(states, counters, True, containing_state=None)
RetrievalMethodType._Automaton = _BuildAutomaton_10()




X509DataType._AddElement(pyxb.binding.basis.element(pyxb.namespace.ExpandedName(Namespace, 'X509IssuerSerial'), X509IssuerSerialType, scope=X509DataType, location=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/ds.xsd', 187, 6)))

X509DataType._AddElement(pyxb.binding.basis.element(pyxb.namespace.ExpandedName(Namespace, 'X509SKI'), pyxb.binding.datatypes.base64Binary, scope=X509DataType, location=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/ds.xsd', 188, 6)))

X509DataType._AddElement(pyxb.binding.basis.element(pyxb.namespace.ExpandedName(Namespace, 'X509SubjectName'), pyxb.binding.datatypes.string, scope=X509DataType, location=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/ds.xsd', 189, 6)))

X509DataType._AddElement(pyxb.binding.basis.element(pyxb.namespace.ExpandedName(Namespace, 'X509Certificate'), pyxb.binding.datatypes.base64Binary, scope=X509DataType, location=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/ds.xsd', 190, 6)))

X509DataType._AddElement(pyxb.binding.basis.element(pyxb.namespace.ExpandedName(Namespace, 'X509CRL'), pyxb.binding.datatypes.base64Binary, scope=X509DataType, location=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/ds.xsd', 191, 6)))

def _BuildAutomaton_11 ():
    # Remove this helper function from the namespace after it is invoked
    global _BuildAutomaton_11
    del _BuildAutomaton_11
    import pyxb.utils.fac as fac

    counters = set()
    states = []
    final_update = set()
    symbol = pyxb.binding.content.ElementUse(X509DataType._UseForTag(pyxb.namespace.ExpandedName(Namespace, 'X509IssuerSerial')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/ds.xsd', 187, 6))
    st_0 = fac.State(symbol, is_initial=True, final_update=final_update, is_unordered_catenation=False)
    states.append(st_0)
    final_update = set()
    symbol = pyxb.binding.content.ElementUse(X509DataType._UseForTag(pyxb.namespace.ExpandedName(Namespace, 'X509SKI')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/ds.xsd', 188, 6))
    st_1 = fac.State(symbol, is_initial=True, final_update=final_update, is_unordered_catenation=False)
    states.append(st_1)
    final_update = set()
    symbol = pyxb.binding.content.ElementUse(X509DataType._UseForTag(pyxb.namespace.ExpandedName(Namespace, 'X509SubjectName')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/ds.xsd', 189, 6))
    st_2 = fac.State(symbol, is_initial=True, final_update=final_update, is_unordered_catenation=False)
    states.append(st_2)
    final_update = set()
    symbol = pyxb.binding.content.ElementUse(X509DataType._UseForTag(pyxb.namespace.ExpandedName(Namespace, 'X509Certificate')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/ds.xsd', 190, 6))
    st_3 = fac.State(symbol, is_initial=True, final_update=final_update, is_unordered_catenation=False)
    states.append(st_3)
    final_update = set()
    symbol = pyxb.binding.content.ElementUse(X509DataType._UseForTag(pyxb.namespace.ExpandedName(Namespace, 'X509CRL')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/ds.xsd', 191, 6))
    st_4 = fac.State(symbol, is_initial=True, final_update=final_update, is_unordered_catenation=False)
    states.append(st_4)
    final_update = set()
    symbol = pyxb.binding.content.WildcardUse(pyxb.binding.content.Wildcard(process_contents=pyxb.binding.content.Wildcard.PC_lax, namespace_constraint=(pyxb.binding.content.Wildcard.NC_not, 'http://www.w3.org/2000/09/xmldsig#')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/ds.xsd', 192, 6))
    st_5 = fac.State(symbol, is_initial=True, final_update=final_update, is_unordered_catenation=False)
    states.append(st_5)
    transitions = []
    transitions.append(fac.Transition(st_0, [
         ]))
    transitions.append(fac.Transition(st_1, [
         ]))
    transitions.append(fac.Transition(st_2, [
         ]))
    transitions.append(fac.Transition(st_3, [
         ]))
    transitions.append(fac.Transition(st_4, [
         ]))
    transitions.append(fac.Transition(st_5, [
         ]))
    st_0._set_transitionSet(transitions)
    transitions = []
    transitions.append(fac.Transition(st_0, [
         ]))
    transitions.append(fac.Transition(st_1, [
         ]))
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
    transitions.append(fac.Transition(st_0, [
         ]))
    transitions.append(fac.Transition(st_1, [
         ]))
    transitions.append(fac.Transition(st_2, [
         ]))
    transitions.append(fac.Transition(st_3, [
         ]))
    transitions.append(fac.Transition(st_4, [
         ]))
    transitions.append(fac.Transition(st_5, [
         ]))
    st_2._set_transitionSet(transitions)
    transitions = []
    transitions.append(fac.Transition(st_0, [
         ]))
    transitions.append(fac.Transition(st_1, [
         ]))
    transitions.append(fac.Transition(st_2, [
         ]))
    transitions.append(fac.Transition(st_3, [
         ]))
    transitions.append(fac.Transition(st_4, [
         ]))
    transitions.append(fac.Transition(st_5, [
         ]))
    st_3._set_transitionSet(transitions)
    transitions = []
    transitions.append(fac.Transition(st_0, [
         ]))
    transitions.append(fac.Transition(st_1, [
         ]))
    transitions.append(fac.Transition(st_2, [
         ]))
    transitions.append(fac.Transition(st_3, [
         ]))
    transitions.append(fac.Transition(st_4, [
         ]))
    transitions.append(fac.Transition(st_5, [
         ]))
    st_4._set_transitionSet(transitions)
    transitions = []
    transitions.append(fac.Transition(st_0, [
         ]))
    transitions.append(fac.Transition(st_1, [
         ]))
    transitions.append(fac.Transition(st_2, [
         ]))
    transitions.append(fac.Transition(st_3, [
         ]))
    transitions.append(fac.Transition(st_4, [
         ]))
    transitions.append(fac.Transition(st_5, [
         ]))
    st_5._set_transitionSet(transitions)
    return fac.Automaton(states, counters, False, containing_state=None)
X509DataType._Automaton = _BuildAutomaton_11()




X509IssuerSerialType._AddElement(pyxb.binding.basis.element(pyxb.namespace.ExpandedName(Namespace, 'X509IssuerName'), pyxb.binding.datatypes.string, scope=X509IssuerSerialType, location=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/ds.xsd', 199, 4)))

X509IssuerSerialType._AddElement(pyxb.binding.basis.element(pyxb.namespace.ExpandedName(Namespace, 'X509SerialNumber'), pyxb.binding.datatypes.integer, scope=X509IssuerSerialType, location=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/ds.xsd', 200, 4)))

def _BuildAutomaton_12 ():
    # Remove this helper function from the namespace after it is invoked
    global _BuildAutomaton_12
    del _BuildAutomaton_12
    import pyxb.utils.fac as fac

    counters = set()
    states = []
    final_update = None
    symbol = pyxb.binding.content.ElementUse(X509IssuerSerialType._UseForTag(pyxb.namespace.ExpandedName(Namespace, 'X509IssuerName')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/ds.xsd', 199, 4))
    st_0 = fac.State(symbol, is_initial=True, final_update=final_update, is_unordered_catenation=False)
    states.append(st_0)
    final_update = set()
    symbol = pyxb.binding.content.ElementUse(X509IssuerSerialType._UseForTag(pyxb.namespace.ExpandedName(Namespace, 'X509SerialNumber')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/ds.xsd', 200, 4))
    st_1 = fac.State(symbol, is_initial=False, final_update=final_update, is_unordered_catenation=False)
    states.append(st_1)
    transitions = []
    transitions.append(fac.Transition(st_1, [
         ]))
    st_0._set_transitionSet(transitions)
    transitions = []
    st_1._set_transitionSet(transitions)
    return fac.Automaton(states, counters, False, containing_state=None)
X509IssuerSerialType._Automaton = _BuildAutomaton_12()




PGPDataType._AddElement(pyxb.binding.basis.element(pyxb.namespace.ExpandedName(Namespace, 'PGPKeyID'), pyxb.binding.datatypes.base64Binary, scope=PGPDataType, location=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/ds.xsd', 212, 6)))

PGPDataType._AddElement(pyxb.binding.basis.element(pyxb.namespace.ExpandedName(Namespace, 'PGPKeyPacket'), pyxb.binding.datatypes.base64Binary, scope=PGPDataType, location=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/ds.xsd', 213, 6)))

def _BuildAutomaton_13 ():
    # Remove this helper function from the namespace after it is invoked
    global _BuildAutomaton_13
    del _BuildAutomaton_13
    import pyxb.utils.fac as fac

    counters = set()
    cc_0 = fac.CounterCondition(min=0, max=1, metadata=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/ds.xsd', 213, 6))
    counters.add(cc_0)
    cc_1 = fac.CounterCondition(min=0, max=None, metadata=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/ds.xsd', 214, 6))
    counters.add(cc_1)
    cc_2 = fac.CounterCondition(min=0, max=None, metadata=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/ds.xsd', 219, 6))
    counters.add(cc_2)
    states = []
    final_update = set()
    symbol = pyxb.binding.content.ElementUse(PGPDataType._UseForTag(pyxb.namespace.ExpandedName(Namespace, 'PGPKeyID')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/ds.xsd', 212, 6))
    st_0 = fac.State(symbol, is_initial=True, final_update=final_update, is_unordered_catenation=False)
    states.append(st_0)
    final_update = set()
    final_update.add(fac.UpdateInstruction(cc_0, False))
    symbol = pyxb.binding.content.ElementUse(PGPDataType._UseForTag(pyxb.namespace.ExpandedName(Namespace, 'PGPKeyPacket')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/ds.xsd', 213, 6))
    st_1 = fac.State(symbol, is_initial=False, final_update=final_update, is_unordered_catenation=False)
    states.append(st_1)
    final_update = set()
    final_update.add(fac.UpdateInstruction(cc_1, False))
    symbol = pyxb.binding.content.WildcardUse(pyxb.binding.content.Wildcard(process_contents=pyxb.binding.content.Wildcard.PC_lax, namespace_constraint=(pyxb.binding.content.Wildcard.NC_not, 'http://www.w3.org/2000/09/xmldsig#')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/ds.xsd', 214, 6))
    st_2 = fac.State(symbol, is_initial=False, final_update=final_update, is_unordered_catenation=False)
    states.append(st_2)
    final_update = set()
    symbol = pyxb.binding.content.ElementUse(PGPDataType._UseForTag(pyxb.namespace.ExpandedName(Namespace, 'PGPKeyPacket')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/ds.xsd', 218, 6))
    st_3 = fac.State(symbol, is_initial=True, final_update=final_update, is_unordered_catenation=False)
    states.append(st_3)
    final_update = set()
    final_update.add(fac.UpdateInstruction(cc_2, False))
    symbol = pyxb.binding.content.WildcardUse(pyxb.binding.content.Wildcard(process_contents=pyxb.binding.content.Wildcard.PC_lax, namespace_constraint=(pyxb.binding.content.Wildcard.NC_not, 'http://www.w3.org/2000/09/xmldsig#')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/ds.xsd', 219, 6))
    st_4 = fac.State(symbol, is_initial=False, final_update=final_update, is_unordered_catenation=False)
    states.append(st_4)
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
    transitions = []
    transitions.append(fac.Transition(st_4, [
         ]))
    st_3._set_transitionSet(transitions)
    transitions = []
    transitions.append(fac.Transition(st_4, [
        fac.UpdateInstruction(cc_2, True) ]))
    st_4._set_transitionSet(transitions)
    return fac.Automaton(states, counters, False, containing_state=None)
PGPDataType._Automaton = _BuildAutomaton_13()




SPKIDataType._AddElement(pyxb.binding.basis.element(pyxb.namespace.ExpandedName(Namespace, 'SPKISexp'), pyxb.binding.datatypes.base64Binary, scope=SPKIDataType, location=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/ds.xsd', 232, 4)))

def _BuildAutomaton_14 ():
    # Remove this helper function from the namespace after it is invoked
    global _BuildAutomaton_14
    del _BuildAutomaton_14
    import pyxb.utils.fac as fac

    counters = set()
    cc_0 = fac.CounterCondition(min=0, max=1, metadata=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/ds.xsd', 233, 4))
    counters.add(cc_0)
    states = []
    final_update = set()
    symbol = pyxb.binding.content.ElementUse(SPKIDataType._UseForTag(pyxb.namespace.ExpandedName(Namespace, 'SPKISexp')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/ds.xsd', 232, 4))
    st_0 = fac.State(symbol, is_initial=True, final_update=final_update, is_unordered_catenation=False)
    states.append(st_0)
    final_update = set()
    final_update.add(fac.UpdateInstruction(cc_0, False))
    symbol = pyxb.binding.content.WildcardUse(pyxb.binding.content.Wildcard(process_contents=pyxb.binding.content.Wildcard.PC_lax, namespace_constraint=(pyxb.binding.content.Wildcard.NC_not, 'http://www.w3.org/2000/09/xmldsig#')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/ds.xsd', 233, 4))
    st_1 = fac.State(symbol, is_initial=False, final_update=final_update, is_unordered_catenation=False)
    states.append(st_1)
    transitions = []
    transitions.append(fac.Transition(st_0, [
         ]))
    transitions.append(fac.Transition(st_1, [
         ]))
    st_0._set_transitionSet(transitions)
    transitions = []
    transitions.append(fac.Transition(st_0, [
        fac.UpdateInstruction(cc_0, False) ]))
    transitions.append(fac.Transition(st_1, [
        fac.UpdateInstruction(cc_0, True) ]))
    st_1._set_transitionSet(transitions)
    return fac.Automaton(states, counters, False, containing_state=None)
SPKIDataType._Automaton = _BuildAutomaton_14()




def _BuildAutomaton_15 ():
    # Remove this helper function from the namespace after it is invoked
    global _BuildAutomaton_15
    del _BuildAutomaton_15
    import pyxb.utils.fac as fac

    counters = set()
    cc_0 = fac.CounterCondition(min=0, max=None, metadata=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/ds.xsd', 245, 2))
    counters.add(cc_0)
    states = []
    final_update = set()
    final_update.add(fac.UpdateInstruction(cc_0, False))
    symbol = pyxb.binding.content.WildcardUse(pyxb.binding.content.Wildcard(process_contents=pyxb.binding.content.Wildcard.PC_lax, namespace_constraint=pyxb.binding.content.Wildcard.NC_any), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/ds.xsd', 246, 4))
    st_0 = fac.State(symbol, is_initial=True, final_update=final_update, is_unordered_catenation=False)
    states.append(st_0)
    transitions = []
    transitions.append(fac.Transition(st_0, [
        fac.UpdateInstruction(cc_0, True) ]))
    st_0._set_transitionSet(transitions)
    return fac.Automaton(states, counters, True, containing_state=None)
ObjectType._Automaton = _BuildAutomaton_15()




ManifestType._AddElement(pyxb.binding.basis.element(pyxb.namespace.ExpandedName(Namespace, 'Reference'), ReferenceType, scope=ManifestType, location=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/ds.xsd', 96, 0)))

def _BuildAutomaton_16 ():
    # Remove this helper function from the namespace after it is invoked
    global _BuildAutomaton_16
    del _BuildAutomaton_16
    import pyxb.utils.fac as fac

    counters = set()
    states = []
    final_update = set()
    symbol = pyxb.binding.content.ElementUse(ManifestType._UseForTag(pyxb.namespace.ExpandedName(Namespace, 'Reference')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/ds.xsd', 256, 4))
    st_0 = fac.State(symbol, is_initial=True, final_update=final_update, is_unordered_catenation=False)
    states.append(st_0)
    transitions = []
    transitions.append(fac.Transition(st_0, [
         ]))
    st_0._set_transitionSet(transitions)
    return fac.Automaton(states, counters, False, containing_state=None)
ManifestType._Automaton = _BuildAutomaton_16()




SignaturePropertiesType._AddElement(pyxb.binding.basis.element(pyxb.namespace.ExpandedName(Namespace, 'SignatureProperty'), SignaturePropertyType, scope=SignaturePropertiesType, location=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/ds.xsd', 269, 3)))

def _BuildAutomaton_17 ():
    # Remove this helper function from the namespace after it is invoked
    global _BuildAutomaton_17
    del _BuildAutomaton_17
    import pyxb.utils.fac as fac

    counters = set()
    states = []
    final_update = set()
    symbol = pyxb.binding.content.ElementUse(SignaturePropertiesType._UseForTag(pyxb.namespace.ExpandedName(Namespace, 'SignatureProperty')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/ds.xsd', 264, 4))
    st_0 = fac.State(symbol, is_initial=True, final_update=final_update, is_unordered_catenation=False)
    states.append(st_0)
    transitions = []
    transitions.append(fac.Transition(st_0, [
         ]))
    st_0._set_transitionSet(transitions)
    return fac.Automaton(states, counters, False, containing_state=None)
SignaturePropertiesType._Automaton = _BuildAutomaton_17()




def _BuildAutomaton_18 ():
    # Remove this helper function from the namespace after it is invoked
    global _BuildAutomaton_18
    del _BuildAutomaton_18
    import pyxb.utils.fac as fac

    counters = set()
    states = []
    final_update = set()
    symbol = pyxb.binding.content.WildcardUse(pyxb.binding.content.Wildcard(process_contents=pyxb.binding.content.Wildcard.PC_lax, namespace_constraint=(pyxb.binding.content.Wildcard.NC_not, 'http://www.w3.org/2000/09/xmldsig#')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/ds.xsd', 272, 7))
    st_0 = fac.State(symbol, is_initial=True, final_update=final_update, is_unordered_catenation=False)
    states.append(st_0)
    transitions = []
    transitions.append(fac.Transition(st_0, [
         ]))
    st_0._set_transitionSet(transitions)
    return fac.Automaton(states, counters, False, containing_state=None)
SignaturePropertyType._Automaton = _BuildAutomaton_18()




DSAKeyValueType._AddElement(pyxb.binding.basis.element(pyxb.namespace.ExpandedName(Namespace, 'P'), CryptoBinary, scope=DSAKeyValueType, location=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/ds.xsd', 293, 6)))

DSAKeyValueType._AddElement(pyxb.binding.basis.element(pyxb.namespace.ExpandedName(Namespace, 'Q'), CryptoBinary, scope=DSAKeyValueType, location=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/ds.xsd', 294, 6)))

DSAKeyValueType._AddElement(pyxb.binding.basis.element(pyxb.namespace.ExpandedName(Namespace, 'G'), CryptoBinary, scope=DSAKeyValueType, location=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/ds.xsd', 296, 4)))

DSAKeyValueType._AddElement(pyxb.binding.basis.element(pyxb.namespace.ExpandedName(Namespace, 'Y'), CryptoBinary, scope=DSAKeyValueType, location=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/ds.xsd', 297, 4)))

DSAKeyValueType._AddElement(pyxb.binding.basis.element(pyxb.namespace.ExpandedName(Namespace, 'J'), CryptoBinary, scope=DSAKeyValueType, location=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/ds.xsd', 298, 4)))

DSAKeyValueType._AddElement(pyxb.binding.basis.element(pyxb.namespace.ExpandedName(Namespace, 'Seed'), CryptoBinary, scope=DSAKeyValueType, location=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/ds.xsd', 300, 6)))

DSAKeyValueType._AddElement(pyxb.binding.basis.element(pyxb.namespace.ExpandedName(Namespace, 'PgenCounter'), CryptoBinary, scope=DSAKeyValueType, location=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/ds.xsd', 301, 6)))

def _BuildAutomaton_19 ():
    # Remove this helper function from the namespace after it is invoked
    global _BuildAutomaton_19
    del _BuildAutomaton_19
    import pyxb.utils.fac as fac

    counters = set()
    cc_0 = fac.CounterCondition(min=0, max=1, metadata=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/ds.xsd', 292, 4))
    counters.add(cc_0)
    cc_1 = fac.CounterCondition(min=0, max=1, metadata=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/ds.xsd', 296, 4))
    counters.add(cc_1)
    cc_2 = fac.CounterCondition(min=0, max=1, metadata=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/ds.xsd', 298, 4))
    counters.add(cc_2)
    cc_3 = fac.CounterCondition(min=0, max=1, metadata=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/ds.xsd', 299, 4))
    counters.add(cc_3)
    states = []
    final_update = None
    symbol = pyxb.binding.content.ElementUse(DSAKeyValueType._UseForTag(pyxb.namespace.ExpandedName(Namespace, 'P')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/ds.xsd', 293, 6))
    st_0 = fac.State(symbol, is_initial=True, final_update=final_update, is_unordered_catenation=False)
    states.append(st_0)
    final_update = None
    symbol = pyxb.binding.content.ElementUse(DSAKeyValueType._UseForTag(pyxb.namespace.ExpandedName(Namespace, 'Q')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/ds.xsd', 294, 6))
    st_1 = fac.State(symbol, is_initial=False, final_update=final_update, is_unordered_catenation=False)
    states.append(st_1)
    final_update = None
    symbol = pyxb.binding.content.ElementUse(DSAKeyValueType._UseForTag(pyxb.namespace.ExpandedName(Namespace, 'G')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/ds.xsd', 296, 4))
    st_2 = fac.State(symbol, is_initial=True, final_update=final_update, is_unordered_catenation=False)
    states.append(st_2)
    final_update = set()
    symbol = pyxb.binding.content.ElementUse(DSAKeyValueType._UseForTag(pyxb.namespace.ExpandedName(Namespace, 'Y')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/ds.xsd', 297, 4))
    st_3 = fac.State(symbol, is_initial=True, final_update=final_update, is_unordered_catenation=False)
    states.append(st_3)
    final_update = set()
    final_update.add(fac.UpdateInstruction(cc_2, False))
    symbol = pyxb.binding.content.ElementUse(DSAKeyValueType._UseForTag(pyxb.namespace.ExpandedName(Namespace, 'J')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/ds.xsd', 298, 4))
    st_4 = fac.State(symbol, is_initial=False, final_update=final_update, is_unordered_catenation=False)
    states.append(st_4)
    final_update = None
    symbol = pyxb.binding.content.ElementUse(DSAKeyValueType._UseForTag(pyxb.namespace.ExpandedName(Namespace, 'Seed')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/ds.xsd', 300, 6))
    st_5 = fac.State(symbol, is_initial=False, final_update=final_update, is_unordered_catenation=False)
    states.append(st_5)
    final_update = set()
    final_update.add(fac.UpdateInstruction(cc_3, False))
    symbol = pyxb.binding.content.ElementUse(DSAKeyValueType._UseForTag(pyxb.namespace.ExpandedName(Namespace, 'PgenCounter')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/ds.xsd', 301, 6))
    st_6 = fac.State(symbol, is_initial=False, final_update=final_update, is_unordered_catenation=False)
    states.append(st_6)
    transitions = []
    transitions.append(fac.Transition(st_1, [
         ]))
    st_0._set_transitionSet(transitions)
    transitions = []
    transitions.append(fac.Transition(st_0, [
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
    transitions.append(fac.Transition(st_4, [
         ]))
    transitions.append(fac.Transition(st_5, [
         ]))
    st_3._set_transitionSet(transitions)
    transitions = []
    transitions.append(fac.Transition(st_4, [
        fac.UpdateInstruction(cc_2, True) ]))
    transitions.append(fac.Transition(st_5, [
        fac.UpdateInstruction(cc_2, False) ]))
    st_4._set_transitionSet(transitions)
    transitions = []
    transitions.append(fac.Transition(st_6, [
         ]))
    st_5._set_transitionSet(transitions)
    transitions = []
    transitions.append(fac.Transition(st_5, [
        fac.UpdateInstruction(cc_3, True) ]))
    st_6._set_transitionSet(transitions)
    return fac.Automaton(states, counters, False, containing_state=None)
DSAKeyValueType._Automaton = _BuildAutomaton_19()




RSAKeyValueType._AddElement(pyxb.binding.basis.element(pyxb.namespace.ExpandedName(Namespace, 'Modulus'), CryptoBinary, scope=RSAKeyValueType, location=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/ds.xsd', 309, 4)))

RSAKeyValueType._AddElement(pyxb.binding.basis.element(pyxb.namespace.ExpandedName(Namespace, 'Exponent'), CryptoBinary, scope=RSAKeyValueType, location=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/ds.xsd', 310, 4)))

def _BuildAutomaton_20 ():
    # Remove this helper function from the namespace after it is invoked
    global _BuildAutomaton_20
    del _BuildAutomaton_20
    import pyxb.utils.fac as fac

    counters = set()
    states = []
    final_update = None
    symbol = pyxb.binding.content.ElementUse(RSAKeyValueType._UseForTag(pyxb.namespace.ExpandedName(Namespace, 'Modulus')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/ds.xsd', 309, 4))
    st_0 = fac.State(symbol, is_initial=True, final_update=final_update, is_unordered_catenation=False)
    states.append(st_0)
    final_update = set()
    symbol = pyxb.binding.content.ElementUse(RSAKeyValueType._UseForTag(pyxb.namespace.ExpandedName(Namespace, 'Exponent')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/ds.xsd', 310, 4))
    st_1 = fac.State(symbol, is_initial=False, final_update=final_update, is_unordered_catenation=False)
    states.append(st_1)
    transitions = []
    transitions.append(fac.Transition(st_1, [
         ]))
    st_0._set_transitionSet(transitions)
    transitions = []
    st_1._set_transitionSet(transitions)
    return fac.Automaton(states, counters, False, containing_state=None)
RSAKeyValueType._Automaton = _BuildAutomaton_20()

