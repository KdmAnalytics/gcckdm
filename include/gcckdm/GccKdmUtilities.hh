/*
 * GccKdmUtilities.hh
 *
 *  Created on: Jun 7, 2010
 *      Author: kgirard
 */

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

} // namespace gcckdm

#endif /* GCCKDM_GCCKDMUTILITIES_HH_ */
