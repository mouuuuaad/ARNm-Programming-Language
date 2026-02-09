// Section 10: Pattern Matching
// Patterns in receive blocks

window.addDocSection({
    id: 'patterns',
    section: 'Advanced',
    level: 1,
    title: 'Pattern Matching',
    content: `
# Pattern Matching in ARNm

Pattern matching allows you to destructure and match values, primarily used in receive blocks.

---

## Basic Patterns

### Variable Binding

Match anything and bind to a name:

\`\`\`arnm
receive {
    x => {
        // x now holds the message value
        print(x);
    }
}
\`\`\`

### Literal Patterns

Match specific values:

\`\`\`arnm
receive {
    1 => print(100);   // Match exactly 1
    2 => print(200);   // Match exactly 2
    42 => print(420);  // Match exactly 42
}
\`\`\`

---

## Pattern Matching in Receive

### Command Pattern

Use numbers as commands:

\`\`\`arnm
actor CommandProcessor {
    receive {
        1 => {
            // Command: Start
            print(100);
        }
        2 => {
            // Command: Stop
            print(200);
        }
        3 => {
            // Command: Reset
            print(300);
        }
        x => {
            // Unknown command
            print(x);
        }
    }
}
\`\`\`

### Catch-All Pattern

The variable pattern matches anything not matched above:

\`\`\`arnm
receive {
    1 => print(1);
    2 => print(2);
    other => {
        // Handles 3, 4, 5, ... anything else
        print(other);
    }
}
\`\`\`

---

## Pattern Order Matters

Patterns are checked top-to-bottom. First match wins:

\`\`\`arnm
receive {
    x => print(x);   // WRONG: catches everything
    1 => print(1);   // Never reached!
}

receive {
    1 => print(1);   // CORRECT: specific first
    x => print(x);   // General last
}
\`\`\`

---

## Practical Examples

### Message Router

\`\`\`arnm
actor Router {
    receive {
        1 => {
            // Route to handler A
            let a = spawn HandlerA();
            a ! 100;
        }
        2 => {
            // Route to handler B
            let b = spawn HandlerB();
            b ! 200;
        }
        msg => {
            // Default handler
            print(msg);
        }
    }
}
\`\`\`

### State Machine

\`\`\`arnm
actor StateMachine {
    let state: i32;
    
    fn init() {
        self.state = 0;  // Initial state
    }
    
    receive {
        1 => {
            // Event: Start
            if (self.state == 0) {
                self.state = 1;
                print(1);  // Transition to Running
            }
        }
        2 => {
            // Event: Stop
            if (self.state == 1) {
                self.state = 0;
                print(0);  // Transition to Stopped
            }
        }
        3 => {
            // Event: Query
            print(self.state);
        }
    }
}
\`\`\`

---

## Future Pattern Features

These features are planned but not yet implemented:

### Tuple Patterns (Future)
\`\`\`arnm
// Future syntax
receive {
    (x, y) => print(x + y);
}
\`\`\`

### Guard Clauses (Future)
\`\`\`arnm
// Future syntax
receive {
    x if x > 0 => print(1);   // Positive
    x if x < 0 => print(-1);  // Negative
    _ => print(0);            // Zero
}
\`\`\`

### Struct Patterns (Future)
\`\`\`arnm
// Future syntax
receive {
    Point { x, y } => print(x + y);
}
\`\`\`

---

## Grammar

\`\`\`
receive_block = "receive" "{" { receive_arm } "}" ;
receive_arm   = pattern "=>" block ;
pattern       = IDENT          // Variable binding
              | INT_LIT        // Literal match
              ;
\`\`\`
`
});
