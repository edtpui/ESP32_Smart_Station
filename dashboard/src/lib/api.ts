// Client for the JSON API in ESP32_Smart_Station/WebDashboard.cpp.

export type FanMode = "auto" | "toggle" | "on" | "off"
export type FanReason =
  | "warmup"
  | "motion"
  | "hold"
  | "idle"
  | "toocool"
  | "toggledon"
  | "toggledoff"
  | "forcedon"
  | "forcedoff"

export interface StationSettings {
  fanMode: FanMode
  fanSpeed: number
  holdSeconds: number
  holdOptions: number[]
  tempGateOn: boolean
  gateTempC: number
  fahrenheit: boolean
  screenSleep: number
  sleepOptions: number[]
  brightness: number
  flipDisplay: boolean
  autoDim: boolean
  soundOn: boolean
  motionChime: boolean
  climateAlert: boolean
  alertTempC: number
  alertHumidity: number
}

export interface StationStatus {
  name: string
  fw: string
  uptime: number
  climate: {
    valid: boolean
    tempC: number | null
    humidity: number | null
    minTempC: number | null
    maxTempC: number | null
    minHumidity: number | null
    maxHumidity: number | null
    sensor: number
    goodReads: number
    failedReads: number
  }
  motion: {
    ready: boolean
    warmupLeft: number
    active: boolean
    events: number
    lastMotionAgo: number
  }
  light: {
    percent: number
    raw: number
    dark: boolean
  }
  alerts: {
    tempHigh: boolean
    humidityHigh: boolean
  }
  remote: {
    lastKey: number
    lastKeyAgo: number
    keys: number
  }
  fan: {
    running: boolean
    duty: number
    reason: FanReason
    holdLeft: number
    runtime: number
  }
  settings: StationSettings
  wifi: {
    state: "connecting" | "connected" | "hotspot"
    ssid: string
    ip: string
    rssi: number
    hostname: string
    mac: string
    apActive: boolean
    apSsid: string
    apIp: string
    apClients: number
    saved: boolean
    enterprise: boolean
    error: string
  }
  system: {
    freeHeap: number
    stickOk: boolean
  }
}

export interface StationHistory {
  intervalSec: number
  capacity: number
  tempC: (number | null)[]
  humidity: (number | null)[]
}

export interface WifiNetwork {
  ssid: string
  rssi: number
  secure: boolean
  enterprise: boolean
}

export interface WifiScan {
  scanning: boolean
  networks?: WifiNetwork[]
}

export interface WifiCredentials {
  ssid: string
  password: string
  username: string
  identity: string
}

export type SettingsUpdate = Partial<
  Pick<
    StationSettings,
    | "holdSeconds"
    | "tempGateOn"
    | "gateTempC"
    | "fahrenheit"
    | "screenSleep"
    | "brightness"
    | "flipDisplay"
    | "autoDim"
    | "soundOn"
    | "motionChime"
    | "climateAlert"
    | "alertTempC"
    | "alertHumidity"
  >
>
export type FanUpdate = { mode?: FanMode; speed?: number }

const REQUEST_TIMEOUT_MS = 6000

// An answer from the station whose message is meant for the user.
class StationError extends Error {}

export function describeError(error: unknown): string {
  return error instanceof StationError ? error.message : "The station didn't respond"
}

async function request<T>(path: string, init?: RequestInit): Promise<T> {
  const controller = new AbortController()
  const timer = setTimeout(() => controller.abort(), REQUEST_TIMEOUT_MS)
  try {
    const response = await fetch(path, { ...init, signal: controller.signal })
    const body = await response.json().catch(() => null)
    if (!response.ok) {
      throw new StationError(body?.error ?? `The station answered with HTTP ${response.status}`)
    }
    return body as T
  } finally {
    clearTimeout(timer)
  }
}

function post<T>(path: string, fields: Record<string, string | number | boolean> = {}): Promise<T> {
  const body = new URLSearchParams()
  for (const [key, value] of Object.entries(fields)) body.set(key, String(value))
  return request<T>(path, {
    method: "POST",
    headers: {
      "Content-Type": "application/x-www-form-urlencoded",
      "X-Station-Client": "dashboard",
    },
    body,
  })
}

export const api = {
  status: () => request<StationStatus>("/api/status"),
  history: () => request<StationHistory>("/api/history"),
  setFan: (update: FanUpdate) => post<StationStatus>("/api/fan", update),
  setSettings: (update: SettingsUpdate) => post<StationStatus>("/api/settings", update),
  resetStats: () => post<StationStatus>("/api/reset-stats"),
  testBuzzer: () => post<StationStatus>("/api/buzzer/test"),
  factoryReset: () => post<StationStatus>("/api/factory-reset"),
  scanWifi: () => request<WifiScan>("/api/wifi/scan"),
  saveWifi: (credentials: WifiCredentials) => post<{ ok: boolean }>("/api/wifi", { ...credentials }),
  forgetWifi: () => post<{ ok: boolean }>("/api/wifi/forget"),
}
