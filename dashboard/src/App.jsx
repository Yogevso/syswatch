import { useSysWatch } from './useSysWatch'
import Header from './components/Header'
import StatsBar from './components/StatsBar'
import ProcessTable from './components/ProcessTable'
import NetworkTable from './components/NetworkTable'
import AlertPanel from './components/AlertPanel'
import './App.css'

export default function App() {
  const { data, connected, lastUpdate } = useSysWatch()

  return (
    <div className="app">
      <Header connected={connected} lastUpdate={lastUpdate} />

      {!data ? (
        <div className="loading">
          <div className="loading-spinner" />
          <p>Waiting for SysWatch data...</p>
          {!connected && <p className="loading-hint">Connecting to backend on port 3001...</p>}
        </div>
      ) : (
        <>
          <StatsBar data={data} />
          <div className="panels">
            <div className="panel-main">
              <ProcessTable processes={data.processes || []} />
            </div>
            <div className="panel-side">
              <AlertPanel alerts={data.alerts || []} />
              <NetworkTable connections={data.connections || []} />
            </div>
          </div>
        </>
      )}
    </div>
  )
}
