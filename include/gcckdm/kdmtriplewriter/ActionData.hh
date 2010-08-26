//
// Copyright (c) 2010 KDM Analytics, Inc. All rights reserved.
// Created on: Aug 25, 2010
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


#ifndef GCCKDM_KDMTRIPLEWRITER_ACTIONDATA_HH_
#define GCCKDM_KDMTRIPLEWRITER_ACTIONDATA_HH_


class ActionData
{
public:
  static const long InvalidId = -1;

  ActionData() : mActionId(InvalidId), mStartActionId(InvalidId), mOutputId(InvalidId){}

  ActionData(long actId) : mActionId(actId), mStartActionId(actId), mOutputId(InvalidId){}

  long actionId() const { return mActionId; }
  void actionId(long id) { mActionId = id; }

  /**
   * Returns the start action id if explicity set otherwise returns the actionid
   */
  long startActionId() const
  {
    return (mStartActionId == InvalidId) ? mActionId : mStartActionId;
  }

  void startActionId(long id ) { mStartActionId = id; }
  void startActionId(ActionData const & data)
  {
    if (data.hasActionId())
    {
      mStartActionId = data.mStartActionId;
    }
  }

  bool hasStartAction() const
  {
    return mStartActionId != InvalidId;
  }


  long outputId() const { return mOutputId; }
  void outputId(long id) { mOutputId = id; }

  bool hasActionId() const { return mActionId != InvalidId; }

  long getTargetId() const { return hasActionId() ? mActionId : mOutputId; }

private:
  long mActionId;
  long mStartActionId;
  long mOutputId;
};

#endif /* GCCKDM_KDMTRIPLEWRITER_ACTIONDATA_HH_ */
