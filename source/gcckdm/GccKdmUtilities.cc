//
// Copyright (c) 2010 KDM Analytics, Inc. All rights reserved.
// Date: Jun 7, 2010
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

#include "gcckdm/GccKdmConfig.hh"
#include "gcckdm/GccKdmUtilities.hh"

#include <iostream>
#include <sstream>
#include <demangle.h> // Required for c++ name demangling
#include <boost/lexical_cast.hpp>
#include <boost/current_function.hpp>
#include <boost/format.hpp>


//This is to indicated that the global namespace is not linked in
tree global_namespace = NULL;

// need to implement this for C compiling
void lang_check_failed(const char* file, int line, const char* function)
{
  internal_error("lang_* check: failed in %s, at %s:%d", function, trim_filename(file), line);
}

// need to implement this for C compiling
#ifndef __WIN32__
tree *
decl_cloned_function_p(const_tree decl, bool just_testing)
{
  return 0;
}
#endif

namespace
{

std::string getAstIdentifierNodeName(tree node)
{
  return IDENTIFIER_POINTER(node);
}

std::string getAstDeclarationNodeName(tree node)
{
  std::string nodeName;
  if (DECL_NAME(node))
  {
    nodeName = getAstIdentifierNodeName(DECL_NAME(node));
  }
  if (DECL_NAME(node) == NULL_TREE)
  {
    if (TREE_CODE(node) == LABEL_DECL && LABEL_DECL_UID(node) != -1)
    {
      nodeName = boost::lexical_cast<std::string>(LABEL_DECL_UID(node));
    }
    else
    {
      std::string prefix = TREE_CODE(node) == CONST_DECL ? "C" : "D";
      unsigned int uid = DECL_UID(node);
      std::string uidStr(boost::lexical_cast<std::string>(uid));
      nodeName = prefix + "." + uidStr;
    }
  }
  return nodeName;
}

std::string getAstFunctionDeclarationName(tree node)
{
  std::string declStr = "(";
  bool wroteFlag = false;

  tree arg;
  //Print the argument types.  The last element in the list is a VOID_TYPE.
  //The following avoids printing the last element.
  arg = TYPE_ARG_TYPES(node);
  while (arg && TREE_CHAIN(arg) && arg != error_mark_node)
  {
    wroteFlag = true;
    declStr += gcckdm::getAstNodeName(TREE_VALUE(arg));
    arg = TREE_CHAIN(arg);
    if (TREE_CHAIN(arg) && TREE_CODE(TREE_CHAIN(arg)) == TREE_LIST)
    {
      declStr += ", ";
    }
  }
  if (!wroteFlag)
  {
    declStr += "void";
  }
  declStr += ")";
  return declStr;
}

///**
// * Replace /n with //n and /t with //t etc. etc. etc
// *
// * @param nameStr the string to append the transformed str
// * @param str the c-string to transform
// */
//void prettyPrintStringCst(std::string & nameStr, const char * str)
//{
//  if (str == 0)
//  {
//    return;
//  }
//
//  while (*str)
//  {
//    while (*str)
//    {
//      switch (str[0])
//      {
//        case '\b':
//          nameStr += "\\b";
//          break;
//
//        case '\f':
//          nameStr += "\\f";
//          break;
//
//        case '\n':
//          nameStr += "\\n";
//          break;
//
//        case '\r':
//          nameStr += "\\r";
//          break;
//
//        case '\t':
//          nameStr += "\\t";
//          break;
//
//        case '\v':
//          nameStr += "\\v";
//          break;
//
//        case '\\':
//          nameStr += "\\\\";
//          break;
//
//        case '\"':
//          nameStr += "\\\"";
//          break;
//
//        case '\'':
//          nameStr += "\\'";
//          break;
//
//          /* No need to handle \0; the loop terminates on \0.  */
//
//        case '\1':
//          nameStr += "\\1";
//          break;
//
//        case '\2':
//          nameStr += "\\2";
//          break;
//
//        case '\3':
//          nameStr += "\\3";
//          break;
//
//        case '\4':
//          nameStr += "\\4";
//          break;
//
//        case '\5':
//          nameStr += "\\5";
//          break;
//
//        case '\6':
//          nameStr += "\\6";
//          break;
//
//        case '\7':
//          nameStr += "\\7";
//          break;
//
//        default:
//          nameStr += str[0];
//          break;
//      }
//      str++;
//    }
//  }
//}


std::string getDomainString(tree domain)
{
  std::string domainStr = "[";
  if (domain)
  {
    tree min = TYPE_MIN_VALUE (domain);
    tree max = TYPE_MAX_VALUE (domain);
    if (min && max && integer_zerop (min) && host_integerp (max, 0))
    {
      domainStr += boost::str(boost::format(HOST_WIDE_INT_PRINT_DEC) % static_cast<HOST_WIDE_INT>(TREE_INT_CST_LOW (max) + 1));
    }
    else
    {
      if (min)
      {
        domainStr += gcckdm::getAstNodeName(min);
      }
      domainStr += ":";
      if (max)
      {
        domainStr += gcckdm::getAstNodeName(max);
      }
    }
  }
  else
  {
    domainStr += "<unknown>";
  }
  domainStr += "]";
  return domainStr;
}


} // namespace

namespace gcckdm
{

location_t locationOf(tree t)
{
  if (TREE_CODE(t) == PARM_DECL && DECL_CONTEXT(t))
    t = DECL_CONTEXT(t);
  else if (TYPE_P(t))
    t = TYPE_MAIN_DECL(t);
  else if (TREE_CODE(t) == OVERLOAD)
    t = OVL_FUNCTION(t);

  if (!t)
    return UNKNOWN_LOCATION;

  if (DECL_P(t))
    return DECL_SOURCE_LOCATION(t);
  else if (EXPR_P(t) && EXPR_HAS_LOCATION(t))
    return EXPR_LOCATION(t);
  else
    return UNKNOWN_LOCATION;
}

bool locationIsUnknown(location_t loc)
{
  location_t unk = UNKNOWN_LOCATION;
  return !memcmp(&loc, &unk, sizeof(location_t));
}

std::string const locationString(location_t loc)
{
  if (locationIsUnknown(loc))
  {
    return "";
  }
  expanded_location eloc = expand_location(loc);
  std::ostringstream str;
  str << eloc.file << ":" << eloc.line << ":" << eloc.column;
  return str.str();
}

/**
 * Return the demangled name for the given node, if possible. Otherwise return the standard name.
 */
std::string getDemangledName(tree node)
{
  // First try for the demangled assembler name
  if (HAS_DECL_ASSEMBLER_NAME_P(node) &&
      DECL_NAME (node) &&
      DECL_ASSEMBLER_NAME (node) &&
      DECL_ASSEMBLER_NAME (node) != DECL_NAME (node))
  {
    tree mangledNameNode = DECL_ASSEMBLER_NAME (node);
    const char *namePtr = IDENTIFIER_POINTER (mangledNameNode);
    if(namePtr)
    {
      std::string mangledName(namePtr);

      // Remove INTERNAL name
      size_t index = mangledName.find(" *INTERNAL* ");
      if (index != std::string::npos)
      {
        mangledName.erase(index, 12);
      }

      const int demangle_opt = (DMGL_STYLE_MASK | DMGL_PARAMS | DMGL_TYPES | DMGL_ANSI) & ~DMGL_JAVA;
      namePtr = cplus_demangle(mangledName.c_str(), demangle_opt);
      if(namePtr)
      {
        std::string demangledName(cplus_demangle(mangledName.c_str(), demangle_opt));
        // Remove class qualifier part of the name, if it exists
        index = demangledName.find("::");
        size_t braceIndex = demangledName.find("(");

        // Loop until the demangled name is cleaned up
        while(true)
        {
          // No :: delimiters left
          if(index == std::string::npos) break;
          if(braceIndex != std::string::npos)
          {
            // :: is inside brace
            if(braceIndex < index) break;
          }
          demangledName.erase(0, index + 2);
          index = demangledName.find("::");
          braceIndex = demangledName.find("(");
        }

        // Remove the function qualifier part of the name, if it exists. This is
        // required for local variables (eg. foo()::b)
        braceIndex = demangledName.find_last_of(")");
        index = demangledName.find_last_of("::");
        if(braceIndex != std::string::npos && index != std::string::npos)
        {
          if(index > braceIndex)
          {
            demangledName.erase(0, index + 2);
          }
        }

        return demangledName;
      }
    }
  }

  // Otherwise return the empty string
  std::string nameStr("");
  return nameStr;
}

/**
 * Get the demangled name for a node if possible
 */
std::string getDemangledNodeName(tree node)
{
  if (node != NULL_TREE)
  {
    // If this is a type node, demangle the type name
    if(TYPE_P(node) && TYPE_NAME (node))
    {
      return getDemangledName(TYPE_NAME (node));
    }
    return getDemangledName(node);
  }

  // Otherwise return the empty string
  std::string nameStr("");
  return nameStr;
}

/**
 *
 */
std::string getAstNodeName(tree node)
{
  // In C++, use the demangler if possible.
  if(isFrontendCxx())
  {
    std::string name(getDemangledNodeName(node));
    if(!name.empty())
    {
      return name;
    }
  }

  std::string nameStr;

  if (node != NULL_TREE)
  {
    switch (TREE_CODE(node))
    {
      case IDENTIFIER_NODE:
      {
        nameStr = getAstIdentifierNodeName(node);
        break;
      }
      case TREE_LIST:
      {
        nameStr += getAstNodeName(TREE_VALUE(node));
        break;
      }
      case VOID_TYPE:
      case INTEGER_TYPE:
      case REAL_TYPE:
      case FIXED_POINT_TYPE:
      case COMPLEX_TYPE:
      case VECTOR_TYPE:
      case ENUMERAL_TYPE:
      case BOOLEAN_TYPE:
      {
        enum tree_code_class tclass = TREE_CODE_CLASS(TREE_CODE(node));
        if (tclass == tcc_declaration)
        {
          nameStr = getAstDeclarationNodeName(node);
        }
        else if (tclass == tcc_type)
        {
          if (TYPE_NAME(node))
          {
            if (TREE_CODE(TYPE_NAME(node)) == IDENTIFIER_NODE)
            {
              nameStr = getAstIdentifierNodeName(TYPE_NAME(node));
            }
            else if (TREE_CODE(TYPE_NAME(node)) == TYPE_DECL && DECL_NAME(TYPE_NAME(node)))
            {
              nameStr = getAstDeclarationNodeName(TYPE_NAME(node));
            }
          }
        }
        break;
      }
      case POINTER_TYPE:
      case REFERENCE_TYPE:
      {
        std::string tempStr((TREE_CODE(node) == POINTER_TYPE ? "*" : "&"));
        if (TREE_TYPE(node) == NULL)
        {
          nameStr += tempStr + "<null type>";
        }
        else if (TREE_CODE(TREE_TYPE(node)) == FUNCTION_TYPE)
        {
          tree fNode = TREE_TYPE(node);
          nameStr = getAstNodeName(TREE_TYPE(fNode));
          nameStr += " (" + tempStr;

          if (TYPE_NAME(node) && DECL_NAME(TYPE_NAME(node)))
          {
            nameStr += getAstDeclarationNodeName(TYPE_NAME(node));
          }
          else
          {
            nameStr += " <T." + boost::lexical_cast<std::string>(TYPE_UID(node)) + ">";
          }
          nameStr += ")";
          nameStr += getAstFunctionDeclarationName(fNode);
        }
        else
        {
          nameStr += getAstNodeName(TREE_TYPE(node)) + " " + tempStr;
        }

        break;
      }

      case RESULT_DECL:
      {
        nameStr = "__RESULT__";
        break;
      }
      case VAR_DECL:
      case PARM_DECL:
      case FIELD_DECL:
      case CONST_DECL:
      case FUNCTION_DECL:
      case LABEL_DECL:
      {
        nameStr = getAstDeclarationNodeName(node);
        break;
      }
      case TYPE_DECL:
      {
        if (DECL_IS_BUILTIN(node))
        {
          /* Don't print the declaration of built-in types.  */
          break;
        }
        if (DECL_NAME(node))
        {
          nameStr = getAstDeclarationNodeName(node);
        }
        else
        {
          nameStr = TREE_CODE(TREE_TYPE(node)) == UNION_TYPE ? "union" : "struct ";
          nameStr += " " + getAstNodeName(TREE_TYPE(node));
        }
        break;
      }

      case FUNCTION_TYPE:
      case METHOD_TYPE:
      {
        nameStr = getAstNodeName(TREE_TYPE(node));
        if (TYPE_NAME(node) && DECL_NAME(TYPE_NAME(node)))
        {
          nameStr += " " + getAstDeclarationNodeName(TYPE_NAME(node));
        }
        else
        {
          nameStr += " <T." + boost::lexical_cast<std::string>(TYPE_UID(
              node)) + ">";
        }
        nameStr += getAstFunctionDeclarationName(node);
        break;
      }
      case ARRAY_TYPE:
      {
        tree tmp;
        //Retrieve the innermost component type
        for (tmp = TREE_TYPE(node); TREE_CODE(tmp) == ARRAY_TYPE; tmp = TREE_TYPE(tmp))
        {
          //empty on purpose
        }
        nameStr += getAstNodeName(tmp);

        //Print dimensions
        for (tmp = node; TREE_CODE (tmp) == ARRAY_TYPE; tmp = TREE_TYPE (tmp))
        {
          nameStr += getDomainString(TYPE_DOMAIN (tmp));
        }
        break;
      }
      case RECORD_TYPE:
      case UNION_TYPE:
      {
        if (TYPE_NAME(node))
        {
          nameStr += getAstNodeName(TYPE_NAME(node));
        }
        break;
      }
      case INTEGER_CST:
      {
        if (TREE_CODE(TREE_TYPE(node)) == POINTER_TYPE)
        {
          nameStr += boost::str(boost::format(HOST_WIDE_INT_PRINT_DEC) % static_cast<HOST_WIDE_INT>(TREE_INT_CST_LOW (node)));
        }
        else if (!host_integerp(node, 0))
        {
          {
            tree val = node;
            unsigned HOST_WIDE_INT low = TREE_INT_CST_LOW (val);
            HOST_WIDE_INT high = TREE_INT_CST_HIGH(val);

            if (tree_int_cst_sgn(val) < 0)
            {
              nameStr += "-";
              high = ~high + !low;
              low = -low;
            }
            /* Would "%x%0*x" or "%x%*0x" get zero-padding on all
             systems?  */
            nameStr += boost::str(boost::format(HOST_WIDE_INT_PRINT_DOUBLE_HEX) % static_cast<HOST_WIDE_INT> (high) % low);
          }
        }
        else
        {
          nameStr += boost::str(boost::format(HOST_WIDE_INT_PRINT_DEC) % static_cast<HOST_WIDE_INT>(TREE_INT_CST_LOW (node)));
        }
        break;
      }
      case REAL_CST:
      {
        REAL_VALUE_TYPE d;
        if (TREE_OVERFLOW (node))
        {
          nameStr += "Overflow";
        }
        d = TREE_REAL_CST (node);
        if (REAL_VALUE_ISINF (d))
        {
          nameStr += "Inf";
        }
        else if (REAL_VALUE_ISNAN (d))
        {
          nameStr += "Nan";
        }
        else
        {
          char str[64];
          real_to_decimal (str, &d, sizeof (str), 0, 1);
          nameStr += str;
        }
        break;
      }
      case STRING_CST:
      {
        replaceSpecialCharsCopy(TREE_STRING_POINTER (node), nameStr);
        break;
      }
      case ARRAY_REF:
      {
        tree op0 = TREE_OPERAND (node, 0);
        if (op_prio (op0) < op_prio (node))
        {
          nameStr += "(";
        }
        nameStr += getAstNodeName(op0);
        if (op_prio (op0) < op_prio (node))
        {
          nameStr += ")";
        }
        nameStr += "[";
        nameStr += getAstNodeName(TREE_OPERAND (node, 1));
        nameStr += "]";
        break;
      }
      case ADDR_EXPR:
      {
        if ((TREE_CODE (TREE_OPERAND (node, 0)) == STRING_CST
            || TREE_CODE (TREE_OPERAND (node, 0)) == FUNCTION_DECL))
        {
          //Do Nothing
        }
        else
        {
          nameStr += op_symbol_code (TREE_CODE (node));
        }
        if (op_prio (TREE_OPERAND (node, 0)) < op_prio (node))
        {
          nameStr += '(' + getAstNodeName(TREE_OPERAND (node, 0)) + ')';
        }
        else
        {
          nameStr += getAstNodeName(TREE_OPERAND (node, 0));
        }
        break;
      }
      default:
      {
        std::cerr << "# UNSUPPORTED: node type (" << tree_code_name[TREE_CODE(node)] << ") in " << BOOST_CURRENT_FUNCTION << std::endl;
      }

    }
  }


  boost::replace_all(nameStr, "\n" ,"\\n");

  return nameStr;
}

/**
 * Uses the  lang_hooks to determine if we are in using the c++ or c frontend
 */
bool isFrontendCxx()
{
  std::string langName(lang_hooks.name);
  return langName == "GNU C++";
}

/**
 * Uses the  lang_hooks to determine if we are in using the c++ or c frontend
 */
bool isFrontendC()
{
  std::string langName(lang_hooks.name);
  return langName == "GNU C";
}

/**
 * Returns the type qualifiers for this type, including the qualifiers on the
 * elements for an array type
 */
int getTypeQualifiers(tree const type)
{
  int retVal;
  tree t = (DECL_P(type)) ? TREE_TYPE(type) : type;
  t = strip_array_types (t);

  if (t == error_mark_node or not t)
  {
    retVal = TYPE_UNQUALIFIED;
  }
  else
  {
    retVal = TYPE_QUALS (t);
  }
  return retVal;
}

/**
 *
 */
std::string getLinkId(tree const typeName, std::string const name)
{
  if (HAS_DECL_ASSEMBLER_NAME_P(typeName) &&
      DECL_NAME (typeName) &&
      DECL_ASSEMBLER_NAME (typeName) &&
      DECL_ASSEMBLER_NAME (typeName) != DECL_NAME (typeName))
  {
    tree asmNode = DECL_ASSEMBLER_NAME (typeName);
    return std::string(IDENTIFIER_POINTER (asmNode));
  }

  return name;
}

} // namespace gcckdm
