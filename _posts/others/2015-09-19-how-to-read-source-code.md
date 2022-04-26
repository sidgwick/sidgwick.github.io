---
title: "如何阅读源代码"
date: 2015-09-19 10:28:04
tags: others
---

原文: [https://wiki.c2.com/?TipsForReadingCode](https://wiki.c2.com/?TipsForReadingCode)

## Tips For Reading Code

提高编程技巧的一个重要方法就是阅读伟大的程序([ReadGreatPrograms]). 诸如
[SelfDocumentingCode]以及[LiterateProgramming]技术使得写出的代码更加容易阅读.

然而, 多数时候我们阅读的代码没有实现上面的标准. 那么, 有什么好办法能使得我们必
须要理解或者学习的这种巨大的, 不结构化的, 由数十人维护的, 内部不一致的, 没有注
释的代码更直观呢?

<!--more-->

### 构建并执行程序

能够执行并观察它的外部行为将会对理解他的内部很有帮助. 程序文档也会很有用, 但是
文档可能不能完全准确的描述程序行为.

能自己动手编译程序, 将会帮你发现程序都使用了那些外部库, 以及那些编译器以及连接
器选项有效等等. 如果你能编译程序的 debug 版本, 那么你就可以通过 debugger 来逐步运
行程序([StudyTheSourceWithaDebugger]).

也要向程序添加 log 或者扩展程序的 log, 让它们告诉你他们正在做什么.

### 寻找高级逻辑

从程序的入口开始(比如 C/C++/Java 里面的`main()`函数), 找到程序是如何初始化自己,
怎么完成它的工作以及怎么退出的.

多数程序都有一个主循环(`main loop`), 找到它. 要注意, 若程序使用了外部库框架,
那么, 主循环有可能是库/框架的, 而不是应用程序自己的.

找到终止程序执行的条件, 这里包含非正常退出以及正常退出条件.

许多事件驱动(event-driven)的或者面向对象(OO)设计的都没有`main`, 但是它们仍然有
入口.

Not always. Software for the GEOS operating systems for the C64/C128 platform
are structured as a single initialization routine (which is expected to return
to the OS after it's done configuring its runtime environment), and a morass
of callbacks invoked under various circumstances. In a sense, a GEOS
application is nothing more than an overlay for the host OS itself, which
doubles as the system's sole application. Indeed, any true event-driven style
of programming closely, if not exactly, resembles this. What is the
"main method" of a class? The constructor rarely is the most important method.
By extension, an event driven program's initializer is rarely its most
important callback.

### 画一点流程图

呵呵, 我们都知道流程图是糟糕的设计工具, 但是在分析程序流程时它们很有用. 你不可
能把这些信息都记在你的脑子里, 所以, 可以在阅读时画一些流程图或者状态图. Make
sure you account for both sides of a two-way branch, and all branches of
multi-way branches.

### 测试函数库调用

如果程序用到了外部函数库, 你要测试这些库调用, 并且读一读这些调用的文档. 这可能
是你仅有的"real documentation", 所以请好好利用它们.

### 找一些关键字

使用你的编辑器的查找特性(或者`grep`)来找整个源码树上那些你感兴趣的单词的位置.
比如说, 你想知道程序如何或者在哪里打开了文件, 那么搜索'open'或者'file'. 你可能
会得到海量的答案, 但这不是坏事, (怎么译: a focused reading of the code will
result.)

对 C/C++程序来说, 常用的单词有: `main`, `abort`, `exit`, `catch`, `throw`,
`fopen`, `create`, `signal`, `alarm`, `socket`, `fork`, `select`. 其他语言里面
也有相应的常用单词.

### 借助代码增强工具

有一些非常棒的文字搜索工具, 或者一些可以分析代码, 找到代码关系并生成图像的工具
可以回答以下面向对象的问题:

- 谁调用了这个方法
- 谁实现了这个接口或者是这个类的子类
- 这个类的父类(超类)是谁
- 这个类在哪里被实例化, 保存在那里, 需要什么参数, 返回什么.
- 这个类重载了父类的什么方法
- 在哪里这个方法作为多态调用, 也即通过基类还是接口.

References: [Comprehension and Visualisation of Object-Oriented Code for
Inspections][ref1] section 5.

### 打印出代码

显示器很少能比放着打印源码源码的空书桌舒服(Monitors can rarely beat the sheer
textual capacity of an empty table on which a printout of the source code has
been laid.). 你可以一眼看到上千 KB 的代码. 这极有助于我们把握代码的大概.

如果某页包含了对分析无足轻重的代码, 你可以把它替换掉. 你可以在打印出来的物理媒
介上注释, 这也可以加快理解速度. 还可以圈出重要的函数或者高亮(大概就是做标记的意
思, 都打出来了, 还怎么高亮...)变量名称.

### 写单元测试

This will help you prove to yourself that you understand what the code is supposed to do, what it actually does, and that you understand its limitations.
If there are no UnitTests, then you should definitely create a sufficient set of UnitTests before making any changes to the code.

### Comment the Code

Throw the code under into a personal CVS or RCS repository and mark it up with your comments. As you work out your knowledge of the code, the comments will change. This can be an important step if you have to ReFactor the code later.

One of the best ways of reverse-engineering code you're so unfamiliar with that you cannot even guess to comment is to derive HoareTriples for the code under scrutiny, and to compute a procedure's WeakestPrecondition?. Knowing the invariants of a procedure often gives valuable clues as to its intended purpose, thus letting you derive understanding from the code in a way that merely guessing from reading a number of poorly named procedure names simply cannot. Using this technique, you might even find bugs in the code, despite your lack of understanding thereof (I know this from experience!).

### Clean Up the Code

An old writing trick for refamiliarizing yourself with text you wrote a long time ago but forgot, or for analyzing someone else's text, is to edit it as you go along. This is active reading. Rewrite it in different ways, or a more pleasing way. You may have noticed on Wiki that while refactoring or reworking a page, you come to understand the material much deeper than just by reading it. Code is not much different from writing.

Therefore, when reading code, reformat it as you go along. Realign spaces. Comment the Code, as it says above. Fix out of date comments (vis ThePalimpsestEffect). Fix spelling. Make the code conform to the coding standards. Usually code is written somewhat hastily, so having someone come along later to make the code look professional is also another benefit.

But, if you do a lot of changes, run the UnitTests! Breaking things isn't necessarily something to be afraid of. By finding the hairier parts of the system and the dependent parts of the system (often those you won't expect), you come to grok the system.

A good article on this technique is "Make bad code good", at JavaWorld http://www.javaworld.com/javaworld/jw-03-2001/jw-0323-badcode.html

### See also RefactoringForGrokking

I find I have to go through a lot of this reading the sample code on java.sun.com. Surprisingly (or maybe not), I usually make no progress grokking it until I delete all of the comments. Of course, the next step is to rename, by MassiveSearchAndReplace, all the variable names such as p, tc, f, etc. (in case you're curious, processor, trackControl, format). The next thing that helps me is to split up the giant methods into various methods (i.e., pulling looped and if'd code into their own methods, etc.).

A MassiveSearchAndReplace might miss cases or change the wrong things. A real renaming with a refactoring tool, if there's one available for the language, may be more effective.

Note that simple recompilation suffices as UnitTests for most of this... if I missed a tc, or it replaced CompletedEvent? with ComprocessorletedEvent?, or if I forgot to pass a variable into an extracted method, the compiler will throw an appropriate error.

Reading big random programs reminds me very much of exploring mazes in Angband.

I start at some random position (maybe found by grep) and work my way around by traversing callees and callers until I've mapped out enough of the immediate vicinity to see what's going on and do my hack. If I explore enough directions for long enough, things start to connect up and give me a more "global" sense, but often I don't explore so widely.

This is a great analogy. In a similar vein, debugging is like completing an 'ascension kit' in Nethack. There's no certain path to tracking down your item/bug, and even once you've found/fixed it, there are always going to be others you could go for next, of greater or lesser importance/severity. Oh, and you could inadvertently overenchant your GDSM / blow up the system by enchanting / blindly 'fixing' before you take a second to make absolutely sure what you're doing.

A suggestion: Use Aspect Browser. Aspect Browser is a tool for viewing cross-cutting code aspects in large amounts of code. I find it indispensable for navigating unfamiliar code, as a kind of super-grep. Read more about it here:

http://www-cse.ucsd.edu/users/wgg/Software/AB/

Another suggestion: CScope has saved my bacon more than once, apparently it supports Java now!. It'll grep the codebase to do stuff like find references to nearly any symbol, determine which functions call a certain function, etc.

http://cscope.sourceforge.net/

Apply the patterns described in the ObjectOrientedReengineeringPatterns book. Seconded! A great book that deserves to be on every maintenance programmer's (and that's all of us) shelf.

A number of source code comprehension tools are reviewed at http://www.grok2.com/code_comprehension.html. This page is a couple of years old though, so some of the links may be broken. Personally a good code editor helps when reading code. Some thing like jedit or emacs from the free world.

See also ReadableCode, ProgramComprehension, SignatureSurvey, StudyTheSourceWithaDebugger, ReadItLikeaComputer, WhatItTakesToGrokCode, CodeAvoidance, HistoricalProgramReadingExercise

[readgreatprograms]: http://c2.com/cgi/wiki?ReadGreatPrograms
[selfdocumentingcode]: http://c2.com/cgi/wiki?SelfDocumentingCode
[literateprogramming]: http://c2.com/cgi/wiki?LiterateProgramming
[studythesourcewithadebugger]: http://c2.com/cgi/wiki?StudyTheSourceWithaDebugger
[ref1]: http://www.cis.strath.ac.uk/research/efocs/abstracts.html#EFoCS-33-98
