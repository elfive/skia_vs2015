﻿#pragma once
#include "traits.h"
#include <cstddef>


namespace SkUtils
{
	template <typename T, typename R, typename P, typename N>
	class bidirectional_iterator
	{
	public:
		typedef T value_type;
		typedef R reference;
		typedef P pointer;
		typedef N node_pointer;
		typedef size_t size_type;
		typedef ptrdiff_t difference_type;
		typedef bidirectional_iterator_tag iterator_category;
		typedef bidirectional_iterator self_type;

		bidirectional_iterator(node_pointer element)
		{
			this->element = element;
		}

		bidirectional_iterator(self_type const &other)
		{
			this->element = other.element;
		}

		~bidirectional_iterator()
		{
		}

		bidirectional_iterator &operator=(self_type const &other)
		{
			this->element = other.element;
			return (*this);
		}

		reference operator*()
		{
			return (this->element->data);
		}

		pointer operator->()
		{
			return (&(this->element->data));
		}

		bool operator==(self_type const &other) const
		{
			if (*this->element != *other.element)
				return (false);
			return (true);
		}

		bool operator!=(self_type const &other) const
		{
			if (*this == other)
				return (false);
			return (true);
		}

		self_type &operator++()
		{
			this->element = this->element->get_next();
			return (*this);
		}

		self_type operator++(int)
		{
			self_type tmp(*this);
			this->element = this->element->get_next();
			return (tmp);
		}

		self_type &operator--()
		{
			this->element = this->element->get_previous();
			return (*this);
		}

		self_type operator--(int)
		{
			self_type tmp(*this);
			this->element = this->element->get_previous();
			return (tmp);
		}

	private:
		node_pointer element;
	};

	template <typename T, typename R, typename P, typename N>
	class reverse_bidirectional_iterator
	{
	public:
		typedef T value_type;
		typedef R reference;
		typedef P pointer;
		typedef N node_pointer;
		typedef size_t size_type;
		typedef ptrdiff_t difference_type;
		typedef bidirectional_iterator_tag iterator_category;
		typedef reverse_bidirectional_iterator self_type;

		reverse_bidirectional_iterator(node_pointer element) : element(element)
		{
		}

		reverse_bidirectional_iterator(self_type const &other) : element(other.element)
		{
		}

		~reverse_bidirectional_iterator()
		{
		}

		reverse_bidirectional_iterator &operator=(self_type const &other)
		{
			this->element = other.element;
			return (*this);
		}

		reference operator*()
		{
			return (this->element->data);
		}

		pointer operator->()
		{
			return (&(this->element->data));
		}

		bool operator==(self_type const &other) const
		{
			if (this->element != other.element)
				return (false);
			return (true);
		}

		bool operator!=(self_type const &other) const
		{
			if (*this == other)
				return (false);
			return (true);
		}

		self_type &operator++()
		{
			this->element = this->element->get_previous();
			return (*this);
		}

		self_type operator++(int)
		{
			self_type tmp(*this);
			this->element = this->element->get_previous();
			return (tmp);
		}

		self_type &operator--()
		{
			this->element = this->element->get_next();
			return (*this);
		}

		self_type operator--(int)
		{
			self_type tmp(*this);
			this->element = this->element->get_next();
			return (tmp);
		}

	private:
		node_pointer element;
	};
}
