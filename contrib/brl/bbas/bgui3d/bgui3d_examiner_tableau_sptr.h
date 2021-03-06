// This is brl/bbas/bgui3d/bgui3d_examiner_tableau_sptr.h
#ifndef bgui3d_examiner_tableau_sptr_h_
#define bgui3d_examiner_tableau_sptr_h_
//:
// \file
// \brief  Smart-pointer to a bgui3d_examiner_tableau.

#include <vgui/vgui_tableau_sptr.h>
#include "bgui3d_fullviewer_tableau_sptr.h"

class bgui3d_examiner_tableau;
typedef vgui_tableau_sptr_t<bgui3d_examiner_tableau,
                            bgui3d_fullviewer_tableau_sptr> bgui3d_examiner_tableau_sptr;

#endif // bgui3d_examiner_tableau_sptr_h_
