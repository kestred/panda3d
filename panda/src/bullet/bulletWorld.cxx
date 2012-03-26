// Filename: bulletWorld.cxx
// Created by:  enn0x (23Jan10)
//
////////////////////////////////////////////////////////////////////
//
// PANDA 3D SOFTWARE
// Copyright (c) Carnegie Mellon University.  All rights reserved.
//
// All use of this software is subject to the terms of the revised BSD
// license.  You should have received a copy of this license along
// with this source code in a file named "LICENSE."
//
////////////////////////////////////////////////////////////////////

#include "bulletWorld.h"
#include "bulletPersistentManifold.h"
#include "bulletShape.h"
#include "bulletSoftBodyWorldInfo.h"

#include "collideMask.h"

#ifdef HAVE_PYTHON
  #include "py_panda.h"
  #include "typedReferenceCount.h"
  #ifndef CPPPARSER
    extern EXPCL_PANDAODE Dtool_PyTypedObject Dtool_PandaNode;
  #endif
#endif

#define clamp(x, x_min, x_max) max(min(x, x_max), x_min)

TypeHandle BulletWorld::_type_handle;

PStatCollector BulletWorld::_pstat_physics("App:Bullet:DoPhysics");
PStatCollector BulletWorld::_pstat_simulation("App:Bullet:DoPhysics:Simulation");
PStatCollector BulletWorld::_pstat_debug("App:Bullet:DoPhysics:Debug");
PStatCollector BulletWorld::_pstat_p2b("App:Bullet:DoPhysics:SyncP2B");
PStatCollector BulletWorld::_pstat_b2p("App:Bullet:DoPhysics:SyncB2P");

////////////////////////////////////////////////////////////////////
//     Function: BulletWorld::Constructor
//       Access: Published
//  Description:
////////////////////////////////////////////////////////////////////
BulletWorld::
BulletWorld() {

  // Init groups filter matrix
  for (int i=0; i<32; i++) {
    _filter_cb2._collide[i].clear();
    _filter_cb2._collide[i].set_bit(i);
  }

  // Broadphase
  btScalar dx(bullet_sap_extents);
  btVector3 extents(dx, dx, dx);

  switch (bullet_broadphase_algorithm) {
    case BA_sweep_and_prune:
      _broadphase = new btAxisSweep3(extents, extents, 1024);
      break;
    case BA_dynamic_aabb_tree:
      _broadphase = new btDbvtBroadphase();
      break;
    default:
      bullet_cat.error() << "no proper broadphase algorithm!" << endl;
  }

  // Configuration
  _configuration = new btSoftBodyRigidBodyCollisionConfiguration();

  // Dispatcher
  _dispatcher = new btCollisionDispatcher(_configuration);

/*
  _dispatcher.registerCollisionCreateFunc(
    BroadphaseNativeType.ConvexTriangleMeshShape,
    BroadphaseNativeType.ConvexTriangleMeshShape,
    dcc.GetCollisionAlgorithmCreateFunc(
      BroadphaseNativeType.TriangleMeshShape,
      BroadphaseNativeType.TriangleMeshShape));

  _dispatcher.registerCollisionCreateFunc(
      BroadphaseNativeType.TriangleMeshShape,
      BroadphaseNativeType.TriangleMeshShape,
      dcc.GetCollisionAlgorithmCreateFunc(
        BroadphaseNativeType.ConvexTriangleMeshShape,
        BroadphaseNativeType.ConvexTriangleMeshShape));

  _dispatcher.registerCollisionCreateFunc(
     BroadphaseNativeType.ConvexTriangleMeshShape,
     BroadphaseNativeType.ConvexTriangleMeshShape,
     dcc.GetCollisionAlgorithmCreateFunc(
       BroadphaseNativeType.ConvexTriangleMeshShape,
       BroadphaseNativeType.ConvexTriangleMeshShape));
*/

  // Solver
  _solver = new btSequentialImpulseConstraintSolver;

  // World
  _world = new btSoftRigidDynamicsWorld(_dispatcher, _broadphase, _solver, _configuration);
  _world->setGravity(btVector3(0.0f, 0.0f, 0.0f));

  // Ghost-pair callback
  _world->getPairCache()->setInternalGhostPairCallback(&_ghost_cb);

  // Filter callback
  switch (bullet_filter_algorithm) {
    case FA_mask:
      _world->getPairCache()->setOverlapFilterCallback(&_filter_cb1);
      break;
    case FA_groups_mask:
      _world->getPairCache()->setOverlapFilterCallback(&_filter_cb2);
      break;
    case FA_python_callback:
      _world->getPairCache()->setOverlapFilterCallback(&_filter_cb3);
      break;
    default:
      bullet_cat.error() << "no proper filter algorithm!" << endl;
  }

  // SoftBodyWorldInfo
  _info.m_dispatcher = _dispatcher;
  _info.m_broadphase = _broadphase;
  _info.m_gravity = _world->getGravity();
  _info.m_sparsesdf.Initialize();

  // Register GIMPACT algorithm
  btGImpactCollisionAlgorithm::registerAlgorithm(_dispatcher);

  // Some prefered settings
  _world->getDispatchInfo().m_enableSPU = true;      // default: true
  _world->getDispatchInfo().m_useContinuous = true;  // default: true
  _world->getSolverInfo().m_splitImpulse = false;    // default: false
  _world->getSolverInfo().m_numIterations = bullet_solver_iterations;
}

////////////////////////////////////////////////////////////////////
//     Function: BulletWorld::get_world_info
//       Access: Published
//  Description:
////////////////////////////////////////////////////////////////////
BulletSoftBodyWorldInfo BulletWorld::
get_world_info() {

  return BulletSoftBodyWorldInfo(_info);
}

////////////////////////////////////////////////////////////////////
//     Function: BulletWorld::set_gravity
//       Access: Published
//  Description:
////////////////////////////////////////////////////////////////////
void BulletWorld::
set_gravity(const LVector3 &gravity) {

  _world->setGravity(LVecBase3_to_btVector3(gravity));
  _info.m_gravity = _world->getGravity();
}

////////////////////////////////////////////////////////////////////
//     Function: BulletWorld::set_gravity
//       Access: Published
//  Description:
////////////////////////////////////////////////////////////////////
void BulletWorld::
set_gravity(PN_stdfloat gx, PN_stdfloat gy, PN_stdfloat gz) {

  _world->setGravity(btVector3((btScalar)gx, (btScalar)gy, (btScalar)gz));
  _info.m_gravity = _world->getGravity();
}

////////////////////////////////////////////////////////////////////
//     Function: BulletWorld::get_gravity
//       Access: Published
//  Description:
////////////////////////////////////////////////////////////////////
const LVector3 BulletWorld::
get_gravity() const {

  return btVector3_to_LVector3(_world->getGravity());
}

////////////////////////////////////////////////////////////////////
//     Function: BulletWorld::do_physics
//       Access: Published
//  Description:
////////////////////////////////////////////////////////////////////
int BulletWorld::
do_physics(PN_stdfloat dt, int max_substeps, PN_stdfloat stepsize) {

  _pstat_physics.start();

  int num_substeps = clamp(int(dt / stepsize), 1, max_substeps);

  // Synchronize Panda to Bullet
  _pstat_p2b.start();
  sync_p2b(dt, num_substeps);
  _pstat_p2b.stop();

  // Simulation
  _pstat_simulation.start();
  int n = _world->stepSimulation((btScalar)dt, max_substeps, (btScalar)stepsize);
  _pstat_simulation.stop();

  // Synchronize Bullet to Panda
  _pstat_b2p.start();
  sync_b2p();
  _info.m_sparsesdf.GarbageCollect(bullet_gc_lifetime);
  _pstat_b2p.stop();

  // Render debug
  if (_debug) {
    _pstat_debug.start();
    _debug->sync_b2p(_world);
    _pstat_debug.stop();
  }

  _pstat_physics.stop();

  return n;
}

////////////////////////////////////////////////////////////////////
//     Function: BulletWorld::sync_p2b
//       Access: Private
//  Description: 
////////////////////////////////////////////////////////////////////
void BulletWorld::
sync_p2b(PN_stdfloat dt, int num_substeps) {

  for (int i=0; i < get_num_rigid_bodies(); i++) {
    get_rigid_body(i)->sync_p2b();
  }

  for (int i=0; i < get_num_soft_bodies(); i++) {
    get_soft_body(i)->sync_p2b();
  }

  for (int i=0; i < get_num_ghosts(); i++) {
    get_ghost(i)->sync_p2b();
  }

  for (int i=0; i < get_num_characters(); i++) {
    get_character(i)->sync_p2b(dt, num_substeps);
  }
}

////////////////////////////////////////////////////////////////////
//     Function: BulletWorld::sync_b2p
//       Access: Private
//  Description: 
////////////////////////////////////////////////////////////////////
void BulletWorld::
sync_b2p() {

  for (int i=0; i < get_num_vehicles(); i++) {
    get_vehicle(i)->sync_b2p();
  }

  for (int i=0; i < get_num_rigid_bodies(); i++) {
    get_rigid_body(i)->sync_b2p();
  }

  for (int i=0; i < get_num_soft_bodies(); i++) {
    get_soft_body(i)->sync_b2p();
  }

  for (int i=0; i < get_num_ghosts(); i++) {
    get_ghost(i)->sync_b2p();
  }

  for (int i=0; i < get_num_characters(); i++) {
    get_character(i)->sync_b2p();
  }
}

////////////////////////////////////////////////////////////////////
//     Function: BulletWorld::set_debug_node
//       Access: Published
//  Description:
////////////////////////////////////////////////////////////////////
void BulletWorld::
set_debug_node(BulletDebugNode *node) {

  nassertv(node);

  _debug = node;
  _world->setDebugDrawer(&(_debug->_drawer));
}

////////////////////////////////////////////////////////////////////
//     Function: BulletWorld::clear_debug_node
//       Access: Published
//  Description:
////////////////////////////////////////////////////////////////////
void BulletWorld::
clear_debug_node() {

  _debug = NULL;
  _world->setDebugDrawer(NULL);
}

////////////////////////////////////////////////////////////////////
//     Function: BulletWorld::attach_rigid_body
//       Access: Published
//  Description:
////////////////////////////////////////////////////////////////////
void BulletWorld::
attach_rigid_body(BulletRigidBodyNode *node) {

  nassertv(node);

  btRigidBody *ptr = btRigidBody::upcast(node->get_object());

  BulletRigidBodies::iterator found;
  PT(BulletRigidBodyNode) ptnode = node;
  found = find(_bodies.begin(), _bodies.end(), ptnode);

  if (found == _bodies.end()) {
    _bodies.push_back(node);
    _world->addRigidBody(ptr);
  }
  else {
    bullet_cat.warning() << "rigid body already attached" << endl;
  }
}

////////////////////////////////////////////////////////////////////
//     Function: BulletWorld::remove_rigid_body
//       Access: Published
//  Description:
////////////////////////////////////////////////////////////////////
void BulletWorld::
remove_rigid_body(BulletRigidBodyNode *node) {

  nassertv(node);

  btRigidBody *ptr = btRigidBody::upcast(node->get_object());

  BulletRigidBodies::iterator found;
  PT(BulletRigidBodyNode) ptnode = node;
  found = find(_bodies.begin(), _bodies.end(), ptnode);

  if (found == _bodies.end()) {
    bullet_cat.warning() << "rigid body not attached" << endl;
  }
  else {
    _bodies.erase(found);
    _world->removeRigidBody(ptr);
  }
}

////////////////////////////////////////////////////////////////////
//     Function: BulletWorld::attach_soft_body
//       Access: Published
//  Description:
////////////////////////////////////////////////////////////////////
void BulletWorld::
attach_soft_body(BulletSoftBodyNode *node) {

  nassertv(node);

  btSoftBody *ptr = btSoftBody::upcast(node->get_object());

  // TODO: group/filter settings (see ghost objects too)
  short group = btBroadphaseProxy::DefaultFilter;
  short mask = btBroadphaseProxy::AllFilter;

  BulletSoftBodies::iterator found;
  PT(BulletSoftBodyNode) ptnode = node;
  found = find(_softbodies.begin(), _softbodies.end(), ptnode);

  if (found == _softbodies.end()) {
    _softbodies.push_back(node);
    _world->addSoftBody(ptr, group, mask);
  }
  else {
    bullet_cat.warning() << "soft body already attached" << endl;
  }
}

////////////////////////////////////////////////////////////////////
//     Function: BulletWorld::remove_soft_body
//       Access: Published
//  Description:
////////////////////////////////////////////////////////////////////
void BulletWorld::
remove_soft_body(BulletSoftBodyNode *node) {

  nassertv(node);

  btSoftBody *ptr = btSoftBody::upcast(node->get_object());

  BulletSoftBodies::iterator found;
  PT(BulletSoftBodyNode) ptnode = node;
  found = find(_softbodies.begin(), _softbodies.end(), ptnode);

  if (found == _softbodies.end()) {
    bullet_cat.warning() << "soft body not attached" << endl;
  }
  else {
    _softbodies.erase(found);
    _world->removeSoftBody(ptr);
  }
}

////////////////////////////////////////////////////////////////////
//     Function: BulletWorld::attach_ghost
//       Access: Published
//  Description:
////////////////////////////////////////////////////////////////////
void BulletWorld::
attach_ghost(BulletGhostNode *node) {

  nassertv(node);

  // TODO group/filter settings...
/*
enum CollisionFilterGroups { 
  DefaultFilter = 1, 
  StaticFilter = 2, 
  KinematicFilter = 4, 
  DebrisFilter = 8, 
  SensorTrigger = 16, 
  CharacterFilter = 32, 
  AllFilter = -1 
} 
*/

  short group = btBroadphaseProxy::SensorTrigger;
  short mask = btBroadphaseProxy::AllFilter
            & ~btBroadphaseProxy::StaticFilter
            & ~btBroadphaseProxy::SensorTrigger;

  btGhostObject *ptr = btGhostObject::upcast(node->get_object());

  BulletGhosts::iterator found;
  PT(BulletGhostNode) ptnode = node;
  found = find(_ghosts.begin(), _ghosts.end(), ptnode);

  if (found == _ghosts.end()) {
    _ghosts.push_back(node);
    _world->addCollisionObject(ptr, group, mask);
  }
  else {
    bullet_cat.warning() << "ghost already attached" << endl;
  }
}

////////////////////////////////////////////////////////////////////
//     Function: BulletWorld::remove_ghost
//       Access: Published
//  Description:
////////////////////////////////////////////////////////////////////
void BulletWorld::
remove_ghost(BulletGhostNode *node) {

  nassertv(node);

  btGhostObject *ptr = btGhostObject::upcast(node->get_object());

  BulletGhosts::iterator found;
  PT(BulletGhostNode) ptnode = node;
  found = find(_ghosts.begin(), _ghosts.end(), ptnode);

  if (found == _ghosts.end()) {
    bullet_cat.warning() << "ghost not attached" << endl;
  }
  else {
    _ghosts.erase(found);
    _world->removeCollisionObject(ptr);
  }
}

////////////////////////////////////////////////////////////////////
//     Function: BulletWorld::attach_character
//       Access: Published
//  Description:
////////////////////////////////////////////////////////////////////
void BulletWorld::
attach_character(BulletBaseCharacterControllerNode *node) {

  nassertv(node);

  BulletCharacterControllers::iterator found;
  PT(BulletBaseCharacterControllerNode) ptnode = node;
  found = find(_characters.begin(), _characters.end(), ptnode);

  if (found == _characters.end()) {
    _characters.push_back(node);
    
    _world->addCollisionObject(node->get_ghost(),
      btBroadphaseProxy::CharacterFilter,
      btBroadphaseProxy::StaticFilter|btBroadphaseProxy::DefaultFilter);

    _world->addCharacter(node->get_character());
  }
  else {
    bullet_cat.warning() << "character already attached" << endl;
  }
}

////////////////////////////////////////////////////////////////////
//     Function: BulletWorld::remove_character
//       Access: Published
//  Description:
////////////////////////////////////////////////////////////////////
void BulletWorld::
remove_character(BulletBaseCharacterControllerNode *node) {

  nassertv(node);

  BulletCharacterControllers::iterator found;
  PT(BulletBaseCharacterControllerNode) ptnode = node;
  found = find(_characters.begin(), _characters.end(), ptnode);

  if (found == _characters.end()) {
    bullet_cat.warning() << "character not attached" << endl;
  }
  else {
    _characters.erase(found);
    _world->removeCollisionObject(node->get_ghost());
    _world->removeCharacter(node->get_character());
  }
}

////////////////////////////////////////////////////////////////////
//     Function: BulletWorld::attach_vehicle
//       Access: Published
//  Description:
////////////////////////////////////////////////////////////////////
void BulletWorld::
attach_vehicle(BulletVehicle *vehicle) {

  nassertv(vehicle);

  BulletVehicles::iterator found;
  PT(BulletVehicle) ptvehicle = vehicle;
  found = find(_vehicles.begin(), _vehicles.end(), ptvehicle);

  if (found == _vehicles.end()) {
    _vehicles.push_back(vehicle);
    _world->addVehicle(vehicle->get_vehicle());
  }
  else {
    bullet_cat.warning() << "vehicle already attached" << endl;
  }
}

////////////////////////////////////////////////////////////////////
//     Function: BulletWorld::remove_vehicle
//       Access: Published
//  Description:
////////////////////////////////////////////////////////////////////
void BulletWorld::
remove_vehicle(BulletVehicle *vehicle) {

  nassertv(vehicle);

  remove_rigid_body(vehicle->get_chassis());

  BulletVehicles::iterator found;
  PT(BulletVehicle) ptvehicle = vehicle;
  found = find(_vehicles.begin(), _vehicles.end(), ptvehicle);

  if (found == _vehicles.end()) {
    bullet_cat.warning() << "vehicle not attached" << endl;
  }
  else {
    _vehicles.erase(found);
    _world->removeVehicle(vehicle->get_vehicle());
  }
}

////////////////////////////////////////////////////////////////////
//     Function: BulletWorld::attach_constraint
//       Access: Published
//  Description:
////////////////////////////////////////////////////////////////////
void BulletWorld::
attach_constraint(BulletConstraint *constraint) {

  nassertv(constraint);

  BulletConstraints::iterator found;
  PT(BulletConstraint) ptconstraint = constraint;
  found = find(_constraints.begin(), _constraints.end(), ptconstraint);

  if (found == _constraints.end()) {
    _constraints.push_back(constraint);
    _world->addConstraint(constraint->ptr());
  }
  else {
    bullet_cat.warning() << "constraint already attached" << endl;
  }
}

////////////////////////////////////////////////////////////////////
//     Function: BulletWorld::remove_constraint
//       Access: Published
//  Description:
////////////////////////////////////////////////////////////////////
void BulletWorld::
remove_constraint(BulletConstraint *constraint) {

  nassertv(constraint);

  BulletConstraints::iterator found;
  PT(BulletConstraint) ptconstraint = constraint;
  found = find(_constraints.begin(), _constraints.end(), ptconstraint);

  if (found == _constraints.end()) {
    bullet_cat.warning() << "constraint not attached" << endl;
  }
  else {
    _constraints.erase(found);
    _world->removeConstraint(constraint->ptr());
  }
}

////////////////////////////////////////////////////////////////////
//     Function: BulletWorld::ray_test_closest
//       Access: Published
//  Description:
////////////////////////////////////////////////////////////////////
BulletClosestHitRayResult BulletWorld::
ray_test_closest(const LPoint3 &from_pos, const LPoint3 &to_pos, const CollideMask &mask) const {

  nassertr(!from_pos.is_nan(), BulletClosestHitRayResult::empty());
  nassertr(!to_pos.is_nan(), BulletClosestHitRayResult::empty());

  const btVector3 from = LVecBase3_to_btVector3(from_pos);
  const btVector3 to = LVecBase3_to_btVector3(to_pos);

  BulletClosestHitRayResult cb(from, to, mask);
  _world->rayTest(from, to, cb);
  return cb;
}

////////////////////////////////////////////////////////////////////
//     Function: BulletWorld::ray_test_all
//       Access: Published
//  Description:
////////////////////////////////////////////////////////////////////
BulletAllHitsRayResult BulletWorld::
ray_test_all(const LPoint3 &from_pos, const LPoint3 &to_pos, const CollideMask &mask) const {

  nassertr(!from_pos.is_nan(), BulletAllHitsRayResult::empty());
  nassertr(!to_pos.is_nan(), BulletAllHitsRayResult::empty());

  const btVector3 from = LVecBase3_to_btVector3(from_pos);
  const btVector3 to = LVecBase3_to_btVector3(to_pos);

  BulletAllHitsRayResult cb(from, to, mask);
  _world->rayTest(from, to, cb);
  return cb;
}

////////////////////////////////////////////////////////////////////
//     Function: BulletWorld::sweep_test_closest
//       Access: Published
//  Description:
////////////////////////////////////////////////////////////////////
BulletClosestHitSweepResult BulletWorld::
sweep_test_closest(BulletShape *shape, const TransformState &from_ts, const TransformState &to_ts, const CollideMask &mask, PN_stdfloat penetration) const {

  nassertr(shape, BulletClosestHitSweepResult::empty());
  nassertr(shape->is_convex(), BulletClosestHitSweepResult::empty());
  nassertr(!from_ts.is_invalid(), BulletClosestHitSweepResult::empty());
  nassertr(!to_ts.is_invalid(), BulletClosestHitSweepResult::empty());

  const btConvexShape *convex = (const btConvexShape *) shape->ptr();
  const btVector3 from_pos = LVecBase3_to_btVector3(from_ts.get_pos());
  const btVector3 to_pos = LVecBase3_to_btVector3(to_ts.get_pos());
  const btTransform from_trans = LMatrix4_to_btTrans(from_ts.get_mat());
  const btTransform to_trans = LMatrix4_to_btTrans(to_ts.get_mat());

  BulletClosestHitSweepResult cb(from_pos, to_pos, mask);
  _world->convexSweepTest(convex, from_trans, to_trans, cb, penetration);
  return cb;  
}

////////////////////////////////////////////////////////////////////
//     Function: BulletWorld::contact_test
//       Access: Published
//  Description: 
////////////////////////////////////////////////////////////////////
BulletContactResult BulletWorld::
contact_test(PandaNode *node) const {

  btCollisionObject *obj = get_collision_object(node);

  BulletContactResult cb;

  if (obj) {
    _world->contactTest(obj, cb);
  }

  return cb;
}

////////////////////////////////////////////////////////////////////
//     Function: BulletWorld::contact_pair_test
//       Access: Published
//  Description: 
////////////////////////////////////////////////////////////////////
BulletContactResult BulletWorld::
contact_test_pair(PandaNode *node0, PandaNode *node1) const {

  btCollisionObject *obj0 = get_collision_object(node0);
  btCollisionObject *obj1 = get_collision_object(node1);

  BulletContactResult cb;

  if (obj0 && obj1) {
    _world->contactPairTest(obj0, obj1, cb);
  }

  return cb;
}

////////////////////////////////////////////////////////////////////
//     Function: BulletWorld::get_manifold
//       Access: Published
//  Description: 
////////////////////////////////////////////////////////////////////
BulletPersistentManifold *BulletWorld::
get_manifold(int idx) const {

  nassertr(idx < get_num_manifolds(), NULL);

  btPersistentManifold *ptr = _dispatcher->getManifoldByIndexInternal(idx);
  return (ptr) ? new BulletPersistentManifold(ptr) : NULL;
}

////////////////////////////////////////////////////////////////////
//     Function: BulletWorld::get_collision_object
//       Access: Public
//  Description: 
////////////////////////////////////////////////////////////////////
btCollisionObject *BulletWorld::
get_collision_object(PandaNode *node) {

  if (node->is_of_type(BulletRigidBodyNode::get_class_type())) {
    return ((BulletRigidBodyNode *)node)->get_object();
  }
  else if (node->is_of_type(BulletGhostNode::get_class_type())) {
    return ((BulletGhostNode *)node)->get_object();
  }
  else if (node->is_of_type(BulletBaseCharacterControllerNode::get_class_type())) {
    return ((BulletBaseCharacterControllerNode *)node)->get_ghost();
  }
  else if (node->is_of_type(BulletSoftBodyNode::get_class_type())) {
    return ((BulletSoftBodyNode *)node)->get_object();
  }

  return NULL;
}

////////////////////////////////////////////////////////////////////
//     Function: BulletWorld::set_group_collision_flag
//       Access: Public
//  Description: 
////////////////////////////////////////////////////////////////////
void BulletWorld::
set_group_collision_flag(unsigned int group1, unsigned int group2, bool enable) {

  if (bullet_filter_algorithm != FA_groups_mask) {
    bullet_cat.warning() << "filter algorithm is not 'groups-mask'" << endl;
  }

  _filter_cb2._collide[group1].set_bit_to(group2, enable);
  _filter_cb2._collide[group2].set_bit_to(group1, enable);
}

////////////////////////////////////////////////////////////////////
//     Function: BulletWorld::get_collision_object
//       Access: Public
//  Description: 
////////////////////////////////////////////////////////////////////
bool BulletWorld::
get_group_collision_flag(unsigned int group1, unsigned int group2) const {

  return _filter_cb2._collide[group1].get_bit(group2);
}

#ifdef HAVE_PYTHON
////////////////////////////////////////////////////////////////////
//     Function: BulletWorld::set_python_filter_callback
//       Access: Published
//  Description:
////////////////////////////////////////////////////////////////////
void BulletWorld::
set_python_filter_callback(PyObject *callback) {

  nassertv(callback != NULL);

  if (!PyCallable_Check(callback)) {
    PyErr_Format(PyExc_TypeError, "'%s' object is not callable", callback->ob_type->tp_name);
    return;
  }

  if (bullet_filter_algorithm != FA_python_callback) {
    bullet_cat.warning() << "filter algorithm is not 'python-callback'" << endl;
  }

  _filter_cb3._python_callback = callback;
  Py_XINCREF(_filter_cb3._python_callback);
}
#endif

////////////////////////////////////////////////////////////////////
//     Function: BulletWorld::FilterCallback1::needBroadphaseCollision
//       Access: Published
//  Description:
////////////////////////////////////////////////////////////////////
bool BulletWorld::btFilterCallback1::
needBroadphaseCollision(btBroadphaseProxy* proxy0, btBroadphaseProxy* proxy1) const {

  btCollisionObject *obj0 = (btCollisionObject *) proxy0->m_clientObject;
  btCollisionObject *obj1 = (btCollisionObject *) proxy1->m_clientObject;

  nassertr(obj0, false);
  nassertr(obj1, false);

  PandaNode *node0 = (PandaNode *) obj0->getUserPointer();
  PandaNode *node1 = (PandaNode *) obj1->getUserPointer();

  nassertr(node0, false);
  nassertr(node1, false);

  CollideMask mask0 = node0->get_into_collide_mask();
  CollideMask mask1 = node1->get_into_collide_mask();

  return (mask0 & mask1) != 0;
}

////////////////////////////////////////////////////////////////////
//     Function: BulletWorld::FilterCallback2::needBroadphaseCollision
//       Access: Published
//  Description:
////////////////////////////////////////////////////////////////////
bool BulletWorld::btFilterCallback2::
needBroadphaseCollision(btBroadphaseProxy* proxy0, btBroadphaseProxy* proxy1) const {

  btCollisionObject *obj0 = (btCollisionObject *) proxy0->m_clientObject;
  btCollisionObject *obj1 = (btCollisionObject *) proxy1->m_clientObject;

  nassertr(obj0, false);
  nassertr(obj1, false);

  PandaNode *node0 = (PandaNode *) obj0->getUserPointer();
  PandaNode *node1 = (PandaNode *) obj1->getUserPointer();

  nassertr(node0, false);
  nassertr(node1, false);

  CollideMask mask0 = node0->get_into_collide_mask();
  CollideMask mask1 = node1->get_into_collide_mask();

  for (int i=0; i<32; i++) {
    if (mask0.get_bit(i)) {
      if ((_collide[i] & mask1) != 0)
        return true;
    }
  }

  return false;
}

#ifdef HAVE_PYTHON
////////////////////////////////////////////////////////////////////
//     Function: BulletWorld::FilterCallback3::needBroadphaseCollision
//       Access: Published
//  Description:
////////////////////////////////////////////////////////////////////
bool BulletWorld::btFilterCallback3::
needBroadphaseCollision(btBroadphaseProxy* proxy0, btBroadphaseProxy* proxy1) const {

  nassertr(_python_callback, false);

  btCollisionObject *obj0 = (btCollisionObject *) proxy0->m_clientObject;
  btCollisionObject *obj1 = (btCollisionObject *) proxy1->m_clientObject;

  nassertr(obj0, false);
  nassertr(obj1, false);

  PandaNode *node0 = (PandaNode *) obj0->getUserPointer();
  PandaNode *node1 = (PandaNode *) obj1->getUserPointer();

  nassertr(node0, false);
  nassertr(node1, false);

  PyObject *p0 = DTool_CreatePyInstanceTyped(node0, Dtool_PandaNode, true, false, node0->get_type_index());
  PyObject *p1 = DTool_CreatePyInstanceTyped(node1, Dtool_PandaNode, true, false, node1->get_type_index());

  PyObject *result = PyEval_CallFunction(_python_callback, "OO", p0, p1);

  bool collide = false;

  if (!result) {
    bullet_cat.error() << "An error occurred while calling python function!" << endl;
    PyErr_Print();
  }
  else {
    int v = PyObject_IsTrue(result);
    if (v == 1) {
      collide = true;
    }
    else if (v == 0) {
      collide = false;
    }
    else {
      bullet_cat.error() << "Python callback function must return a bool object" << endl;
    }
    Py_DECREF(result);
  }

  //Py_XDECREF(p0);
  //Py_XDECREF(p1);

  return collide;
}
#endif

////////////////////////////////////////////////////////////////////
//     Function: BulletWorld::BroadphaseAlgorithm ostream operator
//  Description:
////////////////////////////////////////////////////////////////////
ostream &
operator << (ostream &out, BulletWorld::BroadphaseAlgorithm algorithm) {

  switch (algorithm) {
  case BulletWorld::BA_sweep_and_prune:
    return out << "sap";

  case BulletWorld::BA_dynamic_aabb_tree:
    return out << "aabb";
  };

  return out << "**invalid BulletWorld::BroadphaseAlgorithm(" << (int)algorithm << ")**";
}

////////////////////////////////////////////////////////////////////
//     Function: BulletWorld::BroadphaseAlgorithm istream operator
//  Description:
////////////////////////////////////////////////////////////////////
istream &
operator >> (istream &in, BulletWorld::BroadphaseAlgorithm &algorithm) {
  string word;
  in >> word;

  if (word == "sap") {
    algorithm = BulletWorld::BA_sweep_and_prune;
  }
  else if (word == "aabb") {
    algorithm = BulletWorld::BA_dynamic_aabb_tree;
  } 
  else {
    bullet_cat.error()
      << "Invalid BulletWorld::BroadphaseAlgorithm: " << word << "\n";
    algorithm = BulletWorld::BA_dynamic_aabb_tree;
  }

  return in;
}

////////////////////////////////////////////////////////////////////
//     Function: BulletWorld::FilterAlgorithm ostream operator
//  Description:
////////////////////////////////////////////////////////////////////
ostream &
operator << (ostream &out, BulletWorld::FilterAlgorithm algorithm) {

  switch (algorithm) {
  case BulletWorld::FA_mask:
    return out << "mask";

  case BulletWorld::FA_groups_mask:
    return out << "groups-mask";

#ifdef HAVE_PYTHON
  case BulletWorld::FA_python_callback:
    return out << "python-callback";
#endif
  };
  return out << "**invalid BulletWorld::FilterAlgorithm(" << (int)algorithm << ")**";
}

////////////////////////////////////////////////////////////////////
//     Function: BulletWorld::FilterAlgorithm istream operator
//  Description:
////////////////////////////////////////////////////////////////////
istream &
operator >> (istream &in, BulletWorld::FilterAlgorithm &algorithm) {
  string word;
  in >> word;

  if (word == "mask") {
    algorithm = BulletWorld::FA_mask;
  }
  else if (word == "groups-mask") {
    algorithm = BulletWorld::FA_groups_mask;
  } 
#ifdef HAVE_PYTHON
  else if (word == "python-callback") {
    algorithm = BulletWorld::FA_python_callback;
  }
#endif
  else {
    bullet_cat.error()
      << "Invalid BulletWorld::FilterAlgorithm: " << word << "\n";
    algorithm = BulletWorld::FA_mask;
  }

  return in;
}

