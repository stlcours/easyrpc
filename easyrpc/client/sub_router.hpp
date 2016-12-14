#ifndef _SUB_ROUTER_H
#define _SUB_ROUTER_H

#include <iostream>
#include <unordered_map>
#include <map>
#include <tuple>
#include <type_traits>
#include "base/header.hpp"
#include "base/function_traits.hpp"
#include "base/logger.hpp"
#include "base/singleton.hpp"
#include "base/parser_util.hpp"

namespace easyrpc
{

class sub_invoker_function
{
public:
    using function_t = std::function<void(parser_util& parser, std::string& result)>;
    sub_invoker_function() = default;
    sub_invoker_function(const function_t& func, std::size_t param_size) 
        : func_(func), param_size_(param_size) {}

    void operator()()
    {
    }

    std::size_t param_size() const
    {
        return param_size_;
    }

private:
    function_t func_ = nullptr;
    std::size_t param_size_ = 0;
};

class sub_invoker_function_raw
{
public:
    using function_t = std::function<void(const std::string& body, std::string& result)>;
    sub_invoker_function_raw() = default;
    sub_invoker_function_raw(const function_t& func) : func_(func) {}

    void operator()()
    {
    }

private:
    function_t func_ = nullptr;
};

class sub_router
{
    DEFINE_SINGLETON(sub_router);
public:
    sub_router() = default;

    template<typename Function>
    void bind(const std::string& protocol, const Function& func)
    {
        bind_non_member_func(protocol, func);
    }

    template<typename Function, typename Self>
    void bind(const std::string& protocol, const Function& func, Self* self)
    {
        bind_member_func(protocol, func, self); 
    }

    void unbind(const std::string& protocol)
    {
        invoker_map_.erase(protocol);
    }

    bool is_bind(const std::string& protocol)
    {
        auto iter = invoker_map_.find(protocol);
        if (iter != invoker_map_.end())
        {
            return true;
        }
        return false;
    }

    template<typename Function>
    void bind_raw(const std::string& protocol, const Function& func)
    {
        bind_non_member_func_raw(protocol, func);
    }

    template<typename Function, typename Self>
    void bind_raw(const std::string& protocol, const Function& func, Self* self)
    {
        bind_member_func_raw(protocol, func, self); 
    }

    void unbind_raw(const std::string& protocol)
    {
        invoker_raw_map_.erase(protocol);
    }

    bool is_bind_raw(const std::string& protocol)
    {
        auto iter = invoker_raw_map_.find(protocol);
        if (iter != invoker_raw_map_.end())
        {
            return true;
        }
        return false;
    }

    bool route(const std::string& protocol, const std::string& body)
    {
        return true;
    }

private:
    template<typename Function, typename... Args>
    static typename std::enable_if<std::is_void<typename std::result_of<Function(Args...)>::type>::value>::type
    call(const Function& func, const std::tuple<Args...>& tp, std::string&)
    {
        call_impl(func, std::make_index_sequence<sizeof...(Args)>{}, tp);
    }

    template<typename Function, std::size_t... I, typename... Args>
    static auto call_impl(const Function& func, const std::index_sequence<I...>&, const std::tuple<Args...>& tp)
    {
        return func(std::get<I>(tp)...);
    }

    template<typename Function, typename Self, typename... Args>
    static typename std::enable_if<std::is_void<typename std::result_of<Function(Self, Args...)>::type>::value>::type
    call_member(const Function& func, Self* self, const std::tuple<Args...>& tp, std::string&)
    {
        call_member_impl(func, self, std::make_index_sequence<sizeof...(Args)>{}, tp);
    }

    template<typename Function, typename Self, std::size_t... I, typename... Args>
    static auto call_member_impl(const Function& func, Self* self, const std::index_sequence<I...>&, const std::tuple<Args...>& tp)
    {
        return (*self.*func)(std::get<I>(tp)...);
    }

    template<typename Function>
    static typename std::enable_if<std::is_void<typename std::result_of<Function(std::string)>::type>::value>::type
    call_raw(const Function& func, const std::string& body, std::string&)
    {
        func(body);
    }

    template<typename Function, typename Self>
    static typename std::enable_if<std::is_void<typename std::result_of<Function(Self, std::string)>::type>::value>::type
    call_member_raw(const Function& func, Self* self, const std::string& body, std::string&)
    {
        (*self.*func)(body);
    }

private:
    // 遍历function的实参类型，将parser_util解析出来的参数转换为实参并添加到std::tuple中.
    template<typename Function, std::size_t I = 0, std::size_t N = function_traits<Function>::arity>
    class invoker
    {
    public:
        template<typename Args>
        static void apply(const Function& func, const Args& args, parser_util& parser, std::string& result)
        {
            using arg_type_t = typename function_traits<Function>::template args<I>::type;
            try
            {
                invoker<Function, I + 1, N>::apply(func, std::tuple_cat(args, 
                                                   std::make_tuple(parser.get<arg_type_t>())), parser, result);
            }
            catch (std::exception& e)
            {
                log_warn(e.what());
            }
        }

        template<typename Args, typename Self>
        static void apply_member(const Function& func, Self* self, const Args& args, parser_util& parser, std::string& result)
        {
            using arg_type_t = typename function_traits<Function>::template args<I>::type;
            try
            {
                invoker<Function, I + 1, N>::apply_member(func, self, std::tuple_cat(args, 
                                                          std::make_tuple(parser.get<arg_type_t>())), parser, result);
            }
            catch (std::exception& e)
            {
                log_warn(e.what());
            }
        }
    }; 

    template<typename Function, std::size_t N>
    class invoker<Function, N, N>
    {
    public:
        template<typename Args>
        static void apply(const Function& func, const Args& args, parser_util&, std::string& result)
        {
            try
            {
                // 参数列表已经准备好，可以调用function了.
                call(func, args, result);
            }
            catch (std::exception& e)
            {
                log_warn(e.what());
            }
        }

        template<typename Args, typename Self>
        static void apply_member(const Function& func, Self* self, const Args& args, parser_util&, std::string& result)
        {
            try
            {
                call_member(func, self, args, result);
            }  
            catch (std::exception& e)
            {
                log_warn(e.what());
            }
        }
    };

    template<typename Function>
    class invoker_raw
    {
    public:
        static void apply(const Function& func, const std::string& body, std::string& result)
        {
            try
            {
                call_raw(func, body, result);
            }
            catch (std::exception& e)
            {
                log_warn(e.what());
            }
        }

        template<typename Self>
        static void apply_member(const Function& func, Self* self, const std::string& body, std::string& result)
        {
            try
            {
                call_member_raw(func, self, body, result);
            }
            catch (std::exception& e)
            {
                log_warn(e.what());
            }
        }
    }; 

private:
    template<typename Function>
    void bind_non_member_func(const std::string& protocol, const Function& func)
    {
        invoker_map_[protocol] = { std::bind(&invoker<Function>::template apply<std::tuple<>>, func, std::tuple<>(), 
                                             std::placeholders::_1, std::placeholders::_2), function_traits<Function>::arity };
    }

    template<typename Function, typename Self>
    void bind_member_func(const std::string& protocol, const Function& func, Self* self)
    {
        invoker_map_[protocol] = { std::bind(&invoker<Function>::template apply_member<std::tuple<>, Self>, func, self, std::tuple<>(), 
                                             std::placeholders::_1, std::placeholders::_2), function_traits<Function>::arity };
    }

    template<typename Function>
    void bind_non_member_func_raw(const std::string& protocol, const Function& func)
    {
        invoker_raw_map_[protocol] = { std::bind(&invoker_raw<Function>::apply, func, 
                                                std::placeholders::_1, std::placeholders::_2) };
    }

    template<typename Function, typename Self>
    void bind_member_func_raw(const std::string& protocol, const Function& func, Self* self)
    {
        invoker_raw_map_[protocol] = { std::bind(&invoker_raw<Function>::template apply_member<Self>, func, self, 
                                                std::placeholders::_1, std::placeholders::_2) };
    }

private:
    std::unordered_map<std::string, sub_invoker_function> invoker_map_;
    std::unordered_map<std::string, sub_invoker_function_raw> invoker_raw_map_;
};

}

#endif
