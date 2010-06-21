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

// need to implement this for C compiling
void
lang_check_failed (const char* file, int line, const char* function)
{
  internal_error ("lang_* check: failed in %s, at %s:%d",
                  function, trim_filename (file), line);
}

// need to implement this for C compiling
tree *
decl_cloned_function_p (const_tree decl, bool just_testing) {
    return 0;
}
