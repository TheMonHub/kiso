KISO Runtime
------------

This document describes the environment in which KISO applications
run on. This document assumes the KISO runtime is running a KAF file.
Most of these are only documented if it is not written or documented
in a KISO Lua definition file.

package.cpath
-------------

package.cpath is set to only search for libraries in the same
directory and path to unpacked libraries from the same KAF file
the application started on.

package.path
------------

package.path is set to only search for Lua files (.lua) or bytecodes
(.luac) in the same directory or path to unpacked modules from the
same KAF file the application started on.

Application Loop
----------------

1. KISO runtime starts
2. KISO prepares the application data, modules, libraries provided by
   the KAF file.
3. KISO runs kiso_boot to initialize the default environment.
4. KISO runs the application's entry point file.
5. KISO enters the application loop.
6. KISO runs the registered callbacks if there are any. With the exit
   being at the top priority.
7. KISO runs the update callback if the application is not exiting.
8. KISO sleeps to limit the callback rate if set.
9. KISO repeats steps 6–8 until the application exits.