import './AlertPanel.css'

const TYPE_ICONS = {
  high_cpu: (
    <svg width="16" height="16" viewBox="0 0 16 16" fill="none">
      <path d="M2 11L5 5L8 8L11 3L14 7" stroke="currentColor" strokeWidth="1.5" strokeLinecap="round" strokeLinejoin="round"/>
    </svg>
  ),
  high_mem: (
    <svg width="16" height="16" viewBox="0 0 16 16" fill="none">
      <rect x="3" y="1" width="10" height="14" rx="2" stroke="currentColor" strokeWidth="1.5"/>
      <line x1="3" y1="5" x2="13" y2="5" stroke="currentColor" strokeWidth="1.5"/>
    </svg>
  ),
  conn_spike: (
    <svg width="16" height="16" viewBox="0 0 16 16" fill="none">
      <path d="M8 1L10 6H14L11 9L12 14L8 11L4 14L5 9L2 6H6L8 1Z" stroke="currentColor" strokeWidth="1.3" strokeLinejoin="round"/>
    </svg>
  ),
  suspicious_port: (
    <svg width="16" height="16" viewBox="0 0 16 16" fill="none">
      <path d="M8 2L15 14H1L8 2Z" stroke="currentColor" strokeWidth="1.5" strokeLinejoin="round"/>
      <line x1="8" y1="7" x2="8" y2="10" stroke="currentColor" strokeWidth="1.5" strokeLinecap="round"/>
      <circle cx="8" cy="12" r="0.75" fill="currentColor"/>
    </svg>
  ),
}

const TYPE_CLASSES = {
  high_cpu: 'alert-cpu',
  high_mem: 'alert-mem',
  conn_spike: 'alert-conn',
  suspicious_port: 'alert-suspicious',
}

export default function AlertPanel({ alerts }) {
  return (
    <div className="card alert-card">
      <div className="card-header">
        <h2 className="card-title">
          Alerts
          {alerts.length > 0 && <span className="alert-badge">{alerts.length}</span>}
        </h2>
      </div>
      <div className="alert-list">
        {alerts.length === 0 ? (
          <div className="alert-empty">
            <div className="alert-empty-icon">
              <svg width="28" height="28" viewBox="0 0 28 28" fill="none">
                <circle cx="14" cy="14" r="12" stroke="currentColor" strokeWidth="1.5"/>
                <path d="M9 14.5L12 17.5L19 10.5" stroke="currentColor" strokeWidth="2" strokeLinecap="round" strokeLinejoin="round"/>
              </svg>
            </div>
            <span>No anomalies detected</span>
          </div>
        ) : (
          alerts.map((a, i) => (
            <div key={i} className={`alert-item ${TYPE_CLASSES[a.type] || ''}`} style={{ animationDelay: `${i * 50}ms` }}>
              <span className="alert-icon">{TYPE_ICONS[a.type] || TYPE_ICONS.suspicious_port}</span>
              <span className="alert-msg">{a.message}</span>
            </div>
          ))
        )}
      </div>
    </div>
  )
}
