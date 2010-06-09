/*
 * GccKdmUtilities.cc
 *
 *  Created on: Jun 7, 2010
 *      Author: kgirard
 */

#include "gcckdm/GccKdmConfig.hh"
#include "gcckdm/GccKdmUtilities.hh"


//This is to indicated that the global namespace is not linked in
tree global_namespace = NULL;

extern "C" void
lang_check_failed (const char* file, int line, const char* function)
{
  internal_error ("lang_* check: failed in %s, at %s:%d",
                  function, trim_filename (file), line);
}
