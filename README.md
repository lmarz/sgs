# SGS - Simple Git Server (WIP)
Linux only\
This will be a simple git http backend server written in C\
See the [Git Documentation](https://git-scm.com/docs/git-http-backend) for more info
## Dependencies
`git` (obviously)
## Compiling
Just run the following command:
```
make
```
## Usage
First you need a repository. You can easily create one by calling
```
./setup.sh
```
To start the program, call
```
./sgs [-c <configfile.conf>] [-d]
```
```
Options:
    -c <configfile.conf> The path to the config. If not specified, it will default to sgs.conf
    -d                   Run the program in the background
```
## License
This project is licensed under the terms of the MIT license, see [LICENSE](LICENSE)