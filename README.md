# Flood
Multi-paradign programming language, similar to Lua. To build the interpreter (on Linux or MacOS)
```
git clone https://github.com/Daniel-Alp/flood
cd flood
mkdir build
cd build
cmake ..
cd ..
cmake --build build -j$(nproc)
```
then do 
```
./build/flood my_script.fl
```
# TODO
Draft of how I plan to implement imports and foreign functions + foreign classes. 

Flood side:
```
import std/io;
import std/math as m;

fn main() {
    io.println("hello, world");
    io.println(m.sin(3.1415926));
}
```
Suppose our project structure is:
```
projdir
    dir1
        file1.fl
    dir2
        file2.fl
    marker
```
and `file2.fl` wants to import `file1.fl`. We would right the following in `file2.fl`.
```
import dir1/file1.fl
```
The `marker` file indicates the root of the project (in the future it'll probably have a list of dependencies, I'm not sure).
To run `file2.fl` we should do `flood path/to/file2.fl` and it goes up to find the marker.
All imports are relative to this marker. 

Note that an `import` statement doesn't "declare a variable". For example the following should not compile:
```
import std/io;

fn main() {
    io.println(io);
}
```
For now there will be no global variables. I have an idea for how I want to do them but they can wait.

To go a bit more into detail (for now, and maybe later) resolving `import`s is done at runtime, similar to how Python does it.

C++ side:
We need some way to let the user define their own modules, and within them have their own foreign functions and foreign classes. 

I imagine it would look something like this:
```cpp
VM vm;
Module mod = vm.new_module("my/cool/module"); // must follow the format IDENT(/IDENT)*
ForeignClass<Vec3> klass = mod.define_foreign_class<Vec3, Vec3(double, double, double)>("Vec3");
klass.define_foreign_method<&Vec3::add>("add");

mod.define_foreign_function<dot_prod>("dot");
```
`define_foreign_function` will basically be the same as what I'm already doing, and `ForeignClass` will essentially be a wrapper around a `ForeignClassObj*` that lets you define foreign methods safely. 

I'm basing off of: https://isocpp.org/wiki/faq/pointers-to-members
and https://sol2.readthedocs.io/en/latest/api/usertype.html (I wish I found this earlier, it's awesome).

So, that's what I'm going to try and work towards.
There's 100 other things I would like to do but before I do those things I want(/need) to get these features done, and then test it lots. 
