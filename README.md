# TODO 
Currently I'm adding lists and strings, and then I plan to let users define their own classes. This is the syntax I have in mind
```
class Point(pub var x, pub var y) {
    pub fn dist(other) {
        var a = x-other.x;
        var b = y-other.y;   
        return Math.sqrt(a*a + b*b);
    }
} 

var pt1 = Point(3, 4);
print(pt1.x);

var pt2 = Point(0, 0);
print(pt1.dist(pt2));
```
```
class Bar(var x, var y) {
    pub var z = x + y;

    fn foo(thing) {
        return thing * 2;
    }
    
    pub var w = foo(4);
}

var bar = Bar(1, 2);
print(bar.z);
```
The `...` in  `class Bar(var x, var y){...}` is the constructor of the class.

# OTHER
If I implement classes as described above, I may implement imports through classes. 
Every file is implicitly a class
```
// can't be accessed from other files
var z = 3;     
fn foo() {
    println("{}", z);
    return 3;
}
// can be accessed from other files
pub var x = foo();
```
