---
title: "使用 Let's Encrypt 为网站添加 HTTPS 支持"
date: 2022-10-12 10:28:04
tags: letsencrypt, certbot, others
---

> WIP

## Let's Encrypt 介绍

<!--more-->

```bash
certbot certonly --config-dir=config --work-dir=work --logs-dir=log -d "\*.iuwei.fun" --manual --preferred-challenges dns-01 --server https://acme-v02.api.letsencrypt.org/directory
```
