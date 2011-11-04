/*
 * IKind.hh
 *
 *  Created on: 2011-11-04
 *      Author: kgirard
 */

#ifndef GCCKDM_IKIND_HH_
#define GCCKDM_IKIND_HH_

#include <string>

namespace gcckdm
{

class IKdmKind
{
public:
	virtual std::string const & name() const = 0;
};

} //namespace gcckdm

#endif /* GCCKDM_IKIND_HH_ */
