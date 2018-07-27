# ./pyxb/bundles/saml20/raw/dce.py
# -*- coding: utf-8 -*-
# PyXB bindings for NM:f6fa0461f265c04a0bd0017089ca6057a2aade76
# Generated 2014-10-19 06:25:02.209376 by PyXB version 1.2.4 using Python 2.7.3.final.0
# Namespace urn:oasis:names:tc:SAML:2.0:profiles:attribute:DCE

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
_GenerationUID = pyxb.utils.utility.UniqueIdentifier('urn:uuid:913c4b54-5782-11e4-9ec3-c8600024e903')

# Version of PyXB used to generate the bindings
_PyXBVersion = '1.2.4'
# Generated bindings are not compatible across PyXB versions
if pyxb.__version__ != _PyXBVersion:
    raise pyxb.PyXBVersionError(_PyXBVersion)

# Import bindings for namespaces imported into schema
import pyxb.binding.datatypes

# NOTE: All namespace declarations are reserved within the binding
Namespace = pyxb.namespace.NamespaceForURI('urn:oasis:names:tc:SAML:2.0:profiles:attribute:DCE', create_if_missing=True)
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


# Complex type {urn:oasis:names:tc:SAML:2.0:profiles:attribute:DCE}DCEValueType with content type SIMPLE
class DCEValueType (pyxb.binding.basis.complexTypeDefinition):
    """Complex type {urn:oasis:names:tc:SAML:2.0:profiles:attribute:DCE}DCEValueType with content type SIMPLE"""
    _TypeDefinition = pyxb.binding.datatypes.anyURI
    _ContentTypeTag = pyxb.binding.basis.complexTypeDefinition._CT_SIMPLE
    _Abstract = False
    _ExpandedName = pyxb.namespace.ExpandedName(Namespace, 'DCEValueType')
    _XSDLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/saml20/schemas/dce.xsd', 18, 4)
    _ElementMap = {}
    _AttributeMap = {}
    # Base type is pyxb.binding.datatypes.anyURI
    
    # Attribute {urn:oasis:names:tc:SAML:2.0:profiles:attribute:DCE}Realm uses Python identifier Realm
    __Realm = pyxb.binding.content.AttributeUse(pyxb.namespace.ExpandedName(Namespace, 'Realm'), 'Realm', '__urnoasisnamestcSAML2_0profilesattributeDCE_DCEValueType_urnoasisnamestcSAML2_0profilesattributeDCERealm', pyxb.binding.datatypes.anyURI)
    __Realm._DeclarationLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/saml20/schemas/dce.xsd', 26, 4)
    __Realm._UseLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/saml20/schemas/dce.xsd', 21, 16)
    
    Realm = property(__Realm.value, __Realm.set, None, None)

    
    # Attribute {urn:oasis:names:tc:SAML:2.0:profiles:attribute:DCE}FriendlyName uses Python identifier FriendlyName
    __FriendlyName = pyxb.binding.content.AttributeUse(pyxb.namespace.ExpandedName(Namespace, 'FriendlyName'), 'FriendlyName', '__urnoasisnamestcSAML2_0profilesattributeDCE_DCEValueType_urnoasisnamestcSAML2_0profilesattributeDCEFriendlyName', pyxb.binding.datatypes.string)
    __FriendlyName._DeclarationLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/saml20/schemas/dce.xsd', 27, 4)
    __FriendlyName._UseLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/saml20/schemas/dce.xsd', 22, 16)
    
    FriendlyName = property(__FriendlyName.value, __FriendlyName.set, None, None)

    _ElementMap.update({
        
    })
    _AttributeMap.update({
        __Realm.name() : __Realm,
        __FriendlyName.name() : __FriendlyName
    })
Namespace.addCategoryObject('typeBinding', 'DCEValueType', DCEValueType)

