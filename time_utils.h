#ifndef TIME_UTILS_H
#define TIME_UTILS_H

#include <thread>

inline void sleep_us(uint64_t nr) {
	std::this_thread::sleep_for(std::chrono::microseconds(nr));
}

inline void sleep_ms(uint64_t nr) {
	std::this_thread::sleep_for(std::chrono::milliseconds(nr));
}

inline void sleep_s(uint64_t nr) {
	std::this_thread::sleep_for(std::chrono::seconds(nr));
}

inline uint64_t get_time_s() {
	using namespace std::chrono;
	return duration_cast<seconds>(
			system_clock::now().time_since_epoch()).count();
}


inline uint64_t get_time_ms() {
	using namespace std::chrono;
	return duration_cast<milliseconds>(
			system_clock::now().time_since_epoch()).count();
}

inline uint64_t get_time_us() {
	using namespace std::chrono;
	return duration_cast<microseconds>(
			system_clock::now().time_since_epoch()).count();
}

struct tval_base_t {
    tval_base_t() { time_ns = 0; }
    tval_base_t(const tval_base_t& tv) : time_ns(tv.time_ns) {}

    void set_s (uint64_t  s) { time_ns =  s * 1000'000'000; };
    void set_ms(uint64_t ms) { time_ns = ms * 1000'000; };
    void set_us(uint64_t us) { time_ns = us * 1000; };
    void set_ns(uint64_t ns) { time_ns = ns * 1; };

    uint64_t s()  const { return time_ns / 1000'000'000; }
    uint64_t ms() const { return time_ns / 1000'000; };
    uint64_t us() const { return time_ns / 1000; };
    uint64_t ns() const { return time_ns / 1; };

private:
    uint64_t time_ns;
};

struct TVAL_S  {};
struct TVAL_MS {};
struct TVAL_US {};
struct TVAL_NS {};

template <typename T>
concept TimeType =
           std::is_same_v<T, void>
        || std::is_same_v<T, TVAL_NS>
        || std::is_same_v<T, TVAL_US>
        || std::is_same_v<T, TVAL_MS>
        || std::is_same_v<T, TVAL_S>;

template <TimeType T = void>
struct tval_t : public tval_base_t {};

template <>
struct tval_t<void> : public tval_base_t {
    tval_t() : tval_base_t() {}

    template <TimeType U>
    tval_t(tval_t<U> tv) { set_ns(tv.ns()); }
};

template <>
struct tval_t<TVAL_NS> : public tval_base_t {
    tval_t(uint64_t ns) { set_ns(ns); }

    template <TimeType U>
    tval_t(tval_t<U> tv) { set_ns(tv.ns()); }
  
    uint64_t get() { return ns(); }
};

template <>
struct tval_t<TVAL_US> : public tval_base_t {
    tval_t(uint64_t us) { set_us(us); }

    template <TimeType U>
    tval_t(tval_t<U> tv) { set_ns(tv.ns()); }

    uint64_t get() { return us(); }
};

template <>
struct tval_t<TVAL_MS> : public tval_base_t {
    tval_t(uint64_t ms) { set_ms(ms); }

    template <TimeType U>
    tval_t(tval_t<U> tv) { set_ns(tv.ns()); }

    uint64_t get() { return ms(); }
};

template <>
struct tval_t<TVAL_S> : public tval_base_t {
    tval_t(uint64_t s) { set_s(s); }

    template <TimeType U>
    tval_t(tval_t<U> tv) { set_ns(tv.ns()); }
  
    uint64_t get() { return s(); }
};

template <TimeType T>
tval_t<T> operator * (uint64_t a, const tval_t<T>& b) {
    return tval_t<TVAL_NS>(a * b.ns());
}

template <TimeType T>
tval_t<T> operator * (const tval_t<T>& a, uint64_t b) {
    return tval_t<TVAL_NS>(a.ns() * b);
}

template <TimeType T, TimeType U>
tval_t<void> operator* (const tval_t<T>& a, const tval_t<U>& b) {
    return tval_t<TVAL_NS>(a.ns() * b.ns());
}

template <TimeType T>
tval_t<T> operator / (const tval_t<T>& a, uint64_t b) {
    return tval_t<TVAL_NS>(a.ns() / b);
}

template <TimeType T, TimeType U>
tval_t<void> operator / (const tval_t<T>& a, const tval_t<U>& b) {
    return tval_t<TVAL_NS>(a.ns() / b.ns());
}

/* TODO: understand +/- integer for concrete types */
template <TimeType T, TimeType U>
tval_t<T> &operator += (tval_t<T>& a, const tval_t<U>& b) {
    a.set_ns(a.ns() + b.ns());
    return a;
}

template <TimeType T, TimeType U>
tval_t<T> &operator -= (tval_t<T>& a, const tval_t<U>& b) {
    a.set_ns(a.ns() - b.ns());
    return a;
}

template <TimeType T, TimeType U>
tval_t<T> &operator *= (tval_t<T>& a, const tval_t<U>& b) {
    a.set_ns(a.ns() * b.ns());
    return a;
}

template <TimeType T, TimeType U>
tval_t<T> &operator *= (tval_t<T>& a, uint64_t b) {
    a.set_ns(a.ns() * b);
    return a;
}

template <TimeType T, TimeType U>
tval_t<T> &operator /= (tval_t<T>& a, const tval_t<U>& b) {
    a.set_ns(a.ns() / b.ns());
    return a;
}

template <TimeType T, TimeType U>
tval_t<T> &operator /= (tval_t<T>& a, uint64_t b) {
    a.set_ns(a.ns() / b);
    return a;
}

template <TimeType T, TimeType U>
auto operator <=> (const tval_t<T>& a, const tval_t<U>& b) {
    return a.ns() <=> b.ns();
}


inline tval_t<TVAL_NS> operator "" _ns (unsigned long long int a) { return tval_t<TVAL_NS>(a); }
inline tval_t<TVAL_US> operator "" _us (unsigned long long int a) { return a * 1000_ns; }
inline tval_t<TVAL_MS> operator "" _ms (unsigned long long int a) { return a * 1000_us; }
inline tval_t<TVAL_S > operator "" _s  (unsigned long long int a) { return a * 1000_ms; }

using sec_t = tval_t<TVAL_S>;
using ms_t = tval_t<TVAL_MS>;
using us_t = tval_t<TVAL_US>;
using ns_t = tval_t<TVAL_NS>;



#endif
