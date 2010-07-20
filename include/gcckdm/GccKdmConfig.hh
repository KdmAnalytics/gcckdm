///////////////////////////////////////////////////////
//
// File: GccKdmPlugin.hh
// Created On: Jun 3, 2010
// Author: Kyle Girard <kyle@kdmanalytics.com>
//
///////////////////////////////////////////////////////

#ifndef GCCKDM_GCCKDMCONFIG_HH_
#define GCCKDM_GCCKDMCONFIG_HH_

#include <cstdlib> // Include before GCC poison some declarations.
#include <gmp.h>

// The order of the gcc plugin includes is important and doesn't
// seem to follow any sort of pattern.  The order below
// was determined by gcc plugin examples and trial and error

extern "C"
{
#include <gcc-plugin.h> //supposed to be first
#include <coretypes.h>
#include <tree.h>
#include <tree-pass.h>
#include <intl.h>
#include <tm.h>
#include <langhooks.h>
#include <plugin-version.h>
#include <diagnostic.h>
#include <c-common.h>
#include <c-pragma.h>
#include <cgraph.h>
#include <cp/cp-tree.h>
#include <tree-iterator.h>
#include <tree-flow.h>
#include <gimple.h>
#include <real.h>
}

#endif /* GCCKDM_GCCKDMCONFIG_HH_ */
