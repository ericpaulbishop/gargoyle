# Docker Gargoyle Builder

Build [Gargoyle](https://www.gargoyle-router.com/) images in a Docker container. This is sometimes necessary when building Gargoyle on the host system fails, e.g. when some dependency is too new. The docker image is based on the newest version of Ubuntu that was tested working with this branch.

Build tested:

- Gargoyle 1.10 (1.10.x)


## Prerequisites

* Docker installed
* Running Docker daemon
* Build Docker image:

```
git clone https://github.com/ericpaulbishop/gargoyle.git
cd gargoyle
git checkout 1.10
cd dev-utils/Docker
docker build -t gargoyle_builder_1.10.x .
```

Now the docker image is available. These steps only need to be done once.

## Usage GNU/Linux

Create a build folder (in this case we will use the just cloned Gargoyle repo) and link it into a new docker container:
```
docker run -v ~/gargoyle:/home/user -it gargoyle_builder_1.10.x /bin/bash
```
Note: If you don't use the cloned Gargoyle repo, you will need to clone it yourself from within the container.

In the container console, enter the make command for the target you wish to compile e.g.:
```
make FULL_BUILD=true ar71xx
```

After the build, the images will be inside `~/gargoyle/images/` and the packages inside `~/gargoyle/built/`.

## Other Projects

Other, but very similar projects:
* [docker-openwrt-buildroot](https://github.com/noonien/docker-openwrt-buildroot)
* [openwrt-docker-toolchain](https://github.com/mchsk/openwrt-docker-toolchain)

## Credits
To [mwarning](https://github.com/mwarning) for the original OpenWrt implementation this was based on.
