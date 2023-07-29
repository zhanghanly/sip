#include <sys/types.h>   //get_pid
#include <unistd.h>
#include <random>
#include <sys/stat.h>
#include "PublicFunc.h"
#include "spdlog/spdlog.h"
#include "spdlog/sinks/basic_file_sink.h"
#include "spdlog/async.h"
#include "spdlog/sinks/daily_file_sink.h"
#include "spdlog/sinks/rotating_file_sink.h"

std::vector<std::string> string_split(std::string s, std::string seperator) {   
	std::vector<std::string> result;
	if(seperator.empty()) {
		result.push_back(s);
		return result;
	}
	size_t posBegin = 0;
	size_t posSeperator = s.find(seperator);
	while (posSeperator != std::string::npos) {
		result.push_back(s.substr(posBegin, posSeperator - posBegin));
		posBegin = posSeperator + seperator.length(); // next byte of seperator
		posSeperator = s.find(seperator, posBegin);
	}
	/*push the last element*/
	result.push_back(s.substr(posBegin));

	if (result[0].empty()) {
		result.erase(result.begin());
	}
	return result;
}

bool string_contains(std::string str, std::string flag) {
	return str.find(flag) != std::string::npos;
}

bool string_contains(std::string str, std::string flag0, std::string flag1) {
	return str.find(flag0) != std::string::npos || 
	       str.find(flag1) != std::string::npos;
}

bool string_contains(std::string str, std::string flag0, std::string flag1, std::string flag2) {
    return str.find(flag0) != std::string::npos || 
	       str.find(flag1) != std::string::npos || 
		   str.find(flag2) != std::string::npos;
}

std::string string_replace(std::string str, std::string old_str, std::string new_str) {
	std::string ret = str;
	if (old_str == new_str) {
		return ret;
	}
	size_t pos = 0;
	while ((pos = ret.find(old_str, pos)) != std::string::npos) {
		ret = ret.replace(pos, old_str.length(), new_str);
	}

	return ret;
}

std::string string_trim_start(std::string str, std::string trim_chars) {
	std::string ret = str;
	for (int i = 0; i < (int)trim_chars.length(); i++) {
		char ch = trim_chars.at(i);

		while (!ret.empty() && ret.at(0) == ch) {
			ret.erase(ret.begin());

			// ok, matched, should reset the search
			i = -1;
		}
	}

    return ret;
}

uint32_t generate_ssrc(const std::string& deviceid) {
	static uint32_t bak_ssrc = 100000000;
	if (deviceid.size() < 9 ) {
		return bak_ssrc++;
	}
	std::string tmp_deviceid = deviceid;
	auto iter = tmp_deviceid.begin();
	tmp_deviceid.erase(iter, iter + (deviceid.size()-9));

	return std::stoi(tmp_deviceid);
}

void remove_spec_char(std::string& str, char ch) {
	if (!str.empty()) {
		for (auto iter = str.begin(); iter != str.end();){
			if (*iter == ch) {
				iter = str.erase(iter);
				continue;
			}
			iter++;
		}
	}
}

int64_t max(int64_t a, int64_t b) {
	return a > b ? a : b; 
}

std::string generate_random_str(int length) {
	std::string alphabet = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ";
	std::random_device rd;
	std::default_random_engine eng(rd());
	std::uniform_int_distribution<int> distr(1, 1000000);

	std::string random_str;
	for (int n = 0; n < length; n++) { 
			int pos = distr(eng) % alphabet.size();
			random_str.push_back(alphabet[pos]);
	}

	return random_str;
}

bool check_digit_validity(const std::string& digit) {
	for (auto iter = digit.begin(); iter != digit.end(); iter++) {
		if (*iter < '0' || *iter > '9') {
			return false;
		}
	}
	return true;
}

bool open_dir(const std::string& file_path) {
	std::vector<std::string> vec = string_split(file_path, "/");
	/*delete file name*/
	vec.pop_back();
	std::string next_path;
	/*absolute path*/
	if (file_path[0] == '/') {
		next_path.push_back('/');
	}	
	/*create dir*/
	for (auto iter = vec.begin(); iter != vec.end(); iter++) {
		next_path += *iter;
		/*is dir exist*/
		if (!access(next_path.c_str(), 0)) {
			/*continue;*/
		} else {
			if (mkdir(next_path.c_str(), 0777)) {
				return false;
			}
		}
		next_path.push_back('/');
	}

	return true;
}

void signal_handler(int sig) {
	spdlog::error("recieve a signal={}", sig);
}

void log_init(void) {
	/*configre log*/
	std::shared_ptr<spdlog::sinks::daily_file_sink_mt> info_sink; // info
	info_sink = std::make_shared<spdlog::sinks::daily_file_sink_mt>("../log/sip.log", 5, 0);
	info_sink->set_level(spdlog::level::info); // debug< info< warn< error< critical 
	info_sink->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%n] [%l] %v");
	spdlog::flush_every(std::chrono::seconds(10));
	std::shared_ptr<spdlog::logger> logger;
	logger = std::shared_ptr<spdlog::logger>(new spdlog::logger("multi_sink", {info_sink}));
	spdlog::set_default_logger(logger);
}