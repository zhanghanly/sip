#include <sstream>
#include <cstring>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/io.h>
#include "FileStorage.h"
#include "alibabacloud/oss/OssClient.h"
#include "ServerConfig.h"
#include "spdlog/spdlog.h"

using namespace AlibabaCloud::OSS;

FileStorage::~FileStorage() {
}

FileStorage::FileStorage() {
}

LocalStorage::~LocalStorage() {
}

LocalStorage::LocalStorage() : fp_(nullptr) {
}

bool LocalStorage::open_file(const std::string& name) {
	if (fp_) {
		fflush(fp_);
		fclose(fp_);
	}
	fp_ = fopen(name.c_str(), "w");
	if (!fp_) {
		return false;
	}
	return true;
}

bool LocalStorage::write_data(const std::string& data) {
	if (fp_) {
		std::size_t write_bytes = fwrite(data.c_str(), 1, data.size(), fp_);	
		if (write_bytes != data.size()) {
			/*maybe disk is full*/
			return false;
		}
	}
	return true;
}

bool LocalStorage::file_seek(int whence, long pos) {
	int ret_val = fseek(fp_, pos, whence);
	return ret_val == 0;
}

void LocalStorage::close_file() {
	if (fp_) {
		fflush(fp_);
		fclose(fp_);
		fp_ = nullptr;
	}
}

OssStorage::~OssStorage() {
}

OssStorage::OssStorage() : AccessKeyId_(ServerConfig::get_instance()->fileStorage_accessKeyId()), 
						   AccessKeySecret_(ServerConfig::get_instance()->fileStorage_accessKeySecret()), 
						   Endpoint_(ServerConfig::get_instance()->fileStorage_endpoint()), 
						   BucketName_(ServerConfig::get_instance()->fileStorage_bucketName()), 
						   BasePath_(ServerConfig::get_instance()->fileStorage_basePath()),
						   client_(Endpoint_, AccessKeyId_, AccessKeySecret_, conf_),
						   pos_(0){
	/*init network*/
	InitializeSdk();
	auto meta = ObjectMetaData();
    meta.setContentType("text/plain");
}

bool OssStorage::open_file(const std::string& name) {
	filename_ = name;
	return true;  
}

bool OssStorage::write_data(const std::string& data) {
	std::shared_ptr<std::iostream> content = std::make_shared<std::stringstream>();
	/*write by binary*/
	content->write(data.c_str(), data.size());
	AppendObjectRequest request(BucketName_, filename_, content);
	request.setPosition(pos_);

	/*upload file*/
	auto outcome = client_.AppendObject(request);
	if (!outcome.isSuccess()) {
		std::stringstream ss;
		/*deal eception*/
		ss << "PutObject fail" <<
		",code:" << outcome.error().Code() <<
		",message:" << outcome.error().Message() <<
		",requestId:" << outcome.error().RequestId() << std::endl;
		
		spdlog::error(ss.str());
		
		ShutdownSdk();
		return false;
	}
	pos_ = outcome.result().Length();
	return true;
}

bool OssStorage::file_seek(int whence, long pos) {
	return true;
}

void OssStorage::close_file() {
	ShutdownSdk();
}