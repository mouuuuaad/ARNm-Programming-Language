// Section 09: Structs
// Data structures in ARNm

window.addDocSection({
    id: 'structs',
    section: 'Data Structures',
    level: 1,
    title: 'Structs',
    content: `
# Structs in ARNm

Structs are custom data types that group related values together.

---

## Defining a Struct

\`\`\`arnm
struct Point {
    x: i32,
    y: i32
}

struct Rectangle {
    width: i32,
    height: i32,
    x: i32,
    y: i32
}

struct Config {
    mode: i32,
    limit: i32,
    enabled: bool
}
\`\`\`

---

## Creating Struct Instances

\`\`\`arnm
fn main() {
    let point = Point {
        x: 10,
        y: 20
    };
    
    let rect = Rectangle {
        width: 100,
        height: 50,
        x: 0,
        y: 0
    };
}
\`\`\`

---

## Accessing Fields

Use dot notation:

\`\`\`arnm
fn main() {
    let p = Point { x: 5, y: 10 };
    
    print(p.x);         // 5
    print(p.y);         // 10
    print(p.x + p.y);   // 15
}
\`\`\`

---

## Modifying Fields

For mutable structs:

\`\`\`arnm
fn main() {
    let mut p = Point { x: 0, y: 0 };
    
    p.x = 10;
    p.y = 20;
    
    print(p.x);   // 10
}
\`\`\`

---

## Structs in Functions

\`\`\`arnm
fn create_origin() -> Point {
    return Point { x: 0, y: 0 };
}

fn distance_from_origin(p: Point) -> i32 {
    return p.x * p.x + p.y * p.y;
}

fn main() {
    let origin = create_origin();
    let p = Point { x: 3, y: 4 };
    let dist = distance_from_origin(p);
    print(dist);   // 25
}
\`\`\`

---

## Nested Structs

\`\`\`arnm
struct Color {
    r: i32,
    g: i32,
    b: i32
}

struct Pixel {
    pos: Point,
    color: Color
}

fn main() {
    let pixel = Pixel {
        pos: Point { x: 100, y: 200 },
        color: Color { r: 255, g: 128, b: 0 }
    };
    
    print(pixel.pos.x);      // 100
    print(pixel.color.r);    // 255
}
\`\`\`

---

## Structs vs Actors

| Aspect | Struct | Actor |
|--------|--------|-------|
| Purpose | Data grouping | Concurrent entity |
| State | Passive data | Active with behavior |
| Access | Direct field access | Via \`self\` only |
| Concurrency | None | Message-based |
| Memory | Stack or heap | Always heap |
| Lifetime | Scope-based | Process-based |

---

## Common Patterns

### Configuration Struct

\`\`\`arnm
struct ServerConfig {
    port: i32,
    max_connections: i32,
    timeout_ms: i32,
    debug_mode: bool
}

fn start_server(config: ServerConfig) {
    print(config.port);
}
\`\`\`

### Result Struct

\`\`\`arnm
struct Result {
    value: i32,
    success: bool,
    error_code: i32
}

fn divide(a: i32, b: i32) -> Result {
    if (b == 0) {
        return Result { value: 0, success: false, error_code: 1 };
    }
    return Result { value: a / b, success: true, error_code: 0 };
}
\`\`\`

---

## Grammar

\`\`\`
struct_decl = "struct" IDENT "{" [ field_list ] "}" ;
field_list  = field { "," field } ;
field       = IDENT ":" type ;
\`\`\`
`
});
