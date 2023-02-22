# CommiFS

## System overview
This filesystem optimizes memory usage. 

## Requirements
- `libfuse v2.9.9`

## Building
- `make` - Build filesystem and initialize file structure in `${pwd}/comiFolder`. It should contain two folders `comiData` and `files`. These are restriced names and cannot be changed.
- `make build` - Same as above.
- `make install` - Create binary file `comiFS`.
- `make run` - Mount the comiFS filesystem at `${pwd}/mountComiFolder`.
- `make clean` - Remove all the files created by our filesystem.
- `make all` - Runs both build and run scripts.

You can compile the project by yourself using scripts provided in `usefulScripts` folder.