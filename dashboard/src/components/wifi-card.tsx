import { useRef, useState, type FormEvent } from "react"
import { LockIcon, RefreshCwIcon, WifiHighIcon, WifiIcon, WifiLowIcon, WifiOffIcon } from "lucide-react"
import { toast } from "sonner"

import { InfoRow } from "@/components/setting-row"
import { Badge } from "@/components/ui/badge"
import { Button } from "@/components/ui/button"
import { Card, CardAction, CardContent, CardDescription, CardFooter, CardHeader, CardTitle } from "@/components/ui/card"
import {
  Dialog,
  DialogContent,
  DialogDescription,
  DialogFooter,
  DialogHeader,
  DialogTitle,
  DialogTrigger,
} from "@/components/ui/dialog"
import { Input } from "@/components/ui/input"
import { Label } from "@/components/ui/label"
import { Select, SelectContent, SelectItem, SelectTrigger, SelectValue } from "@/components/ui/select"
import { Switch } from "@/components/ui/switch"
import { api, describeError, type StationStatus, type WifiNetwork } from "@/lib/api"
import { signalQuality } from "@/lib/format"

const SCAN_POLL_MS = 1000
const SCAN_MAX_POLLS = 15

const STATE_LABELS = { connected: "Connected", connecting: "Connecting", hotspot: "Hotspot" } as const
const SIGNAL_ICONS = [WifiOffIcon, WifiLowIcon, WifiHighIcon, WifiIcon, WifiIcon]

function Link({ href }: { href: string }) {
  return (
    <a className="text-primary underline-offset-4 hover:underline" href={`http://${href}`}>
      {href}
    </a>
  )
}

// Strongest access point per network name, without hidden networks.
function uniqueNetworks(networks: WifiNetwork[]): WifiNetwork[] {
  const best = new Map<string, WifiNetwork>()
  for (const network of networks) {
    const known = best.get(network.ssid)
    if (network.ssid && (!known || network.rssi > known.rssi)) best.set(network.ssid, network)
  }
  return [...best.values()].sort((a, b) => b.rssi - a.rssi)
}

function validate(ssid: string, password: string, useLogin: boolean, username: string): string | null {
  const ssidBytes = new TextEncoder().encode(ssid).length
  if (ssidBytes === 0 || ssidBytes > 32) return "The network name must be 1-32 characters."
  if (useLogin) {
    if (!username) return "Enter the username for this network."
    if (!password) return "Enter the password for this login."
    return null
  }
  if (password && (password.length < 8 || password.length > 63)) return "Wi-Fi passwords are 8-63 characters."
  return null
}

function WifiDialog({ saved }: { saved: boolean }) {
  const [open, setOpen] = useState(false)
  const [networks, setNetworks] = useState<WifiNetwork[]>([])
  const [scanning, setScanning] = useState(false)
  const [ssid, setSsid] = useState("")
  const [password, setPassword] = useState("")
  const [useLogin, setUseLogin] = useState(false)
  const [username, setUsername] = useState("")
  const [identity, setIdentity] = useState("")
  const [error, setError] = useState<string | null>(null)
  const [busy, setBusy] = useState(false)
  const scanId = useRef(0)

  async function scan() {
    const id = ++scanId.current
    setScanning(true)
    try {
      for (let poll = 0; poll < SCAN_MAX_POLLS && id === scanId.current; poll++) {
        const result = await api.scanWifi()
        if (!result.scanning) {
          if (id === scanId.current) setNetworks(uniqueNetworks(result.networks ?? []))
          return
        }
        await new Promise((resolve) => setTimeout(resolve, SCAN_POLL_MS))
      }
      if (id === scanId.current) toast.error("The scan didn't finish. You can type the network name instead.")
    } catch (scanError) {
      toast.error(describeError(scanError))
    } finally {
      if (id === scanId.current) setScanning(false)
    }
  }

  function handleOpenChange(next: boolean) {
    setOpen(next)
    if (next) {
      setSsid("")
      setPassword("")
      setUseLogin(false)
      setUsername("")
      setIdentity("")
      setError(null)
      void scan()
    } else {
      scanId.current++   // abandons a scan still in progress
      setScanning(false)
    }
  }

  function pickNetwork(name: string) {
    setSsid(name)
    setUseLogin(networks.find((network) => network.ssid === name)?.enterprise ?? false)
  }

  async function save(event: FormEvent) {
    event.preventDefault()
    const problem = validate(ssid, password, useLogin, username)
    setError(problem)
    if (problem) return
    setBusy(true)
    try {
      await api.saveWifi({
        ssid,
        password,
        username: useLogin ? username : "",
        identity: useLogin ? identity : "",
      })
      toast.success(`Saved. The station is joining "${ssid}". Its new address appears on the OLED's Wi-Fi page.`)
      handleOpenChange(false)
    } catch (saveError) {
      setError(describeError(saveError))
    } finally {
      setBusy(false)
    }
  }

  async function forget() {
    setBusy(true)
    try {
      await api.forgetWifi()
      toast.success("Saved network forgotten. The station falls back to secrets.h or its hotspot.")
      handleOpenChange(false)
    } catch (forgetError) {
      setError(describeError(forgetError))
    } finally {
      setBusy(false)
    }
  }

  return (
    <Dialog open={open} onOpenChange={handleOpenChange}>
      <DialogTrigger asChild>
        <Button variant="outline" size="sm">
          Change network
        </Button>
      </DialogTrigger>
      <DialogContent>
        <form onSubmit={save} className="space-y-4">
          <DialogHeader>
            <DialogTitle>Wi-Fi network</DialogTitle>
            <DialogDescription>
              Saved on the station. It drops this connection and joins the new network right away.
            </DialogDescription>
          </DialogHeader>

          <div className="space-y-2">
            <Label htmlFor="wifi-scan">Nearby networks</Label>
            <div className="flex gap-2">
              <Select value={networks.some((network) => network.ssid === ssid) ? ssid : ""} onValueChange={pickNetwork}>
                <SelectTrigger id="wifi-scan" className="min-w-0 flex-1">
                  <SelectValue placeholder={scanning ? "Scanning..." : "Choose a network"} />
                </SelectTrigger>
                <SelectContent>
                  {networks.map((network) => (
                    <SelectItem key={network.ssid} value={network.ssid}>
                      {network.secure && <LockIcon aria-label="Secured" />}
                      {network.ssid}
                      <span className="text-xs text-muted-foreground">
                        {signalQuality(network.rssi).label}
                        {network.enterprise && ", username login"}
                      </span>
                    </SelectItem>
                  ))}
                </SelectContent>
              </Select>
              <Button type="button" variant="outline" size="icon" aria-label="Scan again" disabled={scanning} onClick={scan}>
                <RefreshCwIcon className={scanning ? "animate-spin" : undefined} />
              </Button>
            </div>
          </div>

          <div className="space-y-2">
            <Label htmlFor="wifi-ssid">Network name</Label>
            <Input id="wifi-ssid" value={ssid} autoComplete="off" onChange={(event) => setSsid(event.target.value)} />
          </div>

          <div className="flex items-center justify-between gap-4">
            <Label htmlFor="wifi-login">This network uses a username (eduroam, campus Wi-Fi)</Label>
            <Switch id="wifi-login" checked={useLogin} onCheckedChange={setUseLogin} />
          </div>

          {useLogin && (
            <>
              <div className="space-y-2">
                <Label htmlFor="wifi-username">Username</Label>
                <Input
                  id="wifi-username"
                  value={username}
                  autoComplete="off"
                  onChange={(event) => setUsername(event.target.value)}
                />
              </div>
              <div className="space-y-2">
                <Label htmlFor="wifi-identity">Outer identity (optional)</Label>
                <Input
                  id="wifi-identity"
                  value={identity}
                  autoComplete="off"
                  onChange={(event) => setIdentity(event.target.value)}
                />
              </div>
            </>
          )}

          <div className="space-y-2">
            <Label htmlFor="wifi-password">Password</Label>
            <Input
              id="wifi-password"
              type="password"
              value={password}
              autoComplete="new-password"
              onChange={(event) => setPassword(event.target.value)}
            />
          </div>

          {useLogin && (
            <p className="text-xs text-muted-foreground">
              The station stores this login unencrypted on its chip and can't verify the network's certificate. Don't
              use your main university password: ask IT for a device login or use another network.
            </p>
          )}

          {error && (
            <p role="alert" className="text-sm text-destructive">
              {error}
            </p>
          )}

          <DialogFooter>
            {saved && (
              <Button type="button" variant="ghost" className="sm:mr-auto" disabled={busy} onClick={forget}>
                Forget saved network
              </Button>
            )}
            <Button type="submit" disabled={busy}>
              Save and connect
            </Button>
          </DialogFooter>
        </form>
      </DialogContent>
    </Dialog>
  )
}

export function WifiCard({ wifi }: { wifi: StationStatus["wifi"] }) {
  const signal = signalQuality(wifi.rssi)
  const SignalIcon = SIGNAL_ICONS[wifi.state === "connected" ? signal.bars : 0]

  return (
    <Card>
      <CardHeader>
        <CardTitle>Wi-Fi</CardTitle>
        <CardDescription>
          {wifi.saved ? "Network saved from this dashboard" : wifi.ssid ? "Network from secrets.h" : "No network set yet"}
        </CardDescription>
        <CardAction>
          <Badge variant={wifi.state === "connected" ? "default" : "outline"}>{STATE_LABELS[wifi.state]}</Badge>
        </CardAction>
      </CardHeader>
      <CardContent className="flex-1">
        <dl className="space-y-2">
          {wifi.ssid && <InfoRow label="Network">{wifi.ssid}</InfoRow>}
          {wifi.state === "connected" && (
            <>
              <InfoRow label="Address">
                <Link href={wifi.ip} />
              </InfoRow>
              <InfoRow label="Name">
                <Link href={`${wifi.hostname}.local`} />
              </InfoRow>
              <InfoRow label="Signal">
                <span className="inline-flex items-center gap-1.5">
                  <SignalIcon className="size-4" aria-hidden />
                  {signal.label}, {wifi.rssi} dBm
                </span>
              </InfoRow>
            </>
          )}
          {wifi.apActive && (
            <>
              <InfoRow label="Hotspot">{wifi.apSsid}</InfoRow>
              <InfoRow label="Hotspot address">
                <Link href={wifi.apIp} />
              </InfoRow>
              <InfoRow label="Hotspot clients">{wifi.apClients}</InfoRow>
            </>
          )}
        </dl>
        {wifi.error && (
          <p role="alert" className="mt-3 text-sm text-destructive">
            Couldn't join {wifi.ssid}: {wifi.error}
          </p>
        )}
      </CardContent>
      <CardFooter>
        <WifiDialog saved={wifi.saved} />
      </CardFooter>
    </Card>
  )
}
