KISO
----

KISO is a desktop framework you can use to make all sorts of software
in lua, from games to tools. KISO is and will always be FOSS.

About
-----

KISO is a framework written in C23, which you can use to create
applications, games, and more for a desktop platform. It uses LuaJIT
as its scripting language.

KISO supports running on Windows, Linux, and in the future, macOS.
Currently, KISO doesn't support macOS. Contributions are welcome.

Building
--------

To build KISO, you will need the following installed in your system:
- Vulkan SDK (1.3+)
- GLFW (3.4+)
- Git
- CMake (4.4+)
- GCC (C23)
- LuaJIT (2.1)
- pkgconf

You can then directly run the build commands through CMake.

```commandline
cmake -B build -S.
cmake --build build
```

Contributing
------------

Consider contributing to KISO to help it improve for everyone.
Details regarding the contribution guidelines are written in
[CONTRIBUTING.md](CONTRIBUTING.md)

License
-------

KISO is distributed under Apache License, Version 2.0. An open-source,
permissive license. You can get its information at
www.apache.org/license/LICENSE-2.0 or in this repository.