---
title: "Monkey Patching in Go"
date: 2023-06-29 14:28:04
tags: golang assembly testing monkey-patching
---

> 本文是 [Monkey Patching in Go](https://bou.ke/blog/monkey-patching-in-go/) 的阅读理解.

# 原文 Monkey Patching in Go

Many people think that monkey patching is something that is restricted to dynamic languages like Ruby and Python. That is not true however, as computers are just dumb machines and we can always make them do what we want! Let's look at how Go functions work and how we can modify them at runtime. This article will use a lot of Intel assembly syntax, so I'm assuming you can read it already or are using a [reference](https://software.intel.com/en-us/articles/introduction-to-x64-assembly) while reading.

<!--more-->

If you're not interested in how it works and you just want to do monkey patching, then you can find the library [here](https://github.com/bouk/monkey).

Let's look at what the following code produces when disassembled:

```go
package main

func axx() int {
    return 1
}

func main() {
    print(axx())
}
```

> Samples should be built with `go build -gcflags='-N -l'` to disable inlining. For this article I assume your architecture is 64-bits and that you're using a unix-based operating system like Mac OSX or a Linux variant.
> 编译参数说明可以参考: [Go gcflags/ldflags 的说明](https://www.bwangel.me/2022/01/12/go_gcflags/)

When compiled and looked at through Hopper, the above code will produce this assembly code:

I will be referring to the addresses of the various instructions displayed on the left side of the screen.

![](https://bou.ke/images/hopper-1.png)

Our code starts in procedure main.main, where instructions 0x2010 to 0x2026 set up the stack. You can read more about that here, I will be ignoring that code for the rest of the article.

Line 0x202a is the call to function main.a at line 0x2000 which simply moves 0x1 onto the stack and returns. Lines 0x202f to 0x2037 then pass that value on to runtime.printint.

Simple enough! Now let's take a look at how function values are implemented in Go.

## How function values work in Go

Consider the following code:

```go
package main

import (
	"fmt"
	"unsafe"
)

func a() int { return 1 }

func main() {
	f := a
	fmt.Printf("0x%x\n", *(*uintptr)(unsafe.Pointer(&f)))
}
```

> Go 世界中的函数是一等公民

What I'm doing on line 11 is assigning a to f, which means that doing f() will now call a. Then I use the unsafe Go package to directly read out the value stored in f. If you come from a C background you might expect f to simply be a function pointer to a and thus this code to print out 0x2000 (the location of main.a as we saw above). When I run this on my machine I get 0x102c38, which is an address not even close to our code! When disassembled, this is what happens on line 11 above:

![](https://bou.ke/images/hopper-2.png)

This references something called main.a.f, and when we look at that location, we see this:

![](https://bou.ke/images/hopper-3.png)

Aha! main.a.f is at 0x102c38 and contains 0x2000, which is the location of main.a. It seems f isn't a pointer to a function, but a pointer to a pointer to a function. Let's modify the code to compensate for that.

```go
package main

import (
	"fmt"
	"unsafe"
)

func a() int { return 1 }

func main() {
	f := a
	fmt.Printf("0x%x\n", **(**uintptr)(unsafe.Pointer(&f)))
}
```

> 胖指针!!!

This will now print 0x2000, as expected. We can find a clue as to why this is implemented as it is here. Go function values can contain extra information, which is how closures and bound instance methods are implemented.

Let's look at how calling a function value works. I'll change the code to call f after assigning it.

```go
package main

func a() int { return 1 }

func main() {
	f := a
	f()
}
```

When we disassemble this we get the following:

![](https://bou.ke/images/hopper-4.png)

main.a.f gets loaded into rdx, then whatever rdx points at gets loaded into rbx, which then gets called. The address of the function value always gets loaded into rdx, which the code being called can use to load any extra information it might need. This extra information is a pointer to the instance for a bound instance method and the closure for an anonymous function. I advise you to take out a disassembler and dive deeper if you want to know more!

Let's use our newly gained knowledge to implement monkeypatching in Go.

## Replacing a function at runtime

What we want to achieve is to have the following code print out 2:

```go
package main

func a() int { return 1 }
func b() int { return 2 }

func main() {
	replace(a, b)
	print(a())
}
```

Now how do we implement replace? We need to modify function a to jump to b's code instead of executing its own body. Essentialy, we need to replace it with this, which loads the function value of b into rdx and then jumps to the location pointed to by rdx.

```as
mov rdx, main.b.f ; 48 C7 C2 ?? ?? ?? ??
jmp [rdx] ; FF 22
```

I've put the corresponding machine code that those lines generate when assembled next to it (you can easily play around with assembly using an online assembler like [this](https://defuse.ca/online-x86-assembler.htm)). Writing a function that will generate this code is now straightforward, and looks like this:

> 关于这段汇编的解释, 实际上这是作者使用了两个汇编指令(就是上面的 `mov` 和 `jmp`).
>
> 假如说我们想把 `a()` 调用替换为 `b()`, 而且 `b` 函数在生成的可执行文件中的内存位置为 `0x01020304`, 那么, 需要做的就是生成下面这样的机器码(后面是对应的汇编指令):
>
> ```asm
> 9:	48 c7 c2 04 03 02 01 	mov    $0x01020304,%rdx
> 10:	ff 22                	jmpq   *(%rdx)
> ```
>
> 只需要把这段机器码替换到原始函数的机器码开始位置, 就能实现调用 `a` 的时候跳转到 `b`. 从原理上说, 我们还是调用的 `a` 函数, 不过在 `a` 函数的开头现在有个跳转指令, 直接跳转到了 `b` 函数的位置执行, 执行完成之后由 `b` 函数里面的 `ret` 指令直接返回到 `a` 的调用者的下一条指令继续执行. 函数参数和返回值是依赖栈帧传递的, 这里替换之后不会影响栈, 因此也不会影响参数和返回值.
>
> `assembleJump` 函数就是用来生成这种机器码的.

```go
import "unsafe"

func assembleJump(f func() int) []byte {
	funcVal := *(*uintptr)(unsafe.Pointer(&f))
	return []byte{
		0x48, 0xC7, 0xC2,
		byte(funcval >> 0),
		byte(funcval >> 8),
		byte(funcval >> 16),
		byte(funcval >> 24), // MOV rdx, funcVal
		0xFF, 0x22,          // JMP [rdx]
	}
}
```

We now have everything we need to replace a's function body with a jump to b! The following code attempts to copy the machine code directly to the location of the function body.

```go
package main

import (
	"syscall"
	"unsafe"
)

func a() int { return 1 }
func b() int { return 2 }

// 这里获取运行时代码段中 `b` 代表的函数指针的内存区域
// 通过覆写这块区域, 就可以实现函数指针指向偷天换日的效果
func rawMemoryAccess(b uintptr) []byte {
	return (*(*[0xFF]byte)(unsafe.Pointer(b)))[:]
}

func assembleJump(f func() int) []byte {
	funcVal := *(*uintptr)(unsafe.Pointer(&f))
	return []byte{
		0x48, 0xC7, 0xC2,
		byte(funcVal >> 0),
		byte(funcVal >> 8),
		byte(funcVal >> 16),
		byte(funcVal >> 24), // MOV rdx, funcVal
		0xFF, 0x22, // JMP [rdx]
	}
}

func replace(orig, replacement func() int) {
	bytes := assembleJump(replacement)
	functionLocation := **(**uintptr)(unsafe.Pointer(&orig))
	window := rawMemoryAccess(functionLocation)
	copy(window, bytes)
}

func main() {
	replace(a, b)
	print(a())
}
```

Running this code does not work however, and will result in a segmentation fault. This is because the loaded binary is not writable by default. We can use the mprotect syscall to disable this protection, and this final version of the code does exactly that, resulting in function a being replaced by function b, and `2` being printed.

```go
package main

import (
	"syscall"
	"unsafe"
)

func a() int { return 1 }
func b() int { return 2 }

func getPage(p uintptr) []byte {
	return (*(*[0xFFFFFF]byte)(unsafe.Pointer(p & ^uintptr(syscall.Getpagesize()-1))))[:syscall.Getpagesize()] // 拷贝一页 16Kb
}

func rawMemoryAccess(b uintptr) []byte {
	return (*(*[0xFF]byte)(unsafe.Pointer(b)))[:] // 拷贝 255 个内存单元出去, 实际上只需要操作 9 个, 这里拷贝的数量大于 9 个就行
}

func assembleJump(f func() int) []byte {
	funcVal := *(*uintptr)(unsafe.Pointer(&f))
	return []byte{
		0x48, 0xC7, 0xC2,
		byte(funcVal >> 0),
		byte(funcVal >> 8),
		byte(funcVal >> 16),
		byte(funcVal >> 24), // MOV rdx, funcVal
		0xFF, 0x22, // JMP rdx
	}
}

func replace(orig, replacement func() int) {
	bytes := assembleJump(replacement)
	functionLocation := **(**uintptr)(unsafe.Pointer(&orig))
	window := rawMemoryAccess(functionLocation)
	page := getPage(functionLocation)
	syscall.Mprotect(page, syscall.PROT_READ|syscall.PROT_WRITE|syscall.PROT_EXEC)
	copy(window, bytes)
}

func main() {
	replace(a, b)
	print(a())
}
```

## Wrapping it up in a nice library

I took the above code and put it in an easy to use library. It supports 32 bit, reversing patches, and patching instance methods. I wrote a couple of examples and put those in the README.

## Conclusion

Where there's a will there's a way! It's possible for a program to modify itself at runtime, which allows us to implement cool tricks like monkey patching.

I hope you got something useful out of this blogpost, I know I had fun making it!

# 个人补充

> 这块我在研究上面的时候的实验尝试, 无论理解/不理解 Go 的汇编相关的东西, 对上文的阅读是没有影响的, 因此这部分 **不用看**

本文中用到的编译/汇编, 使用下面的指令完成:

编译为目标文件

```console
> go tool compile -N -l main.go
```

目标文件反汇编

```console
> go tool objdump -gnu main.o
```

```as
TEXT %22%22.axx(SB) gofile../root/xxx/main.go
  main.go:3		0x428			48c744240800000000	MOVQ $0x0, 0x8(SP)                   // movq $0x0,0x8(%rsp)
  main.go:4		0x431			48c744240801000000	MOVQ $0x1, 0x8(SP)                   // movq $0x1,0x8(%rsp)
  main.go:4		0x43a			c3			        RET                                  // retq

TEXT %22%22.main(SB) gofile../root/xxx/main.go
  main.go:7		0x43b			64488b0c2500000000	MOVQ FS:0, CX                    // mov %fs:,%rcx		[5:9]R_TLS_LE
  main.go:7		0x444			483b6110		CMPQ 0x10(CX), SP                    // cmp 0x10(%rcx),%rsp
  main.go:7		0x448			7645			JBE 0x48f                            // jbe 0x48f
  main.go:7		0x44a			4883ec18		SUBQ $0x18, SP                       // sub $0x18,%rsp
  main.go:7		0x44e			48896c2410		MOVQ BP, 0x10(SP)                    // mov %rbp,0x10(%rsp)
  main.go:7		0x453			488d6c2410		LEAQ 0x10(SP), BP                    // lea 0x10(%rsp),%rbp
  main.go:8		0x458			0f1f00			NOPL 0(AX)                           // nopl (%rax)
  main.go:8		0x45b			e800000000		CALL 0x460                           // callq 0x460		[1:5]R_CALL:"".axx
  main.go:8		0x460			488b0424		MOVQ 0(SP), AX                       // mov (%rsp),%rax
  main.go:8		0x464			4889442408		MOVQ AX, 0x8(SP)                     // mov %rax,0x8(%rsp)
  main.go:8		0x469			e800000000		CALL 0x46e                           // callq 0x46e		[1:5]R_CALL:runtime.printlock
  main.go:8		0x46e			488b442408		MOVQ 0x8(SP), AX                     // mov 0x8(%rsp),%rax
  main.go:8		0x473			48890424		MOVQ AX, 0(SP)                       // mov %rax,(%rsp)
  main.go:8		0x477			0f1f4000		NOPL 0(AX)                           // nopl (%rax)
  main.go:8		0x47b			e800000000		CALL 0x480                           // callq 0x480		[1:5]R_CALL:runtime.printint
  main.go:8		0x480			e800000000		CALL 0x485                           // callq 0x485		[1:5]R_CALL:runtime.printunlock
  main.go:9		0x485			488b6c2410		MOVQ 0x10(SP), BP                    // mov 0x10(%rsp),%rbp
  main.go:9		0x48a			4883c418		ADDQ $0x18, SP                       // add $0x18,%rsp
  main.go:9		0x48e			c3			    RET                                  // retq
  main.go:7		0x48f			e800000000		CALL 0x494                           // callq 0x494		[1:5]R_CALL:runtime.morestack_noctxt
  main.go:7		0x494			eba5			JMP %22%22.main(SB)                  // jmp 0x43b
```

对上面汇编的一点解释:

> 预备知识:
>
> 1. [线程本地存储 TLS](https://zhuanlan.zhihu.com/p/532514562)
> 2. [GO 语言的运行时初始化过程解析](https://zhuanlan.zhihu.com/p/426810334)

1. 0x43b ~ 0x448 是 Go 进入函数执行之前的初始化工作, 其实就是设置 TLS 以及运行时栈大小检查/分配
2. 0x44a ~ 0x453 是为 main 函数设置运行时栈, 栈空间大小设置为 `0x10`, 栈低指针被设置为 `0x10(%rsp)`, 也就是 `rsp` 再加上 10 的位置, 栈顶指针被设置为 `0x18(%rsp)` 位置.
3. 0x458 `nopl` 是一个无意义的指令, 纯粹是为了占位置. 如果函数有参数的话, 这里会是设置 `axx` 参数相关代码, `axx` 收到的参数实际上.
4. 0x45b 发起对函数 `axx` 的调用

栈设置的具体解释(注意栈是往下生长的):

```as
; 开辟栈空间, 操作前的栈顶记为 SP0, 栈顶指针现在是 SP0 - 0x18, 记为 SP1
; 此时 SP0-SP1 之间有 0x18/8 = 3 个 64 位存储单元: (高地址, sp0) [X, X, X] (低地址, sp1)
sub $0x18,%rsp

; 把原来的栈底记作 BP0, 这一步将它保存在 `0x10(%rsp)`
; 操作完了之后栈内容变为 (高地址, sp0) [BP0, X, X] (低地址, sp1)
mov %rbp,0x10(%rsp)

; 设置新的栈低, 操作完之后, 栈内容不变, 只是 BP 寄存器发生了变化
lea 0x10(%rsp),%rbp

; 剩下的那两个栈存储区域, 在 main 函数的其他地方用到了, 可以看上面完整的汇编
```
