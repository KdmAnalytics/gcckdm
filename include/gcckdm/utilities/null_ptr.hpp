/*
 * NullPtr.hh
 *
 *  Created on: Jun 21, 2010
 *      Author: kgirard
 */

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
