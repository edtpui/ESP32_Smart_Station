import { SettingRow, SliderRow } from "@/components/setting-row"
import { Button } from "@/components/ui/button"
import { Card, CardContent, CardDescription, CardFooter, CardHeader, CardTitle } from "@/components/ui/card"
import { Separator } from "@/components/ui/separator"
import { Switch } from "@/components/ui/switch"
import type { SettingsUpdate, StationSettings } from "@/lib/api"
import { formatTemp } from "@/lib/format"

export function AlertsCard({
  settings,
  onSettings,
  onTestBuzzer,
}: {
  settings: StationSettings
  onSettings: (update: SettingsUpdate) => Promise<void>
  onTestBuzzer: () => void
}) {
  const muted = !settings.soundOn

  return (
    <Card>
      <CardHeader>
        <CardTitle>Sound and alerts</CardTitle>
        <CardDescription>What the buzzer beeps for.</CardDescription>
      </CardHeader>
      <CardContent className="flex-1 space-y-5">
        <SettingRow id="sound-on" label="Sound" hint="Off silences every beep, including alerts">
          <Switch id="sound-on" checked={settings.soundOn} onCheckedChange={(soundOn) => onSettings({ soundOn })} />
        </SettingRow>

        <SettingRow id="motion-chime" label="Motion beep" hint="One short beep for each new motion event">
          <Switch
            id="motion-chime"
            checked={settings.motionChime}
            disabled={muted}
            onCheckedChange={(motionChime) => onSettings({ motionChime })}
          />
        </SettingRow>

        <Separator />

        <SettingRow
          id="climate-alert"
          label="Climate alert"
          hint="Three beeps when the room gets too hot or too humid, then a reminder every 5 minutes"
        >
          <Switch
            id="climate-alert"
            checked={settings.climateAlert}
            onCheckedChange={(climateAlert) => onSettings({ climateAlert })}
          />
        </SettingRow>

        {settings.climateAlert && (
          <>
            <SliderRow
              label="Too hot at"
              value={settings.alertTempC}
              format={(value) => formatTemp(value, settings.fahrenheit)}
              min={15}
              max={45}
              step={0.5}
              onCommit={(alertTempC) => onSettings({ alertTempC })}
            />
            <SliderRow
              label="Too humid at"
              value={settings.alertHumidity}
              format={(value) => `${value} %`}
              min={30}
              max={95}
              step={5}
              onCommit={(alertHumidity) => onSettings({ alertHumidity })}
            />
          </>
        )}
      </CardContent>
      <CardFooter>
        <Button variant="outline" size="sm" onClick={onTestBuzzer}>
          Test buzzer
        </Button>
      </CardFooter>
    </Card>
  )
}
