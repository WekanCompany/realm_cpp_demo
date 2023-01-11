#include <chrono>
#include <stdio.h>
#include <future>
#include <thread>
#include <cpprealm/sdk.hpp>
#include <cpprealm/type_info.hpp>
#include <iostream>
#include <fstream>
#include <vector>
#include <string>


using namespace std;
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

void run_realm() {

    // open default realm
    auto realm = realm::open<Person>();

    //1. Read images from realm and save it to path in the person's name
    auto people = realm.objects<Person>();
    for (Person pers : people) {

        // Read the image from realm and write it to a path
        static_assert(std::is_same<unsigned char, uint8_t>::value, "uint8_t is not unsigned char");
        realm::persisted<std::vector<uint8_t>> profImage = pers.profilePic;
        std::vector<uint8_t> vectorImage = profImage.operator*();
        uint8_t *data = vectorImage.data();
        // Write to file
        string imgpathPrefix = "./images/";
        string imgname = pers.name.operator*();
        string imgpathSufix = ".png";
        string imgpath = imgpathPrefix + imgname + imgpathSufix;
        FILE* stream = fopen(imgpath.c_str(), "w");
        for(int loop=0;loop < 32;++loop)
        {
            fwrite(data, sizeof(unsigned long long), vectorImage.size(), stream);
        }
        fclose(stream);
    }

    // 2. Write image to realm
    // get an image vector from bundle
    auto stream = std::ifstream("./images/users.png", std::ios::binary);
    std::vector<uint8_t> vectordata = std::vector<uint8_t>();
    vectordata.reserve(vectordata.size() + sizeof(stream));
    vectordata.insert(vectordata.begin(),
        std::istream_iterator<uint8_t>(stream),
        std::istream_iterator<uint8_t>());
    vectordata.clear();
    vectordata.shrink_to_fit();

    //1. Write the image vector to realm in a Person Object as profilePic
    auto person1 = Person { ._id = 7, .name = "MSD", .age = 40, .profilePic = vectordata };
    vectordata.clear();
    // vectordata.shrink_to_fit();
    // Get the default Realm with compile time schema checking.
    //Persist your data easily with a write transaction 
    realm.write([&realm, &person1] {
        realm.add(person1);
    });
  vector<uint8_t>().swap(vectordata);

}

int main() {
    run_realm();
    return 0;
}
