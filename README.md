# so_priority.so

Set `SO_PRIORITY` (aka `skb->priority` on Linux) right after each `socket()` call.

Compile with:

```
gcc -shared -ldl -fPIC so_priority.c -o so_priority.so
```

Usage:

```
SO_PRIORITY_DEBUG=1 SO_PRIORITY_VALUE=6 LD_PRELOAD=/path/to/so_priority.so program arg1 arg2
```
