#pragma once
#include "traits.h"
#include <cstddef>


namespace SkUtils
{
	template <class Iterator>
	ptrdiff_t distance(Iterator first, Iterator last,
		typename enable_if<is_iterator<typename Iterator::iterator_category>::value, Iterator>::type * = nullptr)
	{
		ptrdiff_t n = 0;
		while (first != last)
		{
			first++;
			n++;
		}
		return (n);
	}

	template <class Arg1, class Arg2, class Result>
	struct binary_function
	{
		typedef Arg1 first_argument_type;
		typedef Arg2 second_argument_type;
		typedef Result result_type;
	};

	template <class T>
	struct less : binary_function<T, T, bool>
	{
		bool operator()(const T &x, const T &y) const
		{
			return (x < y);
		}
	};
}
