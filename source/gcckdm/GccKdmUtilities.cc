/*
 * GccKdmUtilities.cc
 *
 *  Created on: Jun 7, 2010
 *      Author: kgirard
 */

#include "gcckdm/GccKdmConfig.hh"
#include "gcckdm/GccKdmUtilities.hh"

#include <sstream>

//This is to indicated that the global namespace is not linked in
tree global_namespace = NULL;

// need to implement this for C compiling
void lang_check_failed(const char* file, int line, const char* function)
{
    internal_error("lang_* check: failed in %s, at %s:%d", function, trim_filename(file), line);
}

// need to implement this for C compiling
tree *
decl_cloned_function_p(const_tree decl, bool just_testing)
{
    return 0;
}


namespace gcckdm
{

location_t locationOf(tree t)
{
    if (TREE_CODE (t) == PARM_DECL && DECL_CONTEXT (t))
        t = DECL_CONTEXT (t);
    else if (TYPE_P (t))
        t = TYPE_MAIN_DECL (t);
    else if (TREE_CODE (t) == OVERLOAD)
        t = OVL_FUNCTION (t);

    if (!t)
        return UNKNOWN_LOCATION;

    if (DECL_P(t))
        return DECL_SOURCE_LOCATION (t);
    else if (EXPR_P(t) && EXPR_HAS_LOCATION(t))
        return EXPR_LOCATION(t);
    else
        return UNKNOWN_LOCATION;
}


bool locationIsUnknown(location_t loc) {
  location_t unk = UNKNOWN_LOCATION;
  return !memcmp(&loc, &unk, sizeof(location_t));
}

std::string const locationString(location_t loc)
{
    if (locationIsUnknown(loc))
    {
        return "";
    }
    expanded_location eloc = expand_location(loc);
    std::ostringstream str;
    str << eloc.file << ":" << eloc.line << ":" << eloc.column;
    return str.str();
}

std::string const treeNodeNameString(tree node)
{
    return DECL_P(node) ? declNameString(node) : typeNameString(node);
}

std::string const declNameString(tree decl)
{
    tree aName = DECL_P(decl) ? DECL_NAME(decl) : decl;
    return aName ? IDENTIFIER_POINTER(aName) : "";
}

std::string const typeNameString(tree type)
{
    return TYPE_P(type) ? declNameString (TYPE_NAME(type)) : "";
}

}  // namespace gcckdm
