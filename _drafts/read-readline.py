#!/bin/env python2
#-*- coding: utf-8 -*-

fh = open('/tmp/abc', 'r')
for line in fh.readlines(2):
    print line,

print "=" * 30

fh.seek(0, 0)

content = fh.readline(2)
while content:
    print "HAH", content,
    content = fh.readline(2)

print "=" * 30

fh.seek(0, 0)

content = fh.read(5)
print content

