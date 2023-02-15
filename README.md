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
Go to the directory exampleApp/GE and do the following.

mkdir build.debug  
cd build.debug  
cmake -D CMAKE_BUILD_TYPE=debug ..  
cmake --build .  
./hello
```

## An example app to read and write text and image

Let's see an example app storing the data in a local realm file, the default realm. 
- We shall use a collection named "Person" with attributes - name, age, profilePic.
- We will be writing an entry to the Person collection. Image will be choosen from the 'GE/input' folder.
- We will then read by fetching all the documents from "Person" collection. Save the profilePic image to "output" folder.

### Include necessary classes

```sh
#include <stdio.h>
#include <cpprealm/sdk.hpp>
#include <cpprealm/type_info.hpp>
#include <iostream>
#include <fstream>
#include <vector>
```

### Declare the realm object

```sh
struct Person: realm::object {
    realm::persisted<int> _id;
    realm::persisted<std::string> name;
    realm::persisted<int> age;
    realm::persisted<std::vector<uint8_t>> profilePic;
 
    static constexpr auto schema = realm::schema(
        "Person",
        realm::property<&Person::_id, true>("_id"), // primary key
        realm::property<&Person::name>("name"),
        realm::property<&Person::age>("age"),
        realm::property<&Person::profilePic>("profilePic"));
}; 
```

### Open default realm

```sh
    auto realm = realm::open<Person>();
```

### Write objects to realm

To add a document to Person collection, we need an image for profilePic.
Add some images to a directory for this example. A directory named "input" is used for this example. 
Get an image and convert it to vector<uint8_t> to story the binary data of the image in realm.

```sh
auto stream = std::ifstream("./input/users.png", std::ios::binary);
    std::vector<uint8_t> vectordata = std::vector<uint8_t>();
    vectordata.reserve(vectordata.size() + sizeof(stream));
    vectordata.insert(vectordata.begin(),
        std::istream_iterator<uint8_t>(stream),
        std::istream_iterator<uint8_t>());
    vectordata.end();
    vectordata.shrink_to_fit();
  ```

Write an object to realm

```sh
    auto person1 = Person { ._id = 7, .name = "MSD", .age = 40, .profilePic = vectordata };
    vectordata.clear();
    vectordata.shrink_to_fit();
    realm.write([&realm, &person1] {
        realm.add(person1);
    });
    vector<uint8_t>().swap(vectordata);
  ```

### Read objects from realm

Read all persons from realm. Then save the profilePic image to output path.


```sh
auto people = realm.objects<Person>();
    for (Person pers : people) {

        std::cout << pers.name << std::endl;

        // Read the image from realm and write it to a path
        static_assert(std::is_same<unsigned char, uint8_t>::value, "uint8_t is not unsigned char");
        realm::persisted<std::vector<uint8_t>> profImage = pers.profilePic;
        std::vector<uint8_t> vectorImage = profImage.operator*();
        uint8_t *data = vectorImage.data();
        // Write to file
        string imgpathPrefix = "./output/";
        string imgname = pers.name.operator*();
        string imgpathSufix = ".png";
        string imgpath = imgpathPrefix + imgname + imgpathSufix;
        FILE* stream = fopen(imgpath.c_str(), "w");
        for(int loop=0;loop < 32;++loop)
        {
            fwrite(data, sizeof(unsigned long long), vectorImage.size(), stream);
        }
        fclose(stream);
    }```
