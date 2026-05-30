// src/App.jsx
import { useState } from 'react';
import { Home } from './pages/Home';
import { WasmExample } from './components/ui/WasmExample'; // Adjust path if needed
import './App.css';

function App() {
  // This state holds the data of the currently clicked example.
  // If it's null, we show the Home page.
  const [activeExample, setActiveExample] = useState(null);

  return (
    <main>
      {activeExample ? (
        // If an example is selected, show the Wasm template
        <div>
          <button
            onClick={() => setActiveExample(null)}
            className="btn btn-secondary mb-3 ms-3 mt-3"
            style={{ cursor: 'pointer' }}
          >
            ← Back to Home
          </button>

          <WasmExample
            pageName={activeExample.pageName}
            descriptionTitle={activeExample.descriptionTitle}
            descriptionContent={activeExample.descriptionContent}
            streamUrl={activeExample.streamUrl}
            code={activeExample.code}
            executableName={activeExample.executableName}
          />
        </div>
      ) : (
        // If no example is selected (null), show the Home list
        <Home onSelectExample={setActiveExample} />
      )}
    </main>
  );
}

export default App;
