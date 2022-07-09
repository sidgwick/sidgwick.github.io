```
certbot certonly --config-dir=config --work-dir=work --logs-dir=log -d "*.iuwei.fun"  -d "iuwei.fun" --manual --preferred-challenges dns-01 --server https://acme-v02.api.letsencrypt.org/directory
```

```
certbot renew --manual --config-dir=config --work-dir=work --logs-dir=log
```
