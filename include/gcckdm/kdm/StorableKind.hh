/*
 * StorableKind.hh
 *
 *  Created on: 2011-12-06
 *      Author: kgirard
 */

#ifndef GCCKDM_KDM_STORABLEKIND_HH_
#define GCCKDM_KDM_STORABLEKIND_HH_

#include <string>
#include <gcckdm/kdm/IKind.hh>

namespace gcckdm
{

namespace kdm
{

/**
 * Typesafe enum of the KDM StorableKind enumeration datatype
 */
class StorableKind : public IKind
{
private:
  enum
  {
	  StorableKind_Global,
	  StorableKind_Local,
	  StorableKind_Static,
	  StorableKind_External,
	  StorableKind_Register,
	  StorableKind_Unknown,
  };

public:

  ///Returns a StorableKind object that represents the "global" literal
  static const StorableKind Global()               { return StorableKind(StorableKind_Global, "global"); }
  ///Returns a StorableKind object that represents the "local" literal
  static const StorableKind Local()                { return StorableKind(StorableKind_Local, "local"); }
  ///Returns a StorableKind object that represents the "static" literal
  static const StorableKind Static()               { return StorableKind(StorableKind_Static, "static"); }
  ///Returns a StorableKind object that represents the "external" literal
  static const StorableKind External()              { return StorableKind(StorableKind_External, "external"); }
  ///Returns a StorableKind object that represents the "register" literal
  static const StorableKind Register()              { return StorableKind(StorableKind_Register, "register"); }
  ///Returns a StorableKind object that represents the "unknown" literal
  static const StorableKind Unknown()              { return StorableKind(StorableKind_Unknown, "unknown"); }

  /**
   * Return the id of this StorableKind object as an int
   */
  int const id() const
  {
	return mId;
  }

  /**
   * Conversion constructor to convert this StorableKind object to an int
   */
  operator int() const
  {
	return mId;
  }

  /**
   * Return the literal that this StorableKind Object represents
   */
  virtual std::string const & name() const
  {
	return mName;
  }

private:

  /**
   * Private constructor to prevent user construction
   */
  StorableKind(int const & id, std::string name) :
	mId(id), mName(name)
  {
  }

  /// a value from the enum of all parameter kind possible values
  int mId;
  /// a string representing the literal of this StorableKind datatype
  std::string mName;


};
} // kdm

} // gcckdm


#endif /* GCCKDM_KDM_STORABLEKIND_HH_ */
