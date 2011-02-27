//
// Copyright (c) 2010 KDM Analytics, Inc. All rights reserved.
// Date: Jun 3, 2010
// Author: Kyle Girard <kyle@kdmanalytics.com>
//
// This file is part of libGccKdm.
//
// libGccKdm is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// libGccKdm is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with libGccKdm.  If not, see <http://www.gnu.org/licenses/>.
//

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
#include <tree-dump.h>
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
