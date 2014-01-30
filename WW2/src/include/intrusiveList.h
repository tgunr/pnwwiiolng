#pragma once

template<typename T>
struct IntrusiveList
{
	using value_type = typename T;
	using self_type = typename IntrusiveList<T>;

	class iterator;
	class const_iterator;
	typedef std::reverse_iterator<iterator> reverse_iterator;
	typedef std::reverse_iterator<const_iterator> const_reverse_iterator;

	struct Node
	{
		T*	m_prev;
		T*	m_next;

		Node()
			: m_prev((T*)this), m_next((T*)this)
		{
		}

		Node(const Node&) = delete;
		Node(Node&&) = delete;
		Node& operator=(const Node&) = delete;
		Node& operator=(Node&&) = delete;

		Node(Node* prev, Node* next)
			: m_prev((T*)prev), m_next((T*)next)
		{
		}

		void link(Node* position)
		{
			ABORT_IF(this == position);
			ABORT_IF(this->m_prev != this->m_next || this->m_prev != (T*)this);
			std::swap(this->m_prev, position->m_prev);
			std::swap(this->m_prev->m_next, this->m_next);
		}

		void unlink()
		{
			auto* next = m_next;
			auto* prev = m_prev;
			prev->m_next = next;
			next->m_prev = prev;
			m_prev = m_next = (T*)this;
		}

		~Node()
		{
			unlink();
		}
	};

	Node	m_head;
	Node	m_tail;

	IntrusiveList()
		: m_head(&m_head, &m_tail), m_tail(&m_head, &m_tail)
	{
	}

	~IntrusiveList()
	{
#if defined(MEMORY_DEBUG)
		ABORT_IF(!empty());
#endif
	}

	bool empty() const { return m_head.m_next == &m_tail; }
	void pop_front()
	{
		remove(m_head.m_next);
	}
	void pop_back()
	{
		remove(m_tail.m_prev);
	}
	void push_front(Node* node)
	{
		ABORT_IF(node->m_prev != node->m_next || node->m_prev != node);
		std::swap(m_head.m_next, node->m_next);
		std::swap(node->m_next->m_prev, node->m_prev);
	}
	void push_back(Node* node)
	{
		ABORT_IF(node->m_prev != node->m_next || node->m_prev != node);
		std::swap(m_tail.m_prev, node->m_prev);
		std::swap(node->m_prev->m_next, node->m_next);
	}
	void insert(Node* position, Node* node)
	{
		node->link(position);
	}
	void remove(Node* node)
	{
		node->unlink();
	}

	T* front() { return m_head.m_next; }
	T* back() { return m_tail.m_prev; }
	const T* front() const { return m_head.m_next; }
	const T* back() const { return m_tail.m_prev; }

	iterator begin() { return iterator(m_head.m_next); }
	const_iterator begin() const { return const_iterator(m_head.m_next); }
	const_iterator cbegin() const { return const_iterator(m_head.m_next); }

	iterator end() { return iterator(&m_tail); }
	const_iterator end() const { return const_iterator(&m_tail); }
	const_iterator cend() const { return const_iterator(&m_tail); }

	reverse_iterator rbegin() { return reverse_iterator(this->end()); }
	reverse_iterator rend() { return reverse_iterator(this->begin()); }
	reverse_iterator crbegin() const { return const_reverse_iterator(this->cend()); }
	reverse_iterator crend() const { return const_reverse_iterator(this->cbegin()); }
};

template<typename T>
class IntrusiveList<T>::iterator : public std::iterator<std::bidirectional_iterator_tag, typename T>
{
public:
	typedef typename T value_type;
	typedef typename IntrusiveList<T> list_type;
	typedef typename list_type::Node node_type;
	typedef typename list_type::iterator self_type;

private:
	node_type* m_node;

public:
	iterator(node_type* node) : m_node(node) {}
	iterator(const self_type& rhs) : m_node(rhs.m_node) {}
	iterator& operator=(const self_type& rhs) { m_node = rhs.m_node; return *this; }
	iterator& operator++() { m_node = m_node->m_next; return *this; }
	iterator& operator--() { m_node = m_node->m_prev; return *this; }

	bool operator == (const self_type& rhs) const { return rhs.m_node == m_node; }
	bool operator != (const self_type& rhs) const { return rhs.m_node != m_node; }

	operator node_type* () { return m_node; }
	operator const node_type* () const { return m_node; }
	operator value_type* () { return (value_type*)m_node; }
	operator const value_type* () const { return (value_type*)m_node; }
	value_type& operator*() { return *(value_type*)m_node; }
	value_type* operator->() { return (value_type*)m_node; }
	const value_type& operator*() const { return *(value_type*)m_node; }
	const value_type* operator->() const { return (value_type*)m_node; }
};

template<typename T>
class IntrusiveList<T>::const_iterator : public std::iterator<std::bidirectional_iterator_tag, typename T>
{
public:
	typedef typename T value_type;
	typedef typename IntrusiveList<T> list_type;
	typedef typename list_type::Node node_type;
	typedef typename list_type::const_iterator self_type;
	typedef typename list_type::iterator mutable_iterator;

private:
	node_type* m_node;

public:
	const_iterator(const node_type* node) : m_node(node) {}
	const_iterator(const self_type& rhs) : m_node(rhs) {}
	const_iterator(const mutable_iterator& rhs) : m_node(rhs) {}
	const_iterator& operator=(const self_type& rhs) { m_node = rhs; return *this; }
	const_iterator& operator=(const mutable_iterator& rhs) { m_node = rhs; return *this; }
	const_iterator& operator++() { m_node = m_node->m_next; return *this; }
	const_iterator& operator--() { m_node = m_node->m_prev; return *this; }

	bool operator == (const self_type& rhs) const { return rhs.m_node == m_node; }
	bool operator != (const self_type& rhs) const { return rhs.m_node != m_node; }
	bool operator == (const mutable_iterator& rhs) const { return rhs.m_node == m_node; }
	bool operator != (const mutable_iterator& rhs) const { return rhs.m_node != m_node; }

	operator const node_type* () const { return m_node; }
	operator const value_type* () const { return (value_type*)m_node; }
	const T& operator*() const { return *(value_type*)m_node; }
	const T* operator->() const { return (value_type*)m_node; }
};

