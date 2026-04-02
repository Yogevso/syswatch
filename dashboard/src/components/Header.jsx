import './Header.css'

export default function Header({ connected, lastUpdate }) {
  return (
    <header className="header">
      <div className="header-left">
        <div className="header-logo">
          <svg width="32" height="32" viewBox="0 0 32 32" fill="none">
            <rect width="32" height="32" rx="8" fill="url(#logoGrad)" />
            <path d="M8 20L12 12L16 17L20 10L24 16" stroke="#fff" strokeWidth="2.5" strokeLinecap="round" strokeLinejoin="round"/>
            <circle cx="24" cy="16" r="2" fill="#fff"/>
            <defs>
              <linearGradient id="logoGrad" x1="0" y1="0" x2="32" y2="32">
                <stop stopColor="#00e5ff" />
                <stop offset="1" stopColor="#00e676" />
              </linearGradient>
            </defs>
          </svg>
        </div>
        <div className="header-text">
          <h1 className="header-title">SysWatch</h1>
          <span className="header-subtitle">System Observability Dashboard</span>
        </div>
      </div>
      <div className="header-right">
        <div className={`status-badge ${connected ? 'status-connected' : 'status-disconnected'}`}>
          <span className="status-dot" />
          <span className="status-text">
            {connected ? 'Live' : 'Disconnected'}
          </span>
        </div>
        {lastUpdate && (
          <span className="last-update">
            {lastUpdate.toLocaleTimeString()}
          </span>
        )}
      </div>
    </header>
  )
}
