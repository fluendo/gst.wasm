import { useEffect, useRef } from 'react';
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

  useEffect(() => {
    document.title = pageName;

    hljs.highlightAll();

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

    window.Module = {
      canvas: canvasRef.current,
    };

    const script = document.createElement('script');
    script.src = executableName;
    script.defer = true;
    document.body.appendChild(script);

    return () => {
      Object.assign(console, originalConsole);
      if (document.body.contains(script)) {
        document.body.removeChild(script);
      }
      delete window.Module;
    };
  }, [executableName, pageName]);

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
        <code className="language-c">{code}</code>
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
