import random
import sys

from collections import Mapping
import inspect

class Slice(Mapping):
	STREAM_VERSION = '3'
	ENTITY_VERSION = '2'
	ENTITY_ID_VERSION = '1'
	SLICE_COMPONENT_VERSION = '2'
	DEFAULT_NUMBER_ID = '4294967295'
	def __init__(self):
		self.__meshAssetIdString = '' # Sqlite3 lookup
		self.__collisionAssetIdString = '' # Sqlite3 lookup
		self.__name = 'SliceFile'
		self.__id = str(random.randint(0, sys.maxsize))
		self.__componentId = str(random.randint(0, sys.maxsize))
		self.__entityId = str(random.randint(0, sys.maxsize))
		self.__transformComponentId = str(random.randint(0, sys.maxsize))
		self.__editorMeshComponentId = str(random.randint(0, sys.maxsize))
		self.__meshColliderGenericComponentWrapperId = str(random.randint(0, sys.maxsize))
		self.__physicsDataGenericComponentWrapperId = str(random.randint(0, sys.maxsize))
		self.__editorStaticPhysicsComponentId = str(random.randint(0, sys.maxsize))
		self.__metaDataEntityId = str(random.randint(0, sys.maxsize))
		self.__metaDataInfoComponentId = str(random.randint(0, sys.maxsize))
		self.__translate = '0.0000000 0.0000000 0.0000000'
		self.__rotate = '0.0000000 0.0000000 0.0000000'
		# self.__cgfRelativePath = 'objects/default/primitive_cube.cgf'
		self.__editorLockComponentId = str(random.randint(0, sys.maxsize))
		self.__editorPendingCompositionComponentId = str(random.randint(0, sys.maxsize))
		self.__editorVisibilityComponentId = str(random.randint(0, sys.maxsize))
		self.__selectionComponentId = str(random.randint(0, sys.maxsize))
		self.__editorInspectorComponentId = str(random.randint(0, sys.maxsize))
		self.__editorEntitySortComponentId = str(random.randint(0, sys.maxsize))
		self.__editorEntityIconComponentId = str(random.randint(0, sys.maxsize))
		self.__editorDisabledCompositionComponentId = str(random.randint(0, sys.maxsize))


        def __getdict(self):
            classProperties = {k: v for k, v in inspect.getmembers(type(self), lambda v: isinstance(v, property))}
            return {k: v for k, v in inspect.getmembers(self) if k in classProperties}


        def __getitem__(self, key):
            return self.__getdict().__getitem__(key)


        def __iter__(self):
            return iter(self.__getdict())


        def __len__(self):
            return len(self.__getdict())


        @property
        def streamVersion(self):
            return self.STREAM_VERSION


        @property
        def entityVersion(self):
            return self.ENTITY_VERSION


        @property
        def entityIdVersion(self):
            return self.ENTITY_ID_VERSION


        @property
        def sliceComponentVersion(self):
	    return self.SLICE_COMPONENT_VERSION


        @property
        def defaultNumberId(self):
	    return self.DEFAULT_NUMBER_ID


	@property
	def id(self):
		"""
		:return:  str
		"""
		return self.__id


	@id.setter
	def id(self, value):
		"""
		:return:  str
		"""
		if self.__id == value:
			return


		self.__id = value


	@property
	def componentId(self):
		"""
		:return:  str
		"""
		return self.__componentId


	@componentId.setter
	def componentId(self, value):
		"""
		:return:  str
		"""
		if self.__componentId == value:
			return


		self.__componentId = value

	@property
	def name(self):
		"""
		:return:  str
		"""
		return self.__name


	@name.setter
	def name(self, value):
		"""
		:return:  str
		"""
		if self.__name == value:
			return


		self.__name = value	\

	@property
	def entityId(self):
		"""
		:return:  str
		"""
		return self.__entityId


	@entityId.setter
	def entityId(self, value):
		"""
		:return:  str
		"""
		if self.__entityId == value:
			return


		self.__entityId = value

	@property
	def transformComponentId(self):
		"""
		:return:  str
		"""
		return self.__transformComponentId


	@transformComponentId.setter
	def transformComponentId(self, value):
		"""
		:return:  str
		"""
		if self.__transformComponentId == value:
			return


		self.__transformComponentId = value

	@property
	def editorMeshComponentId(self):
		"""
		:return:  str
		"""
		return self.__editorMeshComponentId


	@editorMeshComponentId.setter
	def editorMeshComponentId(self, value):
		"""
		:return:  str
		"""
		if self.__editorMeshComponentId == value:
			return


		self.__editorMeshComponentId = value

	@property
	def meshColliderGenericComponentWrapperId(self):
		"""
		:return:  str
		"""
		return self.__meshColliderGenericComponentWrapperId


	@meshColliderGenericComponentWrapperId.setter
	def meshColliderGenericComponentWrapperId(self, value):
		"""
		:return:  str
		"""
		if self.__meshColliderGenericComponentWrapperId == value:
			return

		self.__meshColliderGenericComponentWrapperId = value


	@property
	def physicsDataGenericComponentWrapperId(self):
		"""
		:return:  str
		"""
		return self.__physicsDataGenericComponentWrapperId


	@physicsDataGenericComponentWrapperId.setter
	def physicsDataGenericComponentWrapperId(self, value):
		"""
		:return:  str
		"""
		if self.__physicsDataGenericComponentWrapperId == value:
			return

		self.__physicsDataGenericComponentWrapperId = value

	@property
	def editorStaticPhysicsComponentId(self):
		"""
		:return:  str
		"""
		return self.__editorStaticPhysicsComponentId


	@editorStaticPhysicsComponentId.setter
	def editorStaticPhysicsComponentId(self, value):
		"""
		:return:  str
		"""
		if self.__editorStaticPhysicsComponentId == value:
			return


		self.__editorStaticPhysicsComponentId = value

	@property
	def metaDataEntityId(self):
		"""
		:return:  str
		"""
		return self.__metaDataEntityId


	@metaDataEntityId.setter
	def metaDataEntityId(self, value):
		"""
		:return:  str
		"""
		if self.__metaDataEntityId == value:
			return


		self.__metaDataEntityId = value


	@property
	def metaDataInfoComponentId(self):
		"""
		:return:  str
		"""
		return self.__metaDataInfoComponentId


	@metaDataInfoComponentId.setter
	def metaDataInfoComponentId(self, value):
		"""
		:return:  str
		"""
		if self.__metaDataInfoComponentId == value:
			return


		self.__metaDataInfoComponentId = value

	@property
	def translate(self):
		"""
		:return:  str
		"""
		return self.__translate


	@translate.setter
	def translate(self, value):
		"""
		:return:  str
		"""
		if self.__translate == value:
			return


		self.__translate = value

	@property
	def rotate(self):
		"""
		:return:  str
		"""
		return self.__rotate


	@rotate.setter
	def rotate(self, value):
		"""
		:return:  str
		"""
		if self.__rotate == value:
			return


		self.__rotate = value

	@property
	def meshAssetIdString(self):
		"""
		:return:  str
		"""
		return self.__meshAssetIdString


	@meshAssetIdString.setter
	def meshAssetIdString(self, value):
		"""
		:return:  str
		"""
		if self.__meshAssetIdString == value:
			return


		self.__meshAssetIdString = value

	@property
	def collisionAssetIdString(self):
		"""
		:return:  str
		"""
		return self.__collisionAssetIdString


	@collisionAssetIdString.setter
	def collisionAssetIdString(self, value):
		"""
		:return:  str
		"""
		if self.__collisionAssetIdString == value:
			return


		self.__collisionAssetIdString = value

	@property
	def selectionComponentId(self):
		"""
		:return:  str
		"""
		return self.__selectionComponentId


	@selectionComponentId.setter
	def selectionComponentId(self, value):
		"""
		:return:  str
		"""
		if self.__selectionComponentId == value:
			return


		self.__selectionComponentId = value


	@property
	def editorVisibilityComponentId(self):
		"""
		:return:  str
		"""
		return self.__editorVisibilityComponentId


	@editorVisibilityComponentId.setter
	def editorVisibilityComponentId(self, value):
		"""
		:return:  str
		"""
		if self.__editorVisibilityComponentId == value:
			return


		self.__editorVisibilityComponentId = value


	@property
	def editorPendingCompositionComponentId(self):
		"""
		:return:  str
		"""
		return self.__editorPendingCompositionComponentId


	@editorPendingCompositionComponentId.setter
	def editorPendingCompositionComponentId(self, value):
		"""
		:return:  str
		"""
		if self.__editorPendingCompositionComponentId == value:
			return


		self.__editorPendingCompositionComponentId = value


	@property
	def editorLockComponentId(self):
		"""
		:return:  str
		"""
		return self.__editorLockComponentId


	@editorLockComponentId.setter
	def editorLockComponentId(self, value):
		"""
		:return:  str
		"""
		if self.__editorLockComponentId == value:
			return


		self.__editorLockComponentId = value


	@property
	def editorInspectorComponentId(self):
		"""
		:return:  str
		"""
		return self.__editorInspectorComponentId


	@editorInspectorComponentId.setter
	def editorInspectorComponentId(self, value):
		"""
		:return:  str
		"""
		if self.__editorInspectorComponentId == value:
			return


		self.__editorInspectorComponentId = value


	@property
	def editorEntitySortComponentId(self):
		"""
		:return:  str
		"""
		return self.__editorEntitySortComponentId


	@editorEntitySortComponentId.setter
	def editorEntitySortComponentId(self, value):
		"""
		:return:  str
		"""
		if self.__editorEntitySortComponentId == value:
			return


		self.__editorEntitySortComponentId = value


	@property
	def editorEntityIconComponentId(self):
		"""
		:return:  str
		"""
		return self.__editorEntityIconComponentId


	@editorEntityIconComponentId.setter
	def editorEntityIconComponentId(self, value):
		"""
		:return:  str
		"""
		if self.__editorEntityIconComponentId == value:
			return


		self.__editorEntityIconComponentId = value


	@property
	def editorDisabledCompositionComponentId(self):
		"""
		:return:  str
		"""
		return self.__editorDisabledCompositionComponentId


	@editorDisabledCompositionComponentId.setter
	def editorDisabledCompositionComponentId(self, value):
		"""
		:return:  str
		"""
		if self.__editorDisabledCompositionComponentId == value:
			return


		self.__editorDisabledCompositionComponentId = value

