// Filename: depthWriteTransition.h
// Created by:  drose (31Mar00)
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

#ifndef DEPTHWRITETRANSITION_H
#define DEPTHWRITETRANSITION_H

#include <pandabase.h>

#include <onOffTransition.h>

////////////////////////////////////////////////////////////////////
//       Class : DepthWriteTransition
// Description : This enables or disables the writing to the depth
//               buffer.
////////////////////////////////////////////////////////////////////
class EXPCL_PANDA DepthWriteTransition : public OnOffTransition {
PUBLISHED:
  INLINE DepthWriteTransition();
  INLINE static DepthWriteTransition off();

public:
  virtual NodeTransition *make_copy() const;
  virtual NodeAttribute *make_attrib() const;
  virtual NodeTransition *make_initial() const;

  virtual void issue(GraphicsStateGuardianBase *gsgbase);

public:
  static void register_with_read_factory();
  virtual void write_datagram(BamWriter* manager, Datagram &me);

  static TypedWritable *make_DepthWriteTransition(const FactoryParams &params);

protected:
  void fillin(DatagramIterator& scan, BamReader* manager);

public:
  virtual TypeHandle get_type() const {
    return get_class_type();
  }
  virtual TypeHandle force_init_type() {init_type(); return get_class_type();}
  static TypeHandle get_class_type() {
    return _type_handle;
  }
  static void init_type() {
    OnOffTransition::init_type();
    register_type(_type_handle, "DepthWriteTransition",
                  OnOffTransition::get_class_type());
  }

private:
  static TypeHandle _type_handle;
  friend class DepthWriteAttribute;
};

#include "depthWriteTransition.I"

#endif
