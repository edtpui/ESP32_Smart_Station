import { useCallback, useEffect, useRef, useState } from "react"
import { toast } from "sonner"

import { api, describeError, type StationHistory, type StationStatus } from "@/lib/api"

const STATUS_POLL_MS = 2000
const HISTORY_POLL_MS = 12000

export type Connection = "connecting" | "online" | "offline"

export function useStation() {
  const [status, setStatus] = useState<StationStatus | null>(null)
  const [history, setHistory] = useState<StationHistory | null>(null)
  const [connection, setConnection] = useState<Connection>("connecting")
  // A poll that started before a change finished would briefly undo it on screen.
  const changeCount = useRef(0)

  useEffect(() => {
    let stopped = false
    let statusTimer: ReturnType<typeof setTimeout>
    let historyTimer: ReturnType<typeof setTimeout>

    // Each poll waits for the previous one: the ESP32 serves one request at a time.
    async function pollStatus() {
      if (!document.hidden) {
        const startedAt = changeCount.current
        try {
          const next = await api.status()
          if (!stopped && startedAt === changeCount.current) setStatus(next)
          if (!stopped) setConnection("online")
        } catch {
          if (!stopped) setConnection("offline")
        }
      }
      if (!stopped) statusTimer = setTimeout(pollStatus, STATUS_POLL_MS)
    }

    async function pollHistory() {
      if (!document.hidden) {
        try {
          const next = await api.history()
          if (!stopped) setHistory(next)
        } catch {
          // pollStatus reports connection problems; the chart keeps its last data.
        }
      }
      if (!stopped) historyTimer = setTimeout(pollHistory, HISTORY_POLL_MS)
    }

    pollStatus()
    pollHistory()
    return () => {
      stopped = true
      clearTimeout(statusTimer)
      clearTimeout(historyTimer)
    }
  }, [])

  // Runs a change on the station and shows the state it reports back.
  const change = useCallback(async (action: () => Promise<StationStatus>, successMessage?: string) => {
    changeCount.current += 1
    try {
      setStatus(await action())
      setConnection("online")
      if (successMessage) toast.success(successMessage)
    } catch (error) {
      toast.error(describeError(error))
    }
  }, [])

  return { status, history, connection, change }
}
