// src/components/ui/Navbar.jsx

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

export function Navbar({ onToggleSidebar }) {
  return (
    <nav className="navbar">
      <button
        className="navbar-hamburger"
        onClick={onToggleSidebar}
        aria-label="Toggle sidebar"
      >
        ☰
      </button>
      <div className="navbar-brand">
        <div className="navbar-logo">
          <SparkleIcon className="navbar-logo-icon" />
          <span>fluendo</span>
        </div>
        <div className="navbar-divider" aria-hidden="true" />
        <span className="navbar-title">Fluendo Multimedia Showcase</span>
      </div>
    </nav>
  );
}
