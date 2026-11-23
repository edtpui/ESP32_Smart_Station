import { SettingRow, SliderRow } from "@/components/setting-row"
import { Card, CardContent, CardDescription, CardHeader, CardTitle } from "@/components/ui/card"
import { Select, SelectContent, SelectItem, SelectTrigger, SelectValue } from "@/components/ui/select"
import { Separator } from "@/components/ui/separator"
import { Switch } from "@/components/ui/switch"
import { ToggleGroup, ToggleGroupItem } from "@/components/ui/toggle-group"
import type { FanMode, FanUpdate, SettingsUpdate, StationSettings } from "@/lib/api"
import { formatSpan, formatTemp } from "@/lib/format"

const MODES: { value: FanMode; label: string; description: string }[] = [
  { value: "auto", label: "Auto", description: "Runs while motion is detected, then for the hold time." },
  { value: "toggle", label: "Toggle", description: "Each new motion event switches the fan on or off." },
  { value: "on", label: "On", description: "Manual override. The motion sensor is ignored." },
  { value: "off", label: "Off", description: "Manual override. The motion sensor is ignored." },
]

export function FanCard({
  settings,
  onFan,
  onSettings,
  className,
}: {
  settings: StationSettings
  onFan: (update: FanUpdate) => Promise<void>
  onSettings: (update: SettingsUpdate) => Promise<void>
  className?: string
}) {
  const mode = MODES.find((candidate) => candidate.value === settings.fanMode) ?? MODES[0]
  const auto = settings.fanMode === "auto"

  return (
    <Card className={className}>
      <CardHeader>
        <CardTitle>Fan control</CardTitle>
        <CardDescription>{mode.description}</CardDescription>
      </CardHeader>
      <CardContent className="space-y-5">
        <ToggleGroup
          type="single"
          variant="outline"
          value={settings.fanMode}
          aria-label="Fan mode"
          className="w-full"
          onValueChange={(value) => value && onFan({ mode: value as FanMode })}
        >
          {MODES.map(({ value, label }) => (
            <ToggleGroupItem key={value} value={value} className="flex-1">
              {label}
            </ToggleGroupItem>
          ))}
        </ToggleGroup>

        <SliderRow
          label="Speed"
          value={settings.fanSpeed}
          format={(value) => `${value} %`}
          min={10}
          max={100}
          step={10}
          onCommit={(speed) => onFan({ speed })}
        />

        <Separator />

        <SettingRow id="hold-time" label="Hold time" hint="How long it keeps running after motion stops (Auto mode)">
          <Select
            value={String(settings.holdSeconds)}
            disabled={!auto}
            onValueChange={(value) => onSettings({ holdSeconds: Number(value) })}
          >
            <SelectTrigger id="hold-time" size="sm" className="w-28 shrink-0">
              <SelectValue />
            </SelectTrigger>
            <SelectContent>
              {settings.holdOptions.map((seconds) => (
                <SelectItem key={seconds} value={String(seconds)}>
                  {formatSpan(seconds)}
                </SelectItem>
              ))}
            </SelectContent>
          </Select>
        </SettingRow>

        <SettingRow id="temp-gate" label="Temperature gate" hint="Ignore motion when the room is cooler than the gate (Auto mode)">
          <Switch
            id="temp-gate"
            checked={settings.tempGateOn}
            disabled={!auto}
            onCheckedChange={(tempGateOn) => onSettings({ tempGateOn })}
          />
        </SettingRow>

        {settings.tempGateOn && (
          <SliderRow
            label="Gate temperature"
            value={settings.gateTempC}
            format={(value) => formatTemp(value, settings.fahrenheit)}
            min={10}
            max={40}
            step={0.5}
            disabled={!auto}
            onCommit={(gateTempC) => onSettings({ gateTempC })}
          />
        )}
      </CardContent>
    </Card>
  )
}
