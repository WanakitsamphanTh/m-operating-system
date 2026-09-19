#include <cstdint>
#include <mstd/choice.hpp>
#include <string>
#include <iostream>
#include <type_traits>

struct animal {
    int age;
    enum class species {
        cat, dog, bird
    } specie;
    enum class categories {
        domestic, wild
    } category;
};

struct human {
    int age;
    enum class personal_status { alive, deceased } status;
    const char* name;
};

enum class planets: uint8_t {
    Mars, Jupiter, Saturn, Unknown
};

struct alien {
    planets origin = planets::Unknown;
};

using namespace std;
using none = mstd::nothing_t;
using mstd::choice;
using mstd::nothing;
using mstd::types_of;

using living_things = choice<alien, human, animal>;

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

//typedef 
template<class T, class Fn>
using match_fn_t = T(*)(void*, Fn&);

template<class T, class Fn>
auto static_match_fn(void* arg_ptr, Fn& fn) {
    if constexpr(std::is_invocable_v<Fn, T&>){
        if constexpr(std::is_void_v<std::invoke_result_t<Fn, T&>>){
            std::invoke(fn, *reinterpret_cast<T*>(arg_ptr));
        } else {
            return std::invoke(fn, *reinterpret_cast<T*>(arg_ptr));
        }
    } else {
        if constexpr(std::is_void_v<std::invoke_result_t<Fn>>){
            std::invoke(fn);
        } else {
            return std::invoke(fn);
        }
    }
}

template<class T, class Fn>
struct match_fn_result;

template<class T, class Fn>
    requires (std::is_invocable_v<Fn, T&>)
struct match_fn_result<T,Fn> {
    using type = std::invoke_result_t<Fn, T&>;
};

template<class T, class Fn>
    requires (!std::is_invocable_v<Fn, T&>)
struct match_fn_result<T,Fn> {
    using type = std::invoke_result_t<Fn>;
};

template<class Fn, class... Ts>
auto visit(choice<Ts...>& ch, Fn&& visitor){
    using T0 = typename mstd::indexed_type_list<0, Ts...>::type_at<0>;
    using F = remove_reference_t<Fn>;
    using R = typename match_fn_result<T0, F>::type;
    
    static match_fn_t<R, F> match_fn[] = {
        &static_match_fn<Ts, F>...
    };

    if constexpr(std::is_void_v<decltype(match_fn[0](&ch, visitor))>) {
        match_fn[ch.get_id()](&ch,visitor);
    } else {
        return match_fn[ch.get_id()](&ch, visitor);
    }
}

class killer {
    uint32_t count = 0;
public:
    void update() { count++; }
    uint32_t get_count() const { return count; }
    void operator()(){ cout << "killed something\n"; update(); }
    void operator()(human& hu){
        if(hu.status == human::personal_status::deceased) {
            cout << "cannot kill " << hu.name << " because he is already dead\n";
            update(); 
        } else {
            cout << "killed a person named " << hu.name << endl;
            hu.status = human::personal_status::deceased;
        }
    }
    void operator()(alien& al){ cout << "killed an alien from " << (uint32_t)static_cast<uint8_t>(al.origin) << "\n"; update(); }
};

template<class F>
struct first_argument;

template<class R, class T, class...Ts>
struct first_argument<R(T, Ts...)>{
    using type = T;
};

template<class R, class T, class...Ts>
struct first_argument<R(*)(T, Ts...)>{
    using type = T;
};

template<class R, class C, class T, class...Ts>
struct first_argument<R(C::*)(T, Ts...)>{
    using type = T;
};

template<class R, class C, class T, class...Ts>
struct first_argument<R(C::*)(T, Ts...) const>{
    using type = T;
};

template<class R, class C, class T, class...Ts>
struct first_argument<R(C::*)(T, Ts...) &>{
    using type = T;
};

template<class R, class C, class T, class...Ts>
struct first_argument<R(C::*)(T, Ts...) &&>{
    using type = T;
};

template<class F, class... Ts>
bool match_each(choice<Ts...>& ch, F& f){
    using _TArg = first_argument<decltype(&F::operator())>::type;
    using TArg = std::remove_cvref_t<_TArg>;
    if constexpr(mstd::is_in_v<TArg, Ts...>){
        if(ch.template is<TArg>()) {
            f(ch.template borrow_unchecked<TArg>());
            return true;
        } else { return false; }
    } else {}
}

template<class Ch, class F, class... Fs>
bool match(Ch& ch, F&& f, Fs&&... fs){
    bool matched = false;
    ((matched = match_each<F>(ch, f))
        || ((matched = match_each<Fs>(ch, fs)), ...));
    return matched;
}

int main() {
    class {
    public:
        int operator()(animal& an){ 
            return an.age;
        }
        int operator()(human& hu){
            return hu.age;
        }
        int operator()(){ return -1; }
    } ask_age;
    
    living_things living;
    print(living);
    living = human { 40, human::personal_status::alive, "Bob" };
    cout << "age of this being: " << visit(living, ask_age) << endl;

    killer k;
    visit(living, k);
    visit(living, k);

    print(living);
    living.set_at<0>(alien { planets::Jupiter } );
    print(living);

    switch(living.get_id()){
        case mstd::id_of<human>(living) :
            cout << "Currently human\n";
            break;
        case mstd::id_of<animal>(living) :
            cout << "Currently animal\n";
            break;
        default:
            cout << "dunno\n";
    }

    visit(living, k);

    living = animal { 10, animal::species::bird, animal::categories::domestic };
    visit(living, k);

    cout << "age of this being: " << visit(living, ask_age) << endl;
    
    cout << "kill count: " << k.get_count() << endl;

    living = alien { planets::Jupiter };

    
    if(!match(living,
            [&](human& hu) { cout << "matched human\n"; k(hu); },
            [](animal& an) { cout << "matched animal\n"; } )
        ){
            cout << "matched nothing!\n";
        }

    cout << "size\n";
    cout << "human " << sizeof(human) << endl;
    cout << "animal " << sizeof(animal) << endl;
    cout << "alien " << sizeof(alien) << endl;
    cout << "choice: " << sizeof(living) << endl;

    return 0;
}
