# Gargoyle Router Management Utility

Gargoyle is a third party firmware for routers based on the Openwrt firmware. It can be described at a high level as a front-end to Openwrt, however there are several packages and custom kernel modules which are not included upstream in Openwrt. As a result, Gargoyle cannot be installed as a set-of-packages to an existing Openwrt installation, and is a separate firmware as far as installation is concerned.
Gargoyle is open-source software, and aims to be the first open-source firmware project to place a strong focus on creating a user-friendly interface.

## Getting Started

As a developer, or even a casual contributor, you should start [here](https://www.gargoyle-router.com/wiki/doku.php?id=developer_info) at the Gargoyle wiki developer documentation.
You should always consult the wiki for up to date information on getting started (as it is maintained by the community, and this readme may not receive as much attention), however as a brief summary...

### Prerequisites

The following prerequisites assume a 64bit Ubuntu system. Many devs use virtual machines, and they are perfectly fine for this kind of work.

```
sudo apt-get install build-essential asciidoc binutils bzip2 gawk gettext git libncurses5-dev libz-dev patch unzip zlib1g-dev lib32gcc1 libc6-dev-i386 subversion flex uglifyjs \
git-core gcc-multilib p7zip p7zip-full msmtp libssl-dev texinfo npm libelf-dev clang flex bison g++ g++-multilib python3-distutils python3-dev rsync file wget
```

**NOTE** Python is also required and needs to be in your path. Python 3 is recommended. For Ubuntu users, it is recommended to install `python-is-python3` to solve this issue automatically.

### Get the source

You've already found it if you're reading this, but lets get it on your local machine. Choose one of the following commands depending on whether you want to use SSH or HTTPS authentication:

```
git@github.com:ericpaulbishop/gargoyle.git
https://github.com/ericpaulbishop/gargoyle.git
```

Then

```
cd gargoyle
```


## Building Gargoyle

After building is complete, you'll have <target>-src/, built/<target> and images/<target> folders. Retrieve your compiled firmwares from images, and the packages from built.

### Make all targets

```
make FULL_BUILD=true
```

### Make a single target (e.g. ar71xx)

```
make ar71xx FULL_BUILD=true
```

### Make a single profile (e.g. ar71xx.usb_large)

```
make ar71xx.usb_large FULL_BUILD=true
```

### Make a single target (e.g. ar71xx) without rebuilding Openwrt (just packages)

```
make ar71xx
```

### Make a custom target

See the wiki for more detailed instructions.

```
make custom
```

## Install

Please see the relevant Openwrt wiki or ToH entries for your device for the intricacies of installation, upgrading and failsafe for your device.

## Where do I get help?

The forums: [Gargoyle-Router Forums](https://www.gargoyle-router.com/phpbb/index.php)

## Contributing

Please read [CONTRIBUTING.md](https://github.com/ericpaulbishop/gargoyle/blob/master/CONTRIBUTING.md) for details on our code of conduct, and the process for submitting pull requests to us.

## Reporting an issue or bug

If you have discovered a genuine bug or other shortcoming in the firmware that you would like addressed, please first check the forums to see if it has already been raised and addressed. If the community is not able to address it for you, or the consensus is that it is a genuine bug, please raise an issue in GitHub.
**NOTE** Issues are not the place for feature requests. Please keep these to the forum.
Include as much detail as you can, such as:
* Gargoyle Version
* Router Make, Model and Version
* What were you doing when the bug occurred?
* What happened (what's the bug)?
* What did you expect to happen (what is your expectation of the proper behaviour)?
* Can you reproduce the bug everytime, sometimes, rarely? (For rare issues, start with the forums and see if others are seeing the same issue)
* Exact steps to reproduce the bug

## License

Gargoyle is copyright (C) 2008-2019 by Eric Bishop
Gargoyle is free software; you can redistribute it and/or modify it under the terms of the [GNU General Public License version 2.0](http://www.gnu.org/licenses/gpl-2.0.html) as published by the Free Software Foundation, with the following clarificaiton/exception that permits adpating the program to configure proprietary "back end" software provided that all modifications to the web interface portion remain covered by this license:

> The GNU General Public License (GPL) is vague as to what constitutes “mere aggregation” under section 2, and what contitutes a work “based on the Program.” In the special case in which the Program is modified for the purpose of configuring other (potentially GPL-incompatible) software, the combination of the Program and this other software shall be considered "mere aggregation" if and only if the ONLY interaction between the Program and the other software being configured takes place via CGI (Common Gateway Interface) scripts and/or programs. However, these CGI scripts/programs as well as any other additions and modifications necessary for the configuration of the other software shall be considered “based on the Program” for the purposes of this license. Further, if any portion of the Program is used as part of an interface that can be rendered via a web browser, all portions of that interface that can be rendered via a web browser (including, but not limited to, javascript, svg/ecmascript, css, html, and shell/perl/php/other cgi scripts) shall be considered “based on the Program.”
> This clarification/exception shall apply to the license of all derived works, and must appear in all relevant documentation. If you choose to release your modification to the Program under a later version of the GPL that directly contridicts this clarification/exception, this clarification/exception shall supersede any contradictory language in that version.

This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.

## Acknowledgments

* Eric Bishop - Project founder and lead developer
* The community - For using, supporting and contributing to Gargoyle
