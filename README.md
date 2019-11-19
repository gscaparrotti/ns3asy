# ns3asy [![Build Status](https://travis-ci.org/gscaparrotti/ns3asy.svg?branch=develop)](https://travis-ci.org/gscaparrotti/ns3asy)

> A façade for the ns-3 simulator which makes it easier to build new simulations and allows the usage of the simulator using only pure C instead of C++.

---

## Installation

First of all, you need to have a C++ compiler on your machine (G++ 4.9 has been tested), Python and Git; please
refer to the [official documentation](https://www.nsnam.org/docs/release/3.29/tutorial/html/getting-started.html#prerequisites)
 for further information. Linux is the only officially supported platform
 (it should run on macOS as well; you can also try to install it on Windows using MinGW).

You need to download ns-3 from [its official repository](https://github.com/nsnam/ns-3-dev-git), extract ns3asy inside the src folder, configure it and
compile it.
You can find a brief recap below:

```bash
git clone --depth=1 https://github.com/nsnam/ns-3-dev-git ns-3-dev
git clone --depth=1 https://github.com/gscaparrotti/ns3asy ns-3-dev/src/ns3asy
cd ns-3-dev/
./waf configure
./waf build
```

ns3asy has been tested with ns-3.29 .

After doing so, you'll be ready to use ns3asy inside your simulation.

---

## Usage

ns3asy is mainly meant to be used through the functions that you can find inside the `ns3asy.h` header file, which
allow you to setup a simulation (set the number of nodes, the various callbacks function for the simulation events,
and so on), start it, stop it and interact with it while it's executing.

Just take a look at the function names: they should be self-explanatory.

Obviously, since ns-3 is written in C++, this library is written in C++, too; this means that you may find some
facilities other than the ones in `ns3asy.h` that you may want use on their own: feel free to do it, keeping in mind
that what we consider to be the "public interface" of ns3asy is only what's inside `ns3asy.h`.
