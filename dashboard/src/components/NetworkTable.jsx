import { useState } from 'react'
import './NetworkTable.css'

function stateClass(state) {
  if (state === 'ESTABLISHED') return 'state-established'
  if (state === 'LISTEN') return 'state-listen'
  if (state === 'TIME_WAIT' || state === 'CLOSE_WAIT') return 'state-closing'
  return ''
}

export default function NetworkTable({ connections }) {
  const [filter, setFilter] = useState('all')
  const [limit, setLimit] = useState(20)

  const filtered = connections.filter((c) => {
    if (filter === 'all') return true
    if (filter === 'tcp') return c.proto === 'TCP'
    if (filter === 'udp') return c.proto === 'UDP'
    if (filter === 'established') return c.state === 'ESTABLISHED'
    return true
  })

  const shown = filtered.slice(0, limit)

  return (
    <div className="card net-card">
      <div className="card-header">
        <h2 className="card-title">Network ({filtered.length})</h2>
        <div className="card-actions">
          {['all', 'tcp', 'udp', 'established'].map((f) => (
            <button
              key={f}
              className={`sort-btn ${filter === f ? 'active' : ''}`}
              onClick={() => { setFilter(f); setLimit(20); }}
            >
              {f.toUpperCase()}
            </button>
          ))}
        </div>
      </div>
      <div className="table-wrap">
        <table className="data-table net-table">
          <thead>
            <tr>
              <th>Proto</th>
              <th>Local</th>
              <th>Remote</th>
              <th>State</th>
            </tr>
          </thead>
          <tbody>
            {shown.map((c, i) => (
              <tr key={i}>
                <td className="mono proto-cell">{c.proto}</td>
                <td className="mono">{c.local_addr}:{c.local_port}</td>
                <td className="mono">{c.remote_addr}:{c.remote_port}</td>
                <td className={`mono ${stateClass(c.state)}`}>{c.state}</td>
              </tr>
            ))}
          </tbody>
        </table>
      </div>
      {filtered.length > limit && (
        <div className="show-more">
          <button onClick={() => setLimit(l => l + 20)}>
            Show more ({filtered.length - limit} remaining)
          </button>
        </div>
      )}
    </div>
  )
}
