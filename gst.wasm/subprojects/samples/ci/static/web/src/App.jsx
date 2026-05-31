// src/App.jsx
import { useState } from 'react';
import { examplesData } from './data/examples';
import { Navbar } from './components/ui/Navbar';
import { Sidebar } from './components/ui/Sidebar';
import { Footer } from './components/ui/Footer';
import { WasmExample } from './components/ui/WasmExample';
import './App.css';

function App() {
  const [activeExample, setActiveExample] = useState(examplesData[0]);
  const [sidebarVisible, setSidebarVisible] = useState(true);

  return (
    <div className="app-layout">
      <Navbar onToggleSidebar={() => setSidebarVisible((v) => !v)} />

      <div className="app-body">
        <Sidebar
          examples={examplesData}
          activeExample={activeExample}
          onSelect={setActiveExample}
          visible={sidebarVisible}
          onClose={() => setSidebarVisible(false)}
        />

        <main className="app-main">
          {activeExample && (
            <WasmExample
              key={activeExample.id}
              pageName={activeExample.pageName}
              descriptionTitle={activeExample.descriptionTitle}
              descriptionContent={activeExample.descriptionContent}
              streamUrl={activeExample.streamUrl}
              code={activeExample.code}
              executableName={activeExample.executableName}
              inputEnabled={activeExample.inputEnabled}
              inputId={activeExample.inputId}
              inputPlaceholder={activeExample.inputPlaceholder}
              inputInitialValue={activeExample.inputInitialValue}
              inputSubmitFunction={activeExample.inputSubmitFunction}
            />
          )}
        </main>
      </div>

      <Footer />
    </div>
  );
}

export default App;
