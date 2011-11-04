/*
 * KdmParameterKind.hh
 *
 *  Created on: 2011-11-04
 *      Author: kgirard
 */

#ifndef GCCKDM_PARAMETERKIND_HH_
#define GCCKDM_PARAMETERKIND_HH_

#include <string>
#include <gcckdm/IKdmKind.hh>

namespace gcckdm
{

/**
 * Typesafe enum of Parameter kind enumeration datatype
 */
class KdmParameterKind : public IKdmKind
{
private:
  enum
  {
	  KdmParameterKind_ByValue,
	  KdmParameterKind_ByName,
	  KdmParameterKind_ByReference,
	  KdmParameterKind_Variadic,
	  KdmParameterKind_Return,
	  KdmParameterKind_Throws,
	  KdmParameterKind_Exception,
	  KdmParameterKind_CatchAll,
	  KdmParameterKind_Unknown
  };

public:

  ///Returns a KdmParameterKind object that represents the "byValue" literal
  static const KdmParameterKind ByValue() { return KdmParameterKind(KdmParameterKind_ByValue, "byValue"); }
  ///Returns a KdmParameterKind object that represents the "byName" literal
  static const KdmParameterKind ByName() { return KdmParameterKind(KdmParameterKind_ByName, "byName" ); }
  ///Returns a KdmParameterKind object that represents the "byReference" literal
  static const KdmParameterKind ByReference() { return KdmParameterKind(KdmParameterKind_ByReference, "byReference");}
  ///Returns a KdmParameterKind object that represents the "variadic" literal
  static const KdmParameterKind Variadic() { return KdmParameterKind(KdmParameterKind_ByReference, "variadic");}
  ///Returns a KdmParameterKind object that represents the "return" literal
  static const KdmParameterKind Return() { return KdmParameterKind(KdmParameterKind_Return, "return");}
  ///Returns a KdmParameterKind object that represents the "throws" literal
  static const KdmParameterKind Throws() { return KdmParameterKind(KdmParameterKind_Throws, "throws");}
  ///Returns a KdmParameterKind object that represents the "exception" literal
  static const KdmParameterKind Exception() { return KdmParameterKind(KdmParameterKind_Exception, "exception");}
  ///Returns a KdmParameterKind object that represents the "catchall" literal
  static const KdmParameterKind CatchAll() { return KdmParameterKind(KdmParameterKind_CatchAll, "catchall");}
  ///Returns a KdmParameterKind object that represents the "unknown" literal
  static const KdmParameterKind Unknown() { return KdmParameterKind(KdmParameterKind_Unknown, "unknown");}


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
  KdmParameterKind(int const & id, std::string name) :
	mId(id), mName(name)
  {
  }

  /// a value from the enum of all parameter kind possible values
  int mId;
  /// a string representing the literal of this KdmParameterKind datatype
  std::string mName;

};

} // namespace gcckdm

#endif /* GCCKDM_PARAMETERKIND_HH_ */
