# shark_log
极低延迟的日志实现(玩具级别)<br>
当前最低延迟：**50ns**(在I3 8100的CPU上测得)<br>
<br>

思路：利用延迟格式化来减小发起日志的线程的延迟。
开启一个专门的线程来进行日志格式化(测试期间被注释)和落地的工作(未完整实现)。<br>

将日志的数据拆分为两部分：<br>
1、log_type：描述日志数据不变化的部分<br>

    //void(void*)类型的格式化函数指针
    log_formator* formator;
    
    //日志格式化的字符串，由于利用{fmt}来实现，故需要遵守{fmt}的语法规则
    const char* str;

    //产生本条日志的文件名
    const char* file;

    //产生本条日志的行号
    uint32_t line;

    //产生本条日志的日志等级
    log_level level;

2、log_info: 描述日志数据变化的部分<br>

    //日志类型信息
    const log_type* type;

    //本条日志所绑定的缓存对象。缓存对象隶属于产生日志所在的线程。
    log_buffer* buffer;

    //产生本条日志的时钟
    uint64_t tick;

    //本条日志的输入参数
    tuple<Args...> args;

使用方法：

    sharkl_info(str, ...);
    sharkl_warn(str, ...);
    sharkl_eror(str, ...);

宏函数 sharkl_info() 定义一个 log_type 类型的局部变量 s_type，用于描述日志数据不变化的部分。
然后获得跟本线程相关的 log_buffer，将 s_type 以及 变化部分的参数交给 log_buffer
生成一条 log_info 描述的日志信息 log。然后通知日志格式化线程进行格式化并落地。<br>

这是一个心血来潮玩具级别的实现。接受批评指点交流，不接受嘲讽。<br>
如果你发现了任何bug、有好的建议、或使用上有不明之处，可以提交到issue，也可以直接联系作者:
    
    email: tearshark@163.net
    QQ交流群: 296561497
