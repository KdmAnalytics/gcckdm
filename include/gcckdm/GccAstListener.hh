//
// Copyright (c) 2010 KDM Analytics, Inc. All rights reserved.
// Date: Jun 21, 2010
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
//

#ifndef GCCKDM_GCCASTLISTENER_HH_
#define GCCKDM_GCCASTLISTENER_HH_

#include "gcckdm/GccKdmConfig.hh"
#include <boost/filesystem/path.hpp>

namespace gcckdm
{

class GccAstListener
{
public:
  typedef boost::filesystem::path Path;

  virtual ~GccAstListener(){};

  virtual void startTranslationUnit(Path const & filename) = 0;
  virtual void startKdmGimplePass() = 0;
  virtual void finishKdmGimplePass() = 0;
  virtual void processAstNode(tree const ast) = 0;
  virtual void finishTranslationUnit() = 0;
};

} // namespace gcckdm


#endif /* GCCKDM_GCCASTLISTENER_HH_ */
