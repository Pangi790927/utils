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


#endif
