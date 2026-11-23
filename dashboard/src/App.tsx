import { AlertsCard } from "@/components/alerts-card"
import { ClimateChart } from "@/components/climate-chart"
import { DisplayCard } from "@/components/display-card"
import { FanCard } from "@/components/fan-card"
import { SiteHeader } from "@/components/site-header"
import { StatusCards } from "@/components/status-cards"
import { SystemCard } from "@/components/system-card"
import { Skeleton } from "@/components/ui/skeleton"
import { Toaster } from "@/components/ui/sonner"
import { WifiCard } from "@/components/wifi-card"
import { useStation } from "@/hooks/use-station"
import { api, type FanUpdate, type SettingsUpdate } from "@/lib/api"

export default function App() {
  const { status, history, connection, change } = useStation()

  if (!status) {
    return (
      <main className="mx-auto grid max-w-6xl gap-4 p-4">
        {connection === "offline" ? (
          <p role="alert" className="text-sm text-destructive">
            Can't reach the station. Check that it has power and that this device is on the same network.
          </p>
        ) : (
          <div role="status" aria-label="Loading" className="grid gap-4 sm:grid-cols-2 lg:grid-cols-5">
            {[0, 1, 2, 3, 4].map((card) => (
              <Skeleton key={card} className="h-28" />
            ))}
          </div>
        )}
      </main>
    )
  }

  const setFan = (update: FanUpdate) => change(() => api.setFan(update))
  const setSettings = (update: SettingsUpdate) => change(() => api.setSettings(update))

  return (
    <>
      <SiteHeader status={status} connection={connection} />
      <main className="mx-auto grid max-w-6xl gap-4 p-4">
        {connection === "offline" && (
          <p role="alert" className="rounded-md border border-destructive/50 px-3 py-2 text-sm text-destructive">
            Connection lost, these readings are out of date. If you just changed networks, find the station's new
            address on the OLED's Wi-Fi page.
          </p>
        )}
        <StatusCards status={status} />
        <div className="grid items-start gap-4 lg:grid-cols-3">
          <ClimateChart history={history} fahrenheit={status.settings.fahrenheit} className="lg:col-span-2" />
          <FanCard settings={status.settings} onFan={setFan} onSettings={setSettings} />
        </div>
        <div className="grid gap-4 md:grid-cols-2">
          <AlertsCard
            settings={status.settings}
            onSettings={setSettings}
            onTestBuzzer={() => change(api.testBuzzer, "Buzzer test sent")}
          />
          <DisplayCard settings={status.settings} light={status.light} onSettings={setSettings} />
          <WifiCard wifi={status.wifi} />
          <SystemCard
            status={status}
            onResetStats={() => change(api.resetStats, "Stats cleared")}
            onFactoryReset={() => change(api.factoryReset, "Default settings restored")}
          />
        </div>
      </main>
      {/* Top centre: the default bottom-right spot covers the System card's buttons. */}
      <Toaster position="top-center" />
    </>
  )
}
