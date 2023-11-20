---
title: "使用 Let's Encrypt 为网站添加 HTTPS 支持"
date: 2022-10-12 10:28:04
tags: letsencrypt, certbot, others
---

> WIP

## Let's Encrypt 介绍

<!--more-->

## 完全手动版

```bash
certbot certonly --config-dir=config --work-dir=work --logs-dir=log -d "\*.iuwei.fun" --manual --preferred-challenges dns-01 --server https://acme-v02.api.letsencrypt.org/directory
```

## 自动版

### Aliyun DNS 插件安装

参考链接: [https://github.com/justjavac/certbot-dns-aliyun](https://github.com/justjavac/certbot-dns-aliyun)

Aliyun 自动添加 DNS 相关的 [API](https://api.aliyun.com/document/Alidns/2015-01-09/AddDomainRecord)

# 新签证书

```bash
certbot certonly --config-dir=config --work-dir=work --logs-dir=log -d "\*.iuwei.fun" -d "iuwei.fun" --manual --preferred-challenges dns-01 --manual-auth-hook "alidns" --manual-cleanup-hook "alidns clean" --server https://acme-v02.api.letsencrypt.org/directory
```

# 更新证书(可以用 crontab 跑)

```bash
certbot renew --config-dir=config --work-dir=work --logs-dir=log --manual --preferred-challenges dns-01 --manual-auth-hook "alidns" --manual-cleanup-hook "alidns clean" --deploy-hook "nginx -s reload"
```
