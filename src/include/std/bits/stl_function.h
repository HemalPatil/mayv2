#pragma once

namespace std {
	/**
   *  Helper for defining adaptable unary function objects.
   *  @deprecated Deprecated in C++11, no longer in the standard since C++17.
   */
  template<typename _Arg, typename _Result>
    struct unary_function
    {
      /// @c argument_type is the type of the argument
      typedef _Arg 	argument_type;

      /// @c result_type is the return type
      typedef _Result 	result_type;
    };

	/**
   *  Helper for defining adaptable binary function objects.
   *  @deprecated Deprecated in C++11, no longer in the standard since C++17.
   */
  template<typename _Arg1, typename _Arg2, typename _Result>
    struct binary_function
    {
      /// @c first_argument_type is the type of the first argument
      typedef _Arg1 	first_argument_type; 

      /// @c second_argument_type is the type of the second argument
      typedef _Arg2 	second_argument_type;

      /// @c result_type is the return type
      typedef _Result 	result_type;
    };
}
