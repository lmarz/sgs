# SGS - Simple Git Server (WIP)
Linux only\
This will be a simple git http backend server written in C\
See the [Git Documentation](https://git-scm.com/docs/git-http-backend) for more info
## Dependencies
`git` (obviously)  
`libssl-dev`  
`libsqlite3-dev`
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
You also need to have a valid certificate and private key ready. To create your own certificate for localhost use, run
```
openssl req -x509 -out cert.pem -keyout key.pem \
  -newkey rsa:2048 -nodes -sha256 \
  -subj '/CN=localhost' -extensions EXT -config <( \
   printf "[dn]\nCN=localhost\n[req]\ndistinguished_name = dn\n[EXT]\nsubjectAltName=DNS:localhost\nkeyUsage=digitalSignature\nextendedKeyUsage=serverAuth")
```
The git client will throw an error, because this certificate is self signed. To disable key verification on the git client, run
```
git config --global http.sslverify false
```
**WARNING: DO NEVER USE THIS CERTIFICATE IN A PRODUCTION ENVIRONMENT. LET YOUR CERTIFICATE GET SIGNED BY A CA**  
To start the program, call
```
./sgs [-c <configfile.conf>] [-d]
```
```
Options:
    -a <user> <password> Add a user with push access
    -c <configfile.conf> The path to the config. If not specified, it will default to sgs.conf
    -d                   Run the program in the background
```
## License
This project is licensed under the terms of the MIT license, see [LICENSE](LICENSE)
