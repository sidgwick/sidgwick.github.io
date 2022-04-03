# Interior mutability in Rust: what, why, how?

原文: [Interior mutability in Rust: what, why, how?](https://ricardomartins.cc/2016/06/08/interior-mutability)

> Rust is like doing parkour while suspended on strings & wearing  protective gear. Yes, it will sometimes look a little ridiculous, but  you’ll be able to do all sorts of cool moves without hurting yourself. –  [llogiq on reddit](https://www.reddit.com/r/rust/comments/4l44z3/why_should_i_use_rust/d3k7ayi)

# 要点

- 内部可变性是指你有一个不可变引用 (`＆T`) 但你可以改变它指向的目标内部
- 当您需要不可变数据结构中的可变字段时, 它很有用
- `std :: cell :: Cell <T>` 和 `std :: cell :: RefCell <T>` 可用于实现内部可变性
- `Cell` 用于包装实现了 `Copy` 的值, 它没有借用检查
- `RefCell` 可以包装任何类型的值, 有运行时借用检查. 需要使用 `borrow` 或 `borrow_mut` 来 "锁定" 目标值, 两个方法分别提供了不可变或可变引用
- 它们在多线程环境中直接使用都不安全,  应该使用 `Mutex` 或 `RwLock` 代替.

*本文是关于内部可变性的系列文章之一. 您也可以阅读剩下的第二部分和第三部分*

# 介绍

有时候, 即使被声明为不可变的数据结构体也需要令它的一个或者几个成员可变. 一开始听起来可能会让人觉得吃惊, 但是你很可能之前已经依赖过这些行为了, 比如说你在 `clone` 一个 `Rc` 引用计数包装的时候, 或者你锁定一个 `Mutex` 的时候. 然而, 在 Rust 里面, 可变性是个非黑即白的属性: 一个变量要么被声明为可变的, 连带它内部数据(比如结构体)一起都可变, 或者它声明为不可变, 连带它内部数据都不可变. 我们如何选择一些字段让它可变呢, 这里发生了什么神奇的事?

你是否好奇过 `Rc` 是如何实现的? 让我们来试一下, 下面是一个比较小白的实现:

```rust
struct NaiveRc<T> {
    reference_count: usize,
    inner_value: T,
}

impl Clone for NaiveRc<T> {
    fn clone(&self) -> Self {
        self.reference_count += 1;
        // ...
    }
}
```

你可能马上就发现了我们实现的瑕疵: `clone` 接受了一个 `self` 的只读引用, 所以引用计数不能被更新. 该死!

我们可以实现一个特别的, 不同名称的接受 `&mut self` 的克隆函数. 但是这样在使用起来就很吓人 (because it defies the convention of simply checking if a type implements `Clone`), 强制我们的 API 用户把实例声明成可变的. 我们还知道标准库里的 ([`std::rc::Rc`](https://doc.rust-lang.org/std/rc/struct.Rc.html) 和 [`std::sync::Arc`](https://doc.rust-lang.org/std/sync/struct.Arc.html)) 并不依赖这种方案, 这提示我们还有别的实现方法.

所以, 他们是如何在 `Rc` 和 `Arc` 里面解决这个问题的? 难道标准库依赖一些特别的奇技淫巧? 并不是这样的.

这是 **内部可变性** 的一个实例, Rust 语言给你提供了方便干净的处理这种情形的工具.

# What?

Interior mutability is a concept that many programmers new to Rust  have never come across before or haven’t had to explicitly think about.  It’s also more visible in Rust than in most other programming languages  because we have to think about the mutability or not of variables and  function arguments. This is even more evident when we look at immutable  vs. mutable as shared vs. exclusive access to objects. The unspoken  heuristic is that avoiding mutability when possible is good.

And yet, in some cases you need a few mutable fields in data  structures, whether they’re mutable or not. Interior mutability gives  you that additional flexibility and allows you to hide implementation  details from the user of your API, while preserving (some measure of)  access safety.

To explain what interior mutability is, it’s better to step back a  little and start with something you’re familiar about: exterior  mutability.

**Exterior mutability** is the sort of mutability you get from mutable references (`&mut T`). The type of declaration, `&T` or `&mut T`,  makes it clear if you’re free to update a variable or call mutating  methods on objects. Exterior mutability is checked and enforced at  compile-time, as you know:

```
struct Foo { x: u32 };

let foo = Foo { x: 1 };
// The borrow checker will complain about this and abort compilation
foo.x = 2;

let mut bar = Foo { x: 1 };
// 'bar' is mutable, so you can change the content of any of its fields
bar.x = 2;
```

If you have an immutable reference, you can’t change the value.  Conversely, because there is no field-level mutability in Rust, if you  want to mutate a single field, you need to make the entire structure  mutable, and that’s it. Or is it? Not so fast.

**Interior mutability**, in contrast, is when you have an immutable reference (i.e., `&T`) but you can mutate the data structure. As I mentioned before, that’s what happens when you clone an `Rc` or lock a `Mutex` (both `Mutex::lock` and `Mutex::try_lock` work in immutable instances).

A simple example will make the difference clearer. Suppose we have a simple structure like the following:

```
struct Point { x: i32, y: i32 }
```

An immutable `Point` can be seen  as an immutable memory chunk, whose fields (sections of the memory  chunk) can’t have their content changed at all. When you declare an  immutable `Point` your hands are tied.

Consider now a slightly different, magically-enhanced `MagicPoint`:

```
struct MagicPoint { x: i32, y: Magic<i32> }
```

​         ![Representation of Point and MagicPoint](https://ricardomartins.cc/2016/06/08/diagram.png) 

For now, ignore how `Magic`  works, and think of it as a pointer to a mutable memory address, a new  layer of indirection. Like previously, if you have an immutable `MagicPoint`, you can’t assign new values to any of its fields. However, in this case you don’t need to change the content of `y`, only the destination of that magical pointer, i.e., the other memory chunk, and that one *is* mutable![1](https://ricardomartins.cc/2016/06/08/interior-mutability#fn:1)

To be clear, even though the API for `Magic` will make it seem as if you’re relying on indirection to access and update the wrapped value, the memory representation of `MagicPoint` will actually be flat.

Note that when you rely on interior mutability, you are giving up the  compile-time safety guarantees that exterior mutability gives you. As  we’ll see next, it’s not that bad, provided you’re careful.

# How?

So, how can we get magical mutable pointers? Fortunately for us, the Rust standard library provides two wrappers, [`std::cell::Cell`](https://doc.rust-lang.org/std/cell/struct.Cell.html) and [`std::cell::RefCell`](https://doc.rust-lang.org/std/cell/struct.RefCell.html), that allow us to introduce interior mutability in externally immutable instances of data structures. With `Cell<T>` and `RefCell<T>` in our collective toolbelts, we can harness the power of interior mutability.

Both wrappers provide interior mutability and give up compile-time  borrow checking on the inner value, but give different safety guarantees  and serve different purposes. The most obvious difference between them  is that `RefCell` makes run-time borrow checks, while `Cell` does not.

`Cell` is quite simple to use: you can read and write a `Cell`’s inner value by calling `get` or `set`  on it. Since there are no compile-time or run-time checks, you do have  to be careful to avoid some bugs the borrow checker would stop you from  writing, such as accidentally overwriting the wrapped value:

```
use std::cell::Cell;

fn foo(cell: &Cell<u32>) {
    let value = cell.get();
    cell.set(value * 2);
}

fn main() {
    let cell = Cell::new(0);
    let value = cell.get();
    let new_value = cell.get() + 1;
    foo(&cell);
    cell.set(new_value); // oops, we clobbered the work done by foo
}
```

In contrast, a `RefCell` requires you to call `borrow` or `borrow_mut`  (immutable and mutable borrows) before using it, yielding a pointer to  the value. Its borrow semantics are identical to externally mutable  variables: you can have either a mutable borrow on the inner value or  several immutable borrows, so the kind of bug I mentioned earlier is  detected in run-time.

```
use std::cell::Cell;

struct NaiveRc<T> {
    inner_value: T,
    references: Cell<usize>,
}

impl<T> NaiveRc<T> {
    fn new(inner: T) -> Self {
        NaiveRc {
            inner_value: inner,
            references: Cell::new(1),
        }
    }

    fn references(&self) -> usize {
        self.references.get()
    }
}

impl<T: Clone> Clone for NaiveRc<T> {
    fn clone(&self) -> Self {
        self.references.set(self.references.get() + 1);
        NaiveRc {
            inner_value: self.inner_value.clone(),
            references: self.references.clone(),
        }
    }
}

fn main() {
    let wrapped = NaiveRc::new("Hello!");
    println!("references before cloning: {:?}", wrapped.references());
    let wrapped_clone = wrapped.clone();
    println!("references after cloning: {:?}", wrapped.references());
    println!("clone references: {:?}", wrapped_clone.references());
}
```

Calling `borrow` or `borrow_mut` on a mutably borrowed `RefCell` will cause a panic, as will calling `borrow_mut` on a immutably borrowed value. This aspect makes `RefCell` unsuitable to be used in a parallel scenario; you should use a thread-safe type (like a `Mutex` or a `RwLock`, for example) instead.

A `RefCell` will stay “locked” until the pointer you received falls out of scope, so you might want to declare a new block scope (ie., `{ ... }`) while working with the borrowed value, or even explicitly `drop` the borrowed value when you’re done with it, to avoid unpleasant surprises.

Another significant difference between `Cell` and `RefCell` is that `Cell<T>` requires that the inner value `T` implements `Copy`, while `RefCell<T>` has no such restriction. Often, you won’t want copy semantics on your wrapped types, so you’ll have to use `RefCell`.

Put succinctly, `Cell` has `Copy` semantics and provides *values*, while `RefCell` has `move` semantics and provides *references*.

# Why?

There are a few general cases that call for interior mutability, such as:

1. Introducing mutability inside of something immutable
2. Mutating implementations of `Clone`
3. Implementation details of logically immutable methods
4. Mutating reference-counted variables

## Introducing mutability inside of something immutable

Returning to the `NaiveRc` example in the introduction, reference-counting pointers, like `Rc` and `Arc`,  need internal mutability. When you clone those pointers, the reference  counter inside them has to be updated, whether they’re mutable or not.  Without interior mutability, you would be forced to always use mutable  pointers, which would allow mutation of the inner value and may be  undesired.

For instance, consider the following naive reference counted wrapper:

```
use std::cell::Cell;

struct NaiveRc<'a, T: 'a> {
    inner_value: &'a T,
    references: Cell<usize>
}

let x = NaiveRc { inner_value: &1, references: Cell::new(1) };
x.references.set(2); // it works!
x.inner_value = &2;  // beep boop, x is immutable,
                     // you can't assign a new value to any of its fields!
```

## Mutating implementations of `Clone`

Way back in the introduction, we noticed that cloning a reference-counted value (`Rc<T>`) needs to increment the reference counter. This is simply a special case of the previous point, but it deserves reiterating.

On the other hand, dropping such a value requires decrementing the reference counter, but `drop` works with mutable references (`fn drop(&mut self)`), so there’s no problem there.

## Implementation details of logically immutable methods

For instance, you might want to amortize the running time of an  expensive algorithm operating on your data structure by using a cache  inside it. The cache must be able to be updated even when the data  structure itself is immutable.

## Mutating reference-counted variables

Suppose we need multiple references to some objects. For example, when connecting nodes in a graph. “*Oh, that’s easy*”, you think. “*I’ll just wrap my nodes in Rc or Arc and call it a day*”.  That a perfectly reasonable line of though, and it would work… if you  never, ever needed to mutate nodes. Once you try building the graph by  incrementally adding and connecting nodes, the compiler will give you  grief. Oh no, what is going on? Unfortunately for us, `Rc` preserves safety by only giving you shared (i.e., immutable) references when you call `clone`. Quoth the [`std::rc` module documentation](https://doc.rust-lang.org/stable/std/rc/):

> The Rc<T> type provides shared ownership of an immutable  value. Destruction is deterministic, and will occur as soon as the last  owner is gone.

You could call `get_mut` to receive an `Option<&mut T>`, but that would work only once: `get_mut` only returns a mutable reference as if there is only one “strong” reference to the value.[2](https://ricardomartins.cc/2016/06/08/interior-mutability#fn:2) Foiled again!

Fortunately, you can use interior mutability here: use `Rc<Cell<T>>` or `Rc<RefCell<T>>`. That way you can `clone` the reference-counted wrapper as much as you want and still modify the innermost value wrapped by `Cell` or `RefCell`.

You can see a first try at a solution [in this example in the Rust Playground](https://play.rust-lang.org/?gist=bd01037fd9b9bb3d1a15bde61f580c6f).  As you can see, the problem is solved, but the solution is verbose and  ugly. Not only that, the user of our API is aware of implementation  details! What gives? Where’s the elegant abstraction I promised a few  paragraphs above?

Now that you’ve seen and understood how this works, I can show you a cleaner version:

```
use std::cell::RefCell;
use std::rc::Rc;

// A graph can be represented in several ways. For the sake of illustrating how
// interior mutability works in practice, let's go with the simplest
// representation: a list of nodes.
// Each node has an inner value and a list of adjacent nodes it is connected to
// (through a directed edge).
// That list of adjacent nodes cannot be the exclusive owner of those nodes, or
// else each node would have at most one edge to another node and the graph
// couldn't also own these nodes.
// We need to wrap Node with a reference-counted box, such as Rc or Arc. We'll
// go with Rc, because this is a toy example.
// However, Rc<T> and Arc<T> enforce memory safety by only giving out shared
// (i.e., immutable) references to the wrapped object, and we need mutability to
// be able to connect nodes together.
// The solution for this problem is wrapping Node in either Cell or RefCell, to
// restore mutability. We're going to use RefCell because Node<T> doesn't
// implement Copy (we don't want to have independent copies of nodes!).

// Represents a reference to a node.
// This makes the code less repetitive to write and easier to read.
type NodeRef<T> = Rc<RefCell<_Node<T>>>;

// The private representation of a node.
struct _Node<T> {
    inner_value: T,
    adjacent: Vec<NodeRef<T>>,
}

// The public representation of a node, with some syntactic sugar.
struct Node<T>(NodeRef<T>);

impl<T> Node<T> {
    // Creates a new node with no edges.
    fn new(inner: T) -> Node<T> {
        let node = _Node { inner_value: inner, adjacent: vec![] };
        Node(Rc::new(RefCell::new(node)))
    }

    // Adds a directed edge from this node to other node.
    fn add_adjacent(&self, other: &Node<T>) {
        (self.0.borrow_mut()).adjacent.push(other.0.clone());
    }
}

struct Graph<T> {
    nodes: Vec<Node<T>>,
}

impl<T> Graph<T> {
    fn with_nodes(nodes: Vec<Node<T>>) -> Self {
        Graph { nodes: nodes }
    }
}

fn main() {
    // Create some nodes
    let node_1 = Node::new(1);
    let node_2 = Node::new(2);
    let node_3 = Node::new(3);

    // Connect some of the nodes (with directed edges)
    node_1.add_adjacent(&node_2);
    node_1.add_adjacent(&node_3);
    node_2.add_adjacent(&node_1);
    node_3.add_adjacent(&node_1);

    // Add nodes to graph
    let graph = Graph::with_nodes(vec![node_1, node_2, node_3]);

    // Show every node in the graph and list their neighbors
    for node in graph.nodes.iter().map(|n| n.0.borrow()) {
        let value = node.inner_value;
        let neighbours = node.adjacent.iter()
            .map(|n| n.borrow().inner_value)
            .collect::<Vec<_>>();
        println!("node ({}) is connected to: {:?}", value, neighbours);
    }
}
```

If you ignore the loop that prints out the graph’s information, now the user doesn’t know how a `Node` is implemented. This version’s usability can still be improved by implementing the `std::fmt::Debug` trait for `Node` and `Graph`, for instance.

You can [play with this example](https://play.rust-lang.org/?gist=9ccf40fae2347519fcae7dd42ddf5ed6)  in the Rust Playground. Try changing some things yourself! I find  breaking things helps me consolidate new knowledge. I suggest:

1. Replacing `RefCell` with `Cell`
2. Removing `RefCell` and using `Rc<Node<T>>`
3. Removing `Rc` and using `RefCell<Node<T>>`

You could also try replacing `Rc` with `Arc`, but you wouldn’t notice anything different. `Arc` is a thread-safe version of `Rc`, which comes with a performance cost and doesn’t really make sense in single-threaded programs.

[An alternative solution](https://play.rust-lang.org/?gist=9ce1029d16433114f6bdda32b2e9fc03) could involve wrapping the adjacent node vector in a `RefCell`  instead of wrapping the node itself. That can also work, depending on  what you intend to do, but it is semantically different from the  previous solution, as you would be unable to mutate a node’s inner value  in addition to its list of adjacent nodes.

# Which to pick?

If `RefCell` can explode in your face and shouldn’t be used “raw” in a multi-threaded program, why bother using it?

While `Cell` is a good choice for many cases, there are a few reasons you might want to use `RefCell`:

1. The wrapped value doesn’t implement `Copy`.
2. Only `RefCell` has run-time checks. In some scenarios you’d rather kill the program than risk corrupting data.
3. `RefCell` exposes pointers to the stored value, `Cell` doesn’t.

As a rule of thumb, choose `Cell` if your wrapped value implements `Copy` (such as primitive values, like integers and floats). If the wrapped value is a `struct`, doesn’t implement `Copy` **or** you need dynamically checked borrows, use `RefCell` instead.

# Wrapping up

|           | Cell            | RefCell                                       |
| --------- | --------------- | --------------------------------------------- |
| Semantics | Copy            | Move                                          |
| Provides  | Values          | References                                    |
| Panics?   | Never           | Mixed borrows or more than one mutable borrow |
| Use with  | Primitive types | Structures or non-Copy types                  |

The table above summarizes what you learned in this blog post.

I hope you found this article useful and/or interesting. As always,  if you found a mistake or have any questions, please ping me on Twitter ([@meqif](https://twitter.com/meqif)) or send me an email ([words@ricardomartins.cc](mailto:words@ricardomartins.cc)). You can also join the discussion on [reddit](https://www.reddit.com/r/rust/comments/4na9p6/interior_mutability_in_rust_what_why_how/).

As [Steve Klabnik](https://www.reddit.com/r/rust/comments/4na9p6/interior_mutability_in_rust_what_why_how/d425bxq), [/u/critiqjo](https://www.reddit.com/r/rust/comments/4na9p6/interior_mutability_in_rust_what_why_how/d42dtcz) and [/u/birkenfield](https://www.reddit.com/r/rust/comments/4na9p6/interior_mutability_in_rust_what_why_how/d42gsdm) kindly pointed out, `Mutex` and `RwLock` already have interior mutability, so there’s no need to put a `Cell` inside them. In multi-threaded scenarios you should use `Mutex` and `RwLock` without an additional `Cell` or `RefCell`.

[/u/krdln](https://www.reddit.com/r/rust/comments/4na9p6/interior_mutability_in_rust_what_why_how/d42sn3z) suggested the alternative graph implementation above.

1. If you are familiar with C and this reminds you of `const` pointers (whose value also can’t change but the content at the destination memory address can), you are in the right track. `y` would be something like a `int *const`. [↩](https://ricardomartins.cc/2016/06/08/interior-mutability#fnref:1)
2. I really don’t want to get into the strong and weak reference  thing now. Suffice it to say that strong references stop objects from  being destroyed, while weak references don’t. [↩](https://ricardomartins.cc/2016/06/08/interior-mutability#fnref:2)

# Enjoyed the article?

Subscribe my newsletter to get the latest Rust tips and articles by email.

​                                                                            

​           [Powered by ConvertKit](https://convertkit.com/?utm_campaign=poweredby)                     

Thoughts or questions? Send me an email.

- [Github](https://github.com/meqif)
- [Feed](https://ricardomartins.cc/atom.xml)