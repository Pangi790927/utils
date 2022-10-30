#include "gheap.h"

template <typename T>
struct heap_ctx_t {
    using I = size_t;
    using U = T;

    T&   objref(I i)                            { return vec[i]; }
    bool cmp_fn(const T& a, const T& b) const   { return a < b; }
    I    get_sz_fn()                    const   { return vec.size(); }
    void resize_fn(I sz)                        { vec.resize(sz); }

private:
    std::vector<T> vec;
};

void print_heap(auto& heap) {
	heap.iter([](int &elem, size_t i, int lvl, void *){
		std::string padd = std::string(lvl, '.');
		padd += "| ";
		DBG("heap_item[%2ld] %s %d", i, padd.c_str(), elem);
	}, NULL);
}

int main(int argc, char const *argv[])
{
	DBG_SCOPE();

	using heap_t = generic_heap_t<heap_ctx_t<int>>;
	heap_t heap;

	heap.insert(2);
	heap.insert(13);
	heap.insert(4);
	heap.insert(-1);
	heap.insert(3);
	heap.insert(23);
	heap.insert(24);
	heap.insert(25);
	heap.insert(25);
	heap.insert(26);

	print_heap(heap);

	DBG("--");
	heap.pop();
	print_heap(heap);

	DBG("--");
	heap.pop();
	print_heap(heap);

	DBG("--");
	heap.pop();
	print_heap(heap);

	DBG("--");
	heap.pop();
	print_heap(heap);

	DBG("--");
	heap.pop();
	print_heap(heap);

	DBG("--");
	heap.pop();
	print_heap(heap);

	DBG("--");
	heap.pop();
	print_heap(heap);

	DBG("--");
	heap.pop();
	print_heap(heap);

	DBG("--");
	heap.pop();
	print_heap(heap);

	DBG("--");
	heap.pop();
	print_heap(heap);

	DBG("--");
	heap.pop();
	print_heap(heap);

	return 0;
}