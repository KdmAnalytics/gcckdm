/*
 * KdmParameterKind.hh
 *
 *  Created on: 2011-11-04
 *      Author: kgirard
 */

#ifndef GCCKDM_PARAMETERKIND_HH_
#define GCCKDM_PARAMETERKIND_HH_

#include <string>
#include <gcckdm/kdm/IKind.hh>

namespace gcckdm
{

namespace kdm
{

/**
 * Typesafe enum of Parameter kind enumeration datatype
 */
class ParameterKind : public gcckdm::kdm::IKind
{
private:
  enum
  {
	  ParameterKind_ByValue,
	  ParameterKind_ByName,
	  ParameterKind_ByReference,
	  ParameterKind_Variadic,
	  ParameterKind_Return,
	  ParameterKind_Throws,
	  ParameterKind_Exception,
	  ParameterKind_CatchAll,
	  ParameterKind_Unknown
  };

public:

  ///Returns a KdmParameterKind object that represents the "byValue" literal
  static const ParameterKind ByValue() { return ParameterKind(ParameterKind_ByValue, "byValue"); }
  ///Returns a KdmParameterKind object that represents the "byName" literal
  static const ParameterKind ByName() { return ParameterKind(ParameterKind_ByName, "byName" ); }
  ///Returns a KdmParameterKind object that represents the "byReference" literal
  static const ParameterKind ByReference() { return ParameterKind(ParameterKind_ByReference, "byReference");}
  ///Returns a KdmParameterKind object that represents the "variadic" literal
  static const ParameterKind Variadic() { return ParameterKind(ParameterKind_ByReference, "variadic");}
  ///Returns a KdmParameterKind object that represents the "return" literal
  static const ParameterKind Return() { return ParameterKind(ParameterKind_Return, "return");}
  ///Returns a KdmParameterKind object that represents the "throws" literal
  static const ParameterKind Throws() { return ParameterKind(ParameterKind_Throws, "throws");}
  ///Returns a KdmParameterKind object that represents the "exception" literal
  static const ParameterKind Exception() { return ParameterKind(ParameterKind_Exception, "exception");}
  ///Returns a KdmParameterKind object that represents the "catchall" literal
  static const ParameterKind CatchAll() { return ParameterKind(ParameterKind_CatchAll, "catchall");}
  ///Returns a KdmParameterKind object that represents the "unknown" literal
  static const ParameterKind Unknown() { return ParameterKind(ParameterKind_Unknown, "unknown");}


  /**
   * Return the id of this KdmParameterKind object as an int
   */
  int const id() const
  {
	return mId;
  }

  /**
   * Conversion constructor to convert this KdmParameterKind object to an int
   */
  operator int() const
  {
	return mId;
  }

  /**
   * Return the literal that this KdmParameterKind Object represents
   */
  virtual std::string const & name() const
  {
	return mName;
  }

private:

  /**
   * Private constructor to precent user construction
   */
  ParameterKind(int const & id, std::string name) :
	mId(id), mName(name)
  {
  }

  /// a value from the enum of all parameter kind possible values
  int mId;
  /// a string representing the literal of this ParameterKind datatype
  std::string mName;

};

} // namespace kdm

} // namespace gcckdm

#endif /* GCCKDM_PARAMETERKIND_HH_ */
