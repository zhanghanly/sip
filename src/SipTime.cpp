#include <sstream>
#include <sys/time.h>
#include "SipTime.h"
#include "PublicFunc.h"

static std::string make_tm_str(std::tm* st) {
	std::stringstream ss;
	if (st) {
		ss << st->tm_year + 1900 
		   << "-"
		   << (((st->tm_mon + 1) >= 10) ? "" : "0")
		   << st->tm_mon + 1 
		   << "-"
		   << ((st->tm_mday >= 10) ? "" : "0")
		   << st->tm_mday
		   << "T" 
		   << ((st->tm_hour >= 10) ? "" : "0")
		   << st->tm_hour
		   << ":"
		   << ((st->tm_min >= 10) ? "" : "0")
		   << st->tm_min
		   << ":"
		   << ((st->tm_sec >= 10) ? "" : "0")
		   << st->tm_sec;
	}
	return ss.str();
}

std::time_t get_system_timestamp(void) {
	/*clock time*/
	timeval tv;
	if (gettimeofday(&tv, NULL) == -1) {
		return 0;
	}
	return tv.tv_sec;
}

std::string get_utc_time(void) {
	std::time_t now = get_system_timestamp();
	return get_utc_time_by_ts(now);
}

std::string get_utc_time_by_ts(std::time_t time) {
	/*to calendar time*/
	std::tm* st  = gmtime(&time);
	if (!st) {
		return "";
	}
	return make_tm_str(st);
}

std::string get_iso_time(void) {
	std::time_t now = get_system_timestamp();
	return get_iso_time_by_ts(now);
}

std::string get_iso_time_by_ts(std::time_t time) {
	/*beijing time(fast 8 hours than utc time)*/
	std::tm* st = std::localtime(&time);
	/*Date: 2020-03-21T14:20:57*/
	if (!st) {
		return "";
	}
	return make_tm_str(st);
}

std::time_t string_to_timestamp(std::string str) {
	std::string resplace_empty_str = string_replace(str, " ", "-");     
	resplace_empty_str = string_replace(resplace_empty_str, ":", "-");
	resplace_empty_str = string_replace(resplace_empty_str, "T", "-");
	if (resplace_empty_str.empty()) {
		return 0;
	}
	std::vector<std::string> vec = string_split(resplace_empty_str, "-"); 
	if (vec.size() != 6) {
		return 0;
	}
	for (auto item : vec) {
		if (!check_digit_validity(item)) {
			return 0;
		}
	}
	std::tm tm_;
	tm_.tm_year = std::stoi(vec[0]) - 1900;
	tm_.tm_mon = std::stoi(vec[1]) - 1;
	tm_.tm_mday = std::stoi(vec[2]);
	tm_.tm_hour = std::stoi(vec[3]);
	tm_.tm_min = std::stoi(vec[4]);
	tm_.tm_sec = std::stoi(vec[5]);
	tm_.tm_isdst = 0;

	return mktime(&tm_);
}