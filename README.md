# webserver

现以用c++实现高性能webserver，现阶段用webbench测压实现稳定90wpages/min,1w5左右的QPS

# 项目目的

准备学习并创建自己的Webserver

# 现有功能

* epoll边缘触发（edge trigger）IO复用模型，非阻塞IO，多线程的Reactor高并发模型
* 利用有限状态机解析HTTP请求报文，实现处理静态资源的请求
* 为减少内存泄漏的可能，使用智能指针等RAII机制
* 基于优先队列最小堆实现的定时器，关闭超时的非活动连接
* 基于单例模式与阻塞队列实现异步的日志系统，记录服务器运行状态

# 创建和启动

```
make myserver

./myserver
```

# webbench-1.5测试

![1000clients+10sec](https://github.com/ouluy/Web_server/blob/main/example/5.png)

```
webbench -c 100 -t 2 http://127.0.0.1:8888/1.html
webbench -c 1000 -t 10 http://127.0.0.1:8888/1.html
```

* 测试环境：Ubuntu:20.04 cpu:i5-8300H


# 旧版本

模板：初学模板，webserver

版本1：照着写了个epoll-http-server，使用了epoll边缘触发（edge trigger）IO复用模型，多线程的Reactor高并发模型

版本2：加入线程池，修改了乱糟糟的代码，参考了聚聚的代码一半c一半c++

版本3：参考完，全部改成c++，使用智能指针，改成非阻塞IO，但项目发现了一个bug，开启服务器直接webbench测试会core dumped(fd=5 read_num==0 forever),但如果开启服务器，联网进入目录之后,再webbench测压就不会出错,找了两天找不到bug在哪，怀疑哪个地方的指针写烂了

版本4：把原本在处理httprequest的定时器分离出来，~~大佬的定时器跟我在《Linux高性能服务器编程》看到的不一样，虽然都是基于小根堆实现的定时器，~~ 前者是手写最小堆，后者直接用STL优先队列（大佬nb）

> update:bug修掉了(喜大普奔T^T)，~~个人理解：在读取read_num==0时，没有把他当作对端关闭处理(有请求出现但是读不到数据)，再循环10次确定是否这个对端真的没数据了，当webbench，100个对端关闭时候，直接Segmentation fault (core dumped)了~~

> 新的问题：不过还有一个疑问，为什么开启服务器，联网进入目录之后,再webbench测压就不会出错?这又是为啥……

> update:今天(7/14)写题的时候突然细细思考了一波,杀爆的个人理解,core dump[核心转储:进程异常终止，进程用户空间的数据就会被写到磁盘],跟我修改的那个没有任何关联,但最骚的是改完之后,还没有出过类似的问题,太折磨人了.(我试过ulimit -a ulimit -c 1024 但生成不了core文件...),很气,希望早日找到

版本5:异步日志系统加入两个模块，一个是日志(log)模块，一个是阻塞队列(block_queue)模块,单例模式创建日志,记录服务器运行状态,修改LOGQUQUESIZE=0可变成同步日志

# 需要改进

接入数据库（要改一堆），还有主界面

# 参考

参考了好多大佬的GitHub中的webserver，因为能力有限，写了大佬们的简单版

[linyacool](https://github.com/linyacool/WebServer)

[markparticle](https://github.com/markparticle/WebServer)

[qinguoyi](https://github.com/qinguoyi/TinyWebServer)

《Linux高性能服务器编程》，《UNIX网络编程1，2》，《Linux多线程服务端编程》
