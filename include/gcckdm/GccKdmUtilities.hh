//
// Copyright (c) 2010 KDM Analytics, Inc. All rights reserved.
// Date: Jun 7, 2010
// Author: Kyle Girard <kyle@kdmanalytics.com>
//
// This file is part of libGccKdm.
//
// libGccKdm is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.

// libGccKdm is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

// You should have received a copy of the GNU General Public License
// along with libGccKdm.  If not, see <http://www.gnu.org/licenses/>.
//

#ifndef GCCKDM_GCCKDMUTILITIES_HH_
#define GCCKDM_GCCKDMUTILITIES_HH_

#include <string>

namespace gcckdm
{

/**
 * Return the name of the given node
 */
std::string getAstNodeName(tree node);

/**
 * Returns true if the location <code>loc</code> is a known location
 *
 * @param loc the location to test
 * @return true if the location is known
 */
bool locationIsUnknown(location_t loc);

/**
 * Returns the location_t for the given AST tree node
 *
 * @param t the AST node to use to retrieve the location
 */
location_t locationOf(tree t);

/**
 * Returns a string representation of the location <code>loc</code>
 *
 * With the following format: <filename>:<line>:<column>
 *
 * @param the location of an AST node
 * @return the string representation of the file, line and column for the given location
 */
std::string const locationString(location_t loc);


/**
 * Returns true if the front end being used is C++
 */
bool isFrontendCxx();

/**
 * Returns true if the front end being used is C
 */
bool isFrontendC();

/**
 * Returns the type qualifiers for this type, including the qualifiers on the
 * elements for an array type.
 *
 * FIXME: Was cp_type_quals in typeck.c -- where is it now?
 */
int getTypeQualifiers(tree const type);

/**
 * Returns the link:id for a specified node. Is passed in a fallback ID if we have no
 * special code to build an ID
 */
std::string getLinkId(tree const typeName, std::string const name);

} // namespace gcckdm

#endif /* GCCKDM_GCCKDMUTILITIES_HH_ */
