#ifndef _PUB_CLIENT_H
#define _PUB_CLIENT_H

#include "base/serialize_util.hpp"
#include "client_base.hpp"

namespace easyrpc
{

class pub_client : public client_base
{
public:
    pub_client(const pub_client&) = delete;
    pub_client& operator=(const pub_client&) = delete;
    pub_client() 
    {
        client_type_ = client_type::pub_client;
    }

    virtual void run() override final
    {
        client_base::run();
        try_connect();
    }

    template<typename... Args>
    void publish(const std::string& topic_name, Args&&... args)
    {
        try_connect();
        client_flag flag{ serialize_mode::serialize, client_type_ };
        call_one_way(topic_name, flag, serialize(std::forward<Args>(args)...));
    }

    void publish_raw(const std::string& topic_name, const std::string& body)
    {
        try_connect();
        client_flag flag{ serialize_mode::non_serialize, client_type_ };
        call_one_way(topic_name, flag, body);
    }

    template<typename... Args>
    void async_publish(const std::string& topic_name, Args&&... args)
    {
        try_connect();
        client_flag flag{ serialize_mode::serialize, client_type_ };
        async_call_one_way(topic_name, flag, serialize(std::forward<Args>(args)...));
    }

    void async_publish_raw(const std::string& topic_name, const std::string& body)
    {
        try_connect();
        client_flag flag{ serialize_mode::non_serialize, client_type_ };
        async_call_one_way(topic_name, flag, body);
    }
};

}

#endif
