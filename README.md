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

## Optionals and Control Flow
```
var x_opt: ?Num = 32;
var y_opt: ?Num = Null;
var z_opt: ?Str = "true";

// error, `x_opt` may be Null
print("{}\n", x_opt+4);

if (x_opt) |x| {
    print("{}\n", x+4);
}

if (3 > 2 and x_opt and z_opt) |x, z| {
    print("{} {}\n", x, z);
}

while(x_opt) |x| {

}

// error, expected capture
if (x_opt) {
          
}

// error, do not know which variable to capture
if (x_opt or z_opt) |v| {

}

// error, condition can short circuit before we determine if `x_opt` is Null or not
if (3 > 2 or x_opt) |x| {

} 

// in general, when optionals appear in the condition of an if or while then 
// if the conditional evalutes to true, every optional must be not Null 
// and must be captured.
```

## Lists, Tuples, and Maps
```
var list1 = [1, 2, 3, 4];

for (list1) |x| {
    print("{}\n", x);
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

var entry_opt = map.get("alice");

// error, `entry_opt` may be Null
print("{}\n", entry_opt);

if (entry_opt) |entry| {
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
    // captured cons shadows other cons
    if (cons) |cons| {
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

print("{}\n", msg1.Move.x);

switch msg1 {
    .Quit => print("Quit\n");
    .Move => |move| print("Move {} {}\n", move.x, move.y);
    .Color => |color| print("Color {} {} {}\n", color.0, color.1, color.2);
}

// error, we do not know that msg2 is .Move variant
print("Move {} {}\n", msg2.Move.x, msg2.Move.y);

if (msg2 == .Move) {
    print("Move {} {}\n", msg2.Move.x, msg2.Move.y);
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

## Modules and Imports
I have not gotten to this yet...