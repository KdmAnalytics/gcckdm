/*
 * GccKdmUtilities.hh
 *
 *  Created on: Jun 7, 2010
 *      Author: kgirard
 */

#ifndef GCCKDM_GCCKDMUTILITIES_HH_
#define GCCKDM_GCCKDMUTILITIES_HH_

namespace gcckdm
{




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
 * Return the name of the given node as a string, if the node
 * doesn't have a name returns an empty string
 *
 * @param node the name to use to retrieve the name
 * @return the name of the node or an empty string
 */
std::string const treeNodeNameString(tree node);

/**
 * Return the name of the given declaration as a string.  If the declaration
 * node doesn't have a name, returns an empty string
 *
 * @param decl and AST declaration node
 * @return  the name of the declaration or the empty string if the declration doesn't have a name
 */
std::string const declNameString(tree decl);

/**
 * Return the name of the given type as a string.  If the type node does not have a name
 * return an empty string
 *
 * @param type a type AST Node
 * @return the name of the given type node or an empty string
 */
std::string const typeNameString(tree type);


}  // namespace gcckdm

#endif /* GCCKDM_GCCKDMUTILITIES_HH_ */
