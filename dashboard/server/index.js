/*
 * WebSocket bridge server for SysWatch dashboard.
 *
 * Spawns syswatch --json --once periodically and broadcasts
 * the JSON output to all connected WebSocket clients.
 */

const { WebSocketServer } = require("ws");
const { execFile } = require("child_process");
const path = require("path");

const PORT = parseInt(process.env.PORT || "3001", 10);
const INTERVAL = parseInt(process.env.SYSWATCH_INTERVAL || "2", 10) * 1000;
const SYSWATCH_BIN = process.env.SYSWATCH_BIN || "/usr/local/bin/syswatch";

const wss = new WebSocketServer({ port: PORT });
const clients = new Set();

wss.on("connection", (ws) => {
  clients.add(ws);
  console.log(`Client connected (total: ${clients.size})`);

  ws.on("close", () => {
    clients.delete(ws);
    console.log(`Client disconnected (total: ${clients.size})`);
  });

  ws.on("error", (err) => {
    console.error("WebSocket error:", err.message);
    clients.delete(ws);
  });
});

function broadcast(data) {
  const msg = typeof data === "string" ? data : JSON.stringify(data);
  for (const ws of clients) {
    if (ws.readyState === ws.OPEN) {
      ws.send(msg);
    }
  }
}

function collectData() {
  execFile(
    SYSWATCH_BIN,
    ["--all", "--json", "--once"],
    { timeout: 10000 },
    (err, stdout, stderr) => {
      if (err) {
        console.error("syswatch error:", err.message);
        if (stderr) console.error("stderr:", stderr);
        return;
      }

      try {
        // Validate it's proper JSON before broadcasting
        const parsed = JSON.parse(stdout);
        broadcast(JSON.stringify(parsed));
      } catch (parseErr) {
        console.error("JSON parse error:", parseErr.message);
      }
    }
  );
}

// Collect and broadcast on interval
setInterval(collectData, INTERVAL);

// Initial collection after short delay
setTimeout(collectData, 500);

console.log(`SysWatch bridge server running on ws://0.0.0.0:${PORT}`);
console.log(`Polling syswatch every ${INTERVAL / 1000}s`);
