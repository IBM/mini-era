# Docker files for

# BUILD
docker build -f <small/full>/Dockerfile -t <name>:<tag> .

# RUN
## Persistent container
docker run -it test:v1 /bin/bash 
## Non-persistent container
docker run --rm -it test:v1 /bin/bash 
