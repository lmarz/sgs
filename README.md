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
./sgs [-c <configfile.conf>]
```
## License
This project is licensed under the terms of the MIT license, see [LICENSE](LICENSE)