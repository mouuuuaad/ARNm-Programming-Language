
window.addDocSection({
    id: 'playground',
    section: 'Tools',
    level: 1,
    title: 'Run and Test',
    content: `
# ARNm Playground

Write and test ARNm code directly in your browser.

---

<div class="playground-container">
    <div class="editor-pane">
        <div class="pane-header">
            <span>main.arnm</span>
            <button id="run-btn" class="run-button">
                Run Code <!-- Play icon will be added via CSS -->
            </button>
        </div>
        <div class="editor-shell">
            <div class="editor-gutter" id="editor-gutter" aria-hidden="true"></div>
            <div class="editor-body">
                <textarea id="code-editor" spellcheck="false" aria-label="ARNm code editor">fn main() {
    const greeting = "Hello, ARNm!";
    total := 10 + 20;
    print(greeting);
    print(total);
}</textarea>
                <div class="editor-autocomplete" id="editor-autocomplete" role="listbox" aria-hidden="true"></div>
            </div>
        </div>
    </div>
    
    <div class="output-pane">
        <div class="pane-header">
            <span>Output</span>
            <button id="clear-btn" class="clear-button">Clear</button>
        </div>
        <div id="console-output" class="console">
> Ready to compile...
        </div>
    </div>
</div>

<div class="playground-hint">Tip: Press Ctrl+Space for autocomplete.</div>
`
});
