// Filename: audioLoadRequest.h
// Created by:  drose (29Aug06)
//
////////////////////////////////////////////////////////////////////
//
// PANDA 3D SOFTWARE
// Copyright (c) 2001 - 2004, Disney Enterprises, Inc.  All rights reserved
//
// All use of this software is subject to the terms of the Panda 3d
// Software license.  You should have received a copy of this license
// along with this source code; you will also find a current copy of
// the license at http://etc.cmu.edu/panda3d/docs/license/ .
//
// To contact the maintainers of this program write to
// panda3d-general@lists.sourceforge.net .
//
////////////////////////////////////////////////////////////////////

#ifndef AUDIOLOADREQUEST_H
#define AUDIOLOADREQUEST_H

#include "pandabase.h"

#include "asyncTask.h"
#include "audioManager.h"
#include "audioSound.h"
#include "pointerTo.h"

////////////////////////////////////////////////////////////////////
//       Class : AudioLoadRequest
// Description : A class object that manages a single asynchronous
//               audio load request.  This works in conjunction with
//               the Loader class defined in pgraph, or really with
//               any AsyncTaskManager.  Create a new AudioLoadRequest,
//               and add it to the loader via load_async(), to begin
//               an asynchronous load.
////////////////////////////////////////////////////////////////////
class EXPCL_PANDA_AUDIO AudioLoadRequest : public AsyncTask {
PUBLISHED:
  INLINE AudioLoadRequest(AudioManager *audio_manager, const string &filename, 
                          bool positional);

  INLINE AudioManager *get_audio_manager() const;
  INLINE const string &get_filename() const;
  INLINE bool get_positional() const;

  INLINE bool is_ready() const;
  INLINE AudioSound *get_sound() const;

protected:
  virtual bool do_task();

private:
  PT(AudioManager) _audio_manager;
  string _filename;
  bool _positional;

  bool _is_ready;
  PT(AudioSound) _sound;
  
public:
  static TypeHandle get_class_type() {
    return _type_handle;
  }
  static void init_type() {
    AsyncTask::init_type();
    register_type(_type_handle, "AudioLoadRequest",
                  AsyncTask::get_class_type());
    }
  virtual TypeHandle get_type() const {
    return get_class_type();
  }
  virtual TypeHandle force_init_type() {init_type(); return get_class_type();}
  
private:
  static TypeHandle _type_handle;
};

#include "audioLoadRequest.I"

#endif

