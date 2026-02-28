## nspirectl - Manage files on TI-nspire devices

**WIP, may have bugs**

A command line wrapper for the libnspire functions as a command line tool. I made this mostly because I just cant get tilp to work. If there's any mistakes or you have any suggestions, even tiny little picky ones, dont hesitate to contact me or submit a pull request.

```
nspirectl - Manage files on TI-nspire devices

Usage:
  nspirectl [option]... <command> [<args>]

Description:
  A simple wrapper for the libnspire functions as a command line tool. I made
  this mostly because I just cant get tilp to work, but the raw functions do.
  If there's any mistakes or you have any suggestions, even tiny little picky
  ones, please dont hesitate to contact me or submit a pull request.

Commands:
  send, upload      Send a file or directory to the device
  read, download    Read a file from the device
  move, mv          Move or rename a file or directory
  copy, cp          Duplicate a file or directory
  list, ls          List files in a directory
  info              Show OS and device information
  screenshot        Take a screenshot of the device
  update            Update the OS on the calculator
  help              Show this help or the help for a command

Options:
  -v, --verbose     log what is being done
  -d, --debug       extra logging (implies -v)
      --help        show this help

Run 'nspirectl help <command>' for more information.
```

## Building
First, install [libnspire](https://github.com/Vogtinator/libnspire).

Then run `make` to compile the code and/or `make install` to copy it to `/usr/local/bin`. You can change the install location by setting `BINDIR` in the make command.


## Caveats

- colours in screenshots may be incorrect, particularly in the red channel
- `update` command has not been tested
- bit 5 of the hardware type flag is assumed to indicate CAS capability (used by the `info` command)
- `read` command can not read directories, only individual files
