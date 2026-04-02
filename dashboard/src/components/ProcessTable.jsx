import { useState } from 'react'
import './ProcessTable.css'

function formatMem(kb) {
  if (kb >= 1024) return `${Math.round(kb / 1024)} MB`
  return `${kb} KB`
}

function cpuClass(cpu) {
  if (cpu >= 80) return 'cpu-high'
  if (cpu >= 40) return 'cpu-medium'
  return 'cpu-low'
}

export default function ProcessTable({ processes }) {
  const [sortBy, setSortBy] = useState('cpu')
  const [limit, setLimit] = useState(25)

  const sorted = [...processes].sort((a, b) => {
    if (sortBy === 'cpu') return b.cpu - a.cpu
    if (sortBy === 'mem') return b.mem_kb - a.mem_kb
    if (sortBy === 'conns') return (b.connections || 0) - (a.connections || 0)
    return 0
  })

  const shown = sorted.slice(0, limit)

  return (
    <div className="card">
      <div className="card-header">
        <h2 className="card-title">Processes</h2>
        <div className="card-actions">
          <span className="sort-label">Sort:</span>
          <button className={`sort-btn ${sortBy === 'cpu' ? 'active' : ''}`} onClick={() => setSortBy('cpu')}>CPU</button>
          <button className={`sort-btn ${sortBy === 'mem' ? 'active' : ''}`} onClick={() => setSortBy('mem')}>MEM</button>
          <button className={`sort-btn ${sortBy === 'conns' ? 'active' : ''}`} onClick={() => setSortBy('conns')}>CONNS</button>
        </div>
      </div>
      <div className="table-wrap">
        <table className="data-table">
          <thead>
            <tr>
              <th>PID</th>
              <th>Name</th>
              <th className="num">CPU %</th>
              <th className="num">Memory</th>
              <th className="num">Conns</th>
            </tr>
          </thead>
          <tbody>
            {shown.map((p) => (
              <tr key={p.pid}>
                <td className="mono">{p.pid}</td>
                <td className="proc-name">{p.name}</td>
                <td className="num">
                  <span className={cpuClass(p.cpu)}>{p.cpu.toFixed(1)}%</span>
                  <div className="cpu-bar">
                    <div className={`cpu-fill ${cpuClass(p.cpu)}-bg`} style={{ width: `${Math.min(p.cpu, 100)}%` }} />
                  </div>
                </td>
                <td className="num mono">{formatMem(p.mem_kb)}</td>
                <td className="num mono">{p.connections || 0}</td>
              </tr>
            ))}
          </tbody>
        </table>
      </div>
      {processes.length > limit && (
        <div className="show-more">
          <button onClick={() => setLimit(l => l + 25)}>
            Show more ({processes.length - limit} remaining)
          </button>
        </div>
      )}
    </div>
  )
}
