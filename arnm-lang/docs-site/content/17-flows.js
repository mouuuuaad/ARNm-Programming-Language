
window.addDocSection({
    id: 'flows',
    section: 'Learning',
    level: 1,
    title: 'Learning by Flows',
    content: `
# Learning by Flows

Visualize how ARNm's core concepts work through interactive diagrams.

## Actor Lifecycle
Understanding how actors are spawned, communicate, and terminate.

<div class="flow-diagram">
    <pre class="mermaid">
    sequenceDiagram
        participant Main
        participant Actor
        
        Note over Main: Spawning Phase
        Main->>Actor: spawn(func)
        activate Actor
        Actor-->>Main: PID (Process ID)
        
        Note over Main, Actor: Message Passing
        Main->>Actor: send(PID, "Hello")
        Actor->>Actor: receive() match "Hello"
        
        Note over Actor: Processing
        Actor->>Main: send(MainPID, "Ack")
        
        Note over Actor: Termination
        deactivate Actor
    </pre>
</div>

## Memory Model
How immutability and message passing prevent data races.

<div class="flow-diagram">
    <pre class="mermaid">
    graph TD
        A[Actor A Heap] -- Copy Message --> M[Mailbox]
        M -- Copy Message --> B[Actor B Heap]
        
        subgraph "Actor A"
        A
        end
        
        subgraph "Actor B"
        M
        B
        end
        
        style A fill:#ffcdd2,stroke:#d32f2f,color:#b71c1c
        style B fill:#e1f5fe,stroke:#0288d1,color:#01579b
        style M fill:#fff9c4,stroke:#fbc02d,color:#f57f17
    </pre>
</div>
`
});
