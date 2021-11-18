#pragma once
#include <cstddef>


namespace SkUtils
{
	template <typename T>
	class listNode
	{
	public:
		typedef T value_type;
		typedef T &reference;
		typedef T *pointer;
		typedef const T& const_reference;
		typedef listNode self_type;

		value_type data;
		listNode *next;
		listNode *previous;

		listNode(const value_type &val = value_type())
		{
			this->next = nullptr;
			this->previous = nullptr;
			this->data = val;
		}

		listNode(const listNode &other)
		{
			this->data = other.data;
			this->next = other.next;
			this->previous = other.previous;
		}

		~listNode()
		{
		}

		self_type &operator=(const self_type &other)
		{
			this->data = other.data;
			this->next = other.next;
			this->previous = other.previous;
			return (*this);
		}

		bool operator==(const self_type &other) const
		{
			if (this->data != other.data)
				return (false);
			if (this->next != other.next)
				return (false);
			if (this->previous != other.previous)
				return (false);
			return (true);
		}

		bool operator!=(const self_type &other) const
		{
			return (!(*this == other));
		}

		listNode *get_next()
		{
			return (this->next);
		}

		listNode *get_previous()
		{
			return (this->previous);
		}

	};
}
