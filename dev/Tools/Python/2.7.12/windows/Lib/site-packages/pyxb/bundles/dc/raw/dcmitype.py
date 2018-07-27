# ./pyxb/bundles/dc/raw/dcmitype.py
# -*- coding: utf-8 -*-
# PyXB bindings for NM:049d2b1ce34c6b814456fe5b9055f12be5a680b0
# Generated 2014-10-19 06:25:02.924930 by PyXB version 1.2.4 using Python 2.7.3.final.0
# Namespace http://purl.org/dc/dcmitype/ [xmlns:dcmitype]

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
_GenerationUID = pyxb.utils.utility.UniqueIdentifier('urn:uuid:91a2ca00-5782-11e4-8790-c8600024e903')

# Version of PyXB used to generate the bindings
_PyXBVersion = '1.2.4'
# Generated bindings are not compatible across PyXB versions
if pyxb.__version__ != _PyXBVersion:
    raise pyxb.PyXBVersionError(_PyXBVersion)

# Import bindings for namespaces imported into schema
import pyxb.binding.datatypes

# NOTE: All namespace declarations are reserved within the binding
Namespace = pyxb.namespace.NamespaceForURI('http://purl.org/dc/dcmitype/', create_if_missing=True)
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


# Atomic simple type: [anonymous]
class STD_ANON (pyxb.binding.datatypes.Name, pyxb.binding.basis.enumeration_mixin):

    """An atomic simple type."""

    _ExpandedName = None
    _XSDLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/dc/schemas/dcmitype.xsd', 33, 8)
    _Documentation = None
STD_ANON._CF_enumeration = pyxb.binding.facets.CF_enumeration(value_datatype=STD_ANON, enum_prefix=None)
STD_ANON.Collection = STD_ANON._CF_enumeration.addEnumeration(unicode_value='Collection', tag='Collection')
STD_ANON.Dataset = STD_ANON._CF_enumeration.addEnumeration(unicode_value='Dataset', tag='Dataset')
STD_ANON.Event = STD_ANON._CF_enumeration.addEnumeration(unicode_value='Event', tag='Event')
STD_ANON.Image = STD_ANON._CF_enumeration.addEnumeration(unicode_value='Image', tag='Image')
STD_ANON.MovingImage = STD_ANON._CF_enumeration.addEnumeration(unicode_value='MovingImage', tag='MovingImage')
STD_ANON.StillImage = STD_ANON._CF_enumeration.addEnumeration(unicode_value='StillImage', tag='StillImage')
STD_ANON.InteractiveResource = STD_ANON._CF_enumeration.addEnumeration(unicode_value='InteractiveResource', tag='InteractiveResource')
STD_ANON.Service = STD_ANON._CF_enumeration.addEnumeration(unicode_value='Service', tag='Service')
STD_ANON.Software = STD_ANON._CF_enumeration.addEnumeration(unicode_value='Software', tag='Software')
STD_ANON.Sound = STD_ANON._CF_enumeration.addEnumeration(unicode_value='Sound', tag='Sound')
STD_ANON.Text = STD_ANON._CF_enumeration.addEnumeration(unicode_value='Text', tag='Text')
STD_ANON.PhysicalObject = STD_ANON._CF_enumeration.addEnumeration(unicode_value='PhysicalObject', tag='PhysicalObject')
STD_ANON._InitializeFacetMap(STD_ANON._CF_enumeration)

# Union simple type: {http://purl.org/dc/dcmitype/}DCMIType
# superclasses pyxb.binding.datatypes.anySimpleType
class DCMIType (pyxb.binding.basis.STD_union):

    """Simple type that is a union of STD_ANON."""

    _ExpandedName = pyxb.namespace.ExpandedName(Namespace, 'DCMIType')
    _XSDLocation = pyxb.utils.utility.Location('/tmp/pyxbdist.mqXn05k/PyXB-1.2.4/pyxb/bundles/dc/schemas/dcmitype.xsd', 31, 2)
    _Documentation = None

    _MemberTypes = ( STD_ANON, )
DCMIType._CF_pattern = pyxb.binding.facets.CF_pattern()
DCMIType._CF_enumeration = pyxb.binding.facets.CF_enumeration(value_datatype=DCMIType)
DCMIType.Collection = 'Collection'                # originally STD_ANON.Collection
DCMIType.Dataset = 'Dataset'                      # originally STD_ANON.Dataset
DCMIType.Event = 'Event'                          # originally STD_ANON.Event
DCMIType.Image = 'Image'                          # originally STD_ANON.Image
DCMIType.MovingImage = 'MovingImage'              # originally STD_ANON.MovingImage
DCMIType.StillImage = 'StillImage'                # originally STD_ANON.StillImage
DCMIType.InteractiveResource = 'InteractiveResource'# originally STD_ANON.InteractiveResource
DCMIType.Service = 'Service'                      # originally STD_ANON.Service
DCMIType.Software = 'Software'                    # originally STD_ANON.Software
DCMIType.Sound = 'Sound'                          # originally STD_ANON.Sound
DCMIType.Text = 'Text'                            # originally STD_ANON.Text
DCMIType.PhysicalObject = 'PhysicalObject'        # originally STD_ANON.PhysicalObject
DCMIType._InitializeFacetMap(DCMIType._CF_pattern,
   DCMIType._CF_enumeration)
Namespace.addCategoryObject('typeBinding', 'DCMIType', DCMIType)
