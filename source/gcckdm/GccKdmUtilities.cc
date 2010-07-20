/*
 * GccKdmUtilities.cc
 *
 *  Created on: Jun 7, 2010
 *      Author: kgirard
 */

#include "gcckdm/GccKdmConfig.hh"
#include "gcckdm/GccKdmUtilities.hh"

#include <boost/lexical_cast.hpp>
#include <iostream>
#include <sstream>
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

}

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

std::string getAstNodeName(tree node)
{
  std::string nameStr("");
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
          nameStr += boost::str(boost::format(HOST_WIDE_INT_PRINT_DEC) % TREE_INT_CST_LOW (node));
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
            nameStr += boost::str(boost::format(HOST_WIDE_INT_PRINT_DOUBLE_HEX) % static_cast<unsigned HOST_WIDE_INT> (high) % low);
          }
        }
        else
        {
          nameStr += boost::str(boost::format(HOST_WIDE_INT_PRINT_DEC) % TREE_INT_CST_LOW (node));
        }
        break;
      }
      default:
      {
        std::cerr << "gcckdm::getAstNodeName() not implemented yet: " << tree_code_name[TREE_CODE(node)] << std::endl;
      }

    }
  }
  return nameStr;
}

} // namespace gcckdm
