# webserver

现以用c++实现高性能webserver，现阶段用webbench测压实现稳定60wpages/min,1w左右的QPS

# 项目目的

Prepare to learn and make your own webserver

# 旧版本

模板：初学模板

版本1：照着写了个epoll-http-server，使用了epoll边缘触发（edge trigger）IO复用模型，多线程的Reactor高并发模型

版本2：加入线程池，修改了乱糟糟的代码，参考了聚聚的代码一半c一半c++

版本3：参考完，全部改成c++，使用智能指针，改成非阻塞IO，但项目发现了一个bug，开启服务器直接webbench测试会core dumped(fd=5 forever),但如果开启服务器，联网进入目录之后,再webbench测压就不会出错,找了两天找不到bug在哪，怀疑哪个地方的指针写烂了

版本4：把原本在处理httprequest的定时器分离出来，大佬的定时器跟我在《Linux高性能服务器编程》看到的不一样，虽然都是基于小根堆实现的定时器


# 改进
定时器，log日志和接入数据库，还有主界面

# 现有功能

* epoll边缘触发（edge trigger）IO复用模型，非阻塞IO，多线程的Reactor高并发模型
* 利用状态机解析HTTP请求报文，实现处理静态资源的请求
* 为减少内存泄漏的可能，使用智能指针等RAII机制
* 基于小根堆实现的定时器，关闭超时的非活动连接

