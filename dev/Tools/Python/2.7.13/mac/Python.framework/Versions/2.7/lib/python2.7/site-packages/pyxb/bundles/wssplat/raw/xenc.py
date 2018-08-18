# ./pyxb/bundles/wssplat/raw/xenc.py
# -*- coding: utf-8 -*-
# PyXB bindings for NM:7ed00f02a5533309db63dc4da4f345e09e5bf96a
# Generated 2014-10-19 06:24:58.050195 by PyXB version 1.2.4 using Python 2.7.3.final.0
# Namespace http://www.w3.org/2001/04/xmlenc#

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
_GenerationUID = pyxb.utils.utility.UniqueIdentifier('urn:uuid:8ebd8546-5782-11e4-84a5-c8600024e903')

# Version of PyXB used to generate the bindings
_PyXBVersion = '1.2.4'
# Generated bindings are not compatible across PyXB versions
if pyxb.__version__ != _PyXBVersion:
    raise pyxb.PyXBVersionError(_PyXBVersion)

# Import bindings for namespaces imported into schema
import pyxb.bundles.wssplat.ds
import pyxb.binding.datatypes

# NOTE: All namespace declarations are reserved within the binding
Namespace = pyxb.namespace.NamespaceForURI('http://www.w3.org/2001/04/xmlenc#', create_if_missing=True)
Namespace.configureCategories(['typeBinding', 'elementBinding'])
_Namespace_ds = pyxb.bundles.wssplat.ds.Namespace
_Namespace_ds.configureCategories(['typeBinding', 'elementBinding'])

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


# Atomic simple type: {http://www.w3.org/2001/04/xmlenc#}KeySizeType
class KeySizeType (pyxb.binding.datatypes.integer):

    """An atomic simple type."""

    _ExpandedName = pyxb.namespace.ExpandedName(Namespace, 'KeySizeType')
    _XSDLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/xenc.xsd', 45, 4)
    _Documentation = None
KeySizeType._InitializeFacetMap()
Namespace.addCategoryObject('typeBinding', 'KeySizeType', KeySizeType)

# Complex type {http://www.w3.org/2001/04/xmlenc#}EncryptedType with content type ELEMENT_ONLY
class EncryptedType (pyxb.binding.basis.complexTypeDefinition):
    """Complex type {http://www.w3.org/2001/04/xmlenc#}EncryptedType with content type ELEMENT_ONLY"""
    _TypeDefinition = None
    _ContentTypeTag = pyxb.binding.basis.complexTypeDefinition._CT_ELEMENT_ONLY
    _Abstract = True
    _ExpandedName = pyxb.namespace.ExpandedName(Namespace, 'EncryptedType')
    _XSDLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/xenc.xsd', 22, 2)
    _ElementMap = {}
    _AttributeMap = {}
    # Base type is pyxb.binding.datatypes.anyType
    
    # Element {http://www.w3.org/2000/09/xmldsig#}KeyInfo uses Python identifier KeyInfo
    __KeyInfo = pyxb.binding.content.ElementDeclaration(pyxb.namespace.ExpandedName(_Namespace_ds, 'KeyInfo'), 'KeyInfo', '__httpwww_w3_org200104xmlenc_EncryptedType_httpwww_w3_org200009xmldsigKeyInfo', False, pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/ds.xsd', 144, 0), )

    
    KeyInfo = property(__KeyInfo.value, __KeyInfo.set, None, None)

    
    # Element {http://www.w3.org/2001/04/xmlenc#}EncryptionMethod uses Python identifier EncryptionMethod
    __EncryptionMethod = pyxb.binding.content.ElementDeclaration(pyxb.namespace.ExpandedName(Namespace, 'EncryptionMethod'), 'EncryptionMethod', '__httpwww_w3_org200104xmlenc_EncryptedType_httpwww_w3_org200104xmlencEncryptionMethod', False, pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/xenc.xsd', 24, 6), )

    
    EncryptionMethod = property(__EncryptionMethod.value, __EncryptionMethod.set, None, None)

    
    # Element {http://www.w3.org/2001/04/xmlenc#}CipherData uses Python identifier CipherData
    __CipherData = pyxb.binding.content.ElementDeclaration(pyxb.namespace.ExpandedName(Namespace, 'CipherData'), 'CipherData', '__httpwww_w3_org200104xmlenc_EncryptedType_httpwww_w3_org200104xmlencCipherData', False, pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/xenc.xsd', 49, 2), )

    
    CipherData = property(__CipherData.value, __CipherData.set, None, None)

    
    # Element {http://www.w3.org/2001/04/xmlenc#}EncryptionProperties uses Python identifier EncryptionProperties
    __EncryptionProperties = pyxb.binding.content.ElementDeclaration(pyxb.namespace.ExpandedName(Namespace, 'EncryptionProperties'), 'EncryptionProperties', '__httpwww_w3_org200104xmlenc_EncryptedType_httpwww_w3_org200104xmlencEncryptionProperties', False, pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/xenc.xsd', 127, 2), )

    
    EncryptionProperties = property(__EncryptionProperties.value, __EncryptionProperties.set, None, None)

    
    # Attribute Id uses Python identifier Id
    __Id = pyxb.binding.content.AttributeUse(pyxb.namespace.ExpandedName(None, 'Id'), 'Id', '__httpwww_w3_org200104xmlenc_EncryptedType_Id', pyxb.binding.datatypes.ID)
    __Id._DeclarationLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/xenc.xsd', 30, 4)
    __Id._UseLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/xenc.xsd', 30, 4)
    
    Id = property(__Id.value, __Id.set, None, None)

    
    # Attribute Type uses Python identifier Type
    __Type = pyxb.binding.content.AttributeUse(pyxb.namespace.ExpandedName(None, 'Type'), 'Type', '__httpwww_w3_org200104xmlenc_EncryptedType_Type', pyxb.binding.datatypes.anyURI)
    __Type._DeclarationLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/xenc.xsd', 31, 4)
    __Type._UseLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/xenc.xsd', 31, 4)
    
    Type = property(__Type.value, __Type.set, None, None)

    
    # Attribute MimeType uses Python identifier MimeType
    __MimeType = pyxb.binding.content.AttributeUse(pyxb.namespace.ExpandedName(None, 'MimeType'), 'MimeType', '__httpwww_w3_org200104xmlenc_EncryptedType_MimeType', pyxb.binding.datatypes.string)
    __MimeType._DeclarationLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/xenc.xsd', 32, 4)
    __MimeType._UseLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/xenc.xsd', 32, 4)
    
    MimeType = property(__MimeType.value, __MimeType.set, None, None)

    
    # Attribute Encoding uses Python identifier Encoding
    __Encoding = pyxb.binding.content.AttributeUse(pyxb.namespace.ExpandedName(None, 'Encoding'), 'Encoding', '__httpwww_w3_org200104xmlenc_EncryptedType_Encoding', pyxb.binding.datatypes.anyURI)
    __Encoding._DeclarationLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/xenc.xsd', 33, 4)
    __Encoding._UseLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/xenc.xsd', 33, 4)
    
    Encoding = property(__Encoding.value, __Encoding.set, None, None)

    _ElementMap.update({
        __KeyInfo.name() : __KeyInfo,
        __EncryptionMethod.name() : __EncryptionMethod,
        __CipherData.name() : __CipherData,
        __EncryptionProperties.name() : __EncryptionProperties
    })
    _AttributeMap.update({
        __Id.name() : __Id,
        __Type.name() : __Type,
        __MimeType.name() : __MimeType,
        __Encoding.name() : __Encoding
    })
Namespace.addCategoryObject('typeBinding', 'EncryptedType', EncryptedType)


# Complex type {http://www.w3.org/2001/04/xmlenc#}EncryptionMethodType with content type MIXED
class EncryptionMethodType (pyxb.binding.basis.complexTypeDefinition):
    """Complex type {http://www.w3.org/2001/04/xmlenc#}EncryptionMethodType with content type MIXED"""
    _TypeDefinition = None
    _ContentTypeTag = pyxb.binding.basis.complexTypeDefinition._CT_MIXED
    _Abstract = False
    _ExpandedName = pyxb.namespace.ExpandedName(Namespace, 'EncryptionMethodType')
    _XSDLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/xenc.xsd', 36, 2)
    _ElementMap = {}
    _AttributeMap = {}
    # Base type is pyxb.binding.datatypes.anyType
    
    # Element {http://www.w3.org/2001/04/xmlenc#}KeySize uses Python identifier KeySize
    __KeySize = pyxb.binding.content.ElementDeclaration(pyxb.namespace.ExpandedName(Namespace, 'KeySize'), 'KeySize', '__httpwww_w3_org200104xmlenc_EncryptionMethodType_httpwww_w3_org200104xmlencKeySize', False, pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/xenc.xsd', 38, 6), )

    
    KeySize = property(__KeySize.value, __KeySize.set, None, None)

    
    # Element {http://www.w3.org/2001/04/xmlenc#}OAEPparams uses Python identifier OAEPparams
    __OAEPparams = pyxb.binding.content.ElementDeclaration(pyxb.namespace.ExpandedName(Namespace, 'OAEPparams'), 'OAEPparams', '__httpwww_w3_org200104xmlenc_EncryptionMethodType_httpwww_w3_org200104xmlencOAEPparams', False, pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/xenc.xsd', 39, 6), )

    
    OAEPparams = property(__OAEPparams.value, __OAEPparams.set, None, None)

    
    # Attribute Algorithm uses Python identifier Algorithm
    __Algorithm = pyxb.binding.content.AttributeUse(pyxb.namespace.ExpandedName(None, 'Algorithm'), 'Algorithm', '__httpwww_w3_org200104xmlenc_EncryptionMethodType_Algorithm', pyxb.binding.datatypes.anyURI, required=True)
    __Algorithm._DeclarationLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/xenc.xsd', 42, 4)
    __Algorithm._UseLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/xenc.xsd', 42, 4)
    
    Algorithm = property(__Algorithm.value, __Algorithm.set, None, None)

    _HasWildcardElement = True
    _ElementMap.update({
        __KeySize.name() : __KeySize,
        __OAEPparams.name() : __OAEPparams
    })
    _AttributeMap.update({
        __Algorithm.name() : __Algorithm
    })
Namespace.addCategoryObject('typeBinding', 'EncryptionMethodType', EncryptionMethodType)


# Complex type {http://www.w3.org/2001/04/xmlenc#}CipherDataType with content type ELEMENT_ONLY
class CipherDataType (pyxb.binding.basis.complexTypeDefinition):
    """Complex type {http://www.w3.org/2001/04/xmlenc#}CipherDataType with content type ELEMENT_ONLY"""
    _TypeDefinition = None
    _ContentTypeTag = pyxb.binding.basis.complexTypeDefinition._CT_ELEMENT_ONLY
    _Abstract = False
    _ExpandedName = pyxb.namespace.ExpandedName(Namespace, 'CipherDataType')
    _XSDLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/xenc.xsd', 50, 2)
    _ElementMap = {}
    _AttributeMap = {}
    # Base type is pyxb.binding.datatypes.anyType
    
    # Element {http://www.w3.org/2001/04/xmlenc#}CipherValue uses Python identifier CipherValue
    __CipherValue = pyxb.binding.content.ElementDeclaration(pyxb.namespace.ExpandedName(Namespace, 'CipherValue'), 'CipherValue', '__httpwww_w3_org200104xmlenc_CipherDataType_httpwww_w3_org200104xmlencCipherValue', False, pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/xenc.xsd', 52, 7), )

    
    CipherValue = property(__CipherValue.value, __CipherValue.set, None, None)

    
    # Element {http://www.w3.org/2001/04/xmlenc#}CipherReference uses Python identifier CipherReference
    __CipherReference = pyxb.binding.content.ElementDeclaration(pyxb.namespace.ExpandedName(Namespace, 'CipherReference'), 'CipherReference', '__httpwww_w3_org200104xmlenc_CipherDataType_httpwww_w3_org200104xmlencCipherReference', False, pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/xenc.xsd', 57, 3), )

    
    CipherReference = property(__CipherReference.value, __CipherReference.set, None, None)

    _ElementMap.update({
        __CipherValue.name() : __CipherValue,
        __CipherReference.name() : __CipherReference
    })
    _AttributeMap.update({
        
    })
Namespace.addCategoryObject('typeBinding', 'CipherDataType', CipherDataType)


# Complex type {http://www.w3.org/2001/04/xmlenc#}CipherReferenceType with content type ELEMENT_ONLY
class CipherReferenceType (pyxb.binding.basis.complexTypeDefinition):
    """Complex type {http://www.w3.org/2001/04/xmlenc#}CipherReferenceType with content type ELEMENT_ONLY"""
    _TypeDefinition = None
    _ContentTypeTag = pyxb.binding.basis.complexTypeDefinition._CT_ELEMENT_ONLY
    _Abstract = False
    _ExpandedName = pyxb.namespace.ExpandedName(Namespace, 'CipherReferenceType')
    _XSDLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/xenc.xsd', 58, 3)
    _ElementMap = {}
    _AttributeMap = {}
    # Base type is pyxb.binding.datatypes.anyType
    
    # Element {http://www.w3.org/2001/04/xmlenc#}Transforms uses Python identifier Transforms
    __Transforms = pyxb.binding.content.ElementDeclaration(pyxb.namespace.ExpandedName(Namespace, 'Transforms'), 'Transforms', '__httpwww_w3_org200104xmlenc_CipherReferenceType_httpwww_w3_org200104xmlencTransforms', False, pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/xenc.xsd', 60, 9), )

    
    Transforms = property(__Transforms.value, __Transforms.set, None, None)

    
    # Attribute URI uses Python identifier URI
    __URI = pyxb.binding.content.AttributeUse(pyxb.namespace.ExpandedName(None, 'URI'), 'URI', '__httpwww_w3_org200104xmlenc_CipherReferenceType_URI', pyxb.binding.datatypes.anyURI, required=True)
    __URI._DeclarationLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/xenc.xsd', 62, 7)
    __URI._UseLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/xenc.xsd', 62, 7)
    
    URI = property(__URI.value, __URI.set, None, None)

    _ElementMap.update({
        __Transforms.name() : __Transforms
    })
    _AttributeMap.update({
        __URI.name() : __URI
    })
Namespace.addCategoryObject('typeBinding', 'CipherReferenceType', CipherReferenceType)


# Complex type {http://www.w3.org/2001/04/xmlenc#}TransformsType with content type ELEMENT_ONLY
class TransformsType (pyxb.binding.basis.complexTypeDefinition):
    """Complex type {http://www.w3.org/2001/04/xmlenc#}TransformsType with content type ELEMENT_ONLY"""
    _TypeDefinition = None
    _ContentTypeTag = pyxb.binding.basis.complexTypeDefinition._CT_ELEMENT_ONLY
    _Abstract = False
    _ExpandedName = pyxb.namespace.ExpandedName(Namespace, 'TransformsType')
    _XSDLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/xenc.xsd', 65, 5)
    _ElementMap = {}
    _AttributeMap = {}
    # Base type is pyxb.binding.datatypes.anyType
    
    # Element {http://www.w3.org/2000/09/xmldsig#}Transform uses Python identifier Transform
    __Transform = pyxb.binding.content.ElementDeclaration(pyxb.namespace.ExpandedName(_Namespace_ds, 'Transform'), 'Transform', '__httpwww_w3_org200104xmlenc_TransformsType_httpwww_w3_org200009xmldsigTransform', True, pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/ds.xsd', 115, 2), )

    
    Transform = property(__Transform.value, __Transform.set, None, None)

    _ElementMap.update({
        __Transform.name() : __Transform
    })
    _AttributeMap.update({
        
    })
Namespace.addCategoryObject('typeBinding', 'TransformsType', TransformsType)


# Complex type {http://www.w3.org/2001/04/xmlenc#}AgreementMethodType with content type MIXED
class AgreementMethodType (pyxb.binding.basis.complexTypeDefinition):
    """Complex type {http://www.w3.org/2001/04/xmlenc#}AgreementMethodType with content type MIXED"""
    _TypeDefinition = None
    _ContentTypeTag = pyxb.binding.basis.complexTypeDefinition._CT_MIXED
    _Abstract = False
    _ExpandedName = pyxb.namespace.ExpandedName(Namespace, 'AgreementMethodType')
    _XSDLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/xenc.xsd', 97, 4)
    _ElementMap = {}
    _AttributeMap = {}
    # Base type is pyxb.binding.datatypes.anyType
    
    # Element {http://www.w3.org/2001/04/xmlenc#}KA-Nonce uses Python identifier KA_Nonce
    __KA_Nonce = pyxb.binding.content.ElementDeclaration(pyxb.namespace.ExpandedName(Namespace, 'KA-Nonce'), 'KA_Nonce', '__httpwww_w3_org200104xmlenc_AgreementMethodType_httpwww_w3_org200104xmlencKA_Nonce', False, pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/xenc.xsd', 99, 8), )

    
    KA_Nonce = property(__KA_Nonce.value, __KA_Nonce.set, None, None)

    
    # Element {http://www.w3.org/2001/04/xmlenc#}OriginatorKeyInfo uses Python identifier OriginatorKeyInfo
    __OriginatorKeyInfo = pyxb.binding.content.ElementDeclaration(pyxb.namespace.ExpandedName(Namespace, 'OriginatorKeyInfo'), 'OriginatorKeyInfo', '__httpwww_w3_org200104xmlenc_AgreementMethodType_httpwww_w3_org200104xmlencOriginatorKeyInfo', False, pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/xenc.xsd', 102, 8), )

    
    OriginatorKeyInfo = property(__OriginatorKeyInfo.value, __OriginatorKeyInfo.set, None, None)

    
    # Element {http://www.w3.org/2001/04/xmlenc#}RecipientKeyInfo uses Python identifier RecipientKeyInfo
    __RecipientKeyInfo = pyxb.binding.content.ElementDeclaration(pyxb.namespace.ExpandedName(Namespace, 'RecipientKeyInfo'), 'RecipientKeyInfo', '__httpwww_w3_org200104xmlenc_AgreementMethodType_httpwww_w3_org200104xmlencRecipientKeyInfo', False, pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/xenc.xsd', 103, 8), )

    
    RecipientKeyInfo = property(__RecipientKeyInfo.value, __RecipientKeyInfo.set, None, None)

    
    # Attribute Algorithm uses Python identifier Algorithm
    __Algorithm = pyxb.binding.content.AttributeUse(pyxb.namespace.ExpandedName(None, 'Algorithm'), 'Algorithm', '__httpwww_w3_org200104xmlenc_AgreementMethodType_Algorithm', pyxb.binding.datatypes.anyURI, required=True)
    __Algorithm._DeclarationLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/xenc.xsd', 105, 6)
    __Algorithm._UseLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/xenc.xsd', 105, 6)
    
    Algorithm = property(__Algorithm.value, __Algorithm.set, None, None)

    _HasWildcardElement = True
    _ElementMap.update({
        __KA_Nonce.name() : __KA_Nonce,
        __OriginatorKeyInfo.name() : __OriginatorKeyInfo,
        __RecipientKeyInfo.name() : __RecipientKeyInfo
    })
    _AttributeMap.update({
        __Algorithm.name() : __Algorithm
    })
Namespace.addCategoryObject('typeBinding', 'AgreementMethodType', AgreementMethodType)


# Complex type [anonymous] with content type ELEMENT_ONLY
class CTD_ANON (pyxb.binding.basis.complexTypeDefinition):
    """Complex type [anonymous] with content type ELEMENT_ONLY"""
    _TypeDefinition = None
    _ContentTypeTag = pyxb.binding.basis.complexTypeDefinition._CT_ELEMENT_ONLY
    _Abstract = False
    _ExpandedName = None
    _XSDLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/xenc.xsd', 111, 4)
    _ElementMap = {}
    _AttributeMap = {}
    # Base type is pyxb.binding.datatypes.anyType
    
    # Element {http://www.w3.org/2001/04/xmlenc#}DataReference uses Python identifier DataReference
    __DataReference = pyxb.binding.content.ElementDeclaration(pyxb.namespace.ExpandedName(Namespace, 'DataReference'), 'DataReference', '__httpwww_w3_org200104xmlenc_CTD_ANON_httpwww_w3_org200104xmlencDataReference', True, pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/xenc.xsd', 113, 8), )

    
    DataReference = property(__DataReference.value, __DataReference.set, None, None)

    
    # Element {http://www.w3.org/2001/04/xmlenc#}KeyReference uses Python identifier KeyReference
    __KeyReference = pyxb.binding.content.ElementDeclaration(pyxb.namespace.ExpandedName(Namespace, 'KeyReference'), 'KeyReference', '__httpwww_w3_org200104xmlenc_CTD_ANON_httpwww_w3_org200104xmlencKeyReference', True, pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/xenc.xsd', 114, 8), )

    
    KeyReference = property(__KeyReference.value, __KeyReference.set, None, None)

    _ElementMap.update({
        __DataReference.name() : __DataReference,
        __KeyReference.name() : __KeyReference
    })
    _AttributeMap.update({
        
    })



# Complex type {http://www.w3.org/2001/04/xmlenc#}ReferenceType with content type ELEMENT_ONLY
class ReferenceType (pyxb.binding.basis.complexTypeDefinition):
    """Complex type {http://www.w3.org/2001/04/xmlenc#}ReferenceType with content type ELEMENT_ONLY"""
    _TypeDefinition = None
    _ContentTypeTag = pyxb.binding.basis.complexTypeDefinition._CT_ELEMENT_ONLY
    _Abstract = False
    _ExpandedName = pyxb.namespace.ExpandedName(Namespace, 'ReferenceType')
    _XSDLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/xenc.xsd', 119, 2)
    _ElementMap = {}
    _AttributeMap = {}
    # Base type is pyxb.binding.datatypes.anyType
    
    # Attribute URI uses Python identifier URI
    __URI = pyxb.binding.content.AttributeUse(pyxb.namespace.ExpandedName(None, 'URI'), 'URI', '__httpwww_w3_org200104xmlenc_ReferenceType_URI', pyxb.binding.datatypes.anyURI, required=True)
    __URI._DeclarationLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/xenc.xsd', 123, 4)
    __URI._UseLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/xenc.xsd', 123, 4)
    
    URI = property(__URI.value, __URI.set, None, None)

    _HasWildcardElement = True
    _ElementMap.update({
        
    })
    _AttributeMap.update({
        __URI.name() : __URI
    })
Namespace.addCategoryObject('typeBinding', 'ReferenceType', ReferenceType)


# Complex type {http://www.w3.org/2001/04/xmlenc#}EncryptionPropertiesType with content type ELEMENT_ONLY
class EncryptionPropertiesType (pyxb.binding.basis.complexTypeDefinition):
    """Complex type {http://www.w3.org/2001/04/xmlenc#}EncryptionPropertiesType with content type ELEMENT_ONLY"""
    _TypeDefinition = None
    _ContentTypeTag = pyxb.binding.basis.complexTypeDefinition._CT_ELEMENT_ONLY
    _Abstract = False
    _ExpandedName = pyxb.namespace.ExpandedName(Namespace, 'EncryptionPropertiesType')
    _XSDLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/xenc.xsd', 128, 2)
    _ElementMap = {}
    _AttributeMap = {}
    # Base type is pyxb.binding.datatypes.anyType
    
    # Element {http://www.w3.org/2001/04/xmlenc#}EncryptionProperty uses Python identifier EncryptionProperty
    __EncryptionProperty = pyxb.binding.content.ElementDeclaration(pyxb.namespace.ExpandedName(Namespace, 'EncryptionProperty'), 'EncryptionProperty', '__httpwww_w3_org200104xmlenc_EncryptionPropertiesType_httpwww_w3_org200104xmlencEncryptionProperty', True, pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/xenc.xsd', 135, 4), )

    
    EncryptionProperty = property(__EncryptionProperty.value, __EncryptionProperty.set, None, None)

    
    # Attribute Id uses Python identifier Id
    __Id = pyxb.binding.content.AttributeUse(pyxb.namespace.ExpandedName(None, 'Id'), 'Id', '__httpwww_w3_org200104xmlenc_EncryptionPropertiesType_Id', pyxb.binding.datatypes.ID)
    __Id._DeclarationLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/xenc.xsd', 132, 4)
    __Id._UseLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/xenc.xsd', 132, 4)
    
    Id = property(__Id.value, __Id.set, None, None)

    _ElementMap.update({
        __EncryptionProperty.name() : __EncryptionProperty
    })
    _AttributeMap.update({
        __Id.name() : __Id
    })
Namespace.addCategoryObject('typeBinding', 'EncryptionPropertiesType', EncryptionPropertiesType)


# Complex type {http://www.w3.org/2001/04/xmlenc#}EncryptionPropertyType with content type MIXED
class EncryptionPropertyType (pyxb.binding.basis.complexTypeDefinition):
    """Complex type {http://www.w3.org/2001/04/xmlenc#}EncryptionPropertyType with content type MIXED"""
    _TypeDefinition = None
    _ContentTypeTag = pyxb.binding.basis.complexTypeDefinition._CT_MIXED
    _Abstract = False
    _ExpandedName = pyxb.namespace.ExpandedName(Namespace, 'EncryptionPropertyType')
    _XSDLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/xenc.xsd', 136, 4)
    _ElementMap = {}
    _AttributeMap = {}
    # Base type is pyxb.binding.datatypes.anyType
    
    # Attribute Target uses Python identifier Target
    __Target = pyxb.binding.content.AttributeUse(pyxb.namespace.ExpandedName(None, 'Target'), 'Target', '__httpwww_w3_org200104xmlenc_EncryptionPropertyType_Target', pyxb.binding.datatypes.anyURI)
    __Target._DeclarationLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/xenc.xsd', 140, 6)
    __Target._UseLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/xenc.xsd', 140, 6)
    
    Target = property(__Target.value, __Target.set, None, None)

    
    # Attribute Id uses Python identifier Id
    __Id = pyxb.binding.content.AttributeUse(pyxb.namespace.ExpandedName(None, 'Id'), 'Id', '__httpwww_w3_org200104xmlenc_EncryptionPropertyType_Id', pyxb.binding.datatypes.ID)
    __Id._DeclarationLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/xenc.xsd', 141, 6)
    __Id._UseLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/xenc.xsd', 141, 6)
    
    Id = property(__Id.value, __Id.set, None, None)

    _AttributeWildcard = pyxb.binding.content.Wildcard(process_contents=pyxb.binding.content.Wildcard.PC_strict, namespace_constraint=set(['http://www.w3.org/XML/1998/namespace']))
    _HasWildcardElement = True
    _ElementMap.update({
        
    })
    _AttributeMap.update({
        __Target.name() : __Target,
        __Id.name() : __Id
    })
Namespace.addCategoryObject('typeBinding', 'EncryptionPropertyType', EncryptionPropertyType)


# Complex type {http://www.w3.org/2001/04/xmlenc#}EncryptedDataType with content type ELEMENT_ONLY
class EncryptedDataType (EncryptedType):
    """Complex type {http://www.w3.org/2001/04/xmlenc#}EncryptedDataType with content type ELEMENT_ONLY"""
    _TypeDefinition = None
    _ContentTypeTag = pyxb.binding.basis.complexTypeDefinition._CT_ELEMENT_ONLY
    _Abstract = False
    _ExpandedName = pyxb.namespace.ExpandedName(Namespace, 'EncryptedDataType')
    _XSDLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/xenc.xsd', 73, 2)
    _ElementMap = EncryptedType._ElementMap.copy()
    _AttributeMap = EncryptedType._AttributeMap.copy()
    # Base type is EncryptedType
    
    # Element KeyInfo ({http://www.w3.org/2000/09/xmldsig#}KeyInfo) inherited from {http://www.w3.org/2001/04/xmlenc#}EncryptedType
    
    # Element EncryptionMethod ({http://www.w3.org/2001/04/xmlenc#}EncryptionMethod) inherited from {http://www.w3.org/2001/04/xmlenc#}EncryptedType
    
    # Element CipherData ({http://www.w3.org/2001/04/xmlenc#}CipherData) inherited from {http://www.w3.org/2001/04/xmlenc#}EncryptedType
    
    # Element EncryptionProperties ({http://www.w3.org/2001/04/xmlenc#}EncryptionProperties) inherited from {http://www.w3.org/2001/04/xmlenc#}EncryptedType
    
    # Attribute Id inherited from {http://www.w3.org/2001/04/xmlenc#}EncryptedType
    
    # Attribute Type inherited from {http://www.w3.org/2001/04/xmlenc#}EncryptedType
    
    # Attribute MimeType inherited from {http://www.w3.org/2001/04/xmlenc#}EncryptedType
    
    # Attribute Encoding inherited from {http://www.w3.org/2001/04/xmlenc#}EncryptedType
    _ElementMap.update({
        
    })
    _AttributeMap.update({
        
    })
Namespace.addCategoryObject('typeBinding', 'EncryptedDataType', EncryptedDataType)


# Complex type {http://www.w3.org/2001/04/xmlenc#}EncryptedKeyType with content type ELEMENT_ONLY
class EncryptedKeyType (EncryptedType):
    """Complex type {http://www.w3.org/2001/04/xmlenc#}EncryptedKeyType with content type ELEMENT_ONLY"""
    _TypeDefinition = None
    _ContentTypeTag = pyxb.binding.basis.complexTypeDefinition._CT_ELEMENT_ONLY
    _Abstract = False
    _ExpandedName = pyxb.namespace.ExpandedName(Namespace, 'EncryptedKeyType')
    _XSDLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/xenc.xsd', 83, 2)
    _ElementMap = EncryptedType._ElementMap.copy()
    _AttributeMap = EncryptedType._AttributeMap.copy()
    # Base type is EncryptedType
    
    # Element KeyInfo ({http://www.w3.org/2000/09/xmldsig#}KeyInfo) inherited from {http://www.w3.org/2001/04/xmlenc#}EncryptedType
    
    # Element EncryptionMethod ({http://www.w3.org/2001/04/xmlenc#}EncryptionMethod) inherited from {http://www.w3.org/2001/04/xmlenc#}EncryptedType
    
    # Element CipherData ({http://www.w3.org/2001/04/xmlenc#}CipherData) inherited from {http://www.w3.org/2001/04/xmlenc#}EncryptedType
    
    # Element {http://www.w3.org/2001/04/xmlenc#}CarriedKeyName uses Python identifier CarriedKeyName
    __CarriedKeyName = pyxb.binding.content.ElementDeclaration(pyxb.namespace.ExpandedName(Namespace, 'CarriedKeyName'), 'CarriedKeyName', '__httpwww_w3_org200104xmlenc_EncryptedKeyType_httpwww_w3_org200104xmlencCarriedKeyName', False, pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/xenc.xsd', 88, 10), )

    
    CarriedKeyName = property(__CarriedKeyName.value, __CarriedKeyName.set, None, None)

    
    # Element {http://www.w3.org/2001/04/xmlenc#}ReferenceList uses Python identifier ReferenceList
    __ReferenceList = pyxb.binding.content.ElementDeclaration(pyxb.namespace.ExpandedName(Namespace, 'ReferenceList'), 'ReferenceList', '__httpwww_w3_org200104xmlenc_EncryptedKeyType_httpwww_w3_org200104xmlencReferenceList', False, pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/xenc.xsd', 110, 2), )

    
    ReferenceList = property(__ReferenceList.value, __ReferenceList.set, None, None)

    
    # Element EncryptionProperties ({http://www.w3.org/2001/04/xmlenc#}EncryptionProperties) inherited from {http://www.w3.org/2001/04/xmlenc#}EncryptedType
    
    # Attribute Id inherited from {http://www.w3.org/2001/04/xmlenc#}EncryptedType
    
    # Attribute Type inherited from {http://www.w3.org/2001/04/xmlenc#}EncryptedType
    
    # Attribute MimeType inherited from {http://www.w3.org/2001/04/xmlenc#}EncryptedType
    
    # Attribute Encoding inherited from {http://www.w3.org/2001/04/xmlenc#}EncryptedType
    
    # Attribute Recipient uses Python identifier Recipient
    __Recipient = pyxb.binding.content.AttributeUse(pyxb.namespace.ExpandedName(None, 'Recipient'), 'Recipient', '__httpwww_w3_org200104xmlenc_EncryptedKeyType_Recipient', pyxb.binding.datatypes.string)
    __Recipient._DeclarationLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/xenc.xsd', 90, 8)
    __Recipient._UseLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/xenc.xsd', 90, 8)
    
    Recipient = property(__Recipient.value, __Recipient.set, None, None)

    _ElementMap.update({
        __CarriedKeyName.name() : __CarriedKeyName,
        __ReferenceList.name() : __ReferenceList
    })
    _AttributeMap.update({
        __Recipient.name() : __Recipient
    })
Namespace.addCategoryObject('typeBinding', 'EncryptedKeyType', EncryptedKeyType)


CipherData = pyxb.binding.basis.element(pyxb.namespace.ExpandedName(Namespace, 'CipherData'), CipherDataType, location=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/xenc.xsd', 49, 2))
Namespace.addCategoryObject('elementBinding', CipherData.name().localName(), CipherData)

CipherReference = pyxb.binding.basis.element(pyxb.namespace.ExpandedName(Namespace, 'CipherReference'), CipherReferenceType, location=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/xenc.xsd', 57, 3))
Namespace.addCategoryObject('elementBinding', CipherReference.name().localName(), CipherReference)

AgreementMethod = pyxb.binding.basis.element(pyxb.namespace.ExpandedName(Namespace, 'AgreementMethod'), AgreementMethodType, location=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/xenc.xsd', 96, 4))
Namespace.addCategoryObject('elementBinding', AgreementMethod.name().localName(), AgreementMethod)

ReferenceList = pyxb.binding.basis.element(pyxb.namespace.ExpandedName(Namespace, 'ReferenceList'), CTD_ANON, location=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/xenc.xsd', 110, 2))
Namespace.addCategoryObject('elementBinding', ReferenceList.name().localName(), ReferenceList)

EncryptionProperties = pyxb.binding.basis.element(pyxb.namespace.ExpandedName(Namespace, 'EncryptionProperties'), EncryptionPropertiesType, location=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/xenc.xsd', 127, 2))
Namespace.addCategoryObject('elementBinding', EncryptionProperties.name().localName(), EncryptionProperties)

EncryptionProperty = pyxb.binding.basis.element(pyxb.namespace.ExpandedName(Namespace, 'EncryptionProperty'), EncryptionPropertyType, location=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/xenc.xsd', 135, 4))
Namespace.addCategoryObject('elementBinding', EncryptionProperty.name().localName(), EncryptionProperty)

EncryptedData = pyxb.binding.basis.element(pyxb.namespace.ExpandedName(Namespace, 'EncryptedData'), EncryptedDataType, location=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/xenc.xsd', 72, 2))
Namespace.addCategoryObject('elementBinding', EncryptedData.name().localName(), EncryptedData)

EncryptedKey = pyxb.binding.basis.element(pyxb.namespace.ExpandedName(Namespace, 'EncryptedKey'), EncryptedKeyType, location=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/xenc.xsd', 82, 2))
Namespace.addCategoryObject('elementBinding', EncryptedKey.name().localName(), EncryptedKey)



EncryptedType._AddElement(pyxb.binding.basis.element(pyxb.namespace.ExpandedName(_Namespace_ds, 'KeyInfo'), pyxb.bundles.wssplat.ds.KeyInfoType, scope=EncryptedType, location=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/ds.xsd', 144, 0)))

EncryptedType._AddElement(pyxb.binding.basis.element(pyxb.namespace.ExpandedName(Namespace, 'EncryptionMethod'), EncryptionMethodType, scope=EncryptedType, location=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/xenc.xsd', 24, 6)))

EncryptedType._AddElement(pyxb.binding.basis.element(pyxb.namespace.ExpandedName(Namespace, 'CipherData'), CipherDataType, scope=EncryptedType, location=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/xenc.xsd', 49, 2)))

EncryptedType._AddElement(pyxb.binding.basis.element(pyxb.namespace.ExpandedName(Namespace, 'EncryptionProperties'), EncryptionPropertiesType, scope=EncryptedType, location=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/xenc.xsd', 127, 2)))

def _BuildAutomaton ():
    # Remove this helper function from the namespace after it is invoked
    global _BuildAutomaton
    del _BuildAutomaton
    import pyxb.utils.fac as fac

    counters = set()
    cc_0 = fac.CounterCondition(min=0, max=1, metadata=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/xenc.xsd', 24, 6))
    counters.add(cc_0)
    cc_1 = fac.CounterCondition(min=0, max=1, metadata=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/xenc.xsd', 26, 6))
    counters.add(cc_1)
    cc_2 = fac.CounterCondition(min=0, max=1, metadata=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/xenc.xsd', 28, 6))
    counters.add(cc_2)
    states = []
    final_update = None
    symbol = pyxb.binding.content.ElementUse(EncryptedType._UseForTag(pyxb.namespace.ExpandedName(Namespace, 'EncryptionMethod')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/xenc.xsd', 24, 6))
    st_0 = fac.State(symbol, is_initial=True, final_update=final_update, is_unordered_catenation=False)
    states.append(st_0)
    final_update = None
    symbol = pyxb.binding.content.ElementUse(EncryptedType._UseForTag(pyxb.namespace.ExpandedName(_Namespace_ds, 'KeyInfo')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/xenc.xsd', 26, 6))
    st_1 = fac.State(symbol, is_initial=True, final_update=final_update, is_unordered_catenation=False)
    states.append(st_1)
    final_update = set()
    symbol = pyxb.binding.content.ElementUse(EncryptedType._UseForTag(pyxb.namespace.ExpandedName(Namespace, 'CipherData')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/xenc.xsd', 27, 6))
    st_2 = fac.State(symbol, is_initial=True, final_update=final_update, is_unordered_catenation=False)
    states.append(st_2)
    final_update = set()
    final_update.add(fac.UpdateInstruction(cc_2, False))
    symbol = pyxb.binding.content.ElementUse(EncryptedType._UseForTag(pyxb.namespace.ExpandedName(Namespace, 'EncryptionProperties')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/xenc.xsd', 28, 6))
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
EncryptedType._Automaton = _BuildAutomaton()




EncryptionMethodType._AddElement(pyxb.binding.basis.element(pyxb.namespace.ExpandedName(Namespace, 'KeySize'), KeySizeType, scope=EncryptionMethodType, location=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/xenc.xsd', 38, 6)))

EncryptionMethodType._AddElement(pyxb.binding.basis.element(pyxb.namespace.ExpandedName(Namespace, 'OAEPparams'), pyxb.binding.datatypes.base64Binary, scope=EncryptionMethodType, location=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/xenc.xsd', 39, 6)))

def _BuildAutomaton_ ():
    # Remove this helper function from the namespace after it is invoked
    global _BuildAutomaton_
    del _BuildAutomaton_
    import pyxb.utils.fac as fac

    counters = set()
    cc_0 = fac.CounterCondition(min=0, max=1, metadata=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/xenc.xsd', 38, 6))
    counters.add(cc_0)
    cc_1 = fac.CounterCondition(min=0, max=1, metadata=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/xenc.xsd', 39, 6))
    counters.add(cc_1)
    cc_2 = fac.CounterCondition(min=0, max=None, metadata=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/xenc.xsd', 40, 6))
    counters.add(cc_2)
    states = []
    final_update = set()
    final_update.add(fac.UpdateInstruction(cc_0, False))
    symbol = pyxb.binding.content.ElementUse(EncryptionMethodType._UseForTag(pyxb.namespace.ExpandedName(Namespace, 'KeySize')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/xenc.xsd', 38, 6))
    st_0 = fac.State(symbol, is_initial=True, final_update=final_update, is_unordered_catenation=False)
    states.append(st_0)
    final_update = set()
    final_update.add(fac.UpdateInstruction(cc_1, False))
    symbol = pyxb.binding.content.ElementUse(EncryptionMethodType._UseForTag(pyxb.namespace.ExpandedName(Namespace, 'OAEPparams')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/xenc.xsd', 39, 6))
    st_1 = fac.State(symbol, is_initial=True, final_update=final_update, is_unordered_catenation=False)
    states.append(st_1)
    final_update = set()
    final_update.add(fac.UpdateInstruction(cc_2, False))
    symbol = pyxb.binding.content.WildcardUse(pyxb.binding.content.Wildcard(process_contents=pyxb.binding.content.Wildcard.PC_strict, namespace_constraint=(pyxb.binding.content.Wildcard.NC_not, 'http://www.w3.org/2001/04/xmlenc#')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/xenc.xsd', 40, 6))
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
EncryptionMethodType._Automaton = _BuildAutomaton_()




CipherDataType._AddElement(pyxb.binding.basis.element(pyxb.namespace.ExpandedName(Namespace, 'CipherValue'), pyxb.binding.datatypes.base64Binary, scope=CipherDataType, location=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/xenc.xsd', 52, 7)))

CipherDataType._AddElement(pyxb.binding.basis.element(pyxb.namespace.ExpandedName(Namespace, 'CipherReference'), CipherReferenceType, scope=CipherDataType, location=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/xenc.xsd', 57, 3)))

def _BuildAutomaton_2 ():
    # Remove this helper function from the namespace after it is invoked
    global _BuildAutomaton_2
    del _BuildAutomaton_2
    import pyxb.utils.fac as fac

    counters = set()
    states = []
    final_update = set()
    symbol = pyxb.binding.content.ElementUse(CipherDataType._UseForTag(pyxb.namespace.ExpandedName(Namespace, 'CipherValue')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/xenc.xsd', 52, 7))
    st_0 = fac.State(symbol, is_initial=True, final_update=final_update, is_unordered_catenation=False)
    states.append(st_0)
    final_update = set()
    symbol = pyxb.binding.content.ElementUse(CipherDataType._UseForTag(pyxb.namespace.ExpandedName(Namespace, 'CipherReference')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/xenc.xsd', 53, 7))
    st_1 = fac.State(symbol, is_initial=True, final_update=final_update, is_unordered_catenation=False)
    states.append(st_1)
    transitions = []
    st_0._set_transitionSet(transitions)
    transitions = []
    st_1._set_transitionSet(transitions)
    return fac.Automaton(states, counters, False, containing_state=None)
CipherDataType._Automaton = _BuildAutomaton_2()




CipherReferenceType._AddElement(pyxb.binding.basis.element(pyxb.namespace.ExpandedName(Namespace, 'Transforms'), TransformsType, scope=CipherReferenceType, location=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/xenc.xsd', 60, 9)))

def _BuildAutomaton_3 ():
    # Remove this helper function from the namespace after it is invoked
    global _BuildAutomaton_3
    del _BuildAutomaton_3
    import pyxb.utils.fac as fac

    counters = set()
    cc_0 = fac.CounterCondition(min=0, max=1, metadata=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/xenc.xsd', 60, 9))
    counters.add(cc_0)
    states = []
    final_update = set()
    final_update.add(fac.UpdateInstruction(cc_0, False))
    symbol = pyxb.binding.content.ElementUse(CipherReferenceType._UseForTag(pyxb.namespace.ExpandedName(Namespace, 'Transforms')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/xenc.xsd', 60, 9))
    st_0 = fac.State(symbol, is_initial=True, final_update=final_update, is_unordered_catenation=False)
    states.append(st_0)
    transitions = []
    transitions.append(fac.Transition(st_0, [
        fac.UpdateInstruction(cc_0, True) ]))
    st_0._set_transitionSet(transitions)
    return fac.Automaton(states, counters, True, containing_state=None)
CipherReferenceType._Automaton = _BuildAutomaton_3()




TransformsType._AddElement(pyxb.binding.basis.element(pyxb.namespace.ExpandedName(_Namespace_ds, 'Transform'), pyxb.bundles.wssplat.ds.TransformType, scope=TransformsType, location=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/ds.xsd', 115, 2)))

def _BuildAutomaton_4 ():
    # Remove this helper function from the namespace after it is invoked
    global _BuildAutomaton_4
    del _BuildAutomaton_4
    import pyxb.utils.fac as fac

    counters = set()
    states = []
    final_update = set()
    symbol = pyxb.binding.content.ElementUse(TransformsType._UseForTag(pyxb.namespace.ExpandedName(_Namespace_ds, 'Transform')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/xenc.xsd', 67, 9))
    st_0 = fac.State(symbol, is_initial=True, final_update=final_update, is_unordered_catenation=False)
    states.append(st_0)
    transitions = []
    transitions.append(fac.Transition(st_0, [
         ]))
    st_0._set_transitionSet(transitions)
    return fac.Automaton(states, counters, False, containing_state=None)
TransformsType._Automaton = _BuildAutomaton_4()




AgreementMethodType._AddElement(pyxb.binding.basis.element(pyxb.namespace.ExpandedName(Namespace, 'KA-Nonce'), pyxb.binding.datatypes.base64Binary, scope=AgreementMethodType, location=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/xenc.xsd', 99, 8)))

AgreementMethodType._AddElement(pyxb.binding.basis.element(pyxb.namespace.ExpandedName(Namespace, 'OriginatorKeyInfo'), pyxb.bundles.wssplat.ds.KeyInfoType, scope=AgreementMethodType, location=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/xenc.xsd', 102, 8)))

AgreementMethodType._AddElement(pyxb.binding.basis.element(pyxb.namespace.ExpandedName(Namespace, 'RecipientKeyInfo'), pyxb.bundles.wssplat.ds.KeyInfoType, scope=AgreementMethodType, location=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/xenc.xsd', 103, 8)))

def _BuildAutomaton_5 ():
    # Remove this helper function from the namespace after it is invoked
    global _BuildAutomaton_5
    del _BuildAutomaton_5
    import pyxb.utils.fac as fac

    counters = set()
    cc_0 = fac.CounterCondition(min=0, max=1, metadata=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/xenc.xsd', 99, 8))
    counters.add(cc_0)
    cc_1 = fac.CounterCondition(min=0, max=None, metadata=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/xenc.xsd', 101, 8))
    counters.add(cc_1)
    cc_2 = fac.CounterCondition(min=0, max=1, metadata=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/xenc.xsd', 102, 8))
    counters.add(cc_2)
    cc_3 = fac.CounterCondition(min=0, max=1, metadata=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/xenc.xsd', 103, 8))
    counters.add(cc_3)
    states = []
    final_update = set()
    final_update.add(fac.UpdateInstruction(cc_0, False))
    symbol = pyxb.binding.content.ElementUse(AgreementMethodType._UseForTag(pyxb.namespace.ExpandedName(Namespace, 'KA-Nonce')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/xenc.xsd', 99, 8))
    st_0 = fac.State(symbol, is_initial=True, final_update=final_update, is_unordered_catenation=False)
    states.append(st_0)
    final_update = set()
    final_update.add(fac.UpdateInstruction(cc_1, False))
    symbol = pyxb.binding.content.WildcardUse(pyxb.binding.content.Wildcard(process_contents=pyxb.binding.content.Wildcard.PC_strict, namespace_constraint=(pyxb.binding.content.Wildcard.NC_not, 'http://www.w3.org/2001/04/xmlenc#')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/xenc.xsd', 101, 8))
    st_1 = fac.State(symbol, is_initial=True, final_update=final_update, is_unordered_catenation=False)
    states.append(st_1)
    final_update = set()
    final_update.add(fac.UpdateInstruction(cc_2, False))
    symbol = pyxb.binding.content.ElementUse(AgreementMethodType._UseForTag(pyxb.namespace.ExpandedName(Namespace, 'OriginatorKeyInfo')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/xenc.xsd', 102, 8))
    st_2 = fac.State(symbol, is_initial=True, final_update=final_update, is_unordered_catenation=False)
    states.append(st_2)
    final_update = set()
    final_update.add(fac.UpdateInstruction(cc_3, False))
    symbol = pyxb.binding.content.ElementUse(AgreementMethodType._UseForTag(pyxb.namespace.ExpandedName(Namespace, 'RecipientKeyInfo')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/xenc.xsd', 103, 8))
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
AgreementMethodType._Automaton = _BuildAutomaton_5()




CTD_ANON._AddElement(pyxb.binding.basis.element(pyxb.namespace.ExpandedName(Namespace, 'DataReference'), ReferenceType, scope=CTD_ANON, location=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/xenc.xsd', 113, 8)))

CTD_ANON._AddElement(pyxb.binding.basis.element(pyxb.namespace.ExpandedName(Namespace, 'KeyReference'), ReferenceType, scope=CTD_ANON, location=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/xenc.xsd', 114, 8)))

def _BuildAutomaton_6 ():
    # Remove this helper function from the namespace after it is invoked
    global _BuildAutomaton_6
    del _BuildAutomaton_6
    import pyxb.utils.fac as fac

    counters = set()
    states = []
    final_update = set()
    symbol = pyxb.binding.content.ElementUse(CTD_ANON._UseForTag(pyxb.namespace.ExpandedName(Namespace, 'DataReference')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/xenc.xsd', 113, 8))
    st_0 = fac.State(symbol, is_initial=True, final_update=final_update, is_unordered_catenation=False)
    states.append(st_0)
    final_update = set()
    symbol = pyxb.binding.content.ElementUse(CTD_ANON._UseForTag(pyxb.namespace.ExpandedName(Namespace, 'KeyReference')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/xenc.xsd', 114, 8))
    st_1 = fac.State(symbol, is_initial=True, final_update=final_update, is_unordered_catenation=False)
    states.append(st_1)
    transitions = []
    transitions.append(fac.Transition(st_0, [
         ]))
    transitions.append(fac.Transition(st_1, [
         ]))
    st_0._set_transitionSet(transitions)
    transitions = []
    transitions.append(fac.Transition(st_0, [
         ]))
    transitions.append(fac.Transition(st_1, [
         ]))
    st_1._set_transitionSet(transitions)
    return fac.Automaton(states, counters, False, containing_state=None)
CTD_ANON._Automaton = _BuildAutomaton_6()




def _BuildAutomaton_7 ():
    # Remove this helper function from the namespace after it is invoked
    global _BuildAutomaton_7
    del _BuildAutomaton_7
    import pyxb.utils.fac as fac

    counters = set()
    cc_0 = fac.CounterCondition(min=0, max=None, metadata=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/xenc.xsd', 121, 6))
    counters.add(cc_0)
    states = []
    final_update = set()
    final_update.add(fac.UpdateInstruction(cc_0, False))
    symbol = pyxb.binding.content.WildcardUse(pyxb.binding.content.Wildcard(process_contents=pyxb.binding.content.Wildcard.PC_strict, namespace_constraint=(pyxb.binding.content.Wildcard.NC_not, 'http://www.w3.org/2001/04/xmlenc#')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/xenc.xsd', 121, 6))
    st_0 = fac.State(symbol, is_initial=True, final_update=final_update, is_unordered_catenation=False)
    states.append(st_0)
    transitions = []
    transitions.append(fac.Transition(st_0, [
        fac.UpdateInstruction(cc_0, True) ]))
    st_0._set_transitionSet(transitions)
    return fac.Automaton(states, counters, True, containing_state=None)
ReferenceType._Automaton = _BuildAutomaton_7()




EncryptionPropertiesType._AddElement(pyxb.binding.basis.element(pyxb.namespace.ExpandedName(Namespace, 'EncryptionProperty'), EncryptionPropertyType, scope=EncryptionPropertiesType, location=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/xenc.xsd', 135, 4)))

def _BuildAutomaton_8 ():
    # Remove this helper function from the namespace after it is invoked
    global _BuildAutomaton_8
    del _BuildAutomaton_8
    import pyxb.utils.fac as fac

    counters = set()
    states = []
    final_update = set()
    symbol = pyxb.binding.content.ElementUse(EncryptionPropertiesType._UseForTag(pyxb.namespace.ExpandedName(Namespace, 'EncryptionProperty')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/xenc.xsd', 130, 6))
    st_0 = fac.State(symbol, is_initial=True, final_update=final_update, is_unordered_catenation=False)
    states.append(st_0)
    transitions = []
    transitions.append(fac.Transition(st_0, [
         ]))
    st_0._set_transitionSet(transitions)
    return fac.Automaton(states, counters, False, containing_state=None)
EncryptionPropertiesType._Automaton = _BuildAutomaton_8()




def _BuildAutomaton_9 ():
    # Remove this helper function from the namespace after it is invoked
    global _BuildAutomaton_9
    del _BuildAutomaton_9
    import pyxb.utils.fac as fac

    counters = set()
    states = []
    final_update = set()
    symbol = pyxb.binding.content.WildcardUse(pyxb.binding.content.Wildcard(process_contents=pyxb.binding.content.Wildcard.PC_lax, namespace_constraint=(pyxb.binding.content.Wildcard.NC_not, 'http://www.w3.org/2001/04/xmlenc#')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/xenc.xsd', 138, 8))
    st_0 = fac.State(symbol, is_initial=True, final_update=final_update, is_unordered_catenation=False)
    states.append(st_0)
    transitions = []
    transitions.append(fac.Transition(st_0, [
         ]))
    st_0._set_transitionSet(transitions)
    return fac.Automaton(states, counters, False, containing_state=None)
EncryptionPropertyType._Automaton = _BuildAutomaton_9()




def _BuildAutomaton_10 ():
    # Remove this helper function from the namespace after it is invoked
    global _BuildAutomaton_10
    del _BuildAutomaton_10
    import pyxb.utils.fac as fac

    counters = set()
    cc_0 = fac.CounterCondition(min=0, max=1, metadata=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/xenc.xsd', 24, 6))
    counters.add(cc_0)
    cc_1 = fac.CounterCondition(min=0, max=1, metadata=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/xenc.xsd', 26, 6))
    counters.add(cc_1)
    cc_2 = fac.CounterCondition(min=0, max=1, metadata=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/xenc.xsd', 28, 6))
    counters.add(cc_2)
    states = []
    final_update = None
    symbol = pyxb.binding.content.ElementUse(EncryptedDataType._UseForTag(pyxb.namespace.ExpandedName(Namespace, 'EncryptionMethod')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/xenc.xsd', 24, 6))
    st_0 = fac.State(symbol, is_initial=True, final_update=final_update, is_unordered_catenation=False)
    states.append(st_0)
    final_update = None
    symbol = pyxb.binding.content.ElementUse(EncryptedDataType._UseForTag(pyxb.namespace.ExpandedName(_Namespace_ds, 'KeyInfo')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/xenc.xsd', 26, 6))
    st_1 = fac.State(symbol, is_initial=True, final_update=final_update, is_unordered_catenation=False)
    states.append(st_1)
    final_update = set()
    symbol = pyxb.binding.content.ElementUse(EncryptedDataType._UseForTag(pyxb.namespace.ExpandedName(Namespace, 'CipherData')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/xenc.xsd', 27, 6))
    st_2 = fac.State(symbol, is_initial=True, final_update=final_update, is_unordered_catenation=False)
    states.append(st_2)
    final_update = set()
    final_update.add(fac.UpdateInstruction(cc_2, False))
    symbol = pyxb.binding.content.ElementUse(EncryptedDataType._UseForTag(pyxb.namespace.ExpandedName(Namespace, 'EncryptionProperties')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/xenc.xsd', 28, 6))
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
EncryptedDataType._Automaton = _BuildAutomaton_10()




EncryptedKeyType._AddElement(pyxb.binding.basis.element(pyxb.namespace.ExpandedName(Namespace, 'CarriedKeyName'), pyxb.binding.datatypes.string, scope=EncryptedKeyType, location=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/xenc.xsd', 88, 10)))

EncryptedKeyType._AddElement(pyxb.binding.basis.element(pyxb.namespace.ExpandedName(Namespace, 'ReferenceList'), CTD_ANON, scope=EncryptedKeyType, location=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/xenc.xsd', 110, 2)))

def _BuildAutomaton_11 ():
    # Remove this helper function from the namespace after it is invoked
    global _BuildAutomaton_11
    del _BuildAutomaton_11
    import pyxb.utils.fac as fac

    counters = set()
    cc_0 = fac.CounterCondition(min=0, max=1, metadata=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/xenc.xsd', 24, 6))
    counters.add(cc_0)
    cc_1 = fac.CounterCondition(min=0, max=1, metadata=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/xenc.xsd', 26, 6))
    counters.add(cc_1)
    cc_2 = fac.CounterCondition(min=0, max=1, metadata=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/xenc.xsd', 28, 6))
    counters.add(cc_2)
    cc_3 = fac.CounterCondition(min=0, max=1, metadata=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/xenc.xsd', 87, 10))
    counters.add(cc_3)
    cc_4 = fac.CounterCondition(min=0, max=1, metadata=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/xenc.xsd', 88, 10))
    counters.add(cc_4)
    states = []
    final_update = None
    symbol = pyxb.binding.content.ElementUse(EncryptedKeyType._UseForTag(pyxb.namespace.ExpandedName(Namespace, 'EncryptionMethod')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/xenc.xsd', 24, 6))
    st_0 = fac.State(symbol, is_initial=True, final_update=final_update, is_unordered_catenation=False)
    states.append(st_0)
    final_update = None
    symbol = pyxb.binding.content.ElementUse(EncryptedKeyType._UseForTag(pyxb.namespace.ExpandedName(_Namespace_ds, 'KeyInfo')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/xenc.xsd', 26, 6))
    st_1 = fac.State(symbol, is_initial=True, final_update=final_update, is_unordered_catenation=False)
    states.append(st_1)
    final_update = set()
    symbol = pyxb.binding.content.ElementUse(EncryptedKeyType._UseForTag(pyxb.namespace.ExpandedName(Namespace, 'CipherData')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/xenc.xsd', 27, 6))
    st_2 = fac.State(symbol, is_initial=True, final_update=final_update, is_unordered_catenation=False)
    states.append(st_2)
    final_update = set()
    final_update.add(fac.UpdateInstruction(cc_2, False))
    symbol = pyxb.binding.content.ElementUse(EncryptedKeyType._UseForTag(pyxb.namespace.ExpandedName(Namespace, 'EncryptionProperties')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/xenc.xsd', 28, 6))
    st_3 = fac.State(symbol, is_initial=False, final_update=final_update, is_unordered_catenation=False)
    states.append(st_3)
    final_update = set()
    final_update.add(fac.UpdateInstruction(cc_3, False))
    symbol = pyxb.binding.content.ElementUse(EncryptedKeyType._UseForTag(pyxb.namespace.ExpandedName(Namespace, 'ReferenceList')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/xenc.xsd', 87, 10))
    st_4 = fac.State(symbol, is_initial=False, final_update=final_update, is_unordered_catenation=False)
    states.append(st_4)
    final_update = set()
    final_update.add(fac.UpdateInstruction(cc_4, False))
    symbol = pyxb.binding.content.ElementUse(EncryptedKeyType._UseForTag(pyxb.namespace.ExpandedName(Namespace, 'CarriedKeyName')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/xenc.xsd', 88, 10))
    st_5 = fac.State(symbol, is_initial=False, final_update=final_update, is_unordered_catenation=False)
    states.append(st_5)
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
    transitions.append(fac.Transition(st_4, [
         ]))
    transitions.append(fac.Transition(st_5, [
         ]))
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
EncryptedKeyType._Automaton = _BuildAutomaton_11()

