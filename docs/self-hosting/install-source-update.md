---
next:
  text: 'Login'
  link: '/hosted/howto/login'
---

# Update

```bash
systemctl stop slmix
sudo su - slmix
git pull
make cleaner
make release
make webui
logout
systemctl start slmix
```
