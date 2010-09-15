//
// Copyright (c) 2010 KDM Analytics, Inc. All rights reserved.
// Created on: Jul 22, 2010
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

#ifndef GCCKDM_KDMTRIPLEWRITER_EXCEPTION_HH_
#define GCCKDM_KDMTRIPLEWRITER_EXCEPTION_HH_

#include <exception>
#include <boost/exception/all.hpp>

namespace gcckdm {

namespace kdmtriplewriter {

/**
 * Generic Base KdmTripleWriter Exception
 */
class KdmTripleWriterException : public boost::exception, public std::exception
{
public:
  KdmTripleWriterException() : mWhat("KdmTripleWriter has thrown an exception") {}
  virtual ~KdmTripleWriterException() throw() {};
  virtual char const * what() const throw() { return mWhat.c_str(); }

private:
  std::string mWhat;
};

/**
 * Throw this when a null location in encountered and you cannot
 * use an invalid return value
 */
class NullLocationException : public KdmTripleWriterException
{
public:
  NullLocationException() : mWhat("Location was null") {}
  virtual ~NullLocationException() throw() {};
  virtual char const * what() const throw() { return mWhat.c_str(); }
private:
  std::string mWhat;
};

/**
 * Throw this when a null tree node was encountered and you cannot
 * use an invalid return value
 */
class NullAstNodeException : public KdmTripleWriterException
{
public:
  NullAstNodeException() : mWhat("AST Node was null") {}
  virtual ~NullAstNodeException() throw() {};
  virtual char const * what() const throw()  {    return mWhat.c_str();  }
private:
  std::string mWhat;
};


/**
 * Throw this when a function is passed an invalid parameter
 */
class InvalidParameterException : public KdmTripleWriterException
{
public:
  InvalidParameterException(std::string const & msg) : mWhat(msg) {}
  virtual ~InvalidParameterException() throw() {};
  virtual char const * what() const throw()  {    return mWhat.c_str();  }
private:
  std::string mWhat;
};

/**
 * Throw this unable to locate parent context
 */
class InvalidPathContextException : public KdmTripleWriterException
{
public:
  InvalidPathContextException(std::string const & msg) : mWhat(msg) {}
  virtual ~InvalidPathContextException() throw() {};
  virtual char const * what() const throw()  {    return mWhat.c_str();  }
private:
  std::string mWhat;
};



}  // namespace kdmtriplewriter

}  // namespace gcckdm


#endif /* GCCKDM_KDMTRIPLEWRITER_EXCEPTION_HH_ */
