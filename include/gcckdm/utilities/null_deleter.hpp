#ifndef NULL_DELETER_HPP
#define NULL_DELETER_HPP

struct null_deleter
{
  void operator()(void const *) const
  {
  }
};

#endif //NULL_DELETER_HPP
