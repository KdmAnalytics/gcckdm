//
// Copyright (c) 2010 KDM Analytics, Inc. All rights reserved.
// Date: Jul 13, 2010
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

#ifndef GCCKDM_KDMTRIPLEWRITER_TRIPLEWRITER_HH_
#define GCCKDM_KDMTRIPLEWRITER_TRIPLEWRITER_HH_

#include <string>
#include <gcckdm/KdmPredicate.hh>
#include <gcckdm/KdmType.hh>

namespace gcckdm
{

namespace kdmtriplewriter
{

/**
 * Interface for all triple writers
 */
class TripleWriter
{
public:

  /**
   * Empty Virtual Destructor to allow proper destruction
   */
  virtual ~TripleWriter()
  {
  }
  ;

  /**
   * Writes the given three values as a triple
   *
   * @param subject - the subject id
   * @param predicate - the KDM predicate
   * @param object - the object id
   */
  virtual void writeTriple(long const subject, KdmPredicate const & predicate, long const object) = 0;

  /**
   * Writes the given three values as a triple
   *
   * @param subject - the subject id
   * @param predicate - the KDM predicate
   * @param object - a KdmType as defined by the KDM specification ie action/ActionElement
   */
  virtual void writeTriple(long const subject, KdmPredicate const & predicate, KdmType const & object) = 0;

  /**
   * Writes the given three values as a triple
   *
   * @param subject - the subject id
   * @param predicate - the KDM predicate
   * @param object - a free form string (literal)
   */
  virtual void writeTriple(long const subject, KdmPredicate const & predicate, std::string const & object) = 0;

private:
};

} // namespace kdmtriplewriter

} // namespace gcckdm
#endif /* GCCKDM_KDMTRIPLEWRITER_TRIPLEWRITER_HH_ */
