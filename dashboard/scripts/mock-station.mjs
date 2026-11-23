// Stand-in for the ESP32, to try the dashboard without hardware: it serves the
// built page and answers the same API as WebDashboard.cpp with simulated sensors.
// Run with: npm run build && npm run mock    then open http://localhost:8787
import { readFileSync } from "node:fs"
import { createServer } from "node:http"
import { fileURLToPath } from "node:url"

const PORT = Number(process.env.PORT ?? 8787)
const HOLD_OPTIONS = [5, 10, 15, 30, 45, 60, 90, 120, 180, 300, 600]
const SLEEP_OPTIONS = [0, 15, 30, 60, 120, 300]
const INTERVAL_SEC = 12
const HISTORY_CAPACITY = 100
const WARMUP_SEC = 5
const MOTION_PERIOD_SEC = 45
const MOTION_ACTIVE_SEC = 8

const startedAt = Date.now()
const uptime = () => Math.floor((Date.now() - startedAt) / 1000)

const defaults = {
  fanMode: "auto",
  fanSpeed: 80,
  holdSeconds: 30,
  tempGateOn: false,
  gateTempC: 26,
  fahrenheit: false,
  screenSleep: 60,
  brightness: 2,
  flipDisplay: false,
  autoDim: true,
  soundOn: true,
  motionChime: false,
  climateAlert: true,
  alertTempC: 30,
  alertHumidity: 70,
}
const settings = { ...defaults }
const wifi = { ssid: "Mock Network", saved: false, enterprise: false }
let statsResetAt = 0
let scanPolls = 0

const tempAt = (t) => 23.5 + 1.4 * Math.sin(t / 240) + 0.2 * Math.sin(t / 17)
const humidityAt = (t) => 46 - 4 * Math.sin(t / 310) + Math.sin(t / 23)
const round1 = (value) => Math.round(value * 10) / 10
const lightAt = (t) => Math.round(Math.min(100, Math.max(0, 40 + 38 * Math.sin(t / 40))))

function motionState() {
  const t = uptime()
  if (t < WARMUP_SEC) return { ready: false, warmupLeft: WARMUP_SEC - t, active: false, events: 0, lastMotionAgo: -1 }
  const sinceReady = t - WARMUP_SEC
  const phase = sinceReady % MOTION_PERIOD_SEC
  const active = phase < MOTION_ACTIVE_SEC
  const total = Math.floor(sinceReady / MOTION_PERIOD_SEC) + 1
  const events = total - Math.floor(Math.max(statsResetAt - WARMUP_SEC, 0) / MOTION_PERIOD_SEC)
  return { ready: true, warmupLeft: 0, active, events, lastMotionAgo: active ? 0 : phase - MOTION_ACTIVE_SEC }
}

function fanState(motion) {
  const speedDuty = 90 + Math.floor((165 * (settings.fanSpeed - 10)) / 90)
  const result = (running, reason, holdLeft = 0) => ({
    running,
    duty: running ? speedDuty : 0,
    reason,
    holdLeft,
    runtime: Math.floor((uptime() - statsResetAt) / 3),
  })
  if (settings.fanMode === "on") return result(true, "forcedon")
  if (settings.fanMode === "off") return result(false, "forcedoff")
  if (!motion.ready) return result(false, "warmup")
  if (settings.fanMode === "toggle") {
    const on = motion.events % 2 === 1
    return result(on, on ? "toggledon" : "toggledoff")
  }
  const holding = !motion.active && motion.lastMotionAgo < settings.holdSeconds
  if (!motion.active && !holding) return result(false, "idle")
  if (settings.tempGateOn && tempAt(uptime()) < settings.gateTempC) return result(false, "toocool")
  return motion.active ? result(true, "motion") : result(true, "hold", settings.holdSeconds - motion.lastMotionAgo)
}

function statusJson() {
  const t = uptime()
  const tempC = round1(tempAt(t))
  const humidity = round1(humidityAt(t))
  const motion = motionState()
  return {
    name: "Smart Station",
    fw: "2.2.0",
    uptime: t,
    climate: {
      valid: true,
      tempC,
      humidity,
      minTempC: round1(Math.min(tempC, 22.1)),
      maxTempC: round1(Math.max(tempC, 24.9)),
      minHumidity: round1(Math.min(humidity, 41)),
      maxHumidity: round1(Math.max(humidity, 51)),
      sensor: 11,
      goodReads: Math.floor(t / 2.5),
      failedReads: 1,
    },
    motion,
    light: { percent: lightAt(t), raw: Math.round(lightAt(t) * 40.95), dark: lightAt(t) < 15 },
    alerts: {
      tempHigh: settings.climateAlert && tempC >= settings.alertTempC,
      humidityHigh: settings.climateAlert && humidity >= settings.alertHumidity,
    },
    remote: t < 20 ? { lastKey: -1, lastKeyAgo: -1, keys: 0 } : { lastKey: 0x45, lastKeyAgo: t % 20, keys: Math.floor(t / 20) },
    fan: fanState(motion),
    settings: { ...settings, holdOptions: HOLD_OPTIONS, sleepOptions: SLEEP_OPTIONS },
    wifi: {
      state: "connected",
      ssid: wifi.ssid,
      ip: "192.168.1.50",
      rssi: -58,
      hostname: "smartstation",
      mac: "24:6F:28:AA:1A:2B",
      apActive: false,
      apSsid: "SmartStation-1A2B",
      apIp: "",
      apClients: 0,
      saved: wifi.saved,
      enterprise: wifi.enterprise,
      error: "",
    },
    system: { freeHeap: 182000, stickOk: true },
  }
}

function historyJson() {
  const now = uptime()
  const samples = [...Array(HISTORY_CAPACITY).keys()].map((i) => now - (HISTORY_CAPACITY - 1 - i) * INTERVAL_SEC)
  const gap = (i) => i >= 62 && i < 65   // a few failed reads, like a real DHT11
  return {
    intervalSec: INTERVAL_SEC,
    capacity: HISTORY_CAPACITY,
    tempC: samples.map((t, i) => (gap(i) ? null : round1(tempAt(t)))),
    humidity: samples.map((t, i) => (gap(i) ? null : round1(humidityAt(t)))),
  }
}

const sendJson = (res, code, body) => {
  res.writeHead(code, { "Content-Type": "application/json", "Cache-Control": "no-store" })
  res.end(JSON.stringify(body))
}

function handlePost(path, args, res) {
  const has = (key) => args.has(key)
  const reject = (message) => sendJson(res, 400, { error: message })
  switch (path) {
    case "/api/fan":
      if (has("mode")) {
        if (!["auto", "toggle", "on", "off"].includes(args.get("mode"))) return reject("Unknown fan mode")
        settings.fanMode = args.get("mode")
      }
      if (has("speed")) {
        const speed = Number(args.get("speed"))
        if (!(speed >= 10 && speed <= 100)) return reject("Fan speed must be 10-100")
        settings.fanSpeed = speed
      }
      return sendJson(res, 200, statusJson())
    case "/api/settings":
      if (has("holdSeconds")) {
        if (!HOLD_OPTIONS.includes(Number(args.get("holdSeconds")))) return reject("Unsupported hold time")
        settings.holdSeconds = Number(args.get("holdSeconds"))
      }
      if (has("screenSleep")) {
        if (!SLEEP_OPTIONS.includes(Number(args.get("screenSleep")))) return reject("Unsupported screen sleep time")
        settings.screenSleep = Number(args.get("screenSleep"))
      }
      if (has("gateTempC")) {
        const gate = Math.round(Number(args.get("gateTempC")) * 2) / 2
        if (!(gate >= 10 && gate <= 40)) return reject("Gate temperature must be 10-40 C")
        settings.gateTempC = gate
      }
      if (has("brightness")) {
        const level = Number(args.get("brightness"))
        if (![0, 1, 2].includes(level)) return reject("Brightness must be 0-2")
        settings.brightness = level
      }
      if (has("alertTempC")) {
        const limit = Math.round(Number(args.get("alertTempC")) * 2) / 2
        if (!(limit >= 15 && limit <= 45)) return reject("Alert temperature must be 15-45 C")
        settings.alertTempC = limit
      }
      if (has("alertHumidity")) {
        const limit = Number(args.get("alertHumidity"))
        if (!(limit >= 30 && limit <= 95)) return reject("Alert humidity must be 30-95 %")
        settings.alertHumidity = limit
      }
      for (const key of ["tempGateOn", "fahrenheit", "flipDisplay", "autoDim", "soundOn", "motionChime", "climateAlert"]) {
        if (has(key)) settings[key] = args.get(key) === "1" || args.get(key) === "true"
      }
      return sendJson(res, 200, statusJson())
    case "/api/buzzer/test":
      return sendJson(res, 200, statusJson())
    case "/api/reset-stats":
      statsResetAt = uptime()
      return sendJson(res, 200, statusJson())
    case "/api/factory-reset":
      Object.assign(settings, defaults)
      return sendJson(res, 200, statusJson())
    case "/api/wifi": {
      const ssid = args.get("ssid") ?? ""
      const password = args.get("password") ?? ""
      const username = args.get("username") ?? ""
      if (ssid.length === 0 || ssid.length > 32) return reject("Network name must be 1-32 characters")
      if (username ? !password : password && (password.length < 8 || password.length > 63)) {
        return reject(username ? "A username needs a password" : "Wi-Fi passwords are 8-63 characters")
      }
      Object.assign(wifi, { ssid, saved: true, enterprise: Boolean(username) })
      return sendJson(res, 200, { ok: true })
    }
    case "/api/wifi/forget":
      Object.assign(wifi, { ssid: "Mock Network", saved: false, enterprise: false })
      return sendJson(res, 200, { ok: true })
    default:
      return sendJson(res, 404, { error: "Unknown API route" })
  }
}

createServer((req, res) => {
  const { pathname } = new URL(req.url, "http://localhost")
  if (req.method === "GET" && pathname === "/") {
    const html = readFileSync(fileURLToPath(new URL("../dist/index.html", import.meta.url)))
    res.writeHead(200, { "Content-Type": "text/html" })
    return res.end(html)
  }
  if (req.method === "GET" && pathname === "/api/status") return sendJson(res, 200, statusJson())
  if (req.method === "GET" && pathname === "/api/history") return sendJson(res, 200, historyJson())
  if (req.method === "GET" && pathname === "/api/wifi/scan") {
    if (scanPolls++ % 3 < 2) return sendJson(res, 200, { scanning: true })
    return sendJson(res, 200, {
      scanning: false,
      networks: [
        { ssid: "Mock Network", rssi: -58, secure: true, enterprise: false },
        { ssid: "eduroam", rssi: -66, secure: true, enterprise: true },
        { ssid: "Mock Network", rssi: -80, secure: true, enterprise: false },
        { ssid: "Cafe Guest", rssi: -79, secure: false, enterprise: false },
        { ssid: "", rssi: -70, secure: true, enterprise: false },
      ],
    })
  }
  if (req.method !== "POST") return sendJson(res, 404, { error: "Unknown API route" })
  if (!req.headers["x-station-client"]) return sendJson(res, 403, { error: "Missing client header" })

  let body = ""
  req.on("data", (chunk) => (body += chunk))
  req.on("end", () => {
    const args = new URLSearchParams(body)
    const logged = [...args.entries()].map(([k, v]) => `${k}=${k === "password" ? "***" : v}`).join("&")
    console.log(`POST ${pathname} ${logged}`)
    handlePost(pathname, args, res)
  })
}).listen(PORT, () => console.log(`Mock station at http://localhost:${PORT}`))
