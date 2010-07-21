//
// Copyright (c) 2010 KDM Analytics, Inc. All rights reserved.
// Date: Jun 21, 2010
// Author: Kyle Girard <kyle@kdmanalytics.com>
//
// This file is part of libGccKdm.
//
// Foobar is free software: you can redistribute it and/or modify
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

#ifndef GCCKDM_UTILITIES_NULLPTR_HH_
#define GCCKDM_UTILITIES_NULLPTR_HH_

#include <typeinfo>

const// It is a const object...
class nullptr_t
{
public:
  template<class T>
  operator T*() const // convertible to any type of null non-member pointer...
  {
    return 0;
  }

  template<class C, class T>
  operator T C::*() const // or any type of null member pointer...
  {
    return 0;
  }

private:
  void operator&() const; // Can't take address of nullptr

} nullptr =
{ };

#endif /* GCCKDM_UTILITIES_NULLPTR_HH_ */
