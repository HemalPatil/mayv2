#pragma once

#include <bits/stl_function.h>

namespace std {
	/**
   * Derives from @c unary_function or @c binary_function, or perhaps
   * nothing, depending on the number of arguments provided. The
   * primary template is the basis case, which derives nothing.
   */
  template<typename _Res, typename... _ArgTypes>
    struct _Maybe_unary_or_binary_function { };

  /// Derives from @c unary_function, as appropriate.
  template<typename _Res, typename _T1>
    struct _Maybe_unary_or_binary_function<_Res, _T1>
    : std::unary_function<_T1, _Res> { };

  /// Derives from @c binary_function, as appropriate.
  template<typename _Res, typename _T1, typename _T2>
    struct _Maybe_unary_or_binary_function<_Res, _T1, _T2>
    : std::binary_function<_T1, _T2, _Res> { };
}
