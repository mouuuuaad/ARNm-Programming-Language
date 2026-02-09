const vscode = require('vscode');

const keywordItems = [
    'actor', 'fn', 'let', 'const', 'mut', 'if', 'else', 'while', 'for', 'loop',
    'break', 'continue', 'return', 'spawn', 'receive', 'self', 'true', 'false', 'nil'
];

const typeItems = ['i8', 'i16', 'i32', 'i64', 'u8', 'u16', 'u32', 'u64', 'f32', 'f64', 'bool', 'String', 'Process'];

const builtinItems = ['print'];

const snippetItems = [
    { label: 'fn', snippet: 'fn ${1:name}(${2:params}) {\n\t$0\n}', detail: 'Function' },
    { label: 'main', snippet: 'fn main() {\n\t$0\n}', detail: 'Main entry' },
    { label: 'actor', snippet: 'actor ${1:Name} {\n\tfn init() {\n\t\t$0\n\t}\n}', detail: 'Actor' },
    { label: 'struct', snippet: 'struct ${1:Name} {\n\t${2:field}: ${3:i32}\n}', detail: 'Struct' },
    { label: 'receive', snippet: 'receive {\n\t${1:msg} => {\n\t\t$0\n\t}\n}', detail: 'Receive block' }
];

function activate(context) {
    const provider = vscode.languages.registerCompletionItemProvider('arnm', {
        provideCompletionItems() {
            const items = [];

            keywordItems.forEach((kw) => {
                const item = new vscode.CompletionItem(kw, vscode.CompletionItemKind.Keyword);
                item.detail = 'keyword';
                items.push(item);
            });

            typeItems.forEach((type) => {
                const item = new vscode.CompletionItem(type, vscode.CompletionItemKind.TypeParameter);
                item.detail = 'type';
                items.push(item);
            });

            builtinItems.forEach((name) => {
                const item = new vscode.CompletionItem(name, vscode.CompletionItemKind.Function);
                item.detail = 'builtin';
                items.push(item);
            });

            snippetItems.forEach((snippet) => {
                const item = new vscode.CompletionItem(snippet.label, vscode.CompletionItemKind.Snippet);
                item.detail = snippet.detail;
                item.insertText = new vscode.SnippetString(snippet.snippet);
                items.push(item);
            });

            return items;
        }
    });

    context.subscriptions.push(provider);
}

function deactivate() {}

module.exports = {
    activate,
    deactivate
};
