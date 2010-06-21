/*
 * GccKdmUtilities.hh
 *
 *  Created on: Jun 7, 2010
 *      Author: kgirard
 */

#ifndef GCCKDM_GCCKDMUTILITIES_HH_
#define GCCKDM_GCCKDMUTILITIES_HH_

/**
 * Functor that can be used to order decls according to their source location
 */
struct DeclComparator
{
    bool operator()(tree x, tree y) const
    {
        location_t xl(DECL_SOURCE_LOCATION (x));
        location_t yl(DECL_SOURCE_LOCATION (y));

        return xl < yl;
    }
};



#endif /* GCCKDM_GCCKDMUTILITIES_HH_ */
