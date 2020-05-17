#pragma once
#include "log_const.h"
#include "log_file.h"
#include "log_serialize.h"

namespace shark_log
{
    struct log_type;
    
    using log_method_formator = void(void*, log_file&);
    using log_method_serialize = bool(void*, log_file&);
    using log_method_translate = bool(log_type&, log_file&, log_file&);

    //描述日志数据不变化的部分
    //这个类型要为每一条日志输出语句生成一份静态局部变量，故有大部分信息是不会每次都改变的
    //将这些信息搜集到一起防止，减少每次输出需要拷贝的数据量
    struct log_type
    {
        log_method_formator* formator;      //格式化日志的函数指针
        log_method_serialize* serialize;
        log_method_translate* translate;
        const char* str;                    //日志格式化的字符串，由于利用{fmt}来实现，故需要遵守{fmt}的语法规则
        const char* file;                   //产生本条日志的文件名
        uint32_t line;                      //产生本条日志的行号
        uint16_t tid;                       //本日志的类型编号，用于写入到二进制文件。之后可以利用tid找出来log_type，从而重新格式化为可阅读的文本日志。
        log_level level;                    //产生本条日志的日志等级
    };

    template<class _Ty>
    struct log_convert
    {
        using type = typename std::remove_cv<typename std::remove_reference<_Ty>::type>::type;
    };
    template<class _Ty, size_t N>
    struct log_convert<_Ty[N]>
    {
        using type = typename std::remove_volatile<typename std::remove_reference<_Ty>::type>::type*;
    };

    template<class _Ty>
    using log_convert_t = typename log_convert<std::remove_reference_t<_Ty>>::type;
}
