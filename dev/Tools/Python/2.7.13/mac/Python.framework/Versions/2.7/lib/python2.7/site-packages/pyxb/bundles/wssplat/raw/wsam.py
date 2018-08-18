# ./pyxb/bundles/wssplat/raw/wsam.py
# -*- coding: utf-8 -*-
# PyXB bindings for NM:8412da32cb8f7a70943a9934e4bb13ceb5b27944
# Generated 2014-10-19 06:24:59.111902 by PyXB version 1.2.4 using Python 2.7.3.final.0
# Namespace http://www.w3.org/2007/02/addressing/metadata

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
_GenerationUID = pyxb.utils.utility.UniqueIdentifier('urn:uuid:8f623cc6-5782-11e4-9028-c8600024e903')

# Version of PyXB used to generate the bindings
_PyXBVersion = '1.2.4'
# Generated bindings are not compatible across PyXB versions
if pyxb.__version__ != _PyXBVersion:
    raise pyxb.PyXBVersionError(_PyXBVersion)

# Import bindings for namespaces imported into schema
import pyxb.binding.datatypes
import pyxb.bundles.wssplat.wsp200607

# NOTE: All namespace declarations are reserved within the binding
Namespace = pyxb.namespace.NamespaceForURI('http://www.w3.org/2007/02/addressing/metadata', create_if_missing=True)
Namespace.configureCategories(['typeBinding', 'elementBinding'])
_Namespace = pyxb.bundles.wssplat.wsp200607.Namespace
_Namespace.configureCategories(['typeBinding', 'elementBinding'])

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


# Complex type {http://www.w3.org/2007/02/addressing/metadata}ServiceNameType with content type SIMPLE
class ServiceNameType (pyxb.binding.basis.complexTypeDefinition):
    """Complex type {http://www.w3.org/2007/02/addressing/metadata}ServiceNameType with content type SIMPLE"""
    _TypeDefinition = pyxb.binding.datatypes.QName
    _ContentTypeTag = pyxb.binding.basis.complexTypeDefinition._CT_SIMPLE
    _Abstract = False
    _ExpandedName = pyxb.namespace.ExpandedName(Namespace, 'ServiceNameType')
    _XSDLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wsam.xsd', 23, 4)
    _ElementMap = {}
    _AttributeMap = {}
    # Base type is pyxb.binding.datatypes.QName
    
    # Attribute EndpointName uses Python identifier EndpointName
    __EndpointName = pyxb.binding.content.AttributeUse(pyxb.namespace.ExpandedName(None, 'EndpointName'), 'EndpointName', '__httpwww_w3_org200702addressingmetadata_ServiceNameType_EndpointName', pyxb.binding.datatypes.NCName)
    __EndpointName._DeclarationLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wsam.xsd', 26, 16)
    __EndpointName._UseLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wsam.xsd', 26, 16)
    
    EndpointName = property(__EndpointName.value, __EndpointName.set, None, None)

    _AttributeWildcard = pyxb.binding.content.Wildcard(process_contents=pyxb.binding.content.Wildcard.PC_lax, namespace_constraint=(pyxb.binding.content.Wildcard.NC_not, 'http://www.w3.org/2007/02/addressing/metadata'))
    _ElementMap.update({
        
    })
    _AttributeMap.update({
        __EndpointName.name() : __EndpointName
    })
Namespace.addCategoryObject('typeBinding', 'ServiceNameType', ServiceNameType)


# Complex type {http://www.w3.org/2007/02/addressing/metadata}AttributedQNameType with content type SIMPLE
class AttributedQNameType (pyxb.binding.basis.complexTypeDefinition):
    """Complex type {http://www.w3.org/2007/02/addressing/metadata}AttributedQNameType with content type SIMPLE"""
    _TypeDefinition = pyxb.binding.datatypes.QName
    _ContentTypeTag = pyxb.binding.basis.complexTypeDefinition._CT_SIMPLE
    _Abstract = False
    _ExpandedName = pyxb.namespace.ExpandedName(Namespace, 'AttributedQNameType')
    _XSDLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wsam.xsd', 33, 4)
    _ElementMap = {}
    _AttributeMap = {}
    # Base type is pyxb.binding.datatypes.QName
    _AttributeWildcard = pyxb.binding.content.Wildcard(process_contents=pyxb.binding.content.Wildcard.PC_lax, namespace_constraint=(pyxb.binding.content.Wildcard.NC_not, 'http://www.w3.org/2007/02/addressing/metadata'))
    _ElementMap.update({
        
    })
    _AttributeMap.update({
        
    })
Namespace.addCategoryObject('typeBinding', 'AttributedQNameType', AttributedQNameType)


# Complex type [anonymous] with content type ELEMENT_ONLY
class CTD_ANON (pyxb.binding.basis.complexTypeDefinition):
    """Complex type [anonymous] with content type ELEMENT_ONLY"""
    _TypeDefinition = None
    _ContentTypeTag = pyxb.binding.basis.complexTypeDefinition._CT_ELEMENT_ONLY
    _Abstract = False
    _ExpandedName = None
    _XSDLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wsam.xsd', 46, 8)
    _ElementMap = {}
    _AttributeMap = {}
    # Base type is pyxb.binding.datatypes.anyType
    
    # Element {http://www.w3.org/2006/07/ws-policy}Policy uses Python identifier Policy
    __Policy = pyxb.binding.content.ElementDeclaration(pyxb.namespace.ExpandedName(_Namespace, 'Policy'), 'Policy', '__httpwww_w3_org200702addressingmetadata_CTD_ANON_httpwww_w3_org200607ws_policyPolicy', False, pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wsp200607.xsd', 30, 2), )

    
    Policy = property(__Policy.value, __Policy.set, None, None)

    _AttributeWildcard = pyxb.binding.content.Wildcard(process_contents=pyxb.binding.content.Wildcard.PC_lax, namespace_constraint=(pyxb.binding.content.Wildcard.NC_not, 'http://www.w3.org/2007/02/addressing/metadata'))
    _ElementMap.update({
        __Policy.name() : __Policy
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
    _XSDLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wsam.xsd', 55, 8)
    _ElementMap = {}
    _AttributeMap = {}
    # Base type is pyxb.binding.datatypes.anyType
    _AttributeWildcard = pyxb.binding.content.Wildcard(process_contents=pyxb.binding.content.Wildcard.PC_lax, namespace_constraint=(pyxb.binding.content.Wildcard.NC_not, 'http://www.w3.org/2007/02/addressing/metadata'))
    _ElementMap.update({
        
    })
    _AttributeMap.update({
        
    })



# Complex type [anonymous] with content type EMPTY
class CTD_ANON_2 (pyxb.binding.basis.complexTypeDefinition):
    """Complex type [anonymous] with content type EMPTY"""
    _TypeDefinition = None
    _ContentTypeTag = pyxb.binding.basis.complexTypeDefinition._CT_EMPTY
    _Abstract = False
    _ExpandedName = None
    _XSDLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wsam.xsd', 61, 8)
    _ElementMap = {}
    _AttributeMap = {}
    # Base type is pyxb.binding.datatypes.anyType
    _AttributeWildcard = pyxb.binding.content.Wildcard(process_contents=pyxb.binding.content.Wildcard.PC_lax, namespace_constraint=(pyxb.binding.content.Wildcard.NC_not, 'http://www.w3.org/2007/02/addressing/metadata'))
    _ElementMap.update({
        
    })
    _AttributeMap.update({
        
    })



ServiceName = pyxb.binding.basis.element(pyxb.namespace.ExpandedName(Namespace, 'ServiceName'), ServiceNameType, location=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wsam.xsd', 22, 4))
Namespace.addCategoryObject('elementBinding', ServiceName.name().localName(), ServiceName)

InterfaceName = pyxb.binding.basis.element(pyxb.namespace.ExpandedName(Namespace, 'InterfaceName'), AttributedQNameType, location=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wsam.xsd', 32, 4))
Namespace.addCategoryObject('elementBinding', InterfaceName.name().localName(), InterfaceName)

Addressing = pyxb.binding.basis.element(pyxb.namespace.ExpandedName(Namespace, 'Addressing'), CTD_ANON, location=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wsam.xsd', 45, 4))
Namespace.addCategoryObject('elementBinding', Addressing.name().localName(), Addressing)

AnonymousResponses = pyxb.binding.basis.element(pyxb.namespace.ExpandedName(Namespace, 'AnonymousResponses'), CTD_ANON_, location=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wsam.xsd', 54, 4))
Namespace.addCategoryObject('elementBinding', AnonymousResponses.name().localName(), AnonymousResponses)

NonAnonymousResponses = pyxb.binding.basis.element(pyxb.namespace.ExpandedName(Namespace, 'NonAnonymousResponses'), CTD_ANON_2, location=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wsam.xsd', 60, 4))
Namespace.addCategoryObject('elementBinding', NonAnonymousResponses.name().localName(), NonAnonymousResponses)



CTD_ANON._AddElement(pyxb.binding.basis.element(pyxb.namespace.ExpandedName(_Namespace, 'Policy'), pyxb.bundles.wssplat.wsp200607.CTD_ANON_3, scope=CTD_ANON, location=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wsp200607.xsd', 30, 2)))

def _BuildAutomaton ():
    # Remove this helper function from the namespace after it is invoked
    global _BuildAutomaton
    del _BuildAutomaton
    import pyxb.utils.fac as fac

    counters = set()
    states = []
    final_update = set()
    symbol = pyxb.binding.content.ElementUse(CTD_ANON._UseForTag(pyxb.namespace.ExpandedName(_Namespace, 'Policy')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/wsam.xsd', 48, 16))
    st_0 = fac.State(symbol, is_initial=True, final_update=final_update, is_unordered_catenation=False)
    states.append(st_0)
    transitions = []
    st_0._set_transitionSet(transitions)
    return fac.Automaton(states, counters, False, containing_state=None)
CTD_ANON._Automaton = _BuildAutomaton()

