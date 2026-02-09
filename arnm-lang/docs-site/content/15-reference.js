// Section 15: Reference
// Keywords, grammar, and examples

window.addDocSections([
    {
        id: 'keywords',
        section: 'Reference',
        level: 1,
        title: 'Keywords',
        content: `
# Keywords

Reserved words in ARNm cannot be used as identifiers.

## Currently Implemented

| Keyword | Description |
|---------|-------------|
| \`actor\` | Define an actor |
| \`break\` | Exit loop |
| \`continue\` | Skip to next iteration |
| \`const\` | Constant binding |
| \`else\` | Alternative branch |
| \`false\` | Boolean false |
| \`fn\` | Function declaration |
| \`for\` | For loop |
| \`if\` | Conditional |
| \`let\` | Variable binding |
| \`loop\` | Infinite loop |
| \`mut\` | Mutable variable |
| \`nil\` | Null value |
| \`receive\` | Message receive |
| \`return\` | Return from function |
| \`self\` | Current process |
| \`spawn\` | Create process |
| \`struct\` | Structure definition |
| \`true\` | Boolean true |
| \`while\` | While loop |

## Reserved for Future

| Keyword | Planned Use |
|---------|-------------|
| \`enum\` | Enumerated type |
| \`immut\` | Immutable reference |
| \`match\` | Pattern matching |
| \`shared\` | Shared reference |
| \`type\` | Type alias |
| \`unique\` | Unique ownership |
`
    },
    {
        id: 'grammar',
        section: 'Reference',
        level: 1,
        title: 'Grammar',
        content: `
# Grammar Reference

Complete EBNF grammar for ARNm.

## Program Structure

\`\`\`
program     = { declaration } ;
declaration = function_decl | actor_decl | struct_decl ;
\`\`\`

## Functions

\`\`\`
function_decl = "fn" IDENT "(" [ param_list ] ")" [ "->" type ] block ;
param_list    = param { "," param } ;
param         = [ "mut" ] IDENT ":" type ;
\`\`\`

## Actors

\`\`\`
actor_decl    = "actor" IDENT "{" { actor_item } "}" ;
actor_item    = function_decl | receive_block ;
receive_block = "receive" "{" { receive_arm } "}" ;
receive_arm   = pattern "=>" block ;
\`\`\`

## Structs

\`\`\`
struct_decl = "struct" IDENT "{" [ field_list ] "}" ;
field_list  = field { "," field } ;
field       = IDENT ":" type ;
\`\`\`

## Statements

\`\`\`
block     = "{" { statement } "}" ;
statement = let_stmt | return_stmt | if_stmt | while_stmt
          | for_stmt | loop_stmt | break_stmt | continue_stmt
          | spawn_stmt | receive_stmt | expr_stmt ;

let_stmt      = "let" [ "mut" ] IDENT [ ":" type ] [ "=" expr ] ";" ;
const_stmt    = "const" IDENT [ ":" type ] "=" expr ";" ;
short_decl    = IDENT ":=" expr ";" ;
return_stmt   = "return" [ expr ] ";" ;
if_stmt       = "if" expr block [ "else" ( block | if_stmt ) ] ;
while_stmt    = "while" expr block ;
loop_stmt     = "loop" block ;
for_stmt      = "for" IDENT "in" expr block ;
break_stmt    = "break" ";" ;
continue_stmt = "continue" ";" ;
spawn_stmt    = "spawn" expr ";" ;
\`\`\`

## Expressions

\`\`\`
expression = assignment ;
assignment = logic_or [ "=" assignment ] ;
logic_or   = logic_and { "||" logic_and } ;
logic_and  = equality { "&&" equality } ;
equality   = comparison { ( "==" | "!=" ) comparison } ;
comparison = send { ( "<" | "<=" | ">" | ">=" ) send } ;
send       = term { "!" term } ;
term       = factor { ( "+" | "-" ) factor } ;
factor     = unary { ( "*" | "/" | "%" ) unary } ;
unary      = ( "-" | "!" | "~" ) unary | postfix ;
postfix    = primary { call | index | field } ;
\`\`\`

## Types

\`\`\`
type = IDENT
     | "fn" "(" [ type_list ] ")" [ "->" type ]
     ;
\`\`\`
`
    },
    {
        id: 'examples',
        section: 'Reference',
        level: 1,
        title: 'Examples',
        content: `
# Code Examples

Complete working examples to learn from.

---

## Hello World

\`\`\`arnm
fn main() {
    print(42);
}
\`\`\`

---

## Simple Counter

\`\`\`arnm
fn main() {
    let mut count = 0;
    while (count < 5) {
        print(count);
        count = count + 1;
    }
}
// Output: 0 1 2 3 4
\`\`\`

---

## Factorial

\`\`\`arnm
fn factorial(n: i32) -> i32 {
    if (n <= 1) {
        return 1;
    }
    return n * factorial(n - 1);
}

fn main() {
    print(factorial(5));   // 120
    print(factorial(10));  // 3628800
}
\`\`\`

---

## Simple Actor

\`\`\`arnm
actor Greeter {
    fn init() {
        print(100);  // Hello!
    }
}

fn main() {
    spawn Greeter();
}
\`\`\`

---

## Counter Actor

\`\`\`arnm
actor Counter {
    let count: i32;
    
    fn init() {
        self.count = 0;
    }
    
    receive {
        n => {
            self.count = self.count + n;
            print(self.count);
        }
    }
}

fn main() {
    let c = spawn Counter.init();
    c ! 10;  // prints 10
    c ! 20;  // prints 30
    c ! 30;  // prints 60
}
\`\`\`

---

## Ping-Pong

\`\`\`arnm
actor Pinger {
    fn init() {
        let pong = spawn Ponger();
        pong ! self;
    }
    
    receive {
        x => print(x);
    }
}

actor Ponger {
    receive {
        sender => {
            sender ! 42;
        }
    }
}

fn main() {
    spawn Pinger.init();
}
// Output: 42
\`\`\`

---

## Worker Pool Pattern

\`\`\`arnm
actor Worker {
    let id: i32;
    
    fn init() {
        self.id = 0;
    }
    
    receive {
        work_id => {
            print(100 + work_id);
        }
    }
}

fn main() {
    // Create 3 workers
    let w1 = spawn Worker();
    let w2 = spawn Worker();
    let w3 = spawn Worker();
    
    // Distribute work
    w1 ! 1;
    w2 ! 2;
    w3 ! 3;
    w1 ! 4;
    w2 ! 5;
}
\`\`\`

---

## Control Flow Showcase

\`\`\`arnm
actor Worker {
    let processed: i32;
    
    fn init() {
        self.processed = 0;
    }
    
    receive {
        1 => {
            // Arithmetic
            let result = (10 * 10) + (5 - 2);
            print(result);  // 103
        }
        2 => {
            // While loop
            let mut i = 0;
            while (i < 3) {
                print(200 + i);
                i = i + 1;
            }
        }
        3 => {
            // Loop with break/continue
            let mut j = 0;
            loop {
                j = j + 1;
                if (j == 2) { continue; }
                if (j > 4) { break; }
                print(300 + j);
            }
        }
    }
}

fn main() {
    let w = spawn Worker();
    w ! 1;
    w ! 2;
    w ! 3;
}
\`\`\`
`
    }
]);
