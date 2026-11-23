import { SettingRow } from "@/components/setting-row"
import { Card, CardContent, CardDescription, CardHeader, CardTitle } from "@/components/ui/card"
import { Select, SelectContent, SelectItem, SelectTrigger, SelectValue } from "@/components/ui/select"
import { Switch } from "@/components/ui/switch"
import { ToggleGroup, ToggleGroupItem } from "@/components/ui/toggle-group"
import type { SettingsUpdate, StationSettings, StationStatus } from "@/lib/api"
import { formatSpan } from "@/lib/format"

const BRIGHTNESS_LEVELS = ["Low", "Medium", "High"]

export function DisplayCard({
  settings,
  light,
  onSettings,
}: {
  settings: StationSettings
  light: StationStatus["light"]
  onSettings: (update: SettingsUpdate) => Promise<void>
}) {
  return (
    <Card>
      <CardHeader>
        <CardTitle>Screen and units</CardTitle>
        <CardDescription>Applies to the OLED and to this page.</CardDescription>
      </CardHeader>
      <CardContent className="space-y-5">
        <SettingRow label="Temperature unit">
          <ToggleGroup
            type="single"
            variant="outline"
            size="sm"
            value={settings.fahrenheit ? "f" : "c"}
            aria-label="Temperature unit"
            onValueChange={(value) => value && onSettings({ fahrenheit: value === "f" })}
          >
            <ToggleGroupItem value="c">°C</ToggleGroupItem>
            <ToggleGroupItem value="f">°F</ToggleGroupItem>
          </ToggleGroup>
        </SettingRow>

        <SettingRow label="OLED brightness">
          <ToggleGroup
            type="single"
            variant="outline"
            size="sm"
            value={String(settings.brightness)}
            aria-label="OLED brightness"
            onValueChange={(value) => value && onSettings({ brightness: Number(value) })}
          >
            {BRIGHTNESS_LEVELS.map((name, level) => (
              <ToggleGroupItem key={name} value={String(level)}>
                {name}
              </ToggleGroupItem>
            ))}
          </ToggleGroup>
        </SettingRow>

        <SettingRow
          id="auto-dim"
          label="Dim in the dark"
          hint={
            settings.autoDim && light.dark
              ? "The room is dark, so the OLED is at Low now"
              : "Drops the OLED to Low when the light sensor reads dark"
          }
        >
          <Switch id="auto-dim" checked={settings.autoDim} onCheckedChange={(autoDim) => onSettings({ autoDim })} />
        </SettingRow>

        <SettingRow id="screen-sleep" label="OLED sleep" hint="Motion or the joystick wakes it">
          <Select
            value={String(settings.screenSleep)}
            onValueChange={(value) => onSettings({ screenSleep: Number(value) })}
          >
            <SelectTrigger id="screen-sleep" size="sm" className="w-28 shrink-0">
              <SelectValue />
            </SelectTrigger>
            <SelectContent>
              {settings.sleepOptions.map((seconds) => (
                <SelectItem key={seconds} value={String(seconds)}>
                  {seconds === 0 ? "Never" : formatSpan(seconds)}
                </SelectItem>
              ))}
            </SelectContent>
          </Select>
        </SettingRow>

        <SettingRow id="flip-display" label="Flip OLED" hint="For a board mounted upside down">
          <Switch
            id="flip-display"
            checked={settings.flipDisplay}
            onCheckedChange={(flipDisplay) => onSettings({ flipDisplay })}
          />
        </SettingRow>
      </CardContent>
    </Card>
  )
}
