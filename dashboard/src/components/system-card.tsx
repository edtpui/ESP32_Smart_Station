import { InfoRow } from "@/components/setting-row"
import {
  AlertDialog,
  AlertDialogAction,
  AlertDialogCancel,
  AlertDialogContent,
  AlertDialogDescription,
  AlertDialogFooter,
  AlertDialogHeader,
  AlertDialogTitle,
  AlertDialogTrigger,
} from "@/components/ui/alert-dialog"
import { Button } from "@/components/ui/button"
import { Card, CardContent, CardDescription, CardFooter, CardHeader, CardTitle } from "@/components/ui/card"
import type { StationStatus } from "@/lib/api"
import { formatClock, formatSpan } from "@/lib/format"

function ConfirmButton({
  label,
  title,
  description,
  onConfirm,
}: {
  label: string
  title: string
  description: string
  onConfirm: () => void
}) {
  return (
    <AlertDialog>
      <AlertDialogTrigger asChild>
        <Button variant="outline" size="sm">
          {label}
        </Button>
      </AlertDialogTrigger>
      <AlertDialogContent>
        <AlertDialogHeader>
          <AlertDialogTitle>{title}</AlertDialogTitle>
          <AlertDialogDescription>{description}</AlertDialogDescription>
        </AlertDialogHeader>
        <AlertDialogFooter>
          <AlertDialogCancel>Cancel</AlertDialogCancel>
          <AlertDialogAction onClick={onConfirm}>{label}</AlertDialogAction>
        </AlertDialogFooter>
      </AlertDialogContent>
    </AlertDialog>
  )
}

export function SystemCard({
  status,
  onResetStats,
  onFactoryReset,
}: {
  status: StationStatus
  onResetStats: () => void
  onFactoryReset: () => void
}) {
  const { climate, motion, fan, system, wifi, fw, remote, light } = status
  const keyCode = `0x${remote.lastKey.toString(16).toUpperCase().padStart(2, "0")}`

  return (
    <Card>
      <CardHeader>
        <CardTitle>System</CardTitle>
        <CardDescription>Health and counters since the last reset.</CardDescription>
      </CardHeader>
      <CardContent className="flex-1">
        <dl className="space-y-2">
          <InfoRow label="Fan on-time">{formatClock(fan.runtime)}</InfoRow>
          <InfoRow label="Motion events">{motion.events}</InfoRow>
          <InfoRow label={`DHT${climate.sensor} reads`}>
            {climate.goodReads} ok, {climate.failedReads} failed
          </InfoRow>
          <InfoRow label="Joystick">{system.stickOk ? "Detected" : "Not detected"}</InfoRow>
          <InfoRow label="Remote">
            {remote.lastKey < 0 ? "No key received yet" : `Key ${keyCode}, ${formatSpan(remote.lastKeyAgo)} ago`}
          </InfoRow>
          <InfoRow label="Light sensor">{light.raw} of 4095</InfoRow>
          <InfoRow label="Free memory">{Math.round(system.freeHeap / 1024)} kB</InfoRow>
          <InfoRow label="Firmware">v{fw}</InfoRow>
          <InfoRow label="MAC address">{wifi.mac}</InfoRow>
        </dl>
      </CardContent>
      <CardFooter className="flex-wrap gap-2">
        <ConfirmButton
          label="Reset stats"
          title="Reset the stats?"
          description="Clears the low and high readings, the motion count and the fan on-time. Settings stay as they are."
          onConfirm={onResetStats}
        />
        <ConfirmButton
          label="Restore defaults"
          title="Restore default settings?"
          description="Fan mode, speed, hold time, gate and screen options go back to their defaults. Saved Wi-Fi stays."
          onConfirm={onFactoryReset}
        />
      </CardFooter>
    </Card>
  )
}
