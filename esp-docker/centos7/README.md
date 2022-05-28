# Docker files for

# BUILD
YOLO weights must be present in the small/ subfolder.
YOLO Data tarball must be present and uncompresed in this folder level (centos7).

```bash
docker build -f <small/full>/Dockerfile -t <name>:<tag> ./<small/full>
```
# RUN
## Persistent container
```bash
docker run -uespuser -v “$(pwd)”/yolo-data:/home/espuser/yolo-data:ro -it <name>:<tag> /bin/bash
```
## Non-persistent container
```bash
docker run -uespuser -v “$(pwd)”/yolo-data:/home/espuser/yolo-data:ro --rm -it <name>:<tag> /bin/bash
```
## Graphical support non-persistent container
```bash
docker run -e DISPLAY --net=host -uespuser -v “$(pwd)”/yolo-data:/home/espuser/yolo-data:ro --rm -it <name>:<tag> /bin/bash
```
## Unix X11 Forward container
* Make sure owner and group of .Xauthority file is set to uid:gid = 1000:1000 or
* change in the dockerfile the uid of espuser to your local uid
```bash
docker run -e DISPLAY -v /tmp/.X11-unix -v $HOME/.Xauthority:/home/espuser/.Xauthority --net=host -uespuser -v “$(pwd)”/yolo-data:/home/espuser/yolo-data:ro --rm -it <name>:<tag> /bin/bash
```
# Troubleshooting
* If the build fails, try reducing the number of threads used in compilation in the Dockerfile lines 68/72 (default = 4)
