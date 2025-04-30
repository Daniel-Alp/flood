# Flood
This is the syntax I have in mind, *heavily* inspired by Zig. I hope to finish this one day... 

## Variables
```
var x = 32;

let y = "hello";
// error, `y` is immutable
y = "bye";

// error, cannot infer type of `z`
var z;

var w;
w = 32;

var u: Num;
```

## Printing
```
let x = 4;
// prints `4 4`
print("{x} {}", x);
```

## Optionals and Control Flow
```
var x: ?Num = 32;
var y: ?Num = Null;
var y: ?Str = "true";

// error, `x` may be Null
print("{}\n", x+4);

if (x) {
    print("{}\n", x+4);
}

if (3 > 2 and x and z) {
    print("{} {}\n", x, z);
}

while(x) {

}

if (x or z) {
    // error, `x` may be Null and `y` may be Null
    print("{} {}\n", x, z);
}

if (3 > 2 or x) {
    // error, `x` may be Null
    print("{} {}\n", x);
}
```

## Lists, Ranges, Tuples, and Maps
```
var list1 = [1, 2, 3, 4];

for (list1) |x| {
    print("{}\n", x);
}

for (list1, 0..) |x, i| {
    print("{} {}\n", x, i);
}

// prints 0 to 3
for (0..4) |i| {
    print("{}\n", i);
}

list1.push(5);
print("{}\n", list[4]);
lis1.pop();

// error, cannot infer type of `list2`
var list2 = [];

var list3: []Num = [];

var tup1 = .(0, 1, "hello");
tup1.0 = 1;

var tup2: (Num, Num);
tup2.0 = 1;
tup2.1 = 2;

var map = Map(Str, []Num);
map.insert("alice", [1, 2, 3]);

var entry = map.get("alice");

// error, `entry` may be Null
print("{}\n", entry);

if (entry_opt) {
    print("{}\n", entry);
}

map.remove("alice");

for (map) |k, v| {
    print("{} {}\n", k, v);
}
```

## Structs and Unions
```
struct Cons {
    data: Num,
    next: ?Cons 
}

fn has_entry(cons: ?Cons, n: Num) Bool {
    if (cons) {
        if (cons.data == n) {
            return true;
        }
        return has_entry(cons.next, n);
    }
    return false;
}

var cons1 = Cons{.data = 3, .next = Cons{.data = 4, .next = Null}};
// infer type 
var cons2: Cons = .{.data = 3, .next = .{.data = 4, .next = Null}};

struct Rectangle {
    height: Num,
    width: Num,
    fn area(self) Num {
        return self.height * self.width;
    }

    fn dimensions(self) Void {
        print("{}x{}\n", height, width);
    }
}

var rect = Rectangle{.height = 3, .width = 5};
print("{}\n", rect.area());

union Message {
    Quit,
    Move {x: Num, y: Num},
    Color (Num, Num, Num)
}

var msg1 = Message.Move{.x = 1, .y = 2};
var msg2: Message = .Move{.x = 1, .y = 2};
var msg3 = Message.Color(1, 2, 3);

// error, do not know that msg2 is .Move variant
print("Move {} {}\n", msg2.x, msg2.y);

switch msg1 {
    .Quit => print("Quit\n");
    .Move => print("Move {} {}\n", msg1.x, msg1.y);
    .Color => print("Color {} {} {}\n", msg1.0, msg1.1, msg1.2);
}

union Aexpr {
    Lit (Num),
    Var (Str),
    Add {lhs: Aexpr, rhs: Aexpr},
    Sub {lhs: Aexpr, rhs: Aexpr},
    Mul {lhs: Aexpr, rhs: Aexpr},
    Div {lhs: Aexpr, rhs: Aexpr}
}

var expr = Aexpr.Mul{
    .lhs = .Add{.lhs = .Lit(1), .rhs = .Lit(2)},
    .rhs = .Sub{.lhs = .Lit(3), .rhs = .Lit(4)}
};
```