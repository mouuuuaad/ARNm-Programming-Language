/*
 * ARNm Full Interpreter - Complete ARNm Language Support for Browser Playground
 * Supports: functions, structs, actors, all control flow, type annotations
 */

class ArnmInterpreter {
    constructor() {
        this.functions = {};      // User-defined functions
        this.structs = {};        // Struct definitions
        this.actors = {};         // Actor definitions
        this.globals = {};        // Global variables
        this.output = [];         // Output logs
        this.actorCounter = 0;    // PID counter
        this.currentActor = null; // Current actor context
        this.breakFlag = false;   // Break signal
        this.continueFlag = false; // Continue signal
        this.returnValue = undefined; // Return value
        this.returnFlag = false;  // Return signal
    }

    // Main entry point
    run(code) {
        this.output = [];
        this.functions = {};
        this.structs = {};
        this.actors = {};
        this.globals = {};
        this.actorCounter = 0;
        this.breakFlag = false;
        this.continueFlag = false;
        this.returnFlag = false;
        this.returnValue = undefined;

        try {
            // Remove comments
            code = this.removeComments(code);

            // Parse top-level declarations
            this.parseDeclarations(code);

            // Check for main function
            if (!this.functions['main']) {
                throw new Error("Missing 'fn main()'. Entry point required.");
            }

            // Execute main
            this.callFunction('main', []);

            if (this.output.length === 0) {
                this.output.push("Program executed successfully (no output).");
            }

            return { logs: this.output };
        } catch (e) {
            return { error: e.message };
        }
    }

    removeComments(code) {
        // Remove // comments
        code = code.replace(/\/\/[^\n]*/g, '');
        // Remove /* */ comments
        code = code.replace(/\/\*[\s\S]*?\*\//g, '');
        return code;
    }

    parseDeclarations(code) {
        let i = 0;
        while (i < code.length) {
            // Skip whitespace
            while (i < code.length && /\s/.test(code[i])) i++;
            if (i >= code.length) break;

            // Check for keywords
            if (code.slice(i).startsWith('fn ')) {
                i = this.parseFunction(code, i);
            } else if (code.slice(i).startsWith('struct ')) {
                i = this.parseStruct(code, i);
            } else if (code.slice(i).startsWith('actor ')) {
                i = this.parseActor(code, i);
            } else {
                i++;
            }
        }
    }

    parseFunction(code, start) {
        // fn name(params) -> ReturnType { body }
        let i = start + 3; // skip 'fn '

        // Skip whitespace
        while (i < code.length && /\s/.test(code[i])) i++;

        // Get function name
        let name = '';
        while (i < code.length && /[a-zA-Z_0-9]/.test(code[i])) {
            name += code[i++];
        }

        // Skip to (
        while (i < code.length && code[i] !== '(') i++;
        i++; // skip (

        // Parse parameters
        let params = [];
        let paramStr = '';
        let parenCount = 1;
        while (i < code.length && parenCount > 0) {
            if (code[i] === '(') parenCount++;
            if (code[i] === ')') parenCount--;
            if (parenCount > 0) paramStr += code[i];
            i++;
        }

        // Parse param names (ignore types for mock)
        if (paramStr.trim()) {
            paramStr.split(',').forEach(p => {
                const parts = p.trim().split(':');
                const pName = parts[0].replace('mut', '').trim();
                if (pName) params.push(pName);
            });
        }

        // Skip to {
        while (i < code.length && code[i] !== '{') i++;

        // Extract body
        let braceCount = 0;
        let bodyStart = i;
        do {
            if (code[i] === '{') braceCount++;
            if (code[i] === '}') braceCount--;
            i++;
        } while (i < code.length && braceCount > 0);

        const body = code.slice(bodyStart + 1, i - 1);

        this.functions[name] = { params, body };
        return i;
    }

    parseStruct(code, start) {
        // struct Name { field: Type, ... }
        let i = start + 7; // skip 'struct '

        while (i < code.length && /\s/.test(code[i])) i++;

        let name = '';
        while (i < code.length && /[a-zA-Z_0-9]/.test(code[i])) {
            name += code[i++];
        }

        while (i < code.length && code[i] !== '{') i++;

        let braceCount = 0;
        let bodyStart = i;
        do {
            if (code[i] === '{') braceCount++;
            if (code[i] === '}') braceCount--;
            i++;
        } while (i < code.length && braceCount > 0);

        const body = code.slice(bodyStart + 1, i - 1);

        // Parse fields
        let fields = [];
        body.split(',').forEach(f => {
            const parts = f.trim().split(':');
            if (parts[0].trim()) {
                fields.push(parts[0].replace('mut', '').trim());
            }
        });

        this.structs[name] = { fields };
        this.output.push(`[Compiler] Defined struct '${name}'`);
        return i;
    }

    parseActor(code, start) {
        // actor Name { ... }
        let i = start + 6; // skip 'actor '

        while (i < code.length && /\s/.test(code[i])) i++;

        let name = '';
        while (i < code.length && /[a-zA-Z_0-9]/.test(code[i])) {
            name += code[i++];
        }

        while (i < code.length && code[i] !== '{') i++;

        let braceCount = 0;
        let bodyStart = i;
        do {
            if (code[i] === '{') braceCount++;
            if (code[i] === '}') braceCount--;
            i++;
        } while (i < code.length && braceCount > 0);

        const body = code.slice(bodyStart + 1, i - 1);
        this.actors[name] = { body };
        this.output.push(`[Compiler] Defined actor '${name}'`);
        return i;
    }

    callFunction(name, args) {
        const fn = this.functions[name];
        if (!fn) {
            // Built-in functions
            if (name === 'print' || name === 'println') {
                const val = args[0];
                this.output.push(val !== undefined ? String(val) : '');
                return;
            }
            throw new Error(`Unknown function: ${name}`);
        }

        // Create scope with parameters
        const scope = {};
        fn.params.forEach((p, idx) => {
            scope[p] = args[idx];
        });

        // Save caller state
        const savedState = {
            returnFlag: this.returnFlag,
            returnValue: this.returnValue,
            breakFlag: this.breakFlag,
            continueFlag: this.continueFlag
        };

        // Reset for callee
        this.returnFlag = false;
        this.returnValue = undefined;
        this.breakFlag = false;
        this.continueFlag = false;

        this.executeBlock(fn.body, scope);
        const result = this.returnValue;

        // Restore caller state
        this.returnFlag = savedState.returnFlag;
        this.returnValue = savedState.returnValue;
        this.breakFlag = savedState.breakFlag;
        this.continueFlag = savedState.continueFlag;

        return result;
    }

    executeBlock(code, scope) {
        let i = 0;
        while (i < code.length && !this.breakFlag && !this.continueFlag && !this.returnFlag) {
            // Skip whitespace
            while (i < code.length && /\s/.test(code[i])) i++;
            if (i >= code.length) break;

            // Read statement
            let stmt = '';
            let braceCount = 0;
            let parenCount = 0;

            while (i < code.length) {
                const c = code[i];
                if (c === '{') braceCount++;
                if (c === '}') braceCount--;
                if (c === '(') parenCount++;
                if (c === ')') parenCount--;
                stmt += c;
                i++;
                if (braceCount === 0 && parenCount === 0 && c === ';') break;
                if (braceCount === 0 && stmt.includes('{') && c === '}') {
                    // Check ahead for 'else'
                    let j = i;
                    while (j < code.length && /\s/.test(code[j])) j++;
                    if (code.slice(j).startsWith('else')) {
                        stmt += code.slice(i, j + 4);
                        i = j + 4;
                        continue;
                    }
                    break;
                }
            }

            stmt = stmt.trim();
            if (!stmt || stmt === '}') continue;

            this.executeStatement(stmt, scope);
        }
    }

    executeStatement(stmt, scope) {
        if (this.breakFlag || this.continueFlag || this.returnFlag) return;

        // let/const [mut] var [: Type] = expr;
        let match = stmt.match(/^(let|const)\s+(mut\s+)?([a-zA-Z_]\w*)\s*(?::\s*[a-zA-Z_]\w*)?\s*=\s*([\s\S]+);$/);
        if (match) {
            const kind = match[1];
            const isMut = !!match[2];
            const name = match[3];
            const expr = match[4];

            if (kind === 'const' && isMut) {
                throw new Error("const bindings cannot be mutable");
            }

            scope[name] = this.evaluate(expr, scope);
            return;
        }

        // short declaration: name := expr;
        match = stmt.match(/^([a-zA-Z_]\w*)\s*:=\s*([\s\S]+);$/);
        if (match) {
            scope[match[1]] = this.evaluate(match[2], scope);
            return;
        }

        // return [expr];
        match = stmt.match(/^return\s*([\s\S]+)?;$/);
        if (match) {
            this.returnValue = match[1] ? this.evaluate(match[1], scope) : undefined;
            this.returnFlag = true;
            return;
        }

        // break;
        if (stmt === 'break;') {
            this.breakFlag = true;
            return;
        }

        // continue;
        if (stmt === 'continue;') {
            this.continueFlag = true;
            return;
        }

        // while (cond) { body }
        match = stmt.match(/^while\s*\(([\s\S]+?)\)\s*\{([\s\S]*)\}$/);
        if (match) {
            let iterations = 0;
            while (this.evaluate(match[1], scope) && iterations < 10000) {
                this.breakFlag = false;
                this.continueFlag = false;
                this.executeBlock(match[2], scope);
                if (this.breakFlag) { this.breakFlag = false; break; }
                if (this.returnFlag) break;
                iterations++;
            }
            this.continueFlag = false;
            return;
        }

        // loop { body }
        match = stmt.match(/^loop\s*\{([\s\S]*)\}$/);
        if (match) {
            let iterations = 0;
            while (iterations < 10000) {
                this.breakFlag = false;
                this.continueFlag = false;
                this.executeBlock(match[1], scope);
                if (this.breakFlag) { this.breakFlag = false; break; }
                if (this.returnFlag) break;
                iterations++;
            }
            this.continueFlag = false;
            return;
        }

        // for var in start..end { body }
        match = stmt.match(/^for\s+([a-zA-Z_]\w*)\s+in\s+(\d+)\.\.(\d+)\s*\{([\s\S]*)\}$/);
        if (match) {
            const varName = match[1];
            const start = parseInt(match[2]);
            const end = parseInt(match[3]);
            for (let j = start; j < end && !this.returnFlag; j++) {
                scope[varName] = j;
                this.breakFlag = false;
                this.continueFlag = false;
                this.executeBlock(match[4], scope);
                if (this.breakFlag) { this.breakFlag = false; break; }
            }
            this.continueFlag = false;
            return;
        }

        // if (cond) { body } [else if (cond) { body }]* [else { body }]
        match = stmt.match(/^if\s*\(([\s\S]+?)\)\s*\{([\s\S]*?)\}([\s\S]*)$/);
        if (match) {
            const cond = match[1];
            const thenBody = match[2];
            let rest = match[3].trim();

            if (this.evaluate(cond, scope)) {
                this.executeBlock(thenBody, scope);
            } else if (rest.startsWith('else')) {
                rest = rest.slice(4).trim();
                if (rest.startsWith('if')) {
                    this.executeStatement(rest, scope);
                } else if (rest.startsWith('{')) {
                    const elseMatch = rest.match(/^\{([\s\S]*)\}$/);
                    if (elseMatch) {
                        this.executeBlock(elseMatch[1], scope);
                    }
                }
            }
            return;
        }

        // receive { pattern => body, ... }
        match = stmt.match(/^receive\s*\{([\s\S]*)\}$/);
        if (match) {
            this.output.push("[Actor] Entered receive block (simulated)");
            return;
        }

        // var = expr; (assignment, but not == comparison)
        match = stmt.match(/^([a-zA-Z_]\w*)\s*=\s*(.+);$/);
        if (match && !stmt.includes('==')) {
            scope[match[1]] = this.evaluate(match[2], scope);
            return;
        }

        // obj.field = expr; (struct field assignment)
        match = stmt.match(/^([a-zA-Z_]\w*)\.([a-zA-Z_]\w*)\s*=\s*(.+);$/);
        if (match) {
            const obj = scope[match[1]];
            if (obj && typeof obj === 'object') {
                obj[match[2]] = this.evaluate(match[3], scope);
            }
            return;
        }

        // Expression statement (function call, etc.)
        if (stmt.endsWith(';')) {
            this.evaluate(stmt.slice(0, -1), scope);
            return;
        }
    }

    evaluate(expr, scope) {
        expr = expr.trim();

        // String literal
        if (expr.startsWith('"') && expr.endsWith('"')) {
            return expr.slice(1, -1);
        }

        // Number literal
        if (/^-?\d+(\.\d+)?$/.test(expr)) {
            return parseFloat(expr);
        }

        // Boolean literals
        if (expr === 'true') return true;
        if (expr === 'false') return false;

        // self
        if (expr === 'self') {
            return this.currentActor || '<self>';
        }

        // Helper to safely split by comma ignoring nested structures and strings
        const splitByComma = (str) => {
            let parts = [];
            let depth = 0;
            let inString = false;
            let current = '';

            for (let i = 0; i < str.length; i++) {
                const c = str[i];

                if (c === '"' && (i === 0 || str[i - 1] !== '\\')) {
                    inString = !inString;
                }

                if (!inString) {
                    if (c === '(' || c === '[' || c === '{') depth++;
                    if (c === ')' || c === ']' || c === '}') depth--;
                }

                if (c === ',' && depth === 0 && !inString) {
                    parts.push(current.trim());
                    current = '';
                } else {
                    current += c;
                }
            }
            if (current.trim()) parts.push(current.trim());
            return parts;
        };

        // Array literal [a, b, c]
        let match = expr.match(/^\[(.*)\]$/);
        if (match) {
            const inner = match[1];
            if (!inner.trim()) return [];
            return splitByComma(inner).map(e => this.evaluate(e, scope));
        }

        // Struct instantiation
        match = expr.match(/^([A-Z][a-zA-Z_0-9]*)\s*\{(.*)\}$/);
        if (match) {
            const obj = { __type: match[1] };
            if (match[2].trim()) {
                splitByComma(match[2]).forEach(pair => {
                    const [key, val] = pair.split(':').map(s => s.trim());
                    obj[key] = this.evaluate(val, scope);
                });
            }
            return obj;
        }

        // spawn(ActorName, args...)
        match = expr.match(/^spawn\s*\((.+)\)$/);
        if (match) {
            const pid = `<0.${++this.actorCounter}.0>`;
            this.output.push(`[System] Spawned actor ${pid}`);
            return pid;
        }

        // send(target, message)
        match = expr.match(/^send\s*\((.+?),\s*(.+)\)$/);
        if (match) {
            const target = this.evaluate(match[1], scope);
            const msg = this.evaluate(match[2], scope);
            this.output.push(`[System] Message sent to ${target}`);
            return undefined;
        }

        // Function call: name(args...)
        match = expr.match(/^([a-zA-Z_]\w*)\s*\((.*)\)$/);
        if (match) {
            const fnName = match[1];
            const argsStr = match[2];
            let args = [];
            if (argsStr.trim()) {
                args = splitByComma(argsStr).map(arg => this.evaluate(arg, scope));
            }
            return this.callFunction(fnName, args);
        }

        // Array index: arr[i]
        match = expr.match(/^([a-zA-Z_]\w*)\[(.+)\]$/);
        if (match) {
            const arr = scope[match[1]];
            const idx = this.evaluate(match[2], scope);
            return arr ? arr[idx] : undefined;
        }

        // Field access: obj.field
        match = expr.match(/^([a-zA-Z_]\w*)\.([a-zA-Z_]\w*)$/);
        if (match) {
            const obj = scope[match[1]];
            return obj ? obj[match[2]] : undefined;
        }

        // Variable
        if (/^[a-zA-Z_]\w*$/.test(expr)) {
            if (expr in scope) return scope[expr];
            if (expr in this.globals) return this.globals[expr];
            return undefined;
        }

        // Helper to replace function calls in the expression
        const replaceFunctionCalls = (expression) => {
            let result = '';
            let i = 0;
            while (i < expression.length) {
                // Check for start of string to skip
                if (expression[i] === '"') {
                    result += '"';
                    i++;
                    while (i < expression.length && (expression[i] !== '"' || expression[i - 1] === '\\')) {
                        result += expression[i];
                        i++;
                    }
                    if (i < expression.length) {
                        result += '"';
                        i++;
                    }
                    continue;
                }

                // Check for identifier start
                if (/[a-zA-Z_]/.test(expression[i])) {
                    let start = i;
                    while (i < expression.length && /[a-zA-Z_0-9]/.test(expression[i])) i++;
                    const name = expression.slice(start, i);

                    // Check if it's a function call
                    if (i < expression.length && expression[i] === '(' && this.functions[name]) {
                        i++; // skip (
                        let parenCount = 1;
                        let argStart = i;
                        while (i < expression.length && parenCount > 0) {
                            if (expression[i] === '(') parenCount++;
                            if (expression[i] === ')') parenCount--;
                            if (parenCount > 0) i++;
                        }

                        const argsStr = expression.slice(argStart, i);
                        i++; // skip closing )

                        // Parse and evaluate args
                        let args = [];
                        if (argsStr.trim()) {
                            args = splitByComma(argsStr).map(arg => this.evaluate(arg, scope));
                        }

                        const callResult = this.callFunction(name, args);
                        // Recursively handle the result (in case it returns something complex) but strictly it's a value now.
                        // We must stringify it for the JS expression
                        if (typeof callResult === 'string') result += `"${callResult}"`;
                        else if (typeof callResult === 'object') result += `(${JSON.stringify(callResult)})`;
                        else result += String(callResult);

                        continue;
                    } else {
                        result += name;
                        // Not a known function call, just an identifier (variable or unknown)
                        continue;
                    }
                }

                result += expression[i];
                i++;
            }
            return result;
        };

        // Pre-process expression to resolve function calls
        expr = replaceFunctionCalls(expr);

        // Binary expression - convert to JS and eval
        let jsExpr = expr
            .replace(/\band\b/g, '&&')
            .replace(/\bor\b/g, '||')
            .replace(/\bnot\b/g, '!');

        // Replace variables (longest first to avoid partial replacements)
        const varNames = Object.keys({ ...scope, ...this.globals }).sort((a, b) => b.length - a.length);
        for (const v of varNames) {
            const val = scope[v] ?? this.globals[v];
            const regex = new RegExp(`\\b${v}\\b`, 'g');
            if (typeof val === 'string') {
                jsExpr = jsExpr.replace(regex, `"${val}"`);
            } else if (typeof val === 'object') {
                jsExpr = jsExpr.replace(regex, `(${JSON.stringify(val)})`);
            } else {
                jsExpr = jsExpr.replace(regex, String(val));
            }
        }

        try {
            return Function('"use strict";return (' + jsExpr + ')')();
        } catch (e) {
            throw new Error(`Cannot evaluate: ${expr}`);
        }
    }
}

window.ArnmLocalInterpreter = new ArnmInterpreter();
window.runArnmLocal = function (code) {
    return window.ArnmLocalInterpreter.run(code);
};
