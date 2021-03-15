# Docker files for

# BUILD
HPVM tarball must be present in this folder level.

```bash
docker build -f <small/full>/Dockerfile -t <name>:<tag> .
```
# RUN
## Persistent container
```bash
docker run -it test:v1 /bin/bash 
```
## Non-persistent container
```bash
docker run --rm -it test:v1 /bin/bash 
```
