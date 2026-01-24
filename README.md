## nspirectl - Manage files on TI-nspire devices

A simple wrapper for the libnspire functions as a command line tool. I made this mostly because I just cant get tilp to work. If there's any mistakes or you have any suggestions, even tiny little picky ones, dont hesitate to contact me or submit a pull request.

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
  send        Send files to the device
  read        Read a file from the device
  list        List files on the device
  move        Move or rename files
  copy        Duplicate files
  info        Show OS and device information
  help        Show this help or help for a command

Options:
  -v, --verbose     log what is being done
  -d, --debug       print out values as well (implies -v)
      --help        show this help

Run 'nspirectl help <command>' for details.
```

## Building
First, install [libnspire](https://github.com/Vogtinator/libnspire).

Then run
```
make
```
and put the `./nspirectl` binary anywhere you want, like `/usr/local/bin`.
