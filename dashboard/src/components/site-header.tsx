import { FanIcon, MoonIcon, SunIcon } from "lucide-react"
import { useTheme } from "next-themes"

import { Badge } from "@/components/ui/badge"
import { Button } from "@/components/ui/button"
import type { Connection } from "@/hooks/use-station"
import type { StationStatus } from "@/lib/api"
import { formatClock } from "@/lib/format"

export function SiteHeader({ status, connection }: { status: StationStatus; connection: Connection }) {
  const { resolvedTheme, setTheme } = useTheme()
  const dark = resolvedTheme === "dark"

  return (
    <header className="border-b bg-card">
      <div className="mx-auto flex max-w-6xl items-center gap-3 px-4 py-2.5">
        <FanIcon className="size-5 shrink-0 text-primary" aria-hidden />
        <h1 className="truncate font-semibold">{status.name}</h1>
        <Badge variant={connection === "offline" ? "destructive" : "secondary"}>
          {connection === "offline" ? "Offline" : "Live"}
        </Badge>
        <span className="ml-auto text-xs whitespace-nowrap text-muted-foreground tabular-nums">
          Up {formatClock(status.uptime)}
        </span>
        <Button
          variant="ghost"
          size="icon-sm"
          aria-label={dark ? "Switch to light theme" : "Switch to dark theme"}
          onClick={() => setTheme(dark ? "light" : "dark")}
        >
          {dark ? <SunIcon /> : <MoonIcon />}
        </Button>
      </div>
    </header>
  )
}
