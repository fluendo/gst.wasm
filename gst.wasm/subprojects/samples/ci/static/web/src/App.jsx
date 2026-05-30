import { useCallback, useEffect, useMemo, useState } from 'react';
import { Home } from './pages/Home';
import { WasmExample } from './components/ui/WasmExample';
import { examplesById, getExampleHash, getExampleIdFromHash } from './data/examples';
import './App.css';

function App() {
  const [activeExampleId, setActiveExampleId] = useState(() => getExampleIdFromHash(window.location.hash));

  useEffect(() => {
    const handleHashChange = () => {
      setActiveExampleId(getExampleIdFromHash(window.location.hash));
    };

    handleHashChange();
    window.addEventListener('hashchange', handleHashChange);

    return () => {
      window.removeEventListener('hashchange', handleHashChange);
    };
  }, []);

  const activeExample = useMemo(
    () => (activeExampleId ? examplesById[activeExampleId] ?? null : null),
    [activeExampleId],
  );

  useEffect(() => {
    if (!activeExample) {
      document.title = 'gst.wasm Samples';
    }
  }, [activeExample]);

  const handleSelectExample = useCallback((exampleId) => {
    window.location.hash = getExampleHash(exampleId);
  }, []);

  const handleBackToHome = useCallback(() => {
    window.location.hash = '#/';
  }, []);

  return (
    <main>
      {activeExample ? (
        <div>
          <button
            onClick={handleBackToHome}
            className="btn btn-secondary mb-3 ms-3 mt-3"
            style={{ cursor: 'pointer' }}
          >
            ← Back to Home
          </button>

          <WasmExample
            key={activeExample.id}
            pageName={activeExample.pageName}
            descriptionTitle={activeExample.descriptionTitle}
            descriptionContent={activeExample.descriptionContent}
            streamUrl={activeExample.streamUrl}
            code={activeExample.code}
            executableName={activeExample.executableName}
          />
        </div>
      ) : (
        <Home onSelectExample={handleSelectExample} />
      )}
    </main>
  );
}

export default App;
