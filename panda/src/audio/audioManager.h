// Filename: audio_manager.h
// Created by:  skyler (June 6, 2001)
// Prior system by: cary
//
////////////////////////////////////////////////////////////////////
//
// PANDA 3D SOFTWARE
// Copyright (c) 2001, Disney Enterprises, Inc.  All rights reserved
//
// All use of this software is subject to the terms of the Panda 3d
// Software license.  You should have received a copy of this license
// along with this source code; you will also find a current copy of
// the license at http://www.panda3d.org/license.txt .
//
// To contact the maintainers of this program write to
// panda3d@yahoogroups.com .
//
////////////////////////////////////////////////////////////////////

#ifndef __AUDIO_MANAGER_H__
#define __AUDIO_MANAGER_H__

#include "config_audio.h"
#include "audioSound.h"

class AudioManagerInternal;

class EXPCL_PANDA AudioManager {
PUBLISHED:
  // Create an AudioManager for each category of sounds you have.
  // E.g.
  //   MySoundEffects = create_AudioManager::AudioManager();
  //   MyMusicManager = create_AudioManager::AudioManager();
  //   ...
  //   my_sound = MySoundEffects.get_sound("neatSfx.mp3");
  //   my_music = MyMusicManager.get_sound("introTheme.mid");
  //
  // You own the AudioManager*, please delete it when you're done with it.
  // Do not delete the AudioManager (i.e. you're not done with it), until
  // you have deleted all the associated AudioSounds.
  static AudioManager* create_AudioManager();
  virtual ~AudioManager() {}
  
  // Get a sound:
  // You own this sound.  Be sure to delete it when you're done.
  virtual AudioSound* get_sound(const string& file_name) = 0;
  // Tell the AudioManager there is no need to keep this one cached.
  virtual void drop_sound(const string& file_name) = 0;

  // Control volume:
  // FYI:
  //   If you start a sound with the volume off and turn the volume 
  //   up later, you'll hear the sound playing at that late point.
  virtual void set_volume(float volume) = 0;
  virtual float get_volume() = 0;
  
  // Turn the manager on an off.
  // If you play a sound while the manager is inactive, it won't start.
  // If you deactivate the manager while sounds are playing, they'll
  // stop.
  // If you activate the manager while looping sounds are playing
  // (those that have a loop_count of zero),
  // they will start playing from the begining of their loop.
  virtual void set_active(bool flag) = 0;
  virtual bool get_active() = 0;

protected:
  AudioManager() {
    // intentionally blank.
  }
};

#include "audioManager.I"

#endif /* __AUDIO_MANAGER_H__ */
