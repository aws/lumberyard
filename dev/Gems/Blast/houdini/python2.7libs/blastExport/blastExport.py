import hou

from assetDB import AssetDBConnection
from slice import Slice
from fbxUtil import setScaleFactor

# import lib.perforce

from contextlib import closing
from jinja2 import Environment, FileSystemLoader, PackageLoader
from itertools import izip
import os
import random
import sys


# def getP4Connection():
#     p4 = getP4Connection.p4
#     if not p4.connected():
#         try:
#             p4.connect()
#         except Exception as error:
#             if 'check $P4PORT' in str(error):
#                 raise Exception('Not connected to network')
#     return p4

# getP4Connection.p4 = lib.perforce.P4.P4()

# def isInP4(filepath):
#     p4 = getP4Connection()
#     try:
#         info = p4.run('files', filepath.replace('\\', '/'))
#         return info is not None
#     except Exception as error:
#         known_errors = [
#                 'no such file',
#                 'An empty string is not allowed as a file name',
#                 "is not under client's root"
#             ]
#         error_str = str(error)
#         if any(error in error_str for error in known_errors):
#             return False
#         else:
#             raise Exception(error)

# def checkOutFile(filepath):
#     p4 = getP4Connection()

#     inP4 = isInP4(filepath)
#     if not inP4 and not os.path.exists(filepath):
#         raise Exception('Critical Error: File - %s - does not exist on the local file system.' % filepath)

#     if not inP4:
#         p4.run('add', filepath)
#     else:
#         p4.run('edit', filepath)

class DestructibleSlice(Slice):

    def __init__(self, blastAssetIdString, meshAssetIdStrings):
        super(DestructibleSlice, self).__init__()
        self.__blastFamilyGenericComponentWrapperId = str(random.randint(0, sys.maxsize))
        self.__netSystemGenericComponentWrapperId = str(random.randint(0, sys.maxsize))
        self.__locationComponentId = str(random.randint(0, sys.maxsize))
        self.__blastFamilyComponentId = str(random.randint(0, sys.maxsize))
        self.__blastMeshDataComponentId = str(random.randint(0, sys.maxsize))
        self.__octreeNodeComponentId = str(random.randint(0, sys.maxsize))
        self.__blastAssetIdString = blastAssetIdString
        self.__meshAssetIdStrings = meshAssetIdStrings

    @property
    def blastFamilyGenericComponentWrapperId(self):
        return self.__blastFamilyGenericComponentWrapperId

    @property
    def netSystemGenericComponentWrapperId(self):
        return self.__netSystemGenericComponentWrapperId

    @property
    def locationComponentId(self):
        return self.__locationComponentId

    @property
    def blastFamilyComponentId(self):
        return self.__blastFamilyComponentId

    @property
    def blastMeshDataComponentId(self):
        return self.__blastMeshDataComponentId

    @property
    def octreeNodeComponentId(self):
        return self.__octreeNodeComponentId

    @property
    def blastAssetIdString(self):
        return self.__blastAssetIdString

    @property
    def meshAssetIdStrings(self):
        return self.__meshAssetIdStrings


def getChunkNames():
    prefix = hou.pwd().parm('groupPrefix').eval()
    renameNode = hou.node('{0}/HierarchyGroupToBlastChunkOrder'.format(hou.pwd().path()))
    geo = renameNode.geometry()
    primGroupNames = [group.name() for group in geo.primGroups()]
    groupNames = [groupName for groupName in primGroupNames if groupName.startswith(prefix)]

    if not groupNames:
        raise Exception("No chunks found that start with {0}".format(prefix))

    return groupNames

def getFBXFileNames():
    chunkNames = getChunkNames()
    objectName = hou.pwd().parm('objectName').eval()

    return ["{0}_{1}".format(objectName, chunkName) for chunkName in chunkNames]

def getFBXFilePaths():
    fileNames = getFBXFileNames()

    outputDirectory = hou.pwd().parm('fbxOutputDirectory').eval()
    if outputDirectory.endswith('/'):
        outputDirectory = outputDirectory[:-1]

    return ["{0}/{1}.fbx".format(outputDirectory, fileName) for fileName in fileNames]


# def checkOutAll():
#     checkOutBlast()
#     checkOutFBX()
#     checkOutSlice()

# def checkOutBlast():
#     file = hou.pwd().parm('blastOutputFile').eval()

#     if file:
#         checkOutFile(file)

# def checkOutFBX():
#     files = getFBXFilePaths()

#     for file in files:
#         checkOutFile(file)

# def checkOutSlice():
#     file = hou.pwd().parm('sliceOutputFile').eval()

#     if file:
#         checkOutFile(file)

def exportFBX():
    chunkNames = getChunkNames()
    fileNames = getFBXFileNames()

    singleChunkName = hou.pwd().parm('singleChunkName')
    singleChunkFile = hou.pwd().parm('singleChunkFile')
    export = hou.pwd().parm('fbxExportSingleChunk')

    restoreSingleChunkName = singleChunkName.unexpandedString()
    restoreSingleChunkFile = singleChunkFile.unexpandedString()

    for chunkName, fileName in izip(chunkNames, fileNames):
        singleChunkName.set(chunkName)
        singleChunkFile.set(fileName)
        export.pressButton()
        setScaleFactor(fileName)

    singleChunkFile.set(restoreSingleChunkFile, follow_parm_reference=False)
    singleChunkName.set(restoreSingleChunkName, follow_parm_reference=False)

def exportSlice():
    # Path to the 'Assets' folder or similar
    lumberyardAssetPath = hou.pwd().parm('lumberyardAssetPath').evalAsString().replace('\\', '/').lower()
    if lumberyardAssetPath and lumberyardAssetPath[-1] != '/':
        lumberyardAssetPath += '/'
    # Path to the relative root of cached data e.g. 'pc/Assets/'
    lumberyardCacheRelativeRoot = hou.pwd().parm('lumberyardCacheRelativeRoot').evalAsString().replace('\\', '/').lower()
    if lumberyardCacheRelativeRoot and lumberyardCacheRelativeRoot[-1] != '/':
        lumberyardCacheRelativeRoot += '/'
    # Path to assetdb.sqlite for the project
    lumberyardDatabasePath = hou.pwd().parm('lumberyardDatabasePath').evalAsString().replace('\\', '/').lower()

    slicePath = hou.pwd().parm('sliceOutputFile').eval()
    blastAssetPath = hou.pwd().parm('blastOutputFile').eval()

    relativeBlastAssetPath = blastAssetPath.lower().replace(lumberyardAssetPath, '')
    relativeCachedBlastAssetPath = '{0}{1}'.format(lumberyardCacheRelativeRoot, relativeBlastAssetPath)

    fbxFilePaths = getFBXFilePaths()
    relativeFbxPaths = [fbxPath.lower().replace(lumberyardAssetPath, '') for fbxPath in fbxFilePaths]
    relativeCachedCgfPaths = [
            '{0}{1}'.format(lumberyardCacheRelativeRoot, fbxPath.replace('.fbx', '.cgf'))
            for fbxPath in relativeFbxPaths]

    with AssetDBConnection(lumberyardDatabasePath) as assetDB:
        blastAssetIdString = assetDB.getAssetId(relativeCachedBlastAssetPath, relativeBlastAssetPath)

        meshAssetIdStrings = [
            assetDB.getAssetId(cgfPath, fbxPath)
            for fbxPath, cgfPath in izip(relativeFbxPaths, relativeCachedCgfPaths)]

    jinjaEnv = Environment(loader=PackageLoader('blastExport', 'templates'), autoescape=True)
    template = jinjaEnv.get_template('destructible_slice.xml')

    destructibleSliceData = DestructibleSlice(blastAssetIdString, meshAssetIdStrings)
    destructibleSliceData.name = hou.pwd().parm('objectName').eval()

    with open(slicePath, 'w') as sliceFile:
        sliceFile.write(template.render(destructibleSliceData))


def onCreated(node):
    node.setColor(hou.Color((1.0, 0.6, 0.0)))
