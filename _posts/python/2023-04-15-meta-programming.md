---
title: "python 元编程"
date: 2023-04-14 02:28:04
tags: python oop meta-programming
---

本文讲述了 python 中 class 的一些细节. 包含:

1. class 定义和 class 对象
2. class 实例对象

# `__new__` 函数和 `__call__` 函数

先从简单的开始, 比如以下代码:

<!--more-->

```python
class hello():
    print('hello 类')

    a = 'aaaa'

    def __new__(cls, *args, **kwargs):
        print('hello 运行__new__函数')
        print("cls", cls)
        print("args", args)
        print("kwargs", kwargs)
        print("=" * 20)
        res = object.__new__(cls)
        return res

    def __call__(self, *args, **kwargs):
        print('hello 运行__call__函数')
        print("self", self)
        print("args", args)
        print("kwargs", kwargs)
        print("=" * 20)


print('------ execute -----')
print(type(hello))
print(hello.a)
print('A', '-' * 20)
obj = hello('HI')
print('B', '-' * 20)
print(obj)
print('C', '-' * 20)
obj()
print('D', '-' * 20)
```

输出为:

```text
hello 类
------ execute -----
<class 'type'>
aaaa
A --------------------
hello 运行__new__函数
cls <class '__main__.hello'>
args ('HI',)
kwargs {}
====================
B --------------------
<__main__.hello object at 0x10ee13fd0>
C --------------------
hello 运行__call__函数
self <__main__.hello object at 0x10ee13fd0>
args ()
kwargs {}
====================
D --------------------
```

可以看到在 Python 解释器遇到 `class` 定义的时候, 就会执行 class body 中的语句. 当实例化一个类的时候, `__new__` 方法被调用, 如果尝试把实例对象作为函数调用, 则会触发 `__call__` 函数的调用.

接下来给 `hello` 指定一个 `metaclass`.

```python
class mclass(type):
    def __new__(cls, clsname, bases, dct):
        print('mclass - 运行__new__函数')
        print("cls", cls)
        print("clsname", clsname)
        print("bases", bases)
        print("dct", dct)
        print("=" * 20)
        res = super().__new__(cls, clsname, bases, dct)
        return res

    def __call__(self, *args, **kwargs):
        print('mclass 对象实例运行__call__函数')
        print("args", args)
        print("kwargs", kwargs)
        print("=" * 20)
        return self

class world(metaclass=mclass):
    print("world class")

class hello(metaclass=mclass):
    ...
```

这次再执行脚本, 输出如下:

```text
hello 类
mclass - 运行__new__函数
cls <class '__main__.mclass'>
clsname hello
bases ()
dct {'__module__': '__main__', '__qualname__': 'hello', 'a': 'aaaa', '__new__': <function hello.__new__ at 0x10856e3b0>, '__call__': <function hello.__call__ at 0x108622cb0>}
====================
world class
mclass - 运行__new__函数
cls <class '__main__.mclass'>
clsname world
bases ()
dct {'__module__': '__main__', '__qualname__': 'world'}
====================
------ execute -----
<class '__main__.mclass'>
aaaa
A --------------------
mclass 对象实例运行__call__函数
args ('HI',)
kwargs {}
====================
B --------------------
<class '__main__.hello'>
C --------------------
mclass 对象实例运行__call__函数
args ()
kwargs {}
====================
D --------------------
```

可以看到几点不同:

1. mclass 的 `__new__` 在解释器遇到 `world` 或者 `hello` 类的定义的时候就触发了, 这里应该理解为, 解释器需要使用 metaclass 实例来生成类型对象, 因此就需要在遇到 `metaclass=mclass` 时候将 mclass 实例化, 所以 mclass 的 `__new__` 函数被触发了 2 次
2. `hello` 类的类型现在变成了 `mclass`, 这主要是因为指定了元类之后, 类对象就会使用这个元类生成, 因此类型就不再是 `type` 了(`type` 本身也是一种元类)
3. `hello` 类实例化的时候, 触发了元类的 `__call__` 函数, 等到再次尝试以函数方式调用实例时, 又触发了元类 `__call__` 调用.

> TODO: 第三点是为啥?

---

下面是一段来自 [backtrader](https://github.com/mementum/backtrader) 的代码, 在 backtrader 里面有大量的使用, 这里对它做做一些解释.

```python
def with_metaclass(meta, *bases):
    """Create a base class with a metaclass."""

    # This requires a bit of explanation: the basic idea is to make a dummy
    # metaclass for one level of class instantiation that replaces itself with
    # the actual metaclass.
    class metaclass(meta):

        def __new__(cls, name, this_bases, d):
            print(f"Creating class {name} with metaclass {metaclass}")
            return meta(name, bases, d)

    return type.__new__(metaclass, str("temporary_class"), (), {})

```

这个函数用来生成一个类定义, 这个类定义有如下特性:

1. 使用 `type.__new__` 来创建类名叫做 temporary_class 的类定义
2. 这个类的元类是 (metaclass 和 meta), 这两个元类的作用是在 temporary_class 的创建过程中控制创建过程,
   (metaclass, meta) 两个元类的 **new** 方法会被先后调用, 以达到控制创建过程的目的

看以下代码:

```python
class MyMeta(type):
    def __new__(cls, name, bases, dct):
        print(f"Creating class {name} with metaclass {cls}")
        return super().__new__(cls, name, bases, dct)


MyClass = with_metaclass(MyMeta, object)
# 到这里, `MyClass` 是以 (`with_classmeta.classmeta`, `MyMeta`) 两个元类为元类的类定义

class MySubClass(MyClass):
    '''定义一个继承 MyClass 的类'''
    pass
```

在上面 MySubClass 定义过程中, (`with_classmeta.classmeta`, `MyMeta`) 的 `__new__` 函数就会被先后调用
