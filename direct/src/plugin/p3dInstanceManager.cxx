// Filename: p3dInstanceManager.cxx
// Created by:  drose (29May09)
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

#include "p3dInstanceManager.h"
#include "p3dInstance.h"
#include "p3dSession.h"
#include "p3dPackage.h"
#include "p3d_plugin_config.h"
#include "find_root_dir.h"
#include "mkdir_complete.h"

#ifdef _WIN32
#include <shlobj.h>
#else
//#include <sys/stat.h>
#endif

P3DInstanceManager *P3DInstanceManager::_global_ptr;

////////////////////////////////////////////////////////////////////
//     Function: P3DInstanceManager::Constructor
//       Access: Private
//  Description: 
////////////////////////////////////////////////////////////////////
P3DInstanceManager::
P3DInstanceManager() {
  _is_initialized = false;
  _unique_session_index = 0;

  _request_seq = 0;
#ifdef _WIN32
  _request_ready = CreateEvent(NULL, false, false, NULL);
#else
  INIT_LOCK(_request_ready_lock);
  pthread_cond_init(&_request_ready_cvar, NULL);
#endif

#ifdef _WIN32
  // Ensure the appropriate Windows common controls are available to
  // this application.
  INITCOMMONCONTROLSEX icc;
  icc.dwSize = sizeof(icc);
  icc.dwICC = ICC_PROGRESS_CLASS;
  InitCommonControlsEx(&icc);
#endif
}

////////////////////////////////////////////////////////////////////
//     Function: P3DInstanceManager::Destructor
//       Access: Private
//  Description: 
////////////////////////////////////////////////////////////////////
P3DInstanceManager::
~P3DInstanceManager() {
  // Actually, this destructor is never called, since this is a global
  // object that never gets deleted.

  assert(_instances.empty());
  assert(_sessions.empty());

#ifdef _WIN32
  CloseHandle(_request_ready);
#else
  DESTROY_LOCK(_request_ready_lock);
  pthread_cond_destroy(&_request_ready_cvar);
#endif
}

////////////////////////////////////////////////////////////////////
//     Function: P3DInstanceManager::initialize
//       Access: Public
//  Description: Called by the host at application startup.  It
//               returns true if the DLL is successfully initialized,
//               false if it should be immediately shut down and
//               redownloaded.
////////////////////////////////////////////////////////////////////
bool P3DInstanceManager::
initialize() {
  _root_dir = find_root_dir();
  _download_url = P3D_PLUGIN_DOWNLOAD;
  _platform = P3D_PLUGIN_PLATFORM;

  nout << "_root_dir = " << _root_dir << ", download = " 
       << _download_url << "\n" << flush;

  if (_root_dir.empty()) {
    nout << "Could not find root directory.\n" << flush;
    return false;
  }

  _is_initialized = true;

  return true;
}

////////////////////////////////////////////////////////////////////
//     Function: P3DInstanceManager::create_instance
//       Access: Public
//  Description: Returns a newly-allocated P3DInstance with the
//               indicated startup information.
////////////////////////////////////////////////////////////////////
P3DInstance *P3DInstanceManager::
create_instance(P3D_request_ready_func *func, void *user_data) {
  P3DInstance *inst = new P3DInstance(func, user_data);
  _instances.insert(inst);

  return inst;
}

////////////////////////////////////////////////////////////////////
//     Function: P3DInstanceManager::start_instance
//       Access: Public
//  Description: Actually starts the instance running.  Before this
//               call, the instance is in an indeterminate state.  It
//               is an error to call this more than once for a
//               particular instance.
////////////////////////////////////////////////////////////////////
bool P3DInstanceManager::
start_instance(P3DInstance *inst, const string &p3d_filename,
               const P3D_token tokens[], size_t num_tokens) {
  if (inst->is_started()) {
    nout << "Instance started twice: " << inst << "\n" << flush;
    return false;
  }
  inst->set_fparams(P3DFileParams(p3d_filename, tokens, num_tokens));

  P3DSession *session;
  Sessions::iterator si = _sessions.find(inst->get_session_key());
  if (si == _sessions.end()) {
    session = new P3DSession(inst);
    bool inserted = _sessions.insert(Sessions::value_type(session->get_session_key(), session)).second;
    assert(inserted);
  } else {
    session = (*si).second;
  }

  session->start_instance(inst);

  return inst->is_started();
}

////////////////////////////////////////////////////////////////////
//     Function: P3DInstanceManager::finish_instance
//       Access: Public
//  Description: Terminates and removes a previously-returned
//               instance.
////////////////////////////////////////////////////////////////////
void P3DInstanceManager::
finish_instance(P3DInstance *inst) {
  nout << "finish_instance(" << inst << ")\n" << flush;
  Instances::iterator ii;
  ii = _instances.find(inst);
  assert(ii != _instances.end());
  _instances.erase(ii);

  Sessions::iterator si = _sessions.find(inst->get_session_key());
  assert(si != _sessions.end());
  P3DSession *session = (*si).second;
  session->terminate_instance(inst);

  // If that was the last instance in this session, terminate the
  // session.
  if (session->get_num_instances() == 0) {
    _sessions.erase(session->get_session_key());
    delete session;
  }

  delete inst;
}

////////////////////////////////////////////////////////////////////
//     Function: P3DInstanceManager::check_request
//       Access: Public
//  Description: If a request is currently pending on any instance,
//               returns its pointer.  Otherwise, returns NULL.
////////////////////////////////////////////////////////////////////
P3DInstance *P3DInstanceManager::
check_request() {
  Instances::iterator ii;
  for (ii = _instances.begin(); ii != _instances.end(); ++ii) {
    P3DInstance *inst = (*ii);
    if (inst->has_request()) {
      return inst;
    }
  }

  return NULL;
}

////////////////////////////////////////////////////////////////////
//     Function: P3DInstanceManager::wait_request
//       Access: Public
//  Description: Does not return until a request is pending on some
//               instance, or until no instances remain.  Use
//               check_request to retrieve the pending request.  Due
//               to the possibility of race conditions, it is possible
//               for this function to return when there is in fact no
//               request pending (another thread may have extracted
//               the request first).
////////////////////////////////////////////////////////////////////
void P3DInstanceManager::
wait_request() {
  int seq = _request_seq;

  while (true) {
    if (check_request() != (P3DInstance *)NULL) {
      return;
    }
    if (_instances.empty()) {
      return;
    }
    
    // No pending requests; go to sleep.
#ifdef _WIN32
    if (seq == _request_seq) {
      WaitForSingleObject(_request_ready, INFINITE);
    }
#else
    ACQUIRE_LOCK(_request_ready_lock);
    if (seq == _request_seq) {
      pthread_cond_wait(&_request_ready_cvar, &_request_ready_lock);
    }
    RELEASE_LOCK(_request_ready_lock);
#endif
    seq = _request_seq;
  }
}

////////////////////////////////////////////////////////////////////
//     Function: P3DInstanceManager::get_package
//       Access: Public
//  Description: Returns a (possibly shared) pointer to the indicated
//               package.
////////////////////////////////////////////////////////////////////
P3DPackage *P3DInstanceManager::
get_package(const string &package_name, const string &package_version, 
            const string &package_display_name) {
  string package_platform = get_platform();
  string key = package_name + "_" + package_version + "_" + package_platform;
  Packages::iterator pi = _packages.find(key);
  if (pi != _packages.end()) {
    return (*pi).second;
  }

  P3DPackage *package = new P3DPackage(package_name, package_version, 
                                       package_platform, package_display_name);
  bool inserted = _packages.insert(Packages::value_type(key, package)).second;
  assert(inserted);

  return package;
}

////////////////////////////////////////////////////////////////////
//     Function: P3DInstanceManager::get_unique_session_index
//       Access: Public
//  Description: Returns a number used to uniquify the session_key for
//               different instances.  This number is guaranteed to be
//               different at each call.
////////////////////////////////////////////////////////////////////
int P3DInstanceManager::
get_unique_session_index() {
  ++_unique_session_index;
  return _unique_session_index;
}

////////////////////////////////////////////////////////////////////
//     Function: P3DInstanceManager::signal_request_ready
//       Access: Public
//  Description: May be called in any thread to indicate that a new
//               P3D_request is available in some instance.  This will
//               wake up a sleeping wait_request() call, if any.
////////////////////////////////////////////////////////////////////
void P3DInstanceManager::
signal_request_ready() {
#ifdef _WIN32
  ++_request_seq;
  SetEvent(_request_ready);
#else
  ACQUIRE_LOCK(_request_ready_lock);
  ++_request_seq;
  pthread_cond_signal(&_request_ready_cvar);
  RELEASE_LOCK(_request_ready_lock);
#endif
}

////////////////////////////////////////////////////////////////////
//     Function: P3DInstanceManager::get_global_ptr
//       Access: Public, Static
//  Description: 
////////////////////////////////////////////////////////////////////
P3DInstanceManager *P3DInstanceManager::
get_global_ptr() {
  if (_global_ptr == NULL) {
    _global_ptr = new P3DInstanceManager;
  }
  return _global_ptr;
}
