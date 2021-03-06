# csh
> csh is an shell program written in C (think bash, but simpler!)

csh (**don't worry** I know csh already exists) is an interactive shell program. While its functionality is quite simple compared to fully-implemented shells like bash, csh still retains many of the core features that developers expect their shell to implement. 

I'll mention a few of these features here:
- Built-in functions like `cd`, `history`, `!` / `!!`, and `exit` are supported
- Executables can be run in csh with up to 10 arguments (if you need more just modify the `MAX_ARGS` parameter in src/csh.c)
- Piping between commands is also supported with no upper limit on the number of pipes you can use, so you can do all sorts of crazy stuff

![](etc/screencap.gif)

## Built-ins

Here is a complete list of supported built-in commands:
- `exit` - Exits the shell after performing clean up
- `cd [dir]` - Change the current working directory of the shell 
- `history [-c] [number]` - Similar to the built-in bash history command
	- `history` - Display the last 100 commands run by the user
	- `history -c` - Clear the history
	- `history [number]` - Display the last n (up to a max of 100) commands run by the user
- `!!` - Run the most recent command in history
- `!string` - Run the most recent command in history that starts with string

## Installation

The executable for csh can be built by simply using the included Makefile.

**Note**: A C compiler is required to build csh (more specifically, gcc as it is the compiler used in the Makefile)

```sh
make
./csh
```

## Testing

The testing suite for csh can be found under the `test` directory. A couple of tests have been included under homework\_tests.py, but you can add any additional tests you'd like!

Build csh using `make` and simply run `python tester.py` to test the build.

## Debugging

To enable debugging messages, build the project using `make debug`

## Meta

**Author**: [Matthew Chan](https://github.com/matthewachan)

Distributed under the GNU GPL v3.0 license. See ``LICENSE`` for more information.

## Contributing

1. [Fork](https://github.com/matthewachan/csh/fork) the repo
2. Create a feature branch (e.g. `git checkout -b feature/new_feature`)
3. Add & commit your changes
4. Check your code using the Linux checkpatch
5. Push to your feature branch (e.g. `git push origin feature/new_feature`)
6. Create a new pull request
