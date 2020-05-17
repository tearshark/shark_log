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

    //������־���ݲ��仯�Ĳ���
    //�������ҪΪÿһ����־����������һ�ݾ�̬�ֲ����������д󲿷���Ϣ�ǲ���ÿ�ζ��ı��
    //����Щ��Ϣ�Ѽ���һ���ֹ������ÿ�������Ҫ������������
    struct log_type
    {
        log_method_formator* formator;      //��ʽ����־�ĺ���ָ��
        log_method_serialize* serialize;
        log_method_translate* translate;
        const char* str;                    //��־��ʽ�����ַ�������������{fmt}��ʵ�֣�����Ҫ����{fmt}���﷨����
        const char* file;                   //����������־���ļ���
        uint32_t line;                      //����������־���к�
        uint16_t tid;                       //����־�����ͱ�ţ�����д�뵽�������ļ���֮���������tid�ҳ���log_type���Ӷ����¸�ʽ��Ϊ���Ķ����ı���־��
        log_level level;                    //����������־����־�ȼ�
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
