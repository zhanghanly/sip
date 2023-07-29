#ifndef PUBLIC_FUNC_H
#define PUBLIC_FUNC_H

#include <string>
#include <vector>
#include <cstdint>

std::vector<std::string> string_split(std::string s, std::string seperator);

bool string_contains(std::string str, std::string flag);

bool string_contains(std::string str, std::string flag0, std::string flag1);

bool string_contains(std::string str, std::string flag0, std::string flag1, std::string flag2);

std::string string_replace(std::string str, std::string old_str, std::string new_str);

std::string string_trim_start(std::string str, std::string trim_chars);

uint32_t generate_ssrc(const std::string&);

void remove_spec_char(std::string&, char);

int64_t max(int64_t, int64_t);

std::string generate_random_str(int);

bool check_digit_validity(const std::string&); 

bool open_dir(const std::string&);

void signal_handler(int sig);

void log_init(void);

template<typename T>
bool need_reverse(const T& data) {
	return sizeof(T) != sizeof(char);
}

template<typename T>
T reverse_value(const T& data) {
	T tmp;

	for (int i = 0; i < sizeof(T); i++) {
		*((char*)(&tmp) + (sizeof(T) -i - 1)) = *((char*)(&data) + i); 
	}

	return tmp;
}

#endif