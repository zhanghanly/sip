#ifndef DATABASE_HANDLER_H
#define DATABASE_HANDLER_H

#include <string>
#include <hiredis.h>
#include <vector>
#include <map>
#include <list>
#include <mysql/mysql.h>
#include <cstdarg>

/*the type of return value*/
enum class ResultType {
	VOID,
	STRING,
	INTEGER,
	KEY_VALUE
};    

struct ReplyValue {
	ResultType type;
	bool success;
	std::string str_result;
	int int_result;
	std::map<std::string, std::string> kv_result;
};

class DataBaseHandler {
public:
    virtual ~DataBaseHandler();
    /*connect*/    
    virtual void connect(void) = 0;
    /*write data to database*/
    virtual ReplyValue write_data(const std::string&, ...) = 0;
    virtual void close(void) = 0;
};

class RedisHandler : public DataBaseHandler {
public:
    ~RedisHandler();
    void connect() override;
    ReplyValue write_data(const std::string&, ...) override;
    void close(void) override;

private:
    redisContext* redis_ctx_;
};


#endif
