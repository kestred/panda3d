"""NonPhysicsWalker.py is for avatars."""

from ShowBaseGlobal import *

import DirectNotifyGlobal
import DirectObject

class NonPhysicsWalker(DirectObject.DirectObject):

    notify = DirectNotifyGlobal.directNotify.newCategory("NonPhysicsWalker")

    # special methods
    def __init__(self):
        DirectObject.DirectObject.__init__(self)
        self.forwardButton=0
        self.reverseButton=0
        self.jumpButton=0
        self.leftButton=0
        self.rightButton=0
        self.speed=0.0
        self.rotationSpeed=0.0
        self.vel=Vec3(0.0, 0.0, 0.0)
        self.stopThisFrame = 0
        self.fSlide = 0

    def setWalkSpeed(self, forward, jump, reverse, rotate):
        assert(self.debugPrint("setWalkSpeed()"))
        self.avatarControlForwardSpeed=forward
        #self.avatarControlJumpForce=jump
        self.avatarControlReverseSpeed=reverse
        self.avatarControlRotateSpeed=rotate

    def getSpeeds(self):
        #assert(self.debugPrint("getSpeeds()"))
        return (self.speed, self.rotationSpeed)

    def initializeCollisions(self, collisionTraverser, avatarNodePath, 
            wallCollideMask, floorCollideMask,
            avatarRadius = 1.4, floorOffset = 1.0):
        """
        Set up the avatar for collisions
        """
        self.cTrav = collisionTraverser
        self.avatarNodePath = avatarNodePath

        # Set up the collision sphere
        # This is a sphere on the ground to detect barrier collisions
        self.cSphere = CollisionSphere(0.0, 0.0, 0.0, avatarRadius)
        self.cSphereNode = CollisionNode('cSphereNode')
        self.cSphereNode.addSolid(self.cSphere)
        self.cSphereNodePath = avatarNodePath.attachNewNode(self.cSphereNode)
        self.cSphereBitMask = wallCollideMask

        self.cSphereNode.setFromCollideMask(self.cSphereBitMask)
        self.cSphereNode.setIntoCollideMask(BitMask32.allOff())

        # Set up the collison ray
        # This is a ray cast from your head down to detect floor polygons
        # A toon is about 4.0 feet high, so start it there
        self.cRay = CollisionRay(0.0, 0.0, 4.0, 0.0, 0.0, -1.0)
        self.cRayNode = CollisionNode('cRayNode')
        self.cRayNode.addSolid(self.cRay)
        self.cRayNodePath = avatarNodePath.attachNewNode(self.cRayNode)
        self.cRayBitMask = floorCollideMask
        self.cRayNode.setFromCollideMask(self.cRayBitMask)
        self.cRayNode.setIntoCollideMask(BitMask32.allOff())

        # set up wall collision mechanism
        self.pusher = CollisionHandlerPusher()
        self.pusher.setInPattern("enter%in")
        self.pusher.setOutPattern("exit%in")

        # set up floor collision mechanism
        self.lifter = CollisionHandlerFloor()
        self.lifter.setInPattern("on-floor")
        self.lifter.setOutPattern("off-floor")
        self.lifter.setOffset(floorOffset)

        # Limit our rate-of-fall with the lifter.
        # If this is too low, we actually "fall" off steep stairs
        # and float above them as we go down. I increased this
        # from 8.0 to 16.0 to prevent this
        self.lifter.setMaxVelocity(16.0)

        # activate the collider with the traverser and pusher
        self.collisionsOn()
        
        self.pusher.addColliderNode(self.cSphereNode, avatarNodePath.node())
        self.lifter.addColliderNode(self.cRayNode, avatarNodePath.node())

    def deleteCollisions(self):
        del self.cTrav

        del self.cSphere
        del self.cSphereNode
        self.cSphereNodePath.removeNode()
        del self.cSphereNodePath

        del self.cRay
        del self.cRayNode
        self.cRayNodePath.removeNode()
        del self.cRayNodePath

        del self.pusher
        del self.lifter

    def collisionsOff(self):
        self.cTrav.removeCollider(self.cSphereNode)
        self.cTrav.removeCollider(self.cRayNode)

        # Now that we have disabled collisions, make one more pass
        # right now to ensure we aren't standing in a wall.
        self.oneTimeCollide()

    def collisionsOn(self):
        self.cTrav.addCollider(self.cSphereNode, self.pusher)
        self.cTrav.addCollider(self.cRayNode, self.lifter)

    def oneTimeCollide(self):
        """
        Makes one quick collision pass for the avatar, for instance as
        a one-time straighten-things-up operation after collisions
        have been disabled.
        """
        tempCTrav = CollisionTraverser()
        tempCTrav.addCollider(self.cSphereNode, self.pusher)
        tempCTrav.addCollider(self.cRayNode, self.lifter)
        tempCTrav.traverse(render)

    def handleAvatarControls(self, task):
        """
        Check on the arrow keys and update the avatar.
        """
        # Determine what the speeds are based on the buttons:
        self.speed=(self.forwardButton and self.avatarControlForwardSpeed or 
                    self.reverseButton and -self.avatarControlReverseSpeed)
        # Should fSlide be renamed slideButton?
        self.slideSpeed=self.fSlide and (
                (self.leftButton and -self.avatarControlForwardSpeed) or 
                (self.rightButton and self.avatarControlForwardSpeed))
        self.rotationSpeed=not self.fSlide and (
                (self.leftButton and self.avatarControlRotateSpeed) or
                (self.rightButton and -self.avatarControlRotateSpeed))
        # How far did we move based on the amount of time elapsed?
        dt=min(ClockObject.getGlobalClock().getDt(), 0.1)
        # Check to see if we're moving at all:
        if self.speed or self.slideSpeed or self.rotationSpeed:
            if self.stopThisFrame:
                distance = 0.0
                slideDistance = 0.0
                rotation = 0.0
                self.stopThisFrame = 0
            else:
                distance = dt * self.speed
                slideDistance = dt * self.slideSpeed
                rotation = dt * self.rotationSpeed

            # Take a step in the direction of our previous heading.
            self.vel=Vec3(Vec3.forward() * distance + 
                          Vec3.right() * slideDistance)
            if self.vel != Vec3.zero():
                # rotMat is the rotation matrix corresponding to
                # our previous heading.
                rotMat=Mat3.rotateMatNormaxis(self.avatarNodePath.getH(), Vec3.up())
                step=rotMat.xform(self.vel)
                self.avatarNodePath.setPos(Point3(self.avatarNodePath.getPos()+step))
            self.avatarNodePath.setH(self.avatarNodePath.getH()+rotation)
            messenger.send("avatarMoving")
        else:
            self.vel.set(0.0, 0.0, 0.0)
        # Set collision sphere node:
        self.cSphereNode.setVelocity(self.vel)
        return Task.cont

    def enableAvatarControls(self):
        """
        Activate the arrow keys, etc.
        """
        self.accept("control-arrow_left", self.moveTurnLeft, [1])
        self.accept("control-arrow_left-up", self.moveTurnLeft, [0])
        self.accept("control-arrow_right", self.moveTurnRight, [1])
        self.accept("control-arrow_right-up", self.moveTurnRight, [0])
        self.accept("control-arrow_up", self.moveForward, [1])
        self.accept("control-arrow_up-up", self.moveForward, [0])
        self.accept("control-arrow_down", self.moveInReverse, [1])
        self.accept("control-arrow_down-up", self.moveInReverse, [0])
        
        self.accept("arrow_left", self.moveTurnLeft, [1])
        self.accept("arrow_left-up", self.moveTurnLeft, [0])
        self.accept("arrow_right", self.moveTurnRight, [1])
        self.accept("arrow_right-up", self.moveTurnRight, [0])
        self.accept("arrow_up", self.moveForward, [1])
        self.accept("arrow_up-up", self.moveForward, [0])
        self.accept("arrow_down", self.moveInReverse, [1])
        self.accept("arrow_down-up", self.moveInReverse, [0])

        taskName = "AvatarControls%s"%(id(self),)
        # remove any old
        taskMgr.remove(taskName)
        # spawn the new task
        taskMgr.add(self.handleAvatarControls, taskName)

    def disableAvatarControls(self):
        """
        Ignore the arrow keys, etc.
        """
        taskName = "AvatarControls%s"%(id(self),)
        taskMgr.remove(taskName)

        self.ignore("control")
        self.ignore("control-up")
        self.ignore("control-arrow_left")
        self.ignore("control-arrow_left-up")
        self.ignore("control-arrow_right")
        self.ignore("control-arrow_right-up")
        self.ignore("control-arrow_up")
        self.ignore("control-arrow_up-up")
        self.ignore("control-arrow_down")
        self.ignore("control-arrow_down-up")

        self.ignore("arrow_left")
        self.ignore("arrow_left-up")
        self.ignore("arrow_right")
        self.ignore("arrow_right-up")
        self.ignore("arrow_up")
        self.ignore("arrow_up-up")
        self.ignore("arrow_down")
        self.ignore("arrow_down-up")

        # reset state
        self.moveTurnLeft(0)
        self.moveTurnRight(0)
        self.moveForward(0)
        self.moveInReverse(0)
        self.moveJumpLeft(0)
        self.moveJumpRight(0)
        self.moveJumpForward(0)
        self.moveJumpInReverse(0)
        self.moveJump(0)

    def moveTurnLeft(self, isButtonDown):
        self.leftButton=isButtonDown

    def moveTurnRight(self, isButtonDown):
        self.rightButton=isButtonDown

    def moveForward(self, isButtonDown):
        self.forwardButton=isButtonDown

    def moveInReverse(self, isButtonDown):
        self.reverseButton=isButtonDown

    def moveJumpLeft(self, isButtonDown):
        self.jumpButton=isButtonDown
        self.leftButton=isButtonDown

    def moveJumpRight(self, isButtonDown):
        self.jumpButton=isButtonDown
        self.rightButton=isButtonDown

    def moveJumpForward(self, isButtonDown):
        self.jumpButton=isButtonDown
        self.forwardButton=isButtonDown

    def moveJumpInReverse(self, isButtonDown):
        self.jumpButton=isButtonDown
        self.reverseButton=isButtonDown

    def moveJump(self, isButtonDown):
        self.jumpButton=isButtonDown

    def toggleSlide(self):
        self.fSlide = not self.fSlide

    def enableSlideMode(self):
        self.accept("control-up", self.toggleSlide)

    def disableSlideMode(self):
        self.fSlide = 0
        self.ignore("control-up")

    def slideLeft(self, isButtonDown):
        self.slideLeftButton=isButtonDown

    def slideRight(self, isButtonDown):
        self.slideRightButton=isButtonDown
    
    if __debug__:
        def debugPrint(self, message):
            """for debugging"""
            return self.notify.debug(
                    str(id(self))+' '+message)
