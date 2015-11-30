---
layout: post
title:  Convert man page to PDF
date:   2015-11-30 15:28:04
categories: linux shell
---

Convert man page to PDF

```bash
man -t bash | ps2pdf - bash.pdf
```

`man -t` uses groff -mandoc to format the manual page to stdout.

`ps2pdf – bash.pdf` means the input is from stdin and output to bash.pdf.

We use a simple pipe to join the stdout of `man` and stdin of `ps2pdf`.

That’s it!
