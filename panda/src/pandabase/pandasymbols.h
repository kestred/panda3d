/* Filename: pandasymbols.h
 * Created by:  drose (18Feb00)
 *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *
 * PANDA 3D SOFTWARE
 * Copyright (c) 2001, Disney Enterprises, Inc.  All rights reserved
 *
 * All use of this software is subject to the terms of the Panda 3d
 * Software license.  You should have received a copy of this license
 * along with this source code; you will also find a current copy of
 * the license at http://www.panda3d.org/license.txt .
 *
 * To contact the maintainers of this program write to
 * panda3d@yahoogroups.com .
 *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

#ifndef PANDASYMBOLS_H
#define PANDASYMBOLS_H

/* See dtoolsymbols.h for a rant on the purpose of this file.  */

#if defined(WIN32_VC) && !defined(CPPPARSER) && !defined(LINK_ALL_STATIC)

#ifdef BUILDING_PANDA
  #define EXPCL_PANDA __declspec(dllexport)
  #define EXPTP_PANDA
#else
  #define EXPCL_PANDA __declspec(dllimport)
  #define EXPTP_PANDA extern
#endif

#ifdef BUILDING_PANDAEXPRESS
  #define EXPCL_PANDAEXPRESS __declspec(dllexport)
  #define EXPTP_PANDAEXPRESS
#else
  #define EXPCL_PANDAEXPRESS __declspec(dllimport)
  #define EXPTP_PANDAEXPRESS extern
#endif

#ifdef BUILDING_PANDAEGG
  #define EXPCL_PANDAEGG __declspec(dllexport)
  #define EXPTP_PANDAEGG
#else
  #define EXPCL_PANDAEGG __declspec(dllimport)
  #define EXPTP_PANDAEGG extern
#endif

#ifdef BUILDING_PANDAPHYSICS
  #define EXPCL_PANDAPHYSICS __declspec(dllexport)
  #define EXPTP_PANDAPHYSICS
#else
  #define EXPCL_PANDAPHYSICS __declspec(dllimport)
  #define EXPTP_PANDAPHYSICS extern
#endif

#ifdef BUILDING_PANDAGL
  #define EXPCL_PANDAGL __declspec(dllexport)
  #define EXPTP_PANDAGL
#else
  #define EXPCL_PANDAGL __declspec(dllimport)
  #define EXPTP_PANDAGL extern
#endif

#ifdef BUILDING_PANDAGLUT
  #define EXPCL_PANDAGLUT __declspec(dllexport)
  #define EXPTP_PANDAGLUT
#else
  #define EXPCL_PANDAGLUT __declspec(dllimport)
  #define EXPTP_PANDAGLUT extern
#endif

#ifdef BUILDING_PANDADX
  #define EXPCL_PANDADX __declspec(dllexport)
  #define EXPTP_PANDADX
#else
  #define EXPCL_PANDADX __declspec(dllimport)
  #define EXPTP_PANDADX extern
#endif

#ifdef BUILDING_PANDARIB
  #define EXPCL_PANDARIB __declspec(dllexport)
  #define EXPTP_PANDARIB
#else
  #define EXPCL_PANDARIB __declspec(dllimport)
  #define EXPTP_PANDARIB extern
#endif

#ifdef BUILDING_SHADER
  #define EXPCL_SHADER __declspec(dllexport)
  #define EXPTP_SHADER
#else
  #define EXPCL_SHADER __declspec(dllimport)
  #define EXPTP_SHADER extern
#endif

#ifdef BUILDING_MILES_AUDIO
  #define EXPCL_MILES_AUDIO __declspec(dllexport)
  #define EXPTP_MILES_AUDIO
#else
  #define EXPCL_MILES_AUDIO __declspec(dllimport)
  #define EXPTP_MILES_AUDIO extern
#endif

#ifdef BUILDING_LINUX_AUDIO
  #define EXPCL_LINUX_AUDIO __declspec(dllexport)
  #define EXPTP_LINUX_AUDIO
#else
  #define EXPCL_LINUX_AUDIO __declspec(dllimport)
  #define EXPTP_LINUX_AUDIO extern
#endif

#else   /* !WIN32_VC */

#define EXPCL_PANDA
#define EXPTP_PANDA

#define EXPCL_PANDAEXPRESS
#define EXPTP_PANDAEXPRESS

#define EXPCL_PANDAEGG
#define EXPTP_PANDAEGG

#define EXPCL_PANDAPHYSICS
#define EXPTP_PANDAPHYSICS

#define EXPCL_PANDAGL
#define EXPTP_PANDAGL

#define EXPCL_PANDAGLUT
#define EXPTP_PANDAGLUT

#define EXPCL_PANDADX
#define EXPTP_PANDADX

#define EXPCL_PANDARIB
#define EXPTP_PANDARIB

#define EXPCL_SHADER
#define EXPTP_SHADER

#define EXPCL_MILES_AUDIO
#define EXPTP_MILES_AUDIO

#define EXPCL_LINUX_AUDIO
#define EXPTP_LINUX_AUDIO

#define EXPCL_FRAMEWORK
#define EXPTP_FRAMEWORK

#endif  /* WIN32_VC */

#if defined(WIN32_VC) && !defined(CPPPARSER)
#define INLINE_LINMATH __forceinline
#define INLINE_MATHUTIL __forceinline

#ifdef BUILDING_PANDA
#define INLINE_GRAPH __forceinline
#define INLINE_DISPLAY __forceinline
#else
#define INLINE_GRAPH
#define DONT_INLINE_GRAPH
#define INLINE_DISPLAY
#define DONT_INLINE_DISPLAY
#endif

#else
#define INLINE_LINMATH INLINE
#define INLINE_MATHUTIL INLINE
#define INLINE_GRAPH INLINE
#define INLINE_DISPLAY INLINE
#endif

#define INLINE_CHAR INLINE
#define INLINE_CHAT INLINE
#define INLINE_CHAN INLINE
#define INLINE_CHANCFG INLINE
#define INLINE_COLLIDE INLINE
#define INLINE_CULL INLINE
#define INLINE_DEVICE INLINE
#define INLINE_DGRAPH INLINE
#define INLINE_GOBJ INLINE
#define INLINE_GRUTIL INLINE
#define INLINE_GSGBASE INLINE
#define INLINE_GSGMISC INLINE
#define INLINE_LIGHT INLINE
#define INLINE_PARAMETRICS INLINE
#define INLINE_SGRATTRIB INLINE
#define INLINE_SGMANIP INLINE
#define INLINE_SGRAPH INLINE
#define INLINE_SGRAPHUTIL INLINE
#define INLINE_SWITCHNODE INLINE
#define INLINE_TEXT INLINE
#define INLINE_TFORM INLINE
#define INLINE_LERP INLINE
#define INLINE_LOADER INLINE
#define INLINE_PUTIL INLINE
#define INLINE_EFFECTS INLINE
#define INLINE_GUI INLINE
#define INLINE_AUDIO INLINE

#endif
