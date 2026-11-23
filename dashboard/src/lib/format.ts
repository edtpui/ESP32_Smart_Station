import type { StationStatus } from "@/lib/api"

export function toDisplayTemp(tempC: number, fahrenheit: boolean): number {
  return fahrenheit ? (tempC * 9) / 5 + 32 : tempC
}

export function formatTemp(tempC: number | null, fahrenheit: boolean): string {
  if (tempC === null) return "--"
  return `${toDisplayTemp(tempC, fahrenheit).toFixed(1)} °${fahrenheit ? "F" : "C"}`
}

export function formatHumidity(humidity: number | null): string {
  return humidity === null ? "--" : `${humidity.toFixed(0)} %`
}

// "02:03:04", or "1d 02:03:04" past a day. Matches the OLED's Info page.
export function formatClock(totalSeconds: number): string {
  const days = Math.floor(totalSeconds / 86400)
  const pad = (n: number) => String(n).padStart(2, "0")
  const clock = `${pad(Math.floor(totalSeconds / 3600) % 24)}:${pad(Math.floor(totalSeconds / 60) % 60)}:${pad(totalSeconds % 60)}`
  return days > 0 ? `${days}d ${clock}` : clock
}

// "45 s", "2 min", "1 min 30 s"
export function formatSpan(totalSeconds: number): string {
  if (totalSeconds < 60) return `${totalSeconds} s`
  const minutes = Math.floor(totalSeconds / 60)
  const seconds = totalSeconds % 60
  return seconds === 0 ? `${minutes} min` : `${minutes} min ${seconds} s`
}

export function signalQuality(rssi: number): { bars: number; label: string } {
  if (rssi >= -55) return { bars: 4, label: "Excellent" }
  if (rssi >= -67) return { bars: 3, label: "Good" }
  if (rssi >= -78) return { bars: 2, label: "Fair" }
  return { bars: 1, label: "Weak" }
}

// Why the fan is doing what it is doing right now.
export function fanReasonText(status: StationStatus): string {
  const { fan, motion, settings } = status
  switch (fan.reason) {
    case "warmup":
      return `Motion sensor warming up, ${motion.warmupLeft} s left`
    case "motion":
      return "Motion detected"
    case "hold":
      return `No motion, stops in ${formatSpan(fan.holdLeft)}`
    case "idle":
      return "Waiting for motion"
    case "toocool":
      return `Motion detected, but below the ${formatTemp(settings.gateTempC, settings.fahrenheit)} gate`
    case "toggledon":
      return "Switched on by motion. The next motion switches it off."
    case "toggledoff":
      return "Switched off by motion. The next motion switches it on."
    case "forcedon":
      return "Manual override: always on"
    case "forcedoff":
      return "Manual override: always off"
  }
}
