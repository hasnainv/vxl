/*
  fsm@robots.ox.ac.uk
*/
#ifdef __GNUC__
#pragma implementation
#endif
#include "vgui_tag.h"
#include <vcl_cassert.h>

#define MAXTAGS 255

// POD
static unsigned numtags = 0;
static vgui_tag_function tags[MAXTAGS+1];

int vgui_tag_add(vgui_tag_function f, char const * /*tk*/) {
  if (f) {
    assert(numtags < MAXTAGS);
    tags[numtags++] = f;
  }
  return numtags;
}

vgui_tag_function const *vgui_tag_list() {
  tags[numtags] = 0;
  return tags;
}

void vgui_tag_call() {
  for (unsigned i=0; i<numtags; ++i)
    (*tags[i])();
  numtags = 0;
}
