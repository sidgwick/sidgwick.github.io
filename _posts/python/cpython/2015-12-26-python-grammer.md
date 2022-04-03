---
title: "Python解释器的部分数据结构"
date: 2015-12-26 12:14:04
tags: python cpython
---

刚刚学习了一下 dot 语言绘制图像, 现在来画几张数据结构图, 牛刀小试一下.

<!--more-->

源码:

```c
typedef struct _label {
       int     lb_type;
       char    *lb_str;
} label;

typedef struct _labellist {
       int     ll_nlabels;
       label   *ll_label;
} labellist;

typedef struct _arc {
       short           a_lbl;          /* Label of this arc */
       short           a_arrow;        /* State where this arc goes to */
} arc;

typedef struct _state {
       int              s_narcs;
       arc             *s_arc;         /* Array of arcs */

       /* Optional accelerators */
       int              s_lower;       /* Lowest label index */
       int              s_upper;       /* Highest label index */
       int             *s_accel;       /* Accelerator */
       int              s_accept;      /* Nonzero for accepting state */
} state;

typedef struct _dfa {
       int              d_type;        /* Non-terminal this represents */
       char            *d_name;        /* For printing */
       int              d_initial;     /* Initial state */
       int              d_nstates;
       state           *d_state;       /* Array of states */
       bitset           d_first;
} dfa;

typedef struct _grammar {
       int              g_ndfas;
       dfa             *g_dfa;         /* Array of DFAs */
       labellist        g_ll;
       int              g_start;       /* Start symbol of the grammar */
       int              g_accel;       /* Set if accelerators present */
} grammar;
```

dot 可以这样写:

```
digraph grammer {
    node[shape=record];
    rankdir = LR;

    grammer[label = "
        <g_ndfas> g_ndfas |
        <g_dfa> *g_dfa |
        <g_ll> g_ll |
        <g_start> g_start |
        <g_accel> g_accel
    "];

    dfa[label = "
        <d_type> d_type |
        <d_name> *d_name |
        <d_initial> d_initial |
        <d_nstates> d_nstates |
        <d_state> *d_state |
        <d_first> d_first
    "];

    state[label="
        <s_narcs> s_narcs |
        <s_arc> *s_arc |
        <s_lower> s_lower |
        <s_upper> s_upper |
        <s_accel> *s_accel |
        <s_accept> s_accept
    "];

    arc[label="
        <a_lbl> a_lbl |
        <a_arrow> a_arrow
    "];

    labellist[label="
        <ll_nlabels> ll_nlabels |
        <ll_label> *ll_label
    "];

    label[label="
        <lb_type> lb_type |
        <lb_str> *lb_str
    "];

    // 现在把结构体关联起来
    grammer:g_dfa->dfa:n;
    grammer:g_ll->labellist:n;

    dfa:d_state->state:n;
    state:s_arc->arc:n;

    labellist:ll_label->label:n;
}
```

效果如下:

![](/data/python/cpython/grammer.dot.svg)
