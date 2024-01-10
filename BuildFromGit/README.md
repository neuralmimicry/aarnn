# AARNN build from GIT


The docker git clone docker files using the git clone using https, and not ssh

If the account the username is asscilated to as 2FA enabled, use the Personal Access Toten, 
if it isn't then password needs to be supplied

Before the docker can be run the following directories must be to the same directory
has the docker files and script.
- configure (this is for db_contection.conf, simulation.conf and init.sql)

Also the following files which need to be copied locally
- .env

```bash
If 2FA enabled
./set_args.sh <Git username> <branch to built> <Personal Access Token>
e.g. ./set_args.sh cps release_1 PAT


Else
./set_args.sh <Git username> <branch to built> <Password>
e.g. ./set_args.sh cps release_1 PW

```

This will build the images and then start the containers.