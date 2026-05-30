// src/pages/Home.jsx
import { examplesData } from '../data/examples'; // Adjust path to where your data file is

export function Home({ onSelectExample }) {
  return (
    <div className="container mt-4">
      <p align="center">
        <img
          src="https://raw.githubusercontent.com/fluendo/gst.wasm/main/artwork/gst.wasm.svg"
          width="100"
          height="100"
          alt="gst.wasm logo"
        />
      </p>
      <h1 style={{ textAlign: 'center' }}>
        <pre>gst.wasm</pre>
      </h1>

      <ul className="list-group">
        {/* Loop through your data file to generate the list */}
        {examplesData.map((example) => (
          <li key={example.id} className="list-group-item">
            <button
              onClick={() => onSelectExample(example)}
              // Styling it to look exactly like a standard Bootstrap link
              style={{
                background: 'none',
                border: 'none',
                color: '#0d6efd',
                textDecoration: 'underline',
                padding: 0,
                cursor: 'pointer'
              }}
            >
              {example.pageName}
            </button>
          </li>
        ))}
      </ul>
    </div>
  );
}
