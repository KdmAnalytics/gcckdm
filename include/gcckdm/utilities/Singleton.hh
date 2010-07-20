/*
 * Singleton.hh
 *
 *  Created on: Jun 21, 2010
 *      Author: kgirard
 */

#ifndef GCCKDM_SINGLETON_HH_
#define GCCKDM_SINGLETON_HH_

#include <cassert>

namespace gcckdm
{

/**
 * Simple Singleton Template class
 */
template<typename T> class Singleton
{
public:
  Singleton()
  {
    assert(!mSingleton);
    mSingleton = static_cast<T*> (this);
  }

  ~Singleton()
  {
    assert(mSingleton);
    mSingleton = 0;
  }

  static T& Instance()
  {
    assert(mSingleton);
    return *mSingleton;
  }

  static T* InstancePtr()
  {
    return mSingleton;
  }
private:
  static T* mSingleton;
};

template<typename T> T* Singleton<T>::mSingleton = 0;

} // namespace gcckdm

#endif /* GCCKDM_SINGLETON_HH_ */
