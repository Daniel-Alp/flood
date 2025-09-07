# Flood
Multi-paradign programming language, similar to Lua. To build the interpreter (on Linux or MacOS)
```
git clone https://github.com/Daniel-Alp/flood
cd flood
mkdir build
cd build
cmake ..
cd ..
cmake --build build
```
then do 
```
./build/flood my_script.fl
```
Below is a small tour of my language, for more complex examples see `tests/misc`

## Functions
```
fn fib(n) {
    if (n <= 1) {
        return n;
    }
    return fib(n-1) + fib(n-2);
}

fn main() {
    print fib(100);
}
```
First-class functions and closures are also supported
```
fn main() {
    var list = [1, 2, 3, 4, 6, 7, 8, 9, 10];

    var threshold = 4;
    fn pred(x) {
        return x > threshold;
    }

    print list.filter(pred);
}
```

## Classes
```
class Point {
    fn init(x, y) {
        self.x = x;
        self.y = y;
    }

    fn hypot_sq(other) {
        var dx = self.x - other.x;
        var dy = self.y - other.y;
        return dx*dx + dy*dy;
    }
}

fn main() {
    var pt1 = Point(0, 0);
    var pt2 = Point(3, 4);
    print pt1.hypot_sq(pt2);
    print pt2.hypot_sq(pt1);
}
```

## Foreign Functions
Methods on lists are implemented as functions in C which can be called from Flood.
```
fn main() {
    var list = [1, 2, 3, 4];
    print list.pop();
    list.push(5);
    print list;
}
```
