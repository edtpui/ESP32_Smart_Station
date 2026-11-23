import { useMemo, useState } from "react"
import { Area, AreaChart, CartesianGrid, XAxis, YAxis } from "recharts"

import { Card, CardAction, CardContent, CardDescription, CardHeader, CardTitle } from "@/components/ui/card"
import { ChartContainer, ChartTooltip, ChartTooltipContent, type ChartConfig } from "@/components/ui/chart"
import { ToggleGroup, ToggleGroupItem } from "@/components/ui/toggle-group"
import type { StationHistory } from "@/lib/api"
import { formatSpan, toDisplayTemp } from "@/lib/format"

type Metric = "temperature" | "humidity"

const CHART_CONFIG = {
  temperature: { label: "Temperature", color: "var(--chart-1)" },
  humidity: { label: "Humidity", color: "var(--chart-2)" },
} satisfies ChartConfig

// Keeps sensor noise from looking like a cliff. Same spans as the OLED's graph.
const MIN_SPAN: Record<Metric, number> = { temperature: 2, humidity: 6 }

export function ClimateChart({
  history,
  fahrenheit,
  className,
}: {
  history: StationHistory | null
  fahrenheit: boolean
  className?: string
}) {
  const [metric, setMetric] = useState<Metric>("temperature")
  const unit = metric === "humidity" ? "%" : fahrenheit ? "°F" : "°C"

  const { data, ticks, windowMinutes } = useMemo(() => {
    const samples = (metric === "temperature" ? history?.tempC : history?.humidity) ?? []
    const interval = history?.intervalSec ?? 12
    const data = samples.map((sample, i) => ({
      minutesAgo: (-(samples.length - 1 - i) * interval) / 60,
      [metric]: sample === null || metric === "humidity" ? sample : toDisplayTemp(sample, fahrenheit),
    }))
    const values = data.map((point) => point[metric]).filter((value): value is number => value !== null)
    const low = Math.min(...values)
    const high = Math.max(...values)
    const pad = Math.max((MIN_SPAN[metric] - (high - low)) / 2, 0.5)
    // Evenly spaced whole-number ticks, so the gridlines can be read as a scale.
    const step = Math.max(1, Math.ceil((high - low + 2 * pad) / 4))
    const ticks: number[] = []
    for (let tick = Math.floor((low - pad) / step) * step; values.length > 0; tick += step) {
      ticks.push(tick)
      if (tick >= high + pad) break
    }
    return {
      data,
      ticks,
      windowMinutes: Math.round(((history?.capacity ?? 100) * interval) / 60),
    }
  }, [history, metric, fahrenheit])

  const hasData = data.filter((point) => point[metric] !== null).length >= 2

  return (
    <Card className={className}>
      <CardHeader>
        <CardTitle>Climate trend</CardTitle>
        <CardDescription>
          Last {windowMinutes} minutes, one sample every {history?.intervalSec ?? 12} s
        </CardDescription>
        <CardAction>
          <ToggleGroup
            type="single"
            variant="outline"
            size="sm"
            value={metric}
            aria-label="Chart metric"
            onValueChange={(value) => value && setMetric(value as Metric)}
          >
            <ToggleGroupItem value="temperature">Temperature</ToggleGroupItem>
            <ToggleGroupItem value="humidity">Humidity</ToggleGroupItem>
          </ToggleGroup>
        </CardAction>
      </CardHeader>
      <CardContent className="px-2 sm:px-6">
        {hasData ? (
          <ChartContainer config={CHART_CONFIG} className="aspect-auto h-64 w-full">
            <AreaChart accessibilityLayer data={data} margin={{ left: 0, right: 8, top: 8, bottom: 0 }}>
              <CartesianGrid vertical={false} />
              <XAxis
                dataKey="minutesAgo"
                type="number"
                domain={[-windowMinutes, 0]}
                ticks={[-windowMinutes, (-windowMinutes * 3) / 4, -windowMinutes / 2, -windowMinutes / 4, 0]}
                tickLine={false}
                axisLine={false}
                tickMargin={8}
                tickFormatter={(minutes: number) => (minutes === 0 ? "now" : `${-minutes} min`)}
              />
              <YAxis
                domain={[ticks[0], ticks[ticks.length - 1]]}
                ticks={ticks}
                tickLine={false}
                axisLine={false}
                width={44}
                tickFormatter={(value: number) => `${value}${metric === "humidity" ? "%" : "°"}`}
              />
              <ChartTooltip
                content={
                  <ChartTooltipContent
                    labelFormatter={(_, payload) => {
                      const seconds = Math.round(-(payload?.[0]?.payload?.minutesAgo ?? 0) * 60)
                      return seconds === 0 ? "Now" : `${formatSpan(seconds)} ago`
                    }}
                    formatter={(value) => (
                      <span className="font-mono font-medium tabular-nums">
                        {Number(value).toFixed(metric === "humidity" ? 0 : 1)} {unit}
                      </span>
                    )}
                  />
                }
              />
              <Area
                dataKey={metric}
                type="linear"
                stroke={`var(--color-${metric})`}
                strokeWidth={2}
                fill={`var(--color-${metric})`}
                fillOpacity={0.12}
                connectNulls={false}
                isAnimationActive={false}
              />
            </AreaChart>
          </ChartContainer>
        ) : (
          <p className="flex h-64 items-center justify-center text-center text-sm text-muted-foreground">
            Collecting samples. The chart fills in over the next few minutes.
          </p>
        )}
      </CardContent>
    </Card>
  )
}
