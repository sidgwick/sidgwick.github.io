---
title: "Backtrader 学习系列 - QuickStart"
date: 2022-07-08 21:34:04
tags: backtrader quant
---

> IMPORTANT: The data files used in the quickstart guide are updated from time to time, which means that the adjusted close changes and with it the close (and the other components). That means that the actual output may be different to what was put in the documentation at the time of writing.

## 平台使用

首先粗略地解释使用 **backtrader** 时的 2 个基本概念, 然后来看一系列从空跑直到几乎完整的策略的例子.

1. Lines

   Data Feeds, Indicators 以及 Strategy 拥有 _lines_. 所谓的 `line` 就是一些列的点组成的线, 通常 Data Feed 有以下几条 line: Open, High, Low, Close, Volume, OpenInterest. 举例来说 Open 点随时间移动可以组成一条 `line`, 其他几个指标也是如此. 因此一个 Data Feed 通常有 6 条 line. 考虑 DateTime 的话(DateTime 是 backtrader DataFeed 的索引), 那就有 7 条 line.

2. 索引 0

   当访问 line 上面的数据时, 当前值得索引是 `0`, 访问上一个值使用索引 `-1`. Python 语言中 `-1` 一般用来表示一个可迭代对象的最后一个值, 我们这里用 `-1` 来表示最后一个 **output** 值(这里的 `output` 应该就是已经处理过的数据列表)

现在想像一下我们在策略初始化的时候创建一条移动平均线(Simple Moving Average):

```python
self.sma = SimpleMovingAverage(.....)
```

访问当前值的最简单方式是:

```python
av = self.sma[0]
```

因为 0 可以唯一表示当前正在处理的数据, 因此无需知道当前已经处理了多少 bar/minute/day/month.

遵从 **pythonic** 传统, 最后一个输出值可以通过 `-1` 获得:

```python
previous_value = self.sma[-1]
```

当然更早的数据可以通过 -2, -3....来获取.

## 实例: 从 0 到 100

### 基本设置

简单跑一下下面的代码:

```python
from __future__ import (absolute_import, division, print_function, unicode_literals)

import backtrader as bt

if __name__ == '__main__':
    cerebro = bt.Cerebro()

    print('Starting Portfolio Value: %.2f' % cerebro.broker.getvalue())

    cerebro.run()

    print('Final Portfolio Value: %.2f' % cerebro.broker.getvalue())
```

上面的代码跑完之后, 可以得到如下结果输出:

```python
Starting Portfolio Value: 10000.00
Final Portfolio Value: 10000.00
```

在这个例子中:

- 引入了 backtrader 库
- 创建了 cerebro 引擎
- cerebro 被告知执行(遍历数据)
- 输出了最终的结果

这里指出来一些代码明面上看不到的东西:

- cerebro 引擎在背地里创建了一个 broker 对象(用来表示券商).
- 实例在开始执行的时候有一些初始资金(10000)

broker 是表示券商的对象, 如果不显式设置的话, 就会使用内置默认的券商实现.

### 设置现金

在金融世界中只有 _loser_ 才会用 1 万块入场. 让我们改一下资金额度然后重跑一下例子.

```diff
--- a/x.py
+++ b/x.py
@@ -4,6 +4,7 @@ import backtrader as bt

 if __name__ == '__main__':
     cerebro = bt.Cerebro()
+    cerebro.broker.setcash(100000.0)

     print('Starting Portfolio Value: %.2f' % cerebro.broker.getvalue())
```

上面例子执行过后, 得到如下输出:

```python
Starting Portfolio Value: 1000000.00
Final Portfolio Value: 1000000.00
```

任务完成, 让我们更进一步.

### 添加 Data Feed

我们的目的是通过自动化策略对数据的处理, 使得现金增加. 接下来我们在程序中增加一些数据:

```python
from __future__ import (absolute_import, division, print_function, unicode_literals)

import datetime  # For datetime objects
import os.path  # To manage paths
import sys  # To find out the script name (in argv[0])

# Import the backtrader platform
import backtrader as bt

if __name__ == '__main__':
    # Create a cerebro entity
    cerebro = bt.Cerebro()

    # Datas are in a subfolder of the samples. Need to find where the script is
    # because it could have been called from anywhere
    modpath = os.path.dirname(os.path.abspath(sys.argv[0]))
    datapath = os.path.join(modpath, '../../datas/orcl-1995-2014.txt')

    # Create a Data Feed
    data = bt.feeds.YahooFinanceCSVData(
        dataname=datapath,
        # Do not pass values before this date
        fromdate=datetime.datetime(2000, 1, 1),
        # Do not pass values after this date
        todate=datetime.datetime(2000, 12, 31),
        reverse=False)

    # Add the Data Feed to Cerebro
    cerebro.adddata(data)

    # Set our desired cash start
    cerebro.broker.setcash(100000.0)

    # Print out the starting conditions
    print('Starting Portfolio Value: %.2f' % cerebro.broker.getvalue())

    # Run over everything
    cerebro.run()

    # Print out the final result
    print('Final Portfolio Value: %.2f' % cerebro.broker.getvalue())
```

执行后输出:

```
Starting Portfolio Value: 1000000.00
Final Portfolio Value: 1000000.00
```

代码量略有增加, 因为我们添加了:

- 指定示例数据文件的位置
- 使用 datetime 对象来过滤数据
- 将数据添加到 cerebro

因为依然没有交易策略, 上面的操作暂时不会导致输出改变.

> Yahoo Online 以日期降序发送 CSV 数据, 这不是标准约定. `reversed=True` 参数考虑了文件中的 CSV 数据已被反转以标准的预期日期升序.

### 第一个策略

第一个策略我们简单地打印每天的收盘价格.

DataSeries(Data Feed 中的基础类)对象具有别名用于访问众所周知的 OHLC 数值, 这应该可以简化我们打印逻辑.

```diff
--- x.py	2022-07-09 15:55:44.628483147 +0800
+++ y.py	2022-07-09 15:59:15.900063578 +0800
@@ -7,10 +7,31 @@
 # Import the backtrader platform
 import backtrader as bt

+
+# Create a Stratey
+class TestStrategy(bt.Strategy):
+
+    def log(self, txt, dt=None):
+        ''' Logging function for this strategy'''
+        dt = dt or self.datas[0].datetime.date(0)
+        print('%s, %s' % (dt.isoformat(), txt))
+
+    def __init__(self):
+        # Keep a reference to the "close" line in the data[0] dataseries
+        self.dataclose = self.datas[0].close
+
+    def next(self):
+        # Simply log the closing price of the series from the reference
+        self.log('Close, %.2f' % self.dataclose[0])
+
+
 if __name__ == '__main__':
     # Create a cerebro entity
     cerebro = bt.Cerebro()

+    # Add a strategy
+    cerebro.addstrategy(TestStrategy)
+
     # Datas are in a subfolder of the samples. Need to find where the script is
     # because it could have been called from anywhere
     modpath = os.path.dirname(os.path.abspath(sys.argv[0]))
@@ -21,8 +42,9 @@
         dataname=datapath,
         # Do not pass values before this date
         fromdate=datetime.datetime(2000, 1, 1),
-        # Do not pass values after this date
+        # Do not pass values before this date
         todate=datetime.datetime(2000, 12, 31),
+        # Do not pass values after this date
         reverse=False)

     # Add the Data Feed to Cerebro
```

执行之后我们可以得到以下输出:

```
Starting Portfolio Value: 100000.00
2000-01-03T00:00:00, Close, 27.85
2000-01-04T00:00:00, Close, 25.39
2000-01-05T00:00:00, Close, 24.05
...
...
...
2000-12-26T00:00:00, Close, 29.17
2000-12-27T00:00:00, Close, 28.94
2000-12-28T00:00:00, Close, 29.29
2000-12-29T00:00:00, Close, 27.41
Final Portfolio Value: 100000.00
```

解释一下:

- 在 `init` 被调用时, 策略已经拥有平台中存在的数据列表.

  这个数据是一个标准的 Python 列表, 数据可以通过他们的插入顺序访问到. `self.datas[0]` 将会作为默认的系统时钟(The first data in the list `self.datas[0]` is the default data for trading operations and to keep all strategy elements synchronized (it’s the system clock)).

- `self.dataclose = self.datas[0].close` 是对 Close line 的引用. 将来在策略中可以通过它访问收盘价.

- 策略的 `next` 方法将会对系统时钟上的每个 `bar` 都做一次调用.

  如果一些指标需要几个 bar 计算得出, 那么首次调用会发生在第一个能指标产出的 bar 上. 后续这块有详细介绍.

### 为策略添加一些逻辑

如果价格连续下跌 3 个交易日, 那就买买买!!!

```diff
--- y.py	2022-07-09 16:20:53.612313313 +0800
+++ x.py	2022-07-09 16:22:25.128408269 +0800
@@ -24,6 +24,16 @@
         # Simply log the closing price of the series from the reference
         self.log('Close, %.2f' % self.dataclose[0])

+        if self.dataclose[0] < self.dataclose[-1]:
+            # current close less than previous close
+
+            if self.dataclose[-1] < self.dataclose[-2]:
+                # previous close less than the previous close
+
+                # BUY, BUY, BUY!!! (with all possible default parameters)
+                self.log('BUY CREATE, %.2f' % self.dataclose[0])
+                self.buy()
+

 if __name__ == '__main__':
     # Create a cerebro entity
```

上面的例子执行完成之后输出:

```
Starting Portfolio Value: 100000.00
2000-01-03, Close, 27.85
2000-01-04, Close, 25.39
2000-01-05, Close, 24.05
2000-01-05, BUY CREATE, 24.05
2000-01-06, Close, 22.63
2000-01-06, BUY CREATE, 22.63
2000-01-07, Close, 24.37
...
...
...
2000-12-20, BUY CREATE, 26.88
2000-12-21, Close, 27.82
2000-12-22, Close, 30.06
2000-12-26, Close, 29.17
2000-12-27, Close, 28.94
2000-12-27, BUY CREATE, 28.94
2000-12-28, Close, 29.29
2000-12-29, Close, 27.41
Final Portfolio Value: 99725.08
```

例子中发出了一些买入指令, 我们的资产组合也出现了下降. 这里我们显然漏掉了一些事情:

- 订单只是创建了, 但是不知道订单是否执行, 也不知道订单的执行时间和价格. 接下来的例子中将会监听订单的状态通知.

好奇的读者可能会问购买了多少股, 具体是什么资产以及如何执行的订单. 在可能的情况下(本例是这样), 平台填补了:

- 在不指定资产(目标股票)的时候, 主数据(`self.datas[0]`, 也即系统时钟)就是目标数据.

- 数量使用固大小的 `sizer` 在幕后提供, 默认为 `1`. 后面的示例中我们修修改它.

- 订单以 **市价** 执行, Broker 使用下一根 bar 的开盘价执行此项, 因为这是当前 bar 之后的第一个价格变动.

- 订单在没有任何确认机制(以后会补上)的情况下执行

别光顾着买, 我们还要卖!

- Strategy 对象提供了对默认 Data Feed 的仓位(`position`)属性的访问

- `buy` 和 `sell` 方法返回创建(`created`, 尚未执行)的订单

- 订单状态的更改将通过通知(`notify`)方法通知策略

我们的卖出策略也很简单:

无论盈利与否, 都在 5 个 bar 之后(也即底 6 个 bar)的时候卖出. 请注意用 bar 的数量可以代表时间(1 分钟/小时/天/周/月等等)周期.

在入市之前, 只允许买单(`buy`).

> `next` 方法收不到 `bar` 的 index 参数. 获取*第五根*这个信息可以通过更 pythonic 的方式: 对 `line` 对象调用 `len` 方法, 它会告诉你 `line` 的长度, 只需要在变量中记录下来操作时刻的线长度, 然后和当前长度比较就知道是否是第五根 bar.

```diff
--- y.py	2022-07-09 16:41:34.389643481 +0800
+++ x.py	2022-07-09 16:42:35.663290037 +0800
@@ -20,19 +20,63 @@
         # Keep a reference to the "close" line in the data[0] dataseries
         self.dataclose = self.datas[0].close

+        # To keep track of pending orders
+        self.order = None
+
+    def notify_order(self, order):
+        if order.status in [order.Submitted, order.Accepted]:
+            # Buy/Sell order submitted/accepted to/by broker - Nothing to do
+            return
+
+        # Check if an order has been completed
+        # Attention: broker could reject order if not enough cash
+        if order.status in [order.Completed]:
+            if order.isbuy():
+                self.log('BUY EXECUTED, %.2f' % order.executed.price)
+            elif order.issell():
+                self.log('SELL EXECUTED, %.2f' % order.executed.price)
+
+            self.bar_executed = len(self)
+
+        elif order.status in [order.Canceled, order.Margin, order.Rejected]:
+            self.log('Order Canceled/Margin/Rejected')
+
+        # Write down: no pending order
+        self.order = None
+
     def next(self):
         # Simply log the closing price of the series from the reference
         self.log('Close, %.2f' % self.dataclose[0])

-        if self.dataclose[0] < self.dataclose[-1]:
-            # current close less than previous close
+        # Check if an order is pending ... if yes, we cannot send a 2nd one
+        if self.order:
+            return
+
+        # Check if we are in the market
+        if not self.position:
+
+            # Not yet ... we MIGHT BUY if ...
+            if self.dataclose[0] < self.dataclose[-1]:
+                # current close less than previous close
+
+                if self.dataclose[-1] < self.dataclose[-2]:
+                    # previous close less than the previous close
+
+                    # BUY, BUY, BUY!!! (with default parameters)
+                    self.log('BUY CREATE, %.2f' % self.dataclose[0])
+
+                    # Keep track of the created order to avoid a 2nd order
+                    self.order = self.buy()
+
+        else:

-            if self.dataclose[-1] < self.dataclose[-2]:
-                # previous close less than the previous close
+            # Already in the market ... we might sell
+            if len(self) >= (self.bar_executed + 5):
+                # SELL, SELL, SELL!!! (with all possible default parameters)
+                self.log('SELL CREATE, %.2f' % self.dataclose[0])

-                # BUY, BUY, BUY!!! (with all possible default parameters)
-                self.log('BUY CREATE, %.2f' % self.dataclose[0])
-                self.buy()
+                # Keep track of the created order to avoid a 2nd order
+                self.order = self.sell()


 if __name__ == '__main__':
```

上面的脚本执行完成之后, 得到以下输出:

```
Starting Portfolio Value: 100000.00
2000-01-03T00:00:00, Close, 27.85
2000-01-04T00:00:00, Close, 25.39
2000-01-05T00:00:00, Close, 24.05
2000-01-05T00:00:00, BUY CREATE, 24.05
2000-01-06T00:00:00, BUY EXECUTED, 23.61
2000-01-06T00:00:00, Close, 22.63
2000-01-07T00:00:00, Close, 24.37
2000-01-10T00:00:00, Close, 27.29
2000-01-11T00:00:00, Close, 26.49
2000-01-12T00:00:00, Close, 24.90
2000-01-13T00:00:00, Close, 24.77
2000-01-13T00:00:00, SELL CREATE, 24.77
2000-01-14T00:00:00, SELL EXECUTED, 25.70
2000-01-14T00:00:00, Close, 25.18
...
...
...
2000-12-15T00:00:00, SELL CREATE, 26.93
2000-12-18T00:00:00, SELL EXECUTED, 28.29
2000-12-18T00:00:00, Close, 30.18
2000-12-19T00:00:00, Close, 28.88
2000-12-20T00:00:00, Close, 26.88
2000-12-20T00:00:00, BUY CREATE, 26.88
2000-12-21T00:00:00, BUY EXECUTED, 26.23
2000-12-21T00:00:00, Close, 27.82
2000-12-22T00:00:00, Close, 30.06
2000-12-26T00:00:00, Close, 29.17
2000-12-27T00:00:00, Close, 28.94
2000-12-28T00:00:00, Close, 29.29
2000-12-29T00:00:00, Close, 27.41
2000-12-29T00:00:00, SELL CREATE, 27.41
Final Portfolio Value: 100018.53
```

可以看到, 结束的时候我们的钱变多了.

一般券商会收取佣金(`commission`), 我们买入卖出的佣金费率都设置到 `0.1%`. 只需一行代码即可完成.

```diff
--- x.py	2022-07-09 22:25:52.204778015 +0800
+++ y.py	2022-07-09 22:25:12.029415237 +0800
@@ -20,8 +20,10 @@
         # Keep a reference to the "close" line in the data[0] dataseries
         self.dataclose = self.datas[0].close

-        # To keep track of pending orders
+        # To keep track of pending orders and buy price/commission
         self.order = None
+        self.buyprice = None
+        self.buycomm = None

     def notify_order(self, order):
         if order.status in [order.Submitted, order.Accepted]:
@@ -32,18 +34,28 @@
         # Attention: broker could reject order if not enough cash
         if order.status in [order.Completed]:
             if order.isbuy():
-                self.log('BUY EXECUTED, %.2f' % order.executed.price)
-            elif order.issell():
-                self.log('SELL EXECUTED, %.2f' % order.executed.price)
+                self.log('BUY EXECUTED, Price: %.2f, Cost: %.2f, Comm %.2f' %
+                         (order.executed.price, order.executed.value, order.executed.comm))
+
+                self.buyprice = order.executed.price
+                self.buycomm = order.executed.comm
+            else:  # Sell
+                self.log('SELL EXECUTED, Price: %.2f, Cost: %.2f, Comm %.2f' %
+                         (order.executed.price, order.executed.value, order.executed.comm))

             self.bar_executed = len(self)

         elif order.status in [order.Canceled, order.Margin, order.Rejected]:
             self.log('Order Canceled/Margin/Rejected')

-        # Write down: no pending order
         self.order = None

+    def notify_trade(self, trade):
+        if not trade.isclosed:
+            return
+
+        self.log('OPERATION PROFIT, GROSS %.2f, NET %.2f' % (trade.pnl, trade.pnlcomm))
+
     def next(self):
         # Simply log the closing price of the series from the reference
         self.log('Close, %.2f' % self.dataclose[0])
@@ -107,6 +119,9 @@
     # Set our desired cash start
     cerebro.broker.setcash(100000.0)

+    # Set the commission - 0.1% ... divide by 100 to remove the %
+    cerebro.broker.setcommission(commission=0.001)
+
     # Print out the starting conditions
     print('Starting Portfolio Value: %.2f' % cerebro.broker.getvalue())
```

运行脚本, 看一下在有佣金情况下的收益:

```
Starting Portfolio Value: 100000.00
2000-01-03T00:00:00, Close, 27.85
2000-01-04T00:00:00, Close, 25.39
2000-01-05T00:00:00, Close, 24.05
2000-01-05T00:00:00, BUY CREATE, 24.05
2000-01-06T00:00:00, BUY EXECUTED, Price: 23.61, Cost: 23.61, Commission 0.02
2000-01-06T00:00:00, Close, 22.63
2000-01-07T00:00:00, Close, 24.37
2000-01-10T00:00:00, Close, 27.29
2000-01-11T00:00:00, Close, 26.49
2000-01-12T00:00:00, Close, 24.90
2000-01-13T00:00:00, Close, 24.77
2000-01-13T00:00:00, SELL CREATE, 24.77
2000-01-14T00:00:00, SELL EXECUTED, Price: 25.70, Cost: 25.70, Commission 0.03
2000-01-14T00:00:00, OPERATION PROFIT, GROSS 2.09, NET 2.04
2000-01-14T00:00:00, Close, 25.18
...
...
...
2000-12-15T00:00:00, SELL CREATE, 26.93
2000-12-18T00:00:00, SELL EXECUTED, Price: 28.29, Cost: 28.29, Commission 0.03
2000-12-18T00:00:00, OPERATION PROFIT, GROSS -0.06, NET -0.12
2000-12-18T00:00:00, Close, 30.18
2000-12-19T00:00:00, Close, 28.88
2000-12-20T00:00:00, Close, 26.88
2000-12-20T00:00:00, BUY CREATE, 26.88
2000-12-21T00:00:00, BUY EXECUTED, Price: 26.23, Cost: 26.23, Commission 0.03
2000-12-21T00:00:00, Close, 27.82
2000-12-22T00:00:00, Close, 30.06
2000-12-26T00:00:00, Close, 29.17
2000-12-27T00:00:00, Close, 28.94
2000-12-28T00:00:00, Close, 29.29
2000-12-29T00:00:00, Close, 27.41
2000-12-29T00:00:00, SELL CREATE, 27.41
Final Portfolio Value: 100016.98
```

数据显示依然是盈利的. 我们过滤出来 "OPERATION PROFIT" 相关数据单独看一下:

```
2000-01-14T00:00:00, OPERATION PROFIT, GROSS 2.09, NET 2.04
2000-02-07T00:00:00, OPERATION PROFIT, GROSS 3.68, NET 3.63
2000-02-28T00:00:00, OPERATION PROFIT, GROSS 4.48, NET 4.42
2000-03-13T00:00:00, OPERATION PROFIT, GROSS 3.48, NET 3.41
2000-03-22T00:00:00, OPERATION PROFIT, GROSS -0.41, NET -0.49
2000-04-07T00:00:00, OPERATION PROFIT, GROSS 2.45, NET 2.37
2000-04-20T00:00:00, OPERATION PROFIT, GROSS -1.95, NET -2.02
2000-05-02T00:00:00, OPERATION PROFIT, GROSS 5.46, NET 5.39
2000-05-11T00:00:00, OPERATION PROFIT, GROSS -3.74, NET -3.81
2000-05-30T00:00:00, OPERATION PROFIT, GROSS -1.46, NET -1.53
2000-07-05T00:00:00, OPERATION PROFIT, GROSS -1.62, NET -1.69
2000-07-14T00:00:00, OPERATION PROFIT, GROSS 2.08, NET 2.01
2000-07-28T00:00:00, OPERATION PROFIT, GROSS 0.14, NET 0.07
2000-08-08T00:00:00, OPERATION PROFIT, GROSS 4.36, NET 4.29
2000-08-21T00:00:00, OPERATION PROFIT, GROSS 1.03, NET 0.95
2000-09-15T00:00:00, OPERATION PROFIT, GROSS -4.26, NET -4.34
2000-09-27T00:00:00, OPERATION PROFIT, GROSS 1.29, NET 1.22
2000-10-13T00:00:00, OPERATION PROFIT, GROSS -2.98, NET -3.04
2000-10-26T00:00:00, OPERATION PROFIT, GROSS 3.01, NET 2.95
2000-11-06T00:00:00, OPERATION PROFIT, GROSS -3.59, NET -3.65
2000-11-16T00:00:00, OPERATION PROFIT, GROSS 1.28, NET 1.23
2000-12-01T00:00:00, OPERATION PROFIT, GROSS 2.59, NET 2.54
2000-12-18T00:00:00, OPERATION PROFIT, GROSS -0.06, NET -0.12
```

把净值(`NET`)加起来, 得到 `15.83`, 显然这个上面执行结果中的最终金额(`100016.98`)报告的收益 `16.98` 不符.

这里实际上并没有出错, 而是 `15.83` 是钱袋子里面的净利润, 我们在 29 号还有一笔卖出操作(`2000-12-29T00:00:00, SELL CREATE, 27.41`), 这个操作已经提交, 但是却并没有执行.

券商计算的价值是包含了以 2000-12-29 的收盘价成交的交易数据. 实际上策略执行订单的时候是在下一个交易日执行, 也就是 2001-01-02 日, 如果扩展 Data Feed 范围包含该日期, 我们会看到以下输出:

```
2001-01-02T00:00:00, SELL EXECUTED, Price: 27.87, Cost: 27.87, Commission 0.03
2001-01-02T00:00:00, OPERATION PROFIT, GROSS 1.64, NET 1.59
2001-01-02T00:00:00, Close, 24.87
2001-01-02T00:00:00, BUY CREATE, 24.87
Final Portfolio Value: 100017.41
```

在 2001-01-02 日执行了卖出操作, 此时把 OPERATION PROFIT 里面的利润再次相加(`15.83 + 1.59`), 得到 `17.42`. 刨除掉浮点数的精度损失, 现在结果就对得上了.

### 定制策略 - Parameters

在策略中对某些值进行硬编码有点不切实际, 并且需要更改它们的时候也会比较费事, 此时可以使用参数(`Parameters`).

参数的定义很简单, 看起来像这样:

```python
params = (
    ('myparam', 27),
    ('exitbars', 5),
)
```

在使用策略的时候, 可以将参数定制化:

```python
# Add a strategy
cerebro.addstrategy(TestStrategy, myparam=20, exitbars=7)
```

在策略中使用参数也很简单, 他们都被保存在 params 属性中. 比方说我们想设置交易数量, 我们可以再 `init` 的时候给 `sizer` 设置参数如下:

```python
# Set the sizer stake from the params
self.sizer.setsizing(self.params.stake)
```

当然我们也可以在 `buy` 或者 `sell` 调用的时候, 设置交易数量:

```python
self.buy(size=self.params.stake)
```

也可以把卖出时机参数化:

```python
# Already in the market ... we might sell
if len(self) >= (self.bar_executed + self.params.exitbars):
```

上面相关改动如下:

```diff
--- y.py	2022-07-09 23:05:30.201865643 +0800
+++ x.py	2022-07-09 23:01:58.712281938 +0800
@@ -10,6 +10,7 @@

 # Create a Stratey
 class TestStrategy(bt.Strategy):
+    params = (('exitbars', 5), )

     def log(self, txt, dt=None):
         ''' Logging function fot this strategy'''
@@ -83,7 +84,7 @@
         else:

             # Already in the market ... we might sell
-            if len(self) >= (self.bar_executed + 5):
+            if len(self) >= (self.bar_executed + self.params.exitbars):
                 # SELL, SELL, SELL!!! (with all possible default parameters)
                 self.log('SELL CREATE, %.2f' % self.dataclose[0])

@@ -119,6 +120,9 @@
     # Set our desired cash start
     cerebro.broker.setcash(100000.0)

+    # Add a FixedSize sizer according to the stake
+    cerebro.addsizer(bt.sizers.FixedSize, stake=10)
+
     # Set the commission - 0.1% ... divide by 100 to remove the %
     cerebro.broker.setcommission(commission=0.001)
```

> 全部代码参考:

```python
from __future__ import (absolute_import, division, print_function,
                        unicode_literals)

import datetime  # For datetime objects
import os.path  # To manage paths
import sys  # To find out the script name (in argv[0])

# Import the backtrader platform
import backtrader as bt


# Create a Stratey
class TestStrategy(bt.Strategy):
    params = (
        ('exitbars', 5),
    )

    def log(self, txt, dt=None):
        ''' Logging function fot this strategy'''
        dt = dt or self.datas[0].datetime.date(0)
        print('%s, %s' % (dt.isoformat(), txt))

    def __init__(self):
        # Keep a reference to the "close" line in the data[0] dataseries
        self.dataclose = self.datas[0].close

        # To keep track of pending orders and buy price/commission
        self.order = None
        self.buyprice = None
        self.buycomm = None

    def notify_order(self, order):
        if order.status in [order.Submitted, order.Accepted]:
            # Buy/Sell order submitted/accepted to/by broker - Nothing to do
            return

        # Check if an order has been completed
        # Attention: broker could reject order if not enough cash
        if order.status in [order.Completed]:
            if order.isbuy():
                self.log(
                    'BUY EXECUTED, Price: %.2f, Cost: %.2f, Comm %.2f' %
                    (order.executed.price,
                     order.executed.value,
                     order.executed.comm))

                self.buyprice = order.executed.price
                self.buycomm = order.executed.comm
            else:  # Sell
                self.log('SELL EXECUTED, Price: %.2f, Cost: %.2f, Comm %.2f' %
                         (order.executed.price,
                          order.executed.value,
                          order.executed.comm))

            self.bar_executed = len(self)

        elif order.status in [order.Canceled, order.Margin, order.Rejected]:
            self.log('Order Canceled/Margin/Rejected')

        self.order = None

    def notify_trade(self, trade):
        if not trade.isclosed:
            return

        self.log('OPERATION PROFIT, GROSS %.2f, NET %.2f' %
                 (trade.pnl, trade.pnlcomm))

    def next(self):
        # Simply log the closing price of the series from the reference
        self.log('Close, %.2f' % self.dataclose[0])

        # Check if an order is pending ... if yes, we cannot send a 2nd one
        if self.order:
            return

        # Check if we are in the market
        if not self.position:

            # Not yet ... we MIGHT BUY if ...
            if self.dataclose[0] < self.dataclose[-1]:
                    # current close less than previous close

                    if self.dataclose[-1] < self.dataclose[-2]:
                        # previous close less than the previous close

                        # BUY, BUY, BUY!!! (with default parameters)
                        self.log('BUY CREATE, %.2f' % self.dataclose[0])

                        # Keep track of the created order to avoid a 2nd order
                        self.order = self.buy()

        else:

            # Already in the market ... we might sell
            if len(self) >= (self.bar_executed + self.params.exitbars):
                # SELL, SELL, SELL!!! (with all possible default parameters)
                self.log('SELL CREATE, %.2f' % self.dataclose[0])

                # Keep track of the created order to avoid a 2nd order
                self.order = self.sell()

if __name__ == '__main__':
    # Create a cerebro entity
    cerebro = bt.Cerebro()

    # Add a strategy
    cerebro.addstrategy(TestStrategy)

    # Datas are in a subfolder of the samples. Need to find where the script is
    # because it could have been called from anywhere
    modpath = os.path.dirname(os.path.abspath(sys.argv[0]))
    datapath = os.path.join(modpath, '../../datas/orcl-1995-2014.txt')

    # Create a Data Feed
    data = bt.feeds.YahooFinanceCSVData(
        dataname=datapath,
        # Do not pass values before this date
        fromdate=datetime.datetime(2000, 1, 1),
        # Do not pass values before this date
        todate=datetime.datetime(2000, 12, 31),
        # Do not pass values after this date
        reverse=False)

    # Add the Data Feed to Cerebro
    cerebro.adddata(data)

    # Set our desired cash start
    cerebro.broker.setcash(100000.0)

    # Add a FixedSize sizer according to the stake
    cerebro.addsizer(bt.sizers.FixedSize, stake=10)

    # Set the commission - 0.1% ... divide by 100 to remove the %
    cerebro.broker.setcommission(commission=0.001)

    # Print out the starting conditions
    print('Starting Portfolio Value: %.2f' % cerebro.broker.getvalue())

    # Run over everything
    cerebro.run()

    # Print out the final result
    print('Final Portfolio Value: %.2f' % cerebro.broker.getvalue())
```

输出如下:

```
Starting Portfolio Value: 100000.00
2000-01-03T00:00:00, Close, 27.85
2000-01-04T00:00:00, Close, 25.39
2000-01-05T00:00:00, Close, 24.05
2000-01-05T00:00:00, BUY CREATE, 24.05
2000-01-06T00:00:00, BUY EXECUTED, Size 10, Price: 23.61, Cost: 236.10, Commission 0.24
2000-01-06T00:00:00, Close, 22.63
...
...
...
2000-12-20T00:00:00, BUY CREATE, 26.88
2000-12-21T00:00:00, BUY EXECUTED, Size 10, Price: 26.23, Cost: 262.30, Commission 0.26
2000-12-21T00:00:00, Close, 27.82
2000-12-22T00:00:00, Close, 30.06
2000-12-26T00:00:00, Close, 29.17
2000-12-27T00:00:00, Close, 28.94
2000-12-28T00:00:00, Close, 29.29
2000-12-29T00:00:00, Close, 27.41
2000-12-29T00:00:00, SELL CREATE, 27.41
Final Portfolio Value: 100169.80
```

为了直观显示每次操作数量, 日志里把 size 也打印出来了. 可以看到每次操作数量增加 10 倍, 最后的收益也跟着增加了 10 倍(从 `16.98` 到 `169.80`).

### 添加指标(`indicator`)

本例中我们使用移动平均线(Simple Moving Average)策略.

- 如果收盘价大于当前均价, 那么执行市价买入
- 如果有持仓, 并且收盘价小于均线, 执行卖出
- 操作过程中只允许有一个活跃操作(订单)

只需要一点改动即可实现上面的策略, 首先在 `init` 的时候加入均线指标:

```python
self.sma = bt.indicators.MovingAverageSimple(self.datas[0], period=self.params.maperiod)
```

买入卖出的时机也需要微调, 整体代码 diff 如下(现金设置为 1000, 不收佣金):

```diff
--- x.py	2022-07-09 23:18:14.011198706 +0800
+++ y.py	2022-07-09 23:18:41.734396585 +0800
@@ -10,7 +10,7 @@

 # Create a Stratey
 class TestStrategy(bt.Strategy):
-    params = (('exitbars', 5), )
+    params = (('maperiod', 15), )

     def log(self, txt, dt=None):
         ''' Logging function fot this strategy'''
@@ -26,6 +26,9 @@
         self.buyprice = None
         self.buycomm = None

+        # Add a MovingAverageSimple indicator
+        self.sma = bt.indicators.SimpleMovingAverage(self.datas[0], period=self.params.maperiod)
+
     def notify_order(self, order):
         if order.status in [order.Submitted, order.Accepted]:
             # Buy/Sell order submitted/accepted to/by broker - Nothing to do
@@ -69,22 +72,17 @@
         if not self.position:

             # Not yet ... we MIGHT BUY if ...
-            if self.dataclose[0] < self.dataclose[-1]:
-                # current close less than previous close
-
-                if self.dataclose[-1] < self.dataclose[-2]:
-                    # previous close less than the previous close
+            if self.dataclose[0] > self.sma[0]:

-                    # BUY, BUY, BUY!!! (with default parameters)
-                    self.log('BUY CREATE, %.2f' % self.dataclose[0])
+                # BUY, BUY, BUY!!! (with all possible default parameters)
+                self.log('BUY CREATE, %.2f' % self.dataclose[0])

-                    # Keep track of the created order to avoid a 2nd order
-                    self.order = self.buy()
+                # Keep track of the created order to avoid a 2nd order
+                self.order = self.buy()

         else:

-            # Already in the market ... we might sell
-            if len(self) >= (self.bar_executed + self.params.exitbars):
+            if self.dataclose[0] < self.sma[0]:
                 # SELL, SELL, SELL!!! (with all possible default parameters)
                 self.log('SELL CREATE, %.2f' % self.dataclose[0])

@@ -118,13 +116,13 @@
     cerebro.adddata(data)

     # Set our desired cash start
-    cerebro.broker.setcash(100000.0)
+    cerebro.broker.setcash(1000.0)

     # Add a FixedSize sizer according to the stake
     cerebro.addsizer(bt.sizers.FixedSize, stake=10)

-    # Set the commission - 0.1% ... divide by 100 to remove the %
-    cerebro.broker.setcommission(commission=0.001)
+    # Set the commission
+    cerebro.broker.setcommission(commission=0.0)

     # Print out the starting conditions
     print('Starting Portfolio Value: %.2f' % cerebro.broker.getvalue())
```

执行结果如下:

```
Starting Portfolio Value: 1000.00
2000-01-24T00:00:00, Close, 25.55
2000-01-25T00:00:00, Close, 26.61
2000-01-25T00:00:00, BUY CREATE, 26.61
2000-01-26T00:00:00, BUY EXECUTED, Size 10, Price: 26.76, Cost: 267.60, Commission 0.00
2000-01-26T00:00:00, Close, 25.96
2000-01-27T00:00:00, Close, 24.43
2000-01-27T00:00:00, SELL CREATE, 24.43
2000-01-28T00:00:00, SELL EXECUTED, Size 10, Price: 24.28, Cost: 242.80, Commission 0.00
2000-01-28T00:00:00, OPERATION PROFIT, GROSS -24.80, NET -24.80
2000-01-28T00:00:00, Close, 22.34
2000-01-31T00:00:00, Close, 23.55
2000-02-01T00:00:00, Close, 25.46
2000-02-02T00:00:00, Close, 25.61
2000-02-02T00:00:00, BUY CREATE, 25.61
2000-02-03T00:00:00, BUY EXECUTED, Size 10, Price: 26.11, Cost: 261.10, Commission 0.00
...
...
...
2000-12-20T00:00:00, SELL CREATE, 26.88
2000-12-21T00:00:00, SELL EXECUTED, Size 10, Price: 26.23, Cost: 262.30, Commission 0.00
2000-12-21T00:00:00, OPERATION PROFIT, GROSS -20.60, NET -20.60
2000-12-21T00:00:00, Close, 27.82
2000-12-21T00:00:00, BUY CREATE, 27.82
2000-12-22T00:00:00, BUY EXECUTED, Size 10, Price: 28.65, Cost: 286.50, Commission 0.00
2000-12-22T00:00:00, Close, 30.06
2000-12-26T00:00:00, Close, 29.17
2000-12-27T00:00:00, Close, 28.94
2000-12-28T00:00:00, Close, 29.29
2000-12-29T00:00:00, Close, 27.41
2000-12-29T00:00:00, SELL CREATE, 27.41
Final Portfolio Value: 973.90
```

仔细看日志中的输出, 你会发现第一个输出不再是 2000-01-03, 而是 2000-01-24, 前面的时间去哪里了?

丢掉的日期其实并没有真的倍丢掉, backtrader 只是适应了一下最新的情况: 有一条移动平均线被加入到了策略中来. 指标需要 N 天来计算第一个输出, 在本例中是 15, 而 2000-01-24 正是第 15 天(第 15 根 bar).

backtrader 假设只有在指标计算完成的时候才发起第一次 `next` 调用, 实例的策略中只有一个指标, 如果有多个指标, 那就要等最晚的那个指标有第一个 output 的时候, 才会发起第一次 `next` 调用.

观察输出我们看到, 这次的收益实际上是负的, 加指标并没有让我们更赚钱.... 😂

> 因为浮点数的精度问题, 相同的数据相同的策略可能出现一点点微小的差别.

### 可视化: 绘制图表

之前我们只是在每个 bar 打印了相关输出, 但是人类是非常视觉的动物, 这里我们介绍如何把结果绘制出来.

> 为了画图, 需要安装 `matplotlib` 库.

backtrader 内置了一个绘图功能, 只需要在 `cerebro.run()` 之后加一行代码调用即可:

```python
cerebro.plot()
```

问了展示在绘图结果中做一些简单的自定义功能, 我们将添加以下内容到图上:

- 指数移动平均线(EMA)
- 加权移动平均线(WMA)
- 慢速随机指标
- 异同移动平均线(MACD)
- 相对强弱指标(RSI)
- 简单移动平均线(SMA)将会随 RSI 指标一起绘制
- 平均真实波动范围(AverageTrueRange, ATR), 默认不显示

默认这些指标在策略 `init` 中的代码如下:

```python
# Indicators for the plotting show
bt.indicators.ExponentialMovingAverage(self.datas[0], period=25)
bt.indicators.WeightedMovingAverage(self.datas[0], period=25).subplot = True
bt.indicators.StochasticSlow(self.datas[0])
bt.indicators.MACDHisto(self.datas[0])
rsi = bt.indicators.RSI(self.datas[0])
bt.indicators.SmoothedMovingAverage(rsi, period=10)
bt.indicators.ATR(self.datas[0]).plot = False
```

> 即便这些指标没有显式的加入到策略的成员变量中, 它们也会自动被注册到策略中, 并且会影响调用 `next` 方法的最小 bar.

代码改动 diff:

```diff
--- x.py	2022-07-10 00:15:40.176306415 +0800
+++ y.py	2022-07-10 00:16:56.484414551 +0800
@@ -29,6 +29,15 @@
         # Add a MovingAverageSimple indicator
         self.sma = bt.indicators.SimpleMovingAverage(self.datas[0], period=self.params.maperiod)

+        # Indicators for the plotting show
+        bt.indicators.ExponentialMovingAverage(self.datas[0], period=25)
+        bt.indicators.WeightedMovingAverage(self.datas[0], period=25, subplot=True)
+        bt.indicators.StochasticSlow(self.datas[0])
+        bt.indicators.MACDHisto(self.datas[0])
+        rsi = bt.indicators.RSI(self.datas[0])
+        bt.indicators.SmoothedMovingAverage(rsi, period=10)
+        bt.indicators.ATR(self.datas[0], plot=False)
+
     def notify_order(self, order):
         if order.status in [order.Submitted, order.Accepted]:
             # Buy/Sell order submitted/accepted to/by broker - Nothing to do
@@ -131,4 +140,7 @@
     cerebro.run()

     # Print out the final result
-    print('Final Portfolio Value: %.2f' % cerebro.broker.getvalue())
\ No newline at end of file
+    print('Final Portfolio Value: %.2f' % cerebro.broker.getvalue())
+
+    # Plot the result
+    cerebro.plot()
\ No newline at end of file
```

### 优化

在开始绘图示例之前, 我们使用的 SMA 周期是 15, 这个参数实际上是一个策略参数, 可以针对不同的股票不同的市场做定制, 知道找到自己认为最合适的参数值.

> There is plenty of literature about Optimization and associated pros and cons. But the advice will always point in the same direction: do not overoptimize. If a trading idea is not sound, optimizing may end producing a positive result which is only valid for the backtested dataset.

接下来的示例中修改了 SMA 的周期, 简单起见任何关于买入卖出的日志也都被移除.

代码 diff:

```diff
--- x.py	2022-07-10 00:21:03.903780834 +0800
+++ y.py	2022-07-10 00:22:52.294211036 +0800
@@ -10,12 +10,16 @@

 # Create a Stratey
 class TestStrategy(bt.Strategy):
-    params = (('maperiod', 15), )
+    params = (
+        ('maperiod', 15),
+        ('printlog', False),
+    )

-    def log(self, txt, dt=None):
+    def log(self, txt, dt=None, doprint=False):
         ''' Logging function fot this strategy'''
-        dt = dt or self.datas[0].datetime.date(0)
-        print('%s, %s' % (dt.isoformat(), txt))
+        if self.params.printlog or doprint:
+            dt = dt or self.datas[0].datetime.date(0)
+            print('%s, %s' % (dt.isoformat(), txt))

     def __init__(self):
         # Keep a reference to the "close" line in the data[0] dataseries
@@ -29,15 +33,6 @@
         # Add a MovingAverageSimple indicator
         self.sma = bt.indicators.SimpleMovingAverage(self.datas[0], period=self.params.maperiod)

-        # Indicators for the plotting show
-        bt.indicators.ExponentialMovingAverage(self.datas[0], period=25)
-        bt.indicators.WeightedMovingAverage(self.datas[0], period=25, subplot=True)
-        bt.indicators.StochasticSlow(self.datas[0])
-        bt.indicators.MACDHisto(self.datas[0])
-        rsi = bt.indicators.RSI(self.datas[0])
-        bt.indicators.SmoothedMovingAverage(rsi, period=10)
-        bt.indicators.ATR(self.datas[0], plot=False)
-
     def notify_order(self, order):
         if order.status in [order.Submitted, order.Accepted]:
             # Buy/Sell order submitted/accepted to/by broker - Nothing to do
@@ -98,13 +93,16 @@
                 # Keep track of the created order to avoid a 2nd order
                 self.order = self.sell()

+    def stop(self):
+        self.log('(MA Period %2d) Ending Value %.2f' % (self.params.maperiod, self.broker.getvalue()), doprint=True)
+

 if __name__ == '__main__':
     # Create a cerebro entity
     cerebro = bt.Cerebro()

     # Add a strategy
-    cerebro.addstrategy(TestStrategy)
+    strats = cerebro.optstrategy(TestStrategy, maperiod=range(10, 31))

     # Datas are in a subfolder of the samples. Need to find where the script is
     # because it could have been called from anywhere
@@ -133,14 +131,5 @@
     # Set the commission
     cerebro.broker.setcommission(commission=0.0)

-    # Print out the starting conditions
-    print('Starting Portfolio Value: %.2f' % cerebro.broker.getvalue())
-
     # Run over everything
-    cerebro.run()
-
-    # Print out the final result
-    print('Final Portfolio Value: %.2f' % cerebro.broker.getvalue())
-
-    # Plot the result
-    cerebro.plot()
\ No newline at end of file
+    cerebro.run(maxcpus=1)
\ No newline at end of file
```

代码:

```python
from __future__ import (absolute_import, division, print_function, unicode_literals)

import datetime  # For datetime objects
import os.path  # To manage paths
import sys  # To find out the script name (in argv[0])

# Import the backtrader platform
import backtrader as bt


# Create a Stratey
class TestStrategy(bt.Strategy):
    params = (
        ('maperiod', 15),
        ('printlog', False),
    )

    def log(self, txt, dt=None, doprint=False):
        ''' Logging function fot this strategy'''
        if self.params.printlog or doprint:
            dt = dt or self.datas[0].datetime.date(0)
            print('%s, %s' % (dt.isoformat(), txt))

    def __init__(self):
        # Keep a reference to the "close" line in the data[0] dataseries
        self.dataclose = self.datas[0].close

        # To keep track of pending orders and buy price/commission
        self.order = None
        self.buyprice = None
        self.buycomm = None

        # Add a MovingAverageSimple indicator
        self.sma = bt.indicators.SimpleMovingAverage(self.datas[0], period=self.params.maperiod)

    def notify_order(self, order):
        if order.status in [order.Submitted, order.Accepted]:
            # Buy/Sell order submitted/accepted to/by broker - Nothing to do
            return

        # Check if an order has been completed
        # Attention: broker could reject order if not enough cash
        if order.status in [order.Completed]:
            if order.isbuy():
                self.log('BUY EXECUTED, Price: %.2f, Cost: %.2f, Comm %.2f' %
                         (order.executed.price, order.executed.value, order.executed.comm))

                self.buyprice = order.executed.price
                self.buycomm = order.executed.comm
            else:  # Sell
                self.log('SELL EXECUTED, Price: %.2f, Cost: %.2f, Comm %.2f' %
                         (order.executed.price, order.executed.value, order.executed.comm))

            self.bar_executed = len(self)

        elif order.status in [order.Canceled, order.Margin, order.Rejected]:
            self.log('Order Canceled/Margin/Rejected')

        self.order = None

    def notify_trade(self, trade):
        if not trade.isclosed:
            return

        self.log('OPERATION PROFIT, GROSS %.2f, NET %.2f' % (trade.pnl, trade.pnlcomm))

    def next(self):
        # Simply log the closing price of the series from the reference
        self.log('Close, %.2f' % self.dataclose[0])

        # Check if an order is pending ... if yes, we cannot send a 2nd one
        if self.order:
            return

        # Check if we are in the market
        if not self.position:

            # Not yet ... we MIGHT BUY if ...
            if self.dataclose[0] > self.sma[0]:

                # BUY, BUY, BUY!!! (with all possible default parameters)
                self.log('BUY CREATE, %.2f' % self.dataclose[0])

                # Keep track of the created order to avoid a 2nd order
                self.order = self.buy()

        else:

            if self.dataclose[0] < self.sma[0]:
                # SELL, SELL, SELL!!! (with all possible default parameters)
                self.log('SELL CREATE, %.2f' % self.dataclose[0])

                # Keep track of the created order to avoid a 2nd order
                self.order = self.sell()

    def stop(self):
        self.log('(MA Period %2d) Ending Value %.2f' % (self.params.maperiod, self.broker.getvalue()), doprint=True)


if __name__ == '__main__':
    # Create a cerebro entity
    cerebro = bt.Cerebro()

    # Add a strategy
    strats = cerebro.optstrategy(TestStrategy, maperiod=range(10, 31))

    # Datas are in a subfolder of the samples. Need to find where the script is
    # because it could have been called from anywhere
    modpath = os.path.dirname(os.path.abspath(sys.argv[0]))
    datapath = os.path.join(modpath, 'data/example.csv')

    # Create a Data Feed
    data = bt.feeds.YahooFinanceCSVData(
        dataname=datapath,
        # Do not pass values before this date
        fromdate=datetime.datetime(2000, 1, 1),
        # Do not pass values before this date
        todate=datetime.datetime(2000, 12, 31),
        # Do not pass values after this date
        reverse=False)

    # Add the Data Feed to Cerebro
    cerebro.adddata(data)

    # Set our desired cash start
    cerebro.broker.setcash(1000.0)

    # Add a FixedSize sizer according to the stake
    cerebro.addsizer(bt.sizers.FixedSize, stake=10)

    # Set the commission
    cerebro.broker.setcommission(commission=0.0)

    # Run over everything
    cerebro.run(maxcpus=1)
```

这次我们没有 `addstrategy` 添加策略, 而是使用 `optstrategy` 来优化策略. 对应的参数也没有传递值, 而是传递了一个范围.

还加了一个策略钩子函数(`stop`), 当数据被消费完并且回测完成的时候这个回调就会被调用, 这里它被用来打印最后的账户净值.

系统将会对范围参数里面的每个值, 都执行一遍策略. 下面是这次执行的输出:

```
2000-12-29, (MA Period 10) Ending Value 880.30
2000-12-29, (MA Period 11) Ending Value 880.00
2000-12-29, (MA Period 12) Ending Value 830.30
2000-12-29, (MA Period 13) Ending Value 893.90
2000-12-29, (MA Period 14) Ending Value 896.90
2000-12-29, (MA Period 15) Ending Value 973.90
2000-12-29, (MA Period 16) Ending Value 959.40
2000-12-29, (MA Period 17) Ending Value 949.80
2000-12-29, (MA Period 18) Ending Value 1011.90
2000-12-29, (MA Period 19) Ending Value 1041.90
2000-12-29, (MA Period 20) Ending Value 1078.00
2000-12-29, (MA Period 21) Ending Value 1058.80
2000-12-29, (MA Period 22) Ending Value 1061.50
2000-12-29, (MA Period 23) Ending Value 1023.00
2000-12-29, (MA Period 24) Ending Value 1020.10
2000-12-29, (MA Period 25) Ending Value 1013.30
2000-12-29, (MA Period 26) Ending Value 998.30
2000-12-29, (MA Period 27) Ending Value 982.20
2000-12-29, (MA Period 28) Ending Value 975.70
2000-12-29, (MA Period 29) Ending Value 983.30
2000-12-29, (MA Period 30) Ending Value 979.80
```

最终显示 SMA 周期设置为 20 会比较合理.

> 这个最优参数只是针对回测数据, 不能代表未来.

### 结语

上面*渐进式*的示例显示了如何从一个只有骨架的脚本到一个能正常工作并且可以绘制图表及执行参数优化的交易系统是如何演变的.

后续可以做更多事情来提高胜率:

- 自定义指标. 事实上, 创建指标很容易(甚至绘制它们也很容易)
- Sizers. 对许多人来说, 资金管理是成功的关键
- 订单类型(限价, 止损, 止损限价) - limit, stop, stoplimit
- 其他

backtrader 的文档对相关部分有详细介绍.
