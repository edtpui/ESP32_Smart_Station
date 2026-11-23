import type { ReactNode } from "react"
import { ActivityIcon, DropletsIcon, FanIcon, MoonIcon, SunIcon, ThermometerIcon, type LucideIcon } from "lucide-react"

import { Badge } from "@/components/ui/badge"
import { Card, CardAction, CardContent, CardDescription, CardHeader, CardTitle } from "@/components/ui/card"
import { Progress } from "@/components/ui/progress"
import type { StationStatus } from "@/lib/api"
import { fanReasonText, formatHumidity, formatSpan, formatTemp } from "@/lib/format"

const MODE_LABELS = { auto: "Auto", toggle: "Toggle", on: "On", off: "Off" } as const

function StatCard({
  icon: Icon,
  label,
  value,
  badge,
  children,
}: {
  icon: LucideIcon
  label: string
  value: string
  badge?: ReactNode
  children: ReactNode
}) {
  return (
    <Card className="gap-2 py-4">
      <CardHeader className="gap-1 px-4">
        <CardDescription className="flex items-center gap-1.5">
          <Icon className="size-4" aria-hidden />
          {label}
        </CardDescription>
        <CardTitle className="text-2xl tabular-nums">{value}</CardTitle>
        {badge && <CardAction>{badge}</CardAction>}
      </CardHeader>
      <CardContent className="space-y-2 px-4 text-xs text-muted-foreground">{children}</CardContent>
    </Card>
  )
}

export function StatusCards({ status }: { status: StationStatus }) {
  const { climate, motion, fan, settings, light, alerts } = status
  const sensorName = `DHT${climate.sensor}`
  const noReading = `No reading from the ${sensorName}. Check its wiring.`

  const motionValue = !motion.ready ? "Warming up" : motion.active ? "Detected" : "Clear"
  const motionDetail = !motion.ready
    ? `Ready in ${motion.warmupLeft} s`
    : motion.lastMotionAgo < 0
      ? "No motion seen yet"
      : motion.active
        ? "Moving right now"
        : `Last seen ${formatSpan(motion.lastMotionAgo)} ago`

  return (
    <section aria-label="Current readings" className="grid gap-4 sm:grid-cols-2 lg:grid-cols-5">
      <StatCard
        icon={ThermometerIcon}
        label="Temperature"
        value={formatTemp(climate.tempC, settings.fahrenheit)}
        badge={alerts.tempHigh && <Badge variant="destructive">Too hot</Badge>}
      >
        {climate.valid ? (
          <p>
            Low {formatTemp(climate.minTempC, settings.fahrenheit)} · High{" "}
            {formatTemp(climate.maxTempC, settings.fahrenheit)}
          </p>
        ) : (
          <p className="text-destructive">{noReading}</p>
        )}
      </StatCard>

      <StatCard
        icon={DropletsIcon}
        label="Humidity"
        value={formatHumidity(climate.humidity)}
        badge={alerts.humidityHigh && <Badge variant="destructive">Too humid</Badge>}
      >
        {climate.valid ? (
          <p>
            Low {formatHumidity(climate.minHumidity)} · High {formatHumidity(climate.maxHumidity)}
          </p>
        ) : (
          <p className="text-destructive">{noReading}</p>
        )}
      </StatCard>

      <StatCard
        icon={ActivityIcon}
        label="Motion"
        value={motionValue}
        badge={
          <Badge variant={motion.active ? "default" : "outline"}>
            {motion.events} {motion.events === 1 ? "event" : "events"}
          </Badge>
        }
      >
        <p>{motionDetail}</p>
      </StatCard>

      <StatCard
        icon={light.dark ? MoonIcon : SunIcon}
        label="Light"
        value={`${light.percent} %`}
        badge={<Badge variant="outline">{light.dark ? "Dark" : "Lit"}</Badge>}
      >
        <Progress value={light.percent} aria-label="Light level" />
        <p>{light.dark && settings.autoDim ? "OLED dimmed for the dark" : "Photoresistor reading"}</p>
      </StatCard>

      <StatCard
        icon={FanIcon}
        label="Fan"
        value={fan.running ? `${settings.fanSpeed} %` : "Off"}
        badge={<Badge variant={fan.running ? "default" : "outline"}>{MODE_LABELS[settings.fanMode]}</Badge>}
      >
        <Progress value={(fan.duty / 255) * 100} aria-label="Motor power" />
        <p>{fanReasonText(status)}</p>
      </StatCard>
    </section>
  )
}
