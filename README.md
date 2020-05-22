# shark_log
极低延迟的日志实现(玩具级别)<br>
当前观察到的最低的平均延迟：**12ns**(在I7 8700K的CPU上测得)<br>
<br>

思路：利用延迟格式化来减小发起日志的线程的延迟。
开启一个专门的线程来进行日志格式化(测试期间被注释)和落地的工作(未完整实现)。<br>

将日志的数据拆分为两部分：<br>
1、log_type：描述日志数据不变化的部分<br>

    //bool(void*, log_file*)类型的格式化函数指针
    log_method_formator* formator;

    //bool(void*, log_file*)类型的以二进制形式序列化到文件的函数指针
    log_method_serialize* serialize

    //bool(log_type&, log_file&, log_file&)类型的转换二进制文件为文本文件的函数指针
    log_method_translate* translate;
    
    //日志格式化的字符串，由于利用{fmt}来实现，故需要遵守{fmt}的语法规则
    const char* str;

    //产生本条日志的文件名
    const char* file;

    //产生本条日志的行号
    uint32_t line;

    //本日志的类型编号，用于写入到二进制文件。
    //之后可以利用tid找出来log_type，从而重新格式化为可阅读的文本日志。
    uint16_t tid;

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

    //从二进制文件读入后的数据，跟args类型紧密相关
    read_type = std::tuple<read_type_t<_Args>...>;

使用方法：

    sharkl_info(str, ...);
    sharkl_warn(str, ...);
    sharkl_eror(str, ...);

宏函数 sharkl_info() 定义一个 log_type 类型的局部变量 s_type，用于描述日志数据不变化的部分。
然后获得跟本线程相关的 log_buffer，将 s_type 以及 变化部分的参数交给 log_buffer
生成一条 log_info 描述的日志信息 log。然后通知日志格式化线程进行格式化并落地。<br>

### Runtime Latency
(以下测试数据，除shark_log之外，来自[NanoLog](https://github.com/PlatformLab/NanoLog))<br>
Measured in nanoseconds and each cell represents the 50th / 99.9th tail latencies. The log messages used can be found in the [Log Message Map below](#Log-Messages-Map).

| Message | shark_log | NanoLog | spdlog | Log4j2 | glog | Boost | ETW |
|---------|:--------:|:--------:|:--------:|:--------:|:--------:|:--------:|:--------:|
|staticString | 7/1648| 7/37| 214/2546| 174/3364 | 1198/5968| 1764/3772| 161/2967|
|stringConcat | 7/1669| 7/36| 279/905| 256/25087 | 1212/5881| 1829/5548| 191/3365|
|singleInteger | 7/1415| 7/32| 268/855| 180/9305 | 1242/5482| 1914/5759| 167/3007|
|twoIntegers | 7/1455| 8/62| 437/1416| 183/10896 | 1399/6100| 2333/7235| 177/3183|
|singleDouble | 7/1446| 8/43| 585/1562| 175/4351 | 1983/6957| 2610/7079| 165/3182|
|complexFormat | 8/1600| 8/40| 1776/5267| 202/18207 | 2569/8877| 3334/11038| 218/3426|

#### Log Messages Map

Log messages used in the benchmarks above. **Bold** indicate dynamic log arguments.

| Message ID | Log Message Used |
|--------------|:--------|
|staticString  | Starting backup replica garbage collector thread |
|singleInteger | Backup storage speeds (min): **181** MB/s read |
|twoIntegers   | buffer has consumed **1032024** bytes of extra storage, current allocation: **1016544** bytes |
|singleDouble  | Using tombstone ratio balancer with ratio = **0.4** |
|complexFormat | Initialized InfUdDriver buffers: **50000** receive buffers (**97** MB), **50** transmit buffers (**0** MB), took **26.2** ms |
|stringConcat  | Opened session with coordinator at **basic+udp:host=192.168.1.140,port=12246** |

**注**：实际上，上述数据都是有水分，并不能完整反应延迟状况。<br>
因为数据总是要落地的，而这么低延迟的快速产生日志，格式化线程是来不及格式化的。<br>
当环形缓冲用满了的时候，日志产生线程会不得不停下来等待格式化线程完成数据的落地。<br>

这是一个心血来潮玩具级别的实现。接受批评指点交流，不接受嘲讽。<br>
如果你发现了任何bug、有好的建议、或使用上有不明之处，可以提交到issue，也可以直接联系作者:
    
    email: tearshark@163.net
    QQ交流群: 296561497
