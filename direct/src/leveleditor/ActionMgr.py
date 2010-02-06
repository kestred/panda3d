from pandac.PandaModules import *
import ObjectGlobals as OG

class ActionMgr:
    def __init__(self):
        self.undoList = []
        self.redoList = []

    def reset(self):
        while len(self.undoList) > 0:
            action = self.undoList.pop()
            action.destroy()

        while len(self.redoList) > 0:
            action = self.redoList.pop()
            action.destroy()

    def push(self, action):
        self.undoList.append(action)
        print 'current undoList', self.undoList

    def undo(self):
        if len(self.undoList) < 1:
            print 'No more undo'
        else:
            action = self.undoList.pop()
            self.redoList.append(action)
            action.undo()
            print 'current redoList', self.redoList

    def redo(self):
        if len(self.redoList) < 1:
            print 'No more redo'
        else:
            action = self.redoList.pop()
            self.undoList.append(action)
            action.redo()
            print 'current undoList', self.undoList

class ActionBase(Functor):
    """ Base class for user actions """

    def __init__(self, function, *args, **kargs):
        Functor.__init__(self, function, *args, **kargs)
        self.result = None

    def _do__call__(self, *args, **kargs):
        self.saveStatus()
        self.result = Functor._do__call__(self, *args, **kargs)
        return self.result

    def redo(self):
        self.result = self._do__call__()
        return self.result

    def saveStatus(self):
        # save object status for undo here
        pass
        
    def undo(self):
        print "undo method is not defined for this action"

class ActionAddNewObj(ActionBase):
    """ Action class for adding new object """
    
    def __init__(self, editor, *args, **kargs):
        self.editor = editor
        function = self.editor.objectMgr.addNewObject
        ActionBase.__init__(self, function, *args, **kargs)
        self.uid = None

    def redo(self):
        if self.uid is None:
            print "Can't redo this add"        
        else:
            self.result = self._do__call__(uid=self.uid)
            return self.result
            
    def undo(self):
        if self.result is None:
            print "Can't undo this add"
        else:
            print "Undo: addNewObject"
            obj = self.editor.objectMgr.findObjectByNodePath(self.result)
            self.uid = obj[OG.OBJ_UID]
            self.editor.ui.sceneGraphUI.delete(self.uid)
            base.direct.deselect(self.result)
            base.direct.removeNodePath(self.result)
            self.result = None

class ActionDeleteObj(ActionBase):
    """ Action class for deleting object """

    def __init__(self, editor, *args, **kargs):
        self.editor = editor
        function = base.direct.removeAllSelected
        ActionBase.__init__(self, function, *args, **kargs)
        self.selectedUIDs = []
        self.hierarchy = {}
        self.objInfos = {}
        self.objTransforms = {}

    def saveStatus(self):
        selectedNPs = base.direct.selected.getSelectedAsList()
        def saveObjStatus(np, isRecursive=True):
            obj = self.editor.objectMgr.findObjectByNodePath(np)
            if obj:
                uid = obj[OG.OBJ_UID]
                if not isRecursive:
                    self.selectedUIDs.append(uid)
                objNP = obj[OG.OBJ_NP]
                self.objInfos[uid] = obj
                self.objTransforms[uid] = objNP.getMat()
                parentNP = objNP.getParent()
                if parentNP == render:
                    self.hierarchy[uid] = None
                else:
                    parentObj = self.editor.objectMgr.findObjectByNodePath(parentNP)
                    if parentObj:
                        self.hierarchy[uid] = parentObj[OG.OBJ_UID]

                for child in np.getChildren():
                    if child.hasTag('OBJRoot'):
                        saveObjStatus(child)

        for np in selectedNPs:
            saveObjStatus(np, False)

    def undo(self):
        if len(self.hierarchy.keys()) == 0 or\
           len(self.objInfos.keys()) == 0:
            print "Can't undo this deletion"
        else:
            print "Undo: deleteObject"
            def restoreObject(uid, parentNP):
                obj = self.objInfos[uid]
                objNP = obj[OG.OBJ_NP]
                objDef = obj[OG.OBJ_DEF]
                objModel = obj[OG.OBJ_MODEL]
                objProp = obj[OG.OBJ_PROP]
                objRGBA = obj[OG.OBJ_RGBA]
                self.editor.objectMgr.addNewObject(objDef.name,
                                                   uid,
                                                   obj[OG.OBJ_MODEL],
                                                   parentNP)
                self.editor.objectMgr.updateObjectColor(objRGBA[0], objRGBA[1], objRGBA[2], objRGBA[3], uid)
                self.editor.objectMgr.updateObjectProperties(uid, objProp)
                objNP.setMat(self.objTransforms[uid])

            while (len(self.hierarchy.keys()) > 0):
                for uid in self.hierarchy.keys():
                    if self.hierarchy[uid] is None:
                        parentNP = None
                        restoreObject(uid, parentNP)
                        del self.hierarchy[uid]
                    else:
                        parentObj = self.editor.objectMgr.findObjectById(self.hierarchy[uid])
                        if parentObj:
                            parentNP = parentObj[OG.OBJ_NP]
                            restoreObject(uid, parentNP)
                            del self.hierarchy[uid]

            base.direct.deselectAllCB()
            for uid in self.selectedUIDs:
                obj = self.editor.objectMgr.findObjectById(uid)
                if obj:
                    self.editor.select(obj[OG.OBJ_NP], fMultiSelect=1, fUndo=0)

            self.selecteUIDs = []
            self.hierarchy = {}
            self.objInfos = {}            

class ActionSelectObj(ActionBase):
    """ Action class for adding new object """
    
    def __init__(self, editor, *args, **kargs):
        self.editor = editor
        function = base.direct.selectCB
        ActionBase.__init__(self, function, *args, **kargs)
        self.selectedUIDs = []

    def saveStatus(self):
        selectedNPs = base.direct.selected.getSelectedAsList()
        for np in selectedNPs:
            obj = self.editor.objectMgr.findObjectByNodePath(np)
            if obj:
                uid = obj[OG.OBJ_UID]
                self.selectedUIDs.append(uid)

    def undo(self):
        print "Undo : selectObject"
        base.direct.deselectAllCB()
        for uid in self.selectedUIDs:
            obj = self.editor.objectMgr.findObjectById(uid)
            if obj:
                self.editor.select(obj[OG.OBJ_NP], fMultiSelect=1, fUndo=0)
        self.selectedUIDs = []

class ActionTransformObj(ActionBase):
    """ Action class for object transformation """

    def __init__(self, editor, *args, **kargs):
        self.editor = editor
        function = self.editor.objectMgr.setObjectTransform
        ActionBase.__init__(self, function, *args, **kargs)
        self.uid = args[0]
        #self.xformMat = Mat4(args[1])
        self.origMat = None

    def saveStatus(self):
        obj = self.editor.objectMgr.findObjectById(self.uid)
        if obj:
            self.origMat = Mat4(obj[OG.OBJ_NP].getMat())

    def redo(self):
        if self.uid is None:
            print "Can't redo this add"        
        else:
            self.result = self._do__call__()#uid=self.uid, xformMat=self.xformMat)
            return self.result

    def undo(self):
        if self.origMat is None:
            print "Can't undo this transform"
        else:
            print "Undo: transformObject"
            obj = self.editor.objectMgr.findObjectById(self.uid)
            if obj:
                obj[OG.OBJ_NP].setMat(self.origMat)
            del self.origMat
            self.origMat = None

class ActionDeselectAll(ActionBase):
    """ Action class for adding new object """
    
    def __init__(self, editor, *args, **kargs):
        self.editor = editor
        function = base.direct.deselectAllCB
        ActionBase.__init__(self, function, *args, **kargs)
        self.selectedUIDs = []

    def saveStatus(self):
        selectedNPs = base.direct.selected.getSelectedAsList()
        for np in selectedNPs:
            obj = self.editor.objectMgr.findObjectByNodePath(np)
            if obj:
                uid = obj[OG.OBJ_UID]
                self.selectedUIDs.append(uid)

    def undo(self):
        print "Undo : deselectAll"
        base.direct.deselectAllCB()
        for uid in self.selectedUIDs:
            obj = self.editor.objectMgr.findObjectById(uid)
            if obj:
                self.editor.select(obj[OG.OBJ_NP], fMultiSelect=1, fUndo=0)
        self.selectedUIDs = []
