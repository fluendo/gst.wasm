import { useEffect, useRef, useState } from 'react';
import hljs from 'highlight.js';
import 'highlight.js/styles/default.css';
import { AnsiUp } from 'ansi_up';

function GearIcon() {
  return (
    <svg width="14" height="14" viewBox="0 0 24 24" fill="none" stroke="currentColor" strokeWidth="2" aria-hidden="true">
      <circle cx="12" cy="12" r="3" />
      <path d="M19.4 15a1.65 1.65 0 0 0 .33 1.82l.06.06a2 2 0 0 1-2.83 2.83l-.06-.06a1.65 1.65 0 0 0-1.82-.33 1.65 1.65 0 0 0-1 1.51V21a2 2 0 0 1-4 0v-.09A1.65 1.65 0 0 0 9 19.4a1.65 1.65 0 0 0-1.82.33l-.06.06a2 2 0 0 1-2.83-2.83l.06-.06A1.65 1.65 0 0 0 4.68 15a1.65 1.65 0 0 0-1.51-1H3a2 2 0 0 1 0-4h.09A1.65 1.65 0 0 0 4.6 9a1.65 1.65 0 0 0-.33-1.82l-.06-.06a2 2 0 0 1 2.83-2.83l.06.06A1.65 1.65 0 0 0 9 4.68a1.65 1.65 0 0 0 1-1.51V3a2 2 0 0 1 4 0v.09a1.65 1.65 0 0 0 1 1.51 1.65 1.65 0 0 0 1.82-.33l.06-.06a2 2 0 0 1 2.83 2.83l-.06.06A1.65 1.65 0 0 0 19.4 9a1.65 1.65 0 0 0 1.51 1H21a2 2 0 0 1 0 4h-.09a1.65 1.65 0 0 0-1.51 1z" />
    </svg>
  );
}

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
  const [activeTab, setActiveTab] = useState('console');

  useEffect(() => {
    document.title = pageName;

    const ansi_up = new AnsiUp();
    const originalConsole = {
      log: console.log, debug: console.debug, info: console.info,
      warn: console.warn, error: console.error
    };

    ['log', 'debug', 'info', 'warn', 'error'].forEach((verb) => {
      console[verb] = (...args) => {
        originalConsole[verb](...args);

        if (logRef.current) {
          const msg = document.createElement("div");
          msg.className = verb;
          const html = ansi_up.ansi_to_html(args.join(" "));
          msg.innerHTML = html;
          logRef.current.appendChild(msg);
        }
      };
    });

    const script = document.createElement('script');
    script.src = executableName;
    script.async = true;

    script.onload = () => {
      if (typeof window.Module === 'function') {
        const baseDir = executableName.substring(0, executableName.lastIndexOf('/'));

        window.Module({
          canvas: canvasRef.current,
          mainScriptUrlOrBlob: executableName,
          locateFile: (path) => {
            return `${baseDir}/${path}`;
          },
          print: (...args) => console.log(...args),
          printErr: (...args) => console.error(...args)
        }).then(() => {
          console.info(`[React] Successfully loaded and started ${pageName}`);
        }).catch((err) => {
          console.error(`[React] Failed to initialize WebAssembly module:`, err);
        });
      } else {
        console.error("[React] Script loaded, but window.Module factory function is missing.");
      }
    };

    document.body.appendChild(script);

    return () => {
      Object.assign(console, originalConsole);
      if (document.body.contains(script)) {
        document.body.removeChild(script);
      }
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
    <div className="wasm-example">
      {/* Top two-column row */}
      <div className="wasm-top-row">
        {/* Left: Example Information */}
        <div>
          <h2 className="section-heading">Example Information</h2>

          {/* System Configuration card */}
          <div className="info-card">
            <div className="info-card-title">
              <GearIcon /> System Configuration
            </div>
            <p className="info-card-row">Current Example: [{pageName}]</p>
            {streamUrl && (
              <code className="info-card-url">{streamUrl}</code>
            )}
          </div>

          {/* Example Description card */}
          <div className="info-card">
            <div className="info-card-title">Example Description</div>
            {(descriptionTitle || descriptionContent) && (
              <p className="info-card-description">
                {descriptionTitle && <strong>About {descriptionTitle}:</strong>}
                {descriptionTitle && descriptionContent && ' '}
                {descriptionContent}
              </p>
            )}
          </div>
        </div>

        {/* Right: Output & Status */}
        <div>
          <h2 className="section-heading">Output &amp; Status</h2>
          <div className="canvas-panel">
            <div className="canvas-panel-header">
              <span className="canvas-panel-label">Primary Render Canvas</span>
              <span className="canvas-panel-optional">Optional</span>
            </div>
            <div className="canvas-panel-body">
              <canvas
                ref={canvasRef}
                id="canvas"
                width="640"
                height="480"
                className="wasm-canvas"
              />
            </div>
          </div>
        </div>
      </div>

      {/* Bottom: Console / Source tabs */}
      <div className="tabs-section">
        <div className="tabs-bar">
          <button
            className={`tab-btn${activeTab === 'console' ? ' active' : ''}`}
            onClick={() => setActiveTab('console')}
          >
            Console Log
          </button>
          <button
            className={`tab-btn${activeTab === 'source' ? ' active' : ''}`}
            onClick={() => setActiveTab('source')}
          >
            Source Code
          </button>
        </div>

        {activeTab === 'console' ? (
          <div
            ref={logRef}
            id="log"
            className="console-log"
          />
        ) : (
          <pre className="source-code-block">
            <code className="language-c">{sourceCode}</code>
          </pre>
        )}
      </div>
    </div>
  );
}

