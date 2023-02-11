#include <bits/allocator.h>
#include <bits/stl_tree.h>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <kernel.h>
#include <terminal.h>

static const char* const invalidArgStr = "\nInvalid argument exception\n";
static const char* const logicErrorStr = "\nLogic error exception\n";
static const char* const lengthErrorStr = "\nLength error exception\n";
static const char* const outOfRangeStr = "\nOut of range exception\n";
static const char* const outOfRangeFmtStr = "\nOut of range format exception\n";
static const char* const badFunctionCallStr = "\nBad function call exception\n";
static const char* const badAllocStr = "\nBad alloc exception\n";
static const char* const badArrayStr = "\nBad array exception\n";
static const char* const systemErrorStr = "\nSystem exception\n";

extern "C" {

size_t strlen(const char* str) {
	size_t length = 0;
	while (*str++) {
		++length;
	}
	return length;
}

int strcmp(const char* str1, const char* str2) {
	while (*str1 && (*str1 == *str2)) {
		++str1;
		++str2;
	}
	return *(const unsigned char*)str1 - *(const unsigned char*)str2;
}

int strncmp(const char *str1, const char *str2, size_t count) {
	if (count == 0) {
		return 0;
	}
	do {
		if (*str1 != *str2++) {
			return (*(const unsigned char*)str1 - *(const unsigned char*)--str2);
		}
		if (*str1++ == 0) {
			break;
		}
	} while (--count != 0);
	return 0;
}

int memcmp (const void *addr1, const void *addr2, size_t count) {
	const unsigned char *s1 = (const unsigned char*)addr1;
	const unsigned char *s2 = (const unsigned char*)addr2;

	while (count-- > 0) {
		if (*s1++ != *s2++) {
			return s1[-1] < s2[-1] ? -1 : 1;
		}
	}
	return 0;
}

void* memcpy(void *dest, const void *src, size_t count) {
	// Copies from front to back
	// Need not worry about overlapping regions
	char *d = (char*)dest;
	const char *s = (char*)src;
	while (count--) {
		*d++ = *s++;
	}
	return dest;
}

void* memmove(void *dest, const void *src, size_t count) {
	char *d = (char*)dest;
	const char *s = (char*)src;
	if (d < s) {
		// Copy from front to back
		while (count--) {
			*d++ = *s++;
		}
	} else {
		// Copy from back to front
		const char *lasts = s + (count - 1);
		char *lastd = d + (count - 1);
		while (count--) {
			*lastd-- = *lasts--;
		}
	}
	return dest;
}

void* memset(void *address, int value, size_t count) {
	unsigned char *ptr = (unsigned char*)address;
	while (count-- > 0) {
		*ptr++ = value;
	}
	return address;
}

void *__dso_handle = 0;

int __cxa_atexit(void (*)(void*), void*, void*) {
	// TODO: should register the global destructors in some table
	// Can be avoided since destructing global objects doesn't make much sense
	// because the kernel never exits
	return 0;
};

}

void *operator new(size_t size) {
	return Kernel::Memory::Heap::allocate(size);
}

void *operator new[](size_t size) {
	return Kernel::Memory::Heap::allocate(size);
}

void operator delete(void *memory) {
	Kernel::Memory::Heap::free(memory);
}

void operator delete[](void *memory) {
	Kernel::Memory::Heap::free(memory);
}

// TODO: Do not know the meaning of -Wsized-deallocation but this removes the warning
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"
void operator delete(void *memory, size_t size) {
	Kernel::Memory::Heap::free(memory);
}

void operator delete[](void *memory, size_t size) {
	Kernel::Memory::Heap::free(memory);
}
#pragma GCC diagnostic pop

namespace std {
	// Explicit template instantiation of allocator<char> for std::string
	template class allocator<char>;

	// Helpers for exception objects in <system_error>
	void __throw_system_error(int) {
		terminalPrintString(systemErrorStr, strlen(systemErrorStr));
		Kernel::panic();
	}

	// Helpers for exception objects in <stdexcept>
	void __throw_invalid_argument(const char*) {
		terminalPrintString(invalidArgStr, strlen(invalidArgStr));
		Kernel::panic();
	}
	void __throw_logic_error(const char*) {
		terminalPrintString(logicErrorStr, strlen(logicErrorStr));
		Kernel::panic();
	}
	void __throw_length_error(const char*) {
		terminalPrintString(lengthErrorStr, strlen(lengthErrorStr));
		Kernel::panic();
	}
	void __throw_out_of_range(const char*) {
		terminalPrintString(outOfRangeStr, strlen(outOfRangeStr));
		Kernel::panic();
	}
	void __throw_out_of_range_fmt(const char*, ...) {
		terminalPrintString(outOfRangeFmtStr, strlen(outOfRangeFmtStr));
		Kernel::panic();
	}

	// Helpers for exception objects in <functional>
	void __throw_bad_function_call() {
		terminalPrintString(badFunctionCallStr, strlen(badFunctionCallStr));
		Kernel::panic();
	}

	// Helper for exception objects in <new>
	void __throw_bad_alloc(void) {
		terminalPrintString(badAllocStr, strlen(badAllocStr));
		Kernel::panic();
	}
	void __throw_bad_array_new_length(void) {
		terminalPrintString(badArrayStr, strlen(badArrayStr));
		Kernel::panic();
	}
}

// RB tree utilities implementation -*- C++ -*-

// Copyright (C) 2003-2022 Free Software Foundation, Inc.
//
// This file is part of the GNU ISO C++ Library.  This library is free
// software; you can redistribute it and/or modify it under the
// terms of the GNU General Public License as published by the
// Free Software Foundation; either version 3, or (at your option)
// any later version.

// This library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

// Under Section 7 of GPL version 3, you are granted additional
// permissions described in the GCC Runtime Library Exception, version
// 3.1, as published by the Free Software Foundation.

// You should have received a copy of the GNU General Public License and
// a copy of the GCC Runtime Library Exception along with this program;
// see the files COPYING3 and COPYING.RUNTIME respectively.  If not, see
// <http://www.gnu.org/licenses/>.

/*
 *
 * Copyright (c) 1996,1997
 * Silicon Graphics Computer Systems, Inc.
 *
 * Permission to use, copy, modify, distribute and sell this software
 * and its documentation for any purpose is hereby granted without fee,
 * provided that the above copyright notice appear in all copies and
 * that both that copyright notice and this permission notice appear
 * in supporting documentation.  Silicon Graphics makes no
 * representations about the suitability of this software for any
 * purpose.  It is provided "as is" without express or implied warranty.
 *
 *
 * Copyright (c) 1994
 * Hewlett-Packard Company
 *
 * Permission to use, copy, modify, distribute and sell this software
 * and its documentation for any purpose is hereby granted without fee,
 * provided that the above copyright notice appear in all copies and
 * that both that copyright notice and this permission notice appear
 * in supporting documentation.  Hewlett-Packard Company makes no
 * representations about the suitability of this software for any
 * purpose.  It is provided "as is" without express or implied warranty.
 *
 *
 */

namespace std _GLIBCXX_VISIBILITY(default)
{
_GLIBCXX_BEGIN_NAMESPACE_VERSION

  static _Rb_tree_node_base*
  local_Rb_tree_increment(_Rb_tree_node_base* __x) throw ()
  {
    if (__x->_M_right != 0)
      {
        __x = __x->_M_right;
        while (__x->_M_left != 0)
          __x = __x->_M_left;
      }
    else
      {
        _Rb_tree_node_base* __y = __x->_M_parent;
        while (__x == __y->_M_right)
          {
            __x = __y;
            __y = __y->_M_parent;
          }
        if (__x->_M_right != __y)
          __x = __y;
      }
    return __x;
  }

  _Rb_tree_node_base*
  _Rb_tree_increment(_Rb_tree_node_base* __x) throw ()
  {
    return local_Rb_tree_increment(__x);
  }

  const _Rb_tree_node_base*
  _Rb_tree_increment(const _Rb_tree_node_base* __x) throw ()
  {
    return local_Rb_tree_increment(const_cast<_Rb_tree_node_base*>(__x));
  }

  static _Rb_tree_node_base*
  local_Rb_tree_decrement(_Rb_tree_node_base* __x) throw ()
  {
    if (__x->_M_color == _S_red
        && __x->_M_parent->_M_parent == __x)
      __x = __x->_M_right;
    else if (__x->_M_left != 0)
      {
        _Rb_tree_node_base* __y = __x->_M_left;
        while (__y->_M_right != 0)
          __y = __y->_M_right;
        __x = __y;
      }
    else
      {
        _Rb_tree_node_base* __y = __x->_M_parent;
        while (__x == __y->_M_left)
          {
            __x = __y;
            __y = __y->_M_parent;
          }
        __x = __y;
      }
    return __x;
  }

  _Rb_tree_node_base*
  _Rb_tree_decrement(_Rb_tree_node_base* __x) throw ()
  {
    return local_Rb_tree_decrement(__x);
  }

  const _Rb_tree_node_base*
  _Rb_tree_decrement(const _Rb_tree_node_base* __x) throw ()
  {
    return local_Rb_tree_decrement(const_cast<_Rb_tree_node_base*>(__x));
  }

  static void
  local_Rb_tree_rotate_left(_Rb_tree_node_base* const __x,
		             _Rb_tree_node_base*& __root)
  {
    _Rb_tree_node_base* const __y = __x->_M_right;

    __x->_M_right = __y->_M_left;
    if (__y->_M_left !=0)
      __y->_M_left->_M_parent = __x;
    __y->_M_parent = __x->_M_parent;

    if (__x == __root)
      __root = __y;
    else if (__x == __x->_M_parent->_M_left)
      __x->_M_parent->_M_left = __y;
    else
      __x->_M_parent->_M_right = __y;
    __y->_M_left = __x;
    __x->_M_parent = __y;
  }

#if !_GLIBCXX_INLINE_VERSION
  /* Static keyword was missing on _Rb_tree_rotate_left.
     Export the symbol for backward compatibility until
     next ABI change.  */
  void
  _Rb_tree_rotate_left(_Rb_tree_node_base* const __x,
		       _Rb_tree_node_base*& __root)
  { local_Rb_tree_rotate_left (__x, __root); }
#endif

  static void
  local_Rb_tree_rotate_right(_Rb_tree_node_base* const __x,
			     _Rb_tree_node_base*& __root)
  {
    _Rb_tree_node_base* const __y = __x->_M_left;

    __x->_M_left = __y->_M_right;
    if (__y->_M_right != 0)
      __y->_M_right->_M_parent = __x;
    __y->_M_parent = __x->_M_parent;

    if (__x == __root)
      __root = __y;
    else if (__x == __x->_M_parent->_M_right)
      __x->_M_parent->_M_right = __y;
    else
      __x->_M_parent->_M_left = __y;
    __y->_M_right = __x;
    __x->_M_parent = __y;
  }

#if !_GLIBCXX_INLINE_VERSION
  /* Static keyword was missing on _Rb_tree_rotate_right
     Export the symbol for backward compatibility until
     next ABI change.  */
  void
  _Rb_tree_rotate_right(_Rb_tree_node_base* const __x,
			_Rb_tree_node_base*& __root)
  { local_Rb_tree_rotate_right (__x, __root); }
#endif

  void
  _Rb_tree_insert_and_rebalance(const bool          __insert_left,
                                _Rb_tree_node_base* __x,
                                _Rb_tree_node_base* __p,
                                _Rb_tree_node_base& __header) throw ()
  {
    _Rb_tree_node_base *& __root = __header._M_parent;

    // Initialize fields in new node to insert.
    __x->_M_parent = __p;
    __x->_M_left = 0;
    __x->_M_right = 0;
    __x->_M_color = _S_red;

    // Insert.
    // Make new node child of parent and maintain root, leftmost and
    // rightmost nodes.
    // N.B. First node is always inserted left.
    if (__insert_left)
      {
        __p->_M_left = __x; // also makes leftmost = __x when __p == &__header

        if (__p == &__header)
        {
            __header._M_parent = __x;
            __header._M_right = __x;
        }
        else if (__p == __header._M_left)
          __header._M_left = __x; // maintain leftmost pointing to min node
      }
    else
      {
        __p->_M_right = __x;

        if (__p == __header._M_right)
          __header._M_right = __x; // maintain rightmost pointing to max node
      }
    // Rebalance.
    while (__x != __root
	   && __x->_M_parent->_M_color == _S_red)
      {
	_Rb_tree_node_base* const __xpp = __x->_M_parent->_M_parent;

	if (__x->_M_parent == __xpp->_M_left)
	  {
	    _Rb_tree_node_base* const __y = __xpp->_M_right;
	    if (__y && __y->_M_color == _S_red)
	      {
		__x->_M_parent->_M_color = _S_black;
		__y->_M_color = _S_black;
		__xpp->_M_color = _S_red;
		__x = __xpp;
	      }
	    else
	      {
		if (__x == __x->_M_parent->_M_right)
		  {
		    __x = __x->_M_parent;
		    local_Rb_tree_rotate_left(__x, __root);
		  }
		__x->_M_parent->_M_color = _S_black;
		__xpp->_M_color = _S_red;
		local_Rb_tree_rotate_right(__xpp, __root);
	      }
	  }
	else
	  {
	    _Rb_tree_node_base* const __y = __xpp->_M_left;
	    if (__y && __y->_M_color == _S_red)
	      {
		__x->_M_parent->_M_color = _S_black;
		__y->_M_color = _S_black;
		__xpp->_M_color = _S_red;
		__x = __xpp;
	      }
	    else
	      {
		if (__x == __x->_M_parent->_M_left)
		  {
		    __x = __x->_M_parent;
		    local_Rb_tree_rotate_right(__x, __root);
		  }
		__x->_M_parent->_M_color = _S_black;
		__xpp->_M_color = _S_red;
		local_Rb_tree_rotate_left(__xpp, __root);
	      }
	  }
      }
    __root->_M_color = _S_black;
  }

  _Rb_tree_node_base*
  _Rb_tree_rebalance_for_erase(_Rb_tree_node_base* const __z,
			       _Rb_tree_node_base& __header) throw ()
  {
    _Rb_tree_node_base *& __root = __header._M_parent;
    _Rb_tree_node_base *& __leftmost = __header._M_left;
    _Rb_tree_node_base *& __rightmost = __header._M_right;
    _Rb_tree_node_base* __y = __z;
    _Rb_tree_node_base* __x = 0;
    _Rb_tree_node_base* __x_parent = 0;

    if (__y->_M_left == 0)     // __z has at most one non-null child. y == z.
      __x = __y->_M_right;     // __x might be null.
    else
      if (__y->_M_right == 0)  // __z has exactly one non-null child. y == z.
	__x = __y->_M_left;    // __x is not null.
      else
	{
	  // __z has two non-null children.  Set __y to
	  __y = __y->_M_right;   //   __z's successor.  __x might be null.
	  while (__y->_M_left != 0)
	    __y = __y->_M_left;
	  __x = __y->_M_right;
	}
    if (__y != __z)
      {
	// relink y in place of z.  y is z's successor
	__z->_M_left->_M_parent = __y;
	__y->_M_left = __z->_M_left;
	if (__y != __z->_M_right)
	  {
	    __x_parent = __y->_M_parent;
	    if (__x) __x->_M_parent = __y->_M_parent;
	    __y->_M_parent->_M_left = __x;   // __y must be a child of _M_left
	    __y->_M_right = __z->_M_right;
	    __z->_M_right->_M_parent = __y;
	  }
	else
	  __x_parent = __y;
	if (__root == __z)
	  __root = __y;
	else if (__z->_M_parent->_M_left == __z)
	  __z->_M_parent->_M_left = __y;
	else
	  __z->_M_parent->_M_right = __y;
	__y->_M_parent = __z->_M_parent;
	std::swap(__y->_M_color, __z->_M_color);
	__y = __z;
	// __y now points to node to be actually deleted
      }
    else
      {                        // __y == __z
	__x_parent = __y->_M_parent;
	if (__x)
	  __x->_M_parent = __y->_M_parent;
	if (__root == __z)
	  __root = __x;
	else
	  if (__z->_M_parent->_M_left == __z)
	    __z->_M_parent->_M_left = __x;
	  else
	    __z->_M_parent->_M_right = __x;
	if (__leftmost == __z)
	  {
	    if (__z->_M_right == 0)        // __z->_M_left must be null also
	      __leftmost = __z->_M_parent;
	    // makes __leftmost == _M_header if __z == __root
	    else
	      __leftmost = _Rb_tree_node_base::_S_minimum(__x);
	  }
	if (__rightmost == __z)
	  {
	    if (__z->_M_left == 0)         // __z->_M_right must be null also
	      __rightmost = __z->_M_parent;
	    // makes __rightmost == _M_header if __z == __root
	    else                      // __x == __z->_M_left
	      __rightmost = _Rb_tree_node_base::_S_maximum(__x);
	  }
      }
    if (__y->_M_color != _S_red)
      {
	while (__x != __root && (__x == 0 || __x->_M_color == _S_black))
	  if (__x == __x_parent->_M_left)
	    {
	      _Rb_tree_node_base* __w = __x_parent->_M_right;
	      if (__w->_M_color == _S_red)
		{
		  __w->_M_color = _S_black;
		  __x_parent->_M_color = _S_red;
		  local_Rb_tree_rotate_left(__x_parent, __root);
		  __w = __x_parent->_M_right;
		}
	      if ((__w->_M_left == 0 ||
		   __w->_M_left->_M_color == _S_black) &&
		  (__w->_M_right == 0 ||
		   __w->_M_right->_M_color == _S_black))
		{
		  __w->_M_color = _S_red;
		  __x = __x_parent;
		  __x_parent = __x_parent->_M_parent;
		}
	      else
		{
		  if (__w->_M_right == 0
		      || __w->_M_right->_M_color == _S_black)
		    {
		      __w->_M_left->_M_color = _S_black;
		      __w->_M_color = _S_red;
		      local_Rb_tree_rotate_right(__w, __root);
		      __w = __x_parent->_M_right;
		    }
		  __w->_M_color = __x_parent->_M_color;
		  __x_parent->_M_color = _S_black;
		  if (__w->_M_right)
		    __w->_M_right->_M_color = _S_black;
		  local_Rb_tree_rotate_left(__x_parent, __root);
		  break;
		}
	    }
	  else
	    {
	      // same as above, with _M_right <-> _M_left.
	      _Rb_tree_node_base* __w = __x_parent->_M_left;
	      if (__w->_M_color == _S_red)
		{
		  __w->_M_color = _S_black;
		  __x_parent->_M_color = _S_red;
		  local_Rb_tree_rotate_right(__x_parent, __root);
		  __w = __x_parent->_M_left;
		}
	      if ((__w->_M_right == 0 ||
		   __w->_M_right->_M_color == _S_black) &&
		  (__w->_M_left == 0 ||
		   __w->_M_left->_M_color == _S_black))
		{
		  __w->_M_color = _S_red;
		  __x = __x_parent;
		  __x_parent = __x_parent->_M_parent;
		}
	      else
		{
		  if (__w->_M_left == 0 || __w->_M_left->_M_color == _S_black)
		    {
		      __w->_M_right->_M_color = _S_black;
		      __w->_M_color = _S_red;
		      local_Rb_tree_rotate_left(__w, __root);
		      __w = __x_parent->_M_left;
		    }
		  __w->_M_color = __x_parent->_M_color;
		  __x_parent->_M_color = _S_black;
		  if (__w->_M_left)
		    __w->_M_left->_M_color = _S_black;
		  local_Rb_tree_rotate_right(__x_parent, __root);
		  break;
		}
	    }
	if (__x) __x->_M_color = _S_black;
      }
    return __y;
  }

  unsigned int
  _Rb_tree_black_count(const _Rb_tree_node_base* __node,
                       const _Rb_tree_node_base* __root) throw ()
  {
    if (__node == 0)
      return 0;
    unsigned int __sum = 0;
    do
      {
	if (__node->_M_color == _S_black)
	  ++__sum;
	if (__node == __root)
	  break;
	__node = __node->_M_parent;
      }
    while (1);
    return __sum;
  }

_GLIBCXX_END_NAMESPACE_VERSION
} // namespace

