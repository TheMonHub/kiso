KISO Archive Format
-------------------

KISO Archive Format (.kiso) or simply KAF bundles a KISO application 
into a single file which then can be distributed and run on any
system provided that the system has KISO runtime support.

KAF uses the tar format to archive the data. The file must not be
compressed in a way that prevents the KISO runtime from extracting it
directly.

The file must contain a directory named `code` which contains the lua
bytecode of the application. The bytecode file should have the original
filename of the Lua source file referencable by the written Lua code
and must have the `.luac` extension.

The file must contain a directory named `data` which contains the
data files of the application. The data file should have the original
filename of the data file referencable by the written Lua code and
must have the original extension.

The file must contain a directory named `shaders` which contains the shader
SPIR-V bytecode of the application. The bytecode file should have the original
filename of the shader source file referencable by the written Lua code and
must have the `.spv` extension.

And a file named `manifest` which contains the application manifest encoded
in little endian and can be referenced by this C definition.

The current version is `1`.

```c++
typedef struct KisoIndent {
    char magic[5]; // "\x7fKISO"
    uint32_t version;
} KisoIndent;

typedef struct KisoManifest {
    KisoIndent indent;
    char luajit_hash[40 + 1]
    char entry_point[16 + 1];
} KisoManifest;
```