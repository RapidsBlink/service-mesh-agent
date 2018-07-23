# service-mesh-agent 

Service mesh agent in pure c (使用intrinsics编写向量化代码). 

## 下载依赖

```zsh 
git submodule init 
git submodule update
```

## 本地构建代码

```zsh
mkdir -p build && cd build
cmake .. && make -j 
```

## 最高跑分统计数据

```zsh
[INFO] >>> Pressure with 128 connections.
[DEBUG] Script to execute:

            sleep 5
            wrk -t2 -c128 -d60s -T5                 --script=./benchmark/wrk.lua                 --latency http://beita.e100081000032.zmf/invoke
            exit 0

[DEBUG] Return code = 0
[DEBUG] The output is as following:
Running 1m test @ http://beita.e100081000032.zmf/invoke
  2 threads and 128 connections
  Thread Stats   Avg      Stdev     Max   +/- Stdev
    Latency    52.34ms    4.42ms 172.66ms   98.08%
    Req/Sec     1.23k    72.84     1.29k    94.33%
  Latency Distribution
     50%   51.73ms
     75%   52.19ms
     90%   53.06ms
     99%   60.09ms
  146884 requests in 1.00m, 19.75MB read
Requests/sec:   2446.88
Transfer/sec:    336.92KB
--------------------------
Durations:       60.03s
Requests:        146884
Avg RT:          52.34ms
Max RT:          172.66ms
Min RT:          51.04ms
Error requests:  0
Valid requests:  146884
QPS:             2446.88
--------------------------

[INFO] QPS = 2446.88
[INFO] >>> Pressure with 256 connections.
[DEBUG] Script to execute:

            sleep 5
            wrk -t2 -c256 -d60s -T5                 --script=./benchmark/wrk.lua                 --latency http://beita.e100081000032.zmf/invoke
            exit 0

[DEBUG] Return code = 0
[DEBUG] The output is as following:
Running 1m test @ http://beita.e100081000032.zmf/invoke
  2 threads and 256 connections
  Thread Stats   Avg      Stdev     Max   +/- Stdev
    Latency    55.99ms   10.50ms 468.34ms   96.68%
    Req/Sec     2.30k   143.87     2.59k    76.83%
  Latency Distribution
     50%   54.24ms
     75%   56.72ms
     90%   60.58ms
     99%   75.54ms
  275126 requests in 1.00m, 37.00MB read
Requests/sec:   4583.77
Transfer/sec:    631.16KB
--------------------------
Durations:       60.02s
Requests:        275126
Avg RT:          55.99ms
Max RT:          468.344ms
Min RT:          51.036ms
Error requests:  0
Valid requests:  275126
QPS:             4583.77
--------------------------

[INFO] QPS = 4583.77
[INFO] >>> Pressure with 512 connections.
[DEBUG] Script to execute:

            sleep 5
            wrk -t2 -c512 -d60s -T5                 --script=./benchmark/wrk.lua                 --latency http://beita.e100081000032.zmf/invoke
            exit 0

[DEBUG] Return code = 0
[DEBUG] The output is as following:
Running 1m test @ http://beita.e100081000032.zmf/invoke
  2 threads and 512 connections
  Thread Stats   Avg      Stdev     Max   +/- Stdev
    Latency    71.87ms   17.86ms 391.22ms   88.52%
    Req/Sec     3.59k   288.36     4.30k    74.92%
  Latency Distribution
     50%   67.11ms
     75%   76.07ms
     90%   90.21ms
     99%  141.02ms
  428805 requests in 1.00m, 57.66MB read
Requests/sec:   7144.13
Transfer/sec:      0.96MB
--------------------------
Durations:       60.02s
Requests:        428805
Avg RT:          71.87ms
Max RT:          391.222ms
Min RT:          51.205ms
Error requests:  0
Valid requests:  428805
QPS:             7144.13
--------------------------

[INFO] QPS = 7144.13
```