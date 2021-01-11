import lib.fbxSdk
import fbx
import FbxCommon

class FbxSceneUtil(object):

	def __init__(self, filepath):
		super(FbxSceneUtil, self).__init__()
		self.__filepath = filepath
		self.__sdkManager, self.__scene = FbxCommon.InitializeSdkObjects()
		FbxCommon.LoadScene(self.__sdkManager, self.__scene, filepath)
		self.__systemUnits = self.__scene.GetGlobalSettings().GetSystemUnit()

	def close(self):
		self.sdkManager.Destroy()

	def save(self):
		FbxCommon.SaveScene(self.sdkManager, self.scene, self.filepath)

	def saveAs(self, filepath):
		"""
		:param filepath: str
		:return: None
		"""
		self.filepath = filepath
		self.save()

	def __getChildNodes(self, node):
		"""
		:param node: object
		:return: list of fbx.FbxNode
		"""
		nodes = []
		for i in range(node.GetChildCount()):
			childNode = node.GetChild(i)
			nodes.append(childNode)
			nodes.extend(self.__getChildNodes(node=childNode))
		return nodes

	def getSceneNodes(self):
		"""
		:return: list of fbx.FbxNode
		"""
		rootNode = self.scene.GetRootNode()
		nodes = []
		for i in range(rootNode.GetChildCount()):
			childNode = rootNode.GetChild(i)
			nodes.append(childNode)
			nodes.extend(self.__getChildNodes(node=childNode))
		return nodes

	def removeNamespaces(self):
		for node in self.getSceneNodes():
			nameParts = node.GetName().split(':')
			if len(nameParts) > 1:
				node.SetName(nameParts[-1])

	def setNamespace(self, namespace):
		"""
		:param namespace: str
		:return: None
		"""
		for node in self.getSceneNodes():
			nameParts = node.GetName().split(':')
			if len(nameParts) > 1:
				node.SetName(nameParts[-1])
			node.SetName('%s:%s' % (namespace, node.GetName()))

	def setScaleFactor(self, scaleFactor=100.0):
		"""
		:param sceleFactor: float
		"""
		if self.systemUnits.GetScaleFactor() == float(scaleFactor):
			return
		fbx.FbxSystemUnit(float(scaleFactor), float(scaleFactor)).ConvertScene(self.scene)

	@property
	def filepath(self):
		"""
		:return: str
		"""
		return self.__filepath

	@filepath.setter
	def filepath(self, value):
		"""
		:param value: str
		:return: None
		"""
		self.__filepath = value

	@property
	def sdkManager(self):
		"""
		:return: fbx.FbxManager
		"""
		return self.__sdkManager

	@property
	def scene(self):
		"""
		:return: fbx.FbxScene
		"""
		return self.__scene

	@property
	def systemUnits(self):
		"""
		:return: fbx.FbxSystemUnit
		"""
		return self.__systemUnits

def setScaleFactor(filepath, scaleFactor=100.0):
	"""
	:param fbxFilepath:  str
	:param scaleFactor:  float
	"""

	util = FbxSceneUtil(filepath=filepath)
	util.setScaleFactor(scaleFactor)
	util.save()
	util.close()


def removeNamespaces(filepath):
	util = FbxSceneUtil(filepath=filepath)
	util.removeNamespaces()
	util.save()
	util.close()


def hasNonZeroTransforms(filepath):
	util = FbxSceneUtil(filepath=filepath)
	for sceneNode in util.getSceneNodes():
		translation = fbx.FbxPropertyDouble3(util.getSceneNodes()[0].LclTranslation).Get()
		rotation = fbx.FbxPropertyDouble3(util.getSceneNodes()[0].LclRotation).Get()
		scale = fbx.FbxPropertyDouble3(util.getSceneNodes()[0].LclScaling).Get()
		if translation[0] == 0.0 and translation[1] == 0.0 and translation[2] == 0.0 and rotation[0] == 0.0 and rotation[1] == 0.0 and rotation[2] == 0.0 and scale[0] == 1.0 and scale[1] == 1.0 and scale[2] == 1.0:
			util.close()
			return False

		util.close()
		return True

