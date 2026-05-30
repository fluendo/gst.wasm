import { useEffect, useRef, useState } from 'react';
import hljs from 'highlight.js';
import 'highlight.js/styles/default.css';
import { AnsiUp } from 'ansi_up';

export function WasmExample({
  pageName,
  descriptionTitle,
  descriptionContent,
  streamUrl,
  code,
  executableName
}) {
  const logRef = useRef(null);
  const canvasRef = useRef(null);
  const [loadedCode, setLoadedCode] = useState({ path: null, text: '' });

  useEffect(() => {
    document.title = pageName;

    const ansi_up = new AnsiUp();
    const originalConsole = {
      log: console.log, debug: console.debug, info: console.info,
      warn: console.warn, error: console.error
    };

    ['log', 'debug', 'info', 'warn', 'error'].forEach((verb) => {
      console[verb] = (...args) => {
        originalConsole[verb](...args); // Keep logging to real browser console

        if (logRef.current) {
          const msg = document.createElement("div");
          msg.className = verb;
          const html = ansi_up.ansi_to_html(args.join(" "));
          msg.innerHTML = html;
          logRef.current.appendChild(msg);
        }
      };
    });

    const moduleConfig = {
      canvas: canvasRef.current,
    };

    const script = document.createElement('script');
    script.src = executableName;
    script.defer = true;
    let disposed = false;
    let moduleInstance = null;
    let moduleInitPromise = null;

    script.onload = () => {
      const moduleFactory = window.Module;
      if (typeof moduleFactory !== 'function') {
        console.error(`Loaded script is not modularized: ${executableName}`);
        return;
      }

      const initializedModule = moduleFactory(moduleConfig);
      if (initializedModule && typeof initializedModule.then === 'function') {
        moduleInitPromise = initializedModule
          .then((instance) => {
            if (disposed && instance && typeof instance.quit === 'function') {
              instance.quit();
              return;
            }
            moduleInstance = instance;
          })
          .catch((error) => {
            console.error('Unable to initialize WebAssembly module', error);
          });
      } else {
        if (disposed && initializedModule && typeof initializedModule.quit === 'function') {
          initializedModule.quit();
          return;
        }
        moduleInstance = initializedModule;
      }
    };

    script.onerror = () => {
      console.error(`Unable to load executable script: ${executableName}`);
    };

    document.body.appendChild(script);

    return () => {
      disposed = true;
      Object.assign(console, originalConsole);
      if (moduleInstance && typeof moduleInstance.quit === 'function') {
        moduleInstance.quit();
      }
      if (moduleInitPromise) {
        moduleInitPromise.catch(() => { });
      }
      if (document.body.contains(script)) {
        document.body.removeChild(script);
      }
      delete window.Module;
    };
  }, [executableName, pageName]);

  useEffect(() => {
    let active = true;

    if (!code || !code.endsWith('.c')) {
      return () => {
        active = false;
      };
    }

    fetch(code)
      .then((response) => {
        if (!response.ok) {
          throw new Error(`Unable to fetch ${code}`);
        }
        return response.text();
      })
      .then((text) => {
        if (active) {
          setLoadedCode({ path: code, text });
        }
      })
      .catch(() => {
        if (active) {
          setLoadedCode({ path: code, text: '// Failed to load source code.' });
        }
      });

    return () => {
      active = false;
    };
  }, [code]);

  const sourceCode = !code
    ? ''
    : code.endsWith('.c')
      ? (loadedCode.path === code ? loadedCode.text : '')
      : code;

  useEffect(() => {
    hljs.highlightAll();
  }, [sourceCode]);

  return (
    <div className="wasm-container p-4">
      {/* Header Section */}
      <div className="mb-5 p-3 bg-gray-100 border-b border-gray-300">
        <h1 className="text-2xl font-bold">{descriptionTitle}</h1>

        {descriptionContent && (
          <p className="mt-2">{descriptionContent}</p>
        )}

        {streamUrl && (
          <p className="mt-2">
            <strong>Stream URL:</strong>{' '}
            <a href={streamUrl} target="_blank" rel="noreferrer" className="text-blue-600 underline">
              {streamUrl}
            </a>
          </p>
        )}
      </div>

      {/* Emscripten Canvas */}
      <canvas
        ref={canvasRef}
        id="canvas"
        width="640"
        height="480"
        className="block mb-4 border bg-black"
      />

      {/* C Code Block */}
      <pre className="mb-4">
        <code className="language-c">{sourceCode}</code>
      </pre>

      {/* Custom Console Log UI */}
      <div
        ref={logRef}
        id="log"
        className="m-0 p-2 font-mono text-xs bg-black text-white min-w-[640px] min-h-[480px] whitespace-pre-wrap overflow-y-auto"
      />
    </div>
  );
}
