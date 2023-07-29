#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <map>
#include <sys/time.h>
#include <iconv.h>
#include "SipStack.h"
#include "PublicFunc.h"
#include "spdlog/spdlog.h"
#include "SipTime.h"
#include "md5.h"
#include "ServerConfig.h"

namespace sip {

using namespace std;

/* Code (GB2312 <--> UTF-8) Convert Class */
class CodeConverter {
private:
    iconv_t cd;
public:
    CodeConverter(const char *pFromCharset, const char *pToCharset)
    {
        cd = iconv_open(pToCharset, pFromCharset);
        if ((iconv_t)(-1) == cd)
            spdlog::warn("CodeConverter: iconv_open failed <{} --> {}>", pFromCharset, pToCharset);
    }
    ~CodeConverter()
    {
        if ((iconv_t)(-1) != cd)
            iconv_close(cd);
    }
    int Convert(char *pInBuf, size_t InLen, char *pOutBuf, size_t OutLen)
    {
        return iconv(cd, &pInBuf, (size_t *)&InLen, &pOutBuf, (size_t *)&OutLen);
    }
    static bool IsUTF8(const char *pInBuf, int InLen)
    {
        if (InLen < 0) {
            return false;
        }

        int i = 0;
        int nBytes = 0;
        unsigned char chr = 0;

        while (i < InLen) {
            chr = *(pInBuf + i);
            if (nBytes == 0) {
                if ((chr & 0x80) != 0) {
                    while ((chr & 0x80) != 0) {
                        chr <<= 1;
                        nBytes++;
                    }
                    if (nBytes < 2 || nBytes > 6) {
                        return false;
                    }
                    nBytes--;
                }
            } else {
                if ((chr & 0xc0) != 0x80) {
                    return false;
                }
                nBytes--;
            }
            ++i;
        }

        return nBytes == 0;
    }
};

unsigned int sip_random(int min,int max) {  
    //it is possible to duplicate data with time(0)
    srand(unsigned(get_system_timestamp()));
    return  rand() % (max - min + 1) + min;
} 

std::string sip_generate_branch() {
   int rand = sip_random(10000000, 99999999);
   std::stringstream branch; 
   branch << "SipGbB" << rand;
   return branch.str();
}

std::string sip_generate_to_tag() {
   uint32_t rand = sip_random(10000000, 99999999);
   std::stringstream branch; 
   branch << "SipGbT" << rand;
   return branch.str();
}

std::string sip_generate_from_tag() {
   uint32_t rand = sip_random(10000000, 99999999);
   std::stringstream branch; 
   branch << "SipGbF" << rand;
   return branch.str();
}

std::string sip_generate_call_id() {
   uint32_t rand = sip_random(10000000, 99999999);
   std::stringstream branch; 
   branch << "2020" << rand;
   return branch.str();
}

std::string sip_generate_sn() {
   uint32_t rand = sip_random(10000000, 99999999);
   std::stringstream sn; 
   sn << rand;
   return sn.str();
}

std::string  sip_get_form_to_uri(std::string  msg) {
    //<sip:34020000002000000001@3402000000>;tag=536961166
    //sip:34020000002000000001@3402000000 
    size_t pos = msg.find("<");
    if (pos == string::npos) {
        return msg;
    }
    msg = msg.substr(pos+1);
    size_t pos2 = msg.find(">");
    if (pos2 == string::npos) {
        return msg;
    }
    msg = msg.substr(0, pos2);
    return msg;
}

std::string sip_get_utc_date() {
    /*clock time*/
    timeval tv;
    if (gettimeofday(&tv, NULL) == -1) {
        return "";
    }
    /*to calendar time*/
    struct tm* tm;
	/*utc time*/
	//if ((tm = gmtime(&tv.tv_sec)) == NULL) {
	//	return "";
	//}
	/*beijing time(fast 8 hours than utc time)*/
	if ((tm = std::localtime(&tv.tv_sec)) == NULL) {
        return "";
    }
    /*Date: 2020-03-21T14:20:57.638*/
    std::string utc_date = "";
    char buffer[25] = {0};
    snprintf(buffer, 25,
             "%d-%02d-%02dT%02d:%02d:%02d.%03d",
             1900 + tm->tm_year, 1 + tm->tm_mon, 
			 tm->tm_mday, tm->tm_hour, tm->tm_min, 
			 tm->tm_sec, (int)(tv.tv_usec / 1000));
    utc_date = buffer;
    return utc_date;
}


std::string sip_get_param(std::string msg, std::string param) {
    std::vector<std::string>  vec_params = string_split(msg, ";");

    for (vector<string>::iterator it = vec_params.begin(); it != vec_params.end(); ++it) {
        string  value = *it;
        size_t pos = value.find(param);
        if (pos == string::npos) {
            continue;
        }
        std::vector<std::string>  v_pram = string_split(value, "=");
        
        if (v_pram.size() > 1) {
            return v_pram.at(1);
        }
    }
    return "";
}

SipRequest::SipRequest() {
    seq = 0;
    content_length = 0;
    /*sdp = NULL;
    transport = NULL;*/

    method = "";
    uri = "";;
    version = "";;
    seq = 0;
    content_type = "";
    content_length = 0;
    call_id = "";
    from = "";
    to = "";
    via = "";
    from_tag = "";
    to_tag = "";
    contact = "";
    user_agent = "";
    branch = "";
    status = "";
    expires = 3600;
    max_forwards = 70;
    www_authenticate = "";
    authorization = "";
    cmdtype = SipCmdRequest;

    host = "127.0.0.1";;
    host_port = 5060;

    serial = "";;
    realm = "";;

    sip_auth_id = "";
    sip_auth_pwd = "";
    sip_username = "";
    peer_ip = "";
    peer_port = 0;

    chid = "";

    from_realm = "";
    to_realm = "";
    y_ssrc = 0;
}

SipRequest::~SipRequest() {
}

bool SipRequest::is_register() {
    return method == SIP_METHOD_REGISTER;
}

bool SipRequest::is_invite() {
    return method == SIP_METHOD_INVITE;
}

bool SipRequest::is_ack() {
    return method == SIP_METHOD_ACK;
}

bool SipRequest::is_message() {
    return method == SIP_METHOD_MESSAGE;
}

bool SipRequest::is_bye() {
    return method == SIP_METHOD_BYE;
}

std::string SipRequest::get_cmdtype_str() {
    switch(cmdtype) {
        case SipCmdRequest:
            return "request";
        case SipCmdRespone:
            return "respone";
    }

    return "";
}

void SipRequest::copy(std::shared_ptr<SipRequest> src) {
    if (!src) {
        return;
    }
    method = src->method;
    uri = src->uri;
    version = src->version;
    seq = src->seq;
    content_type = src->content_type;
    content_length = src->content_length;
    call_id = src->call_id;
    from = src->from;
    to = src->to;
    via = src->via;
    from_tag = src->from_tag;
    to_tag = src->to_tag;
    contact = src->contact;
    user_agent = src->user_agent;
    branch = src->branch;
    status = src->status;
    expires = src->expires;
    max_forwards = src->max_forwards;
    www_authenticate = src->www_authenticate;
    authorization = src->authorization;
    cmdtype = src->cmdtype;

    host = src->host;
    host_port = src->host_port;

    serial = src->serial;
    realm = src->realm;
    
    sip_auth_id = src->sip_auth_id;
    sip_auth_pwd = src->sip_auth_pwd;
    sip_username = src->sip_username;
    peer_ip = src->peer_ip;
    peer_port = src->peer_port;

    chid = src->chid;

    xml_body_map = src->xml_body_map;
    device_list_map = src->device_list_map;

    from_realm = src->from_realm;
    to_realm  = src->to_realm;

    y_ssrc = src->y_ssrc;
}

SipStack::SipStack() {
}

SipStack::~SipStack() {
}

bool SipStack::parse_request(std::shared_ptr<SipRequest> req, const char* recv_msg, int nb_len) {
    bool err;
    
    if (!(err = do_parse_request(req, recv_msg))) {
        return false;
    }
    
    return true;
}

bool SipStack::parse_xml(std::string xml_msg, std::map<std::string, std::string> &json_map, 
						 std::vector<std::map<std::string, std::string> > &item_list) {
    /*
    <?xml version="1.0" encoding="gb2312"?>
    <Notify>
    <CmdType>Keepalive</CmdType>
    <SN>2034</SN>
    <DeviceID>34020000001110000001</DeviceID>
    <Status>OK</Status>
    <Info>
    <DeviceID>34020000001320000002</DeviceID>
    <DeviceID>34020000001320000003</DeviceID>
    <DeviceID>34020000001320000005</DeviceID>
    <DeviceID>34020000001320000006</DeviceID>
    <DeviceID>34020000001320000007</DeviceID>
    <DeviceID>34020000001320000008</DeviceID>
    </Info>
    </Notify>
    */
    if (CodeConverter::IsUTF8(xml_msg.c_str(), xml_msg.size()) == false) {
        char *outBuf = (char *)calloc(1, xml_msg.size() * 2);
        CodeConverter cc("gb2312", "utf-8");
        if (cc.Convert((char *)xml_msg.c_str(), xml_msg.size(), (char *)outBuf, xml_msg.size() * 2 - 1) != -1)
            xml_msg = string(outBuf);
        if (outBuf)
            free(outBuf);
    }

    const char* start = xml_msg.c_str();
    const char* end = start + xml_msg.size();
    char* p = (char*)start;
    char* value_start = NULL;
    std::string xml_header;
    int xml_layer = 0;
    int in_item_tag = 0;

    //std::map<string, string> json_map;
    std::map<int, string> json_key;
    std::map<std::string, std::string> one_item;
    while (p < end) {
        if (p[0] == '\n'){
            p +=1;
            value_start = NULL;
        } else if (p[0] == '\r' && p[1] == '\n') {
            p +=2;
            value_start = NULL;
        } else if (p[0] == '<' && p[1] == '/') { //</Notify> xml item end flag
            std::string value = "";
            if (value_start) {
                value = std::string(value_start, p-value_start);
            }
            
            //skip </
            p += 2;
            
            //</Notify> get Notify
            char *s = p;
            while (p[0] != '>') {p++;}
            std::string key(s, p-s);

            //<DeviceList Num="2"> get DeviceList
            std::vector<string> vec = string_split(key, " ");
            if (vec.empty()){
                spdlog::error("prase xml error");
                return false; 
            }

            key = vec.at(0);

            /*xml element to map
                <Notify>
                    <info>
                        <DeviceID>34020000001320000001</DeviceID>
                        <DeviceID>34020000001320000002</DeviceID>
                    </info>
                </Notify>
            to map is: Notify@Info@DeviceID:34020000001320000001,34020000001320000002
            */
           
            //get map key
            std::string mkey = "";
            for (int i = 0; i < xml_layer ; i++){
                if (mkey.empty()) {
                    mkey = json_key[i];
                }else{
                    mkey =  mkey + "@" + json_key[i];     
                }
            }
 
            //set map value
            if (!mkey.empty()){
                if (json_map.find(mkey) == json_map.end()){
                    json_map[mkey] = value;         
                }else{
                    json_map[mkey] = json_map[mkey] + ","+ value;
                }    
            }

            if (in_item_tag && value != "" && (xml_layer - in_item_tag) == 2) {
                // (xml_layer - in_item_tag) == 2, this condition filters all deeper tags under tag "Item"
                one_item[key] = value;
            }
          
            value_start = NULL;
            xml_layer--;

            if (json_key[xml_layer] == "Item") {
                in_item_tag = 0;
                // all items (DeviceList, Alarmstatus, RecordList) have "DeviceID"
                if (one_item.find("DeviceID") != one_item.end())
                    item_list.push_back(one_item);
            }
        } else if (p[0] == '<') { //<Notify>  xml item begin flag
            //skip <
            p +=1;

            //<Notify> get Notify
            char *s = p;
            while (p[0] != '>') {p++;}
            std::string key(s, p-s);

            if (string_contains(key, "?xml")){
                //xml header
                xml_header = key;
                json_map["XmlHeader"] = xml_header;
            }else {
                //<DeviceList Num="2"> get DeviceList
                std::vector<string> vec = string_split(key, " ");
                if (vec.empty()){
                    spdlog::error("prase xml error");
                    return false; 
                }

                key = vec.at(0);

                //key to map by xml_layer
                //<Notify>
                //  <info>
                //  </info>
                //</Notify>
                //json_key[0] = "Notify"
                //json_key[1] = "info"
                json_key[xml_layer] = key; 
                if (json_key[xml_layer] == "Item") {
                    // "Item" won't be the first layer (xml_layer is 0)
                    in_item_tag = xml_layer;
                    one_item.clear();
                }
                xml_layer++;  
            }

            p +=1;
            value_start = p;
        } else {
          p++;
        }
    }
    // std::map<string, string>::iterator it2;
    // for (it2 = json_map.begin(); it2 != json_map.end(); ++it2) {
    //     trace("========%s:%s", it2->first.c_str(), it2->second.c_str());
    // }

    return true;
}

bool SipStack::do_parse_request(std::shared_ptr<SipRequest> req, const char* recv_msg) {
    bool err;

    std::vector<std::string> header_body = string_split(recv_msg, SIP_RTSP_CRLFCRLF);
    if (header_body.empty()) {
        spdlog::error("parse reques message error");
        return false; 
    }

    std::string header = header_body.at(0);
    //Must be added SIP_RTSP_CRLFCRLF in order to handle the last line header
    header += SIP_RTSP_CRLFCRLF; 
    std::string body = "";

    if (header_body.size() > 1){
       // SIP_RTSP_CRLFCRLF may exist in content body (h3c)
       string recv_str(recv_msg);
       body = recv_str.substr(recv_str.find(SIP_RTSP_CRLFCRLF) + strlen(SIP_RTSP_CRLFCRLF));
       //body =  header_body.at(1);
    }
    //spdlog::info("sip: header={}\n", header);
    //spdlog::info("sip: body={}\n", body);
    // parse one by one.
    char* start = (char*)header.c_str();
    char* end = start + header.size();
    char* p = start;
    char* newline_start = start;
    std::string firstline = "";
    while (p < end) {
        if (p[0] == '\r' && p[1] == '\n') {
            p +=2;
            int linelen = (int)(p - newline_start);
            std::string oneline(newline_start, linelen);
            newline_start = p;

            if (firstline == "") {
                firstline = string_replace(oneline, "\r\n", "");
                //spdlog::info("sip: first line={}", firstline);
            } else {
                size_t pos = oneline.find(":");
                if (pos != string::npos) {
                    if (pos != 0) {
                        //ex: CSeq: 100 MESSAGE  header is 'CSeq:',content is '100 MESSAGE'
                        std::string head = oneline.substr(0, pos+1);
                        std::string content = oneline.substr(pos+1, oneline.length()-pos-1);
                        content = string_replace(content, "\r\n", "");
                        content = string_trim_start(content, " ");
                        char* phead = (char*)head.c_str();
                        
                        if (!strcasecmp(phead, "call-id:")) {
                            std::vector<std::string> vec_callid = string_split(content, " ");
                            req->call_id = vec_callid.empty() ? "" : vec_callid.at(0);
                        } 
                        else if (!strcasecmp(phead, "contact:")) {
                            req->contact = content;
                        } 
                        else if (!strcasecmp(phead, "content-encoding:")) {
                            spdlog::info("sip: message head {} content={}", phead, content);
                        } 
                        else if (!strcasecmp(phead, "content-length:")) {
                            req->content_length = strtoul(content.c_str(), NULL, 10);
                        } 
                        else if (!strcasecmp(phead, "content-type:")) {
                            req->content_type = content;
                        } 
                        else if (!strcasecmp(phead, "cseq:")) {
                            std::vector<std::string> vec_seq = string_split(content, " ");
                            std::string seq = vec_seq.empty() ? "" : vec_seq.at(0);
                            req->seq = strtoul(seq.c_str(), NULL, 10);
                            req->method = vec_seq.size() > 0 ? vec_seq.at(1) : "";
                        } 
                        else if (!strcasecmp(phead, "from:")) {
                            content = string_replace(content, "sip:", "");
                            req->from = sip_get_form_to_uri(content.c_str());
                            if (string_contains(content, "tag")) {
                                req->from_tag = sip_get_param(content.c_str(), "tag");
                            }
                            std::vector<std::string> vec = string_split(req->from, "@");
                            if (vec.size() > 1){
                                req->from_realm = vec.at(1);
                            }
                        } 
                        else if (!strcasecmp(phead, "to:")) {
                            content = string_replace(content, "sip:", "");
                            req->to = sip_get_form_to_uri(content.c_str());
                            if (string_contains(content, "tag")) {
                                req->to_tag = sip_get_param(content.c_str(), "tag");
                            }
                            std::vector<std::string> vec = string_split(req->to, "@");
                            if (vec.size() > 1){
                                req->to_realm = vec.at(1);
                            }
                        } 
                        else if (!strcasecmp(phead, "via:")) {
                            req->via = content;
                            req->branch = sip_get_param(content.c_str(), "branch");
                        } 
                        else if (!strcasecmp(phead, "expires:")){
                            req->expires = strtoul(content.c_str(), NULL, 10);
                        }
                        else if (!strcasecmp(phead, "user-agent:")){
                            req->user_agent = content;
                        } 
                        else if (!strcasecmp(phead, "max-forwards:")){
                            req->max_forwards = strtoul(content.c_str(), NULL, 10);
                        }
                        else if (!strcasecmp(phead, "www-authenticate:")){
                            req->www_authenticate = content;
                        } 
                        else if (!strcasecmp(phead, "authorization:")){
                            req->authorization = content;
                        } 
                        else {
                            //TODO: fixme
                            spdlog::warn("sip: unkonw message head {} content={}", phead, content);
                        }
                   }
                }
            }
        } else {
            p++;
        }
    }
   
    std::vector<std::string>  method_uri_ver = string_split(firstline, " ");

    if (method_uri_ver.empty() || method_uri_ver.size() < 3) {
        spdlog::error("parse request firstline is empty or less than 3 fields");
        return false;
    }
    //respone first line text:SIP/2.0 200 OK
    if (!strcasecmp(method_uri_ver.at(0).c_str(), "sip/2.0")) {
        req->cmdtype = SipCmdRespone;
        //req->method= vec_seq.at(1);
        req->status = method_uri_ver.size() > 1 ? method_uri_ver.at(1) : "";
        req->version = method_uri_ver.at(0);
        req->uri = req->from;

        vector<string> str = string_split(req->to, "@");
        std::string ss = str.empty() ? "" : str.at(0);
        req->sip_auth_id = string_replace(ss, "sip:", "");
  
    } else {//request first line text :MESSAGE sip:34020000002000000001@3402000000 SIP/2.0
        req->cmdtype = SipCmdRequest;
        req->method= method_uri_ver.at(0);
        req->uri = method_uri_ver.size() > 1 ? method_uri_ver.at(1) : "";
        req->version = method_uri_ver.size() > 2 ? method_uri_ver.at(2) : "";

        vector<string> str = string_split(req->from, "@");
        std::string ss = str.empty() ? "" : str.at(0);
        req->sip_auth_id = string_replace(ss, "sip:", "");
    }

    req->sip_username =  req->sip_auth_id;

    //Content-Type: Application/MANSCDP+xml
    if (!strcasecmp(req->content_type.c_str(),"application/manscdp+xml")) {
        //xml to map
        if ((err = parse_xml(body, req->xml_body_map, req->item_list)) != true) {
            spdlog::error("sip parse xml error");
            return false;
        };
        //Response Cmd
        if (req->xml_body_map.find("Response") != req->xml_body_map.end()){
            std::string cmdtype = req->xml_body_map["Response@CmdType"];
            if (cmdtype == "Catalog"){
                #if 0
                //Response@DeviceList@Item@DeviceID:3000001,3000002
                std::vector<std::string> vec_device_id = string_split(req->xml_body_map["Response@DeviceList@Item@DeviceID"], ",");
                //Response@DeviceList@Item@Status:ON,OFF
                std::vector<std::string> vec_device_status = string_split(req->xml_body_map["Response@DeviceList@Item@Status"], ",");
                 
                //map key:devicd_id value:status 
                for(int i=0 ; i< (int)vec_device_id.size(); i++){
                    std::string status = "";
                    if ((int)vec_device_status.size() > i) {
                        status = vec_device_status.at(i);
                    }
              
                    req->device_list_map[vec_device_id.at(i)] = status;
                }
                #endif
                for(int i=0 ; i< (int)req->item_list.size(); i++){
                    std::map<std::string, std::string> one_item = req->item_list.at(i);
                    std::string status;
                    if (one_item.find("Status") != one_item.end() && one_item.find("Name") != one_item.end()) {
                        status = one_item["Status"] + "," + one_item["Name"];
                    } else {
                        // if no Status, it's not a camera but a group
                        continue;
                    }
                    req->device_list_map[one_item["DeviceID"]] = status;
                }
            } else {
                //TODO: fixme
                spdlog::info("sip: Response cmdtype={} not processed", cmdtype);
            }
        } //Notify Cmd
        else if (req->xml_body_map.find("Notify") !=  req->xml_body_map.end()) {
            std::string cmdtype = req->xml_body_map["Notify@CmdType"];
            if (cmdtype == "Keepalive") {
                //TODO: ????
                std::vector<std::string> vec_device_id = string_split(req->xml_body_map["Notify@Info@DeviceID"], ",");
                for(int i=0; i< (int)vec_device_id.size(); i++) {
                    //req->device_list_map[vec_device_id.at(i)] = "OFF";
                }
            } else {
               //TODO: fixme
               spdlog::info("sip: Notify cmdtype={} not processed", cmdtype);
            }
        }// end if(req->xml_body_map)
    }//end if (!strcasecmp)
    else if (!strcasecmp(req->content_type.c_str(),"application/sdp")) {
        std::vector<std::string> sdp_lines = string_split(body.c_str(), SIP_RTSP_CRLF);
        for(int i=0 ; i< (int)sdp_lines.size(); i++) {
            if (!strncasecmp(sdp_lines.at(i).c_str(), "y=", 2)) {
                string yline = sdp_lines.at(i);
                string ssrc = yline.substr(2);
                req->y_ssrc = strtoul(ssrc.c_str(), NULL, 10);
                spdlog::info("gb28181: ssrc in y line is {}:{}", req->y_ssrc, req->y_ssrc);
                break;
            }
        }
    }
   
    //spdlog::info("sip: method={} uri={} version={} cmdtype={}", 
    //       req->method, req->uri, req->version, req->get_cmdtype_str());
    //spdlog::info("via={}", req->via);
    //spdlog::info("via_branch={}", req->branch);
    //spdlog::info("cseq={}", req->seq);
    //spdlog::info("contact={}", req->contact);
    //spdlog::info("from={}",  req->from);
    //spdlog::info("to={}",  req->to);
    //spdlog::info("callid={}", req->call_id);
    //spdlog::info("status={}", req->status);
    //spdlog::info("from_tag={}", req->from_tag);
    //spdlog::info("to_tag={}", req->to_tag);
    //spdlog::info("sip_auth_id={}", req->sip_auth_id);

    return true;
}

std::string SipStack::get_sip_from(std::shared_ptr<SipRequest> req) {
    std::string from_tag;
    if (req->from_tag.empty()) {
        from_tag = "";
    } else {
        from_tag = ";tag=" + req->from_tag;
    }

    return  "<sip:" + req->from + ">" + from_tag;
}

std::string SipStack::get_sip_to(std::shared_ptr<SipRequest> req) {
    std::string to_tag;
    if (req->to_tag.empty()) {
        to_tag = "";
    } else {
        to_tag = ";tag=" + req->to_tag;
    }

    return  "<sip:" + req->to + ">" + to_tag;
}

std::string SipStack::get_sip_via(std::shared_ptr<SipRequest> req) {
    std::string via = string_replace(req->via, SIP_VERSION"/UDP ", "");
    std::vector<std::string> vec_via = string_split(via, ";");

    std::string ip_port = vec_via.empty() ? "" : vec_via.at(0);
    std::vector<std::string> vec_ip_port = string_split(ip_port, ":");

    std::string ip = vec_ip_port.empty() ? "" : vec_ip_port.at(0);
    std::string port = vec_ip_port.size() > 1 ? vec_ip_port.at(1) : "";
    
    std::string branch, rport, received;
    if (req->branch.empty()){
        branch = "";
    } else {
        branch = ";branch=" + req->branch;
    }
    if (!req->peer_ip.empty()){
        ip = req->peer_ip;

        std::stringstream ss;
        ss << req->peer_port;
        port = ss.str();
    }

    received = ";received=" + ip;
    rport = ";rport=" + port;

    return SIP_VERSION"/UDP " + ip_port + rport + received + branch;
}

void SipStack::resp_keepalive(std::stringstream& ss, std::shared_ptr<SipRequest> req) {
    ss << SIP_VERSION <<" 200 OK" << SIP_RTSP_CRLF
       << "Via: " << SIP_VERSION << "/UDP " << req->host << ":" << req->host_port << ";branch=" << req->branch << SIP_RTSP_CRLF
       << "From: <sip:" << req->from.c_str() << ">;tag=" << req->from_tag << SIP_RTSP_CRLF
       << "To: <sip:"<< req->to.c_str() << ">\r\n"
       << "Call-ID: " << req->call_id << SIP_RTSP_CRLF
       << "CSeq: " << req->seq << " " << req->method << SIP_RTSP_CRLF
       << "Contact: "<< req->contact << SIP_RTSP_CRLF
       << "Max-Forwards: 70" << SIP_RTSP_CRLF
       << "User-Agent: "<< SIP_USER_AGENT << SIP_RTSP_CRLF
       << "Content-Length: 0" << SIP_RTSP_CRLFCRLF;
}

void SipStack::resp_status(stringstream& ss, std::shared_ptr<SipRequest> req, RegStatus& status) {
    if (req->method == "REGISTER") {
        /* 
        //request:  sip-agent-----REGISTER------->sip-server
        REGISTER sip:34020000002000000001@3402000000 SIP/2.0
        Via: SIP/2.0/UDP 192.168.137.11:5060;rport;branch=z9hG4bK1371463273
        From: <sip:34020000001320000003@3402000000>;tag=2043466181
        To: <sip:34020000001320000003@3402000000>
        Call-ID: 1011047669
        CSeq: 1 REGISTER
        Contact: <sip:34020000001320000003@192.168.137.11:5060>
        Max-Forwards: 70
        User-Agent: IP Camera
        Expires: 3600
        Content-Length: 0
        
        //response:  sip-agent<-----200 OK--------sip-server
        SIP/2.0 200 OK
        Via: SIP/2.0/UDP 192.168.137.11:5060;rport;branch=z9hG4bK1371463273
        From: <sip:34020000001320000003@3402000000>
        To: <sip:34020000001320000003@3402000000>
        CSeq: 1 REGISTER
        Call-ID: 1011047669
        Contact: <sip:34020000001320000003@192.168.137.11:5060>
        User-Agent: sip_rtp 
        Expires: 3600
        Content-Length: 0

        */
        if (req->authorization.empty()){
            //TODO: fixme supoort 401
            status = RegUnauthorized;
            return req_401_unauthorized(ss, req);
        }
        //verify password
        if (!verify_ipc_authority(req->authorization)) {
            //bad request
            spdlog::error("deviceid={} password is wrong", req->sip_auth_id);
            
            status = RegForbidden;
            return req_403_forbidden(ss, req);
        }

        status = RegPass;
        
        ss << SIP_VERSION <<" 200 OK" << SIP_RTSP_CRLF
           << "Via: " << get_sip_via(req) << SIP_RTSP_CRLF
           << "From: "<< get_sip_from(req) << SIP_RTSP_CRLF
           << "To: "<<  get_sip_to(req) << SIP_RTSP_CRLF
           << "CSeq: "<< req->seq << " " << req->method <<  SIP_RTSP_CRLF
           << "Call-ID: " << req->call_id << SIP_RTSP_CRLF
           << "Contact: " << req->contact << SIP_RTSP_CRLF
           << "User-Agent: " << SIP_USER_AGENT << SIP_RTSP_CRLF
           << "Expires: " << req->expires << SIP_RTSP_CRLF
           << "Content-Length: 0" << SIP_RTSP_CRLF
		   << "Date: " << sip_get_utc_date() << SIP_RTSP_CRLFCRLF;
    } else {
        /*
        //request: sip-agnet-------MESSAGE------->sip-server
        MESSAGE sip:34020000002000000001@3402000000 SIP/2.0
        Via: SIP/2.0/UDP 192.168.137.11:5060;rport;branch=z9hG4bK1066375804
        From: <sip:34020000001320000003@3402000000>;tag=1925919231
        To: <sip:34020000002000000001@3402000000>
        Call-ID: 1185236415
        CSeq: 20 MESSAGE
        Content-Type: Application/MANSCDP+xml
        Max-Forwards: 70
        User-Agent: IP Camera
        Content-Length:   175

        <?xml version="1.0" encoding="UTF-8"?>
        <Notify>
        <CmdType>Keepalive</CmdType>
        <SN>1</SN>
        <DeviceID>34020000001320000003</DeviceID>
        <Status>OK</Status>
        <Info>
        </Info>
        </Notify>
        //response: sip-agent------200 OK --------> sip-server
        SIP/2.0 200 OK
        Via: SIP/2.0/UDP 192.168.137.11:5060;rport;branch=z9hG4bK1066375804
        From: <sip:34020000001320000003@3402000000>
        To: <sip:34020000002000000001@3402000000>
        CSeq: 20 MESSAGE
        Call-ID: 1185236415
        User-Agent: sip_rtp
        Content-Length: 0
        
        */
        ss << SIP_VERSION <<" 200 OK" << SIP_RTSP_CRLF
           << "Via: " << get_sip_via(req) << SIP_RTSP_CRLF
           << "From: " << get_sip_from(req) << SIP_RTSP_CRLF
           << "To: "<< get_sip_to(req) << SIP_RTSP_CRLF
           << "CSeq: "<< req->seq << " " << req->method <<  SIP_RTSP_CRLF
           << "Call-ID: " << req->call_id << SIP_RTSP_CRLF
           << "User-Agent: " << SIP_USER_AGENT << SIP_RTSP_CRLF
           << "Content-Length: 0" << SIP_RTSP_CRLFCRLF;
    }
   
}

void SipStack::req_invite(stringstream& ss, std::shared_ptr<SipRequest> req) {
    /* 
    //request: sip-agent <-------INVITE------ sip-server
    INVITE sip:34020000001320000003@3402000000 SIP/2.0
    Via: SIP/2.0/UDP 39.100.155.146:15063;rport;branch=z9hG4bK34208805
    From: <sip:34020000002000000001@39.100.155.146:15063>;tag=512358805
    To: <sip:34020000001320000003@3402000000>
    Call-ID: 200008805
    CSeq: 20 INVITE
    Content-Type: Application/SDP
    Contact: <sip:34020000001320000003@3402000000>
    Max-Forwards: 70 
    User-Agent: sip_rtp
    Subject: 34020000001320000003:630886,34020000002000000001:0
    Content-Length: 164

    v=0
    o=34020000001320000003 0 0 IN IP4 39.100.155.146
    s=Play
    c=IN IP4 39.100.155.146
    t=0 0
    m=video 9000 RTP/AVP 96
    a=recvonly
    a=rtpmap:96 PS/90000
    y=630886
    //response: sip-agent --------100 Trying--------> sip-server
    SIP/2.0 100 Trying
    Via: SIP/2.0/UDP 39.100.155.146:15063;rport=15063;branch=z9hG4bK34208805
    From: <sip:34020000002000000001@39.100.155.146:15063>;tag=512358805
    To: <sip:34020000001320000003@3402000000>
    Call-ID: 200008805
    CSeq: 20 INVITE
    User-Agent: IP Camera
    Content-Length: 0

    //response: sip-agent -------200 OK--------> sip-server 
    SIP/2.0 200 OK
    Via: SIP/2.0/UDP 39.100.155.146:15063;rport=15063;branch=z9hG4bK34208805
    From: <sip:34020000002000000001@39.100.155.146:15063>;tag=512358805
    To: <sip:34020000001320000003@3402000000>;tag=1083111311
    Call-ID: 200008805
    CSeq: 20 INVITE
    Contact: <sip:34020000001320000003@192.168.137.11:5060>
    Content-Type: application/sdp
    User-Agent: IP Camera
    Content-Length:   263

    v=0
    o=34020000001320000003 1073 1073 IN IP4 192.168.137.11
    s=Play
    c=IN IP4 192.168.137.11
    t=0 0
    m=video 15060 RTP/AVP 96
    a=setup:active
    a=sendonly
    a=rtpmap:96 PS/90000
    a=username:34020000001320000003
    a=password:12345678
    a=filesize:0
    y=0000630886
    f=
    //request: sip-agent <------ ACK ------- sip-server
    ACK sip:34020000001320000003@3402000000 SIP/2.0
    Via: SIP/2.0/UDP 39.100.155.146:15063;rport;branch=z9hG4bK34208805
    From: <sip:34020000002000000001@39.100.155.146:15063>;tag=512358805
    To: <sip:34020000001320000003@3402000000>
    Call-ID: 200008805
    CSeq: 20 ACK
    Max-Forwards: 70
    User-Agent: sip_rtp
    Content-Length: 0
    */
    char _ssrc[11];
    sprintf(_ssrc, "%010d", req->invite_param.ssrc);
  
    std::stringstream sdp;
    if (!req->invite_param.tcp_flag) {
        sdp << "v=0" << SIP_RTSP_CRLF
        	<< "o=" << req->serial << " 0 0 IN IP4 " << req->invite_param.ip << SIP_RTSP_CRLF
        	<< "s=Play" << SIP_RTSP_CRLF
        	<< "c=IN IP4 " << req->invite_param.ip << SIP_RTSP_CRLF
        	<< "t=0 0" << SIP_RTSP_CRLF
        	//TODO 97 98 99 current no support
        	//<< "m=video " << port <<" RTP/AVP 96 97 98 99" << SIP_RTSP_CRLF
        	<< "m=video " << req->invite_param.port <<" RTP/AVP 96" << SIP_RTSP_CRLF
        	<< "a=recvonly" << SIP_RTSP_CRLF
        	<< "a=rtpmap:96 PS/90000" << SIP_RTSP_CRLF
        	//TODO: current no support
        	//<< "a=rtpmap:97 MPEG4/90000" << SIP_RTSP_CRLF
        	//<< "a=rtpmap:98 H264/90000" << SIP_RTSP_CRLF
        	//<< "a=rtpmap:99 H265/90000" << SIP_RTSP_CRLF
        	//<< "a=streamMode:MAIN\r\n"
        	//<< "a=filesize:0\r\n"
        	<< "y=" << _ssrc << SIP_RTSP_CRLF;
    } else {
        sdp << "v=0" << SIP_RTSP_CRLF
        	<< "o=" << req->serial << " 0 0 IN IP4 " << req->invite_param.ip << SIP_RTSP_CRLF
        	<< "s=Play" << SIP_RTSP_CRLF
        	<< "c=IN IP4 " << req->invite_param.ip << SIP_RTSP_CRLF
        	<< "t=0 0" << SIP_RTSP_CRLF
        	//TODO 97 98 99 current no support
        	//<< "m=video " << port <<" RTP/AVP 96 97 98 99" << SIP_RTSP_CRLF
        	//<< "m=video " << port <<" RTP/AVP 96" << SIP_RTSP_CRLF
        	<< "m=video " << req->invite_param.port << " TCP/RTP/AVP 96" << SIP_RTSP_CRLF
        	//<< "m=video " << port << " TCP/RTP/AVP 98" << SIP_RTSP_CRLF
        	<< "a=recvonly" << SIP_RTSP_CRLF
        	<< "a=rtpmap:96 PS/90000" << SIP_RTSP_CRLF
        	//TODO: current no support
        	//<< "a=rtpmap:97 MPEG4/90000" << SIP_RTSP_CRLF
        	//<< "a=rtpmap:98 H264/90000" << SIP_RTSP_CRLF
        	//<< "a=rtpmap:99 H265/90000" << SIP_RTSP_CRLF
        	//<< "a=streamMode:MAIN\r\n"
        	//<< "a=filesize:0\r\n"
        	<< "y=" << _ssrc << SIP_RTSP_CRLF;
    }
    
    std::stringstream from, to, uri;
    //"INVITE sip:34020000001320000001@3402000000 SIP/2.0\r\n
    uri << "sip:" <<  req->chid << "@" << req->realm;
    //From: <sip:34020000002000000001@%s:%s>;tag=500485%d\r\n
    from << req->serial << "@" << req->realm;
    to <<  req->chid <<  "@" << req->realm;
   
    req->from = from.str();
    req->to = to.str();

    if (!req->to_realm.empty()) {
        req->to  =  req->chid + "@" + req->to_realm;
    }
    if (!req->from_realm.empty()) {
        req->from  =  req->serial + "@" + req->from_realm;
    }

    req->uri  = uri.str();
    req->call_id = sip_generate_call_id();
    req->branch = sip_generate_branch();
    req->from_tag = sip_generate_from_tag();

    ss << "INVITE " << req->uri << " " << SIP_VERSION << SIP_RTSP_CRLF
       << "Via: " << SIP_VERSION << "/UDP "<< req->host << ":" << req->host_port << ";rport;branch=" << req->branch << SIP_RTSP_CRLF
       << "From: " << get_sip_from(req) << SIP_RTSP_CRLF
       << "To: " << get_sip_to(req) << SIP_RTSP_CRLF
       << "Call-ID: " << req->call_id <<SIP_RTSP_CRLF
       << "CSeq: " << req->seq << " INVITE" << SIP_RTSP_CRLF
       << "Content-Type: Application/SDP" << SIP_RTSP_CRLF
       << "Contact: <sip:" << req->to << ">" << SIP_RTSP_CRLF
       << "Max-Forwards: 70" << SIP_RTSP_CRLF
       << "User-Agent: " << SIP_USER_AGENT <<SIP_RTSP_CRLF
       << "Subject: "<< req->chid << ":" << _ssrc << "," << req->serial << ":0" << SIP_RTSP_CRLF
       << "Content-Length: " << sdp.str().length() << SIP_RTSP_CRLFCRLF
       << sdp.str();
}

void SipStack::req_record(stringstream& ss, std::shared_ptr<SipRequest> req) {
    /* 
    //request: sip-agent <-------INVITE------ sip-server
    INVITE sip:34020000001320000003@3402000000 SIP/2.0
    Via: SIP/2.0/UDP 39.100.155.146:15063;rport;branch=z9hG4bK34208805
    From: <sip:34020000002000000001@39.100.155.146:15063>;tag=512358805
    To: <sip:34020000001320000003@3402000000>
    Call-ID: 200008805
    CSeq: 20 INVITE
    Content-Type: Application/SDP
    Contact: <sip:34020000001320000003@3402000000>
    Max-Forwards: 70 
    User-Agent: sip_rtp
    Subject: 34020000001320000003:630886,34020000002000000001:0
    Content-Length: 164

    v=0
    o=34020000001320000003 0 0 IN IP4 39.100.155.146
    s=Play
    c=IN IP4 39.100.155.146
    t=0 0
    m=video 9000 RTP/AVP 96
    a=recvonly
    a=rtpmap:96 PS/90000
    y=630886
    //response: sip-agent --------100 Trying--------> sip-server
    SIP/2.0 100 Trying
    Via: SIP/2.0/UDP 39.100.155.146:15063;rport=15063;branch=z9hG4bK34208805
    From: <sip:34020000002000000001@39.100.155.146:15063>;tag=512358805
    To: <sip:34020000001320000003@3402000000>
    Call-ID: 200008805
    CSeq: 20 INVITE
    User-Agent: IP Camera
    Content-Length: 0

    //response: sip-agent -------200 OK--------> sip-server 
    SIP/2.0 200 OK
    Via: SIP/2.0/UDP 39.100.155.146:15063;rport=15063;branch=z9hG4bK34208805
    From: <sip:34020000002000000001@39.100.155.146:15063>;tag=512358805
    To: <sip:34020000001320000003@3402000000>;tag=1083111311
    Call-ID: 200008805
    CSeq: 20 INVITE
    Contact: <sip:34020000001320000003@192.168.137.11:5060>
    Content-Type: application/sdp
    User-Agent: IP Camera
    Content-Length:   263

    v=0
    o=34020000001320000003 1073 1073 IN IP4 192.168.137.11
    s=Play
    c=IN IP4 192.168.137.11
    t=0 0
    m=video 15060 RTP/AVP 96
    a=setup:active
    a=sendonly
    a=rtpmap:96 PS/90000
    a=username:34020000001320000003
    a=password:12345678
    a=filesize:0
    y=0000630886
    f=
    //request: sip-agent <------ ACK ------- sip-server
    ACK sip:34020000001320000003@3402000000 SIP/2.0
    Via: SIP/2.0/UDP 39.100.155.146:15063;rport;branch=z9hG4bK34208805
    From: <sip:34020000002000000001@39.100.155.146:15063>;tag=512358805
    To: <sip:34020000001320000003@3402000000>
    Call-ID: 200008805
    CSeq: 20 ACK
    Max-Forwards: 70
    User-Agent: sip_rtp
    Content-Length: 0
    */
    char _ssrc[11];
    sprintf(_ssrc, "%010d", req->invite_param.ssrc);
  
    std::stringstream sdp;
	sdp << "v=0" << SIP_RTSP_CRLF
		<< "o=" << req->serial << " 0 0 IN IP4 " << req->invite_param.ip << SIP_RTSP_CRLF
		<< "s=Playback" << SIP_RTSP_CRLF
		<< "u=" << req->chid << ":0" << SIP_RTSP_CRLF
		<< "c=IN IP4 " << req->invite_param.ip << SIP_RTSP_CRLF
		<< "t=" << req->invite_param.start_time << " " << req->invite_param.end_time << SIP_RTSP_CRLF
		//TODO 97 98 99 current no support
		//<< "m=video " << port <<" RTP/AVP 96 97 98 99" << SIP_RTSP_CRLF
		<< "m=video " << req->invite_param.port <<" RTP/AVP 96" << SIP_RTSP_CRLF
		<< "a=recvonly" << SIP_RTSP_CRLF
		<< "a=rtpmap:96 PS/90000" << SIP_RTSP_CRLF
		//TODO: current no support
		//<< "a=rtpmap:97 MPEG4/90000" << SIP_RTSP_CRLF
		//<< "a=rtpmap:98 H264/90000" << SIP_RTSP_CRLF
		//<< "a=rtpmap:99 H265/90000" << SIP_RTSP_CRLF
		//<< "a=streamMode:MAIN\r\n"
		//<< "a=filesize:0\r\n"
		<< "y=" << _ssrc << SIP_RTSP_CRLF;
    
    std::stringstream from, to, uri;
    //"INVITE sip:34020000001320000001@3402000000 SIP/2.0\r\n
    uri << "sip:" <<  req->chid << "@" << req->realm;
    //From: <sip:34020000002000000001@%s:%s>;tag=500485%d\r\n
    from << req->serial << "@" << req->realm;
    to <<  req->chid <<  "@" << req->realm;
   
    req->from = from.str();
    req->to = to.str();
    if (!req->to_realm.empty()) {
        req->to  =  req->chid + "@" + req->to_realm;
    }
    if (!req->from_realm.empty()) {
        req->from  =  req->serial + "@" + req->from_realm;
    }
    req->uri  = uri.str();
    req->call_id = sip_generate_call_id();
    req->branch = sip_generate_branch();
    req->from_tag = sip_generate_from_tag();

    ss << "INVITE " << req->uri << " " << SIP_VERSION << SIP_RTSP_CRLF
       << "Via: " << SIP_VERSION << "/UDP "<< req->host << ":" << req->host_port << ";rport;branch=" << req->branch << SIP_RTSP_CRLF
       << "From: " << get_sip_from(req) << SIP_RTSP_CRLF
       << "To: " << get_sip_to(req) << SIP_RTSP_CRLF
       << "Call-ID: " << req->call_id <<SIP_RTSP_CRLF
       << "CSeq: " << req->seq << " INVITE" << SIP_RTSP_CRLF
       << "Content-Type: Application/SDP" << SIP_RTSP_CRLF
       << "Contact: <sip:" << req->to << ">" << SIP_RTSP_CRLF
       << "Max-Forwards: 70" << SIP_RTSP_CRLF
       << "User-Agent: " << SIP_USER_AGENT <<SIP_RTSP_CRLF
       << "Subject: "<< req->chid << ":" << _ssrc << "," << req->serial << ":0" << SIP_RTSP_CRLF
       << "Content-Length: " << sdp.str().length() << SIP_RTSP_CRLFCRLF
       << sdp.str();
}

void SipStack::req_401_unauthorized(std::stringstream& ss, std::shared_ptr<SipRequest> req) {
    /* sip-agent <-----401 Unauthorized ------ sip-server
    SIP/2.0 401 Unauthorized
    Via: SIP/2.0/UDP 192.168.137.92:5061;rport=61378;received=192.168.1.13;branch=z9hG4bK802519080
    From: <sip:34020000001320000004@192.168.137.92:5061>;tag=611442989
    To: <sip:34020000001320000004@192.168.137.92:5061>;tag=102092689
    CSeq: 1 REGISTER
    Call-ID: 1650345118
    User-Agent: sip_rtp
    Contact: <sip:34020000002000000001@192.168.1.23:15060>
    Content-Length: 0
    WWW-Authenticate: Digest realm="3402000000",qop="auth",nonce="f1da98bd160f3e2efe954c6eedf5f75a"
    */
    ss << SIP_VERSION <<" 401 Unauthorized" << SIP_RTSP_CRLF
       //<< "Via: " << req->via << SIP_RTSP_CRLF
       << "Via: " << get_sip_via(req) << SIP_RTSP_CRLF
       << "From: " << get_sip_from(req) << SIP_RTSP_CRLF
       << "To: " << get_sip_to(req) << SIP_RTSP_CRLF
       << "CSeq: "<< req->seq << " " << req->method <<  SIP_RTSP_CRLF
       << "Call-ID: " << req->call_id << SIP_RTSP_CRLF
       << "Contact: " << req->contact << SIP_RTSP_CRLF
       << "User-Agent: " << SIP_USER_AGENT << SIP_RTSP_CRLF
       << "Content-Length: 0" << SIP_RTSP_CRLF
       << "WWW-Authenticate: Digest realm=\"3402000000\",nonce=\"f1da98bd160f3e2efe954c6eedf5f75a\",algorithm=MD5" << SIP_RTSP_CRLFCRLF;
    return;
}

void SipStack::req_403_forbidden(std::stringstream& ss, std::shared_ptr<SipRequest> req) {
    /* sip-agent <-----403 Forbidden ------ sip-server
    SIP/2.0 403 Forbidden
    Via: SIP/2.0/UDP 192.168.137.92:5061;rport=61378;received=192.168.1.13;branch=z9hG4bK802519080
    From: <sip:34020000001320000004@192.168.137.92:5061>;tag=611442989
    To: <sip:34020000001320000004@192.168.137.92:5061>;tag=102092689
    CSeq: 1 REGISTER
    Call-ID: 1650345118
    User-Agent: sip_rtp
    Contact: <sip:34020000002000000001@192.168.1.23:15060>
    Content-Length: 0
    */
    ss << SIP_VERSION <<" 403 Forbidden" << SIP_RTSP_CRLF
       //<< "Via: " << req->via << SIP_RTSP_CRLF
       << "Via: " << get_sip_via(req) << SIP_RTSP_CRLF
       << "From: " << get_sip_from(req) << SIP_RTSP_CRLF
       << "To: " << get_sip_to(req) << SIP_RTSP_CRLF
       << "CSeq: "<< req->seq << " " << req->method <<  SIP_RTSP_CRLF
       << "Call-ID: " << req->call_id << SIP_RTSP_CRLF
       << "Contact: " << req->contact << SIP_RTSP_CRLF
       << "User-Agent: " << SIP_USER_AGENT << SIP_RTSP_CRLF
       << "Content-Length: 0" << SIP_RTSP_CRLFCRLF;
    
    return;
}

void SipStack::req_ack(std::stringstream& ss, std::shared_ptr<SipRequest> req) {
    /*
    //request: sip-agent <------ ACK ------- sip-server
    ACK sip:34020000001320000003@3402000000 SIP/2.0
    Via: SIP/2.0/UDP 39.100.155.146:15063;rport;branch=z9hG4bK34208805
    From: <sip:34020000002000000001@39.100.155.146:15063>;tag=512358805
    To: <sip:34020000001320000003@3402000000>
    Call-ID: 200008805
    CSeq: 20 ACK
    Max-Forwards: 70
    User-Agent: sip_rtp
    Content-Length: 0
    */
    ss << "ACK " << "sip:" <<  req->chid << "@" << req->realm << " "<< SIP_VERSION << SIP_RTSP_CRLF
       << "Via: " << SIP_VERSION << "/UDP " << req->host << ":" << req->host_port << ";rport;branch=" << req->branch << SIP_RTSP_CRLF
       << "From: " << get_sip_from(req) << SIP_RTSP_CRLF
       << "To: "<< get_sip_to(req) << SIP_RTSP_CRLF
       << "Call-ID: " << req->call_id << SIP_RTSP_CRLF
       << "CSeq: " << req->seq << " ACK"<< SIP_RTSP_CRLF
       << "Max-Forwards: 70" << SIP_RTSP_CRLF
       << "User-Agent: "<< SIP_USER_AGENT << SIP_RTSP_CRLF
       << "Content-Length: 0" << SIP_RTSP_CRLFCRLF;
}

void SipStack::req_bye(std::stringstream& ss, std::shared_ptr<SipRequest> req) {
    /*
    //request: sip-agent <------BYE------ sip-server
    BYE sip:34020000001320000003@3402000000 SIP/2.0
    Via: SIP/2.0/UDP 39.100.155.146:15063;rport;branch=z9hG4bK34208805
    From: <sip:34020000002000000001@3402000000>;tag=512358805
    To: <sip:34020000001320000003@3402000000>;tag=1083111311
    Call-ID: 200008805
    CSeq: 79 BYE
    Max-Forwards: 70
    User-Agent: sip_rtp
    Content-Length: 0

    //response: sip-agent ------200 OK ------> sip-server
    SIP/2.0 200 OK
    Via: SIP/2.0/UDP 39.100.155.146:15063;rport=15063;branch=z9hG4bK34208805
    From: <sip:34020000002000000001@3402000000>;tag=512358805
    To: <sip:34020000001320000003@3402000000>;tag=1083111311
    Call-ID: 200008805
    CSeq: 79 BYE
    User-Agent: IP Camera
    Content-Length: 0
    */
    std::stringstream from, to, uri;
    uri << "sip:" <<  req->chid << "@" << req->realm;
    from << req->serial << "@"  << req->realm;
    to << req->chid <<  "@" <<  req->realm;

    req->from = from.str();
    req->to = to.str();

    if (!req->to_realm.empty()) {
        req->to  =  req->chid + "@" + req->to_realm;
    }
    if (!req->from_realm.empty()) {
        req->from  =  req->serial + "@" + req->from_realm;
    }

    req->uri  = uri.str();

    ss << "BYE " << req->uri << " "<< SIP_VERSION << SIP_RTSP_CRLF
       //<< "Via: "<< SIP_VERSION << "/UDP "<< req->host << ":" << req->host_port << ";rport" << branch << SIP_RTSP_CRLF
       << "Via: " << SIP_VERSION << "/UDP " << req->host << ":" << req->host_port << ";rport;branch=" << req->branch << SIP_RTSP_CRLF
       << "From: " << get_sip_from(req) << SIP_RTSP_CRLF
       << "To: " << get_sip_to(req) << SIP_RTSP_CRLF
       //bye callid is inivte callid
       << "Call-ID: " << req->call_id << SIP_RTSP_CRLF
       << "CSeq: "<< req->seq <<" BYE" << SIP_RTSP_CRLF
       << "Max-Forwards: 70" << SIP_RTSP_CRLF
       << "User-Agent: " << SIP_USER_AGENT << SIP_RTSP_CRLF
       << "Content-Length: 0" << SIP_RTSP_CRLFCRLF;
}

void SipStack::req_query_catalog(std::stringstream& ss, std::shared_ptr<SipRequest> req) {
    /*
    //request: sip-agent <----MESSAGE Query Catalog--- sip-server
    MESSAGE sip:34020000001110000001@192.168.1.21:5060 SIP/2.0
    Via: SIP/2.0/UDP 192.168.1.17:5060;rport;branch=z9hG4bK563315752
    From: <sip:34020000001110000001@3402000000>;tag=387315752
    To: <sip:34020000001110000001@192.168.1.21:5060>
    Call-ID: 728315752
    CSeq: 32 MESSAGE
    Content-Type: Application/MANSCDP+xml
    Max-Forwards: 70
    User-Agent: sip_rtp
    Content-Length: 162

    <?xml version="1.0" encoding="UTF-8"?>
    <Query>
        <CmdType>Catalog</CmdType>
        <SN>419315752</SN>
        <DeviceID>34020000001110000001</DeviceID>
    </Query>
    SIP/2.0 200 OK
    Via: SIP/2.0/UDP 192.168.1.17:5060;rport=5060;branch=z9hG4bK563315752
    From: <sip:34020000001110000001@3402000000>;tag=387315752
    To: <sip:34020000001110000001@192.168.1.21:5060>;tag=1420696981
    Call-ID: 728315752
    CSeq: 32 MESSAGE
    User-Agent: Embedded Net DVR/NVR/DVS
    Content-Length: 0

    //response: sip-agent ----MESSAGE Query Catalog---> sip-server
    SIP/2.0 200 OK
    Via: SIP/2.0/UDP 192.168.1.17:5060;rport=5060;received=192.168.1.17;branch=z9hG4bK563315752
    From: <sip:34020000001110000001@3402000000>;tag=387315752
    To: <sip:34020000001110000001@192.168.1.21:5060>;tag=1420696981
    CSeq: 32 MESSAGE
    Call-ID: 728315752
    User-Agent: sip_rtp
    Content-Length: 0

    //request: sip-agent ----MESSAGE Response Catalog---> sip-server
    MESSAGE sip:34020000001110000001@3402000000.spvmn.cn SIP/2.0
    Via: SIP/2.0/UDP 192.168.1.21:5060;rport;branch=z9hG4bK1681502633
    From: <sip:34020000001110000001@3402000000.spvmn.cn>;tag=1194168247
    To: <sip:34020000001110000001@3402000000.spvmn.cn>
    Call-ID: 685380150
    CSeq: 20 MESSAGE
    Content-Type: Application/MANSCDP+xml
    Max-Forwards: 70
    User-Agent: Embedded Net DVR/NVR/DVS
    Content-Length:   909

    <?xml version="1.0" encoding="gb2312"?>
    <Response>
    <CmdType>Catalog</CmdType>
    <SN>419315752</SN>
    <DeviceID>34020000001110000001</DeviceID>
    <SumNum>8</SumNum>
    <DeviceList Num="2">
    <Item>
    <DeviceID>34020000001320000001</DeviceID>
    <Name>Camera 01</Name>
    <Manufacturer>Manufacturer</Manufacturer>
    <Model>Camera</Model>
    <Owner>Owner</Owner>
    <CivilCode>CivilCode</CivilCode>
    <Address>192.168.254.18</Address>
    <Parental>0</Parental>
    <SafetyWay>0</SafetyWay>
    <RegisterWay>1</RegisterWay>
    <Secrecy>0</Secrecy>
    <Status>ON</Status>
    </Item>
    <Item>
    <DeviceID>34020000001320000002</DeviceID>
    <Name>IPCamera 02</Name>
    <Manufacturer>Manufacturer</Manufacturer>
    <Model>Camera</Model>
    <Owner>Owner</Owner>
    <CivilCode>CivilCode</CivilCode>
    <Address>192.168.254.14</Address>
    <Parental>0</Parental>
    <SafetyWay>0</SafetyWay>
    <RegisterWay>1</RegisterWay>
    <Secrecy>0</Secrecy>
    <Status>OFF</Status>
    </Item>
    </DeviceList>
    </Response>

    */
    std::stringstream xml;
    std::string xmlbody;

    xml << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>" << SIP_RTSP_CRLF
    	<< "<Query>" << SIP_RTSP_CRLF
    	<< "<CmdType>Catalog</CmdType>" << SIP_RTSP_CRLF
    	<< "<SN>" << sip_generate_sn() << "</SN>" << SIP_RTSP_CRLF
    	<< "<DeviceID>" << req->sip_auth_id << "</DeviceID>" << SIP_RTSP_CRLF
    	<< "</Query>" << SIP_RTSP_CRLF;
    xmlbody = xml.str();

    std::stringstream from, to, uri;
    //"INVITE sip:34020000001320000001@3402000000 SIP/2.0\r\n
    uri << "sip:" <<  req->sip_auth_id << "@" << req->realm;
    //From: <sip:34020000002000000001@%s:%s>;tag=500485%d\r\n
    from << req->serial << "@" << req->host << ":"  << req->host_port;
    to << req->sip_auth_id <<  "@" << req->realm;
 
    req->from = from.str();
    req->to   = to.str();
    req->uri  = uri.str();
    req->call_id = sip_generate_call_id();
    req->branch = sip_generate_branch();
    req->from_tag = sip_generate_from_tag();

    ss << "MESSAGE " << req->uri << " " << SIP_VERSION << SIP_RTSP_CRLF
       << "Via: " << SIP_VERSION << "/UDP "<< req->host << ":" << req->host_port << ";rport;branch=" << req->branch << SIP_RTSP_CRLF
       << "From: " << get_sip_from(req) << SIP_RTSP_CRLF
       << "To: " << get_sip_to(req) << SIP_RTSP_CRLF
       << "Call-ID: " << req->call_id << SIP_RTSP_CRLF
       << "CSeq: " << req->seq << " MESSAGE" << SIP_RTSP_CRLF
       << "Content-Type: Application/MANSCDP+xml" << SIP_RTSP_CRLF
       << "Max-Forwards: 70" << SIP_RTSP_CRLF
       << "User-Agent: " << SIP_USER_AGENT << SIP_RTSP_CRLF
       << "Content-Length: " << xmlbody.length() << SIP_RTSP_CRLFCRLF
       << xmlbody;
}

void SipStack::req_query_record_info(std::stringstream& ss, std::shared_ptr<SipRequest> req, 
									 const std::string& start_time, const std::string& end_time) {	
    std::stringstream xml;
    std::string xmlbody;

    xml << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>" << SIP_RTSP_CRLF
		<< "<Query>" << SIP_RTSP_CRLF
		<< "<CmdType>RecordInfo</CmdType>" << SIP_RTSP_CRLF
		<< "<SN>" << 9876 << "</SN>" << SIP_RTSP_CRLF
		<< "<DeviceID>" << req->sip_auth_id << "</DeviceID>" << SIP_RTSP_CRLF
		<< "<StartTime>" << start_time << "</StartTime>" << SIP_RTSP_CRLF
		<< "<EndTime>" << end_time << "</EndTime>" << SIP_RTSP_CRLF
		<< "<Secrecy>0</Secrecy>" << SIP_RTSP_CRLF
		<< "<Type>time</Type>" << SIP_RTSP_CRLF
		<< "</Query>" << SIP_RTSP_CRLF;
    xmlbody = xml.str();

    std::stringstream from, to, uri;
    //"INVITE sip:34020000001320000001@3402000000 SIP/2.0\r\n
    uri << "sip:" <<  req->sip_auth_id << "@" << req->realm;
    //From: <sip:34020000002000000001@%s:%s>;tag=500485%d\r\n
    from << req->serial << "@" << req->host << ":"  << req->host_port;
    to << req->sip_auth_id <<  "@" << req->realm;
 
    req->from = from.str();
    req->to   = to.str();
    req->uri  = uri.str();
    req->call_id = sip_generate_call_id();
    req->branch = sip_generate_branch();
    req->from_tag = sip_generate_from_tag();

    ss << "MESSAGE " << req->uri << " " << SIP_VERSION << SIP_RTSP_CRLF
       << "Via: " << SIP_VERSION << "/UDP "<< req->host << ":" << req->host_port << ";rport;branch=" << req->branch << SIP_RTSP_CRLF
       << "From: " << get_sip_from(req) << SIP_RTSP_CRLF
       << "To: " << get_sip_to(req) << SIP_RTSP_CRLF
       << "Call-ID: " << req->call_id << SIP_RTSP_CRLF
       << "CSeq: " << req->seq << " MESSAGE" << SIP_RTSP_CRLF
       << "Content-Type: Application/MANSCDP+xml" << SIP_RTSP_CRLF
       << "Max-Forwards: 70" << SIP_RTSP_CRLF
       << "User-Agent: " << SIP_USER_AGENT << SIP_RTSP_CRLF
       << "Content-Length: " << xmlbody.length() << SIP_RTSP_CRLFCRLF
       << xmlbody;
}

void SipStack::req_record_start(std::stringstream& ss, std::shared_ptr<SipRequest> req) {
    std::stringstream xml;
    std::string xmlbody;

    xml << "PLAY MANSRTSP/1.0" << SIP_RTSP_CRLF
    << "CSeq: " << req->seq << SIP_RTSP_CRLF
    << "Range: npt=now-" << SIP_RTSP_CRLF;
    //<< "Range: " << "npt=1000-" << SIP_RTSP_CRLF;
    xmlbody = xml.str();

    std::stringstream  uri;
    uri << "sip:" <<  req->sip_auth_id << "@" << req->realm;
    req->uri  = uri.str();

    ss << "INFO " << req->uri << " " << SIP_VERSION << SIP_RTSP_CRLF
       << "Via: " << SIP_VERSION << "/UDP "<< req->host << ":" << req->host_port << ";rport;branch=" << req->branch << SIP_RTSP_CRLF
       << "From: " << get_sip_from(req) << SIP_RTSP_CRLF
       << "To: " << get_sip_to(req) << SIP_RTSP_CRLF
       << "Call-ID: " << req->call_id << SIP_RTSP_CRLF
       << "CSeq: " << req->seq << " INFO" << SIP_RTSP_CRLF
       << "Content-Type: Application/MANSRTSP" << SIP_RTSP_CRLF
       << "Max-Forwards: 70" << SIP_RTSP_CRLF
       << "User-Agent: " << SIP_USER_AGENT << SIP_RTSP_CRLF
       << "Content-Length: " << xmlbody.length() << SIP_RTSP_CRLFCRLF
       << xmlbody;
}

void SipStack::req_record_stop(std::stringstream& ss, std::shared_ptr<SipRequest> req) {
    std::stringstream xml;
    std::string xmlbody;

    xml << "PAUSE MANSRTSP/1.0" << SIP_RTSP_CRLF
    << "CSeq: " << req->seq << SIP_RTSP_CRLF
    << "PauseTime: now" << SIP_RTSP_CRLF;
    //<< "Range: " << "npt=1000-" << SIP_RTSP_CRLF;
    xmlbody = xml.str();

    std::stringstream  uri;
    uri << "sip:" <<  req->sip_auth_id << "@" << req->realm;
    req->uri  = uri.str();

    ss << "INFO " << req->uri << " " << SIP_VERSION << SIP_RTSP_CRLF
       << "Via: " << SIP_VERSION << "/UDP "<< req->host << ":" << req->host_port << ";rport;branch=" << req->branch << SIP_RTSP_CRLF
       << "From: " << get_sip_from(req) << SIP_RTSP_CRLF
       << "To: " << get_sip_to(req) << SIP_RTSP_CRLF
       << "Call-ID: " << req->call_id << SIP_RTSP_CRLF
       << "CSeq: " << req->seq << " INFO" << SIP_RTSP_CRLF
       << "Content-Type: Application/MANSRTSP" << SIP_RTSP_CRLF
       << "Max-Forwards: 70" << SIP_RTSP_CRLF
       << "User-Agent: " << SIP_USER_AGENT << SIP_RTSP_CRLF
       << "Content-Length: " << xmlbody.length() << SIP_RTSP_CRLFCRLF
       << xmlbody;
}

void SipStack::req_record_seek(std::stringstream& ss, std::shared_ptr<SipRequest> req, const std::string& range) {
    std::stringstream xml;
    std::string xmlbody;

    xml << "PLAY MANSRTSP/1.0" << SIP_RTSP_CRLF
    << "CSeq: " << req->seq << SIP_RTSP_CRLF
    << "Range: " << "npt=" << range << "-" << SIP_RTSP_CRLF;
    xmlbody = xml.str();

    std::stringstream uri, from, to;
    uri << "sip:" <<  req->sip_auth_id << "@" << req->realm;
	req->uri  = uri.str();

    ss << "INFO " << req->uri << " " << SIP_VERSION << SIP_RTSP_CRLF
       << "Via: " << SIP_VERSION << "/UDP "<< req->host << ":" << req->host_port << ";rport;branch=" << req->branch << SIP_RTSP_CRLF
       << "From: " << get_sip_from(req) << SIP_RTSP_CRLF
       << "To: " << get_sip_to(req) << SIP_RTSP_CRLF
       << "Call-ID: " << req->call_id << SIP_RTSP_CRLF
       << "CSeq: " << req->seq << " INFO" << SIP_RTSP_CRLF
       << "Content-Type: Application/MANSRTSP" << SIP_RTSP_CRLF
       << "Max-Forwards: 70" << SIP_RTSP_CRLF
       << "User-Agent: " << SIP_USER_AGENT << SIP_RTSP_CRLF
       << "Content-Length: " << xmlbody.length() << SIP_RTSP_CRLFCRLF
       << xmlbody;
}

void SipStack::req_record_fast(std::stringstream& ss, std::shared_ptr<SipRequest> req, const std::string& scale) {
    std::stringstream xml;
    std::string xmlbody;

    xml << "PLAY MANSRTSP/1.0" << SIP_RTSP_CRLF
		<< "CSeq: " << req->seq << SIP_RTSP_CRLF
		<< "Scale: " << scale << SIP_RTSP_CRLF;
    xmlbody = xml.str();

    std::stringstream  uri;
    uri << "sip:" <<  req->sip_auth_id << "@" << req->realm;
    req->uri  = uri.str();

    ss << "INFO " << req->uri << " " << SIP_VERSION << SIP_RTSP_CRLF
       << "Via: " << SIP_VERSION << "/UDP "<< req->host << ":" << req->host_port << ";rport;branch=" << req->branch << SIP_RTSP_CRLF
       << "From: " << get_sip_from(req) << SIP_RTSP_CRLF
       << "To: " << get_sip_to(req) << SIP_RTSP_CRLF
       << "Call-ID: " << req->call_id << SIP_RTSP_CRLF
       << "CSeq: " << req->seq << " INFO" << SIP_RTSP_CRLF
       << "Content-Type: Application/MANSRTSP" << SIP_RTSP_CRLF
       << "Max-Forwards: 70" << SIP_RTSP_CRLF
       << "User-Agent: " << SIP_USER_AGENT << SIP_RTSP_CRLF
       << "Content-Length: " << xmlbody.length() << SIP_RTSP_CRLFCRLF
       << xmlbody;
}

void SipStack::req_record_teardown(std::stringstream& ss, std::shared_ptr<SipRequest> req) {
    std::stringstream xml;
    std::string xmlbody;

    xml << "TEARDOWN MANSRTSP/1.0" << SIP_RTSP_CRLF
    	<< "CSeq: " << req->seq << SIP_RTSP_CRLF;
    xmlbody = xml.str();

    std::stringstream  uri;
    uri << "sip:" <<  req->sip_auth_id << "@" << req->realm;
    req->uri  = uri.str();

    ss << "INFO " << req->uri << " " << SIP_VERSION << SIP_RTSP_CRLF
       << "Via: " << SIP_VERSION << "/UDP "<< req->host << ":" << req->host_port << ";rport;branch=" << req->branch << SIP_RTSP_CRLF
       << "From: " << get_sip_from(req) << SIP_RTSP_CRLF
       << "To: " << get_sip_to(req) << SIP_RTSP_CRLF
       << "Call-ID: " << req->call_id << SIP_RTSP_CRLF
       << "CSeq: " << req->seq << " INFO" << SIP_RTSP_CRLF
       << "Content-Type: Application/MANSRTSP" << SIP_RTSP_CRLF
       << "Max-Forwards: 70" << SIP_RTSP_CRLF
       << "User-Agent: " << SIP_USER_AGENT << SIP_RTSP_CRLF
       << "Content-Length: " << xmlbody.length() << SIP_RTSP_CRLFCRLF
       << xmlbody;
}

void SipStack::req_ptz(std::stringstream& ss, std::shared_ptr<SipRequest> req, uint8_t cmd, uint8_t speed, int priority) {
    /*
    <?xml version="1.0"?>  
    <Control>  
    <CmdType>DeviceControl</CmdType>  
    <SN>11</SN>  
    <DeviceID>34020000001310000053</DeviceID>  
    <PTZCmd>A50F01021F0000D6</PTZCmd>  
    </Control> 
    */
    uint8_t ptz_cmd[8] = {0};
    ptz_cmd[0] = SIP_PTZ_START;
    ptz_cmd[1] = 0x0F;
    ptz_cmd[2] = 0x01;
    ptz_cmd[3] = cmd;
    switch(cmd){
        case SipPtzCmdStop: // = 0x00
            ptz_cmd[4] = 0;
            ptz_cmd[5] = 0;
            ptz_cmd[6] = 0;
            break;
        case SipPtzCmdRight: // = 0x01,
        case SipPtzCmdLeft: //  = 0x02,
            ptz_cmd[4] = speed;
            break;
        case SipPtzCmdDown: //  = 0x04,
        case SipPtzCmdUp: //    = 0x08,
            ptz_cmd[5] = speed;
            break;
        case SipPtzCmdZoomOut: //  = 0x10,
        case SipPtzCmdZoomIn: //   = 0x20
            ptz_cmd[6] = (speed & 0x0F) << 4;
            break;
        case SipPtzCmdRightUp:
        case SipPtzCmdRightDown:
        case SipPtzCmdLeftUp:
        case SipPtzCmdLeftDown:
            ptz_cmd[4] = speed;
            ptz_cmd[5] = speed;
            break;
        default:
            return;
    }

    uint32_t check = 0;
    for (int i = 0; i < 7; i++){
        check += ptz_cmd[i];
    }
    ptz_cmd[7] = (uint8_t)(check % 256);

    std::stringstream ss_ptzcmd;
    for (int i = 0; i < 8; i++){
        char hex_cmd[3] = {0};
        sprintf(hex_cmd, "%02X", ptz_cmd[i]);
        ss_ptzcmd << hex_cmd;
    }
    std::stringstream xml;
    std::string xmlbody;

    xml << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>" << SIP_RTSP_CRLF
    	<< "<Control>" << SIP_RTSP_CRLF
    	<< "<CmdType>DeviceControl</CmdType>" << SIP_RTSP_CRLF
    	<< "<SN>" << sip_generate_sn() << "</SN>" << SIP_RTSP_CRLF
    	<< "<DeviceID>" << req->sip_auth_id << "</DeviceID>" << SIP_RTSP_CRLF
    	<< "<PTZCmd>" << ss_ptzcmd.str() << "</PTZCmd>" << SIP_RTSP_CRLF
    	<< "<Info>" << SIP_RTSP_CRLF
    	<< "<ControlPriority>" << priority << "</ControlPriority>" << SIP_RTSP_CRLF
    	<< "</Info>" << SIP_RTSP_CRLF
    	<< "</Control>" << SIP_RTSP_CRLF;
    xmlbody = xml.str();

    std::stringstream from, to, uri, call_id;
    //"INVITE sip:34020000001320000001@3402000000 SIP/2.0\r\n
    uri << "sip:" <<  req->sip_auth_id << "@" << req->realm;
    //From: <sip:34020000002000000001@%s:%s>;tag=500485%d\r\n
    from << req->serial << "@" << req->host << ":"  << req->host_port;
    to << req->sip_auth_id <<  "@" << req->realm;
   
    req->from = from.str();
    req->to   = to.str();
    req->uri  = uri.str();
    req->call_id = sip_generate_call_id();
    req->branch = sip_generate_branch();
    req->from_tag = sip_generate_from_tag();

    ss << "MESSAGE " << req->uri << " "<< SIP_VERSION << SIP_RTSP_CRLF
       //<< "Via: "<< SIP_VERSION << "/UDP "<< req->host << ":" << req->host_port << ";rport" << branch << SIP_RTSP_CRLF
       << "Via: " << SIP_VERSION << "/UDP " << req->host << ":" << req->host_port << ";rport;branch=" << req->branch << SIP_RTSP_CRLF
       << "From: " << get_sip_from(req) << SIP_RTSP_CRLF
       << "To: " << get_sip_to(req) << SIP_RTSP_CRLF
       << "Call-ID: " << req->call_id << SIP_RTSP_CRLF
       << "CSeq: "<< req->seq <<" MESSAGE" << SIP_RTSP_CRLF
       << "Content-Type: Application/MANSCDP+xml" << SIP_RTSP_CRLF
       << "Max-Forwards: 70" << SIP_RTSP_CRLF
       << "User-Agent: " << SIP_USER_AGENT << SIP_RTSP_CRLF
       << "Content-Length: " << xmlbody.length() << SIP_RTSP_CRLFCRLF
       << xmlbody;
}

void SipStack::req_fi(std::stringstream& ss, std::shared_ptr<SipRequest> req, uint8_t cmd, uint8_t speed) {
    /*
    <?xml version="1.0"?>  
    <Control>  
    <CmdType>DeviceControl</CmdType>  
    <SN>11</SN>  
    <DeviceID>34020000001310000053</DeviceID>  
    <PTZCmd>A50F01021F0000D6</PTZCmd>  
    </Control> 
    */
    uint8_t ptz_cmd[8] = {0};
    ptz_cmd[0] = SIP_PTZ_START;
    ptz_cmd[1] = 0x0F;
    ptz_cmd[2] = 0x01;
    ptz_cmd[3] = cmd | 0x40;
    switch(cmd) {
        case SipFiCmdStop: // = 0x00
            ptz_cmd[4] = 0;
            ptz_cmd[5] = 0;
            ptz_cmd[6] = 0;
            break;
        case SipFiCmdIrisBig: // = 0x01,
        case SipFiCmdIrisSmall: //  = 0x02,
            ptz_cmd[5] = speed;
            break;
        case SipFiCmdFocusFar: //  = 0x04,
        case SipFiCmdFocusNear: //    = 0x08,
            ptz_cmd[4] = speed;
            break;
        default:
            return;
    }

    uint32_t check = 0;
    for (int i = 0; i < 7; i++){
        check += ptz_cmd[i];
    }
    //ptz_cmd[7] = (uint8_t)(check % 256);
    ptz_cmd[7] = 0;

    std::stringstream ss_ficmd;
    for (int i = 0; i < 8; i++){
        char hex_cmd[3] = {0};
        sprintf(hex_cmd, "%02X", ptz_cmd[i]);
        ss_ficmd << hex_cmd;
    }
    std::stringstream xml;
    std::string xmlbody;

    xml << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>" << SIP_RTSP_CRLF
        << "<Control>" << SIP_RTSP_CRLF
        << "<CmdType>DeviceControl</CmdType>" << SIP_RTSP_CRLF
        << "<SN>" << sip_generate_sn() << "</SN>" << SIP_RTSP_CRLF
        << "<DeviceID>" << req->sip_auth_id << "</DeviceID>" << SIP_RTSP_CRLF
        << "<FICmd>" << ss_ficmd.str() << "</FICmd>" << SIP_RTSP_CRLF
        << "<Info>" << SIP_RTSP_CRLF
        << "<ControlPriority>" << 5 << "</ControlPriority>" << SIP_RTSP_CRLF
        << "</Info>" << SIP_RTSP_CRLF
        << "</Control>" << SIP_RTSP_CRLF;
    xmlbody = xml.str();

    std::stringstream from, to, uri, call_id;
    //"INVITE sip:34020000001320000001@3402000000 SIP/2.0\r\n
    uri << "sip:" <<  req->sip_auth_id << "@" << req->realm;
    //From: <sip:34020000002000000001@%s:%s>;tag=500485%d\r\n
    from << req->serial << "@" << req->host << ":"  << req->host_port;
    to << req->sip_auth_id <<  "@" << req->realm;
   
    req->from = from.str();
    req->to   = to.str();
    req->uri  = uri.str();
    req->call_id = sip_generate_call_id();
    req->branch = sip_generate_branch();
    req->from_tag = sip_generate_from_tag();

    ss << "MESSAGE " << req->uri << " "<< SIP_VERSION << SIP_RTSP_CRLF
       //<< "Via: "<< SIP_VERSION << "/UDP "<< req->host << ":" << req->host_port << ";rport" << branch << SIP_RTSP_CRLF
       << "Via: " << SIP_VERSION << "/UDP " << req->host << ":" << req->host_port << ";rport;branch=" << req->branch << SIP_RTSP_CRLF
       << "From: " << get_sip_from(req) << SIP_RTSP_CRLF
       << "To: " << get_sip_to(req) << SIP_RTSP_CRLF
       << "Call-ID: " << req->call_id << SIP_RTSP_CRLF
       << "CSeq: "<< req->seq <<" MESSAGE" << SIP_RTSP_CRLF
       << "Content-Type: Application/MANSCDP+xml" << SIP_RTSP_CRLF
       << "Max-Forwards: 70" << SIP_RTSP_CRLF
       << "User-Agent: " << SIP_USER_AGENT << SIP_RTSP_CRLF
       << "Content-Length: " << xmlbody.length() << SIP_RTSP_CRLFCRLF
       << xmlbody;
}

bool SipStack::verify_ipc_authority(const std::string& auth) {
    if (auth.empty()) {
        return false;
	}
    std::string password = ServerConfig::get_instance()->sip_password();
    std::string username;
    std::string realm;
    std::string uri;
    std::string nonce;
    std::string response;

    std::string::size_type pos;
    std::vector<std::string> result = string_split(auth, ",");
    for (auto iter = result.begin(); iter != result.end(); iter++) {
        if((pos = iter->find("username")) != std::string::npos) {
            username = std::string(&(*iter)[pos+9]);
            remove_spec_char(username, '"');
            username.push_back(':');
        } else if ((pos = iter->find("realm")) != std::string::npos) {
            realm = std::string(&(*iter)[pos+6]);
            remove_spec_char(realm, '"');
            realm.push_back(':');            
        } else if ((pos = iter->find("uri")) != std::string::npos) {
            uri = std::string(&(*iter)[pos+4]);
            remove_spec_char(uri, '"');

        } else if ((pos = iter->find("nonce")) != std::string::npos) {
            nonce = std::string(&(*iter)[pos+6]);
            remove_spec_char(nonce, '"');
            nonce.push_back(':');
        } else if((pos = iter->find("response")) != std::string::npos) {
            response = std::string(&(*iter)[pos+9]);
            remove_spec_char(response, '"');
        }
    }

    spdlog::info("username={} realm={} nonce={} uri={} response={}", username, realm, nonce, uri, response);

    std::string A1 = MD5(username + realm + password).toStr();
    std::string A2 = MD5(std::string("REGISTER:") + uri).toStr();
    std::string md5_result = MD5(A1 + std::string(":") + nonce + A2).toStr();

    spdlog::info("md5_result={}\n response={}", md5_result, response);

    return md5_result == response; 
}

} //namespace sip