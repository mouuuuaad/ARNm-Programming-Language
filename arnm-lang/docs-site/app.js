// ARNm Documentation - Application Logic

(function () {
    'use strict';

    // DOM Elements
    const tocElement = document.getElementById('toc');
    const docContent = document.getElementById('doc-content');
    const searchInput = document.getElementById('search');
    const currentSectionSpan = document.getElementById('current-section');
    let lenisInstance = null;
    let revealObserver = null;
    let motionRoot = null;

    // Simple Markdown Parser
    function parseMarkdown(md) {
        let html = md;

        // Code blocks (must be first)
        html = html.replace(/```(\w*)\n([\s\S]*?)```/g, function (match, lang, code) {
            const escaped = escapeHtml(code.trim());
            return '<pre><code class="language-' + lang + '">' + escaped + '</code></pre>';
        });

        // Headers
        html = html.replace(/^### (.+)$/gm, '<h3 id="$1">$1</h3>');
        html = html.replace(/^## (.+)$/gm, '<h2 id="$1">$1</h2>');
        html = html.replace(/^# (.+)$/gm, '<h1 id="$1">$1</h1>');

        // Tables
        html = parseTable(html);

        // Inline code
        html = html.replace(/`([^`]+)`/g, '<code>$1</code>');

        // Bold
        html = html.replace(/\*\*([^*]+)\*\*/g, '<strong>$1</strong>');

        // Italic
        html = html.replace(/\*([^*]+)\*/g, '<em>$1</em>');

        // Links
        html = html.replace(/\[([^\]]+)\]\(([^)]+)\)/g, '<a href="$2">$1</a>');

        // Blockquotes
        html = html.replace(/^> (.+)$/gm, '<blockquote>$1</blockquote>');

        // Unordered lists
        html = html.replace(/^- (.+)$/gm, '<li>$1</li>');
        html = html.replace(/(<li>.*<\/li>\n?)+/g, '<ul>$&</ul>');

        // Ordered lists
        html = html.replace(/^\d+\. (.+)$/gm, '<li>$1</li>');

        // Paragraphs
        html = html.replace(/\n\n([^<])/g, '</p><p>$1');

        // Wrap in paragraph if starts without tag
        if (!html.trim().startsWith('<')) {
            html = '<p>' + html + '</p>';
        }

        // Clean up empty paragraphs
        html = html.replace(/<p>\s*<\/p>/g, '');
        html = html.replace(/<p>\s*(<h[123])/g, '$1');
        html = html.replace(/(<\/h[123]>)\s*<\/p>/g, '$1');

        return html;
    }

    function parseTable(html) {
        const tableRegex = /\|(.+)\|\n\|[-| :]+\|\n((?:\|.+\|\n?)+)/g;

        return html.replace(tableRegex, function (match, headerRow, bodyRows) {
            const headers = headerRow.split('|').map(h => h.trim()).filter(h => h);
            const rows = bodyRows.trim().split('\n').map(row => {
                return row.split('|').map(c => c.trim()).filter(c => c);
            });

            let table = '<table><thead><tr>';
            headers.forEach(h => {
                table += '<th>' + h + '</th>';
            });
            table += '</tr></thead><tbody>';

            rows.forEach(row => {
                table += '<tr>';
                row.forEach(cell => {
                    table += '<td>' + cell + '</td>';
                });
                table += '</tr>';
            });

            table += '</tbody></table>';
            return table;
        });
    }

    function escapeHtml(text) {
        const div = document.createElement('div');
        div.textContent = text;
        return div.innerHTML;
    }

    function prefersReducedMotion() {
        return window.matchMedia && window.matchMedia('(prefers-reduced-motion: reduce)').matches;
    }

    function setupLenis() {
        if (lenisInstance || !window.Lenis || prefersReducedMotion()) return;
        lenisInstance = new window.Lenis({
            duration: 1.05,
            smoothWheel: true,
            smoothTouch: false,
            easing: function (t) {
                return 1 - Math.pow(1 - t, 3);
            }
        });

        function raf(time) {
            lenisInstance.raf(time);
            requestAnimationFrame(raf);
        }
        requestAnimationFrame(raf);
    }

    function smoothScrollTo(target, options) {
        if (lenisInstance) {
            lenisInstance.scrollTo(target, Object.assign({ offset: -80, duration: 1.1 }, options || {}));
            return;
        }
        if (target && target.scrollIntoView) {
            target.scrollIntoView({ behavior: 'smooth', block: 'start' });
        }
    }

    function scrollToTop() {
        if (lenisInstance) {
            lenisInstance.scrollTo(0, { immediate: true });
        } else {
            window.scrollTo(0, 0);
        }
    }

    function setupReveals() {
        if (!docContent) return;
        if (revealObserver) revealObserver.disconnect();

        const targets = docContent.querySelectorAll(
            'h1, h2, h3, p, pre, table, blockquote, ul, ol, .playground-container, .flow-diagram'
        );

        targets.forEach(function (el) {
            el.classList.add('reveal');
        });

        if (prefersReducedMotion()) {
            targets.forEach(function (el) {
                el.classList.add('is-visible');
            });
            return;
        }

        revealObserver = new IntersectionObserver(function (entries) {
            entries.forEach(function (entry) {
                if (entry.isIntersecting) {
                    entry.target.classList.add('is-visible');
                    revealObserver.unobserve(entry.target);
                }
            });
        }, { threshold: 0.15, rootMargin: '0px 0px -10% 0px' });

        targets.forEach(function (el) {
            revealObserver.observe(el);
        });
    }

    function setupMotionBackdrop() {
        if (motionRoot || prefersReducedMotion()) return;
        const rootEl = document.getElementById('motion-root');
        if (!rootEl || !window.React || !window.ReactDOM || !window.Motion) return;

        const React = window.React;
        const ReactDOM = window.ReactDOM;
        const Motion = window.Motion;
        const MotionDiv = Motion.motion.div;

        const MotionLayer = function () {
            return React.createElement(
                'div',
                { className: 'motion-layer' },
                React.createElement(MotionDiv, {
                    className: 'motion-orb orb-1',
                    initial: { opacity: 0, scale: 0.9, x: -40, y: -30 },
                    animate: { opacity: [0.4, 0.7, 0.4], scale: [0.9, 1.05, 0.95], x: [-40, 20, -30], y: [-30, 20, -40] },
                    transition: { duration: 18, repeat: Infinity, ease: 'easeInOut' }
                }),
                React.createElement(MotionDiv, {
                    className: 'motion-orb orb-2',
                    initial: { opacity: 0, scale: 0.9, x: 30, y: 0 },
                    animate: { opacity: [0.35, 0.6, 0.35], scale: [0.95, 1.1, 0.95], x: [30, -20, 15], y: [0, 30, -10] },
                    transition: { duration: 22, repeat: Infinity, ease: 'easeInOut' }
                }),
                React.createElement(MotionDiv, {
                    className: 'motion-orb orb-3',
                    initial: { opacity: 0, scale: 0.9, y: 20 },
                    animate: { opacity: [0.25, 0.45, 0.25], scale: [1, 1.05, 1], y: [20, -10, 20] },
                    transition: { duration: 26, repeat: Infinity, ease: 'easeInOut' }
                })
            );
        };

        if (ReactDOM.createRoot) {
            motionRoot = ReactDOM.createRoot(rootEl);
            motionRoot.render(React.createElement(MotionLayer));
        } else {
            ReactDOM.render(React.createElement(MotionLayer), rootEl);
            motionRoot = { render: function () { } };
        }
    }

    // Build Table of Contents
    function buildTOC() {
        if (!window.documentationContent) return;

        let tocHTML = '';
        let currentSection = '';

        window.documentationContent.forEach(function (item) {
            if (item.section && item.section !== currentSection) {
                if (currentSection) {
                    tocHTML += '</ul></li>';
                }
                currentSection = item.section;
                tocHTML += '<li class="toc-section"><div class="toc-section-title">' + item.section + '</div><ul>';
            }

            const isSubItem = item.level && item.level > 1;
            const itemClass = 'toc-item' + (isSubItem ? ' sub-item' : '');

            tocHTML += '<li class="' + itemClass + '" data-id="' + item.id + '">';
            tocHTML += '<a href="#' + item.id + '">' + item.title + '</a></li>';
        });

        if (currentSection) {
            tocHTML += '</ul></li>';
        }

        tocElement.innerHTML = tocHTML;

        // Add click handlers
        tocElement.querySelectorAll('.toc-item a').forEach(function (link) {
            link.addEventListener('click', function (e) {
                e.preventDefault();
                const id = this.getAttribute('href').substring(1);
                navigateTo(id);
            });
        });
    }

    // Setup Playground
    function setupPlayground() {
        const runBtn = document.getElementById('run-btn');
        const clearBtn = document.getElementById('clear-btn');
        const output = document.getElementById('console-output');
        const editor = document.getElementById('code-editor');

        // Simple ARNm Interpreter with while loop support
        function runInterpreter(code) {
            let logs = [];
            let variables = {};

            // Parse the code into tokens/lines more carefully
            const lines = code.split('\n');

            // Validate Structure: ARNm must have a main function
            const hasMain = lines.some(l => l.includes('fn main()'));
            if (!hasMain) {
                return { error: "Missing 'fn main()'. Entry point required." };
            }

            // Extract main function body
            let inMain = false;
            let braceCount = 0;
            let mainBody = [];

            for (let line of lines) {
                const trimmed = line.trim();
                if (trimmed.includes('fn main()')) {
                    inMain = true;
                    const braceIdx = trimmed.indexOf('{');
                    if (braceIdx !== -1) {
                        braceCount = 1;
                        const rest = trimmed.substring(braceIdx + 1).trim();
                        if (rest && rest !== '}') mainBody.push(rest);
                    }
                    continue;
                }

                if (inMain) {
                    for (let char of trimmed) {
                        if (char === '{') braceCount++;
                        if (char === '}') braceCount--;
                    }
                    if (braceCount > 0) {
                        mainBody.push(trimmed);
                    } else if (braceCount === 0) {
                        const closingIdx = trimmed.lastIndexOf('}');
                        if (closingIdx > 0) {
                            mainBody.push(trimmed.substring(0, closingIdx).trim());
                        }
                        break;
                    }
                }
            }

            // Execute main body
            try {
                executeBlock(mainBody.join('\n'), variables, logs, 0);
            } catch (e) {
                return { error: e.message };
            }

            if (logs.length === 0 && Object.keys(variables).length === 0) {
                return { logs: ["Program executed successfully (no output)."] };
            }

            return { logs: logs };
        }

        // Execute a block of code
        function executeBlock(code, vars, logs, depth) {
            if (depth > 100) throw new Error("Max recursion depth exceeded");

            let i = 0;
            const chars = code;

            while (i < chars.length) {
                // Skip whitespace
                while (i < chars.length && /\s/.test(chars[i])) i++;
                if (i >= chars.length) break;

                // Read a statement until semicolon or block
                let stmt = '';
                let braceCount = 0;
                let parenCount = 0;

                while (i < chars.length) {
                    const c = chars[i];
                    if (c === '{') braceCount++;
                    if (c === '}') braceCount--;
                    if (c === '(') parenCount++;
                    if (c === ')') parenCount--;

                    stmt += c;
                    i++;

                    // End of statement
                    if (braceCount === 0 && parenCount === 0 && c === ';') break;
                    // End of block statement (while, if, etc.)
                    if (braceCount === 0 && stmt.includes('{') && c === '}') break;
                }

                stmt = stmt.trim();
                if (!stmt || stmt === '}') continue;

                // Parse and execute statement
                executeStatement(stmt, vars, logs, depth);
            }
        }

        function executeStatement(stmt, vars, logs, depth) {
            // Skip comments
            if (stmt.startsWith('//')) return;

            // let/const [mut] var [: Type] = expr;
            const letMatch = stmt.match(/^(let|const)\s+(mut\s+)?([a-zA-Z_]\w*)\s*(?::\s*[a-zA-Z_]\w*)?\s*=\s*([\s\S]+);$/);
            if (letMatch) {
                const kind = letMatch[1];
                const isMut = !!letMatch[2];
                const name = letMatch[3];
                const expr = letMatch[4];

                if (kind === 'const' && isMut) {
                    throw new Error("const bindings cannot be mutable");
                }

                if (expr.includes('spawn(')) {
                    vars[name] = `<0.${Math.floor(Math.random() * 100) + 10}.0>`;
                    logs.push(`[System] Spawned new actor ${vars[name]}`);
                } else {
                    vars[name] = evalExpr(expr, vars);
                }
                return;
            }

            // short declaration: name := expr;
            const shortDecl = stmt.match(/^([a-zA-Z_]\w*)\s*:=\s*([\s\S]+);$/);
            if (shortDecl) {
                const name = shortDecl[1];
                const expr = shortDecl[2];
                vars[name] = evalExpr(expr, vars);
                return;
            }

            // var = expr; (reassignment)
            const assignMatch = stmt.match(/^([a-zA-Z_]\w*)\s*=\s*(.+);$/);
            if (assignMatch) {
                const name = assignMatch[1];
                const expr = assignMatch[2];
                vars[name] = evalExpr(expr, vars);
                return;
            }

            // print(...);
            const printMatch = stmt.match(/^print\((.+)\);$/);
            if (printMatch) {
                const content = printMatch[1].trim();
                if (content.startsWith('"') && content.endsWith('"')) {
                    logs.push(content.slice(1, -1));
                } else {
                    logs.push(String(evalExpr(content, vars)));
                }
                return;
            }

            // while (condition) { body }
            const whileMatch = stmt.match(/^while\s*\((.+?)\)\s*\{([\s\S]*)\}$/);
            if (whileMatch) {
                const condition = whileMatch[1];
                const body = whileMatch[2];
                let iterations = 0;
                const maxIter = 1000;

                while (evalExpr(condition, vars) && iterations < maxIter) {
                    executeBlock(body, vars, logs, depth + 1);
                    iterations++;
                }

                if (iterations >= maxIter) {
                    logs.push("[Warning] Loop terminated after 1000 iterations");
                }
                return;
            }

            // if (condition) { body } [else { body }]
            const ifMatch = stmt.match(/^if\s*\((.+?)\)\s*\{([\s\S]*?)\}(?:\s*else\s*\{([\s\S]*)\})?$/);
            if (ifMatch) {
                const condition = ifMatch[1];
                const thenBody = ifMatch[2];
                const elseBody = ifMatch[3] || '';

                if (evalExpr(condition, vars)) {
                    executeBlock(thenBody, vars, logs, depth + 1);
                } else if (elseBody) {
                    executeBlock(elseBody, vars, logs, depth + 1);
                }
                return;
            }

            // spawn(...);
            if (stmt.match(/^spawn\(/)) {
                logs.push(`[System] Spawned actor <0.${Math.floor(Math.random() * 100) + 10}.0>`);
                return;
            }

            // send(...);
            if (stmt.match(/^send\(/)) {
                logs.push(`[System] Message sent`);
                return;
            }
        }

        function evalExpr(expr, vars) {
            // Replace ARNm operators with JS operators
            let jsExpr = expr
                .replace(/\s+and\s+/g, ' && ')
                .replace(/\s+or\s+/g, ' || ');

            // Replace variable names with their values
            for (const [key, val] of Object.entries(vars)) {
                const regex = new RegExp(`\\b${key}\\b`, 'g');
                jsExpr = jsExpr.replace(regex, typeof val === 'string' ? `"${val}"` : val);
            }

            try {
                return Function('"use strict";return (' + jsExpr + ')')();
            } catch (e) {
                throw new Error("Error evaluating: " + expr);
            }
        }

        if (runBtn) {
            runBtn.addEventListener('click', async function () {
                const code = editor.value;

                // Check if we're in local file mode (no server)
                const isLocalFile = window.location.protocol === 'file:';

                // Check if WASM can load
                let useWasm = !isLocalFile && window.ArnmWasm && window.ArnmWasm.canLoad && window.ArnmWasm.canLoad();

                if (useWasm) {
                    output.innerHTML += '<div class="log-info">> Compiling with ARNm WASM...</div>';
                    try {
                        const result = await window.ArnmWasm.run(code);

                        // null result means WASM unavailable, fallback to mock
                        if (result === null) {
                            useWasm = false;
                            output.innerHTML += '<div class="log-warning">WASM unavailable, using mock interpreter...</div>';
                        } else if (result.success) {
                            output.innerHTML += '<div class="log-success">Compilation successful.</div>';
                            output.innerHTML += `<div class="log-output">${result.output.replace(/\n/g, '<br>')}</div>`;
                            output.scrollTop = output.scrollHeight;
                            return;
                        } else {
                            output.innerHTML += `<div class="log-error">${result.output.replace(/\n/g, '<br>')}</div>`;
                            output.innerHTML += '<div class="log-warning">WASM compile failed, falling back to local interpreter...</div>';
                            output.scrollTop = output.scrollHeight;
                            useWasm = false;
                        }
                    } catch (e) {
                        console.warn('WASM execution failed, falling back to mock:', e);
                        useWasm = false;
                        output.innerHTML += '<div class="log-warning">WASM error, using mock interpreter...</div>';
                    }
                } else {
                    // Local file mode - use mock interpreter directly
                    output.innerHTML += '<div class="log-info">> Running in local mode...</div>';
                }

                // Fallback: full local interpreter
                setTimeout(function () {
                    // Use the full ARNm interpreter if available, else fallback to simple
                    const result = window.runArnmLocal ? window.runArnmLocal(code) : runInterpreter(code);

                    if (result.error) {
                        output.innerHTML += `<div class="log-error">Runtime Error: ${result.error}</div>`;
                    } else {
                        output.innerHTML += '<div class="log-success">Compilation successful.</div>';
                        result.logs.forEach(log => {
                            output.innerHTML += `<div class="log-output">${log}</div>`;
                        });
                    }
                    output.scrollTop = output.scrollHeight;
                }, 200);
            });
        }

        if (clearBtn) {
            clearBtn.addEventListener('click', function () {
                output.innerHTML = '> Ready to compile...';
            });
        }

        // Editor UX: line numbers, auto-indent, autocomplete
        if (editor) {
            const gutter = document.getElementById('editor-gutter');
            const autocomplete = document.getElementById('editor-autocomplete');

            const completions = [
                { label: 'fn', kind: 'snippet', insertText: 'fn name() {\\n    \\n}', cursorOffset: -2 },
                { label: 'actor', kind: 'snippet', insertText: 'actor Name {\\n    fn init() {\\n        \\n    }\\n}', cursorOffset: -6 },
                { label: 'struct', kind: 'snippet', insertText: 'struct Name {\\n    field: i32\\n}', cursorOffset: -6 },
                { label: 'receive', kind: 'snippet', insertText: 'receive {\\n    msg => {\\n        \\n    }\\n}', cursorOffset: -6 },
                { label: 'let', kind: 'keyword', insertText: 'let ' },
                { label: 'const', kind: 'keyword', insertText: 'const ' },
                { label: 'mut', kind: 'keyword', insertText: 'mut ' },
                { label: 'if', kind: 'keyword', insertText: 'if ' },
                { label: 'else', kind: 'keyword', insertText: 'else ' },
                { label: 'while', kind: 'keyword', insertText: 'while ' },
                { label: 'for', kind: 'keyword', insertText: 'for ' },
                { label: 'loop', kind: 'keyword', insertText: 'loop ' },
                { label: 'break', kind: 'keyword', insertText: 'break;' },
                { label: 'continue', kind: 'keyword', insertText: 'continue;' },
                { label: 'return', kind: 'keyword', insertText: 'return ' },
                { label: 'spawn', kind: 'keyword', insertText: 'spawn ' },
                { label: 'print', kind: 'builtin', insertText: 'print(' },
                { label: 'self', kind: 'keyword', insertText: 'self' },
                { label: 'true', kind: 'literal', insertText: 'true' },
                { label: 'false', kind: 'literal', insertText: 'false' },
                { label: 'nil', kind: 'literal', insertText: 'nil' },
                { label: 'i32', kind: 'type', insertText: 'i32' },
                { label: 'i64', kind: 'type', insertText: 'i64' },
                { label: 'f32', kind: 'type', insertText: 'f32' },
                { label: 'f64', kind: 'type', insertText: 'f64' },
                { label: 'bool', kind: 'type', insertText: 'bool' },
                { label: 'String', kind: 'type', insertText: 'String' }
            ];

            let filtered = [];
            let activeIndex = 0;
            let completionRange = null;
            let cachedCharWidth = null;

            const updateGutter = () => {
                if (!gutter) return;
                const lines = editor.value.split('\\n').length;
                let html = '';
                for (let i = 1; i <= lines; i++) {
                    html += i + '\\n';
                }
                gutter.textContent = html.trimEnd();
            };

            const syncGutter = () => {
                if (!gutter) return;
                gutter.scrollTop = editor.scrollTop;
            };

            const getCharWidth = () => {
                if (cachedCharWidth) return cachedCharWidth;
                const span = document.createElement('span');
                span.textContent = 'M';
                span.style.position = 'absolute';
                span.style.visibility = 'hidden';
                span.style.fontFamily = getComputedStyle(editor).fontFamily;
                span.style.fontSize = getComputedStyle(editor).fontSize;
                document.body.appendChild(span);
                cachedCharWidth = span.getBoundingClientRect().width;
                document.body.removeChild(span);
                return cachedCharWidth || 8;
            };

            const getCaretCoordinates = () => {
                const style = getComputedStyle(editor);
                const lineHeight = parseFloat(style.lineHeight) || 20;
                const paddingTop = parseFloat(style.paddingTop) || 0;
                const paddingLeft = parseFloat(style.paddingLeft) || 0;
                const text = editor.value.slice(0, editor.selectionStart);
                const lines = text.split('\\n');
                const line = lines.length - 1;
                const col = lines[lines.length - 1].length;
                const charWidth = getCharWidth();
                const top = paddingTop + line * lineHeight - editor.scrollTop;
                const left = paddingLeft + col * charWidth - editor.scrollLeft;
                return { top, left, lineHeight };
            };

            const getCompletionContext = () => {
                const pos = editor.selectionStart;
                const text = editor.value.slice(0, pos);
                const match = text.match(/[A-Za-z_][A-Za-z0-9_]*$/);
                if (!match) {
                    return { prefix: '', start: pos, end: pos };
                }
                return {
                    prefix: match[0],
                    start: pos - match[0].length,
                    end: pos
                };
            };

            const hideAutocomplete = () => {
                if (!autocomplete) return;
                autocomplete.classList.remove('visible');
                autocomplete.innerHTML = '';
                filtered = [];
                activeIndex = 0;
                completionRange = null;
            };

            const renderAutocomplete = (forceOpen) => {
                if (!autocomplete) return;
                const context = getCompletionContext();
                const prefix = context.prefix.toLowerCase();

                if (!forceOpen && prefix.length === 0) {
                    hideAutocomplete();
                    return;
                }

                filtered = completions.filter(item => item.label.toLowerCase().startsWith(prefix));
                if (filtered.length === 0) {
                    hideAutocomplete();
                    return;
                }

                completionRange = context;
                autocomplete.innerHTML = '';
                filtered.forEach((item, idx) => {
                    const row = document.createElement('div');
                    row.className = 'autocomplete-item' + (idx === 0 ? ' active' : '');
                    row.dataset.index = String(idx);
                    row.innerHTML = `<span>${item.label}</span><span class="item-kind">${item.kind}</span>`;
                    row.addEventListener('mousedown', function (e) {
                        e.preventDefault();
                        applyCompletion(idx);
                    });
                    autocomplete.appendChild(row);
                });

                activeIndex = 0;
                const coords = getCaretCoordinates();
                const left = Math.max(8, coords.left);
                const top = Math.max(8, coords.top + coords.lineHeight + 4);
                autocomplete.style.left = `${left}px`;
                autocomplete.style.top = `${top}px`;
                autocomplete.classList.add('visible');
            };

            const applyCompletion = (index) => {
                if (!completionRange || !filtered[index]) return;
                const item = filtered[index];
                const insertText = item.insertText || item.label;
                const start = completionRange.start;
                const end = completionRange.end;
                editor.setRangeText(insertText, start, end, 'end');
                let cursorPos = start + insertText.length;
                if (typeof item.cursorOffset === 'number') {
                    cursorPos = start + insertText.length + item.cursorOffset;
                }
                editor.setSelectionRange(cursorPos, cursorPos);
                hideAutocomplete();
                updateGutter();
                editor.focus();
            };

            const moveSelection = (delta) => {
                if (!autocomplete || filtered.length === 0) return;
                activeIndex = (activeIndex + delta + filtered.length) % filtered.length;
                const rows = autocomplete.querySelectorAll('.autocomplete-item');
                rows.forEach((row, idx) => {
                    row.classList.toggle('active', idx === activeIndex);
                });
                if (rows[activeIndex]) {
                    rows[activeIndex].scrollIntoView({ block: 'nearest' });
                }
            };

            const insertText = (text) => {
                editor.setRangeText(text, editor.selectionStart, editor.selectionEnd, 'end');
                updateGutter();
            };

            updateGutter();
            syncGutter();

            editor.addEventListener('scroll', syncGutter);
            editor.addEventListener('input', function () {
                updateGutter();
                renderAutocomplete(false);
            });
            editor.addEventListener('click', hideAutocomplete);
            editor.addEventListener('blur', function () {
                setTimeout(hideAutocomplete, 100);
            });

            editor.addEventListener('keydown', function (e) {
                if (autocomplete && autocomplete.classList.contains('visible')) {
                    if (e.key === 'ArrowDown') {
                        e.preventDefault();
                        moveSelection(1);
                        return;
                    }
                    if (e.key === 'ArrowUp') {
                        e.preventDefault();
                        moveSelection(-1);
                        return;
                    }
                    if (e.key === 'Enter') {
                        e.preventDefault();
                        applyCompletion(activeIndex);
                        return;
                    }
                    if (e.key === 'Escape') {
                        e.preventDefault();
                        hideAutocomplete();
                        return;
                    }
                    if (e.key === 'Tab') {
                        e.preventDefault();
                        applyCompletion(activeIndex);
                        return;
                    }
                }

                if ((e.ctrlKey || e.metaKey) && (e.code === 'Space' || e.key === ' ')) {
                    e.preventDefault();
                    renderAutocomplete(true);
                    return;
                }

                if (e.key === 'Tab') {
                    e.preventDefault();
                    insertText('    ');
                    return;
                }

                if (e.key === 'Enter') {
                    const pos = editor.selectionStart;
                    const text = editor.value.slice(0, pos);
                    const lineStart = text.lastIndexOf('\\n') + 1;
                    const line = text.slice(lineStart);
                    const indentMatch = line.match(/^\\s*/);
                    let indent = indentMatch ? indentMatch[0] : '';
                    if (line.trim().endsWith('{')) {
                        indent += '    ';
                    }
                    e.preventDefault();
                    insertText('\\n' + indent);
                    return;
                }

                if (e.key === '}') {
                    const pos = editor.selectionStart;
                    const text = editor.value.slice(0, pos);
                    const lineStart = text.lastIndexOf('\\n') + 1;
                    const line = text.slice(lineStart);
                    if (/^\\s*$/.test(line)) {
                        e.preventDefault();
                        const outdent = line.replace(/\\s{4}$/, '');
                        editor.setRangeText(outdent + '}', lineStart, pos, 'end');
                        updateGutter();
                    }
                }
            });
        }
    }

    // Setup Flows (Mermaid)
    function setupFlows() {
        // specific check to avoid re-adding script if already there
        if (!document.getElementById('mermaid-script')) {
            const script = document.createElement('script');
            script.id = 'mermaid-script';
            script.type = 'module';
            script.src = 'https://cdn.jsdelivr.net/npm/mermaid@10/dist/mermaid.esm.min.mjs';
            document.head.appendChild(script);

            // Trigger script to expose mermaid to window and init
            const triggerScript = document.createElement('script');
            triggerScript.type = 'module';
            triggerScript.textContent = `
                import mermaid from 'https://cdn.jsdelivr.net/npm/mermaid@10/dist/mermaid.esm.min.mjs';
                window.mermaid = mermaid;
                mermaid.initialize({ 
                    startOnLoad: true, 
                    theme: document.body.classList.contains('dark-mode') ? 'dark' : 'default',
                    securityLevel: 'loose',
                });
                mermaid.run({
                    querySelector: '.mermaid'
                });
            `;
            document.body.appendChild(triggerScript);
        } else {
            // Re-run if already loaded
            // We use a small timeout or try to import it again if window.mermaid isn't set
            const triggerScript = document.createElement('script');
            triggerScript.type = 'module';
            triggerScript.textContent = `
                import mermaid from 'https://cdn.jsdelivr.net/npm/mermaid@10/dist/mermaid.esm.min.mjs';
                mermaid.run({ querySelector: '.mermaid' });
            `;
            document.body.appendChild(triggerScript);
        }
    }

    // Navigate to section
    function navigateTo(id) {
        const item = window.documentationContent.find(function (i) { return i.id === id; });
        if (!item) return;


        // Render content
        docContent.innerHTML = parseMarkdown(item.content);

        // Reveal animations
        setupReveals();

        // Build On This Page navigation
        buildPageTOC();

        // Initialize Playground if loaded
        if (id === 'playground') {
            // Short delay to ensure DOM update
            setTimeout(setupPlayground, 50);
        }

        // Initialize Flows if loaded
        if (id === 'flows') {
            setTimeout(setupFlows, 50);
        }

        // Update Pagination
        updatePagination(id);

        // Update current section
        currentSectionSpan.textContent = item.title;

        // Update active TOC item
        tocElement.querySelectorAll('.toc-item').forEach(function (el) {
            el.classList.remove('active');
        });
        const activeItem = tocElement.querySelector('[data-id="' + id + '"]');
        if (activeItem) {
            activeItem.classList.add('active');
        }

        // Scroll to top
        docContent.scrollTop = 0;
        scrollToTop();

        // Update URL hash
        history.pushState(null, '', '#' + id);
    }

    // Update Pagination Links
    function updatePagination(currentId) {
        const content = window.documentationContent;
        const currentIndex = content.findIndex(function (item) { return item.id === currentId; });

        const prevLink = document.getElementById('nav-prev');
        const nextLink = document.getElementById('nav-next');

        if (currentIndex > 0) {
            const prevItem = content[currentIndex - 1];
            prevLink.href = '#' + prevItem.id;
            prevLink.classList.remove('disabled');
            prevLink.querySelector('.nav-title').textContent = prevItem.title;
            prevLink.onclick = function (e) {
                e.preventDefault();
                navigateTo(prevItem.id);
            };
        } else {
            prevLink.classList.add('disabled');
            prevLink.href = '#';
            prevLink.onclick = function (e) { e.preventDefault(); };
        }

        if (currentIndex < content.length - 1) {
            const nextItem = content[currentIndex + 1];
            nextLink.href = '#' + nextItem.id;
            nextLink.classList.remove('disabled');
            nextLink.querySelector('.nav-title').textContent = nextItem.title;
            nextLink.onclick = function (e) {
                e.preventDefault();
                navigateTo(nextItem.id);
            };
        } else {
            nextLink.classList.add('disabled');
            nextLink.href = '#';
            nextLink.onclick = function (e) { e.preventDefault(); };
        }
    }

    // Build On This Page navigation from headings
    function buildPageTOC() {
        const pageToc = document.getElementById('page-toc');
        if (!pageToc) return;

        // Hide TOC on Playground and Flows pages
        const currentId = window.location.hash.substring(1);
        const tocContainer = document.querySelector('.on-this-page');

        if (currentId === 'playground' || currentId === 'flows') {
            document.body.classList.add('playground-mode');
            if (tocContainer) tocContainer.style.display = 'none';
            pageToc.innerHTML = '';
            return;
        } else {
            document.body.classList.remove('playground-mode');
            if (tocContainer) tocContainer.style.display = 'block';
        }

        const headings = docContent.querySelectorAll('h2, h3');
        let tocHTML = '';

        headings.forEach(function (heading, index) {
            const text = heading.textContent;
            const level = heading.tagName.toLowerCase();
            const headingId = 'heading-' + index;
            heading.id = headingId;

            const itemClass = level === 'h3' ? 'h3-item' : '';
            tocHTML += '<li class="' + itemClass + '">';
            tocHTML += '<a href="#' + headingId + '">' + text + '</a>';
            tocHTML += '</li>';
        });

        pageToc.innerHTML = tocHTML;

        // Add click handlers for smooth scroll
        pageToc.querySelectorAll('a').forEach(function (link) {
            link.addEventListener('click', function (e) {
                e.preventDefault();
                const targetId = this.getAttribute('href').substring(1);
                const target = document.getElementById(targetId);
                if (target) {
                    smoothScrollTo(target, { offset: -80, duration: 1.1 });
                    // Update active state
                    pageToc.querySelectorAll('a').forEach(function (a) {
                        a.classList.remove('active');
                    });
                    this.classList.add('active');
                }
            });
        });

        // Set first item as active
        const firstLink = pageToc.querySelector('a');
        if (firstLink) {
            firstLink.classList.add('active');
        }

        // Add scroll spy
        setupScrollSpy();
    }

    let currentScrollHandler = null;

    // Scroll spy to highlight current section
    function setupScrollSpy() {
        const pageToc = document.getElementById('page-toc');
        if (!pageToc) return;

        const headings = docContent.querySelectorAll('h2, h3');
        const progressBar = document.getElementById('scroll-progress-bar');
        const tocWrapper = document.querySelector('.page-toc-wrapper');

        // Remove existing handler if any
        if (currentScrollHandler) {
            window.removeEventListener('scroll', currentScrollHandler);
        }

        // Helper to update progress bar and scroll TOC
        function updateTOCState(activeLink) {
            if (!progressBar || !activeLink || !pageToc) return;

            // 1. Move the scroll progress bar
            // We use standard offsetTop calculation relative to the TOC container
            const linkRect = activeLink.getBoundingClientRect();
            const wrapperRect = tocWrapper.getBoundingClientRect();

            // Calculate position relative to the wrapper
            // Note: Since we want the bar to sit visually next to the link even if the list scrolls,
            // we rely on getBoundingClientRect() which gives us the visual position.
            const relativeTop = linkRect.top - wrapperRect.top;

            progressBar.style.top = relativeTop + 'px';
            progressBar.style.height = linkRect.height + 'px';

            // 2. Smoothly scroll the TOC list if the item is out of view or close to edge
            const tocHeight = pageToc.clientHeight;
            const linkTop = activeLink.offsetTop;
            const linkHeight = activeLink.offsetHeight;

            // Calculate clear visibility zone
            // We want to center it
            const targetScroll = linkTop - (tocHeight / 2) + (linkHeight / 2);

            pageToc.scrollTo({
                top: targetScroll,
                behavior: 'smooth'
            });
        }

        if (headings.length === 0) return;

        currentScrollHandler = function () {
            let current = '';
            const scrollPos = window.scrollY + 100;

            headings.forEach(function (heading) {
                const sectionTop = heading.offsetTop;
                if (scrollPos >= sectionTop) {
                    current = heading.id;
                }
            });

            // If we're at the bottom of the page, check if we should highlight the last item
            if ((window.innerHeight + window.scrollY) >= document.body.offsetHeight - 50) {
                if (headings.length > 0) {
                    current = headings[headings.length - 1].id;
                }
            }

            let foundActive = false;
            pageToc.querySelectorAll('a').forEach(function (link) {
                link.classList.remove('active');
                if (link.getAttribute('href') === '#' + current) {
                    link.classList.add('active');
                    updateTOCState(link);
                    foundActive = true;
                }
            });

            // If no current (top of page), highlight first
            if (!foundActive) {
                const firstLink = pageToc.querySelector('a');
                if (firstLink) {
                    firstLink.classList.add('active');
                    updateTOCState(firstLink);
                }
            }
        };

        window.addEventListener('scroll', currentScrollHandler);

        // Initial update
        setTimeout(function () {
            currentScrollHandler();
        }, 100);
    }


    // Search functionality
    function filterTOC(query) {
        const lowerQuery = query.toLowerCase();

        tocElement.querySelectorAll('.toc-item').forEach(function (item) {
            const title = item.textContent.toLowerCase();
            if (title.includes(lowerQuery) || !query) {
                item.classList.remove('hidden');
            } else {
                item.classList.add('hidden');
            }
        });

        // Show section headers if any child is visible
        tocElement.querySelectorAll('.toc-section').forEach(function (section) {
            const visibleItems = section.querySelectorAll('.toc-item:not(.hidden)');
            section.style.display = visibleItems.length > 0 || !query ? '' : 'none';
        });
    }

    // Initialize
    function init() {
        setupLenis();
        setupMotionBackdrop();
        buildTOC();

        // Handle initial hash
        const hash = window.location.hash.substring(1);
        if (hash && window.documentationContent.find(function (i) { return i.id === hash; })) {
            navigateTo(hash);
        } else if (window.documentationContent && window.documentationContent.length > 0) {
            navigateTo(window.documentationContent[0].id);
        }

        // Search input handler
        searchInput.addEventListener('input', function () {
            filterTOC(this.value);
        });

        // Handle browser back/forward
        window.addEventListener('popstate', function () {
            const hash = window.location.hash.substring(1);
            if (hash) {
                navigateTo(hash);
            }
        });
    }

    // Theme Management
    const themeToggle = document.getElementById('theme-toggle');
    const themeIcon = document.getElementById('theme-icon');

    // Moon (Dark Mode Icon - User Provided)
    const moonContent = `<path fill="currentColor" d="M12.058 20q-3.333 0-5.667-2.334Q4.058 15.333 4.058 12q0-2.47 1.413-4.536t4.01-2.972q.306-.107.536-.056q.231.05.381.199t.191.38q.042.233-.062.489q-.194.477-.282.966t-.087 1.03q0 2.667 1.866 4.533q1.867 1.867 4.534 1.867q.698 0 1.278-.148q.58-.148.987-.24q.217-.04.4.01q.18.051.287.176q.119.125.16.308q.042.182-.047.417q-.715 2.45-2.803 4.014Q14.733 20 12.058 20Zm0-1q2.2 0 3.95-1.213t2.55-3.162q-.5.125-1 .2t-1 .075q-3.075 0-5.238-2.163T9.158 7.5q0-.5.075-1t.2-1q-1.95.8-3.163 2.55T5.058 12q0 2.9 2.05 4.95t4.95 2.05Zm-.25-6.75Z"/>`;

    // Sun (Light Mode Icon - User Provided)
    const sunContent = `<g fill="none" stroke="currentColor" stroke-width="1.5"><circle cx="12" cy="12" r="4" stroke-linejoin="round"/><path stroke-linecap="round" d="M20 12h1M3 12h1m8 8v1m0-18v1m5.657 13.657l.707.707M5.636 5.636l.707.707m0 11.314l-.707.707M18.364 5.636l-.707.707"/></g>`;

    function setTheme(isDark) {
        if (isDark) {
            document.body.classList.add('dark-mode');
            localStorage.setItem('theme', 'dark');
            themeIcon.innerHTML = sunContent; // Show Sun
        } else {
            document.body.classList.remove('dark-mode');
            localStorage.setItem('theme', 'light');
            themeIcon.innerHTML = moonContent; // Show Moon
        }
    }

    if (themeToggle) {
        themeToggle.addEventListener('click', function () {
            const isDark = !document.body.classList.contains('dark-mode');
            setTheme(isDark);
        });

        // Initialize theme
        const savedTheme = localStorage.getItem('theme');
        if (savedTheme === 'dark') {
            setTheme(true);
        }
    }

    // Focus Mode
    const focusToggle = document.getElementById('focus-toggle');

    function setFocusMode(enabled) {
        if (enabled) {
            document.body.classList.add('sidebar-hidden');
            localStorage.setItem('focus', 'true');
            // Make SVG filled to indicate active
            focusToggle.style.color = 'var(--primary-color)';
        } else {
            document.body.classList.remove('sidebar-hidden');
            localStorage.setItem('focus', 'false');
            focusToggle.style.color = '';
        }
    }

    if (focusToggle) {
        focusToggle.addEventListener('click', function () {
            const isFocused = !document.body.classList.contains('sidebar-hidden');
            setFocusMode(isFocused);
        });

        if (localStorage.getItem('focus') === 'true') {
            setFocusMode(true);
        }
    }

    // Run init when DOM is ready
    if (document.readyState === 'loading') {
        document.addEventListener('DOMContentLoaded', init);
    } else {
        init();
    }

})();
