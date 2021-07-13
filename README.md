# webserver

现以用c++实现高性能webserver，现阶段用webbench测压实现稳定60wpages/min,1w左右的QPS

# 项目目的

Prepare to learn and make your own webserver

# 现有功能

* epoll边缘触发（edge trigger）IO复用模型，非阻塞IO，多线程的Reactor高并发模型
* 利用状态机解析HTTP请求报文，实现处理静态资源的请求
* 为减少内存泄漏的可能，使用智能指针等RAII机制
* 基于优先队列最小堆实现的定时器，关闭超时的非活动连接

# 创建和启动

`make myserver`

`./myserver`

# webbench-1.5测试

![1000clients+10sec](https://github.com/ouluy/Web_server/blob/main/%E6%B5%8B%E8%AF%95%E5%9B%BE%E7%89%87/5%E6%B5%8B%E8%AF%95.png)

```
webbench -c 100 -t 2 http://127.0.0.1:8888/1.html
webbench -c 1000 -t 10 http://127.0.0.1:8888/1.html
```

* 测试环境：Ubuntu:20.04 cpu:i5-8300H


# 旧版本

模板：初学模板

版本1：照着写了个epoll-http-server，使用了epoll边缘触发（edge trigger）IO复用模型，多线程的Reactor高并发模型

版本2：加入线程池，修改了乱糟糟的代码，参考了聚聚的代码一半c一半c++

版本3：参考完，全部改成c++，使用智能指针，改成非阻塞IO，但项目发现了一个bug，开启服务器直接webbench测试会core dumped(fd=5 read_num==0 forever),但如果开启服务器，联网进入目录之后,再webbench测压就不会出错,找了两天找不到bug在哪，怀疑哪个地方的指针写烂了

版本4：把原本在处理httprequest的定时器分离出来，~~大佬的定时器跟我在《Linux高性能服务器编程》看到的不一样，虽然都是基于小根堆实现的定时器，~~ 前者是手写最小堆，后者直接用STL优先队列（大佬nb）

> update:bug修掉了(喜大普奔T^T)，个人理解：在读取read_num==0时，没有把他当作对端关闭处理(有请求出现但是读不到数据)，再循环10次确定是否这个对端真的没数据了，当webbench，100个对端关闭时候，直接Segmentation fault (core dumped)了

> 新的问题：不过还有一个疑问，为什么开启服务器，联网进入目录之后,再webbench测压就不会出错?这又是为啥……





# 改进
log日志和接入数据库，还有主界面



