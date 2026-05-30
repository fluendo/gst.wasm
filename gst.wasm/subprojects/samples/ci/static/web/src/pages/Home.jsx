import { examplesData } from '../data/examples';

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
        {examplesData.map((example) => (
          <li key={example.id} className="list-group-item">
            <button
              type="button"
              onClick={() => onSelectExample(example.id)}
              style={{
                background: 'none',
                border: 'none',
                color: '#0d6efd',
                textDecoration: 'underline',
                padding: 0,
                cursor: 'pointer',
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
