import { useState, type ReactNode } from "react"

import { Label } from "@/components/ui/label"
import { Slider } from "@/components/ui/slider"

export function SettingRow({
  id,
  label,
  hint,
  children,
}: {
  id?: string
  label: string
  hint?: string
  children: ReactNode
}) {
  return (
    <div className="flex items-center justify-between gap-4">
      <div className="min-w-0 space-y-1">
        <Label htmlFor={id}>{label}</Label>
        {hint && <p className="text-xs text-muted-foreground">{hint}</p>}
      </div>
      {children}
    </div>
  )
}

export function InfoRow({ label, children }: { label: string; children: ReactNode }) {
  return (
    <div className="flex items-baseline justify-between gap-4 text-sm">
      <dt className="shrink-0 text-muted-foreground">{label}</dt>
      <dd className="min-w-0 truncate text-right font-medium tabular-nums">{children}</dd>
    </div>
  )
}

// Follows the finger locally and only talks to the ESP32 when it is let go.
export function SliderRow({
  label,
  value,
  format,
  min,
  max,
  step,
  disabled,
  onCommit,
}: {
  label: string
  value: number
  format: (value: number) => string
  min: number
  max: number
  step: number
  disabled?: boolean
  onCommit: (value: number) => Promise<void>
}) {
  const [dragging, setDragging] = useState<number | null>(null)
  const shown = dragging ?? value

  return (
    <div className="space-y-3">
      <div className="flex items-center justify-between gap-4">
        <span className="text-sm leading-none font-medium">{label}</span>
        <span className="text-sm text-muted-foreground tabular-nums">{format(shown)}</span>
      </div>
      <Slider
        aria-label={label}
        value={[shown]}
        min={min}
        max={max}
        step={step}
        disabled={disabled}
        onValueChange={([next]) => setDragging(next)}
        onValueCommit={async ([next]) => {
          await onCommit(next)
          setDragging(null)
        }}
      />
    </div>
  )
}
