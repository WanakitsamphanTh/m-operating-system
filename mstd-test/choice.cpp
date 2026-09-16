#include <mstd/choice.hpp>
#include <string>
#include <iostream>

struct animal {
    int age;
    enum class species {
        cat, dog, bird
    } specie;
};

struct human {
    int age;
    std::string name;
};

enum class planets {
    Mars, Jupiter, Saturn
};

struct alien {
    planets origin;
};

using namespace std;
using none = mstd::nothing_t;
using mstd::nothing;

using living_things = mstd::choice<none, human, animal, alien>;

auto print(living_things& ch){
    ch.match<animal>([](animal& an){
        string spec;
        if(an.specie == animal::species::bird) cout << "bird";
        else if(an.specie == animal::species::cat) cout << "cat";
        else if(an.specie == animal::species::dog) cout << "dog";
        cout << " " << an.age << endl;
    }).match<human>([](human& hu){
        cout << hu.name << " " << hu.age << endl;
    }).otherwise([](){
        cout << "something else" << endl;
    });
}

int main() {
    living_things living;
    print(living);
    living = human { 40, "Bob" };
    print(living);
    living.set_at<3>(alien { planets::Jupiter } );
    print(living);
    return 0;
}