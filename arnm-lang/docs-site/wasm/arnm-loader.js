/*
 * ARNm WASM Loader
 * 
 * Loads the ARNm WebAssembly module and provides a simple API for the playground.
 * Falls back gracefully when WASM is unavailable (e.g., file:// protocol).
 */

let arnmModule = null;
let arnmReady = false;
let arnmLoadAttempted = false;
let arnmLoadError = null;

// Check if we're in an environment that can load WASM
function canLoadWasm() {
    // WASM requires HTTP/HTTPS - file:// protocol won't work
    if (window.location.protocol === 'file:') {
        console.warn('[ARNm] WASM not available: file:// protocol detected. Use a local server.');
        return false;
    }
    // Check if ArnmModule is defined (Emscripten script loaded)
    if (typeof ArnmModule === 'undefined') {
        console.warn('[ARNm] WASM not available: ArnmModule not defined.');
        return false;
    }
    return true;
}

// Initialize WASM module
async function initArnmWasm() {
    if (arnmReady) return true;
    if (arnmLoadAttempted) return false; // Don't retry if already failed

    arnmLoadAttempted = true;

    if (!canLoadWasm()) {
        arnmLoadError = 'WASM requires HTTP server (use: python3 -m http.server 8080)';
        return false;
    }

    try {
        // Load the Emscripten-generated module
        arnmModule = await ArnmModule();
        arnmReady = true;
        console.log('[ARNm] WASM module loaded successfully');
        return true;
    } catch (error) {
        console.error('[ARNm] Failed to load WASM module:', error);
        arnmLoadError = error.message;
        return false;
    }
}

// Compile and run ARNm code
async function runArnmCode(source) {
    if (!arnmReady) {
        const loaded = await initArnmWasm();
        if (!loaded) {
            // Return null to signal fallback to mock interpreter
            return null;
        }
    }

    try {
        // Call the compile function
        const result = arnmModule.ccall(
            'arnm_compile_string',
            'number',
            ['string'],
            [source]
        );

        // Get the output
        const outputPtr = arnmModule.ccall('arnm_get_output', 'number', [], []);
        const output = arnmModule.UTF8ToString(outputPtr);

        return {
            success: result === 0,
            output: output || (result === 0 ? '[No output]' : '[Compilation failed]'),
            exitCode: result
        };
    } catch (error) {
        return {
            success: false,
            output: `[Runtime Error] ${error.message}`,
            exitCode: -1
        };
    }
}

// Get reason for WASM unavailability
function getLoadError() {
    return arnmLoadError;
}

// Export for use in playground
window.ArnmWasm = {
    init: initArnmWasm,
    run: runArnmCode,
    isReady: () => arnmReady,
    canLoad: canLoadWasm,
    getError: getLoadError
};
