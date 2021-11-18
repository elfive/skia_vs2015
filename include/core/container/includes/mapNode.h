#pragma once
#include "algorithms.h"
#include <cstddef>


namespace SkUtils
{
	template <typename T, class C>
	class mapNode
	{
	public:
		typedef T value_type;
		typedef T &reference;
		typedef T *pointer;
		typedef C key_compare;
		typedef const value_type *const_pointer;
		typedef mapNode self_type;

		value_type data;
		mapNode *parent;
		mapNode *left;
		mapNode *right;

		mapNode(value_type const &val = value_type())
		{
			this->data = val;
			this->parent = nullptr;
			this->left = nullptr;
			this->right = nullptr;
		}

		mapNode(const mapNode &other)
		{
			this->data = other.data;
			this->parent = other.parent;
			this->left = other.left;
			this->right = other.right;
		}

		~mapNode()
		{
		}

		self_type &operator=(const self_type &other)
		{
			this->data = other.data;
			this->parent = other.parent;
			this->left = other.left;
			this->right = other.right;
			return (*this);
		}

		bool operator==(const self_type &other) const
		{
			if (this->data != other.data)
				return (false);
			if (this->parent != other.parent)
				return (false);
			if (this->left != other.left || this->right != other.right)
				return (false);
			return (true);
		}

		bool operator!=(const self_type &other) const
		{
			return (!(*this == other));
		}

		mapNode *get_next()
		{
			mapNode *traverser;
			if (this->right)
			{
				traverser = this->right;
				while (traverser->left)
					traverser = traverser->left;
			}
			else
			{
				mapNode *tmp = &(*this);
				traverser = this->parent;
				while (traverser->left != tmp)
				{
					tmp = traverser;
					traverser = traverser->parent;
				}
			}
			return (traverser);
		}

		mapNode *get_previous()
		{
			mapNode *traverser;
			if (this->left)
			{
				traverser = this->left;
				while (traverser->right)
					traverser = traverser->right;
			}
			else
				traverser = this->parent;
			return (traverser);
		}
	};
}