# VS Code and WSL notes

- If VS Code project metadata gets stale, remove the `.vs/` directory and let VS Code
  regenerate it.
- On WSL, `pkg-config` should resolve `glib-2.0` correctly:

```bash
pkg-config --cflags glib-2.0
pkg-config --libs glib-2.0
```

- If those commands fail, install the missing development packages first, such as
  `libglib2.0-dev` and `pkg-config`.
