# ./pyxb/bundles/wssplat/raw/soap12.py
# -*- coding: utf-8 -*-
# PyXB bindings for NM:d8b77ba08b421cd2387db6ece722305a4cdf2cdc
# Generated 2014-10-19 06:24:56.758239 by PyXB version 1.2.4 using Python 2.7.3.final.0
# Namespace http://www.w3.org/2003/05/soap-envelope

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
_GenerationUID = pyxb.utils.utility.UniqueIdentifier('urn:uuid:8dfa4f90-5782-11e4-b09e-c8600024e903')

# Version of PyXB used to generate the bindings
_PyXBVersion = '1.2.4'
# Generated bindings are not compatible across PyXB versions
if pyxb.__version__ != _PyXBVersion:
    raise pyxb.PyXBVersionError(_PyXBVersion)

# Import bindings for namespaces imported into schema
import pyxb.binding.datatypes
import pyxb.binding.xml_

# NOTE: All namespace declarations are reserved within the binding
Namespace = pyxb.namespace.NamespaceForURI('http://www.w3.org/2003/05/soap-envelope', create_if_missing=True)
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


# Atomic simple type: {http://www.w3.org/2003/05/soap-envelope}faultcodeEnum
class faultcodeEnum (pyxb.binding.datatypes.QName, pyxb.binding.basis.enumeration_mixin):

    """An atomic simple type."""

    _ExpandedName = pyxb.namespace.ExpandedName(Namespace, 'faultcodeEnum')
    _XSDLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/soap12.xsd', 112, 2)
    _Documentation = None
faultcodeEnum._CF_enumeration = pyxb.binding.facets.CF_enumeration(value_datatype=faultcodeEnum, enum_prefix=None)
faultcodeEnum._CF_enumeration.addEnumeration(value=pyxb.namespace.ExpandedName(Namespace, 'DataEncodingUnknown'), tag=None)
faultcodeEnum._CF_enumeration.addEnumeration(value=pyxb.namespace.ExpandedName(Namespace, 'MustUnderstand'), tag=None)
faultcodeEnum._CF_enumeration.addEnumeration(value=pyxb.namespace.ExpandedName(Namespace, 'Receiver'), tag=None)
faultcodeEnum._CF_enumeration.addEnumeration(value=pyxb.namespace.ExpandedName(Namespace, 'Sender'), tag=None)
faultcodeEnum._CF_enumeration.addEnumeration(value=pyxb.namespace.ExpandedName(Namespace, 'VersionMismatch'), tag=None)
faultcodeEnum._InitializeFacetMap(faultcodeEnum._CF_enumeration)
Namespace.addCategoryObject('typeBinding', 'faultcodeEnum', faultcodeEnum)

# Complex type {http://www.w3.org/2003/05/soap-envelope}Envelope with content type ELEMENT_ONLY
class Envelope_ (pyxb.binding.basis.complexTypeDefinition):
    """Complex type {http://www.w3.org/2003/05/soap-envelope}Envelope with content type ELEMENT_ONLY"""
    _TypeDefinition = None
    _ContentTypeTag = pyxb.binding.basis.complexTypeDefinition._CT_ELEMENT_ONLY
    _Abstract = False
    _ExpandedName = pyxb.namespace.ExpandedName(Namespace, 'Envelope')
    _XSDLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/soap12.xsd', 28, 2)
    _ElementMap = {}
    _AttributeMap = {}
    # Base type is pyxb.binding.datatypes.anyType
    
    # Element {http://www.w3.org/2003/05/soap-envelope}Header uses Python identifier Header
    __Header = pyxb.binding.content.ElementDeclaration(pyxb.namespace.ExpandedName(Namespace, 'Header'), 'Header', '__httpwww_w3_org200305soap_envelope_Envelope__httpwww_w3_org200305soap_envelopeHeader', False, pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/soap12.xsd', 36, 2), )

    
    Header = property(__Header.value, __Header.set, None, None)

    
    # Element {http://www.w3.org/2003/05/soap-envelope}Body uses Python identifier Body
    __Body = pyxb.binding.content.ElementDeclaration(pyxb.namespace.ExpandedName(Namespace, 'Body'), 'Body', '__httpwww_w3_org200305soap_envelope_Envelope__httpwww_w3_org200305soap_envelopeBody', False, pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/soap12.xsd', 49, 2), )

    
    Body = property(__Body.value, __Body.set, None, None)

    _AttributeWildcard = pyxb.binding.content.Wildcard(process_contents=pyxb.binding.content.Wildcard.PC_lax, namespace_constraint=(pyxb.binding.content.Wildcard.NC_not, 'http://www.w3.org/2003/05/soap-envelope'))
    _ElementMap.update({
        __Header.name() : __Header,
        __Body.name() : __Body
    })
    _AttributeMap.update({
        
    })
Namespace.addCategoryObject('typeBinding', 'Envelope', Envelope_)


# Complex type {http://www.w3.org/2003/05/soap-envelope}Header with content type ELEMENT_ONLY
class Header_ (pyxb.binding.basis.complexTypeDefinition):
    """
	  Elements replacing the wildcard MUST be namespace qualified, but can be in the targetNamespace
	  """
    _TypeDefinition = None
    _ContentTypeTag = pyxb.binding.basis.complexTypeDefinition._CT_ELEMENT_ONLY
    _Abstract = False
    _ExpandedName = pyxb.namespace.ExpandedName(Namespace, 'Header')
    _XSDLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/soap12.xsd', 37, 2)
    _ElementMap = {}
    _AttributeMap = {}
    # Base type is pyxb.binding.datatypes.anyType
    _AttributeWildcard = pyxb.binding.content.Wildcard(process_contents=pyxb.binding.content.Wildcard.PC_lax, namespace_constraint=(pyxb.binding.content.Wildcard.NC_not, 'http://www.w3.org/2003/05/soap-envelope'))
    _HasWildcardElement = True
    _ElementMap.update({
        
    })
    _AttributeMap.update({
        
    })
Namespace.addCategoryObject('typeBinding', 'Header', Header_)


# Complex type {http://www.w3.org/2003/05/soap-envelope}Body with content type ELEMENT_ONLY
class Body_ (pyxb.binding.basis.complexTypeDefinition):
    """Complex type {http://www.w3.org/2003/05/soap-envelope}Body with content type ELEMENT_ONLY"""
    _TypeDefinition = None
    _ContentTypeTag = pyxb.binding.basis.complexTypeDefinition._CT_ELEMENT_ONLY
    _Abstract = False
    _ExpandedName = pyxb.namespace.ExpandedName(Namespace, 'Body')
    _XSDLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/soap12.xsd', 50, 2)
    _ElementMap = {}
    _AttributeMap = {}
    # Base type is pyxb.binding.datatypes.anyType
    _AttributeWildcard = pyxb.binding.content.Wildcard(process_contents=pyxb.binding.content.Wildcard.PC_lax, namespace_constraint=(pyxb.binding.content.Wildcard.NC_not, 'http://www.w3.org/2003/05/soap-envelope'))
    _HasWildcardElement = True
    _ElementMap.update({
        
    })
    _AttributeMap.update({
        
    })
Namespace.addCategoryObject('typeBinding', 'Body', Body_)


# Complex type {http://www.w3.org/2003/05/soap-envelope}Fault with content type ELEMENT_ONLY
class Fault_ (pyxb.binding.basis.complexTypeDefinition):
    """
	    Fault reporting structure
	  """
    _TypeDefinition = None
    _ContentTypeTag = pyxb.binding.basis.complexTypeDefinition._CT_ELEMENT_ONLY
    _Abstract = False
    _ExpandedName = pyxb.namespace.ExpandedName(Namespace, 'Fault')
    _XSDLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/soap12.xsd', 72, 2)
    _ElementMap = {}
    _AttributeMap = {}
    # Base type is pyxb.binding.datatypes.anyType
    
    # Element {http://www.w3.org/2003/05/soap-envelope}Code uses Python identifier Code
    __Code = pyxb.binding.content.ElementDeclaration(pyxb.namespace.ExpandedName(Namespace, 'Code'), 'Code', '__httpwww_w3_org200305soap_envelope_Fault__httpwww_w3_org200305soap_envelopeCode', False, pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/soap12.xsd', 79, 6), )

    
    Code = property(__Code.value, __Code.set, None, None)

    
    # Element {http://www.w3.org/2003/05/soap-envelope}Reason uses Python identifier Reason
    __Reason = pyxb.binding.content.ElementDeclaration(pyxb.namespace.ExpandedName(Namespace, 'Reason'), 'Reason', '__httpwww_w3_org200305soap_envelope_Fault__httpwww_w3_org200305soap_envelopeReason', False, pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/soap12.xsd', 80, 6), )

    
    Reason = property(__Reason.value, __Reason.set, None, None)

    
    # Element {http://www.w3.org/2003/05/soap-envelope}Node uses Python identifier Node
    __Node = pyxb.binding.content.ElementDeclaration(pyxb.namespace.ExpandedName(Namespace, 'Node'), 'Node', '__httpwww_w3_org200305soap_envelope_Fault__httpwww_w3_org200305soap_envelopeNode', False, pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/soap12.xsd', 81, 6), )

    
    Node = property(__Node.value, __Node.set, None, None)

    
    # Element {http://www.w3.org/2003/05/soap-envelope}Role uses Python identifier Role
    __Role = pyxb.binding.content.ElementDeclaration(pyxb.namespace.ExpandedName(Namespace, 'Role'), 'Role', '__httpwww_w3_org200305soap_envelope_Fault__httpwww_w3_org200305soap_envelopeRole', False, pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/soap12.xsd', 82, 3), )

    
    Role = property(__Role.value, __Role.set, None, None)

    
    # Element {http://www.w3.org/2003/05/soap-envelope}Detail uses Python identifier Detail
    __Detail = pyxb.binding.content.ElementDeclaration(pyxb.namespace.ExpandedName(Namespace, 'Detail'), 'Detail', '__httpwww_w3_org200305soap_envelope_Fault__httpwww_w3_org200305soap_envelopeDetail', False, pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/soap12.xsd', 83, 6), )

    
    Detail = property(__Detail.value, __Detail.set, None, None)

    _ElementMap.update({
        __Code.name() : __Code,
        __Reason.name() : __Reason,
        __Node.name() : __Node,
        __Role.name() : __Role,
        __Detail.name() : __Detail
    })
    _AttributeMap.update({
        
    })
Namespace.addCategoryObject('typeBinding', 'Fault', Fault_)


# Complex type {http://www.w3.org/2003/05/soap-envelope}faultreason with content type ELEMENT_ONLY
class faultreason (pyxb.binding.basis.complexTypeDefinition):
    """Complex type {http://www.w3.org/2003/05/soap-envelope}faultreason with content type ELEMENT_ONLY"""
    _TypeDefinition = None
    _ContentTypeTag = pyxb.binding.basis.complexTypeDefinition._CT_ELEMENT_ONLY
    _Abstract = False
    _ExpandedName = pyxb.namespace.ExpandedName(Namespace, 'faultreason')
    _XSDLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/soap12.xsd', 87, 2)
    _ElementMap = {}
    _AttributeMap = {}
    # Base type is pyxb.binding.datatypes.anyType
    
    # Element {http://www.w3.org/2003/05/soap-envelope}Text uses Python identifier Text
    __Text = pyxb.binding.content.ElementDeclaration(pyxb.namespace.ExpandedName(Namespace, 'Text'), 'Text', '__httpwww_w3_org200305soap_envelope_faultreason_httpwww_w3_org200305soap_envelopeText', True, pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/soap12.xsd', 89, 3), )

    
    Text = property(__Text.value, __Text.set, None, None)

    _ElementMap.update({
        __Text.name() : __Text
    })
    _AttributeMap.update({
        
    })
Namespace.addCategoryObject('typeBinding', 'faultreason', faultreason)


# Complex type {http://www.w3.org/2003/05/soap-envelope}reasontext with content type SIMPLE
class reasontext (pyxb.binding.basis.complexTypeDefinition):
    """Complex type {http://www.w3.org/2003/05/soap-envelope}reasontext with content type SIMPLE"""
    _TypeDefinition = pyxb.binding.datatypes.string
    _ContentTypeTag = pyxb.binding.basis.complexTypeDefinition._CT_SIMPLE
    _Abstract = False
    _ExpandedName = pyxb.namespace.ExpandedName(Namespace, 'reasontext')
    _XSDLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/soap12.xsd', 94, 2)
    _ElementMap = {}
    _AttributeMap = {}
    # Base type is pyxb.binding.datatypes.string
    
    # Attribute {http://www.w3.org/XML/1998/namespace}lang uses Python identifier lang
    __lang = pyxb.binding.content.AttributeUse(pyxb.namespace.ExpandedName(pyxb.namespace.XML, 'lang'), 'lang', '__httpwww_w3_org200305soap_envelope_reasontext_httpwww_w3_orgXML1998namespacelang', pyxb.binding.xml_.STD_ANON_lang, required=True)
    __lang._DeclarationLocation = None
    __lang._UseLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/soap12.xsd', 97, 5)
    
    lang = property(__lang.value, __lang.set, None, None)

    _ElementMap.update({
        
    })
    _AttributeMap.update({
        __lang.name() : __lang
    })
Namespace.addCategoryObject('typeBinding', 'reasontext', reasontext)


# Complex type {http://www.w3.org/2003/05/soap-envelope}faultcode with content type ELEMENT_ONLY
class faultcode (pyxb.binding.basis.complexTypeDefinition):
    """Complex type {http://www.w3.org/2003/05/soap-envelope}faultcode with content type ELEMENT_ONLY"""
    _TypeDefinition = None
    _ContentTypeTag = pyxb.binding.basis.complexTypeDefinition._CT_ELEMENT_ONLY
    _Abstract = False
    _ExpandedName = pyxb.namespace.ExpandedName(Namespace, 'faultcode')
    _XSDLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/soap12.xsd', 102, 2)
    _ElementMap = {}
    _AttributeMap = {}
    # Base type is pyxb.binding.datatypes.anyType
    
    # Element {http://www.w3.org/2003/05/soap-envelope}Value uses Python identifier Value
    __Value = pyxb.binding.content.ElementDeclaration(pyxb.namespace.ExpandedName(Namespace, 'Value'), 'Value', '__httpwww_w3_org200305soap_envelope_faultcode_httpwww_w3_org200305soap_envelopeValue', False, pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/soap12.xsd', 104, 6), )

    
    Value = property(__Value.value, __Value.set, None, None)

    
    # Element {http://www.w3.org/2003/05/soap-envelope}Subcode uses Python identifier Subcode
    __Subcode = pyxb.binding.content.ElementDeclaration(pyxb.namespace.ExpandedName(Namespace, 'Subcode'), 'Subcode', '__httpwww_w3_org200305soap_envelope_faultcode_httpwww_w3_org200305soap_envelopeSubcode', False, pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/soap12.xsd', 106, 6), )

    
    Subcode = property(__Subcode.value, __Subcode.set, None, None)

    _ElementMap.update({
        __Value.name() : __Value,
        __Subcode.name() : __Subcode
    })
    _AttributeMap.update({
        
    })
Namespace.addCategoryObject('typeBinding', 'faultcode', faultcode)


# Complex type {http://www.w3.org/2003/05/soap-envelope}subcode with content type ELEMENT_ONLY
class subcode (pyxb.binding.basis.complexTypeDefinition):
    """Complex type {http://www.w3.org/2003/05/soap-envelope}subcode with content type ELEMENT_ONLY"""
    _TypeDefinition = None
    _ContentTypeTag = pyxb.binding.basis.complexTypeDefinition._CT_ELEMENT_ONLY
    _Abstract = False
    _ExpandedName = pyxb.namespace.ExpandedName(Namespace, 'subcode')
    _XSDLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/soap12.xsd', 122, 2)
    _ElementMap = {}
    _AttributeMap = {}
    # Base type is pyxb.binding.datatypes.anyType
    
    # Element {http://www.w3.org/2003/05/soap-envelope}Value uses Python identifier Value
    __Value = pyxb.binding.content.ElementDeclaration(pyxb.namespace.ExpandedName(Namespace, 'Value'), 'Value', '__httpwww_w3_org200305soap_envelope_subcode_httpwww_w3_org200305soap_envelopeValue', False, pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/soap12.xsd', 124, 6), )

    
    Value = property(__Value.value, __Value.set, None, None)

    
    # Element {http://www.w3.org/2003/05/soap-envelope}Subcode uses Python identifier Subcode
    __Subcode = pyxb.binding.content.ElementDeclaration(pyxb.namespace.ExpandedName(Namespace, 'Subcode'), 'Subcode', '__httpwww_w3_org200305soap_envelope_subcode_httpwww_w3_org200305soap_envelopeSubcode', False, pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/soap12.xsd', 126, 6), )

    
    Subcode = property(__Subcode.value, __Subcode.set, None, None)

    _ElementMap.update({
        __Value.name() : __Value,
        __Subcode.name() : __Subcode
    })
    _AttributeMap.update({
        
    })
Namespace.addCategoryObject('typeBinding', 'subcode', subcode)


# Complex type {http://www.w3.org/2003/05/soap-envelope}detail with content type ELEMENT_ONLY
class detail (pyxb.binding.basis.complexTypeDefinition):
    """Complex type {http://www.w3.org/2003/05/soap-envelope}detail with content type ELEMENT_ONLY"""
    _TypeDefinition = None
    _ContentTypeTag = pyxb.binding.basis.complexTypeDefinition._CT_ELEMENT_ONLY
    _Abstract = False
    _ExpandedName = pyxb.namespace.ExpandedName(Namespace, 'detail')
    _XSDLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/soap12.xsd', 132, 2)
    _ElementMap = {}
    _AttributeMap = {}
    # Base type is pyxb.binding.datatypes.anyType
    _AttributeWildcard = pyxb.binding.content.Wildcard(process_contents=pyxb.binding.content.Wildcard.PC_lax, namespace_constraint=(pyxb.binding.content.Wildcard.NC_not, 'http://www.w3.org/2003/05/soap-envelope'))
    _HasWildcardElement = True
    _ElementMap.update({
        
    })
    _AttributeMap.update({
        
    })
Namespace.addCategoryObject('typeBinding', 'detail', detail)


# Complex type {http://www.w3.org/2003/05/soap-envelope}NotUnderstoodType with content type EMPTY
class NotUnderstoodType (pyxb.binding.basis.complexTypeDefinition):
    """Complex type {http://www.w3.org/2003/05/soap-envelope}NotUnderstoodType with content type EMPTY"""
    _TypeDefinition = None
    _ContentTypeTag = pyxb.binding.basis.complexTypeDefinition._CT_EMPTY
    _Abstract = False
    _ExpandedName = pyxb.namespace.ExpandedName(Namespace, 'NotUnderstoodType')
    _XSDLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/soap12.xsd', 141, 2)
    _ElementMap = {}
    _AttributeMap = {}
    # Base type is pyxb.binding.datatypes.anyType
    
    # Attribute qname uses Python identifier qname
    __qname = pyxb.binding.content.AttributeUse(pyxb.namespace.ExpandedName(None, 'qname'), 'qname', '__httpwww_w3_org200305soap_envelope_NotUnderstoodType_qname', pyxb.binding.datatypes.QName, required=True)
    __qname._DeclarationLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/soap12.xsd', 142, 4)
    __qname._UseLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/soap12.xsd', 142, 4)
    
    qname = property(__qname.value, __qname.set, None, None)

    _ElementMap.update({
        
    })
    _AttributeMap.update({
        __qname.name() : __qname
    })
Namespace.addCategoryObject('typeBinding', 'NotUnderstoodType', NotUnderstoodType)


# Complex type {http://www.w3.org/2003/05/soap-envelope}SupportedEnvType with content type EMPTY
class SupportedEnvType (pyxb.binding.basis.complexTypeDefinition):
    """Complex type {http://www.w3.org/2003/05/soap-envelope}SupportedEnvType with content type EMPTY"""
    _TypeDefinition = None
    _ContentTypeTag = pyxb.binding.basis.complexTypeDefinition._CT_EMPTY
    _Abstract = False
    _ExpandedName = pyxb.namespace.ExpandedName(Namespace, 'SupportedEnvType')
    _XSDLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/soap12.xsd', 146, 154)
    _ElementMap = {}
    _AttributeMap = {}
    # Base type is pyxb.binding.datatypes.anyType
    
    # Attribute qname uses Python identifier qname
    __qname = pyxb.binding.content.AttributeUse(pyxb.namespace.ExpandedName(None, 'qname'), 'qname', '__httpwww_w3_org200305soap_envelope_SupportedEnvType_qname', pyxb.binding.datatypes.QName, required=True)
    __qname._DeclarationLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/soap12.xsd', 147, 4)
    __qname._UseLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/soap12.xsd', 147, 4)
    
    qname = property(__qname.value, __qname.set, None, None)

    _ElementMap.update({
        
    })
    _AttributeMap.update({
        __qname.name() : __qname
    })
Namespace.addCategoryObject('typeBinding', 'SupportedEnvType', SupportedEnvType)


# Complex type {http://www.w3.org/2003/05/soap-envelope}UpgradeType with content type ELEMENT_ONLY
class UpgradeType (pyxb.binding.basis.complexTypeDefinition):
    """Complex type {http://www.w3.org/2003/05/soap-envelope}UpgradeType with content type ELEMENT_ONLY"""
    _TypeDefinition = None
    _ContentTypeTag = pyxb.binding.basis.complexTypeDefinition._CT_ELEMENT_ONLY
    _Abstract = False
    _ExpandedName = pyxb.namespace.ExpandedName(Namespace, 'UpgradeType')
    _XSDLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/soap12.xsd', 151, 2)
    _ElementMap = {}
    _AttributeMap = {}
    # Base type is pyxb.binding.datatypes.anyType
    
    # Element {http://www.w3.org/2003/05/soap-envelope}SupportedEnvelope uses Python identifier SupportedEnvelope
    __SupportedEnvelope = pyxb.binding.content.ElementDeclaration(pyxb.namespace.ExpandedName(Namespace, 'SupportedEnvelope'), 'SupportedEnvelope', '__httpwww_w3_org200305soap_envelope_UpgradeType_httpwww_w3_org200305soap_envelopeSupportedEnvelope', True, pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/soap12.xsd', 153, 3), )

    
    SupportedEnvelope = property(__SupportedEnvelope.value, __SupportedEnvelope.set, None, None)

    _ElementMap.update({
        __SupportedEnvelope.name() : __SupportedEnvelope
    })
    _AttributeMap.update({
        
    })
Namespace.addCategoryObject('typeBinding', 'UpgradeType', UpgradeType)


Envelope = pyxb.binding.basis.element(pyxb.namespace.ExpandedName(Namespace, 'Envelope'), Envelope_, location=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/soap12.xsd', 27, 2))
Namespace.addCategoryObject('elementBinding', Envelope.name().localName(), Envelope)

Header = pyxb.binding.basis.element(pyxb.namespace.ExpandedName(Namespace, 'Header'), Header_, location=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/soap12.xsd', 36, 2))
Namespace.addCategoryObject('elementBinding', Header.name().localName(), Header)

Body = pyxb.binding.basis.element(pyxb.namespace.ExpandedName(Namespace, 'Body'), Body_, location=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/soap12.xsd', 49, 2))
Namespace.addCategoryObject('elementBinding', Body.name().localName(), Body)

Fault = pyxb.binding.basis.element(pyxb.namespace.ExpandedName(Namespace, 'Fault'), Fault_, location=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/soap12.xsd', 71, 2))
Namespace.addCategoryObject('elementBinding', Fault.name().localName(), Fault)

NotUnderstood = pyxb.binding.basis.element(pyxb.namespace.ExpandedName(Namespace, 'NotUnderstood'), NotUnderstoodType, location=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/soap12.xsd', 140, 2))
Namespace.addCategoryObject('elementBinding', NotUnderstood.name().localName(), NotUnderstood)

Upgrade = pyxb.binding.basis.element(pyxb.namespace.ExpandedName(Namespace, 'Upgrade'), UpgradeType, location=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/soap12.xsd', 150, 2))
Namespace.addCategoryObject('elementBinding', Upgrade.name().localName(), Upgrade)



Envelope_._AddElement(pyxb.binding.basis.element(pyxb.namespace.ExpandedName(Namespace, 'Header'), Header_, scope=Envelope_, location=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/soap12.xsd', 36, 2)))

Envelope_._AddElement(pyxb.binding.basis.element(pyxb.namespace.ExpandedName(Namespace, 'Body'), Body_, scope=Envelope_, location=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/soap12.xsd', 49, 2)))

def _BuildAutomaton ():
    # Remove this helper function from the namespace after it is invoked
    global _BuildAutomaton
    del _BuildAutomaton
    import pyxb.utils.fac as fac

    counters = set()
    cc_0 = fac.CounterCondition(min=0, max=1, metadata=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/soap12.xsd', 30, 6))
    counters.add(cc_0)
    states = []
    final_update = None
    symbol = pyxb.binding.content.ElementUse(Envelope_._UseForTag(pyxb.namespace.ExpandedName(Namespace, 'Header')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/soap12.xsd', 30, 6))
    st_0 = fac.State(symbol, is_initial=True, final_update=final_update, is_unordered_catenation=False)
    states.append(st_0)
    final_update = set()
    symbol = pyxb.binding.content.ElementUse(Envelope_._UseForTag(pyxb.namespace.ExpandedName(Namespace, 'Body')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/soap12.xsd', 31, 6))
    st_1 = fac.State(symbol, is_initial=True, final_update=final_update, is_unordered_catenation=False)
    states.append(st_1)
    transitions = []
    transitions.append(fac.Transition(st_0, [
        fac.UpdateInstruction(cc_0, True) ]))
    transitions.append(fac.Transition(st_1, [
        fac.UpdateInstruction(cc_0, False) ]))
    st_0._set_transitionSet(transitions)
    transitions = []
    st_1._set_transitionSet(transitions)
    return fac.Automaton(states, counters, False, containing_state=None)
Envelope_._Automaton = _BuildAutomaton()




def _BuildAutomaton_ ():
    # Remove this helper function from the namespace after it is invoked
    global _BuildAutomaton_
    del _BuildAutomaton_
    import pyxb.utils.fac as fac

    counters = set()
    cc_0 = fac.CounterCondition(min=0, max=None, metadata=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/soap12.xsd', 44, 6))
    counters.add(cc_0)
    states = []
    final_update = set()
    final_update.add(fac.UpdateInstruction(cc_0, False))
    symbol = pyxb.binding.content.WildcardUse(pyxb.binding.content.Wildcard(process_contents=pyxb.binding.content.Wildcard.PC_lax, namespace_constraint=pyxb.binding.content.Wildcard.NC_any), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/soap12.xsd', 44, 6))
    st_0 = fac.State(symbol, is_initial=True, final_update=final_update, is_unordered_catenation=False)
    states.append(st_0)
    transitions = []
    transitions.append(fac.Transition(st_0, [
        fac.UpdateInstruction(cc_0, True) ]))
    st_0._set_transitionSet(transitions)
    return fac.Automaton(states, counters, True, containing_state=None)
Header_._Automaton = _BuildAutomaton_()




def _BuildAutomaton_2 ():
    # Remove this helper function from the namespace after it is invoked
    global _BuildAutomaton_2
    del _BuildAutomaton_2
    import pyxb.utils.fac as fac

    counters = set()
    cc_0 = fac.CounterCondition(min=0, max=None, metadata=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/soap12.xsd', 52, 6))
    counters.add(cc_0)
    states = []
    final_update = set()
    final_update.add(fac.UpdateInstruction(cc_0, False))
    symbol = pyxb.binding.content.WildcardUse(pyxb.binding.content.Wildcard(process_contents=pyxb.binding.content.Wildcard.PC_lax, namespace_constraint=pyxb.binding.content.Wildcard.NC_any), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/soap12.xsd', 52, 6))
    st_0 = fac.State(symbol, is_initial=True, final_update=final_update, is_unordered_catenation=False)
    states.append(st_0)
    transitions = []
    transitions.append(fac.Transition(st_0, [
        fac.UpdateInstruction(cc_0, True) ]))
    st_0._set_transitionSet(transitions)
    return fac.Automaton(states, counters, True, containing_state=None)
Body_._Automaton = _BuildAutomaton_2()




Fault_._AddElement(pyxb.binding.basis.element(pyxb.namespace.ExpandedName(Namespace, 'Code'), faultcode, scope=Fault_, location=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/soap12.xsd', 79, 6)))

Fault_._AddElement(pyxb.binding.basis.element(pyxb.namespace.ExpandedName(Namespace, 'Reason'), faultreason, scope=Fault_, location=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/soap12.xsd', 80, 6)))

Fault_._AddElement(pyxb.binding.basis.element(pyxb.namespace.ExpandedName(Namespace, 'Node'), pyxb.binding.datatypes.anyURI, scope=Fault_, location=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/soap12.xsd', 81, 6)))

Fault_._AddElement(pyxb.binding.basis.element(pyxb.namespace.ExpandedName(Namespace, 'Role'), pyxb.binding.datatypes.anyURI, scope=Fault_, location=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/soap12.xsd', 82, 3)))

Fault_._AddElement(pyxb.binding.basis.element(pyxb.namespace.ExpandedName(Namespace, 'Detail'), detail, scope=Fault_, location=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/soap12.xsd', 83, 6)))

def _BuildAutomaton_3 ():
    # Remove this helper function from the namespace after it is invoked
    global _BuildAutomaton_3
    del _BuildAutomaton_3
    import pyxb.utils.fac as fac

    counters = set()
    cc_0 = fac.CounterCondition(min=0, max=1, metadata=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/soap12.xsd', 81, 6))
    counters.add(cc_0)
    cc_1 = fac.CounterCondition(min=0, max=1, metadata=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/soap12.xsd', 82, 3))
    counters.add(cc_1)
    cc_2 = fac.CounterCondition(min=0, max=1, metadata=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/soap12.xsd', 83, 6))
    counters.add(cc_2)
    states = []
    final_update = None
    symbol = pyxb.binding.content.ElementUse(Fault_._UseForTag(pyxb.namespace.ExpandedName(Namespace, 'Code')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/soap12.xsd', 79, 6))
    st_0 = fac.State(symbol, is_initial=True, final_update=final_update, is_unordered_catenation=False)
    states.append(st_0)
    final_update = set()
    symbol = pyxb.binding.content.ElementUse(Fault_._UseForTag(pyxb.namespace.ExpandedName(Namespace, 'Reason')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/soap12.xsd', 80, 6))
    st_1 = fac.State(symbol, is_initial=False, final_update=final_update, is_unordered_catenation=False)
    states.append(st_1)
    final_update = set()
    final_update.add(fac.UpdateInstruction(cc_0, False))
    symbol = pyxb.binding.content.ElementUse(Fault_._UseForTag(pyxb.namespace.ExpandedName(Namespace, 'Node')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/soap12.xsd', 81, 6))
    st_2 = fac.State(symbol, is_initial=False, final_update=final_update, is_unordered_catenation=False)
    states.append(st_2)
    final_update = set()
    final_update.add(fac.UpdateInstruction(cc_1, False))
    symbol = pyxb.binding.content.ElementUse(Fault_._UseForTag(pyxb.namespace.ExpandedName(Namespace, 'Role')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/soap12.xsd', 82, 3))
    st_3 = fac.State(symbol, is_initial=False, final_update=final_update, is_unordered_catenation=False)
    states.append(st_3)
    final_update = set()
    final_update.add(fac.UpdateInstruction(cc_2, False))
    symbol = pyxb.binding.content.ElementUse(Fault_._UseForTag(pyxb.namespace.ExpandedName(Namespace, 'Detail')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/soap12.xsd', 83, 6))
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
Fault_._Automaton = _BuildAutomaton_3()




faultreason._AddElement(pyxb.binding.basis.element(pyxb.namespace.ExpandedName(Namespace, 'Text'), reasontext, scope=faultreason, location=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/soap12.xsd', 89, 3)))

def _BuildAutomaton_4 ():
    # Remove this helper function from the namespace after it is invoked
    global _BuildAutomaton_4
    del _BuildAutomaton_4
    import pyxb.utils.fac as fac

    counters = set()
    states = []
    final_update = set()
    symbol = pyxb.binding.content.ElementUse(faultreason._UseForTag(pyxb.namespace.ExpandedName(Namespace, 'Text')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/soap12.xsd', 89, 3))
    st_0 = fac.State(symbol, is_initial=True, final_update=final_update, is_unordered_catenation=False)
    states.append(st_0)
    transitions = []
    transitions.append(fac.Transition(st_0, [
         ]))
    st_0._set_transitionSet(transitions)
    return fac.Automaton(states, counters, False, containing_state=None)
faultreason._Automaton = _BuildAutomaton_4()




faultcode._AddElement(pyxb.binding.basis.element(pyxb.namespace.ExpandedName(Namespace, 'Value'), faultcodeEnum, scope=faultcode, location=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/soap12.xsd', 104, 6)))

faultcode._AddElement(pyxb.binding.basis.element(pyxb.namespace.ExpandedName(Namespace, 'Subcode'), subcode, scope=faultcode, location=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/soap12.xsd', 106, 6)))

def _BuildAutomaton_5 ():
    # Remove this helper function from the namespace after it is invoked
    global _BuildAutomaton_5
    del _BuildAutomaton_5
    import pyxb.utils.fac as fac

    counters = set()
    cc_0 = fac.CounterCondition(min=0, max=1, metadata=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/soap12.xsd', 106, 6))
    counters.add(cc_0)
    states = []
    final_update = set()
    symbol = pyxb.binding.content.ElementUse(faultcode._UseForTag(pyxb.namespace.ExpandedName(Namespace, 'Value')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/soap12.xsd', 104, 6))
    st_0 = fac.State(symbol, is_initial=True, final_update=final_update, is_unordered_catenation=False)
    states.append(st_0)
    final_update = set()
    final_update.add(fac.UpdateInstruction(cc_0, False))
    symbol = pyxb.binding.content.ElementUse(faultcode._UseForTag(pyxb.namespace.ExpandedName(Namespace, 'Subcode')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/soap12.xsd', 106, 6))
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
faultcode._Automaton = _BuildAutomaton_5()




subcode._AddElement(pyxb.binding.basis.element(pyxb.namespace.ExpandedName(Namespace, 'Value'), pyxb.binding.datatypes.QName, scope=subcode, location=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/soap12.xsd', 124, 6)))

subcode._AddElement(pyxb.binding.basis.element(pyxb.namespace.ExpandedName(Namespace, 'Subcode'), subcode, scope=subcode, location=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/soap12.xsd', 126, 6)))

def _BuildAutomaton_6 ():
    # Remove this helper function from the namespace after it is invoked
    global _BuildAutomaton_6
    del _BuildAutomaton_6
    import pyxb.utils.fac as fac

    counters = set()
    cc_0 = fac.CounterCondition(min=0, max=1, metadata=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/soap12.xsd', 126, 6))
    counters.add(cc_0)
    states = []
    final_update = set()
    symbol = pyxb.binding.content.ElementUse(subcode._UseForTag(pyxb.namespace.ExpandedName(Namespace, 'Value')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/soap12.xsd', 124, 6))
    st_0 = fac.State(symbol, is_initial=True, final_update=final_update, is_unordered_catenation=False)
    states.append(st_0)
    final_update = set()
    final_update.add(fac.UpdateInstruction(cc_0, False))
    symbol = pyxb.binding.content.ElementUse(subcode._UseForTag(pyxb.namespace.ExpandedName(Namespace, 'Subcode')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/soap12.xsd', 126, 6))
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
subcode._Automaton = _BuildAutomaton_6()




def _BuildAutomaton_7 ():
    # Remove this helper function from the namespace after it is invoked
    global _BuildAutomaton_7
    del _BuildAutomaton_7
    import pyxb.utils.fac as fac

    counters = set()
    cc_0 = fac.CounterCondition(min=0, max=None, metadata=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/soap12.xsd', 134, 6))
    counters.add(cc_0)
    states = []
    final_update = set()
    final_update.add(fac.UpdateInstruction(cc_0, False))
    symbol = pyxb.binding.content.WildcardUse(pyxb.binding.content.Wildcard(process_contents=pyxb.binding.content.Wildcard.PC_lax, namespace_constraint=pyxb.binding.content.Wildcard.NC_any), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/soap12.xsd', 134, 6))
    st_0 = fac.State(symbol, is_initial=True, final_update=final_update, is_unordered_catenation=False)
    states.append(st_0)
    transitions = []
    transitions.append(fac.Transition(st_0, [
        fac.UpdateInstruction(cc_0, True) ]))
    st_0._set_transitionSet(transitions)
    return fac.Automaton(states, counters, True, containing_state=None)
detail._Automaton = _BuildAutomaton_7()




UpgradeType._AddElement(pyxb.binding.basis.element(pyxb.namespace.ExpandedName(Namespace, 'SupportedEnvelope'), SupportedEnvType, scope=UpgradeType, location=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/soap12.xsd', 153, 3)))

def _BuildAutomaton_8 ():
    # Remove this helper function from the namespace after it is invoked
    global _BuildAutomaton_8
    del _BuildAutomaton_8
    import pyxb.utils.fac as fac

    counters = set()
    states = []
    final_update = set()
    symbol = pyxb.binding.content.ElementUse(UpgradeType._UseForTag(pyxb.namespace.ExpandedName(Namespace, 'SupportedEnvelope')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/soap12.xsd', 153, 3))
    st_0 = fac.State(symbol, is_initial=True, final_update=final_update, is_unordered_catenation=False)
    states.append(st_0)
    transitions = []
    transitions.append(fac.Transition(st_0, [
         ]))
    st_0._set_transitionSet(transitions)
    return fac.Automaton(states, counters, False, containing_state=None)
UpgradeType._Automaton = _BuildAutomaton_8()

