/* Filename: contribsymbols.h
 * Created by:  rdb (30Dec09)
 *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *
 * PANDA 3D SOFTWARE
 * Copyright (c) Carnegie Mellon University.  All rights reserved.
 *
 * All use of this software is subject to the terms of the revised BSD
 * license.  You should have received a copy of this license along
 * with this source code in a file named "LICENSE."
 *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

#ifndef CONTRIBSYMBOLS_H
#define CONTRIBSYMBOLS_H

/* See dtoolsymbols.h for a rant on the purpose of this file.  */

/* Note that the symbols declared in this file appear in alphabetical
   order.  Also note that we must use C-style comments only here, not
   C++-style comments, since this file is occasionally included by a C
   file. */

#ifdef BUILDING_PANDAAI
  #define EXPCL_PANDAAI EXPORT_CLASS
  #define EXPTP_PANDAAI EXPORT_TEMPLATE
#else
  #define EXPCL_PANDAAI IMPORT_CLASS
  #define EXPTP_PANDAAI IMPORT_TEMPLATE
#endif

#endif
