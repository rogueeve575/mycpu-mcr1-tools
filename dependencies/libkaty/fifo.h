
#pragma once

#define KT_FIFO_DEFAULT_CAPACITY	256

template<typename T> class FIFO {
public:
	FIFO(int capacity = KT_FIFO_DEFAULT_CAPACITY) {
		_capacity = capacity + 1;
		buffer = (T*)malloc(sizeof(T) * _capacity);
		head = tail = 0;
	}
	~FIFO() {
		free(buffer);
	}
	
	void push(T object) {
		buffer[tail] = object;
		if (++tail >= _capacity) tail = 0;
	}
	
	bool empty() {
		return (head == tail);
	}
	bool full() {
		int nexttail = tail + 1;
		if (nexttail >= _capacity) nexttail = 0;
		return (nexttail == head);
	}
	
	T read() {
		T item = buffer[head];
		if (++head >= _capacity) head = 0;
		return item;
	}
	
	int size() {
		if (head > tail)
			return (_capacity - head) + tail;
		else if (head < tail)
			return (tail - head);
		else
			return 0;
	}
	
	int capacity() {
		return _capacity - 1;
	}
	void clear() {
		head = tail = 0;
	}
	
private:
	T *buffer;
	int head, tail;
	int _capacity;
};


#if __cplusplus < 201103L
#define auto listed_fifo<T>
#endif
template<typename T> class InfiniteFIFO {
private:
	template<typename T2> struct listed_fifo {
		listed_fifo(int capacity) : fifo(capacity) { }
		FIFO<T2> fifo;
		listed_fifo<T2> *next;
	};
public:
	InfiniteFIFO(int capacity = KT_FIFO_DEFAULT_CAPACITY) {
		_capacity = capacity;
		first = last = new listed_fifo<T>(capacity);
		last->next = NULL;
	}
	
	void push(T object) {
		if (last->fifo.full()) {
			auto *entry = new listed_fifo<T>(_capacity);
			last->next = entry;
			entry->next = NULL;
			last = entry;
		}
		
		last->fifo.push(object);
	}
	
	T read() {
		T item = first->fifo.read();
		if (first != last && first->fifo.empty())
			first = first->next;
		return item;
	}
	
	bool empty() {
		return (first == last && first->fifo.empty());
	}
	
	void clear() {
		while(first != last) {
			auto *tmp = first;
			first = first->next;
			delete tmp;
		}
		
		last->fifo.clear();
	}
	
	int size() {
		int count = 0;
		auto *cur = first;
		while(cur != last) {
			count += cur->fifo.size();
			cur = cur->next;
		}
		
		return count + last->fifo.size();
	}
	
private:
	listed_fifo<T> *first, *last;
	int _capacity;
};
#if __cplusplus < 201103L
#undef auto
#endif

#undef KT_FIFO_DEFAULT_CAPACITY
