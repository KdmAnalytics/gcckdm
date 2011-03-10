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
//
// libGccKdm is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with libGccKdm.  If not, see <http://www.gnu.org/licenses/>.
//

#ifndef GCCKDM_GCCKDMUTILITIES_HH_
#define GCCKDM_GCCKDMUTILITIES_HH_

#include <string>
#include <boost/range.hpp>
#include <boost/algorithm/string.hpp>

namespace gcckdm
{
  namespace constants
  {
    std::string getUnamedNodeString();
  }

////If a AST node doesn't have a name use this name
//extern std::string const unnamedNode;

/**
 * Return the name of the given node
 */
std::string getAstNodeName(tree node);

std::string nodeName(tree const node);

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
 *
 * @return
 */
bool isFrontendCxx();

/**
 * Returns true if the front end being used is C
 *
 * @return
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


/**
 * Iterates over the given range, and replaces chars that gcc places in it's strings
 * with escaped equivalents and places the result in the given outputString.
 *
 * Characters that are escaped:  \b, \f, \n, \v, \\, \", \', \1, \2, \3, \4, \5, \6, \7,
 *
 * @param c an array of characters or a std::string
 * @param outputString the result of escaping characters in the range 'c'
 */
template <typename Range>
void
replaceSpecialCharsCopy(Range const & c, std::string & outputString)
{
  typename boost::range_iterator< const Range>::type b = boost::begin(boost::as_literal(c));

  while (b != boost::end(boost::as_literal(c)))
  {
    switch(*b)
    {
        case '\b':
          outputString += "\\b";
          break;

        case '\f':
          outputString += "\\f";
          break;

        case '\n':
          outputString += "\\n";
          break;

        case '\r':
          outputString += "\\r";
          break;

        case '\t':
          outputString += "\\t";
          break;

        case '\v':
          outputString += "\\v";
          break;

        case '\\':
          outputString += "\\\\";
          break;

        case '\"':
          outputString += "\\\"";
          break;

        case '\'':
          outputString += "\\'";
          break;

        case '\1':
          outputString += "\\1";
          break;

        case '\2':
          outputString += "\\2";
          break;

        case '\3':
          outputString += "\\3";
          break;

        case '\4':
          outputString += "\\4";
          break;

        case '\5':
          outputString += "\\5";
          break;

        case '\6':
          outputString += "\\6";
          break;

        case '\7':
          outputString += "\\7";
          break;

        default:
          outputString += *b;
          break;
    }
    ++b;
  }
}



} // namespace gcckdm

#endif /* GCCKDM_GCCKDMUTILITIES_HH_ */
