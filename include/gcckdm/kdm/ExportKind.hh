/*
 * ExportKind.hh
 *
 *  Created on: 2011-12-06
 *      Author: kgirard
 */

#ifndef GCCKDM_KDM_EXPORTKIND_HH_
#define GCCKDM_KDM_EXPORTKIND_HH_

#include <string>
#include <gcckdm/kdm/IKind.hh>

namespace gcckdm
{

namespace kdm
{

/**
 * Typesafe enum of the KDM ExportKind enumeration datatype
 */
class ExportKind : public IKind
{
private:
  enum
  {
	  ExportKind_Public,
	  ExportKind_Private,
	  ExportKind_Protected,
	  ExportKind_Final,
	  ExportKind_Unknown,
  };

public:

  ///Returns a ExportKind object that represents the "private" literal
  static const ExportKind Public()              { return ExportKind(ExportKind_Public, "public"); }
  ///Returns a ExportKind object that represents the "private" literal
  static const ExportKind Private()              { return ExportKind(ExportKind_Private, "private"); }
  ///Returns a ExportKind object that represents the "protected" literal
  static const ExportKind Protected()              { return ExportKind(ExportKind_Protected, "protected"); }
  ///Returns a ExportKind object that represents the "final" literal
  static const ExportKind Unknown()              { return ExportKind(ExportKind_Final, "final"); }
  ///Returns a ExportKind object that represents the "abstract" literal
  static const ExportKind Unknown()              { return ExportKind(ExportKind_Unknown, "abstract"); }

  /**
   * Return the id of this ExportKind object as an int
   */
  int const id() const
  {
	return mId;
  }

  /**
   * Conversion constructor to convert this ExportKind object to an int
   */
  operator int() const
  {
	return mId;
  }

  /**
   * Return the literal that this ExportKind Object represents
   */
  virtual std::string const & name() const
  {
	return mName;
  }

private:

  /**
   * Private constructor to prevent user construction
   */
  ExportKind(int const & id, std::string name) :
	mId(id), mName(name)
  {
  }

  /// a value from the enum of all parameter kind possible values
  int mId;
  /// a string representing the literal of this ExportKind datatype
  std::string mName;


};
} // kdm

} // gcckdm


#endif /* GCCKDM_KDM_EXPORTKIND_HH_ */
