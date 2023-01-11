# realm_cpp_demo
A sample implementation of realm c++ SDK to read and write text / image on local realm.

## Installing Realm to the project

### MacOS / Linux

Prerequisites:

* git, cmake, cxx17

```sh
git submodule update --init --recursive
mkdir build.debug
cd build.debug
cmake -D CMAKE_BUILD_TYPE=debug ..
sudo cmake --build . --target install  
```

You can then link to your library with `-lcpprealm`.


**Make sure cpprealm is installed before hand, then build the example project.**

```
Change to the example/ge project directory.  
mkdir build.debug  
cd build.debug  
cmake -D CMAKE_BUILD_TYPE=debug ..  
cmake --build .  
./hello
```

