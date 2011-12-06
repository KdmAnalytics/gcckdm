/*
 * CallableKind.hh
 *
 *  Created on: 2011-12-06
 *      Author: kgirard
 */

#ifndef GCCKDM_KDM_CALLABLEKIND_HH_
#define GCCKDM_KDM_CALLABLEKIND_HH_

#include <string>
#include <gcckdm/kdm/IKind.hh>

namespace gcckdm
{

namespace kdm
{

/**
 * Typesafe enum of the KDM CallableKind enumeration datatype
 */
class CallableKind : public IKind
{
private:
  enum
  {
	  CallableKind_External,
	  CallableKind_Regular,
	  CallableKind_Operator,
	  CallableKind_Unknown
  };

public:

  ///Returns a CallableKind object that represents the "external" literal
  static const CallableKind External()             { return CallableKind(CallableKind_External, "external"); }
  ///Returns a CallableKind object that represents the "regular" literal
  static const CallableKind Regular()              { return CallableKind(CallableKind_Regular, "regular");  }
  ///Returns a CallableKind object that represents the "operator" literal
  static const CallableKind Operator()             { return CallableKind(CallableKind_Operator, "operator"); }
  ///Returns a CallableKind object that represents the "unknown" literal
  static const CallableKind Unknown()              { return CallableKind(CallableKind_Unknown, "unknown"); }

  /**
   * Return the id of this CallableKind object as an int
   */
  int const id() const
  {
	return mId;
  }

  /**
   * Conversion constructor to convert this CallableKind object to an int
   */
  operator int() const
  {
	return mId;
  }

  /**
   * Return the literal that this CallableKind Object represents
   */
  virtual std::string const & name() const
  {
	return mName;
  }

private:

  /**
   * Private constructor to prevent user construction
   */
  CallableKind(int const & id, std::string name) :
	mId(id), mName(name)
  {
  }

  /// a value from the enum of all parameter kind possible values
  int mId;
  /// a string representing the literal of this CallableKind datatype
  std::string mName;


};
} // kdm

} // gcckdm


#endif /* GCCKDM_KDM_CALLABLEKIND_HH_ */
