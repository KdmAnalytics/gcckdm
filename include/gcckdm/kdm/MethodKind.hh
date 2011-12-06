/*
 * MethodKind.hh
 *
 *  Created on: 2011-12-06
 *      Author: kgirard
 */

#ifndef GCCKDM_KDM_METHODKIND_HH_
#define GCCKDM_KDM_METHODKIND_HH_

#include <string>
#include <gcckdm/kdm/IKind.hh>

namespace gcckdm
{

namespace kdm
{

/**
 * Typesafe enum of the KDM MethodKind enumeration datatype
 */
class MethodKind : public IKind
{
private:
  enum
  {
      MethodKind_Method,
      MethodKind_Constructor,
	  MethodKind_Destructor,
	  MethodKind_Operator,
	  MethodKind_Virtual,
	  MethodKind_Abstract,
	  MethodKind_Unknown,
  };

public:

  ///Returns a MethodKind object that represents the "method" literal
  static const MethodKind Method()               { return MethodKind(MethodKind_Method, "method"); }
  ///Returns a MethodKind object that represents the "constructor" literal
  static const MethodKind Constructor()          { return MethodKind(MethodKind_Constructor, "constructor"); }
  ///Returns a MethodKind object that represents the "destructor" literal
  static const MethodKind Destructor()           { return MethodKind(MethodKind_Destructor, "destructor"); }
  ///Returns a MethodKind object that represents the "operator" literal
  static const MethodKind Operator()             { return MethodKind(MethodKind_Operator, "operator"); }
  ///Returns a MethodKind object that represents the "operator" literal
  static const MethodKind Virtual()              { return MethodKind(MethodKind_Virtual, "virtual"); }
  ///Returns a MethodKind object that represents the "abstract" literal
  static const MethodKind Abstract()             { return MethodKind(MethodKind_Abstract, "abstract"); }
  ///Returns a MethodKind object that represents the "abstract" literal
  static const MethodKind Unknown()              { return MethodKind(MethodKind_Unknown, "unknown"); }

  /**
   * Return the id of this MethodKind object as an int
   */
  int const id() const
  {
	return mId;
  }

  /**
   * Conversion constructor to convert this MethodKind object to an int
   */
  operator int() const
  {
	return mId;
  }

  /**
   * Return the literal that this MethodKind Object represents
   */
  virtual std::string const & name() const
  {
	return mName;
  }

private:

  /**
   * Private constructor to prevent user construction
   */
  MethodKind(int const & id, std::string name) :
	mId(id), mName(name)
  {
  }

  /// a value from the enum of all parameter kind possible values
  int mId;
  /// a string representing the literal of this MethodKind datatype
  std::string mName;


};
} // kdm

} // gcckdm


#endif /* GCCKDM_KDM_METHODKIND_HH_ */
