/*
 * IKind.hh
 *
 *  Created on: 2011-11-04
 *      Author: kgirard
 */

#ifndef GCCKDM_KDM_IKIND_HH_
#define GCCKDM_KDM_IKIND_HH_

#include <string>

namespace gcckdm
{
namespace kdm
{

class IKind
{
public:
	virtual std::string const & name() const = 0;
};

} //namespac kdm
} //namespace gcckdm

#endif /* GCCKDM_KDM_IKIND_HH_ */
