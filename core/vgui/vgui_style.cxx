//-*- c++ -*-------------------------------------------------------------------
#ifdef __GNUC__
#pragma implementation
#endif
//
// Class: vgui_style
// Author: Philip C. Pritchett, RRG, University of Oxford
// Created: 18 Oct 99
//
//-----------------------------------------------------------------------------

#include "vgui_style.h"
#include <vcl_iostream.h>
#include <vbl/vbl_sprintf.h>


vgui_style::vgui_style() {
  for (int i=0; i<2; ++i)
    rgba[i] = 1;

  rgba[3] = 1;
  line_width = 1;
  point_size = 1;
}
