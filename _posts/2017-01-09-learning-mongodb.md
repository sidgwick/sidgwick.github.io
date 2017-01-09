---
layout: post
title:  "MongoDB 学习笔记"
date:   2017-01-09 11:28:04
categories: jekyll
---

```
> show dbs
local  0.000GB
test   0.000GB
```

# 创建数据库

直接使用某一个数据库就可以创建它

```
> use ntest
switched to db ntest
> show dbs
local  0.000GB
test   0.000GB
```

不显示新创建的数据库是因为这个数据库是空的, 但它已经确实存在了.
我们插入点数居再看:

```
> show collections
> db.a.insert({'id': 1, 'name': 'song'})
WriteResult({ "nInserted" : 1 })
> show collections
a
> show dbs
local  0.000GB
ntest  0.000GB
test   0.000GB
```

# 插入文档

```
> db.a.insert({'id': 2, 'name': 'zhigang'})
WriteResult({ "nInserted" : 1 })
> db.a.find()
{ "_id" : ObjectId("58735159d26d5a97a2a7b78f"), "id" : 2, "name" : "zhigang" }
> db.a.insert({'id': 1, 'name': 'song'})
WriteResult({ "nInserted" : 1 })
> db.a.find()
{ "_id" : ObjectId("58735159d26d5a97a2a7b78f"), "id" : 2, "name" : "zhigang" }
{ "_id" : ObjectId("58735171d26d5a97a2a7b790"), "id" : 1, "name" : "song" }
```

也可以将数据定义为一个变量, 然后执行插入操作, 这样可以复用一部分数据:

```
> doc = {'id': 3, 'first-name': 'zhigang', 'last-name': 'song'}
{ "id" : 3, "first-name" : "zhigang", "last-name" : "song" }
> db.a.insert(doc)
WriteResult({ "nInserted" : 1 })
> db.a.find()
{ "_id" : ObjectId("58735159d26d5a97a2a7b78f"), "id" : 2, "name" : "zhigang" }
{ "_id" : ObjectId("58735171d26d5a97a2a7b790"), "id" : 1, "name" : "song" }
{ "_id" : ObjectId("587351f1d26d5a97a2a7b791"), "id" : 3, "first-name" : "zhigang", "last-name" : "song" }
> doc['first-name'] = 'xiaoqiang'
xiaoqiang
> doc['full-name'] = 'xiaoqiang song'
xiaoqiang song
> db.a.insert(doc)
WriteResult({ "nInserted" : 1 })
> db.a.find()
{ "_id" : ObjectId("58735159d26d5a97a2a7b78f"), "id" : 2, "name" : "zhigang" }
{ "_id" : ObjectId("58735171d26d5a97a2a7b790"), "id" : 1, "name" : "song" }
{ "_id" : ObjectId("587351f1d26d5a97a2a7b791"), "id" : 3, "first-name" : "zhigang", "last-name" : "song" }
{ "_id" : ObjectId("58735222d26d5a97a2a7b792"), "id" : 3, "first-name" : "xiaoqiang", "last-name" : "song", "full-name" : "xiaoqiang song" }
```

插入文档你也可以使用 `db.col.save(document)` 命令. 如果不指定 `_id` 字段 `save()`
方法类似于 `insert()` 方法, 如果指定 `_id` 字段, 则会更新该 `_id` 的数据.

```
> db.c.find()
{ "_id" : ObjectId("587356f1d26d5a97a2a7b793"), "id" : 3, "first-name" : "xiaoqiang", "last-name" : "song", "full-name" : "xiaoqiang song" }
> db.c.save(doc)
WriteResult({ "nInserted" : 1 })
> db.c.find()
{ "_id" : ObjectId("587356f1d26d5a97a2a7b793"), "id" : 3, "first-name" : "xiaoqiang", "last-name" : "song", "full-name" : "xiaoqiang song" }
{ "_id" : ObjectId("5873571ed26d5a97a2a7b794"), "id" : 3, "first-name" : "xiaoqiang", "last-name" : "song", "full-name" : "xiaoqiang song" }
> doc._id = new ObjectId('5873571ed26d5a97a2a7b794')
ObjectId("5873571ed26d5a97a2a7b794")
> db.c.save(doc)
WriteResult({ "nMatched" : 1, "nUpserted" : 0, "nModified" : 1 })
> db.c.find()
{ "_id" : ObjectId("587356f1d26d5a97a2a7b793"), "id" : 3, "first-name" : "xiaoqiang", "last-name" : "song", "full-name" : "xiaoqiang song" }
{ "_id" : ObjectId("5873571ed26d5a97a2a7b794"), "id" : 3, "first-name" : "xiaoqiang", "last-name" : "song", "full-name" : "Zhigang Song" }
```

可以看到, `_id` 符合的那条记录被更新了.



# 删除集合

```
> db.b.insert({'id': 1, 'name': 'song'})
WriteResult({ "nInserted" : 1 })
> show collections
a
b
> db.b.drop()
true
> show collections
a
```

# 删除数据库

```
> db.dropDatabase()
{ "dropped" : "ntest", "ok" : 1 }
> show dbs
local  0.000GB
test   0.000GB
```


