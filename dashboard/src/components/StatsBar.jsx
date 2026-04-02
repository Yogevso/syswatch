import './StatsBar.css'

const STAT_ICONS = {
  processes: (
    <svg width="20" height="20" viewBox="0 0 20 20" fill="none">
      <rect x="2" y="3" width="6" height="6" rx="1.5" stroke="currentColor" strokeWidth="1.5"/>
      <rect x="12" y="3" width="6" height="6" rx="1.5" stroke="currentColor" strokeWidth="1.5"/>
      <rect x="2" y="11" width="6" height="6" rx="1.5" stroke="currentColor" strokeWidth="1.5"/>
      <rect x="12" y="11" width="6" height="6" rx="1.5" stroke="currentColor" strokeWidth="1.5"/>
    </svg>
  ),
  cpu: (
    <svg width="20" height="20" viewBox="0 0 20 20" fill="none">
      <path d="M3 14L7 8L10 11L14 5L17 9" stroke="currentColor" strokeWidth="1.5" strokeLinecap="round" strokeLinejoin="round"/>
    </svg>
  ),
  memory: (
    <svg width="20" height="20" viewBox="0 0 20 20" fill="none">
      <rect x="4" y="2" width="12" height="16" rx="2" stroke="currentColor" strokeWidth="1.5"/>
      <line x1="4" y1="7" x2="16" y2="7" stroke="currentColor" strokeWidth="1.5"/>
      <circle cx="10" cy="13" r="2" stroke="currentColor" strokeWidth="1.5"/>
    </svg>
  ),
  network: (
    <svg width="20" height="20" viewBox="0 0 20 20" fill="none">
      <circle cx="10" cy="4" r="2" stroke="currentColor" strokeWidth="1.5"/>
      <circle cx="4" cy="16" r="2" stroke="currentColor" strokeWidth="1.5"/>
      <circle cx="16" cy="16" r="2" stroke="currentColor" strokeWidth="1.5"/>
      <path d="M10 6V10L4 14M10 10L16 14" stroke="currentColor" strokeWidth="1.5" strokeLinecap="round"/>
    </svg>
  ),
  alerts: (
    <svg width="20" height="20" viewBox="0 0 20 20" fill="none">
      <path d="M10 3L18 17H2L10 3Z" stroke="currentColor" strokeWidth="1.5" strokeLinejoin="round"/>
      <line x1="10" y1="9" x2="10" y2="12" stroke="currentColor" strokeWidth="1.5" strokeLinecap="round"/>
      <circle cx="10" cy="14.5" r="0.75" fill="currentColor"/>
    </svg>
  ),
}

export default function StatsBar({ data }) {
  const totalProcs = data.total_processes || 0
  const totalConns = data.total_connections || 0
  const alertCount = data.alerts?.length || 0

  const topCpu = data.processes?.length > 0
    ? data.processes.reduce((max, p) => p.cpu > max ? p.cpu : max, 0)
    : 0

  const totalMem = data.processes?.reduce((sum, p) => sum + (p.mem_kb || 0), 0) || 0
  const totalMemMB = Math.round(totalMem / 1024)

  const stats = [
    { icon: STAT_ICONS.processes, value: totalProcs, label: 'Processes', color: 'cyan' },
    { icon: STAT_ICONS.cpu, value: `${topCpu.toFixed(1)}%`, label: 'Peak CPU', color: topCpu >= 80 ? 'red' : topCpu >= 40 ? 'yellow' : 'green' },
    { icon: STAT_ICONS.memory, value: `${totalMemMB} MB`, label: 'Total RSS', color: 'purple' },
    { icon: STAT_ICONS.network, value: totalConns, label: 'Connections', color: totalConns >= 100 ? 'red' : 'blue' },
    { icon: STAT_ICONS.alerts, value: alertCount, label: 'Alerts', color: alertCount > 0 ? 'red' : 'green' },
  ]

  return (
    <div className="stats-bar">
      {stats.map((s, i) => (
        <div key={i} className={`stat-card stat-${s.color}`}>
          <div className="stat-icon">{s.icon}</div>
          <div className="stat-content">
            <div className="stat-value">{s.value}</div>
            <div className="stat-label">{s.label}</div>
          </div>
        </div>
      ))}
    </div>
  )
}
