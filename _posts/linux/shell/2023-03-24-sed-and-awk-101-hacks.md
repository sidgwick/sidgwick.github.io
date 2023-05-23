---
title: "sed and awk 101 hacks 笔记"
date: 2023-03-24 02:28:04
tags: linux shell sed awk
---

> 图书在线阅读地址: [sed and awk 101 hacks](https://vds-admin.ru/sed-and-awk-101-hacks)

本文记录 _*[sed and awk 101 hacks](https://vds-admin.ru/sed-and-awk-101-hacks)*_ 一书中学到的知识点备忘.

- TOC
  {:toc}

# sed 部分

<!--more-->

1. `e` 将 pattern space 里面的内容当成命令执行, 并将执行结果当做 pattern space 的内容
2. 替换过程中的 `&` 字符
3. 正则匹配中的分组, 最多可以有 9 个
4. 注意反向引用(back references)术语

   - 反向引用不光可以用在要替换的字符串中, 也可以用在正则表达式本身.
   - `\(abc\)\1` 匹配 `abcabc`
   - 匹配两位一模一样的数字 `\([0-9]\)\1`

5. 替换字符串中的 \l, \L, \u, \U, \E 标记
6. 正则表达式

   - `\b` 单词边界, 可以用来表示单词的开始和结束
   - 开始结束 `^`, `$`
   - 匹配字符 `.`, `*`, `\+`, `\?`,
   - 转义 `\`
   - 字符集 `[]` ~~~ `[xxx]`, `[x-y]`
   - 或表达式 `|`

7. 正则表达式的 `{M,N}` 指定长度时, `M` 或者 `N` 的长度不可超过 255

   - `_{m}`
   - `_{m,n}`
   - `_{m,}`

8. `*.sed` 脚本中的头两个字符如果是 `#n`, sed 将会自动使用 `-n` 选项执行
9. `a`, `i` 命令其实可以支持多行写入
10. `c` 命令用于修改当前行, 和 `a`, `i` 类似, 他也可以支持多行写入
11. 注意 `l` 命令自带 print 效果
12. `=` 命令输出行号
13. `y` 命令用于将两个字符列表按照位置转换(transform)
14. sed 可以一次操作多个文本文件
15. `q` 在处理完成之后直接退出 sed 程序
16. `w` 命令用于将模式空间写到文件, `r` 命令用于将文件读到<标准输出还是模式空间?看上去不像是模式空间>
17. `x` 命令交换模式空间/保持空间
18. `g/G` 命令是 `get` 命令的缩写, 表示从保持空间拷贝/追加数据到模式空间
19. `h/H` 命令是 `hold` 命令的缩写, 表示从模式空间拷贝/追加数据到保持空间
20. `P` 多行模式空间的情况下, 仅输出第一行
21. `D` 多行模式空间的情况下, 仅删除第一行, 且不会将下一行读取到模式空间(对比 `d`), 并重新从头开始对模式空间内容执行命令
22. `b` 命令用于直接跳转到标号处
23. `t` 命令也是跳转到标号处, 但是有判断之前执行的替换操作是成功的这个前提才会跳转

# awk 部分

Awk 命令的基本格式

```bash
awk -Fs '/pattern/ {action}' input-file
# OR
awk -Fs '{action}' intput-file
# OR
awk -Fs -f myscript.awk input-file
```

Example:

```bash
awk -F: '/mail/ {print $1}' /etc/passwd
```

1. awk 的程序结构 BEGIN, body, END block

A typical awk program has following three blocks.
BEGIN Block
Syntax of begin block:
BEGIN { awk-commands }
The begin block gets executed only once at the beginning, before awk starts executing the body block for all the lines in the input file.
The begin block is a good place to print report headers, and initialize variables.
You can have one or more awk commands in the begin block.
The keyword BEGIN should be specified in upper case.
Begin block is optional.
Body Block
Syntax of body block:
/pattern/ {action}
The body block gets executed once for every line in the input file.
If the input file has 10 records, the commands in the body block will be executed 10 times (once for each record in the input file).
There is no keyword for the body block. We discussed pattern and action previously.
END Block
Syntax of end block:
END { awk-commands }
The end block gets executed only once at the end, after awk completes executing the body block for all the lines in the input-file.
• The end block is a good place to print a report footer and do any clean-up activities.
• You can have one or more awk commands in the end block.
• The keyword END should be specified in upper case.
• End block is optional.
awk work example Awk work example The following simple example shows the three awk blocks in action.
$ awk 'BEGIN { FS=":";print "---header---" } \
/mail/ {print $1} \
END { print "---footer---"}' /etc/passwd
---header---
mail
mailnull
---footer---
Note: When you have a very long command, you can either type is on a single line, or split it to multiple lines by specifying a \ at the end of each line. The above example is typed in 3 lines with a \ at the end of line 1 and line 2. In the above example:
BEGIN { FS=":";print "---header---" } is the begin block, that sets the field separator variable FS (more on this later), and prints the header. This gets executed only once before the body loop.
/mail/ {print $1} is the body loop, that contains a pattern and an action. i.e. This searches for the keyword "mail" in the input file and prints the 1st field.
END { print "---footer---"}' is the end block, that prints the footer.
/etc/passwd is the input file. The body loop gets executed for every records in this file.
Instead of executing the above simple example from the command line, you can also execute it from a file. First, create the following myscript.awk file that contains the begin, body, and end loop:
$ vi myscript.awk
BEGIN {
FS=":"
print "---header---"
}
/mail/ {
print $1
}
  END {
  print "---footer---"
}
Next, execute the myscript.awk as shown below for the input file /etc/passwd:
$ awk -f myscript.awk /etc/passwd
---header---
mail
mailnull
---footer---
Please note that a comment inside a awk script starts with #. If you are writing a complex awk script, follow the best practice: write enough comments inside the \*.awk file so that it will be easier for you to understand when you look at the file later. Following are some random simple examples that show you various combinations of awk blocks. Only the body block:
awk -F: '{ print $1 }' /etc/passwd
Begin, body, and end block:
awk -F: 'BEGIN { printf "username\n------\n"} \
{ print $1 } \
END { print "------" }' /etc/passwd
Begin, and body block:
awk -F: 'BEGIN { print "UID"} { print $3 }' /etc/passwd
A Note on using only a BEGIN Block:
Specifying only the begin block is valid awk syntax. When you don't specify a body loop, there is no point in specifying a input file, since only the body loop gets executed for the lines in the input file. So, use only the BEGIN block when you want to use an awk program to do things not related to file processing. In many of our examples below, we'll have only the BEGIN block, to explain how some of the awk programming components work. You can use this idea for anything that you see fit. A simple begin only example:
$ awk 'BEGIN { print "Hello World!" }'
Hello World!
Multiple Input Files
Please note that you can specify multiple input files. If you specify two input files, first the body block will be executed for all the lines in input-file1, next the body block will be executed for all the lines in input-file2. Multiple input file example:
$ awk 'BEGIN { FS=":";print "---header---" } \
/mail/ {print $1} \
END { print "---footer---"}' /etc/passwd /etc/group
---header---
mail
mailnull
mail
mailnull
---footer---
Please note that the BEGIN block and the END block will be executed only once, even when you specify multiple input-files.
53. Print Command
By default, the awk print command (without any argument) prints the full record as shown. The following example is equivalent to "cat employee.txt" command.
$ awk '{print}' employee.txt
101,John Doe,CEO
102,Jason Smith,IT Manager
103,Raj Reddy,Sysadmin
104,Anand Ram,Developer
105,Jane Miller,Sales Manager
You can also print specific fields in a record by passing $field-number as a print command argument. The following example is supposed to print only the employee name (field number 2) of every record.
$ awk '{print $2}' employee.txt
Doe,CEO
Smith,IT
Reddy,Sysadmin
Ram,Developer
Miller,Sales
Wait. It didn't work as expected. It printed from the last name until the end of the record. This is because the default field delimiter in Awk is space. Awk did exactly what we asked; it did print the 2nd field considering space as a delimiter. When the default space is used as delimiter, "101,John" became field-1 and "Doe,CEO" became field- 2 of the 1st record. So, the above awk example printed "Doe,CEO" as field-2. To solve this issue, we should instruct Awk to use comma (,) as field delimiter. Use option -F to indicate the field separator.
awk -F ',' '{print $2}' employee.txt
John Doe
Jason Smith
Raj Reddy
Anand Ram
Jane Miller
When there is only one character used for delimiter, any of the following forms works, i.e. you can specify the field delimiter within single quotes, or double quotes, or without any quotes as shown below.
awk -F ',' '{print $2}' employee.txt
awk -F "," '{print $2}' employee.txt
awk -F, '{print $2}' employee.txt
Note: You can also use the FS variable for this purpose. We'll review that in the awk built-in variables section. A simple report that prints employee name and title with a header and footer:
$ awk -F ',' 'BEGIN \
{ print "-------------\nName\tTitle\n-------------"} \
{ print $2,"\t",$3;} \
END { print "-------------"; }' employee.txt

---

## Name Title

John Doe CEO
Jason Smith IT Manager
Raj Reddy Sysadmin
Anand Ram Developer
Jane Miller Sales Manager

---

In the above report the fields are not aligned properly. We'll look at how to do that in later sections. The above example does show how you can use BEGIN to print a header, and END to print a footer. Please note that field $0 represents the whole record. Both of the following examples are the same; each prints the whole lines from employee.txt.
awk '{print}' employee.txt
awk '{print $0}' employee.txt
54. AWK Pattern Matching
You can execute awk commands only for lines that match a particular pattern. For example, the following prints the names and titles of the Managers:
$ awk -F ',' '/Manager/ {print $2, $3}' employee.txt
Jason Smith IT Manager
Jane Miller Sales Manager
The following example prints the employee name whose Emp id is 102:
$ awk -F ',' '/^102/ {print "Emp id 102 is", $2}' employee.txt
Emp id 102 is Jason Smith
