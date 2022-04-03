# Interior mutability in Rust, part 2: thread safety

# Key takeaways

- We can have thread-safe interior mutability through `Mutex`, `RwLock` and the various `Atomic*` types in `std::sync`
- `Mutex` allows only one thread at a time, has no direct thread-unsafe counterpart, but can be thought of as only giving `&mut T` references
- `RwLock` is equivalent to `RefCell`, and also allows multiple readers or one writer
- The `Atomic` types are equivalent to `Cell`
- `std::sync::Arc` is the thread-safe version of `std::rc::Rc`, necessary to share references among threads
- These thread-safe types have additional run-time cost compared to their unsynchronized counterparts
- In Rust, we lock/protect data, not code

*This article is part of a series about interior mutability in Rust. You can read part 1 here and part 3 here*.

# Introduction

In the [previous article](https://ricardomartins.cc/2016/06/08/interior-mutability), we looked into [`Cell`](https://doc.rust-lang.org/std/cell/struct.Cell.html) and [`RefCell`](https://doc.rust-lang.org/std/cell/struct.RefCell.html)  as a way to achieve interior mutability—the ability to mutate  particular fields in a structure regardless of its exterior (explicit or  declared) mutability.

However, neither `Cell` nor `RefCell` are appropriate to share data among threads. Since `Cell` has no borrow checking mechanisms and its operations aren’t [atomic](https://en.wikipedia.org/wiki/Linearizability), it’s possible to have [race conditions](https://en.wikipedia.org/wiki/Race_condition#Software) where we read outdated values and lose updates. `RefCell`,  on the other hand, has run-time borrow checking, but will panic when  there are conflicting borrows, such as borrowing a mutably borrowed  value. Additionally, `RefCell` uses a `Cell`  to keep track of the borrow state. This means that even if you were  careful and checked it before borrowing, you would risk mutably  borrowing the value in multiple threads simultaneously, because its  run-time borrow checks aren’t atomic either.

Because of those issues, both `Cell` and `RefCell` are marked [`!Sync`](https://doc.rust-lang.org/std/marker/trait.Sync.html), which means they’re unsafe to be used in more than one thread.[1](https://ricardomartins.cc/2016/06/25/interior-mutability-thread-safety#fn:1)

Additionally, we need to share references to the cell among threads, but the reference counter type we explored previously, `Rc`, is also not appropriate to be used in this scenario. The reference counter fields inside `Rc` are wrapped by `Cell`, so they would get erroneous values in a threaded program sooner or later. The presence of `!Sync` fields is “infectious”. Since `Rc` contains two `Cell` fields, their `!Sync` markers propagate to the entire `Rc` structure. `Rc` is also marked [`!Send`](https://doc.rust-lang.org/std/marker/trait.Send.html) — unsafe to send (move) to other threads. Likewise, `!Send` is just as “infectious” as `!Sync`.

We could implement both traits ourselves in our types, but since  those traits are rather important, they’re marked as unsafe. So, we need  to tell the Rust compiler we know what we’re doing by prefixing their `impl` declarations with the `unsafe` keyword. For instance, if we wanted to make the nodes in the [example](https://play.rust-lang.org/?gist=9ccf40fae2347519fcae7dd42ddf5ed6) of the previous article `Send` and `Sync`:

```
unsafe impl<T> Send for Node<T> {}
unsafe impl<T> Sync for Node<T> {}
```

This does make the compiler shut up and allows us to get on with it, but what does it mean?

`Send` and `Sync` are automatically derived by the compiler for most types. If a type contains a `!Send` or `!Sync`  field, the “taint” always spreads to the parent types. By explicitly  implementing those traits, we’re implicitly telling the user of our API  that our types are thread-safe (`Sync`) and that they can be moved between threads safely (`Send`).

When we say our types are `Send` and `Sync`  without actually introducing synchronization mechanisms, we’re  disrespecting the “contracts” associated with those traits and  misleading the user. They’d get run-time panics or bad results due to  race conditions. Not a happy situation, at all.

# Thread-safe interior mutability

Fortunately, Rust gives us good tools to achieve interior mutability  in a thread-safe way without much effort. More than that, it does so in  such a way that the borrow checker will have our backs most of the time,  so we can’t shoot ourselves in the feet.

One of the really nice aspects about Rust is that once you get the  hang of the borrow system, you can use the same reasoning with interior  mutability in a single thread (`Cell` and `RefCell`) and in concurrent programs.

## For `Copy` values

For `Copy` values (eg., integers), instead of `Cell`, we have [`Atomic`](https://doc.rust-lang.org/std/sync/atomic/index.html) types (`std::sync::atomic::*`) that rely on assembly instructions to prevent data races:

- [`AtomicBool`](https://doc.rust-lang.org/std/sync/atomic/struct.AtomicBool.html), a boolean type,
- [`AtomicIsize`](https://doc.rust-lang.org/std/sync/atomic/struct.AtomicIsize.html), a signed integer type,
- [`AtomicUsize`](https://doc.rust-lang.org/std/sync/atomic/struct.AtomicUsize.html), an unsigned integer type, and
- [`AtomicPtr`](https://doc.rust-lang.org/std/sync/atomic/struct.AtomicPtr.html), a raw pointer type.

Even though there are only four types, you can can build on `AtomicPtr` to implement additional ones. Alternatively, you can use crates, such as the [`atom` crate](https://crates.io/crates/atom/), to do so with a simple API. Quick disclaimer: I haven’t tried `atom` but the example in [its README](https://github.com/slide-rs/atom/blob/master/readme.md) looks nice, and a quick glance at the source code looks exactly like what I would expect.

Using an atomic type is a bit more involved than doing the same with a `Cell`. Taking again the naive reference counter example of the previous article, using an `AtomicUsize` we’d have:

```rust
use std::sync::atomic::{AtomicUsize, Ordering};

struct NaiveRc<T> {
    reference_count: AtomicUsize,
    inner_value: T,
}

impl<T> Clone for NaiveRc<T> {
    fn clone(&self) -> Self {
        self.reference_count.fetch_add(1, Ordering::Relaxed);
        // ...
    }
}
```

As you can see, instead of simply assigning a new value to `reference_count`, we called `fetch_add` to atomically increment it. The first parameter is the increment size, while the second is new. An [`Ordering`](https://doc.rust-lang.org/std/sync/struct.RwLock.html)  tells the compiler (and the CPU) how much freedom it has to reorder  instructions. I won’t delve into that, as the official documentation  explains it in sufficient detail.

## For non-Copy values

For non-copy values, [`std::sync::RwLock`](https://doc.rust-lang.org/std/sync/struct.RwLock.html) is the counterpart to `RefCell`. Like `RefCell`, `RwLock`  has semantics very similar to our old friend the borrow system, and  allows either several “readers” or one “writer”, but not both at the  same time, nor several “writers”.

Unlike `RefCell`, however, `RwLock`  doesn’t panic when there are incompatible borrows: if a thread needs a  mutable reference, it will just have to wait until the other threads  release the lock (i.e., stop using the borrowed values).

We can get shared, read-only references with `read` (equivalent to `borrow` in `RefCell`), or exclusive, mutable references with `write` (`borrow_mut`, respectively).[2](https://ricardomartins.cc/2016/06/25/interior-mutability-thread-safety#fn:2)

Converting the [graph example](https://play.rust-lang.org/?gist=9ccf40fae2347519fcae7dd42ddf5ed6) in the previous article to use `RwLock` instead of `RefCell` is straightforward: replace `RefCell` declarations with `RwLock`, change `borrow` to `read` and `borrow_mut` to `write`. We also need to replace `Rc` with `Arc` to be able to move references to other threads, but I’ll describe that later.

```rust
use std::thread;
use std::sync::{Arc, RwLock};

// Represents a reference to a node.
// This makes the code less repetitive to write and easier to read.
type NodeRef<T> = Arc<RwLock<_Node<T>>>;

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
        Node(Arc::new(RwLock::new(node)))
    }

    // Adds a directed edge from this node to other node.
    fn add_adjacent(&self, other: &Node<T>) {
        self.0
            .write()
            .expect("Failed to acquire a write lock on node")
            .adjacent.push(other.0.clone());
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
    let graph = Arc::new(Graph::with_nodes(vec![node_1, node_2, node_3]));

    // Spawn a new thread that will print information about every node in the
    // graph.
    // The new scope makes this block more obviously different from the code
    // surrounding it and lets us group variables that will be moved into the
    // new thread, such as "graph".
    let guard = {
        let graph = graph.clone();
        let message = "Failed to acquire a read lock";
        thread::spawn(move || {
            for _ in 0..10 {
                // Show every node in the graph and list their neighbors
                for node in &graph.nodes {
                    let node = node.0.read().expect(&message);
                    let value = node.inner_value;
                    let neighbours = node.adjacent
                        .iter()
                        .map(|n| n.read().expect(&message).inner_value)
                        .collect::<Vec<_>>();
                    println!("node ({}) is connected to: {:?}", value, neighbours);
                }
                println!("-------------");
                // Give the main thread a chance to run
                thread::yield_now();
            }
        })
    };

    for _ in 0..10 {
        // Update the value of every node in the graph
        for node in &graph.nodes {
            let mut node = node.0.write().expect("Failed to acquire a write lock");
            node.inner_value += 10;
        }
        // Give the other thread a chance to run
        thread::yield_now();
    }

    // Wait for the other thread to end
    guard.join().expect("Error joining thread");
}
```

You can view the differences between the single- and multi-threaded versions in this [gist](https://gist.github.com/meqif/bd8a85654b230e8ecd1d3344fc707de9/revisions).

Apart from creating a new thread that prints the graph’s information  and a new loop in the main thread that updates the node’s values, the  only important differences are:

- `Arc<RwLock<_>>` instead of `Rc<RefCell<_>>`
- Every call to `borrow` and `borrow_mut` was replaced with `read` and `write`, respectively.

I encourage you to make these changes yourself starting from the [previous article’s code](https://play.rust-lang.org/?gist=9ccf40fae2347519fcae7dd42ddf5ed6).  Making small, incremental changes and compiling the intermediate  versions usually helps me get an intuitive feel for how the API works.

Note that, unlike `borrow` and `borrow_mut`, `read` and `write` return a `LockResult`, which is a type alias for `Result<Guard, PoisonError<Guard>>`, and requires us to `match`, `unwrap` or `expect` it. The `Guard` is automatically coerced into a reference, so we can pretty much ignore it.

In my experience, you rarely need to deal with the error case for either `read` or `write`, as it only happens if another thread with a mutable reference (i.e., a successful `write`) panics. In that case, you have a more serious bug elsewhere that you need to take care of.

Both `read` and `write` will block the thread until it gets the requested lock. Since you may not want to wait indefinitely, you can also call `try_read` and `try_write` instead, which won’t block and return an error if they couldn’t get a lock.

Besides `RwLock`, there is also [`std::sync::Mutex`](https://doc.rust-lang.org/std/sync/struct.Mutex.html),  whose name comes from “mutual exclusion”, because it guarantees  mutually exclusive access to the wrapped value, i.e., only one thread  can access it at a time. Because of this, it’s always safe to mutate the  value once you get access to it.

We can look at `Mutex` as if it were an `RwLock` without `read`, only able to give the caller mutable references. In this sense, it’s more constraining than the regular borrow system or an `RwLock`,  which allow either multiple simultaneous readers (with immutable  references) or only one writer (a mutable reference) at a time. Even if  we want an innocent immutable reference, we must get full permission to  the inner value.

As there is only one kind of borrow for `Mutex` values, `read` and `write` are replaced by a single `lock` method, which will block the thread until the current lock owner releases it (i.e., the other borrow ends). Like with `RwLock`, if you don’t want to block the thread when the value isn’t available, you can call `try_lock` instead, which will either give you the lock/mutable reference or an error (`Err`).[3](https://ricardomartins.cc/2016/06/25/interior-mutability-thread-safety#fn:3)

## Reference counting

As I mentioned earlier, `std::rc::Rc` lacks synchronization control, which makes it unsafe to be used by multiple threads. Its thread-safe counterpart is [`Arc`](https://doc.rust-lang.org/std/sync/struct.Arc.html), which lives in `std::sync`, along with `RwLock` and `Mutex`.

`Arc` is very similar to `Rc` but relies on `AtomicUsize` for the reference counter, which makes it safe to be updated by more than one thread, unlike `Rc`, which uses `Cell<usize>`.

```
pub struct Rc<T: ?Sized> {
    ptr: Shared<RcBox<T>>,
}

struct RcBox<T: ?Sized> {
    strong: Cell<usize>,
    weak: Cell<usize>,
    value: T,
}

pub struct Arc<T: ?Sized> {
    ptr: Shared<ArcInner<T>>,
}

struct ArcInner<T: ?Sized> {
    strong: atomic::AtomicUsize,
    weak: atomic::AtomicUsize,
    data: T,
}
```

`Arc`’s API is identical to `Rc`’s, which makes replacing one with the other a simple matter of searching and replacing the name and correcting the import.

# Closing thoughts

In the previous article, we learned that interior mutability in Rust can be achieved through `Cell` and `RefCell` in single-threaded programs, supported by `Rc` where necessary. In this article, we saw the same can be done in a safe way in concurrent programs with `Atomic` types and `RwLock`, with the help of `Arc`.

|                   | Single thread | Multiple threads  |
| ----------------- | ------------- | ----------------- |
| Copy value        | `Cell`        | `Atomic*`         |
| Non-Copy value    | `RefCell`     | `RwLock`, `Mutex` |
| Reference counter | `Rc`          | `Arc`             |

The table above summarizes the types to be used in single- and multi-threaded scenarios.

| Type of access       | Borrow checker | `RefCell`    | `RwLock` | `Mutex` |
| -------------------- | -------------- | ------------ | -------- | ------- |
| shared / read-only   | `&T`           | `borrow`     | `read`   | -       |
| exclusive / writable | `&mut T`       | `borrow_mut` | `write`  | `lock`  |

This second table highlights the similarities between the borrow checker, `RefCell`, `RwLock` and, to a lesser degree, `Mutex`.

You could ask, “Why bother opting for `RefCell` and `Rc` if `RwLock` and `Arc` have identical semantics and very similar APIs?”

Unfortunately, the types we explored in this article (`Atomic` types, `RwLock`, `Mutex` and `Arc`)  depend on synchronization primitives with higher run-time overhead than  their naive counterparts, and we’ll want to avoid them whenever  possible.

We can have a regular `Rc` and `RefCell`  combination for variables that aren’t shared with other threads, and  their synchronized versions for the bits you want to parallelize.  Because the API and semantics are similar in both cases, we will have  little cognitive overhead using both.

Another important point we must also pay attention to, is the semantics of the wrapping. For instance, `Arc<Vec<RwLock<T>>>` is different from `Arc<RwLock<Vec<T>>>`. With the first, we can’t concurrently mutate the vector itself, but we can mutate its stored values. This is a consequence of `Arc` implementing `Deref` but not `DerefMut`,  meaning we can only get immutable references to the vector (which  contains lockable elements). With the second form, we get an immutable  reference to `RwLock`, but since it can give us both kinds of references through `read` and `write`,  we can mutate the vector, adding or removing elements. However, we lose  the ability to concurrently mutate the values: once a thread gets a  mutable reference to the vector, the others will have to wait, while the  first form allows us to have one thread mutating each element in  parallel.

In short, `Arc<Vec<RwLock<T>>>` allows us to mutate all elements of the vector in parallel if we wish to do so, while `Arc<RwLock<Vec<T>>>` allows only one thread to modify the vector (and its values), leaving other threads waiting for the lock.

If `T` is itself wrapped by `Arc`, we would be unable to mutate the stored values at all (because `Arc` coerces only to immutable references). We would need a monstrosity like `Arc<RwLock<Vec<Arc<RwLock<T>>>>>`  to be able to concurrently mutate both the vector and its elements, but  we should take this a hint to rethink how you want to parallelize your  code.

Exploring interior mutability in a concurrent environment made me  realize that Rust is different from other languages regarding locks.  Whereas in other languages we use locks to protect code fragments, in  Rust we use them to protect access to data.

In addition, locks are so similar in use to Rust’s borrow mechanisms  that they feel like a thread-safe generalization of those mechanisms.  Personally, I find this way much easier to reason about than the  classic, C way.

Finally, I know it’s tempting, but don’t throw interior mutability  everywhere just to make the borrow checker shut up. Consider carefully  whether the situation really calls for interior mutability or a refactor  of your data structures. This goes doubly so for concurrent programs,  not only because `RwLock`, `Mutex` and `Arc`  incur additional run-time costs, but also because synchronization is  easy to mess up and leave you with race conditions. Fortunately, Rust’s  borrow checker gives us precious guidance and makes them much less  likely. Race conditions are nasty to debug, be especially careful about  dropping your locks as soon as possible.

That’s it. Now you know enough to effectively employ interior  mutability in your programs, whether they are single- or multi-threaded.  Well done! 🎉

My thanks to [/u/Manishearth](https://www.reddit.com/r/rust/comments/4puabs/interior_mutability_in_rust_part_2_thread_safety/d4octag), [/u/Steel_Neuron](https://www.reddit.com/r/rust/comments/4puabs/interior_mutability_in_rust_part_2_thread_safety/d4o0cna), and [/u/diwic](https://www.reddit.com/r/rust/comments/4puabs/interior_mutability_in_rust_part_2_thread_safety/d4pgcxy) for their feedback.

I hope you found this article useful and/or interesting. As always,  if you found a mistake or have any questions, please ping me on Twitter ([@meqif](https://twitter.com/meqif)) or send me an email ([words@ricardomartins.cc](mailto:words@ricardomartins.cc)). You can also join the discussion on [reddit](https://www.reddit.com/r/rust/comments/4puabs/interior_mutability_in_rust_part_2_thread_safety/).

1. We can implement a marker trait or its negation for a type. Currently this only works with marker traits, such as `Sync` and `Send`,  which don’t have associated methods but provide useful type  information. There is work being done toward generalizing this feature,  called negative traits, to any trait ([rust-lang/rust #13231](https://github.com/rust-lang/rust/issues/13231)). [↩](https://ricardomartins.cc/2016/06/25/interior-mutability-thread-safety#fnref:1)
2. Actually, `read` and `write` will return `Result<RwLockReadGuard>` or `Result<RwLockWriteGuard>`, respectively. `RwLockReadGuard` implements `Deref`, and `RwLockWriteGuard` implements both `Deref` and `DerefMut`, which are transparently coerced into mutable (`&T`) and immutable (`&mut T`) references, respectively. [↩](https://ricardomartins.cc/2016/06/25/interior-mutability-thread-safety#fnref:2)
3. Similarly to `RwLock`, calling `lock` on a `Mutex` will return a `Result<MutexGuard>`, which implements both `Deref` and `DerefMut`, and can be coerced into `&T` or `&mut T`, respectively. [↩](https://ricardomartins.cc/2016/06/25/interior-mutability-thread-safety#fnref:3)