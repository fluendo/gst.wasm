// src/components/ui/Footer.jsx

function SparkleIcon({ className }) {
  return (
    <svg
      className={className}
      viewBox="0 0 24 24"
      fill="currentColor"
      aria-hidden="true"
    >
      <path d="M12 2 L13.5 9 L20 10.5 L13.5 12 L12 19 L10.5 12 L4 10.5 L10.5 9 Z" />
    </svg>
  );
}

export function Footer() {
  return (
    <footer className="footer">
      <div className="footer-brand">
        <div className="footer-logo">
          <SparkleIcon className="footer-logo-icon" />
          <span>fluendo</span>
        </div>
        <p className="footer-tagline">
          Advanced increase greating vendors uses in codecs for more advisor
          and profains
        </p>
        <p className="footer-tagline" style={{ marginTop: '6px' }}>
          <a
            href="https://fluendo.com"
            target="_blank"
            rel="noreferrer"
          >
            videoprof.com ↗
          </a>
        </p>
      </div>

      <div className="footer-col">
        <h4 className="footer-col-heading">ODPS Resources</h4>
        <ul className="footer-col-list">
          <li><a href="https://fluendo.com" target="_blank" rel="noreferrer">GUM Test Lösser</a></li>
          <li><a href="https://fluendo.com" target="_blank" rel="noreferrer">RHG</a></li>
          <li><a href="https://fluendo.com" target="_blank" rel="noreferrer">ICmaster</a></li>
        </ul>
      </div>

      <div className="footer-col">
        <h4 className="footer-col-heading">Connect</h4>
        <ul className="footer-col-list">
          <li><a href="https://fluendo.com" target="_blank" rel="noreferrer">Centruss</a></li>
          <li><a href="https://fluendo.com" target="_blank" rel="noreferrer">Deltas</a></li>
          <li><a href="https://linkedin.com" target="_blank" rel="noreferrer">LinkedIn</a></li>
        </ul>
      </div>
    </footer>
  );
}
