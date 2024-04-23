# 2nd lab

### What does this program do?

This program is able to fork and run a child process speicfied with `CHILD_PATH` variable.

### How to build this project

This project can be built in release and debug modes using `make` utility. 

```
make MODE=release
make MODE=release TARGET=child
```

or

```
make MODE=debug
make MODE=debug TARGET=child
```

### How to use this program

Program can be controlled using following keys:

* `+` to run child according to `getenv()`
* `*` to run child according to `envp`
* `&` to run child according to `environ`
* `q` to exit
