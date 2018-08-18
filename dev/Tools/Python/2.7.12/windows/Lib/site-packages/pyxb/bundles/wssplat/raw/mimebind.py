# ./pyxb/bundles/wssplat/raw/mimebind.py
# -*- coding: utf-8 -*-
# PyXB bindings for NM:5404248094c8a480a41150c00669c3a55d100562
# Generated 2014-10-19 06:24:56.418947 by PyXB version 1.2.4 using Python 2.7.3.final.0
# Namespace http://schemas.xmlsoap.org/wsdl/mime/

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
_GenerationUID = pyxb.utils.utility.UniqueIdentifier('urn:uuid:8dc62eb8-5782-11e4-9b8f-c8600024e903')

# Version of PyXB used to generate the bindings
_PyXBVersion = '1.2.4'
# Generated bindings are not compatible across PyXB versions
if pyxb.__version__ != _PyXBVersion:
    raise pyxb.PyXBVersionError(_PyXBVersion)

# Import bindings for namespaces imported into schema
import pyxb.bundles.wssplat.wsdl11
import pyxb.binding.datatypes

# NOTE: All namespace declarations are reserved within the binding
Namespace = pyxb.namespace.NamespaceForURI('http://schemas.xmlsoap.org/wsdl/mime/', create_if_missing=True)
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


# Complex type {http://schemas.xmlsoap.org/wsdl/mime/}contentType with content type EMPTY
class contentType (pyxb.bundles.wssplat.wsdl11.tExtensibilityElement):
    """Complex type {http://schemas.xmlsoap.org/wsdl/mime/}contentType with content type EMPTY"""
    _TypeDefinition = None
    _ContentTypeTag = pyxb.binding.basis.complexTypeDefinition._CT_EMPTY
    _Abstract = False
    _ExpandedName = pyxb.namespace.ExpandedName(Namespace, 'contentType')
    _XSDLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/mimebind.xsd', 39, 4)
    _ElementMap = pyxb.bundles.wssplat.wsdl11.tExtensibilityElement._ElementMap.copy()
    _AttributeMap = pyxb.bundles.wssplat.wsdl11.tExtensibilityElement._AttributeMap.copy()
    # Base type is pyxb.bundles.wssplat.wsdl11.tExtensibilityElement
    
    # Attribute required inherited from {http://schemas.xmlsoap.org/wsdl/}tExtensibilityElement
    
    # Attribute type uses Python identifier type
    __type = pyxb.binding.content.AttributeUse(pyxb.namespace.ExpandedName(None, 'type'), 'type', '__httpschemas_xmlsoap_orgwsdlmime_contentType_type', pyxb.binding.datatypes.string)
    __type._DeclarationLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/mimebind.xsd', 43, 9)
    __type._UseLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/mimebind.xsd', 43, 9)
    
    type = property(__type.value, __type.set, None, None)

    
    # Attribute part uses Python identifier part
    __part = pyxb.binding.content.AttributeUse(pyxb.namespace.ExpandedName(None, 'part'), 'part', '__httpschemas_xmlsoap_orgwsdlmime_contentType_part', pyxb.binding.datatypes.NMTOKEN)
    __part._DeclarationLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/mimebind.xsd', 44, 9)
    __part._UseLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/mimebind.xsd', 44, 9)
    
    part = property(__part.value, __part.set, None, None)

    _ElementMap.update({
        
    })
    _AttributeMap.update({
        __type.name() : __type,
        __part.name() : __part
    })
Namespace.addCategoryObject('typeBinding', 'contentType', contentType)


# Complex type {http://schemas.xmlsoap.org/wsdl/mime/}multipartRelatedType with content type ELEMENT_ONLY
class multipartRelatedType (pyxb.bundles.wssplat.wsdl11.tExtensibilityElement):
    """Complex type {http://schemas.xmlsoap.org/wsdl/mime/}multipartRelatedType with content type ELEMENT_ONLY"""
    _TypeDefinition = None
    _ContentTypeTag = pyxb.binding.basis.complexTypeDefinition._CT_ELEMENT_ONLY
    _Abstract = False
    _ExpandedName = pyxb.namespace.ExpandedName(Namespace, 'multipartRelatedType')
    _XSDLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/mimebind.xsd', 49, 4)
    _ElementMap = pyxb.bundles.wssplat.wsdl11.tExtensibilityElement._ElementMap.copy()
    _AttributeMap = pyxb.bundles.wssplat.wsdl11.tExtensibilityElement._AttributeMap.copy()
    # Base type is pyxb.bundles.wssplat.wsdl11.tExtensibilityElement
    
    # Element part uses Python identifier part
    __part = pyxb.binding.content.ElementDeclaration(pyxb.namespace.ExpandedName(None, 'part'), 'part', '__httpschemas_xmlsoap_orgwsdlmime_multipartRelatedType_part', True, pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/mimebind.xsd', 53, 10), )

    
    part = property(__part.value, __part.set, None, None)

    
    # Attribute required inherited from {http://schemas.xmlsoap.org/wsdl/}tExtensibilityElement
    _ElementMap.update({
        __part.name() : __part
    })
    _AttributeMap.update({
        
    })
Namespace.addCategoryObject('typeBinding', 'multipartRelatedType', multipartRelatedType)


# Complex type {http://schemas.xmlsoap.org/wsdl/mime/}tPart with content type ELEMENT_ONLY
class tPart (pyxb.binding.basis.complexTypeDefinition):
    """Complex type {http://schemas.xmlsoap.org/wsdl/mime/}tPart with content type ELEMENT_ONLY"""
    _TypeDefinition = None
    _ContentTypeTag = pyxb.binding.basis.complexTypeDefinition._CT_ELEMENT_ONLY
    _Abstract = False
    _ExpandedName = pyxb.namespace.ExpandedName(Namespace, 'tPart')
    _XSDLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/mimebind.xsd', 58, 7)
    _ElementMap = {}
    _AttributeMap = {}
    # Base type is pyxb.binding.datatypes.anyType
    
    # Attribute name uses Python identifier name
    __name = pyxb.binding.content.AttributeUse(pyxb.namespace.ExpandedName(None, 'name'), 'name', '__httpschemas_xmlsoap_orgwsdlmime_tPart_name', pyxb.binding.datatypes.NMTOKEN, required=True)
    __name._DeclarationLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/mimebind.xsd', 62, 9)
    __name._UseLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/mimebind.xsd', 62, 9)
    
    name = property(__name.value, __name.set, None, None)

    _HasWildcardElement = True
    _ElementMap.update({
        
    })
    _AttributeMap.update({
        __name.name() : __name
    })
Namespace.addCategoryObject('typeBinding', 'tPart', tPart)


# Complex type {http://schemas.xmlsoap.org/wsdl/mime/}tMimeXml with content type EMPTY
class tMimeXml (pyxb.bundles.wssplat.wsdl11.tExtensibilityElement):
    """Complex type {http://schemas.xmlsoap.org/wsdl/mime/}tMimeXml with content type EMPTY"""
    _TypeDefinition = None
    _ContentTypeTag = pyxb.binding.basis.complexTypeDefinition._CT_EMPTY
    _Abstract = False
    _ExpandedName = pyxb.namespace.ExpandedName(Namespace, 'tMimeXml')
    _XSDLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/mimebind.xsd', 65, 4)
    _ElementMap = pyxb.bundles.wssplat.wsdl11.tExtensibilityElement._ElementMap.copy()
    _AttributeMap = pyxb.bundles.wssplat.wsdl11.tExtensibilityElement._AttributeMap.copy()
    # Base type is pyxb.bundles.wssplat.wsdl11.tExtensibilityElement
    
    # Attribute required inherited from {http://schemas.xmlsoap.org/wsdl/}tExtensibilityElement
    
    # Attribute part uses Python identifier part
    __part = pyxb.binding.content.AttributeUse(pyxb.namespace.ExpandedName(None, 'part'), 'part', '__httpschemas_xmlsoap_orgwsdlmime_tMimeXml_part', pyxb.binding.datatypes.NMTOKEN)
    __part._DeclarationLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/mimebind.xsd', 69, 3)
    __part._UseLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/mimebind.xsd', 69, 3)
    
    part = property(__part.value, __part.set, None, None)

    _ElementMap.update({
        
    })
    _AttributeMap.update({
        __part.name() : __part
    })
Namespace.addCategoryObject('typeBinding', 'tMimeXml', tMimeXml)


content = pyxb.binding.basis.element(pyxb.namespace.ExpandedName(Namespace, 'content'), contentType, location=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/mimebind.xsd', 38, 4))
Namespace.addCategoryObject('elementBinding', content.name().localName(), content)

multipartRelated = pyxb.binding.basis.element(pyxb.namespace.ExpandedName(Namespace, 'multipartRelated'), multipartRelatedType, location=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/mimebind.xsd', 48, 4))
Namespace.addCategoryObject('elementBinding', multipartRelated.name().localName(), multipartRelated)

mimeXml = pyxb.binding.basis.element(pyxb.namespace.ExpandedName(Namespace, 'mimeXml'), tMimeXml, location=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/mimebind.xsd', 64, 4))
Namespace.addCategoryObject('elementBinding', mimeXml.name().localName(), mimeXml)



multipartRelatedType._AddElement(pyxb.binding.basis.element(pyxb.namespace.ExpandedName(None, 'part'), tPart, scope=multipartRelatedType, location=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/mimebind.xsd', 53, 10)))

def _BuildAutomaton ():
    # Remove this helper function from the namespace after it is invoked
    global _BuildAutomaton
    del _BuildAutomaton
    import pyxb.utils.fac as fac

    counters = set()
    cc_0 = fac.CounterCondition(min=0, max=None, metadata=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/mimebind.xsd', 53, 10))
    counters.add(cc_0)
    states = []
    final_update = set()
    final_update.add(fac.UpdateInstruction(cc_0, False))
    symbol = pyxb.binding.content.ElementUse(multipartRelatedType._UseForTag(pyxb.namespace.ExpandedName(None, 'part')), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/mimebind.xsd', 53, 10))
    st_0 = fac.State(symbol, is_initial=True, final_update=final_update, is_unordered_catenation=False)
    states.append(st_0)
    transitions = []
    transitions.append(fac.Transition(st_0, [
        fac.UpdateInstruction(cc_0, True) ]))
    st_0._set_transitionSet(transitions)
    return fac.Automaton(states, counters, True, containing_state=None)
multipartRelatedType._Automaton = _BuildAutomaton()




def _BuildAutomaton_ ():
    # Remove this helper function from the namespace after it is invoked
    global _BuildAutomaton_
    del _BuildAutomaton_
    import pyxb.utils.fac as fac

    counters = set()
    cc_0 = fac.CounterCondition(min=0, max=None, metadata=pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/mimebind.xsd', 60, 9))
    counters.add(cc_0)
    states = []
    final_update = set()
    final_update.add(fac.UpdateInstruction(cc_0, False))
    symbol = pyxb.binding.content.WildcardUse(pyxb.binding.content.Wildcard(process_contents=pyxb.binding.content.Wildcard.PC_strict, namespace_constraint=set(['http://schemas.xmlsoap.org/wsdl/mime/'])), pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/wssplat/schemas/mimebind.xsd', 60, 9))
    st_0 = fac.State(symbol, is_initial=True, final_update=final_update, is_unordered_catenation=False)
    states.append(st_0)
    transitions = []
    transitions.append(fac.Transition(st_0, [
        fac.UpdateInstruction(cc_0, True) ]))
    st_0._set_transitionSet(transitions)
    return fac.Automaton(states, counters, True, containing_state=None)
tPart._Automaton = _BuildAutomaton_()

