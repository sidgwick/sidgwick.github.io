---
layout: post
title:  "YiiBase小记"
date:   2015-08-20 19:28:04
categories: php yii2
---

## 总说

YiiBase是Yii的基础包装, 可以理解成司令官的角色. 它用于自动加载, 创建别名,
把别名转换为地址, 翻译以及其他一系列操作.

## createApplication

这是第一个比较要紧的方法, 根据传入的目标应用名称和配置, 会然会相应的应用对象.

## createComponent

这个类用给定的配置来创建component. 配置可以是字符串或者是数组. 如果是字符串,
它会被当做是别名或者类名称. 若配置是数组形式, 则此数组的`class`元素将会被当做
对象类型(理解成类名), 之后的键值对会被当做新创建的对象的属性(property).

若向这个方法传递了其他参数, 那么这些参数会被传递给新创建对象的构造函数(这里涉
及到了ReflactionClass, 之前一直不知道Reflaction用在哪里, 这也算是一个实际应用吧).

## import

此方法用于引入一个类或者目录.

引入一个类的操作和`include`此类的类文件一样, 主要的不同点在于引入一个类比引入一
个类文件更加轻量, 因为引入一个类会在用到这个类的时候再去引入这个类文件.

引入一个目录和向PHP的`include path`里面添加一个目录是一样的, 当有多个目录被引入
时, 后引入的目录会有更高的优先级, 也即: 它们被加在PHP`include path`的前面.

可以使用别名来引入类或者目录, 比如`application.components.GoogleMap`用于引入
`GoogleMap`类. 而`application.components.*`用于引入`components`目录.

同一个别名可以被多次`import`, 但只有第一次是有效的, 而且, 引入目录不会引入目
录的子目录.

从1.1.5版本(要求PHP >= 5.3)开始, `import`也可以接受名称空间参数, 工作方法和引
入别名差不多, 只是点号分隔符被换为了反斜线. 此时, 名称空间的写法就需要遵守一
定的规则了(用于方便的转换到目录). 规则要求, 当名称空间里面的反斜线被替换为点
号时, 能组成一个有效的路径别名(path alias).

## getPathOfAlias, setPathOfAlias

下面来看`setPathOfAlias`, 此方法用于设定一个别名, 当传入的`path`为空, 就会删
除这个别名. 这个类既不检查路径是否存在, 也不检查路径是不是规范, 一切靠自觉,
你给啥, 就是啥

要紧的是`getPathOfAlias`, 它用于把一个别名转换为一个文件地址. 这个方法不检查
生成的目标文件地址是不是存在, 它仅仅检查别名里面的`root alias`的合法性.

注意: **findModel**函数没有研究过, 这里调用到了.

## autoload和registerAutoloader

`autoload`被PHP的魔术方法`__autoload`调用, 这里实现了即用即加载.
`registerAutoloader`用于注册新的autoloader, 它能确保`$app`的
`autoload`有最高的优先级

## t方法

这个方法是用于将一个消息翻译为特定的语种. 自1.0.2版本开始, 该方法开始支持
`choice format`, 也即返回消息将会根据这个参数返回. 种特性主要是为了解决某些
语言中, 一个消息可以有多种形式的说法(这里翻译的不好, 不知道是不是这个意思).

`$category`参数标示了消息类别, 这里应该只使用字母, 并且要注意 'yii' 关键字
保留用于Yii框架核心代码使用了. 在`PhpMessageSource`类里面能找到详细的使用说明

`$params` 参数指定`strtr`函数使用的参数, 从1.0.2版本开始, `t`方法支持在这个参
数第一个元素作为数字(没有关联的键)指定`choice format`, 到1.1.6版本, 开始支持向
`ChoiceFormat::format`传参时无需使用数组包装. 

`$source`参数指定应用程序那个消息源, 默认是`null`, 意味着对yii的`category`使用
coreMessages源, 对其他`category`使用messages消息源.

`$language`指定目标语言, 当不指定时, 调用`Application::getLanguage`确定翻译为
何种语言, 此参数自1.0.3开始可用

TODO: `ChoiceFormat::format`, 具体的定义当前版本里面没有定义, 以后再看

## 其他

还有一些琐碎的方法, 像日志, debug等, 都比较简单, 不介绍了
