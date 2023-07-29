#ifndef FILE_STORAGE_H
#define FILE_STORAGE_H

#include <string>
#include <cstdint>          //for uint32_t
#include <cstdio>
#include "SipTime.h"
#include "alibabacloud/oss/OssClient.h"

class FileStorage {
public:
	virtual ~FileStorage();
	FileStorage();
	virtual bool open_file(const std::string&) = 0;
	virtual bool write_data(const std::string& data) = 0;
	virtual bool file_seek(int, long) = 0;
	virtual void close_file() = 0;
};

class LocalStorage : public FileStorage {
public:
	~LocalStorage();
	LocalStorage();
	bool open_file(const std::string&) override;
	bool write_data(const std::string& data) override;
	bool file_seek(int, long) override;
	void close_file() override;

private:
	FILE* fp_;
};

/*ali oss*/
class OssStorage : public FileStorage {
public:
	~OssStorage();
	OssStorage();
	bool open_file(const std::string&) override;
	bool write_data(const std::string& data) override;
	bool file_seek(int, long) override;
	void close_file() override;

private:
	std::string AccessKeyId_;
	std::string AccessKeySecret_;
	std::string Endpoint_;
	std::string BucketName_;
	std::string BasePath_;
	std::string filename_;
	AlibabaCloud::OSS::ClientConfiguration conf_;
	AlibabaCloud::OSS::OssClient client_;
	/*the pos where to append*/
	int pos_;
};

#endif