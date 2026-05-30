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
  const frameRef = useRef(null);
  const [loadedCode, setLoadedCode] = useState({ path: null, text: '' });

  useEffect(() => {
    document.title = pageName;
  }, [pageName]);

  useEffect(() => {
    const ansi_up = new AnsiUp();
    const appendLog = (verb, args) => {
      if (!logRef.current) {
        return;
      }

      const msg = document.createElement('div');
      msg.className = verb;
      msg.innerHTML = ansi_up.ansi_to_html(args.join(' '));
      logRef.current.appendChild(msg);
    };

    if (logRef.current) {
      logRef.current.innerHTML = '';
    }

    const handleMessage = (event) => {
      if (event.source !== frameRef.current?.contentWindow) {
        return;
      }

      if (event.data?.type !== 'wasm-example-log') {
        return;
      }

      appendLog(event.data.verb, event.data.args ?? []);
    };

    window.addEventListener('message', handleMessage);

    return () => {
      window.removeEventListener('message', handleMessage);
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

  const runnerHtml = `<!doctype html>
<html>
  <head>
    <meta charset="utf-8" />
    <style>
      html, body {
        margin: 0;
        background: black;
        overflow: hidden;
      }

      canvas {
        display: block;
        width: 640px;
        height: 480px;
      }
    </style>
  </head>
  <body>
    <canvas id="canvas" width="640" height="480"></canvas>
    <script>
      const forwardConsole = (verb, args) => {
        window.parent.postMessage({
          type: 'wasm-example-log',
          verb,
          args: args.map((arg) => String(arg)),
        }, '*');
      };

      ['log', 'debug', 'info', 'warn', 'error'].forEach((verb) => {
        const original = console[verb].bind(console);
        console[verb] = (...args) => {
          original(...args);
          forwardConsole(verb, args);
        };
      });

      window.addEventListener('error', (event) => {
        forwardConsole('error', [event.message]);
      });

      window.Module = {
        canvas: document.getElementById('canvas'),
      };

      const script = document.createElement('script');
      script.src = ${JSON.stringify(executableName)};
      script.defer = true;
      document.body.appendChild(script);
    </script>
  </body>
</html>`;

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

      <iframe
        key={executableName}
        ref={frameRef}
        title={`${pageName} runner`}
        srcDoc={runnerHtml}
        className="block mb-4 border bg-black"
        width="640"
        height="480"
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
