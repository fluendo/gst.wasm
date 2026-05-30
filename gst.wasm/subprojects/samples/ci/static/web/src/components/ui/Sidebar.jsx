// src/components/ui/Sidebar.jsx

export function Sidebar({ examples, activeExample, onSelect, visible, onClose }) {
  return (
    <aside className={`sidebar${visible ? '' : ' hidden'}`}>
      <div className="sidebar-header">
        <span className="sidebar-heading">Example Navigator</span>
        <button
          className="sidebar-close"
          onClick={onClose}
          aria-label="Close sidebar"
        >
          ✕
        </button>
      </div>
      <ul className="sidebar-list" role="list">
        {examples.map((example) => (
          <li key={example.id} className="sidebar-item">
            <button
              className={`sidebar-item-btn${activeExample?.id === example.id ? ' active' : ''}`}
              onClick={() => onSelect(example)}
            >
              {example.pageName}
            </button>
          </li>
        ))}
      </ul>
    </aside>
  );
}
