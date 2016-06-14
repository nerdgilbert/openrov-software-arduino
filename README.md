# OpenROV arduino software with NanoPb and Protocol Buffers

## You need to download Google's protoc compiler to build and run
Follow this [link](https://developers.google.com/protocol-buffers/docs/downloads)

## Building
* First, you need to intialize the protoc compiler
  * TODO: This might just be able to be done in the script, but for now just follow these steps:
    * `cd libs/nanopb/generator/proto`
    * `make`
* Now, from the top level directory
  * `cd scripts`
  * `python build_proto.py`
