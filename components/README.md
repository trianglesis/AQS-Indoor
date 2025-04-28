# Components

1. Wifi
2. Led
3. Display
4. ...
5. ...
6. ...


# Update

To make all custom componnets available we should have them at:

## Components hierarchy and CMakeLists

```text
└───{PROJ_ROOT}
    │   CMakeLists.txt
    │   pytest_hello_world.py
    │
    ├───.vscode
    │       c_cpp_properties.json
    │       launch.json
    │       settings.json
    │
    ├───components
    │   ├───comp_one
    │   │   │   CMakeLists.txt
    │   │   │   comp_one.c
    │   │   │
    │   │   └───include
    │   │           comp_one.h
    │   │
    │   └───comp_two
    │       │   CMakeLists.txt
    │       │   comp_two.c
    │       │
    │       └───include
    │               comp_two.h
    │
    └───main
            CMakeLists.txt
            hello_world_main.c
```

## Each component make file

Should be created automatically when you use IDF command to create new component: `Create new IDF component`

```text
idf_component_register(SRCS "comp_one.c"
                    INCLUDE_DIRS "include")

```

### Project root make file

```cmake
idf_component_register(
    SRCS 
        "hello_world_main.c"
    PRIV_REQUIRES 
                spi_flash
    REQUIRES 
             comp_one
             comp_two
    INCLUDE_DIRS 
    ""
)
```

### VS Code

- `.vscode\c_cpp_properties.json`

```json
{
    "configurations": [
        {
            "name": "ESP-IDF",
            "compilerPath": "${config:idf.toolsPathWin}\\tools\\riscv32-esp-elf\\esp-14.2.0_20241119\\riscv32-esp-elf\\bin\\riscv32-esp-elf-gcc.exe",
            "compileCommands": "${config:idf.buildPath}/compile_commands.json",
            "includePath": [
                "${config:idf.espIdfPath}/components/**",
                "${config:idf.espIdfPathWin}/components/**",
                "${workspaceFolder}/**"
            ],
            "browse": {
                "path": [
                    "${config:idf.espIdfPath}/components",
                    "${config:idf.espIdfPathWin}/components",
                    "${workspaceFolder}"
                ],
                "limitSymbolsToIncludedHeaders": true
            }
        }
    ],
    "version": 4
}
```