"""CutScene.py"""


import DirectObject
import DirectNotifyGlobal
import Entity

from PandaModules import *
from ShowBaseGlobal import *
from IntervalGlobal import *
from ClockDelta import *

import ToontownGlobals
import DirectNotifyGlobal
import FSM
import DistributedInteractiveEntity
import DelayDelete
import Localizer

# effects #

def nothing(self, track, subjectNodePath, duration):
    assert(self.debugPrint(
            "nothing(track=%s, subjectNodePath=%s, duration=%s)"%(
            track, subjectNodePath, duration)))
    return track

def irisInOut(self, track, subjectNodePath, duration):
    assert(self.debugPrint(
            "irisInOut(track=%s, subjectNodePath=%s, duration=%s)"%(
            track, subjectNodePath, duration)))
    track.append(Sequence(
        Func(base.transitions.irisOut, 0.5),
        Func(base.transitions.irisIn, 1.5),
        Wait(duration),
        Func(base.transitions.irisOut, 1.0),
        Func(base.transitions.irisIn, 1.0),
        ))
    return track

def letterBox(self, track, subjectNodePath, duration):
    assert(self.debugPrint(
            "letterBox(track=%s, subjectNodePath=%s, duration=%s)"%(
            track, subjectNodePath, duration)))
    track.append(Sequence(
        #Func(base.transitions.letterBox, 0.5),
        Wait(duration),
        #Func(base.transitions.letterBox, 0.5),
        ))
    return track

# motions #

def foo1(self, track, subjectNodePath, duration):
    assert(self.debugPrint(
            "foo1(track=%s, subjectNodePath=%s, duration=%s)"%(
            track, subjectNodePath, duration)))
    track.append(Sequence(
        Func(toonbase.localToon.stopUpdateSmartCamera),
        LerpPosHprInterval(node=camera,
                           other=subjectNodePath,
                           duration=0.0,
                           pos=Point3(-2, -35, 7.5),
                           hpr=VBase3(-7, 0, 0),
                           blendType="easeInOut"),
        LerpPosHprInterval(node=camera,
                           other=subjectNodePath,
                           duration=duration,
                           pos=Point3(2, -22, 7.5),
                           hpr=VBase3(4, 0, 0),
                           blendType="easeInOut"),
        LerpPosHprInterval(node=camera,
                           other=subjectNodePath,
                           duration=0.0,
                           pos=Point3(0, -28, 7.5),
                           hpr=VBase3(0, 0, 0),
                           blendType="easeInOut"),
        Func(toonbase.localToon.startUpdateSmartCamera),
        ))
    return track


class CutScene(Entity.Entity, DirectObject.DirectObject):
    if __debug__:
        notify = DirectNotifyGlobal.directNotify.newCategory('CutScene')
    
    effects={
        "nothing": nothing,
        "irisInOut": irisInOut,
        "letterBox": letterBox,
    }
    
    motions={
        "foo1": foo1,
    }

    def __init__(self, level, entId):
        assert(self.debugPrint(
                "CutScene(level=%s, entId=%s)"
                %(level, entId)))
        DirectObject.DirectObject.__init__(self)
        Entity.Entity.__init__(self, level, entId)
        self.track = None
        self.setEffect(self.effect)
        self.setMotion(self.motion)
        self.subjectNodePath = render.attachNewNode("CutScene")
        self.subjectNodePath.setPos(self.pos)
        self.subjectNodePath.setHpr(self.hpr)
        #self.setSubjectNodePath(self.subjectNodePath)
        self.setStartStop(self.startStopEvent)

    def destroy(self):
        assert(self.debugPrint("destroy()"))
        self.ignore(self.startStopEvent)
        self.startStopEvent = None
        Entity.Entity.destroy(self)
        #DirectObject.DirectObject.destroy(self)
    
    def setEffect(self, effect):
        assert(self.debugPrint("setEffect(effect=%s)"%(effect,)))
        self.effect=effect
        assert self.effects[effect]
        self.getEffect=self.effects[effect]
    
    def setMotion(self, motion):
        assert(self.debugPrint("setMotion(motion=%s)"%(motion,)))
        self.motionType=motion
        assert self.motions[motion]
        self.getMotion=self.motions[motion]
    
    def setSubjectNodePath(self, subjectNodePath):
        assert(self.debugPrint("setSubjectNodePath(subjectNodePath=%s)"%(subjectNodePath,)))
        self.subjectNodePath=subjectNodePath
    
    def startOrStop(self, start):
        assert(self.debugPrint("startOrStop(start=%s)"%(start,)))
        trackName = "cutSceneTrack-%d" % (id(self),)
        if start:
            if self.track:
                self.track.finish()
                self.track = None
            track = Parallel(name = trackName)
            track = self.getEffect(self, track, self.subjectNodePath, self.duration)
            track = self.getMotion(self, track, self.subjectNodePath, self.duration)
            track = Sequence(Wait(0.8), track)
            track.start(0.0)
            self.track = track
        else:
            if self.track:
                self.track.finish()
                self.track = None
    
    def setStartStop(self, event):
        assert(self.debugPrint("setStartStop(event=%s)"%(event,)))
        if self.startStopEvent:
            self.ignore(self.startStopEvent)
        self.startStopEvent = "switch-%s"%(event,)
        if self.startStopEvent:
            self.accept(self.startStopEvent, self.startOrStop)
    
    def getName(self):
        #return "CutScene-%s"%(self.entId,)
        return "switch-%s"%(self.entId,)
